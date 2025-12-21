/**
 * @file display_controller.c
 * @brief Display Controller Implementation
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "display_controller.h"
#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "tal_api.h"
#include "tkl_output.h"
#include <string.h>
#include <stdlib.h>

static unsigned char *sg_display_buffer = NULL;
static int sg_display_initialized = 0;

/**
 * @brief Initialize display hardware
 */
int display_init(void)
{
    PR_NOTICE("Initializing e-Paper display...");
    
    // Initialize device module
    PR_DEBUG("Step 1: Initializing device module");
    if (DEV_Module_Init() != 0) {
        PR_ERR("Failed to initialize device module");
        return OPRT_COM_ERROR;
    }
    PR_DEBUG("Device module initialized");
    
    // Initialize e-Paper display
    PR_DEBUG("Step 2: Initializing e-Paper");
    EPD_4in26_Init();
    PR_DEBUG("e-Paper initialized");
    
    PR_DEBUG("Step 3: Clearing e-Paper");
    EPD_4in26_Clear();
    PR_DEBUG("e-Paper cleared");
    
    tal_system_sleep(500);
    
    // Calculate buffer size
    size_t buffer_size = ((EPD_4in26_WIDTH % 8 == 0) ? 
                          (EPD_4in26_WIDTH / 8) : 
                          (EPD_4in26_WIDTH / 8 + 1)) * EPD_4in26_HEIGHT;
    
    PR_DEBUG("Step 4: Allocating display buffer, size=%d", buffer_size);
    // Allocate display buffer
    sg_display_buffer = (unsigned char *)tal_malloc(buffer_size);
    if (sg_display_buffer == NULL) {
        PR_ERR("Failed to allocate display buffer");
        return OPRT_MALLOC_FAILED;
    }
    PR_DEBUG("Display buffer allocated at %p", sg_display_buffer);
    
    // Initialize paint library
    PR_DEBUG("Step 5: Initializing paint library");
    Paint_NewImage(sg_display_buffer, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, 0, WHITE);
    Paint_SelectImage(sg_display_buffer);
    Paint_Clear(WHITE);
    PR_DEBUG("Paint library initialized");
    
    sg_display_initialized = 1;
    PR_NOTICE("Display initialized successfully");
    
    return OPRT_OK;
}

/**
 * @brief Clear entire display
 */
void display_clear(void)
{
    if (!sg_display_initialized) {
        return;
    }
    
    Paint_Clear(WHITE);
    EPD_4in26_Display_Base(sg_display_buffer);
}

/**
 * @brief Render text page with header and footer
 */
