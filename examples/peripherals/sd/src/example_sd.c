/**
 * @file example_sd.c
 * @version 0.1
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tkl_fs.h"
#include "utf8_to_gbk.h"
#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "hzk24.h"
#include "DEV_Config.h"
#include "tdl_button_manage.h"
#include "tdd_button_gpio.h"
#include "tkl_gpio.h"

#if defined(EBABLE_EXAMPLE_SD_PINMUX) && (EBABLE_EXAMPLE_SD_PINMUX == 1)
#include "tkl_pinmux.h"
#endif

#include "board_com_api.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TASK_SD_PRIORITY THREAD_PRIO_2
#define TASK_SD_SIZE     (1024 * 16)

#define SDCARD_MOUNT_PATH "/sdcard"

// GPIO pin definitions for 7-key
#define GPIO_PIN_UP     TUYA_GPIO_NUM_27  // P27
#define GPIO_PIN_DOWN   TUYA_GPIO_NUM_31  // P31
#define GPIO_PIN_LEFT   TUYA_GPIO_NUM_36  // P36
#define GPIO_PIN_RIGHT  TUYA_GPIO_NUM_30  // P30
#define GPIO_PIN_MID    TUYA_GPIO_NUM_37  // P37
#define GPIO_PIN_SET    TUYA_GPIO_NUM_32  // P32
#define GPIO_PIN_RET    TUYA_GPIO_NUM_39  // P39

#define BUTTON_ACTIVE_LEVEL  TUYA_GPIO_LEVEL_LOW

// Display Layout
#define FILES_PER_PAGE 10
#define LINE_HEIGHT 30
#define LIST_START_Y 45

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    STATE_FILE_LIST,
    STATE_SHOW_FILE,
    STATE_ERROR
} APP_STATE_E;

typedef struct {
    char name[128]; // File name (UTF-8)
    BOOL_T is_dir;
} FILE_ITEM_T;

typedef struct {
    APP_STATE_E state;
    int current_page;
    int selected_index; // 0 to file_count_in_page - 1
    int total_files;
    int total_pages;
    FILE_ITEM_T files[FILES_PER_PAGE];
    int file_count_in_page;
    char current_path[256];
    char viewing_file[256]; // Full path of file being viewed
    BOOL_T need_refresh;
} APP_CONTEXT_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static THREAD_HANDLE sg_sd_thrd_hdl;
static APP_CONTEXT_T sg_app_ctx;

// Button configuration
static TDL_BUTTON_HANDLE hdl_up, hdl_down, hdl_left, hdl_right, hdl_mid, hdl_set, hdl_ret;

/***********************************************************
***********************function define**********************
***********************************************************/

