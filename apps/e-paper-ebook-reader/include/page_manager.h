/**
 * @file page_manager.h
 * @brief Page Manager for E-Book Reader
 * 
 * Handles text pagination, calculates page breaks, and maintains reading position.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Page context structure
 */
typedef struct {
    char *book_content;        // Full book content in memory
    size_t content_length;     // Total content length
    size_t current_offset;     // Current reading position (byte offset)
    int current_page;          // Current page number (1-based)
    int total_pages;           // Total number of pages
    int chars_per_page;        // Characters that fit on one page
    int chars_per_line;        // Characters per line
    int lines_per_page;        // Lines per page
} page_context_t;

/**
 * @brief Initialize page manager with book content
 * 
 * @param[in,out] ctx Page context structure
 * @param[in] content Book content string
 * @param[in] length Content length
 * @return OPRT_OK on success, error code otherwise
 */
int page_manager_init(page_context_t *ctx, const char *content, size_t length);

/**
 * @brief Calculate total pages based on display dimensions and font
 * 
 * @param[in,out] ctx Page context structure
 * @param[in] screen_width Screen width in pixels
 * @param[in] screen_height Screen height in pixels (content area)
 * @param[in] font_height Font height in pixels
 * @param[in] font_width Average font width in pixels
 * @param[in] margin Margin in pixels
 * @return OPRT_OK on success, error code otherwise
 */
int page_manager_calculate_pages(page_context_t *ctx, int screen_width, 
                                  int screen_height, int font_height, 
                                  int font_width, int margin);

/**
 * @brief Get text for current page
 * 
 * @param[in] ctx Page context structure
 * @param[out] buffer Buffer to store page text
 * @param[in] buffer_size Size of buffer
 * @return Number of characters copied, negative on error
 */
int page_manager_get_current_page(page_context_t *ctx, char *buffer, size_t buffer_size);

/**
 * @brief Navigate to next page
 * 
 * @param[in,out] ctx Page context structure
 * @return OPRT_OK on success, OPRT_COM_ERROR if already on last page
 */
int page_manager_next_page(page_context_t *ctx);

/**
 * @brief Navigate to previous page
 * 
 * @param[in,out] ctx Page context structure
 * @return OPRT_OK on success, OPRT_COM_ERROR if already on first page
 */
int page_manager_prev_page(page_context_t *ctx);

/**
 * @brief Get page info string (e.g., "Page 5/23")
 * 
 * @param[in] ctx Page context structure
 * @param[out] buffer Buffer to store page info string
 * @param[in] buffer_size Size of buffer
 */
void page_manager_get_page_info(page_context_t *ctx, char *buffer, size_t buffer_size);

/**
 * @brief Check if on first page
 * 
 * @param[in] ctx Page context structure
 * @return 1 if on first page, 0 otherwise
 */
int page_manager_is_first_page(page_context_t *ctx);

/**
 * @brief Check if on last page
 * 
 * @param[in] ctx Page context structure
 * @return 1 if on last page, 0 otherwise
 */
int page_manager_is_last_page(page_context_t *ctx);

/**
 * @brief Free page manager resources
 * 
 * @param[in,out] ctx Page context structure
 */
void page_manager_cleanup(page_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __PAGE_MANAGER_H__ */
