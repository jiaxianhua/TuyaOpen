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
#include "hzk24.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/***********************************************************
************************macro define************************
***********************************************************/
// WiFi Configuration - CHANGE THESE TO YOUR WIFI CREDENTIALS
#define WIFI_SSID "1519"
#define WIFI_PASSWORD "15889629702"

// Novel URL - CHANGE THIS TO YOUR NOVEL URL
#define NOVEL_URL "http://120.79.89.230/fanren.txt"

// Time sync settings
#define TIME_SERVER_URL "www.baidu.com"
#define TIME_SERVER_PATH "/"
#define HTTP_REQUEST_TIMEOUT 8000

// Display settings
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define CHARS_PER_LINE 37  // Maximum characters per line (800 / 24 = 33.3)
#define LINES_PER_PAGE 32  // Maximum lines per page (480 / 24 = 20)
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
static int g_time_synced = 0;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Parse HTTP Date header and set system time
 * Example: "Date: Thu, 20 Dec 2024 12:34:56 GMT"
 */
static int parse_http_date(const char *date_str, struct tm *tm_time)
{
    // Skip "Date: " prefix if present
    const char *p = strstr(date_str, "Date:");
    if (p) {
        p += 5;
        while (*p == ' ') p++;
    } else {
        p = date_str;
    }
    
    // Parse: "Thu, 20 Dec 2024 12:34:56 GMT"
    char month_str[4] = {0};
    char weekday[4] = {0};
    
    int parsed = sscanf(p, "%3s, %d %3s %d %d:%d:%d",
                       weekday,
                       &tm_time->tm_mday,
                       month_str,
                       &tm_time->tm_year,
                       &tm_time->tm_hour,
                       &tm_time->tm_min,
                       &tm_time->tm_sec);
    
    if (parsed != 7) {
        PR_ERR("Failed to parse date string: %s", date_str);
        return -1;
    }
    
    // Convert month string to number
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    tm_time->tm_mon = -1;
    for (int i = 0; i < 12; i++) {
        if (strcmp(month_str, months[i]) == 0) {
            tm_time->tm_mon = i;
            break;
        }
    }
    
    if (tm_time->tm_mon == -1) {
        PR_ERR("Invalid month: %s", month_str);
        return -1;
    }
    
    // Adjust year (tm_year is years since 1900)
    tm_time->tm_year -= 1900;
    tm_time->tm_isdst = 0;
    
    return 0;
}

/**
 * @brief Sync time from HTTP server and set system time
 */
static int sync_time_from_http(void)
{
    int rt = OPRT_OK;
    http_client_response_t http_response = {0};
    
    PR_NOTICE("Syncing time from %s...", TIME_SERVER_URL);
    
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "Mozilla/5.0"},
        {.key = "Connection", .value = "close"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .cacert = NULL,
            .cacert_len = 0,
            .host = TIME_SERVER_URL,
            .port = 80,
            .method = "GET",
            .path = TIME_SERVER_PATH,
            .headers = headers,
            .headers_count = 2,
            .body = "",
            .body_length = 0,
            .timeout_ms = HTTP_REQUEST_TIMEOUT
        },
        &http_response);
    
    if (HTTP_CLIENT_SUCCESS != http_status) {
        PR_ERR("HTTP request failed: %d", http_status);
        rt = OPRT_COM_ERROR;
        goto cleanup;
    }
    
    PR_NOTICE("HTTP request successful, status: %d", http_response.status_code);
    
    // Parse Date header from response
    if (http_response.headers && http_response.headers_length > 0) {
        char *headers_str = (char *)tal_malloc(http_response.headers_length + 1);
        if (headers_str) {
            memcpy(headers_str, http_response.headers, http_response.headers_length);
            headers_str[http_response.headers_length] = '\0';
            
            // Find Date header (case insensitive)
            char *date_line = strstr(headers_str, "Date:");
            if (!date_line) {
                date_line = strstr(headers_str, "date:");
            }
            
            if (date_line) {
                char *line_end = strstr(date_line, "\r\n");
                if (line_end) {
                    *line_end = '\0';
                }
                
                PR_NOTICE("Found Date header: %s", date_line);
                
                struct tm tm_time = {0};
                if (parse_http_date(date_line, &tm_time) == 0) {
                    // Convert to timestamp (GMT)
                    time_t server_time_gmt = mktime(&tm_time);
                    // Add 8 hours for China timezone (GMT+8)
                    time_t server_time_local = server_time_gmt + (8 * 3600);
                    
                    // Set system time using tal_time_set_posix
                    tal_time_set_posix(server_time_local, 0);
                    g_time_synced = 1;
                    
                    PR_NOTICE("System time set to: %ld", server_time_local);
                    
                    struct tm *synced_time = localtime(&server_time_local);
                    PR_NOTICE("Synced time (GMT+8): %04d-%02d-%02d %02d:%02d:%02d",
                             synced_time->tm_year + 1900,
                             synced_time->tm_mon + 1,
                             synced_time->tm_mday,
                             synced_time->tm_hour,
                             synced_time->tm_min,
                             synced_time->tm_sec);
                } else {
                    PR_ERR("Failed to parse date header");
                }
            } else {
                PR_WARN("No Date header found in response");
            }
            
            tal_free(headers_str);
        }
    }
    
