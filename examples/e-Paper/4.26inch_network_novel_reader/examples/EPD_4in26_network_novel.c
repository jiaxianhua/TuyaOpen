/**
 * @file EPD_4in26_network_novel.c
 * @brief 4.26inch E-Paper Network Novel Reader Implementation
 * 
 * Features:
 * - Fetch novels from HTTP URLs
 * - Display text with pagination
 * - Button control: Short press = Next page, Long press = Previous page
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "EPD_Test.h"
#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "http_client_interface.h"
#include "netmgr.h"
#include "netconn_wifi.h"
#include "tdl_button_manage.h"
#include "tdd_button_gpio.h"
#include "board_button_config.h"
#include "embedded_novel.h"
#include "hzk16.h"
#include <string.h>
#include <stdlib.h>

/***********************************************************
************************macro define************************
***********************************************************/
// WiFi Configuration - CHANGE THESE TO YOUR WIFI CREDENTIALS
#define WIFI_SSID "1519"
#define WIFI_PASSWORD "15889629702"

// Novel URL - CHANGE THIS TO YOUR NOVEL URL
#define NOVEL_URL "http://120.79.89.230/fanren.txt"

// Display settings
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define CHARS_PER_LINE 40  // Reduced for Chinese characters
#define LINES_PER_PAGE 22  // Adjusted for better display
#define BYTES_PER_PAGE 2000  // For GBK encoding (variable byte length)

// Button settings
#define BUTTON_LONG_PRESS_TIME 3000  // 3 seconds

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    char *content;
    int content_size;
    int current_page;
    int total_pages;
    int network_connected;
    volatile int button_event;  // 1=next, -1=prev, 0=none
    int content_loaded;         // 内容是否已加载
    int *page_offsets;          // Array of page start offsets
    int page_offsets_count;     // Number of pages
} novel_reader_ctx_t;

/***********************************************************
***********************variable define**********************
***********************************************************/
static novel_reader_ctx_t g_reader_ctx = {0};
static UBYTE *g_image_buffer = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Network status callback
 */
static OPERATE_RET network_status_cb(void *data)
{
    netmgr_status_e status = (netmgr_status_e)data;
    
    if (status == NETMGR_LINK_UP) {
        PR_NOTICE("Network connected");
        g_reader_ctx.network_connected = 1;
    } else {
        PR_NOTICE("Network disconnected");
        g_reader_ctx.network_connected = 0;
    }
    
    return OPRT_OK;
}

/**
 * @brief Initialize network
 */
static int init_network(void)
{
    OPERATE_RET rt = OPRT_OK;
    
    PR_NOTICE("Initializing network...");
    
    // Initialize KV storage
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    
    tal_sw_timer_init();
    tal_workq_init();
    
    // Subscribe to network events
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "novel_reader", network_status_cb, SUBSCRIBE_TYPE_NORMAL);
    
#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
    TUYA_LwIP_Init();
#endif
    
    // Initialize network manager
    rt = netmgr_init(NETCONN_WIFI);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to init network: %d", rt);
        return rt;
    }
    
    // Connect to WiFi
    netconn_wifi_info_t wifi_info = {0};
    strncpy(wifi_info.ssid, WIFI_SSID, sizeof(wifi_info.ssid) - 1);
    strncpy(wifi_info.pswd, WIFI_PASSWORD, sizeof(wifi_info.pswd) - 1);
    
    PR_NOTICE("Connecting to WiFi: %s", WIFI_SSID);
    rt = netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to connect WiFi: %d", rt);
        return rt;
    }
    
    // Wait for connection
    int timeout = 30;
    while (timeout > 0 && !g_reader_ctx.network_connected) {
        tal_system_sleep(1000);
        timeout--;
    }
    
    if (!g_reader_ctx.network_connected) {
        PR_ERR("WiFi connection timeout");
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Network initialized successfully");
    return OPRT_OK;
}

/**
 * @brief Check if byte is GBK lead byte
 */
