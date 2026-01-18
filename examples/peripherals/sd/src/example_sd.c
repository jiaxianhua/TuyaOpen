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
#include "sd_image_view.h"
#include "tal_time_service.h"

#include <stdio.h>
#include <string.h>

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
#define GPIO_PIN_RST    TUYA_GPIO_NUM_39  // P39

#define BUTTON_ACTIVE_LEVEL  TUYA_GPIO_LEVEL_LOW

// Display Layout
#define MAX_ITEMS_PER_PAGE 24
#define LIST_LINE_HEIGHT 30

#define TEXT_MARGIN_X 10
#define TEXT_MARGIN_TOP 80
#define TEXT_MARGIN_BOTTOM 10
#define TEXT_LINE_HEIGHT 24

#define PAGE_HISTORY_DEPTH 32
#define LINE_HISTORY_DEPTH 64

#define FILE_READ_WINDOW (96 * 1024)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    STATE_FILE_LIST,
    STATE_SHOW_FILE,
    STATE_ERROR
} APP_STATE_E;

typedef enum {
    VIEW_TEXT,
    VIEW_IMAGE
} VIEW_KIND_E;

typedef struct {
    char name[128]; // File name (UTF-8)
    BOOL_T is_dir;
} FILE_ITEM_T;

typedef struct {
    APP_STATE_E state;
    int current_page;
    int selected_index;
    int total_files;
    int total_pages;
    int items_per_page;
    FILE_ITEM_T files[MAX_ITEMS_PER_PAGE];
    int item_count_in_page;
    char current_path[256];
    char viewing_file[256]; // Full path of file being viewed
    VIEW_KIND_E view_kind;
    UWORD rotate;
    BOOL_T viewing_is_utf8;
    INT64_T viewing_offset;
    INT64_T viewing_size;
    INT64_T page_history[PAGE_HISTORY_DEPTH];
    int page_hist_len;
    INT64_T line_history[LINE_HISTORY_DEPTH];
    int line_hist_len;
    BOOL_T need_refresh;
} APP_CONTEXT_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static THREAD_HANDLE sg_sd_thrd_hdl;
static APP_CONTEXT_T sg_app_ctx;

