/**
 * @file main.c
 * @brief E-Paper E-Book Reader Main Application
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tkl_fs.h"
#include "board_com_api.h"

#include "sd_card_manager.h"
#include "page_manager.h"
#include "display_controller.h"
#include "navigation_controller.h"

#include <string.h>
#include <stdlib.h>

/***********************************************************
************************macro define************************
***********************************************************/
#define SDCARD_MOUNT_PATH "/sdcard"
#define SLEEP_TIMEOUT_MS  30000  // 30 seconds

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    STATE_INIT,
    STATE_MENU,
    STATE_READING,
    STATE_SLEEP,
    STATE_ERROR
} app_state_t;

typedef struct {
    app_state_t state;
    char **book_list;
    int book_count;
    int selected_book_index;
    char current_book_filename[256];
    char current_book_title[128];
    page_context_t page_ctx;
    uint32_t last_activity_time;
    volatile navigation_event_t pending_event;  // For button interrupt safety
    volatile int has_pending_event;
} app_context_t;

/***********************************************************
***********************variable define**********************
***********************************************************/
static app_context_t sg_app_ctx;
static THREAD_HANDLE sg_app_thread = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Handle error state
 */
static void ebook_reader_handle_error(const char *error_msg)
{
    PR_ERR("Error: %s", error_msg ? error_msg : "Unknown");
    sg_app_ctx.state = STATE_ERROR;
    if (error_msg) {
        display_show_error(error_msg);
    }
}

/**
 * @brief Load and display a book
 */
static int ebook_reader_load_book(const char *filename)
{
    if (filename == NULL) {
        return OPRT_INVALID_PARM;
    }
    
    PR_NOTICE("Loading book: %s", filename);
    
    // Open book file
    TUYA_FILE file = sd_manager_open_book(filename);
    if (file == NULL) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Cannot open file: %s", filename);
        ebook_reader_handle_error(error_msg);
        return OPRT_COM_ERROR;
    }
    
    // Get file size
    int file_size = sd_manager_get_file_size(file);
    if (file_size < 0 || file_size > SD_MANAGER_MAX_FILE_SIZE) {
        sd_manager_close_book(file);
        ebook_reader_handle_error("File too large or invalid");
        return OPRT_COM_ERROR;
    }
    
    // Read entire file - use heap allocation to avoid stack overflow
    char *content = (char *)tal_malloc(file_size + 1);
    if (content == NULL) {
        sd_manager_close_book(file);
        ebook_reader_handle_error("Memory allocation failed");
        return OPRT_MALLOC_FAILED;
    }
    
    int bytes_read = sd_manager_read_chunk(file, content, file_size);
    sd_manager_close_book(file);
    
    if (bytes_read != file_size) {
        tal_free(content);
        ebook_reader_handle_error("Failed to read file");
        return OPRT_COM_ERROR;
    }
    
    content[file_size] = '\0';
    
    // Initialize page manager
    page_manager_cleanup(&sg_app_ctx.page_ctx);
    int rt = page_manager_init(&sg_app_ctx.page_ctx, content, file_size);
    tal_free(content);  // Page manager makes its own copy
    
    if (rt != OPRT_OK) {
        ebook_reader_handle_error("Failed to initialize page manager");
        return rt;
    }
    
    // Calculate pagination
    page_manager_calculate_pages(&sg_app_ctx.page_ctx, 
                                  DISPLAY_WIDTH, 
                                  DISPLAY_CONTENT_HEIGHT,
                                  16,  // Font16 height
                                  8,   // Average char width
                                  DISPLAY_MARGIN);
    
    // Store current book info
    strncpy(sg_app_ctx.current_book_filename, filename, sizeof(sg_app_ctx.current_book_filename) - 1);
    
    // Extract title from filename (remove .txt extension)
    strncpy(sg_app_ctx.current_book_title, filename, sizeof(sg_app_ctx.current_book_title) - 1);
    char *ext = strstr(sg_app_ctx.current_book_title, ".txt");
    if (ext != NULL) {
        *ext = '\0';
    }
    
    // Display first page - use static buffers to avoid stack issues
    static char page_buffer[8192];
    static char page_info[64];
    
    page_manager_get_current_page(&sg_app_ctx.page_ctx, page_buffer, sizeof(page_buffer));
    page_manager_get_page_info(&sg_app_ctx.page_ctx, page_info, sizeof(page_info));
    
    display_render_page(sg_app_ctx.current_book_title, page_buffer, page_info);
    
    sg_app_ctx.state = STATE_READING;
    sg_app_ctx.last_activity_time = tal_system_get_millisecond();
    
    PR_NOTICE("Book loaded successfully: %d pages", sg_app_ctx.page_ctx.total_pages);
    return OPRT_OK;
}