// Helper to draw Chinese string with HZK24
static void Paint_DrawString_CN_HZK24(UWORD Xstart, UWORD Ystart, const char * pString, UWORD Color_Foreground, UWORD Color_Background)
{
    // Extern declaration if header include fails
    extern int hzk24_get_font_data(uint8_t gb_high, uint8_t gb_low, uint8_t *buffer);
    
    const char * p_text = pString;
    int x = Xstart;
    int y = Ystart;
    
    while (*p_text != 0) {
        // Handle control characters
        if ((uint8_t)*p_text < 0x20) {
            if (*p_text == '\n') {
                x = Xstart;
                y += 24;
                p_text++;
                continue;
            }
            if (*p_text == '\r') {
                // Ignore CR if followed by LF, otherwise treat as newline
                if (*(p_text + 1) == '\n') {
                    p_text++;
                    continue;
                } else {
                    x = Xstart;
                    y += 24;
                    p_text++;
                    continue;
                }
            }
            if (*p_text == '\t') {
                // Tab = 4 spaces
                for (int i = 0; i < 4; i++) {
                    if (x + Font24.Width > Paint.WidthMemory) {
                        x = Xstart;
                        y += 24;
                        if (y + 24 > Paint.HeightMemory) break;
                    }
                    Paint_DrawChar(x, y, ' ', &Font24, Color_Foreground, Color_Background);
                    x += Font24.Width;
                }
                p_text++;
                continue;
            }
            // Skip other control characters
            p_text++;
            continue;
        }
        
        // Stop if out of vertical bounds
        if (y + 24 > Paint.HeightMemory) break;

        if ((uint8_t)*p_text < 0x80) {
            // ASCII
            // Check horizontal bounds
            if (x + Font24.Width > Paint.WidthMemory) {
                x = Xstart;
                y += 24;
                if (y + 24 > Paint.HeightMemory) break;
            }
            
            Paint_DrawChar(x, y, *p_text, &Font24, Color_Foreground, Color_Background);
            x += Font24.Width;
            p_text++;
        } else {
            // GBK - 2 bytes
            // Check horizontal bounds
            if (x + 24 > Paint.WidthMemory) {
                x = Xstart;
                y += 24;
                if (y + 24 > Paint.HeightMemory) break;
            }

            uint8_t gb_high = (uint8_t)*p_text;
            uint8_t gb_low = (uint8_t)*(p_text + 1);
            
            // Check bounds and validity
            if (gb_low == 0) break;
            
            uint8_t buffer[72]; // 24*24/8 = 72 bytes
            if (hzk24_get_font_data(gb_high, gb_low, buffer) == 0) {
                // Found font - Draw 24x24 bitmap
                for (int row = 0; row < 24; row++) {
                    for (int col_byte = 0; col_byte < 3; col_byte++) {
                        uint8_t data = buffer[row * 3 + col_byte];
                        for (int bit = 0; bit < 8; bit++) {
                            if (data & (0x80 >> bit)) {
                                Paint_SetPixel(x + col_byte * 8 + bit, y + row, Color_Foreground);
                            } else {
                                Paint_SetPixel(x + col_byte * 8 + bit, y + row, Color_Background);
                            }
                        }
                    }
                }
            } else {
                // Not found, draw '?'
                Paint_DrawChar(x, y, '?', &Font24, Color_Foreground, Color_Background);
            }
            
            x += 24;
            p_text += 2;
        }
    }
}

// Heuristic to detect UTF-8
static BOOL_T is_utf8(const uint8_t *data, int len) {
    int score = 0;
    int i = 0;
    while (i < len) {
        if (data[i] < 0x80) {
            i++;
            continue;
        }
        if ((data[i] & 0xE0) == 0xC0) { // 2 bytes
            if (i + 1 >= len) return score > 0; // Truncated at end: assume valid if we saw good chars
            if ((data[i + 1] & 0xC0) != 0x80) return FALSE;
            score++;
            i += 2;
        } else if ((data[i] & 0xF0) == 0xE0) { // 3 bytes
            if (i + 2 >= len) return score > 0; // Truncated at end
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) return FALSE;
            score++;
            i += 3;
        } else if ((data[i] & 0xF8) == 0xF0) { // 4 bytes
            if (i + 3 >= len) return score > 0; // Truncated at end
            if ((data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80 || (data[i + 3] & 0xC0) != 0x80) return FALSE;
            score++;
            i += 4;
        } else {
            return FALSE;
        }
    }
    return score > 0;
}