// Button configuration
static TDL_BUTTON_HANDLE hdl_up, hdl_down, hdl_left, hdl_right, hdl_mid, hdl_set, hdl_rst;

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
        if (y + line_height > Paint.Height) break;
        
        // 3. Handle Visible Characters
        if (*p_text < 0x80) {
             // ASCII
             if (*p_text < 0x20) {
                 p_text++; // Skip other control chars
                 continue;
             }

             if (x + Font24.Width > Xstart + Width || x + Font24.Width > Paint.Width) {
                 x = Xstart;
                 y += line_height;
                 if (y + line_height > Ystart + Height || y + line_height > Paint.Height) break;
             }
             
             Paint_DrawChar(x, y, *p_text, &Font24, Color_Foreground, Color_Background);
             x += Font24.Width;
             p_text++;
        } else {
             // GBK (2 bytes)
             if (*(p_text + 1) == 0) break; // Incomplete

             if (x + 24 > Xstart + Width || x + 24 > Paint.Width) {
                 x = Xstart;
                 y += line_height;
                 if (y + line_height > Ystart + Height || y + line_height > Paint.Height) break;
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
    sg_app_ctx.item_count_in_page = 0;
    
    TUYA_DIR dir_hdl = NULL;
    if (tkl_dir_open(sg_app_ctx.current_path, &dir_hdl) != OPRT_OK) {
        PR_ERR("Failed to open dir: %s", sg_app_ctx.current_path);
        return;
    }

    TUYA_FILEINFO file_info = {0};
    int total_files = 0;
    int skip_files = sg_app_ctx.current_page * sg_app_ctx.items_per_page;
    int files_added = 0;

    // First pass: count total files (optional, but good for pagination)
    // For simplicity, we just scan linearly and pick the slice we want.
    // Optimization: We could cache total count.
    
    while (tkl_dir_read(dir_hdl, &file_info) == OPRT_OK) {
        char *name = NULL;
        if (tkl_dir_name(file_info, (const char**)&name) == OPRT_OK) {
            if (name[0] == '.') continue; // Skip hidden files
            
            if (total_files >= skip_files && files_added < sg_app_ctx.items_per_page) {
                strncpy(sg_app_ctx.files[files_added].name, name, 127);
                BOOL_T is_dir = FALSE;
                if (tkl_dir_is_directory(file_info, &is_dir) != OPRT_OK) {
                    is_dir = FALSE;
                }
                sg_app_ctx.files[files_added].is_dir = is_dir;
                files_added++;
            }
            total_files++;
        }
    }
    tkl_dir_close(dir_hdl);

    sg_app_ctx.total_files = total_files;
    sg_app_ctx.item_count_in_page = files_added;
    sg_app_ctx.total_pages = (total_files + sg_app_ctx.items_per_page - 1) / sg_app_ctx.items_per_page;
    
    if (sg_app_ctx.total_pages == 0) sg_app_ctx.total_pages = 1;
    
    // Adjust selected index if out of bounds
    if (sg_app_ctx.selected_index >= sg_app_ctx.item_count_in_page) {
        sg_app_ctx.selected_index = sg_app_ctx.item_count_in_page - 1;
    }
    if (sg_app_ctx.selected_index < 0) sg_app_ctx.selected_index = 0;
    
    PR_NOTICE("Scanned page %d: %d files. Total: %d", sg_app_ctx.current_page, sg_app_ctx.item_count_in_page, total_files);
}

static BOOL_T path_is_root(const char *path)
{
    return (strcmp(path, SDCARD_MOUNT_PATH) == 0);
}

static void path_to_parent(char *path, size_t path_len)
{
    if (path_is_root(path)) return;
    size_t n = strlen(path);
    while (n > 0 && path[n - 1] == '/') {
        path[n - 1] = 0;
        n--;
    }
    char *slash = strrchr(path, '/');
    if (!slash) {
        strncpy(path, SDCARD_MOUNT_PATH, path_len - 1);
        path[path_len - 1] = 0;
        return;
    }
    if (slash == path) {
        strncpy(path, SDCARD_MOUNT_PATH, path_len - 1);
        path[path_len - 1] = 0;
        return;
    }
    *slash = 0;
    if (strlen(path) == 0) {
        strncpy(path, SDCARD_MOUNT_PATH, path_len - 1);
        path[path_len - 1] = 0;
    }
}

static void path_join(char *out, size_t out_len, const char *base, const char *name)
{
    if (!base || !name) {
        if (out_len) out[0] = 0;
        return;
    }
    if (strcmp(base, "/") == 0) {
        snprintf(out, out_len, "/%s", name);
        return;
    }
    if (base[strlen(base) - 1] == '/') {
        snprintf(out, out_len, "%s%s", base, name);
    } else {
        snprintf(out, out_len, "%s/%s", base, name);
    }
}

static const char *file_ext(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot || dot == name) return "";
    return dot + 1;
}

static BOOL_T ext_eq(const char *ext, const char *rhs)
{
    if (!ext || !rhs) return FALSE;
    while (*ext && *rhs) {
        char a = *ext++;
        char b = *rhs++;
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return FALSE;
    }
    return (*ext == 0 && *rhs == 0);
}

static BOOL_T is_image_file(const char *name)
{
    const char *ext = file_ext(name);
    return ext_eq(ext, "bmp") || ext_eq(ext, "jpg") || ext_eq(ext, "jpeg") || ext_eq(ext, "png");
}

static void update_items_per_page(void)
{
    int screen_h = (sg_app_ctx.rotate == ROTATE_0 || sg_app_ctx.rotate == ROTATE_180) ? EPD_4in26_HEIGHT : EPD_4in26_WIDTH;
    int header_h = 45;
    int available = screen_h - header_h - 10;
    int n = available / LIST_LINE_HEIGHT;
    if (n < 1) n = 1;
    if (n > MAX_ITEMS_PER_PAGE) n = MAX_ITEMS_PER_PAGE;
    sg_app_ctx.items_per_page = n;
}