int display_render_page(const char *title, const char *text, const char *page_info)
{
    if (!sg_display_initialized || sg_display_buffer == NULL) {
        PR_ERR("Display not initialized");
        return OPRT_COM_ERROR;
    }
    
    if (text == NULL || page_info == NULL) {
        PR_ERR("Invalid parameters");
        return OPRT_INVALID_PARM;
    }
    
    // Clear buffer
    Paint_Clear(WHITE);
    
    // Draw header with title
    if (title != NULL) {
        Paint_DrawString_EN(DISPLAY_MARGIN, 5, title, &Font16, WHITE, BLACK);
    }
    
    // Draw header line
    Paint_DrawLine(0, DISPLAY_HEADER_HEIGHT, EPD_4in26_WIDTH, DISPLAY_HEADER_HEIGHT, 
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    // Draw content text
    int y_pos = DISPLAY_HEADER_HEIGHT + 10;
    int x_pos = DISPLAY_MARGIN;
    
    // Simple text rendering (line by line)
    const char *line_start = text;
    const char *ptr = text;
    
    while (*ptr != '\0' && y_pos < (EPD_4in26_HEIGHT - DISPLAY_FOOTER_HEIGHT - 20)) {
        if (*ptr == '\n' || *(ptr + 1) == '\0') {
            // Draw line
            int line_len = ptr - line_start;
            if (*(ptr + 1) == '\0' && *ptr != '\n') {
                line_len++;
            }
            
            if (line_len > 0) {
                char line_buf[256];
                int copy_len = (line_len < 255) ? line_len : 255;
                strncpy(line_buf, line_start, copy_len);
                line_buf[copy_len] = '\0';
                
                Paint_DrawString_EN(x_pos, y_pos, line_buf, &Font16, WHITE, BLACK);
                y_pos += 18;  // Font16 height + spacing
            }
            
            line_start = ptr + 1;
        }
        ptr++;
    }
    
    // Draw footer line
    int footer_y = EPD_4in26_HEIGHT - DISPLAY_FOOTER_HEIGHT;
    Paint_DrawLine(0, footer_y, EPD_4in26_WIDTH, footer_y, 
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    // Draw page info
    Paint_DrawString_EN(EPD_4in26_WIDTH - 150, footer_y + 8, page_info, 
                        &Font16, WHITE, BLACK);
    
    // Display on e-Paper (use Display_Base for full refresh)
    EPD_4in26_Display_Base(sg_display_buffer);
    
    return OPRT_OK;
}

/**
 * @brief Render book selection menu
 */
int display_render_menu(char **book_list, int count, int selected_index)
{
    PR_DEBUG("display_render_menu called: count=%d, selected=%d", count, selected_index);
    
    if (!sg_display_initialized || sg_display_buffer == NULL) {
        PR_ERR("Display not initialized in render_menu");
        return OPRT_COM_ERROR;
    }
    
    // Clear buffer
    PR_DEBUG("Clearing paint buffer");
    Paint_Clear(WHITE);
    
    // Draw title
    PR_DEBUG("Drawing menu title");
    Paint_DrawString_EN(DISPLAY_MARGIN, 10, "Select a Book:", &Font20, WHITE, BLACK);
    
    // Draw line
    PR_DEBUG("Drawing separator line");
    Paint_DrawLine(0, 40, EPD_4in26_WIDTH, 40, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    if (count == 0) {
        PR_DEBUG("Drawing 'no books' message");
        Paint_DrawString_EN(DISPLAY_MARGIN, 60, "No books found", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(DISPLAY_MARGIN, 90, "Please add .txt files to SD card", 
                            &Font16, WHITE, BLACK);
    } else {
        // Draw book list
        int y_pos = 60;
        int max_display = 15;  // Maximum books to display
        
        PR_DEBUG("Drawing %d books", count < max_display ? count : max_display);
        for (int i = 0; i < count && i < max_display; i++) {
            char display_text[128];
            
            if (i == selected_index) {
                snprintf(display_text, sizeof(display_text), "> %s", book_list[i]);
                PR_DEBUG("  [%d] %s (selected)", i, book_list[i]);
                Paint_DrawString_EN(DISPLAY_MARGIN, y_pos, display_text, &Font16, BLACK, WHITE);
            } else {
                snprintf(display_text, sizeof(display_text), "  %s", book_list[i]);
                PR_DEBUG("  [%d] %s", i, book_list[i]);
                Paint_DrawString_EN(DISPLAY_MARGIN, y_pos, display_text, &Font16, WHITE, BLACK);
            }
            
            y_pos += 25;
        }
        
        if (count > max_display) {
            PR_DEBUG("Drawing 'more books' indicator");
            Paint_DrawString_EN(DISPLAY_MARGIN, y_pos, "... more books available", 
                                &Font12, WHITE, BLACK);
        }
    }
    
    // Display on e-Paper (use Display_Base for full refresh)
    PR_DEBUG("Sending buffer to e-Paper display");
    EPD_4in26_Display_Base(sg_display_buffer);
    PR_DEBUG("display_render_menu completed successfully");
    
    return OPRT_OK;
}

/**
 * @brief Display error message
 */
void display_show_error(const char *message)
{
    PR_DEBUG("display_show_error called: %s", message ? message : "NULL");
    
    if (!sg_display_initialized || sg_display_buffer == NULL) {
        PR_ERR("Display not initialized in show_error");
        return;
    }
    
    PR_DEBUG("Clearing display buffer");
    Paint_Clear(WHITE);
    
    // Draw error title
    PR_DEBUG("Drawing error title");
    Paint_DrawString_EN(DISPLAY_MARGIN, 100, "ERROR", &Font24, WHITE, BLACK);
    
    // Draw error message
    if (message != NULL) {
        PR_DEBUG("Drawing error message: %s", message);
        Paint_DrawString_EN(DISPLAY_MARGIN, 150, message, &Font16, WHITE, BLACK);
    }
    
    PR_DEBUG("Displaying on e-Paper");
    EPD_4in26_Display_Base(sg_display_buffer);
    PR_DEBUG("display_show_error completed");
}

/**
 * @brief Enter sleep mode
 */
void display_sleep(void)
{
    if (!sg_display_initialized) {
        return;
    }
    
    PR_DEBUG("Display entering sleep mode");
    EPD_4in26_Sleep();
}

/**
 * @brief Wake from sleep mode
 */
void display_wake(void)
{
    if (!sg_display_initialized) {
        return;
    }
    
    PR_DEBUG("Display waking from sleep");
    EPD_4in26_Init();
}

/**
 * @brief Get display buffer
 */
unsigned char *display_get_buffer(void)
{
    return sg_display_buffer;
}

/**
 * @brief Get buffer size
 */
size_t display_get_buffer_size(void)
{
    return ((EPD_4in26_WIDTH % 8 == 0) ? 
            (EPD_4in26_WIDTH / 8) : 
            (EPD_4in26_WIDTH / 8 + 1)) * EPD_4in26_HEIGHT;
}
