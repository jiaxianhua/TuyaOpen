/**
 * @file test_page_manager.c
 * @brief Tests for Page Manager
 * 
 * Feature: e-paper-ebook-reader
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "page_manager.h"
#include "tal_api.h"
#include "tkl_output.h"
#include <string.h>
#include <assert.h>

/**
 * @brief Property 5: Characters per page calculation
 * Feature: e-paper-ebook-reader, Property 5: Characters per page calculation
 * Validates: Requirements 3.2
 */
void test_property_chars_per_page(void)
{
    PR_NOTICE("=== Property Test 5: Characters per page calculation ===");
    
    page_context_t ctx;
    const char *content = "Test content for pagination";
    page_manager_init(&ctx, content, strlen(content));
    
    // Test calculation formula
    int width = 800, height = 420, font_h = 16, font_w = 8, margin = 10;
    page_manager_calculate_pages(&ctx, width, height, font_h, font_w, margin);
    
    int expected_chars_per_line = (width - 2 * margin) / font_w;
    int expected_lines_per_page = height / font_h;
    int expected_chars_per_page = expected_chars_per_line * expected_lines_per_page;
    
    assert(ctx.chars_per_line == expected_chars_per_line);
    assert(ctx.lines_per_page == expected_lines_per_page);
    assert(ctx.chars_per_page == expected_chars_per_page);
    
    PR_NOTICE("Property Test 5: PASSED");
    page_manager_cleanup(&ctx);
}

/**
 * @brief Property 7: Page position invariant
 * Feature: e-paper-ebook-reader, Property 7: Page position invariant
 * Validates: Requirements 3.4
 */
void test_property_page_position_invariant(void)
{
    PR_NOTICE("=== Property Test 7: Page position invariant ===");
    
    page_context_t ctx;
    const char *content = "This is a test book with multiple pages of content. "
                          "We need enough text to create several pages for testing navigation.";
    page_manager_init(&ctx, content, strlen(content));
    page_manager_calculate_pages(&ctx, 800, 420, 16, 8, 10);
    
    // Test invariant: 1 <= current_page <= total_pages
    assert(ctx.current_page >= 1);
    assert(ctx.current_page <= ctx.total_pages);
    
    // Navigate and check invariant holds
    if (ctx.total_pages > 1) {
        page_manager_next_page(&ctx);
        assert(ctx.current_page >= 1);
        assert(ctx.current_page <= ctx.total_pages);
        
        page_manager_prev_page(&ctx);
        assert(ctx.current_page >= 1);
        assert(ctx.current_page <= ctx.total_pages);
    }
    
    PR_NOTICE("Property Test 7: PASSED");
    page_manager_cleanup(&ctx);
}

/**
 * @brief Property 10: Page number format
 * Feature: e-paper-ebook-reader, Property 10: Page number format
 * Validates: Requirements 3.7
 */
void test_property_page_number_format(void)
{
    PR_NOTICE("=== Property Test 10: Page number format ===");
    
    page_context_t ctx;
    const char *content = "Test content";
    page_manager_init(&ctx, content, strlen(content));
    page_manager_calculate_pages(&ctx, 800, 420, 16, 8, 10);
    
    char page_info[64];
    page_manager_get_page_info(&ctx, page_info, sizeof(page_info));
    
    // Check format matches "Page X/Y"
    assert(strstr(page_info, "Page ") != NULL);
    assert(strstr(page_info, "/") != NULL);
    
    PR_NOTICE("Property Test 10: PASSED - Format: %s", page_info);
    page_manager_cleanup(&ctx);
}

/**
 * @brief Run all page manager tests
 */
void run_page_manager_tests(void)
{
    PR_NOTICE("========================================");
    PR_NOTICE("Running Page Manager Tests");
    PR_NOTICE("========================================");
    
    test_property_chars_per_page();
    test_property_page_position_invariant();
    test_property_page_number_format();
    
    PR_NOTICE("========================================");
    PR_NOTICE("Page Manager Tests Complete");
    PR_NOTICE("========================================");
}