static BOOL_T detect_file_is_utf8(const char *path)
{
    TUYA_FILE f = tkl_fopen(path, "r");
    if (!f) return FALSE;
    uint8_t *buf = (uint8_t *)tal_malloc(4096);
    if (!buf) {
        tkl_fclose(f);
        return FALSE;
    }
    int len = tkl_fread(buf, 4096, f);
    if (len < 0) len = 0;
    BOOL_T r = is_utf8(buf, len);
    tal_free(buf);
    tkl_fclose(f);
    return r;
}

static void open_item_for_view(void)
{
    if (sg_app_ctx.item_count_in_page <= 0) return;
    FILE_ITEM_T *it = &sg_app_ctx.files[sg_app_ctx.selected_index];
    if (it->is_dir) return;
    char full_path[256];
    path_join(full_path, sizeof(full_path), sg_app_ctx.current_path, it->name);
    strncpy(sg_app_ctx.viewing_file, full_path, sizeof(sg_app_ctx.viewing_file) - 1);
    sg_app_ctx.viewing_file[sizeof(sg_app_ctx.viewing_file) - 1] = 0;
    sg_app_ctx.viewing_size = tkl_fgetsize(sg_app_ctx.viewing_file);
    sg_app_ctx.viewing_offset = 0;
    sg_app_ctx.page_hist_len = 0;
    sg_app_ctx.line_hist_len = 0;
    sg_app_ctx.view_kind = is_image_file(it->name) ? VIEW_IMAGE : VIEW_TEXT;
    sg_app_ctx.viewing_is_utf8 = (sg_app_ctx.view_kind == VIEW_TEXT) ? detect_file_is_utf8(sg_app_ctx.viewing_file) : FALSE;
}

static int display_image_1bit(const char *path, int x, int y, int w, int h)
{
    return sd_draw_image_1bit(path, x, y, w, h);
}

static const char *path_basename(const char *path)
{
    if (!path) return "";
    const char *p = strrchr(path, '/');
    return p ? (p + 1) : path;
}

static void format_time_hhmm(char out[6])
{
    POSIX_TM_S tm;
    if (tal_time_get_local_time_custom(0, &tm) == OPRT_OK) {
        snprintf(out, 6, "%02d:%02d", tm.tm_hour, tm.tm_min);
    } else {
        snprintf(out, 6, "--:--");
    }
}

#define BRAND_GBK "\xBC\xD6-AIDevLog"

static int gbk_pixel_width(const char *s)
{
    if (!s) return 0;
    int w = 0;
    const uint8_t *p = (const uint8_t *)s;
    while (*p) {
        if (*p < 0x80) {
            if (*p < 0x20) {
                p++;
                continue;
            }
            w += Font24.Width;
            p++;
        } else {
            if (*(p + 1) == 0) break;
            w += 24;
            p += 2;
        }
    }
    return w;
}

static size_t gbk_prefix_fit_px(const char *s, int max_px, int *out_px)
{
    if (!s || max_px <= 0) {
        if (out_px) *out_px = 0;
        return 0;
    }
    size_t i = 0;
    int w = 0;
    while (s[i]) {
        unsigned char c = (unsigned char)s[i];
        int cw = 0;
        size_t step = 1;
        if (c < 0x80) {
            if (c < 0x20) {
                i += 1;
                continue;
            }
            cw = Font24.Width;
            step = 1;
        } else {
            if (s[i + 1] == 0) break;
            cw = 24;
            step = 2;
        }
        if (w + cw > max_px) break;
        w += cw;
        i += step;
    }
    if (out_px) *out_px = w;
    return i;
}