static void Paint_DrawText_CN_HZK24_Adaptive(UWORD Xstart, UWORD Ystart, UWORD Width, UWORD Height, const char * pString, UWORD Color_Foreground, UWORD Color_Background)
{
    extern int hzk24_get_font_data(uint8_t gb_high, uint8_t gb_low, uint8_t *buffer);
    
    const uint8_t *p_text = (const uint8_t *)pString;
    int x = Xstart;
    int y = Ystart;
    int line_height = 24;
    
    while (*p_text != 0) {
        // 1. Handle Control Characters
        if (*p_text == '\n') {
            x = Xstart;
            y += line_height;
            p_text++;
            continue;
        }
        if (*p_text == '\r') {
            if (*(p_text + 1) == '\n') {
                p_text++; // Skip CR, let \n handle the newline
            } else {
                 x = Xstart;
                 p_text++;
            }
            continue;
        }
        if (*p_text == '\t') {
             x += Font24.Width * 4;
             if (x > Xstart + Width) {
                 x = Xstart;
                 y += line_height;
             }
             p_text++;
             continue;
        }
        
        // 2. Check Vertical Bounds
        if (y + line_height > Ystart + Height) break;
        if (y + line_height > Paint.HeightMemory) break;
        
        // 3. Handle Visible Characters
        if (*p_text < 0x80) {
             // ASCII
             if (*p_text < 0x20) {
                 p_text++; // Skip other control chars
                 continue;
             }

             if (x + Font24.Width > Xstart + Width || x + Font24.Width > Paint.WidthMemory) {
                 x = Xstart;
                 y += line_height;
                 if (y + line_height > Ystart + Height || y + line_height > Paint.HeightMemory) break;
             }
             
             Paint_DrawChar(x, y, *p_text, &Font24, Color_Foreground, Color_Background);
             x += Font24.Width;
             p_text++;
        } else {
             // GBK (2 bytes)
             if (*(p_text + 1) == 0) break; // Incomplete

             if (x + 24 > Xstart + Width || x + 24 > Paint.WidthMemory) {
                 x = Xstart;
                 y += line_height;
                 if (y + line_height > Ystart + Height || y + line_height > Paint.HeightMemory) break;
             }
             
             uint8_t gb_high = *p_text;
             uint8_t gb_low = *(p_text + 1);
             
             uint8_t buffer[72];
             if (hzk24_get_font_data(gb_high, gb_low, buffer) == 0) {
                 for (int row = 0; row < 24; row++) {
                    for (int col_byte = 0; col_byte < 3; col_byte++) {
                        uint8_t data = buffer[row * 3 + col_byte];
                        for (int bit = 0; bit < 8; bit++) {
                            if (data & (0x80 >> bit)) {
                                Paint_SetPixel(x + col_byte * 8 + bit, y + row, Color_Foreground);
                            } else {
                                Paint_SetPixel(x + col_byte * 8 + bit, y + row, Color_Background);
                            }
                        }
                    }
                }
             } else {
                 // Not found, draw space instead of '?' to avoid garbled look for unsupported chars (like fullwidth space)
                 Paint_DrawChar(x, y, ' ', &Font24, Color_Foreground, Color_Background);
             }
             x += 24;
             p_text += 2;
        }
    }
}

// Convert UTF-8 to GBK for display if needed (simple heuristic)
// Note: In this system, filenames read from FATFS might already be GBK if not LFN-enabled or configured differently.
// But based on previous logs, we see some need conversion and some don't.
// We will assume filenames are UTF-8 in the struct, and we convert them for display if they are not ASCII.
// Wait, the previous logs showed: "Found file (GBK->UTF8): ... [Raw: ...]"
// This implies the raw name from `tkl_dir_read` was GBK.
// So `file_info` contains GBK. We should store GBK in our struct to easily open files, 
// but for display, we might want to check encoding.
// Actually, `Paint_DrawString_CN_HZK24` expects GBK. 
// So if the filesystem returns GBK, we can pass it directly to `Paint_DrawString_CN_HZK24`.
// If the filesystem returns UTF-8, we need to convert to GBK.
// Based on "Found file (GBK->UTF8)", the RAW was GBK. So we store Raw (GBK).

