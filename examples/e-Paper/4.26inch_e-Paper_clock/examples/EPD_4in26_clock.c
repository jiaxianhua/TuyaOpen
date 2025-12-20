/*****************************************************************************
* | File      	:   EPD_4in26_clock.c
* | Author      :   
* | Function    :   4.26inch e-paper clock demo with network time sync
* | Info        :   Display current date and time, update every second
*                   Sync time from HTTP server on startup
*----------------
* |	This version:   V2.0
* | Date        :   2024-12-20
* | Info        :   Added network time sync and centered large display
******************************************************************************/
#include "EPD_Test.h"
#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "http_client_interface.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
#include "netconn_wired.h"
#endif

/***********************************************************
*********************** macro define ***********************
***********************************************************/
#define TIME_SERVER_URL  "www.baidu.com"
#define TIME_SERVER_PATH "/"
#define HTTP_REQUEST_TIMEOUT 8000

#ifdef ENABLE_WIFI
#define DEFAULT_WIFI_SSID "1519"
#define DEFAULT_WIFI_PSWD "15889629702"
#endif


/***********************************************************
********************** variable define *********************
***********************************************************/
static int g_time_synced = 0;

/***********************************************************
********************** function define *********************
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
    
    // Use GET instead of HEAD - some servers don't handle HEAD properly
    http_client_header_t headers[] = {
        {.key = "User-Agent", .value = "Mozilla/5.0"},
        {.key = "Connection", .value = "close"}
    };
    
    // Try simple HTTP GET request
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
            
            PR_NOTICE("Response headers length: %d", http_response.headers_length);
            
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
 * @brief Get current time from system
 */
static time_t get_current_time(void)
{
    // Use Tuya's time function instead of standard time()
    return tal_time_get_posix();
}

/**
 * @brief Network link status callback
 */
static OPERATE_RET link_status_callback(void *data)
{
    static netmgr_status_e last_status = NETMGR_LINK_DOWN;
    static int sync_attempted = 0;
    netmgr_status_e status = (netmgr_status_e)data;
    
    if (status == last_status) {
        return OPRT_OK;
    }
    
    last_status = status;
    
    if (status == NETMGR_LINK_UP && !sync_attempted) {
        PR_NOTICE("Network connected, syncing time...");
        sync_attempted = 1;
        tal_system_sleep(2000);  // Wait for network to stabilize
        sync_time_from_http();
    } else if (status == NETMGR_LINK_DOWN) {
        PR_NOTICE("Network disconnected");
    }
    
    return OPRT_OK;
}

int EPD_clock_test(void)
{
    printf("EPD_4in26_clock Demo with Network Time Sync\r\n");
    
    // Initialize network first
    PR_NOTICE("Initializing network...");
    
    // Initialize required services
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tuya_tls_init();
    tuya_register_center_init();
    
    // Subscribe to network status changes
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "epd_clock", link_status_callback, SUBSCRIBE_TYPE_NORMAL);
    
#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
    TUYA_LwIP_Init();
#endif
    
    // Initialize network manager
    netmgr_type_e type = 0;
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    type |= NETCONN_WIFI;
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
    type |= NETCONN_WIRED;
#endif
    netmgr_init(type);
    
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    // Connect to WiFi
    PR_NOTICE("Connecting to WiFi: %s", DEFAULT_WIFI_SSID);
    netconn_wifi_info_t wifi_info = {0};
    strncpy(wifi_info.ssid, DEFAULT_WIFI_SSID, sizeof(wifi_info.ssid) - 1);
    strncpy(wifi_info.pswd, DEFAULT_WIFI_PSWD, sizeof(wifi_info.pswd) - 1);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
    
    // Wait for network connection and time sync
    PR_NOTICE("Waiting for network connection and time sync...");
    for (int i = 0; i < 15; i++) {  // Wait up to 15 seconds
        tal_system_sleep(1000);
        // Check if time has been synced
        if (g_time_synced) {
            PR_NOTICE("Time synced successfully!");
            break;
        }
    }
    
    // If sync failed, set a default time
    if (!g_time_synced) {
        PR_WARN("Network sync timeout, setting default time");
        struct tm default_time = {
            .tm_year = 2025 - 1900, .tm_mon = 11, .tm_mday = 20,
            .tm_hour = 21, .tm_min = 0, .tm_sec = 0, .tm_isdst = 0
        };
        time_t default_timestamp = mktime(&default_time);
        tal_time_set_posix(default_timestamp, 0);
        PR_NOTICE("Default time set");
    }