static int is_gbk_lead_byte(unsigned char c)
{
    return (c >= 0x81 && c <= 0xFE);
}

/**
 * @brief Get character byte length (1 for ASCII, 2 for GBK)
 */
static int get_char_byte_len(const char *str, int pos, int max_len)
{
    if (pos >= max_len) return 0;
    
    unsigned char c = (unsigned char)str[pos];
    
    // ASCII character
    if (c < 0x80) {
        return 1;
    }
    
    // GBK character (2 bytes)
    if (is_gbk_lead_byte(c) && pos + 1 < max_len) {
        unsigned char c2 = (unsigned char)str[pos + 1];
        if (c2 >= 0x40 && c2 <= 0xFE && c2 != 0x7F) {
            return 2;
        }
    }
    
    return 1;  // Fallback to single byte
}

/**
 * @brief Calculate page offsets for content
 */
static int calculate_page_offsets(void)
{
    if (!g_reader_ctx.content || g_reader_ctx.content_size <= 0) {
        return OPRT_INVALID_PARM;
    }
    
    // Estimate max pages
    int max_pages = (g_reader_ctx.content_size / 500) + 100;
    g_reader_ctx.page_offsets = (int *)tal_malloc(max_pages * sizeof(int));
    if (!g_reader_ctx.page_offsets) {
        return OPRT_MALLOC_FAILED;
    }
    
    int page_count = 0;
    int pos = 0;
    int content_len = g_reader_ctx.content_size;
    
    while (pos < content_len && page_count < max_pages) {
        g_reader_ctx.page_offsets[page_count++] = pos;
        
        int line_count = 0;
        int page_start = pos;
        
        // Process lines for this page
        while (line_count < LINES_PER_PAGE && pos < content_len) {
            int line_width = 0;
            int line_start = pos;
            
            // Process characters in this line
            while (pos < content_len && line_width < CHARS_PER_LINE) {
                unsigned char c = (unsigned char)g_reader_ctx.content[pos];
                
                // Skip newline
                if (c == '\n' || c == '\r') {
                    pos++;
                    if (pos < content_len && c == '\r' && g_reader_ctx.content[pos] == '\n') {
                        pos++;  // Skip \r\n
                    }
                    break;
                }
                
                // Skip control characters
                if (c < 0x20 && c != '\t') {
                    pos++;
                    continue;
                }
                
                int char_len = get_char_byte_len(g_reader_ctx.content, pos, content_len);
                
                // Check if character fits in line
                int char_width = (char_len == 2) ? 2 : 1;  // GBK takes 2 units, ASCII takes 1
                
                if (line_width + char_width > CHARS_PER_LINE && line_width > 0) {
                    break;  // Line full, move to next line
                }
                
                pos += char_len;
                line_width += char_width;
            }
            
            line_count++;
        }
        
        // Ensure we made progress
        if (pos == page_start) {
            pos++;  // Force progress to avoid infinite loop
        }
    }
    
    g_reader_ctx.page_offsets_count = page_count;
    g_reader_ctx.total_pages = page_count;
    
    PR_NOTICE("Calculated %d pages", page_count);
    
    return OPRT_OK;
}
/**
 * @brief Load embedded novel content
 */
static int load_embedded_novel(void)
{
    PR_NOTICE("Loading embedded novel...");
    
    // 使用内置的小说数据
    int content_size = embedded_novel_data_size;
    
    g_reader_ctx.content = (char *)tal_malloc(content_size + 1);
    if (!g_reader_ctx.content) {
        PR_ERR("Memory allocation failed");
        return OPRT_MALLOC_FAILED;
    }
    
    // 复制内容
    memcpy(g_reader_ctx.content, embedded_novel_data, content_size);
    g_reader_ctx.content[content_size] = '\0';
    g_reader_ctx.content_size = content_size;
    
    // 计算分页
    int rt = calculate_page_offsets();
    if (rt != OPRT_OK) {
        PR_ERR("Failed to calculate pages");
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
        return rt;
    }
    
    g_reader_ctx.current_page = 0;
    g_reader_ctx.content_loaded = 1;
    
    PR_NOTICE("Loaded %d bytes, %d pages", content_size, g_reader_ctx.total_pages);
    
    return OPRT_OK;
}

