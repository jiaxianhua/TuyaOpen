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
#include "sd_file_manager.h"
#include "GUI_BMPfile.h"
#include "cJSON.h"
#include "utf8_gbk_table.h"
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

// Wallpaper settings
#define WALLPAPER_URL "http://120.79.89.230:8001/files/wallpaper.bmp"
#define WALLPAPER_HOST "120.79.89.230"
#define WALLPAPER_PORT 8001
#define WALLPAPER_PATH "/files/wallpaper.bmp"
#define WALLPAPER_FILE "/sdcard/wallpaper.bmp"
#define WALLPAPER_TIMEOUT 30000  // 30 seconds for image download

// Time sync settings
#define TIME_SERVER_URL "www.baidu.com"
#define TIME_SERVER_PORT 80
#define TIME_SERVER_PATH "/"
#define HTTP_REQUEST_TIMEOUT 8000

// Display settings
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480

// Character counting (NOT pixel-based!)
// CHARS_PER_LINE counts "character units": ASCII=1 unit, GBK=2 units
// With ROTATE_90: logical width=480px, can fit ~20 GBK chars (20*24px=480px)
// But we count in units: 20 GBK chars = 40 units (20*2)
// Mixed text: "Hello你好" = 5 ASCII (5 units) + 2 GBK (4 units) = 9 units
#define CHARS_PER_LINE 38  // Character units per line (not pixels!)
#define LINES_PER_PAGE 32  // Lines per page (800 / 24 = 33.3, use 32 for safety)
#define BYTES_PER_PAGE 2000  // For GBK encoding (variable byte length)

// Button settings
#define BUTTON_LONG_PRESS_TIME 2000  // 2 seconds for long press
#define BUTTON_CLICK_TIMEOUT 500     // 500ms timeout between clicks
#define BUTTON_ACTION_DELAY 2000     // 2s delay before executing action

// Network file browser settings
#define FILE_SERVER_HOST "120.79.89.230"
#define FILE_SERVER_PORT 8001
#define FILE_LIST_PATH "/files"
#define FILE_DOWNLOAD_PATH "/files/"
#define THUMBNAIL_PATH "/files/thumbs/"
#define MAX_FILENAME_LEN 128
#define MAX_URL_LEN 256
#define MAX_NETWORK_FILES 100
#define GRID_ROWS 3
#define GRID_COLS 3
#define GRID_CELL_WIDTH 160
#define GRID_CELL_HEIGHT 160
#define THUMBNAIL_SIZE 120
#define FILES_PER_PAGE (GRID_ROWS * GRID_COLS)  // 9 files per page

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    MODE_FILE_BROWSER = 0,  // 文件浏览模式
    MODE_TEXT_READER,       // 文本阅读模式
    MODE_IMAGE_VIEWER,      // 图片查看模式
    MODE_NETWORK_BROWSER    // 网络文件浏览模式
} app_mode_e;

typedef enum {
    BROWSER_MODE_SD_CARD = 0,  // SD 卡浏览模式
    BROWSER_MODE_NETWORK,      // 网络浏览模式
    BROWSER_MODE_COUNT
} browser_mode_e;

typedef enum {
    BUTTON_ACTION_NONE = 0,
    BUTTON_ACTION_NEXT,          // 单击：下一个
    BUTTON_ACTION_PREV,          // 双击：上一个
    BUTTON_ACTION_OPEN,          // 长按：打开/关闭/下载
    BUTTON_ACTION_MODE_SWITCH    // 三击：切换模式
} button_action_e;

// Network file info structure
typedef struct {
    char filename[MAX_FILENAME_LEN];           // 服务器文件名
    char original_filename[MAX_FILENAME_LEN];  // 原始文件名（UTF-8）
    char display_name[MAX_FILENAME_LEN];       // 显示名称（GBK）
    char url[MAX_URL_LEN];                     // 文件 URL
    char thumbnail_url[MAX_URL_LEN];           // 缩略图 URL
    int size;                                  // 文件大小（字节）
    char filetype[32];                         // 文件类型
    UBYTE *thumbnail_data;                     // 缩略图数据（120x120）
    int thumbnail_loaded;                      // 缩略图是否已加载
} network_file_info_t;

// Network file browser structure
typedef struct {
    network_file_info_t files[MAX_NETWORK_FILES];
    int file_count;
    int current_index;
    int current_page;        // 当前页（每页9个文件）
    int total_pages;         // 总页数
    int files_loaded;        // 文件列表是否已加载
} network_file_browser_t;

// Mode manager structure
typedef struct {
    browser_mode_e current_mode;
    int network_available;
    int sd_available;
} mode_manager_t;

// Grid cell structure for network file display
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int selected;
} grid_cell_t;

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
    
    // SD card file browser
    file_browser_t browser;
    app_mode_e mode;
    int sd_available;
    
    // Network file browser
    network_file_browser_t network_browser;
    mode_manager_t mode_manager;
    
    // Button click detection
    volatile int click_count;
    volatile int long_press_detected;
    volatile TIME_T last_click_time;
    volatile TIME_T action_trigger_time;
    
    // Paged reading for large files
    char current_filepath[300];  // Path to currently open file
    int use_paged_reading;       // 1 if using paged reading mode
    int page_size;               // Size of each page in bytes (e.g., 1024 for 1KB)
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

// Forward declarations
static void draw_gbk_char(int x, int y, unsigned char gb_high, unsigned char gb_low, 
                          UWORD fg_color, UWORD bg_color);
static void draw_gbk_char24(int x, int y, unsigned char gb_high, unsigned char gb_low, 
                            UWORD fg_color, UWORD bg_color);

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
            .port = TIME_SERVER_PORT,
            .method = "GET",
            .path = TIME_SERVER_PATH,
            .headers = headers,
            .headers_count = 2,
            .body = (const uint8_t *)"",
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
 * @brief Download wallpaper from server and save to SD card
 */