#endif
    
    if (DEV_Module_Init() != 0) {
        return -1;
    }

    printf("e-Paper Init and Clear...\r\n");
    EPD_4in26_Init();
    EPD_4in26_Clear();
    DEV_Delay_ms(500);

    // Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_4in26_WIDTH % 8 == 0) ? (EPD_4in26_WIDTH / 8) : (EPD_4in26_WIDTH / 8 + 1)) * EPD_4in26_HEIGHT;
    
    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, 0, WHITE);

    // Display initial full screen
    EPD_4in26_Init();
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    
    EPD_4in26_Display_Base(BlackImage);
    DEV_Delay_ms(500);

    // Prepare for time display - use partial refresh with smaller buffer
    printf("Starting clock display (updates every second)...\r\n");
    
    // Create smaller buffer for partial refresh - just for time display
    // Use 300x100 area for time only
    free(BlackImage);
    UDOUBLE PartImagesize = ((300 % 8 == 0) ? (300 / 8) : (300 / 8 + 1)) * 100;
    if ((BlackImage = (UBYTE *)malloc(PartImagesize)) == NULL) {
        printf("Failed to apply for partial display memory...\r\n");
        return -1;
    }
    Paint_NewImage(BlackImage, 300, 100, 0, WHITE);

    // Continuous clock display
    time_t rawtime;
    struct tm *timeinfo;
    char date_buffer[64];
    char time_buffer[64];
    char weekday_buffer[32];
    
    // Debug: print initial system time
    rawtime = tal_time_get_posix();
    PR_NOTICE("Clock started - system time: %ld", rawtime);
    
    while (1) {
        // Get current time from system
        rawtime = get_current_time();
        
        // Add debug output
        if (rawtime <= 0) {
            PR_ERR("Invalid time from system: %ld", rawtime);
            // Continue anyway to test display
            rawtime = 1734724800;  // 2024-12-20 20:00:00 as fallback
        }
        
        timeinfo = localtime(&rawtime);
        
        // Format date and time strings
        strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", timeinfo);
        strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
        
        // Get weekday
        char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        strncpy(weekday_buffer, weekdays[timeinfo->tm_wday], sizeof(weekday_buffer) - 1);
        
        // Clear the display area
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);
        
        // Partial refresh buffer: 300x100 pixels
        // Will be displayed at position (250, 190) on screen to center it
        // Screen: 800x480, Buffer: 300x100
        // Center position: x=(800-300)/2=250, y=(480-100)/2=190
        
        // Draw time in the center of the buffer
        // Time display (HH:MM:SS = 8 chars)
        int time_char_width = 17;  // Font24 width
        int time_total_width = strlen(time_buffer) * time_char_width;
        int time_x = (300 - time_total_width) / 2;  // Center in 300px width
        int time_y = 40;  // Center vertically in 100px height
        Paint_DrawString_EN(time_x, time_y, time_buffer, &Font24, BLACK, WHITE);
        
        // Draw date above time (smaller)
        int date_char_width = 11;  // Font16 width
        int date_total_width = strlen(date_buffer) * date_char_width;
        int date_x = (300 - date_total_width) / 2;
        int date_y = 10;
        Paint_DrawString_EN(date_x, date_y, date_buffer, &Font16, BLACK, WHITE);
        
        // Draw weekday below time (smaller)
        int weekday_char_width = 11;  // Font16 width
        int weekday_total_width = strlen(weekday_buffer) * weekday_char_width;
        int weekday_x = (300 - weekday_total_width) / 2;
        int weekday_y = 75;
        Paint_DrawString_EN(weekday_x, weekday_y, weekday_buffer, &Font16, BLACK, WHITE);
        
        // Display partial refresh at center of screen
        // Parameters: (buffer, x_start, y_start, width, height)
        PR_DEBUG("Displaying: %s %s", time_buffer, date_buffer);
        EPD_4in26_Display_Part(BlackImage, 250, 190, 300, 100);
        
        // Print to console
        printf("Time: %s %s %s %s\r\n", date_buffer, time_buffer, weekday_buffer, 
               g_time_synced ? "[Synced]" : "[Local]");
        
        // Wait 2 seconds between updates (give e-Paper time to complete)
        DEV_Delay_ms(2000);
    }

    // This code won't be reached due to infinite loop
    free(BlackImage);
    BlackImage = NULL;
    
    return 0;
}