/**
 * @brief Fetch novel from URL
 */
static int fetch_novel(const char *url)
{
    if (!g_reader_ctx.network_connected) {
        PR_ERR("Network not connected");
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Fetching novel from: %s", url);
    
    // Parse URL
    char host[256] = {0};
    char path[512] = {0};
    const char *url_ptr = url;
    
    if (strncmp(url, "http://", 7) == 0) {
        url_ptr += 7;
    }
    
    const char *slash = strchr(url_ptr, '/');
    if (slash) {
        int host_len = slash - url_ptr;
        if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
        strncpy(host, url_ptr, host_len);
        strncpy(path, slash, sizeof(path) - 1);
    } else {
        strncpy(host, url_ptr, sizeof(host) - 1);
        strcpy(path, "/");
    }
    
    PR_DEBUG("Host: %s, Path: %s", host, path);
    
    // HTTP request
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "TuyaOpen-NovelReader/1.0"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = host,
            .method = "GET",
            .path = path,
            .headers = headers,
            .headers_count = 1,
            .body = "",
            .body_length = 0,
            .timeout_ms = 30000
        },
        &http_response
    );
    
    if (http_status != HTTP_CLIENT_SUCCESS) {
        PR_ERR("HTTP request failed: %d", http_status);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    if (http_response.status_code != 200) {
        PR_ERR("HTTP status: %d", http_response.status_code);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    // Allocate memory for content
    int body_len = http_response.body_length;
    if (body_len <= 0 || body_len > 1024 * 1024) {
        PR_ERR("Invalid content size: %d", body_len);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    g_reader_ctx.content = (char *)tal_malloc(body_len + 1);
    if (!g_reader_ctx.content) {
        PR_ERR("Memory allocation failed");
        http_client_free(&http_response);
        return OPRT_MALLOC_FAILED;
    }
    
    memcpy(g_reader_ctx.content, http_response.body, body_len);
    g_reader_ctx.content[body_len] = '\0';
    g_reader_ctx.content_size = body_len;
    
    // Calculate page offsets
    int rt = calculate_page_offsets();
    if (rt != OPRT_OK) {
        PR_ERR("Failed to calculate pages");
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
        http_client_free(&http_response);
        return rt;
    }
    
    g_reader_ctx.current_page = 0;
    
    PR_NOTICE("Fetched %d bytes, %d pages", body_len, g_reader_ctx.total_pages);
    
    http_client_free(&http_response);
    return OPRT_OK;
}

/**
 * @brief Draw a GBK Chinese character using HZK16 font
 */
static void draw_gbk_char(int x, int y, unsigned char gb_high, unsigned char gb_low, 
                          UWORD fg_color, UWORD bg_color)
{
    uint8_t font_data[32];
    
    // Get font data from HZK16
    int ret = hzk16_get_font_data(gb_high, gb_low, font_data);
    
    if (ret != 0) {
        // Character not found, draw placeholder
        Paint_DrawString_EN(x, y, "[]", &Font16, fg_color, bg_color);
        return;
    }
    
    // Draw 16x16 bitmap
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            int byte_idx = row * 2 + col / 8;
            int bit_idx = 7 - (col % 8);
            
            if (font_data[byte_idx] & (1 << bit_idx)) {
                Paint_SetPixel(x + col, y + row, fg_color);
            } else {
                Paint_SetPixel(x + col, y + row, bg_color);
            }
        }
    }
}

/**
 * @brief Display current page
 */