cleanup:
    http_client_free(&http_response);
    return rt;
}

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
    
    // Sync time from network
    PR_NOTICE("Waiting for network to stabilize before time sync...");
    tal_system_sleep(2000);  // Wait for network to stabilize
    
    if (sync_time_from_http() == OPRT_OK && g_time_synced) {
        // Verify time was set correctly
        time_t current = tal_time_get_posix();
        struct tm *tm = localtime(&current);
        if (tm) {
            PR_NOTICE("Time sync verified: %04d-%02d-%02d %02d:%02d:%02d",
                     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                     tm->tm_hour, tm->tm_min, tm->tm_sec);
        }
    } else {
        PR_WARN("Time sync failed, time may be incorrect");
    }
    
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
 * @brief Draw a GBK Chinese character using HZK16 font (16x16)
 */
static void draw_gbk_char(int x, int y, unsigned char gb_high, unsigned char gb_low, 
                          UWORD fg_color, UWORD bg_color)
{
    uint8_t font_data[32];
    
    // Get font data from HZK16
    int ret = hzk16_get_font_data(gb_high, gb_low, font_data);
    
    if (ret != 0) {
        // Character not found, draw placeholder
        Paint_DrawString_EN(x, y, "  ", &Font16, fg_color, bg_color);
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
 * @brief Draw a GBK Chinese character using HZK24 font (24x24)
 */
static void draw_gbk_char24(int x, int y, unsigned char gb_high, unsigned char gb_low, 
                            UWORD fg_color, UWORD bg_color)
{
    uint8_t font_data[72];
    
    // Get font data from HZK24
    int ret = hzk24_get_font_data(gb_high, gb_low, font_data);
    
    if (ret != 0) {
        // Character not found, draw placeholder
        Paint_DrawString_EN(x, y, "  ", &Font24, fg_color, bg_color);
        return;
    }
    
    // Draw 24x24 bitmap
    for (int row = 0; row < 24; row++) {
        for (int col = 0; col < 24; col++) {
            int byte_idx = row * 3 + col / 8;  // 24 pixels = 3 bytes per row
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
    
    // Get current time from system (like clock example)
    time_t rawtime = tal_time_get_posix();
    struct tm *timeinfo = NULL;
    
    PR_DEBUG("Current system time: %ld", rawtime);
    
    // Draw page info and time at top left (compact format)
    char page_info[80];
    if (rawtime > 0) {
        timeinfo = localtime(&rawtime);
        if (timeinfo) {
            PR_DEBUG("Time: %04d-%02d-%02d %02d:%02d:%02d",
                    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                    timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            snprintf(page_info, sizeof(page_info), "T5AI P%d/%d %04d-%02d-%02d %02d:%02d", 
                     g_reader_ctx.current_page + 1, g_reader_ctx.total_pages,
                     timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                     timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            PR_WARN("localtime() returned NULL");
            snprintf(page_info, sizeof(page_info), "P%d/%d", 
                     g_reader_ctx.current_page + 1, g_reader_ctx.total_pages);
        }
    } else {
        PR_WARN("Invalid system time: %ld", rawtime);
        snprintf(page_info, sizeof(page_info), "P%d/%d", 
                 g_reader_ctx.current_page + 1, g_reader_ctx.total_pages);
    }
    Paint_DrawString_EN(42, 2, page_info, &Font20, BLACK, WHITE);
    
    // Draw "贾" character at top right corner (GBK: 0xBCD6)
    // For ROTATE_90, the actual display width is 480 (height becomes width)
    draw_gbk_char24(10, 2, 0xBC, 0xD6, BLACK, WHITE);
    
    // Get page start position
    int page_start = g_reader_ctx.page_offsets[g_reader_ctx.current_page];
    int page_end = (g_reader_ctx.current_page + 1 < g_reader_ctx.total_pages) 
                   ? g_reader_ctx.page_offsets[g_reader_ctx.current_page + 1] 
                   : g_reader_ctx.content_size;
    
    // Draw content line by line
    int y_pos = 24;  // Start right after compact header
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
                    Paint_DrawString_EN(x_pos, y_pos, ascii_str, &Font24, BLACK, WHITE);
                    x_pos += 12;  // ASCII width for Font24
                    i++;
                } else if (is_gbk_lead_byte(c) && i + 1 < line_len) {
                    // GBK character - use HZK24 font
                    unsigned char gb_high = (unsigned char)line_buf[i];
                    unsigned char gb_low = (unsigned char)line_buf[i + 1];
                    draw_gbk_char24(x_pos, y_pos, gb_high, gb_low, BLACK, WHITE);
                    x_pos += 24;  // GBK width (24 pixels)
                    i += 2;
                } else {
                    // Unknown character - use space as placeholder
                    Paint_DrawString_EN(x_pos, y_pos, " ", &Font24, BLACK, WHITE);
                    x_pos += 12;
                    i++;
                }
            }
        }
        
        y_pos += 24;  // Tight line spacing for 24x24 font (no gap)
        line_count++;
    }
    
    // Update display with full refresh
    EPD_4in26_Display(g_image_buffer);
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
    
    // Show "Connecting..." message
    Paint_DrawString_EN(150, 300, "Connecting to WiFi...", &Font24, BLACK, WHITE);
    EPD_4in26_Display(g_image_buffer);
    
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