static void build_preview_header(char *out, size_t out_len, int max_px, const char *file_path, int cur_page, int total_pages, int percent, const char time_hhmm[6])
{
    if (!out_len) return;
    const char *name = path_basename(file_path);
    char base[96];
    snprintf(base, sizeof(base), "%s", name ? name : "");

    char suffix[96];
    snprintf(suffix, sizeof(suffix), " %d/%d %02d%% %s %s", cur_page, total_pages, percent, time_hhmm, BRAND_GBK);
    if (gbk_pixel_width(suffix) > max_px) {
        snprintf(suffix, sizeof(suffix), " %02d%% %s %s", percent, time_hhmm, BRAND_GBK);
    }
    if (gbk_pixel_width(suffix) > max_px) {
        snprintf(suffix, sizeof(suffix), " %02d%% %s", percent, BRAND_GBK);
    }

    int suffix_px = gbk_pixel_width(suffix);
    int base_px_allow = max_px - suffix_px;
    if (base_px_allow <= 0) {
        const char *s = suffix;
        if (s[0] == ' ') s++;
        snprintf(out, out_len, "%s", s);
        return;
    }

    int base_px = 0;
    size_t base_keep = gbk_prefix_fit_px(base, base_px_allow, &base_px);

    char clipped[100];
    size_t base_len = strlen(base);
    if (base_keep < base_len) {
        int tilde_px = Font24.Width;
        if (base_px + tilde_px <= base_px_allow && base_keep + 1 < sizeof(clipped)) {
            memcpy(clipped, base, base_keep);
            clipped[base_keep] = '~';
            clipped[base_keep + 1] = 0;
        } else {
            snprintf(clipped, sizeof(clipped), "~");
        }
    } else {
        snprintf(clipped, sizeof(clipped), "%s", base);
    }

    snprintf(out, out_len, "%s%s", clipped, suffix);
}

