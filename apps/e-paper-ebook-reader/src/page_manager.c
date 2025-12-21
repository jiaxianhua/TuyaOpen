/**
 * @file page_manager.c
 * @brief Page Manager Implementation
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "page_manager.h"
#include "tal_api.h"
#include "tkl_output.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Initialize page manager with book content
 */
int page_manager_init(page_context_t *ctx, const char *content, size_t length)
{
    if (ctx == NULL || content == NULL || length == 0) {
        PR_ERR("Invalid parameters");
        return OPRT_INVALID_PARM;
    }
    
    memset(ctx, 0, sizeof(page_context_t));
    
    // Allocate memory for book content
    ctx->book_content = (char *)tal_malloc(length + 1);
    if (ctx->book_content == NULL) {
        PR_ERR("Failed to allocate memory for book content");
        return OPRT_MALLOC_FAILED;
    }
    
    memcpy(ctx->book_content, content, length);
    ctx->book_content[length] = '\0';
    ctx->content_length = length;
    ctx->current_page = 1;
    ctx->current_offset = 0;
    
    PR_NOTICE("Page manager initialized with %d bytes", length);
    return OPRT_OK;
}

/**
 * @brief Calculate total pages based on display dimensions and font
 */
int page_manager_calculate_pages(page_context_t *ctx, int screen_width, 
                                  int screen_height, int font_height, 
                                  int font_width, int margin)
{
    if (ctx == NULL || ctx->book_content == NULL) {
        PR_ERR("Invalid context");
        return OPRT_INVALID_PARM;
    }
    
    // Calculate characters per line
    int usable_width = screen_width - (2 * margin);
    ctx->chars_per_line = usable_width / font_width;
    
    // Calculate lines per page
    ctx->lines_per_page = screen_height / font_height;
    
    // Calculate characters per page
    ctx->chars_per_page = ctx->chars_per_line * ctx->lines_per_page;
    
    // Calculate total pages
    if (ctx->chars_per_page > 0) {
        ctx->total_pages = (ctx->content_length + ctx->chars_per_page - 1) / ctx->chars_per_page;
        if (ctx->total_pages == 0) {
            ctx->total_pages = 1;
        }
    } else {
        ctx->total_pages = 1;
    }
    
    PR_NOTICE("Pagination: %d chars/line, %d lines/page, %d chars/page, %d total pages",
              ctx->chars_per_line, ctx->lines_per_page, ctx->chars_per_page, ctx->total_pages);
    
    return OPRT_OK;
}

/**
 * @brief Find word boundary for line wrapping
 */
static size_t find_line_break(const char *text, size_t max_len, size_t remaining)
{
    if (remaining <= max_len) {
        return remaining;
    }
    
    // Try to break at word boundary
    size_t break_pos = max_len;
    
    // Look backwards for a space
    for (size_t i = max_len; i > 0 && i > max_len - 20; i--) {
        if (isspace(text[i])) {
            break_pos = i;
            break;
        }
    }
    
    return break_pos;
}

/**
 * @brief Get text for current page
 */
int page_manager_get_current_page(page_context_t *ctx, char *buffer, size_t buffer_size)
{
    if (ctx == NULL || buffer == NULL || buffer_size == 0) {
        PR_ERR("Invalid parameters");
        return -1;
    }
    
    if (ctx->book_content == NULL) {
        PR_ERR("No book content loaded");
        return -1;
    }
    
    // Calculate start offset for current page
    size_t start_offset = (ctx->current_page - 1) * ctx->chars_per_page;
    
    if (start_offset >= ctx->content_length) {
        PR_ERR("Invalid page offset");
        return -1;
    }
    
    // Calculate how much content to copy
    size_t remaining = ctx->content_length - start_offset;
    size_t to_copy = (remaining < ctx->chars_per_page) ? remaining : ctx->chars_per_page;
    
    // Ensure we don't overflow buffer
    if (to_copy >= buffer_size) {
        to_copy = buffer_size - 1;
    }
    
    // Copy content with word wrapping
    size_t copied = 0;
    size_t src_pos = start_offset;
    int current_line = 0;
    
    while (copied < to_copy && current_line < ctx->lines_per_page && src_pos < ctx->content_length) {
        size_t line_remaining = ctx->content_length - src_pos;
        size_t line_len = find_line_break(&ctx->book_content[src_pos], 
                                          ctx->chars_per_line, 
                                          line_remaining);
        
        // Ensure we don't overflow
        if (copied + line_len >= buffer_size - 1) {
            line_len = buffer_size - copied - 1;
        }
        
        // Copy line
        memcpy(&buffer[copied], &ctx->book_content[src_pos], line_len);
        copied += line_len;
        src_pos += line_len;
        
        // Skip whitespace at line break
        while (src_pos < ctx->content_length && isspace(ctx->book_content[src_pos])) {
            src_pos++;
        }
        
        // Add newline if not at end
        if (current_line < ctx->lines_per_page - 1 && src_pos < ctx->content_length && copied < buffer_size - 2) {
            buffer[copied++] = '\n';
        }
        
        current_line++;
    }
    
    buffer[copied] = '\0';
    ctx->current_offset = start_offset;
    
    return copied;
}

/**
 * @brief Navigate to next page
 */
int page_manager_next_page(page_context_t *ctx)
{
    if (ctx == NULL) {
        return OPRT_INVALID_PARM;
    }
    
    if (ctx->current_page >= ctx->total_pages) {
        PR_DEBUG("Already on last page");
        return OPRT_COM_ERROR;
    }
    
    ctx->current_page++;
    PR_DEBUG("Navigated to page %d/%d", ctx->current_page, ctx->total_pages);
    
    return OPRT_OK;
}

/**
 * @brief Navigate to previous page
 */
int page_manager_prev_page(page_context_t *ctx)
{
    if (ctx == NULL) {
        return OPRT_INVALID_PARM;
    }
    
    if (ctx->current_page <= 1) {
        PR_DEBUG("Already on first page");
        return OPRT_COM_ERROR;
    }
    
    ctx->current_page--;
    PR_DEBUG("Navigated to page %d/%d", ctx->current_page, ctx->total_pages);
    
    return OPRT_OK;
}

/**
 * @brief Get page info string
 */
void page_manager_get_page_info(page_context_t *ctx, char *buffer, size_t buffer_size)
{
    if (ctx == NULL || buffer == NULL || buffer_size == 0) {
        return;
    }
    
    snprintf(buffer, buffer_size, "Page %d/%d", ctx->current_page, ctx->total_pages);
}

/**
 * @brief Check if on first page
 */
int page_manager_is_first_page(page_context_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    return (ctx->current_page == 1);
}

/**
 * @brief Check if on last page
 */
int page_manager_is_last_page(page_context_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    return (ctx->current_page >= ctx->total_pages);
}

/**
 * @brief Free page manager resources
 */
void page_manager_cleanup(page_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    if (ctx->book_content != NULL) {
        tal_free(ctx->book_content);
        ctx->book_content = NULL;
    }
    
    memset(ctx, 0, sizeof(page_context_t));
    PR_DEBUG("Page manager cleaned up");
}