/**
 * @brief Show book selection menu
 */
static void ebook_reader_show_menu(void)
{
    PR_NOTICE("========================================");
    PR_NOTICE("ebook_reader_show_menu called");
    PR_NOTICE("Book count: %d", sg_app_ctx.book_count);
    PR_NOTICE("========================================");
    
    if (sg_app_ctx.book_count == 0) {
        PR_WARN("No books available to display");
        ebook_reader_handle_error("No Books Available");
        return;
    }
    
    PR_DEBUG("Setting state to MENU");
    sg_app_ctx.state = STATE_MENU;
    
    PR_DEBUG("Calling display_render_menu");
    int rt = display_render_menu(sg_app_ctx.book_list, sg_app_ctx.book_count, sg_app_ctx.selected_book_index);
    PR_DEBUG("display_render_menu returned: %d", rt);
    
    sg_app_ctx.last_activity_time = tal_system_get_millisecond();
    PR_NOTICE("Menu displayed successfully");
}

/**
 * @brief Handle navigation events (called from button interrupt - keep minimal!)
 */
static void ebook_reader_handle_navigation(navigation_event_t event)
{
    // CRITICAL: This is called from button interrupt context with limited stack!
    // Just set a flag and return immediately
    PR_DEBUG("Nav: %d", event);
    sg_app_ctx.pending_event = event;
    sg_app_ctx.has_pending_event = 1;
}

/**
 * @brief Process pending navigation event (called from main loop)
 */