static void scan_files(void)
{
    sg_app_ctx.file_count_in_page = 0;
    
    TUYA_DIR dir_hdl = NULL;
    if (tkl_dir_open(sg_app_ctx.current_path, &dir_hdl) != OPRT_OK) {
        PR_ERR("Failed to open dir: %s", sg_app_ctx.current_path);
        return;
    }

    TUYA_FILEINFO file_info = {0};
    int total_files = 0;
    int skip_files = sg_app_ctx.current_page * FILES_PER_PAGE;
    int files_added = 0;

    // First pass: count total files (optional, but good for pagination)
    // For simplicity, we just scan linearly and pick the slice we want.
    // Optimization: We could cache total count.
    
    while (tkl_dir_read(dir_hdl, &file_info) == OPRT_OK) {
        char *name = NULL;
        if (tkl_dir_name(file_info, (const char**)&name) == OPRT_OK) {
            if (name[0] == '.') continue; // Skip hidden files
            
            if (total_files >= skip_files && files_added < FILES_PER_PAGE) {
                strncpy(sg_app_ctx.files[files_added].name, name, 127);
                // Simple is_dir check (TuyaOS might have a macro)
                // If not available, assume file for now or check mode if exposed
                sg_app_ctx.files[files_added].is_dir = FALSE; // Default to file
                files_added++;
            }
            total_files++;
        }
    }
    tkl_dir_close(dir_hdl);

    sg_app_ctx.total_files = total_files;
    sg_app_ctx.file_count_in_page = files_added;
    sg_app_ctx.total_pages = (total_files + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    
    if (sg_app_ctx.total_pages == 0) sg_app_ctx.total_pages = 1;
    
    // Adjust selected index if out of bounds
    if (sg_app_ctx.selected_index >= sg_app_ctx.file_count_in_page) {
        sg_app_ctx.selected_index = sg_app_ctx.file_count_in_page - 1;
    }
    if (sg_app_ctx.selected_index < 0) sg_app_ctx.selected_index = 0;
    
    PR_NOTICE("Scanned page %d: %d files. Total: %d", sg_app_ctx.current_page, sg_app_ctx.file_count_in_page, total_files);
}

static void refresh_ui(void)
{
    PR_NOTICE("Refreshing UI...");
    
    if(DEV_Module_Init() != 0) {
        PR_ERR("E-Paper DEV_Module_Init failed");
        return;
    }

    EPD_4in26_Init();
    // EPD_4in26_Clear(); // Avoid full clear every time to speed up? Or maybe needed for ghosting.
    // For now, keep clear.
    
    // Allocate memory
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_4in26_WIDTH % 8 == 0)? (EPD_4in26_WIDTH / 8 ): (EPD_4in26_WIDTH / 8 + 1)) * EPD_4in26_HEIGHT + 256;
    
    if((BlackImage = (UBYTE *)tal_malloc(Imagesize)) == NULL) {
        PR_ERR("Failed to allocate memory...");
        return;
    }
    
    Paint_NewImage(BlackImage, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, 0, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    if (sg_app_ctx.state == STATE_FILE_LIST) {
        // Draw Title
        char title[64];
        snprintf(title, sizeof(title), "Files (%d/%d)", sg_app_ctx.current_page + 1, sg_app_ctx.total_pages);
        Paint_DrawString_EN(10, 10, title, &Font24, BLACK, WHITE);
        Paint_DrawLine(10, 35, 790, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        int y_pos = LIST_START_Y;
        for (int i = 0; i < sg_app_ctx.file_count_in_page; i++) {
            UWORD fg = BLACK;
            UWORD bg = WHITE;
            
            // Highlight selected
            if (i == sg_app_ctx.selected_index) {
                fg = WHITE;
                bg = BLACK;
                Paint_DrawRectangle(5, y_pos - 2, 795, y_pos + 26, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            }

            char *name = sg_app_ctx.files[i].name;
            int is_ascii = 1;
            for(int j=0; name[j]; j++) {
                if((unsigned char)name[j] >= 0x80) {
                    is_ascii = 0;
                    break;
                }
            }

            if(is_ascii) {
                Paint_DrawString_EN(10, y_pos, name, &Font24, fg, bg);
            } else {
                Paint_DrawString_CN_HZK24(10, y_pos, name, fg, bg);
            }
            
            y_pos += LINE_HEIGHT;
        }
        
        if (sg_app_ctx.file_count_in_page == 0) {
            Paint_DrawString_EN(10, 50, "No files found", &Font24, BLACK, WHITE);
        }
    } 
    else if (sg_app_ctx.state == STATE_SHOW_FILE) {
        // Show file name
        Paint_DrawString_EN(10, 10, "Viewing:", &Font24, BLACK, WHITE);
        // Paint_DrawString_CN_HZK24(120, 10, sg_app_ctx.viewing_file, BLACK, WHITE); // Might be too long
        Paint_DrawLine(10, 35, 790, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        
        // Show content (Mock for now, or read first few bytes)
        // Since reading full file is complex for now, we just show path
        Paint_DrawString_EN(10, 50, "File Content Preview:", &Font24, BLACK, WHITE);
        
        char *fname = sg_app_ctx.files[sg_app_ctx.selected_index].name;
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", sg_app_ctx.current_path, fname);
        
        TUYA_FILE f = tkl_fopen(full_path, "r");
        if (f) {
            int buf_size = 4096;
            char *buf = tal_malloc(buf_size + 1);
            if (buf) {
                int len = tkl_fread(buf, buf_size, f);
                if (len < 0) len = 0;
                buf[len] = 0;
                tkl_fclose(f);
                
                if (is_utf8((uint8_t*)buf, len)) {
                     // Add safe null termination for conversion
                     buf[len] = 0; 
                     int gbk_buf_len = len * 2;
                     char *gbk_buf = tal_malloc(gbk_buf_len);
                     if (gbk_buf) {
                         int out_len = utf8_to_gbk_buf((uint8_t*)buf, len, (uint8_t*)gbk_buf, gbk_buf_len);
                         if (out_len > 0) {
                             gbk_buf[out_len] = 0;
                             Paint_DrawText_CN_HZK24_Adaptive(10, 80, 780, 400, gbk_buf, BLACK, WHITE);
                         } else {
                             // Fallback to raw buffer if conversion fails (might be GBK misidentified)
                             Paint_DrawText_CN_HZK24_Adaptive(10, 80, 780, 400, buf, BLACK, WHITE);
                         }
                         tal_free(gbk_buf);
                     } else {
                         Paint_DrawString_EN(10, 80, "Memory Error (GBK Buf)", &Font24, BLACK, WHITE);
                     }
                } else {
                     Paint_DrawText_CN_HZK24_Adaptive(10, 80, 780, 400, buf, BLACK, WHITE);
                }
                tal_free(buf);
            } else {
                Paint_DrawString_EN(10, 80, "Memory Error (Buf)", &Font24, BLACK, WHITE);
                tkl_fclose(f);
            }
        } else {
            Paint_DrawString_EN(10, 80, "Error opening file.", &Font24, BLACK, WHITE);
        }
        
        Paint_DrawString_EN(10, 560, "Press RET to back", &Font24, BLACK, WHITE);
    }

    EPD_4in26_Display(BlackImage);
    tal_free(BlackImage);
    EPD_4in26_Sleep();
}

static void button_cb(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *argc)
{
    if (event != TDL_BUTTON_PRESS_DOWN && event != TDL_BUTTON_LONG_PRESS_START) return;
    
    PR_NOTICE("Button %s pressed", name);
    BOOL_T changed = FALSE;

    if (sg_app_ctx.state == STATE_FILE_LIST) {
        if (strcmp(name, "UP") == 0) {
            if (sg_app_ctx.selected_index > 0) {
                sg_app_ctx.selected_index--;
                changed = TRUE;
            } else if (sg_app_ctx.file_count_in_page > 0) {
                sg_app_ctx.selected_index = sg_app_ctx.file_count_in_page - 1; // Wrap to bottom
                changed = TRUE;
            }
        } else if (strcmp(name, "DOWN") == 0) {
            if (sg_app_ctx.selected_index < sg_app_ctx.file_count_in_page - 1) {
                sg_app_ctx.selected_index++;
                changed = TRUE;
            } else {
                sg_app_ctx.selected_index = 0; // Wrap to top
                changed = TRUE;
            }
        } else if (strcmp(name, "LEFT") == 0) {
            if (sg_app_ctx.current_page > 0) {
                sg_app_ctx.current_page--;
                scan_files();
                changed = TRUE;
            }
        } else if (strcmp(name, "RIGHT") == 0) {
            if (sg_app_ctx.current_page < sg_app_ctx.total_pages - 1) {
                sg_app_ctx.current_page++;
                scan_files();
                changed = TRUE;
            }
        } else if (strcmp(name, "MID") == 0) {
            if (sg_app_ctx.file_count_in_page > 0) {
                sg_app_ctx.state = STATE_SHOW_FILE;
                changed = TRUE;
            }
        }
    } else if (sg_app_ctx.state == STATE_SHOW_FILE) {
        if (strcmp(name, "RET") == 0) {
            sg_app_ctx.state = STATE_FILE_LIST;
            changed = TRUE;
        }
    }

    if (changed) {
        sg_app_ctx.need_refresh = TRUE;
    }
}

static void init_buttons(void)
{
    TDL_BUTTON_CFG_T config = {
        .long_start_valid_time = 2000,
        .long_keep_timer = 500,
        .button_debounce_time = 50,
        .button_repeat_valid_count = 2,
        .button_repeat_valid_time = 500
    };

    BUTTON_GPIO_CFG_T gpio_cfg = {
        .level = BUTTON_ACTIVE_LEVEL,
        .mode = BUTTON_TIMER_SCAN_MODE,
        .pin_type.gpio_pull = TUYA_GPIO_PULLUP
    };

    struct { char *name; TUYA_GPIO_NUM_E pin; TDL_BUTTON_HANDLE *hdl; } btns[] = {
        {"UP", GPIO_PIN_UP, &hdl_up},
        {"DOWN", GPIO_PIN_DOWN, &hdl_down},
        {"LEFT", GPIO_PIN_LEFT, &hdl_left},
        {"RIGHT", GPIO_PIN_RIGHT, &hdl_right},
        {"MID", GPIO_PIN_MID, &hdl_mid},
        {"SET", GPIO_PIN_SET, &hdl_set},
        {"RET", GPIO_PIN_RET, &hdl_ret}
    };

    for (int i = 0; i < 7; i++) {
        gpio_cfg.pin = btns[i].pin;
        tdd_gpio_button_register(btns[i].name, &gpio_cfg);
        tdl_button_create(btns[i].name, &config, btns[i].hdl);
        tdl_button_event_register(*btns[i].hdl, TDL_BUTTON_PRESS_DOWN, button_cb);
        // tdl_button_event_register(*btns[i].hdl, TDL_BUTTON_LONG_PRESS_START, button_cb);
    }
}

static void __example_sd_task(void *param)
{
    // Pinmux config
    #if defined(EBABLE_EXAMPLE_SD_PINMUX) && (EBABLE_EXAMPLE_SD_PINMUX == 1)
    tkl_io_pinmux_config(EXAMPLE_SD_CLK_PIN, TUYA_SDIO_HOST_CLK);
    tkl_io_pinmux_config(EXAMPLE_SD_CMD_PIN, TUYA_SDIO_HOST_CMD);
    tkl_io_pinmux_config(EXAMPLE_SD_D0_PIN, TUYA_SDIO_HOST_D0);
    tkl_io_pinmux_config(EXAMPLE_SD_D1_PIN, TUYA_SDIO_HOST_D1);
    tkl_io_pinmux_config(EXAMPLE_SD_D2_PIN, TUYA_SDIO_HOST_D2);
    tkl_io_pinmux_config(EXAMPLE_SD_D3_PIN, TUYA_SDIO_HOST_D3);
    #endif

    // Init Keys
    init_buttons();

    // Mount SD
    int retry = 0;
    while (tkl_fs_mount(SDCARD_MOUNT_PATH, DEV_SDCARD) != OPRT_OK) {
        PR_ERR("Mount SD card failed, retrying...");
        tal_system_sleep(1000);
        retry++;
        if (retry > 10) break; // Don't block forever
    }

    // Init App State
    memset(&sg_app_ctx, 0, sizeof(sg_app_ctx));
    strcpy(sg_app_ctx.current_path, SDCARD_MOUNT_PATH);
    sg_app_ctx.state = STATE_FILE_LIST;
    sg_app_ctx.current_page = 0;
    sg_app_ctx.selected_index = 0;
    sg_app_ctx.need_refresh = TRUE;

    scan_files();

    while (1) {
        if (sg_app_ctx.need_refresh) {
            sg_app_ctx.need_refresh = FALSE;
            refresh_ui();
        }
        tal_system_sleep(100);
    }
}

/**
 * @brief user_main
 *
 * @return none
 */
void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    board_register_hardware();

    PR_NOTICE("SD Card Reader Demo Started");

    static THREAD_CFG_T thrd_param = {.priority = TASK_SD_PRIORITY, .stackDepth = TASK_SD_SIZE, .thrdname = "sd"};
    TUYA_CALL_ERR_LOG(tal_thread_create_and_start(&sg_sd_thrd_hdl, NULL, NULL, __example_sd_task, NULL, &thrd_param));
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
    while (1) {
        tal_system_sleep(500);
    }
}
#else
static THREAD_HANDLE ty_app_thread = NULL;
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
