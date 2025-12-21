/**
 * @file display_controller.h
 * @brief Display Controller for E-Book Reader
 * 
 * Manages e-Paper display operations including initialization, rendering, and power management.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __DISPLAY_CONTROLLER_H__
#define __DISPLAY_CONTROLLER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display layout constants
 */
#define DISPLAY_WIDTH           800
#define DISPLAY_HEIGHT          480
#define DISPLAY_HEADER_HEIGHT   30
#define DISPLAY_FOOTER_HEIGHT   30
#define DISPLAY_CONTENT_HEIGHT  (DISPLAY_HEIGHT - DISPLAY_HEADER_HEIGHT - DISPLAY_FOOTER_HEIGHT)
#define DISPLAY_MARGIN          10

/**
 * @brief Initialize display hardware
 * 
 * @return OPRT_OK on success, error code otherwise
 */
int display_init(void);

/**
 * @brief Clear entire display
 */
void display_clear(void);

/**
 * @brief Render text page with header and footer
 * 
 * @param[in] title Book title for header
 * @param[in] text Page text content
 * @param[in] page_info Page information string (e.g., "Page 5/23")
 * @return OPRT_OK on success, error code otherwise
 */
int display_render_page(const char *title, const char *text, const char *page_info);

/**
 * @brief Render book selection menu
 * 
 * @param[in] book_list Array of book filenames
 * @param[in] count Number of books
 * @param[in] selected_index Currently selected book index
 * @return OPRT_OK on success, error code otherwise
 */
int display_render_menu(char **book_list, int count, int selected_index);

/**
 * @brief Display error message
 * 
 * @param[in] message Error message to display
 */
void display_show_error(const char *message);

/**
 * @brief Enter sleep mode
 */
void display_sleep(void);

/**
 * @brief Wake from sleep mode
 */
void display_wake(void);

/**
 * @brief Get display buffer
 * 
 * @return Pointer to display buffer
 */
unsigned char *display_get_buffer(void);

/**
 * @brief Get buffer size
 * 
 * @return Buffer size in bytes
 */
size_t display_get_buffer_size(void);

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_CONTROLLER_H__ */