static void display_page(void)
{
    if (!g_reader_ctx.content || !g_reader_ctx.page_offsets) {
        PR_ERR("No content to display");
        return;
    }
    
    if (g_reader_ctx.current_page >= g_reader_ctx.total_pages) {
        PR_ERR("Invalid page number");
        return;
    }
    
    PR_NOTICE("Displaying page %d/%d", g_reader_ctx.current_page + 1, g_reader_ctx.total_pages);
    
    // Clear buffer
    Paint_Clear(WHITE);
    
    // Get current time
    TIME_T current_time = tal_time_get_posix();
    POSIX_TM_S tm_info;
    tal_time_gmtime_r(&current_time, &tm_info);
    
    // Draw time at top right (HH:MM)
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", tm_info.tm_hour, tm_info.tm_min);
    Paint_DrawString_EN(DISPLAY_WIDTH - 80, 10, time_str, &Font16, BLACK, WHITE);
    
    // Draw page info at top left
    char page_info[64];
    snprintf(page_info, sizeof(page_info), "Page %d/%d", 
             g_reader_ctx.current_page + 1, g_reader_ctx.total_pages);
    Paint_DrawString_EN(10, 10, page_info, &Font16, BLACK, WHITE);
    
    // Get page start position
    int page_start = g_reader_ctx.page_offsets[g_reader_ctx.current_page];
    int page_end = (g_reader_ctx.current_page + 1 < g_reader_ctx.total_pages) 
                   ? g_reader_ctx.page_offsets[g_reader_ctx.current_page + 1] 
                   : g_reader_ctx.content_size;
    
    // Draw content line by line
    int y_pos = 40;
    int pos = page_start;
    int line_count = 0;
    
    while (line_count < LINES_PER_PAGE && pos < page_end && pos < g_reader_ctx.content_size) {
        char line_buf[256] = {0};
        int line_len = 0;
        int line_width = 0;
        int line_start = pos;
        
        // Extract one line
        while (pos < page_end && pos < g_reader_ctx.content_size && line_width < CHARS_PER_LINE) {
            unsigned char c = (unsigned char)g_reader_ctx.content[pos];
            
            // Handle newline
            if (c == '\n' || c == '\r') {
                pos++;
                if (pos < g_reader_ctx.content_size && c == '\r' && g_reader_ctx.content[pos] == '\n') {
                    pos++;
                }
                break;
            }
            
            // Skip control characters
            if (c < 0x20 && c != '\t') {
                pos++;
                continue;
            }
            
            int char_len = get_char_byte_len(g_reader_ctx.content, pos, g_reader_ctx.content_size);
            int char_width = (char_len == 2) ? 2 : 1;
            
            // Check if character fits
            if (line_width + char_width > CHARS_PER_LINE && line_width > 0) {
                break;
            }
            
            // Copy character to line buffer
            if (line_len + char_len < sizeof(line_buf) - 1) {
                for (int i = 0; i < char_len; i++) {
                    line_buf[line_len++] = g_reader_ctx.content[pos++];
                }
                line_width += char_width;
            } else {
                break;
            }
        }
        
        // Draw the line
        if (line_len > 0) {
            line_buf[line_len] = '\0';
            
            // Draw ASCII/GBK mixed text
            int x_pos = 10;
            int i = 0;
            while (i < line_len) {
                unsigned char c = (unsigned char)line_buf[i];
                
                if (c < 0x80) {
                    // ASCII character
                    char ascii_str[2] = {line_buf[i], '\0'};
                    Paint_DrawString_EN(x_pos, y_pos, ascii_str, &Font16, BLACK, WHITE);
                    x_pos += 8;  // ASCII width
                    i++;
                } else if (is_gbk_lead_byte(c) && i + 1 < line_len) {
                    // GBK character - use HZK16 font
                    unsigned char gb_high = (unsigned char)line_buf[i];
                    unsigned char gb_low = (unsigned char)line_buf[i + 1];
                    draw_gbk_char(x_pos, y_pos, gb_high, gb_low, BLACK, WHITE);
                    x_pos += 16;  // GBK width (16 pixels)
                    i += 2;
                } else {
                    // Unknown character
                    Paint_DrawString_EN(x_pos, y_pos, "?", &Font16, BLACK, WHITE);
                    x_pos += 8;
                    i++;
                }
            }
        }
        
        y_pos += 20;
        line_count++;
    }
    
    // Update display with fast refresh (no black flash)
    EPD_4in26_Display_Fast(g_image_buffer);
}

/**
 * @brief Button event handler
 */
static void button_handler(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *arg)
{
    PR_DEBUG("Button: %s, Event: %d", name, event);
    
    if (event == TDL_BUTTON_PRESS_DOWN) {
        // Short press = Next page
        g_reader_ctx.button_event = 1;
    } else if (event == TDL_BUTTON_LONG_PRESS_START) {
        // Long press = Previous page
        g_reader_ctx.button_event = -1;
    }
}

/**
 * @brief Initialize button
 */
static int init_button(void)
{
    PR_NOTICE("Initializing button...");
    
    // Register button hardware first
    BUTTON_GPIO_CFG_T button_hw_cfg;
    memset(&button_hw_cfg, 0, sizeof(BUTTON_GPIO_CFG_T));
    
    button_hw_cfg.pin = BOARD_BUTTON_PIN;
    button_hw_cfg.level = BOARD_BUTTON_ACTIVE_LV;
    button_hw_cfg.mode = BUTTON_TIMER_SCAN_MODE;
    button_hw_cfg.pin_type.gpio_pull = TUYA_GPIO_PULLUP;
    
    OPERATE_RET rt = tdd_gpio_button_register(BUTTON_NAME, &button_hw_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to register button hardware: %d", rt);
        return rt;
    }
    
    PR_DEBUG("Button hardware registered on GPIO %d", BOARD_BUTTON_PIN);
    
    // Create button with TDL
    TDL_BUTTON_CFG_T button_cfg = {
        .long_start_valid_time = BUTTON_LONG_PRESS_TIME,
        .long_keep_timer = 1000,
        .button_debounce_time = 50,
        .button_repeat_valid_count = 2,
        .button_repeat_valid_time = 500
    };
    
    TDL_BUTTON_HANDLE button_hdl = NULL;
    rt = tdl_button_create(BUTTON_NAME, &button_cfg, &button_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create button: %d", rt);
        return rt;
    }
    
    tdl_button_event_register(button_hdl, TDL_BUTTON_PRESS_DOWN, button_handler);
    tdl_button_event_register(button_hdl, TDL_BUTTON_LONG_PRESS_START, button_handler);
    
    PR_NOTICE("Button initialized successfully");
    return OPRT_OK;
}

/**
 * @brief Main test function
 */