static void ebook_reader_process_navigation(void)
{
    if (!sg_app_ctx.has_pending_event) {
        return;
    }
    
    navigation_event_t event = sg_app_ctx.pending_event;
    sg_app_ctx.has_pending_event = 0;
    
    PR_DEBUG("Processing nav event: %d, state: %d", event, sg_app_ctx.state);
    
    sg_app_ctx.last_activity_time = tal_system_get_millisecond();
    
    // Wake from sleep if needed
    if (sg_app_ctx.state == STATE_SLEEP) {
        display_wake();
        sg_app_ctx.state = STATE_READING;
        return;
    }
    
    switch (sg_app_ctx.state) {
        case STATE_MENU:
            if (event == NAV_EVENT_SELECT || event == NAV_EVENT_NEXT_PAGE) {
                // Select current book
                if (sg_app_ctx.book_count > 0) {
                    ebook_reader_load_book(sg_app_ctx.book_list[sg_app_ctx.selected_book_index]);
                }
            } else if (event == NAV_EVENT_MENU_DOWN || event == NAV_EVENT_PREV_PAGE) {
                // Navigate menu (simplified - just cycle)
                sg_app_ctx.selected_book_index = (sg_app_ctx.selected_book_index + 1) % sg_app_ctx.book_count;
                ebook_reader_show_menu();
            }
            break;
            
        case STATE_READING:
            if (event == NAV_EVENT_NEXT_PAGE) {
                if (page_manager_next_page(&sg_app_ctx.page_ctx) == OPRT_OK) {
                    // Display next page
                    static char page_buffer[8192];
                    static char page_info[64];
                    
                    page_manager_get_current_page(&sg_app_ctx.page_ctx, page_buffer, sizeof(page_buffer));
                    page_manager_get_page_info(&sg_app_ctx.page_ctx, page_info, sizeof(page_info));
                    
                    display_render_page(sg_app_ctx.current_book_title, page_buffer, page_info);
                } else {
                    PR_NOTICE("End of book");
                    display_show_error("End of book");
                    tal_system_sleep(2000);
                    ebook_reader_show_menu();
                }
            } else if (event == NAV_EVENT_PREV_PAGE) {
                if (page_manager_prev_page(&sg_app_ctx.page_ctx) == OPRT_OK) {
                    // Display previous page
                    static char page_buffer[8192];
                    static char page_info[64];
                    
                    page_manager_get_current_page(&sg_app_ctx.page_ctx, page_buffer, sizeof(page_buffer));
                    page_manager_get_page_info(&sg_app_ctx.page_ctx, page_info, sizeof(page_info));
                    
                    display_render_page(sg_app_ctx.current_book_title, page_buffer, page_info);
                }
            } else if (event == NAV_EVENT_BACK_TO_MENU) {
                ebook_reader_show_menu();
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Initialize all subsystems
 */
static int ebook_reader_init(void)
{
    memset(&sg_app_ctx, 0, sizeof(app_context_t));
    sg_app_ctx.state = STATE_INIT;
    
    PR_NOTICE("========================================");
    PR_NOTICE("E-Paper E-Book Reader");
    PR_NOTICE("========================================");
    
    // Initialize display first
    PR_NOTICE("Initializing display...");
    int rt = display_init();
    if (rt != OPRT_OK) {
        PR_ERR("Failed to initialize display: %d", rt);
        return rt;
    }
    PR_NOTICE("Display init returned OK");
    
    // Show "Loading..." message
    PR_NOTICE("Showing 'Initializing...' message");
    display_show_error("Initializing...");
    PR_NOTICE("Message displayed, sleeping 500ms");
    tal_system_sleep(500);
    
    // Test: Try to display a simple pattern
    PR_NOTICE("TEST: Displaying test pattern...");
    display_clear();  // This should make the screen completely white
    PR_NOTICE("TEST: Screen should now be white");
    tal_system_sleep(3000);  // Wait 3 seconds to see the result
    
    // Initialize SD card with retry logic (non-blocking)
    PR_NOTICE("Starting SD card initialization...");
    int retry_count = 0;
    const int max_retries = 3;
    
    while (retry_count < max_retries) {
        PR_DEBUG("SD card mount attempt %d/%d", retry_count + 1, max_retries);
        rt = sd_manager_init(SDCARD_MOUNT_PATH);
        PR_DEBUG("sd_manager_init returned: %d", rt);
        
        if (rt == OPRT_OK) {
            PR_NOTICE("SD card mounted successfully");
            break;
        }
        
        retry_count++;
        PR_WARN("SD card mount failed (attempt %d/%d): %d", retry_count, max_retries, rt);
        
        if (retry_count < max_retries) {
            tal_system_sleep(1000);  // Wait 1 second before retry
        }
    }
    
    if (rt != OPRT_OK) {
        PR_ERR("SD card initialization failed after %d attempts", max_retries);
        PR_NOTICE("Displaying 'Insert SD Card' error");
        display_show_error("Insert SD Card");
        PR_NOTICE("Error displayed, sleeping 2s");
        tal_system_sleep(2000);
        // Don't return error - continue without SD card
        sg_app_ctx.book_count = 0;
        sg_app_ctx.book_list = NULL;
        PR_NOTICE("Continuing without SD card");
    } else {
        // Scan for books
        PR_NOTICE("SD card OK, scanning for books...");
        rt = sd_manager_scan_books(&sg_app_ctx.book_list, &sg_app_ctx.book_count);
        PR_DEBUG("sd_manager_scan_books returned: %d, count: %d", rt, sg_app_ctx.book_count);
        
        if (rt != OPRT_OK) {
            PR_ERR("Failed to scan books: %d", rt);
            sg_app_ctx.book_count = 0;
            sg_app_ctx.book_list = NULL;
            PR_NOTICE("Displaying 'Scan Failed' error");
            display_show_error("Scan Failed");
            tal_system_sleep(2000);
        } else if (sg_app_ctx.book_count == 0) {
            PR_WARN("No .txt books found on SD card");
            PR_NOTICE("Displaying 'No Books Found' error");
            display_show_error("No Books Found");
            tal_system_sleep(2000);
        } else {
            PR_NOTICE("Found %d books", sg_app_ctx.book_count);
            for (int i = 0; i < sg_app_ctx.book_count && i < 5; i++) {
                PR_DEBUG("  Book %d: %s", i, sg_app_ctx.book_list[i]);
            }
        }
    }
    
    // Initialize navigation
    PR_NOTICE("Initializing navigation...");
    rt = navigation_init();
    if (rt != OPRT_OK) {
        PR_WARN("Failed to initialize navigation: %d", rt);
    } else {
        PR_DEBUG("Navigation initialized successfully");
    }
    
    PR_DEBUG("Registering navigation callback");
    navigation_register_callback(ebook_reader_handle_navigation);
    
    PR_NOTICE("========================================");
    PR_NOTICE("Initialization complete!");
    PR_NOTICE("Book count: %d", sg_app_ctx.book_count);
    PR_NOTICE("========================================");
    return OPRT_OK;
}

/**
 * @brief Main application loop
 */
static void ebook_reader_run(void)
{
    PR_NOTICE("========================================");
    PR_NOTICE("Entering main application loop");
    PR_NOTICE("========================================");
    
    // Show initial menu if books are available
    if (sg_app_ctx.book_count > 0) {
        PR_NOTICE("Books available, showing menu");
        ebook_reader_show_menu();
    } else {
        PR_WARN("Starting without books - waiting for SD card");
        sg_app_ctx.state = STATE_ERROR;
    }
    
    PR_NOTICE("Entering infinite loop");
    // Main loop
    int loop_count = 0;
    while (1) {
        // Process any pending navigation events
        ebook_reader_process_navigation();
        
        if (loop_count % 10 == 0) {  // Log every 10 seconds
            PR_DEBUG("Main loop iteration %d, state=%d", loop_count, sg_app_ctx.state);
        }
        
        // Check for sleep timeout
        if (sg_app_ctx.state == STATE_READING || sg_app_ctx.state == STATE_MENU) {
            uint32_t current_time = tal_system_get_millisecond();
            uint32_t elapsed = current_time - sg_app_ctx.last_activity_time;
            
            if (elapsed > SLEEP_TIMEOUT_MS) {
                PR_NOTICE("Entering sleep mode");
                display_sleep();
                sg_app_ctx.state = STATE_SLEEP;
            }
        }
        
        tal_system_sleep(100);  // Check more frequently for button events
        loop_count++;
    }
}

/**
 * @brief Cleanup and shutdown
 */
static void ebook_reader_cleanup(void)
{
    page_manager_cleanup(&sg_app_ctx.page_ctx);
    sd_manager_free_book_list(sg_app_ctx.book_list, sg_app_ctx.book_count);
}

/**
 * @brief Application thread
 */
static void ebook_reader_thread(void *arg)
{
    // Initialize
    if (ebook_reader_init() != OPRT_OK) {
        PR_ERR("Initialization failed");
        return;
    }
    
    // Run main loop
    ebook_reader_run();
    
    // Cleanup
    ebook_reader_cleanup();
}

/**
 * @brief User main entry point
 */
void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;
    
    // Initialize log system
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    
    // Register hardware
    board_register_hardware();
    
    PR_NOTICE("Application information:");
    PR_NOTICE("Project name:        e-paper-ebook-reader");
    PR_NOTICE("App version:         1.0.0");
    PR_NOTICE("Compile time:        %s", __DATE__);
    
    // Create application thread
    THREAD_CFG_T thrd_param = {
        .stackDepth = 8192,
        .priority = THREAD_PRIO_2,
        .thrdname = "ebook_reader"
    };
    
    rt = tal_thread_create_and_start(&sg_app_thread, NULL, NULL, ebook_reader_thread, NULL, &thrd_param);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create application thread: %d", rt);
    }
}

/**
 * @brief Main entry point
 */
#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
    
    while (1) {
        tal_system_sleep(500);
    }
}
#else

/* Tuya thread handle */
static THREAD_HANDLE ty_app_thread = NULL;

/**
 * @brief Tuya app thread
 */
static void tuya_app_thread(void *arg)
{
    user_main();
    
    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif
