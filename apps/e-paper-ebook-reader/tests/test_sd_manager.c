/**
 * @file test_sd_manager.c
 * @brief Tests for SD Card Manager
 * 
 * Feature: e-paper-ebook-reader
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "sd_card_manager.h"
#include "tal_api.h"
#include "tkl_output.h"
#include <string.h>
#include <assert.h>

/**
 * @brief Property 1: Text file recognition and listing
 * Feature: e-paper-ebook-reader, Property 1: Text file recognition and listing
 * Validates: Requirements 1.4, 2.1
 * 
 * For any set of files on the SD card (mix of .txt and non-.txt files),
 * the system should correctly identify and list only files with .txt extension
 */
void test_property_txt_file_filtering(void)
{
    PR_NOTICE("=== Property Test 1: Text file recognition ===");
    
    // Test various filenames
    const char *test_files[] = {
        "book1.txt",      // Should match
        "book2.TXT",      // Should match (case insensitive)
        "book3.Txt",      // Should match
        "readme.md",      // Should not match
        "data.json",      // Should not match
        "image.png",      // Should not match
        "test.txt.bak",   // Should not match
        ".txt",           // Should not match (no name)
        "a.txt",          // Should match (minimum valid)
        "verylongbookname_with_many_characters.txt",  // Should match
        NULL
    };
    
    int passed = 0;
    int total = 0;
    
    for (int i = 0; test_files[i] != NULL; i++) {
        int result = sd_manager_is_txt_file(test_files[i]);
        int expected = 0;
        
        // Determine expected result
        size_t len = strlen(test_files[i]);
        if (len >= 5) {
            const char *ext = test_files[i] + len - 4;
            if (strcasecmp(ext, ".txt") == 0) {
                expected = 1;
            }
        }
        
        if (result == expected) {
            passed++;
            PR_DEBUG("PASS: %s -> %d", test_files[i], result);
        } else {
            PR_ERR("FAIL: %s -> got %d, expected %d", test_files[i], result, expected);
        }
        total++;
    }
    
    PR_NOTICE("Property Test 1: %d/%d tests passed", passed, total);
    assert(passed == total);
}

/**
 * @brief Unit test: SD manager initialization
 * Validates: Requirements 1.1, 1.2
 */
void test_sd_manager_init(void)
{
    PR_NOTICE("=== Unit Test: SD Manager Init ===");
    
    // Test with NULL parameter
    int result = sd_manager_init(NULL);
    assert(result != OPRT_OK);
    PR_DEBUG("PASS: NULL parameter rejected");
    
    // Note: Actual SD card mount test requires hardware
    PR_NOTICE("Unit Test: SD Manager Init - Basic validation passed");
}

/**
 * @brief Unit test: Book list memory management
 */
void test_book_list_memory(void)
{
    PR_NOTICE("=== Unit Test: Book List Memory ===");
    
    // Test freeing NULL list
    sd_manager_free_book_list(NULL, 0);
    PR_DEBUG("PASS: NULL list handled");
    
    // Test freeing empty list
    char **empty_list = NULL;
    sd_manager_free_book_list(empty_list, 0);
    PR_DEBUG("PASS: Empty list handled");
    
    PR_NOTICE("Unit Test: Book List Memory - Passed");
}

/**
 * @brief Run all SD manager tests
 */
void run_sd_manager_tests(void)
{
    PR_NOTICE("========================================");
    PR_NOTICE("Running SD Card Manager Tests");
    PR_NOTICE("========================================");
    
    test_property_txt_file_filtering();
    test_sd_manager_init();
    test_book_list_memory();
    
    PR_NOTICE("========================================");
    PR_NOTICE("SD Card Manager Tests Complete");
    PR_NOTICE("========================================");
}