static int download_wallpaper(void)
{
    if (!g_reader_ctx.network_connected) {
        PR_ERR("Network not connected");
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Downloading wallpaper from: %s", WALLPAPER_URL);
    
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "TuyaOpen-NovelReader/1.0"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = WALLPAPER_HOST,
            .port = WALLPAPER_PORT,
            .method = "GET",
            .path = WALLPAPER_PATH,
            .headers = headers,
            .headers_count = 1,
            .body = (const uint8_t *)"",
            .body_length = 0,
            .timeout_ms = WALLPAPER_TIMEOUT
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
    
    // Check response size
    int body_len = http_response.body_length;
    if (body_len <= 0 || body_len > 10 * 1024 * 1024) {
        PR_ERR("Invalid image size: %d", body_len);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Downloaded %d bytes, saving to SD card...", body_len);
    
    // Save to SD card
    TUYA_FILE fp = tal_fopen(WALLPAPER_FILE, "wb");
    if (!fp) {
        PR_ERR("Failed to create file: %s", WALLPAPER_FILE);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    int written = tal_fwrite((void *)http_response.body, body_len, fp);
    tal_fclose(fp);
    
    if (written != body_len) {
        PR_ERR("Failed to write file, written=%d, expected=%d", written, body_len);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Wallpaper saved successfully: %s", WALLPAPER_FILE);
    
    http_client_free(&http_response);
    return OPRT_OK;
}

/**
 * @brief Display wallpaper if exists on SD card
 * @return OPRT_OK if wallpaper displayed, error otherwise
 */
static int display_wallpaper(void)
{
    // Check if wallpaper exists
    BOOL_T exists = FALSE;
    if (tal_fs_is_exist(WALLPAPER_FILE, &exists) != OPRT_OK || !exists) {
        PR_DEBUG("Wallpaper file not found: %s", WALLPAPER_FILE);
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Displaying wallpaper: %s", WALLPAPER_FILE);
    
    Paint_Clear(WHITE);
    
    // Display the BMP image
    int rt = sd_display_bmp_image(WALLPAPER_FILE);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to display wallpaper");
        return rt;
    }
    
    EPD_4in26_Display(g_image_buffer);
    
    PR_NOTICE("Wallpaper displayed successfully");
    return OPRT_OK;
}

/**
 * @brief Fetch file list from server
 * @param json_response Output buffer for JSON response (caller must free)
 * @return OPRT_OK on success, error code otherwise
 */
static int fetch_file_list(char **json_response)
{
    if (!g_reader_ctx.network_connected) {
        PR_ERR("Network not connected");
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Fetching file list from: http://%s:%d%s", 
              FILE_SERVER_HOST, FILE_SERVER_PORT, FILE_LIST_PATH);
    
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "TuyaOpen-NovelReader/1.0"},
        {.key = "Accept", .value = "application/json"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = FILE_SERVER_HOST,
            .port = FILE_SERVER_PORT,
            .method = "GET",
            .path = FILE_LIST_PATH,
            .headers = headers,
            .headers_count = 2,
            .body = (const uint8_t *)"",
            .body_length = 0,
            .timeout_ms = HTTP_REQUEST_TIMEOUT
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
    
    // Check response size
    int body_len = http_response.body_length;
    if (body_len <= 0 || body_len > 50 * 1024) {  // Max 50KB for JSON
        PR_ERR("Invalid JSON size: %d", body_len);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("Received %d bytes JSON response", body_len);
    
    // Allocate memory for JSON response
    *json_response = (char *)tal_malloc(body_len + 1);
    if (!*json_response) {
        PR_ERR("Failed to allocate memory for JSON");
        http_client_free(&http_response);
        return OPRT_MALLOC_FAILED;
    }
    
    // Copy response body
    memcpy(*json_response, http_response.body, body_len);
    (*json_response)[body_len] = '\0';
    
    http_client_free(&http_response);
    return OPRT_OK;
}

/**
 * @brief Parse file list JSON response
 * @param json_str JSON string to parse
 * @param browser Network file browser to populate
 * @return OPRT_OK on success, error code otherwise
 */
static int parse_file_list_json(const char *json_str, network_file_browser_t *browser)
{
    if (!json_str || !browser) {
        PR_ERR("Invalid parameters");
        return OPRT_INVALID_PARM;
    }
    
    PR_NOTICE("Parsing JSON response...");
    
    // Parse JSON
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        PR_ERR("Failed to parse JSON");
        return OPRT_COM_ERROR;
    }
    
    // Get files array
    cJSON *files_array = cJSON_GetObjectItem(root, "files");
    if (!files_array || !cJSON_IsArray(files_array)) {
        PR_ERR("No 'files' array in JSON");
        cJSON_Delete(root);
        return OPRT_COM_ERROR;
    }
    
    int array_size = cJSON_GetArraySize(files_array);
    if (array_size == 0) {
        PR_WARN("Empty file list");
        browser->file_count = 0;
        cJSON_Delete(root);
        return OPRT_OK;
    }
    
    PR_NOTICE("Found %d files in JSON", array_size);
    
    // Limit to MAX_NETWORK_FILES
    int file_count = (array_size > MAX_NETWORK_FILES) ? MAX_NETWORK_FILES : array_size;
    
    // Parse each file entry
    for (int i = 0; i < file_count; i++) {
        cJSON *file_obj = cJSON_GetArrayItem(files_array, i);
        if (!file_obj) continue;
        
        network_file_info_t *file_info = &browser->files[i];
        memset(file_info, 0, sizeof(network_file_info_t));
        
        // Extract filename
        cJSON *filename = cJSON_GetObjectItem(file_obj, "filename");
        if (filename && cJSON_IsString(filename)) {
            strncpy(file_info->filename, filename->valuestring, MAX_FILENAME_LEN - 1);
        }
        
        // Extract original_filename (UTF-8)
        cJSON *original_filename = cJSON_GetObjectItem(file_obj, "original_filename");
        if (original_filename && cJSON_IsString(original_filename)) {
            strncpy(file_info->original_filename, original_filename->valuestring, MAX_FILENAME_LEN - 1);
        } else {
            // Fallback to filename if original_filename not present
            strncpy(file_info->original_filename, file_info->filename, MAX_FILENAME_LEN - 1);
        }
        
        // Extract size
        cJSON *size = cJSON_GetObjectItem(file_obj, "size");
        if (size && cJSON_IsNumber(size)) {
            file_info->size = size->valueint;
        }
        
        // Extract url
        cJSON *url = cJSON_GetObjectItem(file_obj, "url");
        if (url && cJSON_IsString(url)) {
            strncpy(file_info->url, url->valuestring, MAX_URL_LEN - 1);
        }
        
        // Extract filetype
        cJSON *filetype = cJSON_GetObjectItem(file_obj, "filetype");
        if (filetype && cJSON_IsString(filetype)) {
            strncpy(file_info->filetype, filetype->valuestring, 31);
        }
        
        // Extract thumbnail URL
        cJSON *thumbnail = cJSON_GetObjectItem(file_obj, "thumbnail");
        if (thumbnail && cJSON_IsString(thumbnail)) {
            strncpy(file_info->thumbnail_url, thumbnail->valuestring, MAX_URL_LEN - 1);
        }
        
        // Initialize thumbnail data
        file_info->thumbnail_data = NULL;
        file_info->thumbnail_loaded = 0;
        
        PR_DEBUG("Parsed file %d: %s (%d bytes)", i, file_info->filename, file_info->size);
    }
    
    browser->file_count = file_count;
    browser->current_index = 0;
    browser->current_page = 0;
    browser->total_pages = (file_count + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    browser->files_loaded = 1;
    
    PR_NOTICE("Successfully parsed %d files, %d pages", file_count, browser->total_pages);
    
    cJSON_Delete(root);
    return OPRT_OK;
}

/**
 * @brief Convert UTF-8 string to GBK encoding
 * @param utf8_str Input UTF-8 string
 * @param gbk_str Output GBK string buffer
 * @param gbk_len Size of output buffer
 * @return OPRT_OK on success, error code otherwise
 */
static int utf8_to_gbk(const char *utf8_str, char *gbk_str, int gbk_len)
{
    if (!utf8_str || !gbk_str || gbk_len <= 0) {
        return OPRT_INVALID_PARM;
    }
    
    int utf8_pos = 0;
    int gbk_pos = 0;
    int utf8_len = strlen(utf8_str);
    
    while (utf8_pos < utf8_len && gbk_pos < gbk_len - 1) {
        unsigned char c = (unsigned char)utf8_str[utf8_pos];
        
        // ASCII character (0x00-0x7F)
        if (c < 0x80) {
            gbk_str[gbk_pos++] = utf8_str[utf8_pos++];
            continue;
        }
        
        // UTF-8 multi-byte character
        // Chinese characters are typically 3 bytes in UTF-8
        if ((c & 0xF0) == 0xE0 && utf8_pos + 2 < utf8_len) {
            // 3-byte UTF-8 character
            uint32_t utf8_code = ((uint32_t)(utf8_str[utf8_pos] & 0xFF) << 16) |
                                 ((uint32_t)(utf8_str[utf8_pos + 1] & 0xFF) << 8) |
                                 ((uint32_t)(utf8_str[utf8_pos + 2] & 0xFF));
            
            // Search in mapping table
            int found = 0;
            for (int i = 0; i < UTF8_GBK_TABLE_SIZE; i++) {
                if (utf8_gbk_table[i].utf8_code == utf8_code) {
                    // Found mapping
                    uint16_t gbk_code = utf8_gbk_table[i].gbk_code;
                    if (gbk_pos + 2 <= gbk_len - 1) {
                        gbk_str[gbk_pos++] = (gbk_code >> 8) & 0xFF;
                        gbk_str[gbk_pos++] = gbk_code & 0xFF;
                        found = 1;
                    }
                    break;
                }
            }
            
            if (!found) {
                // Character not in table, use placeholder
                if (gbk_pos < gbk_len - 1) {
                    gbk_str[gbk_pos++] = '?';
                }
            }
            
            utf8_pos += 3;
        } else if ((c & 0xE0) == 0xC0 && utf8_pos + 1 < utf8_len) {
            // 2-byte UTF-8 character (not common for Chinese)
            // Just use placeholder
            gbk_str[gbk_pos++] = '?';
            utf8_pos += 2;
        } else {
            // Invalid UTF-8 sequence, skip
            utf8_pos++;
        }
    }
    
    gbk_str[gbk_pos] = '\0';
    return OPRT_OK;
}

/**
 * @brief Calculate grid layout for 3x3 file display
 * @param[out] cells Array to store grid cell positions
 */
static void grid_calculate_layout(grid_cell_t cells[GRID_ROWS][GRID_COLS])
{
    const int start_x = 0;
    const int start_y = 30;  // Leave space for title bar
    
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            cells[row][col].x = start_x + col * GRID_CELL_WIDTH;
            cells[row][col].y = start_y + row * GRID_CELL_HEIGHT;
            cells[row][col].width = GRID_CELL_WIDTH;
            cells[row][col].height = GRID_CELL_HEIGHT;
            cells[row][col].selected = 0;
        }
    }
}

/**
 * @brief Convert file index to grid position
 * @param[in] index File index
 * @param[out] row Grid row
 * @param[out] col Grid column
 * @param[out] page Page number
 */
static void grid_index_to_position(int index, int *row, int *col, int *page)
{
    if (!row || !col || !page) {
        return;
    }
    
    *page = index / (GRID_ROWS * GRID_COLS);
    int page_index = index % (GRID_ROWS * GRID_COLS);
    *row = page_index / GRID_COLS;
    *col = page_index % GRID_COLS;
}

/**
 * @brief Render a single grid cell
 * @param[in] file File information
 * @param[in] cell Cell position and size
 * @param[in] selected Whether this cell is selected
 */
static void render_grid_cell(network_file_info_t *file, grid_cell_t *cell, int selected)
{
    if (!file || !cell) {
        return;
    }
    
    // Draw cell border
    UWORD border_color = selected ? BLACK : GRAY2;
    Paint_DrawRectangle(cell->x, cell->y, cell->x + cell->width - 1, 
                       cell->y + cell->height - 1, border_color, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    
    // Calculate thumbnail position (centered)
    int thumb_x = cell->x + (cell->width - THUMBNAIL_SIZE) / 2;
    int thumb_y = cell->y + 5;
    
    // Draw thumbnail if loaded
    if (file->thumbnail_loaded && file->thumbnail_data) {
        // Draw thumbnail (120x120 bitmap)
        for (int y = 0; y < THUMBNAIL_SIZE; y++) {
            for (int x = 0; x < THUMBNAIL_SIZE; x++) {
                int byte_idx = (y * THUMBNAIL_SIZE + x) / 8;
                int bit_idx = 7 - ((y * THUMBNAIL_SIZE + x) % 8);
                UWORD color = (file->thumbnail_data[byte_idx] & (1 << bit_idx)) ? BLACK : WHITE;
                Paint_SetPixel(thumb_x + x, thumb_y + y, color);
            }
        }
    } else {
        // Draw placeholder
        Paint_DrawRectangle(thumb_x, thumb_y, thumb_x + THUMBNAIL_SIZE - 1, 
                           thumb_y + THUMBNAIL_SIZE - 1, GRAY2, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        Paint_DrawString_EN(thumb_x + 30, thumb_y + 50, "Loading", &Font16, BLACK, WHITE);
    }
    
    // Draw filename (2 lines max, GBK encoded)
    int text_y = thumb_y + THUMBNAIL_SIZE + 5;
    int text_x = cell->x + 5;
    
    // Truncate filename if too long
    char display_name[40];
    strncpy(display_name, file->display_name, sizeof(display_name) - 1);
    display_name[sizeof(display_name) - 1] = '\0';
    
    // Draw filename using GBK font
    int x = text_x;
    int y = text_y;
    int line_width = 0;
    int max_width = cell->width - 10;
    
    for (int i = 0; display_name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)display_name[i];
        
        if (c >= 0x80 && display_name[i + 1] != '\0') {
            // GBK character (2 bytes)
            unsigned char gb_high = c;
            unsigned char gb_low = (unsigned char)display_name[i + 1];
            
            if (line_width + 16 > max_width) {
                // Move to next line
                y += 18;
                x = text_x;
                line_width = 0;
                if (y > text_y + 18) break;  // Max 2 lines
            }
            
            draw_gbk_char(x, y, gb_high, gb_low, BLACK, WHITE);
            x += 16;
            line_width += 16;
            i++;  // Skip next byte
        } else {
            // ASCII character
            if (line_width + 8 > max_width) {
                y += 18;
                x = text_x;
                line_width = 0;
                if (y > text_y + 18) break;
            }
            
            char str[2] = {display_name[i], '\0'};
            Paint_DrawString_EN(x, y, str, &Font16, BLACK, WHITE);
            x += 8;
            line_width += 8;
        }
    }
}

/**
 * @brief Display network file grid
 * @param[in] browser Network file browser context
 */
static void display_network_file_grid(network_file_browser_t *browser)
{
    if (!browser || !browser->files_loaded) {
        PR_ERR("Browser not initialized or no files loaded");
        return;
    }
    
    PR_NOTICE("Displaying network file grid, page %d/%d", 
              browser->current_page + 1, browser->total_pages);
    
    // Clear buffer
    Paint_Clear(WHITE);
    
    // Draw title bar with time
    time_t rawtime = tal_time_get_posix();
    struct tm *timeinfo = localtime(&rawtime);
    char title[80];
    
    if (timeinfo && rawtime > 0) {
        snprintf(title, sizeof(title), "Network Files P%d/%d %02d:%02d", 
                 browser->current_page + 1, browser->total_pages,
                 timeinfo->tm_hour, timeinfo->tm_min);
    } else {
        snprintf(title, sizeof(title), "Network Files P%d/%d", 
                 browser->current_page + 1, browser->total_pages);
    }
    
    Paint_DrawString_EN(5, 5, title, &Font16, BLACK, WHITE);
    Paint_DrawLine(0, 28, 480, 28, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    // Calculate grid layout
    grid_cell_t cells[GRID_ROWS][GRID_COLS];
    grid_calculate_layout(cells);
    
    // Calculate file range for current page
    int start_idx = browser->current_page * (GRID_ROWS * GRID_COLS);
    int end_idx = start_idx + (GRID_ROWS * GRID_COLS);
    if (end_idx > browser->file_count) {
        end_idx = browser->file_count;
    }
    
    // Render each cell
    for (int idx = start_idx; idx < end_idx; idx++) {
        int row, col, page;
        grid_index_to_position(idx, &row, &col, &page);
        
        int selected = (idx == browser->current_index);
        cells[row][col].selected = selected;
        
        render_grid_cell(&browser->files[idx], &cells[row][col], selected);
    }
    
    // Draw status bar
    Paint_DrawLine(0, 770, 480, 770, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(5, 775, "Click:Next DblClick:Prev Long:Download 3Click:Mode", 
                       &Font12, BLACK, WHITE);
    
    // Update display
    EPD_4in26_Display(g_image_buffer);
}

/**
 * @brief Parse URL path from full URL (assumes format: http://host:port/path)
 * @param[in] url Full URL
 * @param[out] path Buffer to store path
 * @param[in] path_size Size of path buffer
 * @return OPRT_OK on success, error code otherwise
 */
static int parse_url_path(const char *url, char *path, int path_size)
{
    if (!url || !path || path_size <= 0) {
        return OPRT_INVALID_PARM;
    }
    
    // Check if it's already a path (starts with /)
    if (url[0] == '/') {
        strncpy(path, url, path_size - 1);
        path[path_size - 1] = '\0';
        return OPRT_OK;
    }
    
    // Check if it's a full URL (starts with http:// or https://)
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        // Find the third slash (after http://)
        const char *p = url;
        int slash_count = 0;
        
        while (*p && slash_count < 3) {
            if (*p == '/') {
                slash_count++;
            }
            p++;
        }
        
        if (slash_count < 3) {
            // No path, use root
            strncpy(path, "/", path_size);
            return OPRT_OK;
        }
        
        // Copy path (p-1 points to the character after the third slash)
        snprintf(path, path_size, "/%s", p - 1);
        return OPRT_OK;
    }
    
    // If it doesn't start with / or http, assume it's a relative path
    snprintf(path, path_size, "/%s", url);
    return OPRT_OK;
}

/**
 * @brief Load thumbnail for a single file
 * @param[in,out] file File information
 * @return OPRT_OK on success, error code otherwise
 */
static int load_thumbnail(network_file_info_t *file)
{
    if (!file || file->thumbnail_loaded) {
        return OPRT_OK;
    }
    
    PR_NOTICE("Loading thumbnail: %s", file->thumbnail_url);
    
    // Allocate memory for thumbnail (120x120 = 14400 pixels = 1800 bytes)
    int thumb_size = (THUMBNAIL_SIZE * THUMBNAIL_SIZE) / 8;
    file->thumbnail_data = (UBYTE *)tal_malloc(thumb_size);
    if (!file->thumbnail_data) {
        PR_ERR("Failed to allocate thumbnail memory");
        return OPRT_MALLOC_FAILED;
    }
    
    // Parse URL path
    char path[256];
    if (parse_url_path(file->thumbnail_url, path, sizeof(path)) != OPRT_OK) {
        PR_ERR("Failed to parse thumbnail URL");
        tal_free(file->thumbnail_data);
        file->thumbnail_data = NULL;
        return OPRT_COM_ERROR;
    }
    
    // Download thumbnail
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "TuyaOpen-NovelReader/1.0"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = FILE_SERVER_HOST,
            .port = FILE_SERVER_PORT,
            .method = "GET",
            .path = path,
            .headers = headers,
            .headers_count = 1,
            .body = (const uint8_t *)"",
            .body_length = 0,
            .timeout_ms = HTTP_REQUEST_TIMEOUT
        },
        &http_response
    );
    
    if (http_status != HTTP_CLIENT_SUCCESS || http_response.status_code != 200) {
        PR_ERR("Failed to download thumbnail: %d, status: %d", http_status, http_response.status_code);
        tal_free(file->thumbnail_data);
        file->thumbnail_data = NULL;
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    // Copy thumbnail data
    if (http_response.body && http_response.body_length > 0) {
        int copy_size = (http_response.body_length < thumb_size) ? http_response.body_length : thumb_size;
        memcpy(file->thumbnail_data, http_response.body, copy_size);
        file->thumbnail_loaded = 1;
    }
    
    http_client_free(&http_response);
    PR_NOTICE("Thumbnail loaded successfully");
    
    return OPRT_OK;
}

/**
 * @brief Load thumbnails for current page
 * @param[in,out] browser Network file browser context
 * @return OPRT_OK on success, error code otherwise
 */
static int load_page_thumbnails(network_file_browser_t *browser)
{
    if (!browser || !browser->files_loaded) {
        return OPRT_INVALID_PARM;
    }
    
    int start_idx = browser->current_page * (GRID_ROWS * GRID_COLS);
    int end_idx = start_idx + (GRID_ROWS * GRID_COLS);
    if (end_idx > browser->file_count) {
        end_idx = browser->file_count;
    }
    
    PR_NOTICE("Loading thumbnails for page %d (files %d-%d)", 
              browser->current_page, start_idx, end_idx - 1);
    
    for (int i = start_idx; i < end_idx; i++) {
        if (!browser->files[i].thumbnail_loaded) {
            load_thumbnail(&browser->files[i]);
            // Small delay to avoid overwhelming the server
            tal_system_sleep(100);
        }
    }
    
    return OPRT_OK;
}

/**
 * @brief Free thumbnails for a specific page
 * @param[in,out] browser Network file browser context
 * @param[in] page Page number
 */
static void free_page_thumbnails(network_file_browser_t *browser, int page)
{
    if (!browser) {
        return;
    }
    
    int start_idx = page * (GRID_ROWS * GRID_COLS);
    int end_idx = start_idx + (GRID_ROWS * GRID_COLS);
    if (end_idx > browser->file_count) {
        end_idx = browser->file_count;
    }
    
    PR_NOTICE("Freeing thumbnails for page %d", page);
    
    for (int i = start_idx; i < end_idx; i++) {
        if (browser->files[i].thumbnail_data) {
            tal_free(browser->files[i].thumbnail_data);
            browser->files[i].thumbnail_data = NULL;
            browser->files[i].thumbnail_loaded = 0;
        }
    }
}

/**
 * @brief Display download progress
 * @param[in] filename File name
 * @param[in] downloaded Downloaded bytes
 * @param[in] total Total bytes
 */
static void display_download_progress(const char *filename, int downloaded, int total)
{
    Paint_Clear(WHITE);
    
    // Title
    Paint_DrawString_EN(10, 10, "Downloading File", &Font24, BLACK, WHITE);
    
    // Filename (truncate if too long)
    char display_name[60];
    strncpy(display_name, filename, sizeof(display_name) - 1);
    display_name[sizeof(display_name) - 1] = '\0';
    
    // Draw filename using GBK font
    int x = 10;
    int y = 50;
    for (int i = 0; display_name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)display_name[i];
        if (c >= 0x80 && display_name[i + 1] != '\0') {
            draw_gbk_char(x, y, c, (unsigned char)display_name[i + 1], BLACK, WHITE);
            x += 16;
            i++;
        } else {
            char str[2] = {display_name[i], '\0'};
            Paint_DrawString_EN(x, y, str, &Font16, BLACK, WHITE);
            x += 8;
        }
    }
    
    // Progress bar
    int bar_x = 50;
    int bar_y = 100;
    int bar_width = 380;
    int bar_height = 30;
    
    // Draw border
    Paint_DrawRectangle(bar_x, bar_y, bar_x + bar_width, bar_y + bar_height, 
                       BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    
    // Draw filled portion
    if (total > 0) {
        int fill_width = (downloaded * bar_width) / total;
        if (fill_width > 0) {
            Paint_DrawRectangle(bar_x + 2, bar_y + 2, bar_x + fill_width - 2, 
                               bar_y + bar_height - 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }
    
    // Percentage
    char progress_text[32];
    int percent = total > 0 ? (downloaded * 100) / total : 0;
    snprintf(progress_text, sizeof(progress_text), "%d%%", percent);
    Paint_DrawString_EN(220, 105, progress_text, &Font20, BLACK, WHITE);
    
    // Size info
    char size_text[64];
    snprintf(size_text, sizeof(size_text), "%d / %d bytes", downloaded, total);
    Paint_DrawString_EN(100, 150, size_text, &Font16, BLACK, WHITE);
    
    // Instructions
    Paint_DrawString_EN(100, 200, "Long press to cancel", &Font16, BLACK, WHITE);
    
    // Update display (use fast mode for progress updates)
    EPD_4in26_Display_Fast(g_image_buffer);
}

/**
 * @brief Download file with progress display
 * @param[in] file File information
 * @return OPRT_OK on success, error code otherwise
 */
static int download_file_with_progress(network_file_info_t *file)
{
    if (!file) {
        return OPRT_INVALID_PARM;
    }
    
    PR_NOTICE("Downloading file: %s", file->filename);
    
    // Create save path
    char save_path[256];
    snprintf(save_path, sizeof(save_path), "/sdcard/%s", file->filename);
    
    // Check if file already exists
    if (tkl_fs_is_exist(save_path) == OPRT_OK) {
        PR_WARN("File already exists: %s", save_path);
        Paint_Clear(WHITE);
        Paint_DrawString_EN(100, 200, "File already exists!", &Font24, BLACK, WHITE);
        EPD_4in26_Display(g_image_buffer);
        tal_system_sleep(2000);
        return OPRT_OK;
    }
    
    // Open file for writing
    TUYA_FILE fp = tal_fopen(save_path, "wb");
    if (fp == NULL) {
        PR_ERR("Failed to create file: %s", save_path);
        return OPRT_FILE_OPEN_FAILED;
    }
    
    // Parse URL path
    char path[256];
    if (parse_url_path(file->url, path, sizeof(path)) != OPRT_OK) {
        PR_ERR("Failed to parse file URL");
        tal_fclose(fp);
        tal_fs_remove(save_path);
        return OPRT_COM_ERROR;
    }
    
    // Download file
    http_client_response_t http_response = {0};
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "TuyaOpen-NovelReader/1.0"}
    };
    
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){
            .host = FILE_SERVER_HOST,
            .port = FILE_SERVER_PORT,
            .method = "GET",
            .path = path,
            .headers = headers,
            .headers_count = 1,
            .body = (const uint8_t *)"",
            .body_length = 0,
            .timeout_ms = HTTP_REQUEST_TIMEOUT
        },
        &http_response
    );
    
    if (http_status != HTTP_CLIENT_SUCCESS || http_response.status_code != 200) {
        PR_ERR("Failed to start download: %d, status: %d", http_status, http_response.status_code);
        tal_fclose(fp);
        tal_fs_remove(save_path);
        http_client_free(&http_response);
        return OPRT_COM_ERROR;
    }
    
    // Write file data
    int downloaded = 0;
    int total = file->size;
    
    if (http_response.body && http_response.body_length > 0) {
        int written = tal_fwrite((void *)http_response.body, http_response.body_length, fp);
        if (written != http_response.body_length) {
            PR_ERR("Failed to write file");
            tal_fclose(fp);
            tal_fs_remove(save_path);
            http_client_free(&http_response);
            return OPRT_FILE_WRITE_FAILED;
        }
        downloaded = http_response.body_length;
        
        // Display final progress
        display_download_progress(file->display_name, downloaded, total);
    }
    
    http_client_free(&http_response);
    tal_fclose(fp);
    
    PR_NOTICE("Download completed: %s", save_path);
    
    // Show completion message
    Paint_Clear(WHITE);
    Paint_DrawString_EN(100, 200, "Download Complete!", &Font24, BLACK, WHITE);
    EPD_4in26_Display(g_image_buffer);
    tal_system_sleep(2000);
    
    return OPRT_OK;
}

/**
 * @brief Initialize mode manager
 * @param[out] mgr Mode manager context
 * @return OPRT_OK on success, error code otherwise
 */
static int mode_manager_init(mode_manager_t *mgr)
{
    if (!mgr) {
        return OPRT_INVALID_PARM;
    }
    
    // Check network availability
    mgr->network_available = g_reader_ctx.network_connected ? 1 : 0;
    
    // Check SD card availability
    mgr->sd_available = (tkl_fs_is_exist("/sdcard") == OPRT_OK) ? 1 : 0;
    
    // Choose default mode
    if (mgr->network_available) {
        mgr->current_mode = BROWSER_MODE_NETWORK;
        PR_NOTICE("Mode manager initialized: Network mode");
    } else if (mgr->sd_available) {
        mgr->current_mode = BROWSER_MODE_SD_CARD;
        PR_NOTICE("Mode manager initialized: SD card mode");
    } else {
        PR_ERR("No storage available");
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK;
}

/**
 * @brief Switch browser mode
 * @param[in,out] mgr Mode manager context
 * @return OPRT_OK on success, error code otherwise
 */
static int mode_manager_switch(mode_manager_t *mgr)
{
    if (!mgr) {
        return OPRT_INVALID_PARM;
    }
    
    // Save current mode state
    browser_mode_e old_mode = mgr->current_mode;
    
    // Switch mode
    if (mgr->current_mode == BROWSER_MODE_SD_CARD) {
        if (mgr->network_available) {
            mgr->current_mode = BROWSER_MODE_NETWORK;
            PR_NOTICE("Switched to network mode");
        } else {
            PR_WARN("Network not available");
            return OPRT_COM_ERROR;
        }
    } else {
        if (mgr->sd_available) {
            mgr->current_mode = BROWSER_MODE_SD_CARD;
            PR_NOTICE("Switched to SD card mode");
        } else {
            PR_WARN("SD card not available");
            return OPRT_COM_ERROR;
        }
    }
    
    // Show mode switch message
    Paint_Clear(WHITE);
    const char *mode_name = (mgr->current_mode == BROWSER_MODE_NETWORK) ? 
                            "Network Mode" : "SD Card Mode";
    Paint_DrawString_EN(150, 200, mode_name, &Font24, BLACK, WHITE);
    EPD_4in26_Display(g_image_buffer);
    tal_system_sleep(1000);
    
    return OPRT_OK;
}

/**
 * @brief Handle button events in network browser mode
 * @param[in] action Button action (single, double, long press)
 * @return OPRT_OK on success, error code otherwise
 */
static int handle_network_browser_button(button_action_e action)
{
    network_file_browser_t *browser = &g_reader_ctx.network_browser;
    
    if (!browser->files_loaded || browser->file_count == 0) {
        PR_WARN("No files loaded");
        return OPRT_COM_ERROR;
    }
    
    switch (action) {
        case BUTTON_ACTION_NEXT:
            // Next file (with wrap)
            browser->current_index++;
            if (browser->current_index >= browser->file_count) {
                browser->current_index = 0;
            }
            
            // Check if page changed
            int new_page = browser->current_index / (GRID_ROWS * GRID_COLS);
            if (new_page != browser->current_page) {
                free_page_thumbnails(browser, browser->current_page);
                browser->current_page = new_page;
                load_page_thumbnails(browser);
            }
            
            display_network_file_grid(browser);
            PR_NOTICE("Selected file %d: %s", browser->current_index, 
                     browser->files[browser->current_index].display_name);
            break;
            
        case BUTTON_ACTION_PREV:
            // Previous file (with wrap)
            browser->current_index--;
            if (browser->current_index < 0) {
                browser->current_index = browser->file_count - 1;
            }
            
            // Check if page changed
            new_page = browser->current_index / (GRID_ROWS * GRID_COLS);
            if (new_page != browser->current_page) {
                free_page_thumbnails(browser, browser->current_page);
                browser->current_page = new_page;
                load_page_thumbnails(browser);
            }
            
            display_network_file_grid(browser);
            PR_NOTICE("Selected file %d: %s", browser->current_index, 
                     browser->files[browser->current_index].display_name);
            break;
            
        case BUTTON_ACTION_OPEN:
            // Download selected file
            PR_NOTICE("Downloading file: %s", 
                     browser->files[browser->current_index].display_name);
            download_file_with_progress(&browser->files[browser->current_index]);
            display_network_file_grid(browser);
            break;
            
        default:
            break;
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
 * @brief Calculate page offsets for content (for in-memory content only)
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
 * @brief Load a page dynamically from file (for paged reading mode)
 */
static int load_page_from_file(int page_num)
{
    if (!g_reader_ctx.use_paged_reading) {
        return OPRT_INVALID_PARM;
    }
    
    // Free previous page content
    if (g_reader_ctx.content) {
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
        g_reader_ctx.content_size = 0;
    }
    
    // Read the requested page
    int rt = sd_read_text_page(g_reader_ctx.current_filepath, page_num, 
                               g_reader_ctx.page_size, 
                               &g_reader_ctx.content, 
                               &g_reader_ctx.content_size);
    
    if (rt != OPRT_OK) {
        PR_ERR("Failed to read page %d", page_num);
        return rt;
    }
    
    PR_DEBUG("Loaded page %d: %d bytes", page_num, g_reader_ctx.content_size);
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
            .body = (const uint8_t *)"",
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
    // For paged reading mode, load the page first
    if (g_reader_ctx.use_paged_reading) {
        if (load_page_from_file(g_reader_ctx.current_page) != OPRT_OK) {
            PR_ERR("Failed to load page");
            Paint_Clear(WHITE);
            Paint_DrawString_EN(100, 200, "Failed to load page", &Font24, BLACK, WHITE);
            EPD_4in26_Display(g_image_buffer);
            return;
        }
    }
    
    if (!g_reader_ctx.content) {
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
    
    // For paged reading, we display the entire loaded page content
    // For in-memory content, we use page offsets
    int page_start = 0;
    int page_end = g_reader_ctx.content_size;
    
    if (!g_reader_ctx.use_paged_reading && g_reader_ctx.page_offsets) {
        page_start = g_reader_ctx.page_offsets[g_reader_ctx.current_page];
        page_end = (g_reader_ctx.current_page + 1 < g_reader_ctx.total_pages) 
                   ? g_reader_ctx.page_offsets[g_reader_ctx.current_page + 1] 
                   : g_reader_ctx.content_size;
    }
    
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
 * @brief Display file browser
 */
static void display_file_browser(void)
{
    PR_NOTICE("Displaying file browser");
    
    Paint_Clear(WHITE);
    
    // Get current time
    time_t rawtime = tal_time_get_posix();
    struct tm *timeinfo = NULL;
    
    // Draw title and time at top
    char title_info[80];
    if (rawtime > 0) {
        timeinfo = localtime(&rawtime);
        if (timeinfo) {
            snprintf(title_info, sizeof(title_info), "SD Card %04d-%02d-%02d %02d:%02d", 
                     timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                     timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            snprintf(title_info, sizeof(title_info), "SD Card Files");
        }
    } else {
        snprintf(title_info, sizeof(title_info), "SD Card Files");
    }
    Paint_DrawString_EN(10, 2, title_info, &Font20, BLACK, WHITE);
    
    // Draw "贾" character at top left corner (GBK: 0xBCD6)
    draw_gbk_char24(450, 2, 0xBC, 0xD6, BLACK, WHITE);
    
    // Calculate how many files can fit on screen
    // Screen height: 800px (with ROTATE_90, logical height is 800)
    // Title: 24px, Bottom info: 20px, Margins: 10px
    // Available: 800 - 24 - 20 - 10 = 746px
    // Each file line: 32px (Font24 height + spacing)
    // Max files per screen: 746 / 32 = 23 files
    #define FILES_PER_SCREEN 23
    #define FILE_LINE_HEIGHT 32
    
    int start_idx = (g_reader_ctx.browser.current_index / FILES_PER_SCREEN) * FILES_PER_SCREEN;
    int y_pos = 30;  // Start after title
    
    for (int i = start_idx; i < g_reader_ctx.browser.file_count && i < start_idx + FILES_PER_SCREEN; i++) {
        char line[150];
        const char *type_str = "";
        
        switch (g_reader_ctx.browser.files[i].type) {
            case FILE_TYPE_TXT: type_str = "[TXT]"; break;
            case FILE_TYPE_BMP: type_str = "[BMP]"; break;
            case FILE_TYPE_PNG: type_str = "[PNG]"; break;
            case FILE_TYPE_JPG: type_str = "[JPG]"; break;
            default: type_str = "[???]"; break;
        }
        
        snprintf(line, sizeof(line), "%s %s", type_str, g_reader_ctx.browser.files[i].name);
        
        // Highlight current selection
        if (i == g_reader_ctx.browser.current_index) {
            Paint_DrawRectangle(5, y_pos - 2, 470, y_pos + 26, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            Paint_DrawString_EN(10, y_pos, line, &Font24, WHITE, BLACK);
        } else {
            Paint_DrawString_EN(10, y_pos, line, &Font24, BLACK, WHITE);
        }
        
        y_pos += FILE_LINE_HEIGHT;
    }
    
    // Draw instructions at bottom
    char info[100];
    snprintf(info, sizeof(info), "File %d/%d - Short:Next Long:Open", 
             g_reader_ctx.browser.current_index + 1, g_reader_ctx.browser.file_count);
    Paint_DrawString_EN(10, 775, info, &Font20, BLACK, WHITE);
    
    EPD_4in26_Display(g_image_buffer);
}

/**
 * @brief Open selected file
 */
static int open_selected_file(void)
{
    if (g_reader_ctx.browser.current_index >= g_reader_ctx.browser.file_count) {
        return OPRT_INVALID_PARM;
    }
    
    file_info_t *file = &g_reader_ctx.browser.files[g_reader_ctx.browser.current_index];
    char filepath[300];
    snprintf(filepath, sizeof(filepath), "%s/%s", SDCARD_MOUNT_PATH, file->name);
    
    PR_NOTICE("Opening file: %s (type=%d)", filepath, file->type);
    
    // Clear previous content
    if (g_reader_ctx.content) {
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
    }
    if (g_reader_ctx.page_offsets) {
        tal_free(g_reader_ctx.page_offsets);
        g_reader_ctx.page_offsets = NULL;
    }
    
    if (file->type == FILE_TYPE_TXT) {
        // Store filepath for paged reading
        strncpy(g_reader_ctx.current_filepath, filepath, sizeof(g_reader_ctx.current_filepath) - 1);
        
        // Use 1KB page size for paged reading (as requested by user)
        g_reader_ctx.page_size = 1024;  // 1KB per page
        
        // Get total page count
        int page_count = sd_get_page_count(filepath, g_reader_ctx.page_size);
        if (page_count <= 0) {
            PR_ERR("Failed to get page count");
            return OPRT_COM_ERROR;
        }
        
        g_reader_ctx.total_pages = page_count;
        g_reader_ctx.current_page = 0;
        g_reader_ctx.use_paged_reading = 1;
        g_reader_ctx.mode = MODE_TEXT_READER;
        g_reader_ctx.browser.is_file_open = 1;
        
        PR_NOTICE("Using paged reading: %d pages of %d bytes each", page_count, g_reader_ctx.page_size);
        
        display_page();
        
    } else if (file->type == FILE_TYPE_BMP) {
        // Display BMP image
        Paint_Clear(WHITE);
        
        int rt = sd_display_bmp_image(filepath);
        if (rt != OPRT_OK) {
            Paint_DrawString_EN(100, 200, "Failed to load image", &Font24, BLACK, WHITE);
        }
        
        // Draw filename at top
        Paint_DrawString_EN(10, 2, file->name, &Font20, BLACK, WHITE);
        Paint_DrawString_EN(10, 450, "Long press to return", &Font16, BLACK, WHITE);
        
        EPD_4in26_Display(g_image_buffer);
        
        g_reader_ctx.mode = MODE_IMAGE_VIEWER;
        g_reader_ctx.browser.is_file_open = 1;
        
    } else if (file->type == FILE_TYPE_PNG || file->type == FILE_TYPE_JPG) {
        // PNG/JPG not fully supported yet
        Paint_Clear(WHITE);
        Paint_DrawString_EN(80, 200, "PNG/JPG support coming soon", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(100, 240, "Please convert to BMP", &Font20, BLACK, WHITE);
        Paint_DrawString_EN(10, 450, "Long press to return", &Font16, BLACK, WHITE);
        EPD_4in26_Display(g_image_buffer);
        
        g_reader_ctx.mode = MODE_IMAGE_VIEWER;
        g_reader_ctx.browser.is_file_open = 1;
    }
    
    return OPRT_OK;
}

/**
 * @brief Close current file and return to browser
 */
static void close_current_file(void)
{
    PR_NOTICE("Closing current file");
    
    if (g_reader_ctx.content) {
        tal_free(g_reader_ctx.content);
        g_reader_ctx.content = NULL;
    }
    if (g_reader_ctx.page_offsets) {
        tal_free(g_reader_ctx.page_offsets);
        g_reader_ctx.page_offsets = NULL;
    }
    
    g_reader_ctx.mode = MODE_FILE_BROWSER;
    g_reader_ctx.browser.is_file_open = 0;
    g_reader_ctx.current_page = 0;
    g_reader_ctx.total_pages = 0;
    g_reader_ctx.use_paged_reading = 0;
    g_reader_ctx.current_filepath[0] = '\0';
    
    display_file_browser();
}

/**
 * @brief Button event handler with click counting
 */
static void button_handler(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *arg)
{
    PR_DEBUG("Button: %s, Event: %d", name, event);
    
    TIME_T current_time = tal_system_get_millisecond();
    
    if (event == TDL_BUTTON_PRESS_DOWN) {
        // Single click detected
        TIME_T time_since_last_click = current_time - g_reader_ctx.last_click_time;
        
        if (time_since_last_click < BUTTON_CLICK_TIMEOUT) {
            // Within timeout, increment click count
            g_reader_ctx.click_count++;
        } else {
            // Timeout expired, start new click sequence
            g_reader_ctx.click_count = 1;
        }
        
        g_reader_ctx.last_click_time = current_time;
        g_reader_ctx.action_trigger_time = current_time + BUTTON_ACTION_DELAY;
        g_reader_ctx.long_press_detected = 0;
        
        PR_DEBUG("Click count: %d", g_reader_ctx.click_count);
        
    } else if (event == TDL_BUTTON_LONG_PRESS_START) {
        // Long press detected
        g_reader_ctx.long_press_detected = 1;
        g_reader_ctx.action_trigger_time = current_time + BUTTON_ACTION_DELAY;
        
        PR_DEBUG("Long press detected");
    }
}

/**
 * @brief Process pending button action
 */
static button_action_e get_pending_button_action(void)
{
    TIME_T current_time = tal_system_get_millisecond();
    
    // Check if action delay has expired
    if (g_reader_ctx.action_trigger_time == 0 || current_time < g_reader_ctx.action_trigger_time) {
        return BUTTON_ACTION_NONE;
    }
    
    button_action_e action = BUTTON_ACTION_NONE;
    
    // Determine action based on long press or click count
    if (g_reader_ctx.long_press_detected) {
        action = BUTTON_ACTION_OPEN;
        PR_NOTICE("Action: OPEN (long press)");
    } else if (g_reader_ctx.click_count == 1) {
        action = BUTTON_ACTION_NEXT;
        PR_NOTICE("Action: NEXT (single click)");
    } else if (g_reader_ctx.click_count == 2) {
        action = BUTTON_ACTION_PREV;
        PR_NOTICE("Action: PREV (double click)");
    } else if (g_reader_ctx.click_count >= 3) {
        action = BUTTON_ACTION_MODE_SWITCH;
        PR_NOTICE("Action: MODE_SWITCH (triple click, count=%d)", g_reader_ctx.click_count);
    }
    
    // Reset state
    g_reader_ctx.click_count = 0;
    g_reader_ctx.long_press_detected = 0;
    g_reader_ctx.action_trigger_time = 0;
    
    return action;
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
    
    // TEMPORARILY DISABLED: Skip SD card to test network functionality only
    // Priority 1: Try SD card first
    PR_NOTICE("Skipping SD card check (testing network only)...");
    Paint_DrawString_EN(100, 300, "Network mode (SD disabled)", &Font24, BLACK, WHITE);
    EPD_4in26_Display(g_image_buffer);
    
    g_reader_ctx.sd_available = 0;
    
    /* COMMENTED OUT FOR NETWORK TESTING - SD card initialization disabled
    if (sd_card_init() == OPRT_OK) {
        PR_NOTICE("SD card initialized");
        g_reader_ctx.sd_available = 1;
        
        // Create sample files if needed
        sd_create_sample_files();
        
        // Scan files
        if (sd_scan_files(&g_reader_ctx.browser) == OPRT_OK && g_reader_ctx.browser.file_count > 0) {
            PR_NOTICE("Found %d files on SD card, using SD card mode", g_reader_ctx.browser.file_count);
            g_reader_ctx.mode = MODE_FILE_BROWSER;
            g_reader_ctx.content_loaded = 1;
            
            // Try to display wallpaper first
            if (display_wallpaper() == OPRT_OK) {
                PR_NOTICE("Wallpaper displayed, waiting 3 seconds...");
                tal_system_sleep(3000);  // Show wallpaper for 3 seconds
            } else {
                PR_NOTICE("No wallpaper found, will download after time sync");
            }
            
            // Try to sync time from network (non-blocking)
            PR_NOTICE("Attempting to sync time from network...");
            Paint_Clear(WHITE);
            Paint_DrawString_EN(120, 300, "Syncing time...", &Font24, BLACK, WHITE);
            EPD_4in26_Display_Fast(g_image_buffer);
            
            if (init_network() == OPRT_OK) {
                PR_NOTICE("Time synced successfully");
                
                // Download wallpaper if not exists or check fails
                BOOL_T wallpaper_exists = FALSE;
                int check_result = tkl_fs_is_exist(WALLPAPER_FILE, &wallpaper_exists);
                
                // Download if: check failed OR file doesn't exist
                if (check_result != OPRT_OK || !wallpaper_exists) {
                    if (check_result != OPRT_OK) {
                        PR_NOTICE("Wallpaper check failed, will download");
                    } else {
                        PR_NOTICE("Wallpaper not found, will download");
                    }
                    
                    PR_NOTICE("Downloading wallpaper...");
                    Paint_Clear(WHITE);
                    Paint_DrawString_EN(100, 300, "Downloading wallpaper...", &Font24, BLACK, WHITE);
                    EPD_4in26_Display_Fast(g_image_buffer);
                    
                    if (download_wallpaper() == OPRT_OK) {
                        PR_NOTICE("Wallpaper downloaded, displaying...");
                        display_wallpaper();
                        tal_system_sleep(3000);  // Show wallpaper for 3 seconds
                    } else {
                        PR_WARN("Failed to download wallpaper");
                    }
                } else {
                    PR_NOTICE("Wallpaper already exists, skipping download");
                }
            } else {
                PR_WARN("Time sync failed, will use system time");
            }
            
            // Initialize button
            if (init_button() != OPRT_OK) {
                PR_WARN("Button init failed, continuing without button control");
            }
            
            // Display file browser
            display_file_browser();
            
            // Enter main loop
            goto main_loop;
        } else {
            PR_WARN("No supported files found on SD card");
        }
    } else {
        PR_WARN("SD card not available");
    }
    END OF SD CARD COMMENT BLOCK */
    
    // Priority 2: Try network if SD card not available or empty
    PR_NOTICE("Trying network mode...");
    Paint_Clear(WHITE);
    Paint_DrawString_EN(150, 300, "Connecting to WiFi...", &Font24, BLACK, WHITE);
    EPD_4in26_Display(g_image_buffer);
    
    // Initialize network
    int network_ok = (init_network() == OPRT_OK);
    
    // Initialize mode manager
    mode_manager_t *mode_mgr = &g_reader_ctx.mode_manager;
    mode_manager_init(mode_mgr);
    
    // If network is available, try to fetch file list for network browser
    if (network_ok && mode_mgr->network_available) {
        PR_NOTICE("Network available, fetching file list...");
        Paint_Clear(WHITE);
        Paint_DrawString_EN(120, 300, "Loading file list...", &Font24, BLACK, WHITE);
        EPD_4in26_Display_Fast(g_image_buffer);
        
        char *json_response = NULL;
        if (fetch_file_list(&json_response) == OPRT_OK) {
            network_file_browser_t *net_browser = &g_reader_ctx.network_browser;
            if (parse_file_list_json(json_response, net_browser) == OPRT_OK) {
                // Convert filenames to GBK
                for (int i = 0; i < net_browser->file_count; i++) {
                    utf8_to_gbk(net_browser->files[i].original_filename, 
                               net_browser->files[i].display_name, 
                               MAX_FILENAME_LEN);
                }
                
                PR_NOTICE("Loaded %d network files", net_browser->file_count);
                
                // Load thumbnails for first page
                load_page_thumbnails(net_browser);
                
                // Initialize button
                if (init_button() != OPRT_OK) {
                    PR_WARN("Button init failed, continuing without button control");
                }
                
                // Display network file grid
                display_network_file_grid(net_browser);
                
                // Set mode to network browser
                g_reader_ctx.mode = MODE_NETWORK_BROWSER;
                g_reader_ctx.content_loaded = 1;
                
                tal_free(json_response);
                goto main_loop;
            }
            tal_free(json_response);
        }
        PR_WARN("Failed to fetch network file list, falling back to novel download");
    }
    
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
        // Priority 3: Use embedded novel if no network
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
    g_reader_ctx.mode = MODE_TEXT_READER;
    
    // Initialize button
    if (init_button() != OPRT_OK) {
        PR_WARN("Button init failed, continuing without button control");
    }
    
    // Display first page
    display_page();

main_loop:
    
    // Main loop - handle button events based on mode
    PR_NOTICE("Entering main loop");
    PR_NOTICE("Controls:");
    PR_NOTICE("  Single click: Next file/page");
    PR_NOTICE("  Double click: Previous file/page");
    PR_NOTICE("  Long press (2s): Open/Close file");
    PR_NOTICE("  Note: Wait 2s after clicks for action to execute");
    
    while (1) {
        // Check for pending button action
        button_action_e action = get_pending_button_action();
        
        if (action != BUTTON_ACTION_NONE) {
            
            // Handle mode switch action (triple click)
            if (action == BUTTON_ACTION_MODE_SWITCH) {
                mode_manager_t *mode_mgr = &g_reader_ctx.mode_manager;
                if (mode_manager_switch(mode_mgr) == OPRT_OK) {
                    // Switch successful, update display
                    if (mode_mgr->current_mode == BROWSER_MODE_NETWORK) {
                        g_reader_ctx.mode = MODE_NETWORK_BROWSER;
                        display_network_file_grid(&g_reader_ctx.network_browser);
                    } else {
                        g_reader_ctx.mode = MODE_FILE_BROWSER;
                        display_file_browser();
                    }
                }
                continue;
            }
            
            if (g_reader_ctx.mode == MODE_NETWORK_BROWSER) {
                // Network browser mode
                handle_network_browser_button(action);
                
            } else if (g_reader_ctx.mode == MODE_FILE_BROWSER) {
                // File browser mode
                if (action == BUTTON_ACTION_NEXT) {
                    // Single click: Next file
                    g_reader_ctx.browser.current_index++;
                    if (g_reader_ctx.browser.current_index >= g_reader_ctx.browser.file_count) {
                        g_reader_ctx.browser.current_index = 0;
                    }
                    display_file_browser();
                } else if (action == BUTTON_ACTION_PREV) {
                    // Double click: Previous file
                    g_reader_ctx.browser.current_index--;
                    if (g_reader_ctx.browser.current_index < 0) {
                        g_reader_ctx.browser.current_index = g_reader_ctx.browser.file_count - 1;
                    }
                    display_file_browser();
                } else if (action == BUTTON_ACTION_OPEN) {
                    // Long press: Open file
                    open_selected_file();
                }
                
            } else if (g_reader_ctx.mode == MODE_TEXT_READER) {
                // Text reader mode
                if (action == BUTTON_ACTION_NEXT) {
                    // Single click: Next page
                    if (g_reader_ctx.current_page < g_reader_ctx.total_pages - 1) {
                        g_reader_ctx.current_page++;
                        display_page();
                    } else {
                        PR_NOTICE("Already at last page");
                    }
                } else if (action == BUTTON_ACTION_PREV) {
                    // Double click: Previous page
                    if (g_reader_ctx.current_page > 0) {
                        g_reader_ctx.current_page--;
                        display_page();
                    } else {
                        PR_NOTICE("Already at first page");
                    }
                } else if (action == BUTTON_ACTION_OPEN) {
                    // Long press: Close file (if from SD card)
                    if (g_reader_ctx.sd_available && g_reader_ctx.browser.is_file_open) {
                        close_current_file();
                    }
                }
                
            } else if (g_reader_ctx.mode == MODE_IMAGE_VIEWER) {
                // Image viewer mode
                if (action == BUTTON_ACTION_OPEN) {
                    // Long press: Close image
                    if (g_reader_ctx.sd_available) {
                        close_current_file();
                    }
                }
            }
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