static size_t utf8_seq_len(uint8_t c)
{
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static size_t advance_one_line_in_buf(const uint8_t *buf, size_t len, BOOL_T is_utf8_enc, int max_width)
{
    int x = 0;
    size_t i = 0;
    while (i < len) {
        uint8_t c = buf[i];
        if (c == '\n') {
            return i + 1;
        }
        if (c == '\r') {
            if (i + 1 < len && buf[i + 1] == '\n') return i + 2;
            return i + 1;
        }
        int glyph_w = 0;
        size_t step = 1;
        if (c < 0x80) {
            if (c < 0x20) {
                i += 1;
                continue;
            }
            glyph_w = Font24.Width;
            step = 1;
        } else {
            glyph_w = 24;
            if (is_utf8_enc) {
                step = utf8_seq_len(c);
                if (i + step > len) step = len - i;
            } else {
                step = (i + 2 <= len) ? 2 : 1;
            }
        }
        if (x > 0 && x + glyph_w > max_width) {
            return i;
        }
        x += glyph_w;
        i += step;
    }
    return i;
}

static INT64_T advance_lines_in_file(const char *path, INT64_T start_off, int lines, BOOL_T is_utf8_enc, int max_width)
{
    if (lines <= 0) return start_off;
    TUYA_FILE f = tkl_fopen(path, "r");
    if (!f) return start_off;
    if (tkl_fseek(f, start_off, SEEK_SET) != 0) {
        tkl_fclose(f);
        return start_off;
    }

    uint8_t *win = (uint8_t *)tal_malloc(FILE_READ_WINDOW);
    if (!win) {
        tkl_fclose(f);
        return start_off;
    }
    int rd = tkl_fread(win, FILE_READ_WINDOW, f);
    if (rd < 0) rd = 0;
    size_t pos = 0;
    for (int i = 0; i < lines && pos < (size_t)rd; i++) {
        size_t step = advance_one_line_in_buf(win + pos, (size_t)rd - pos, is_utf8_enc, max_width);
        if (step == 0) break;
        pos += step;
    }
    tal_free(win);
    tkl_fclose(f);
    return start_off + (INT64_T)pos;
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
    
    Paint_NewImage(BlackImage, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, sg_app_ctx.rotate, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    if (sg_app_ctx.state == STATE_FILE_LIST) {
        // Draw Title
        char title[96];
        snprintf(title, sizeof(title), "%s (%d/%d)", sg_app_ctx.current_path, sg_app_ctx.current_page + 1, sg_app_ctx.total_pages);
        Paint_DrawString_EN(10, 10, title, &Font24, BLACK, WHITE);
        Paint_DrawLine(10, 35, Paint.Width - 10, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        int y_pos = 45;
        for (int i = 0; i < sg_app_ctx.item_count_in_page; i++) {
            UWORD fg = BLACK;
            UWORD bg = WHITE;
            
            // Highlight selected
            if (i == sg_app_ctx.selected_index) {
                fg = WHITE;
                bg = BLACK;
                Paint_DrawRectangle(5, y_pos - 2, Paint.Width - 5, y_pos + 26, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            }

            char display_name[140];
            const char *name = sg_app_ctx.files[i].name;
            if (sg_app_ctx.files[i].is_dir) {
                snprintf(display_name, sizeof(display_name), "%s/", name);
                name = display_name;
            }
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
            
            y_pos += LIST_LINE_HEIGHT;
        }
        
        if (sg_app_ctx.item_count_in_page == 0) {
            Paint_DrawString_EN(10, 50, "No files found", &Font24, BLACK, WHITE);
        }
    } 
    else if (sg_app_ctx.state == STATE_SHOW_FILE) {
        int header_y = 10;
        int header_line_y = 35;
        int footer_h = 28;
        int footer_y = (Paint.Height > footer_h) ? (Paint.Height - footer_h) : 0;
        int content_y = header_line_y + 6;
        int content_h = footer_y - content_y - 2;
        if (content_h < TEXT_LINE_HEIGHT) content_h = TEXT_LINE_HEIGHT;

        char time_hhmm[6];
        format_time_hhmm(time_hhmm);

        int percent = 0;
        if (sg_app_ctx.view_kind == VIEW_TEXT && sg_app_ctx.viewing_size > 0) {
            percent = (int)((sg_app_ctx.viewing_offset * 100) / sg_app_ctx.viewing_size);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
        } else if (sg_app_ctx.view_kind == VIEW_IMAGE) {
            percent = 100;
        }

        int cur_page = 1;
        int total_pages = 1;
        int max_px = (int)Paint.Width - 20;

        int max_w = Paint.Width - 2 * TEXT_MARGIN_X;
        int lines_per_page = content_h / TEXT_LINE_HEIGHT;
        if (lines_per_page < 1) lines_per_page = 1;
        if (sg_app_ctx.view_kind == VIEW_TEXT) {
            INT64_T end_off = advance_lines_in_file(sg_app_ctx.viewing_file, sg_app_ctx.viewing_offset, lines_per_page, sg_app_ctx.viewing_is_utf8, max_w);
            INT64_T bytes_per_page = end_off - sg_app_ctx.viewing_offset;
            if (bytes_per_page <= 0) bytes_per_page = 1;
            if (sg_app_ctx.viewing_size > 0) {
                total_pages = (int)((sg_app_ctx.viewing_size + bytes_per_page - 1) / bytes_per_page);
                if (total_pages < 1) total_pages = 1;
            }
            cur_page = sg_app_ctx.page_hist_len + 1;
            if (cur_page < 1) cur_page = 1;
            if (total_pages < cur_page) total_pages = cur_page;
        }

        char header[160];
        build_preview_header(header, sizeof(header), max_px, sg_app_ctx.viewing_file, cur_page, total_pages, percent, time_hhmm);
        Paint_DrawString_CN_HZK24(10, header_y, header, BLACK, WHITE);
        Paint_DrawLine(10, header_line_y, Paint.Width - 10, header_line_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        if (sg_app_ctx.view_kind == VIEW_IMAGE) {
            int x = 0;
            int y = content_y;
            int w = Paint.Width;
            int h = content_h;
            if (display_image_1bit(sg_app_ctx.viewing_file, x, y, w, h) != 0) {
                Paint_DrawString_EN(10, 50, "Image decode failed/unsupported", &Font24, BLACK, WHITE);
            }
        } else {
            int avail_h = content_h;

            INT64_T end_off = advance_lines_in_file(sg_app_ctx.viewing_file, sg_app_ctx.viewing_offset, lines_per_page, sg_app_ctx.viewing_is_utf8, max_w);
            if (end_off < sg_app_ctx.viewing_offset) end_off = sg_app_ctx.viewing_offset;
            INT64_T need = end_off - sg_app_ctx.viewing_offset;
            if (need < 0) need = 0;
            if (need > (INT64_T)FILE_READ_WINDOW) need = (INT64_T)FILE_READ_WINDOW;

            TUYA_FILE f = tkl_fopen(sg_app_ctx.viewing_file, "r");
            if (f) {
                if (tkl_fseek(f, sg_app_ctx.viewing_offset, SEEK_SET) == 0) {
                    char *raw = (char *)tal_malloc((size_t)need + 1);
                    if (raw) {
                        int rd = tkl_fread(raw, (int)need, f);
                        if (rd < 0) rd = 0;
                        raw[rd] = 0;
                        if (sg_app_ctx.viewing_is_utf8) {
                            int gbk_len = rd * 2 + 2;
                            char *gbk = (char *)tal_malloc(gbk_len);
                            if (gbk) {
                                int out_len = utf8_to_gbk_buf((uint8_t *)raw, rd, (uint8_t *)gbk, gbk_len - 1);
                                if (out_len < 0) out_len = 0;
                                gbk[out_len] = 0;
                                Paint_DrawText_CN_HZK24_Adaptive(TEXT_MARGIN_X, content_y, max_w, avail_h, gbk, BLACK, WHITE);
                                tal_free(gbk);
                            } else {
                                Paint_DrawString_EN(10, 50, "Memory Error", &Font24, BLACK, WHITE);
                            }
                        } else {
                            Paint_DrawText_CN_HZK24_Adaptive(TEXT_MARGIN_X, content_y, max_w, avail_h, raw, BLACK, WHITE);
                        }
                        tal_free(raw);
                    } else {
                        Paint_DrawString_EN(10, 50, "Memory Error", &Font24, BLACK, WHITE);
                    }
                }
                tkl_fclose(f);
            } else {
                Paint_DrawString_EN(10, 50, "Error opening file.", &Font24, BLACK, WHITE);
            }

            char status[96];
            if (Paint.Width < 600) {
                snprintf(status, sizeof(status), "UP/DN  LT/RT  SET  RST  %d%%", percent);
            } else {
                snprintf(status, sizeof(status), "UP/DN line  LT/RT page  SET rot  RST back  %d%%", percent);
            }
            Paint_DrawString_EN(10, footer_y, status, &Font24, BLACK, WHITE);
        }
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
            } else if (sg_app_ctx.item_count_in_page > 0) {
                sg_app_ctx.selected_index = sg_app_ctx.item_count_in_page - 1;
                changed = TRUE;
            }
        } else if (strcmp(name, "DOWN") == 0) {
            if (sg_app_ctx.selected_index < sg_app_ctx.item_count_in_page - 1) {
                sg_app_ctx.selected_index++;
                changed = TRUE;
            } else {
                sg_app_ctx.selected_index = 0;
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
            if (sg_app_ctx.item_count_in_page > 0) {
                FILE_ITEM_T *it = &sg_app_ctx.files[sg_app_ctx.selected_index];
                if (it->is_dir) {
                    char next_path[256];
                    path_join(next_path, sizeof(next_path), sg_app_ctx.current_path, it->name);
                    strncpy(sg_app_ctx.current_path, next_path, sizeof(sg_app_ctx.current_path) - 1);
                    sg_app_ctx.current_path[sizeof(sg_app_ctx.current_path) - 1] = 0;
                    sg_app_ctx.current_page = 0;
                    sg_app_ctx.selected_index = 0;
                    scan_files();
                    changed = TRUE;
                } else {
                    sg_app_ctx.state = STATE_SHOW_FILE;
                    open_item_for_view();
                    changed = TRUE;
                }
            }
        } else if (strcmp(name, "RST") == 0) {
            if (!path_is_root(sg_app_ctx.current_path)) {
                path_to_parent(sg_app_ctx.current_path, sizeof(sg_app_ctx.current_path));
                sg_app_ctx.current_page = 0;
                sg_app_ctx.selected_index = 0;
                scan_files();
                changed = TRUE;
            }
        } else if (strcmp(name, "SET") == 0) {
            sg_app_ctx.rotate = (sg_app_ctx.rotate == ROTATE_0) ? ROTATE_90 : ROTATE_0;
            update_items_per_page();
            sg_app_ctx.current_page = 0;
            sg_app_ctx.selected_index = 0;
            scan_files();
            changed = TRUE;
        }
    } else if (sg_app_ctx.state == STATE_SHOW_FILE) {
        if (strcmp(name, "RST") == 0) {
            sg_app_ctx.state = STATE_FILE_LIST;
            changed = TRUE;
        } else if (strcmp(name, "SET") == 0) {
            sg_app_ctx.rotate = (sg_app_ctx.rotate == ROTATE_0) ? ROTATE_90 : ROTATE_0;
            update_items_per_page();
            changed = TRUE;
        } else if (sg_app_ctx.view_kind == VIEW_TEXT) {
            int screen_w = (sg_app_ctx.rotate == ROTATE_0 || sg_app_ctx.rotate == ROTATE_180) ? EPD_4in26_WIDTH : EPD_4in26_HEIGHT;
            int screen_h = (sg_app_ctx.rotate == ROTATE_0 || sg_app_ctx.rotate == ROTATE_180) ? EPD_4in26_HEIGHT : EPD_4in26_WIDTH;
            int header_line_y = 35;
            int footer_h = 28;
            int content_y = header_line_y + 6;
            int content_h = screen_h - content_y - footer_h - 2;
            if (content_h < TEXT_LINE_HEIGHT) content_h = TEXT_LINE_HEIGHT;
            int max_w = screen_w - 2 * TEXT_MARGIN_X;
            int lines_per_page = content_h / TEXT_LINE_HEIGHT;
            if (lines_per_page < 1) lines_per_page = 1;

            if (strcmp(name, "DOWN") == 0) {
                if (sg_app_ctx.line_hist_len < LINE_HISTORY_DEPTH) {
                    sg_app_ctx.line_history[sg_app_ctx.line_hist_len++] = sg_app_ctx.viewing_offset;
                }
                INT64_T next = advance_lines_in_file(sg_app_ctx.viewing_file, sg_app_ctx.viewing_offset, 1, sg_app_ctx.viewing_is_utf8, max_w);
                if (next > sg_app_ctx.viewing_offset) {
                    sg_app_ctx.viewing_offset = next;
                    changed = TRUE;
                } else if (sg_app_ctx.line_hist_len > 0) {
                    sg_app_ctx.line_hist_len--;
                }
            } else if (strcmp(name, "UP") == 0) {
                if (sg_app_ctx.line_hist_len > 0) {
                    sg_app_ctx.viewing_offset = sg_app_ctx.line_history[--sg_app_ctx.line_hist_len];
                    changed = TRUE;
                }
            } else if (strcmp(name, "RIGHT") == 0) {
                if (sg_app_ctx.page_hist_len < PAGE_HISTORY_DEPTH) {
                    sg_app_ctx.page_history[sg_app_ctx.page_hist_len++] = sg_app_ctx.viewing_offset;
                }
                sg_app_ctx.line_hist_len = 0;
                INT64_T next = advance_lines_in_file(sg_app_ctx.viewing_file, sg_app_ctx.viewing_offset, lines_per_page, sg_app_ctx.viewing_is_utf8, max_w);
                if (next > sg_app_ctx.viewing_offset) {
                    sg_app_ctx.viewing_offset = next;
                    changed = TRUE;
                } else if (sg_app_ctx.page_hist_len > 0) {
                    sg_app_ctx.page_hist_len--;
                }
            } else if (strcmp(name, "LEFT") == 0) {
                sg_app_ctx.line_hist_len = 0;
                if (sg_app_ctx.page_hist_len > 0) {
                    sg_app_ctx.viewing_offset = sg_app_ctx.page_history[--sg_app_ctx.page_hist_len];
                    changed = TRUE;
                }
            }
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
        {"RST", GPIO_PIN_RST, &hdl_rst}
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
    sg_app_ctx.rotate = ROTATE_0;
    update_items_per_page();
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