void EPD_network_novel_test(void)
{
    PR_NOTICE("========================================");
    PR_NOTICE("E-Paper Network Novel Reader");
    PR_NOTICE("========================================");
    
    // Initialize hardware
    PR_NOTICE("Initializing hardware...");
    if (DEV_Module_Init() != 0) {
        PR_ERR("Hardware init failed");
        return;
    }
    
    // Initialize e-Paper
    PR_NOTICE("Initializing e-Paper...");
    EPD_4in26_Init();
    EPD_4in26_Clear();
    DEV_Delay_ms(500);
    
    // Allocate image buffer
    UWORD image_size = ((DISPLAY_WIDTH % 8 == 0) ? (DISPLAY_WIDTH / 8) : (DISPLAY_WIDTH / 8 + 1)) * DISPLAY_HEIGHT;
    g_image_buffer = (UBYTE *)tal_malloc(image_size);
    if (!g_image_buffer) {
        PR_ERR("Failed to allocate image buffer");
        goto cleanup;
    }
    
    // Portrait mode: rotate 90 degrees (竖屏模式)
    Paint_NewImage(g_image_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, ROTATE_90, WHITE);
    Paint_SelectImage(g_image_buffer);
    Paint_Clear(WHITE);
    
    // Initialize base image for fast refresh
    EPD_4in26_Display_Base(g_image_buffer);
    DEV_Delay_ms(500);
    
    // Show "Connecting..." message
    Paint_DrawString_EN(150, 300, "Connecting to WiFi...", &Font24, BLACK, WHITE);
    EPD_4in26_Display_Fast(g_image_buffer);
    
    // Initialize network
    int network_ok = (init_network() == OPRT_OK);
    
    if (network_ok) {
        // Show "Downloading..." message
        Paint_Clear(WHITE);
        Paint_DrawString_EN(120, 300, "Downloading novel...", &Font24, BLACK, WHITE);
        EPD_4in26_Display_Fast(g_image_buffer);
        
        // Try to fetch novel from network
        if (fetch_novel(NOVEL_URL) != OPRT_OK) {
            PR_WARN("Network download failed, loading embedded novel...");
            Paint_Clear(WHITE);
            Paint_DrawString_EN(120, 300, "Download Failed!", &Font24, BLACK, WHITE);
            Paint_DrawString_EN(60, 350, "Loading embedded novel...", &Font20, BLACK, WHITE);
            EPD_4in26_Display_Fast(g_image_buffer);
            tal_system_sleep(1000);
            
            // Fallback to embedded novel
            if (load_embedded_novel() != OPRT_OK) {
                Paint_Clear(WHITE);
                Paint_DrawString_EN(150, 300, "Load Failed!", &Font24, BLACK, WHITE);
                EPD_4in26_Display_Fast(g_image_buffer);
                goto cleanup;
            }
        }
    } else {
        // No network, use embedded novel
        PR_NOTICE("No network, loading embedded novel...");
        Paint_Clear(WHITE);
        Paint_DrawString_EN(100, 300, "No WiFi Connection", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(60, 350, "Loading embedded novel...", &Font20, BLACK, WHITE);
        EPD_4in26_Display_Fast(g_image_buffer);
        tal_system_sleep(1000);
        
        if (load_embedded_novel() != OPRT_OK) {
            Paint_Clear(WHITE);
            Paint_DrawString_EN(150, 300, "Load Failed!", &Font24, BLACK, WHITE);
            EPD_4in26_Display_Fast(g_image_buffer);
            goto cleanup;
        }
    }
    
    g_reader_ctx.content_loaded = 1;
    
    // Initialize button
    if (init_button() != OPRT_OK) {
        PR_WARN("Button init failed, continuing without button control");
    }
    
    // Display first page
    display_page();
    
    // Main loop - handle button events
    PR_NOTICE("Entering main loop. Use button to navigate:");
    PR_NOTICE("  Short press = Next page");
    PR_NOTICE("  Long press = Previous page");
    
    while (1) {
        if (g_reader_ctx.button_event != 0) {
            if (g_reader_ctx.button_event == 1) {
                // Next page
                if (g_reader_ctx.current_page < g_reader_ctx.total_pages - 1) {
                    g_reader_ctx.current_page++;
                    display_page();
                } else {
                    PR_NOTICE("Already at last page");
                }
            } else if (g_reader_ctx.button_event == -1) {
                // Previous page
                if (g_reader_ctx.current_page > 0) {
                    g_reader_ctx.current_page--;
                    display_page();
                } else {
                    PR_NOTICE("Already at first page");
                }
            }
            g_reader_ctx.button_event = 0;
        }
        
        tal_system_sleep(100);
    }
    
cleanup:
    PR_NOTICE("Cleaning up...");
    
    if (g_reader_ctx.content) {
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
    }
    
    if (g_reader_ctx.page_offsets) {
        tal_free(g_reader_ctx.page_offsets);
        g_reader_ctx.page_offsets = NULL;
    }
    
    if (g_image_buffer) {
        tal_free(g_image_buffer);
        g_image_buffer = NULL;
    }
    
    EPD_4in26_Sleep();
    DEV_Module_Exit();
    
    PR_NOTICE("Test complete");
}
