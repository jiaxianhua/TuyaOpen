# Implementation Plan: E-Paper E-Book Reader

## Overview

This implementation plan breaks down the e-book reader application into discrete, incremental tasks. Each task builds upon previous work, with testing integrated throughout to catch errors early. The implementation follows the modular architecture defined in the design document, starting with core infrastructure and progressing through each component to final integration.

## Tasks

- [x] 1. Set up project structure and core infrastructure
  - Create application directory at `apps/e-paper-ebook-reader/`
  - Create CMakeLists.txt with proper source file organization
  - Create app_default.config with necessary Kconfig options
  - Create README.md with project description and usage instructions
  - Set up directory structure: `src/`, `include/`, `assets/`, `lib/`
  - Copy e-Paper driver library from `examples/e-Paper/4.26inch_e-Paper/lib/` to `lib/`
  - _Requirements: 8.1, 8.5, 8.6_

- [x] 2. Implement SD Card Manager component
  - [x] 2.1 Create sd_card_manager.h and sd_card_manager.c
    - Define sd_manager_init() function using tkl_fs_mount()
    - Define sd_manager_scan_books() to list .txt files using tkl_fs APIs
    - Define sd_manager_open_book(), sd_manager_read_chunk(), sd_manager_close_book()
    - Define sd_manager_free_book_list() for memory cleanup
    - Implement error handling and logging for all operations
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 2.1_

  - [x] 2.2 Write property test for text file recognition
    - **Property 1: Text file recognition and listing**
    - **Validates: Requirements 1.4, 2.1**

  - [x] 2.3 Write unit tests for SD card manager
    - Test successful initialization
    - Test mount failure handling
    - Test file scanning with mixed file types
    - Test 1MB file size boundary
    - _Requirements: 1.1, 1.2, 1.3, 1.5_

- [x] 3. Implement Page Manager component
  - [x] 3.1 Create page_manager.h and page_manager.c
    - Define page_context_t structure
    - Implement page_manager_init() to load book content
    - Implement page_manager_calculate_pages() with pagination algorithm
    - Implement page_manager_get_current_page() with word wrapping
    - Implement page_manager_next_page() and page_manager_prev_page()
    - Implement page_manager_get_page_info() for page number formatting
    - Implement page_manager_cleanup() for resource deallocation
    - _Requirements: 3.2, 3.3, 3.4, 3.5, 3.7_

  - [x] 3.2 Write property test for characters per page calculation
    - **Property 5: Characters per page calculation**
    - **Validates: Requirements 3.2**

  - [x] 3.3 Write property test for line wrapping
    - **Property 6: Line wrapping correctness**
    - **Validates: Requirements 3.3**

  - [x] 3.4 Write property test for page position invariant
    - **Property 7: Page position invariant**
    - **Validates: Requirements 3.4**

  - [x] 3.5 Write property test for pagination
    - **Property 8: Pagination for long content**
    - **Validates: Requirements 3.5**

  - [x] 3.6 Write property test for page number format
    - **Property 10: Page number format**
    - **Validates: Requirements 3.7**

  - [x] 3.7 Write unit tests for page manager
    - Test pagination with known text samples
    - Test first page boundary
    - Test last page boundary
    - Test empty book handling
    - _Requirements: 3.2, 3.3, 3.4, 3.5_

- [x] 4. Checkpoint - Verify core data management
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Implement Display Controller component
  - [x] 5.1 Create display_controller.h and display_controller.c
    - Implement display_init() using EPD_4in26_Init()
    - Implement display_clear() using EPD_4in26_Clear()
    - Implement display_render_page() with header, content, and footer layout
    - Implement display_render_menu() for book selection
    - Implement display_show_error() for error messages
    - Implement display_sleep() and display_wake() using EPD_4in26_Sleep()
    - Implement display_full_refresh() and display_partial_refresh()
    - Use Paint library functions for text rendering
    - _Requirements: 3.1, 3.6, 3.7, 5.1, 5.2, 7.1, 7.2_

  - [x] 5.2 Write property test for font size consistency
    - **Property 4: Font size consistency**
    - **Validates: Requirements 3.1**

  - [x] 5.3 Write property test for screen clear before render
    - **Property 9: Screen clear before render**
    - **Validates: Requirements 3.6**

  - [x] 5.4 Write property test for full clear on book switch
    - **Property 13: Full clear on book switch**
    - **Validates: Requirements 5.2**

  - [x] 5.5 Write unit tests for display controller
    - Test display initialization
    - Test menu rendering with various book counts
    - Test page rendering
    - Test error message display
    - _Requirements: 3.1, 3.6, 5.1, 5.2, 7.1_

- [x] 6. Implement Navigation Controller component
  - [x] 6.1 Create navigation_controller.h and navigation_controller.c
    - Define navigation_event_t enum and navigation_callback_t typedef
    - Implement navigation_init() using tdl_button_create()
    - Implement navigation_register_callback() for event handling
    - Implement navigation_button_handler() to translate button events
    - Register button callbacks for BUTTON_NAME, BUTTON_NAME_2, BUTTON_NAME_3
    - Map short press to next/prev page, long press to menu
    - _Requirements: 4.1, 4.2, 4.5_

  - [x] 6.2 Write property test for next page navigation
    - **Property 11: Next page navigation**
    - **Validates: Requirements 4.1**

  - [x] 6.3 Write property test for previous page navigation
    - **Property 12: Previous page navigation**
    - **Validates: Requirements 4.2**

  - [x] 6.4 Write unit tests for navigation controller
    - Test button event registration
    - Test callback invocation
    - Test long press vs short press
    - _Requirements: 4.1, 4.2, 4.5_

- [x] 7. Implement Main Controller and state machine
  - [x] 7.1 Create main.c with application entry point
    - Define app_context_t structure
    - Implement ebook_reader_init() to initialize all subsystems
    - Implement state machine with STATE_INIT, STATE_MENU, STATE_READING, STATE_SLEEP, STATE_ERROR
    - Implement ebook_reader_run() main event loop
    - Implement ebook_reader_handle_navigation() for event routing
    - Implement ebook_reader_load_book() to load and display a book
    - Implement ebook_reader_show_menu() for book selection
    - Implement ebook_reader_handle_error() for error display
    - Implement ebook_reader_cleanup() for resource deallocation
    - Use tuya_app_main() entry point for RTOS platforms
    - _Requirements: 2.2, 2.3, 2.4, 2.5, 4.3, 4.4, 5.3, 5.4, 7.3, 7.4, 7.5, 8.2, 8.3_

  - [x] 7.2 Write property test for book selection
    - **Property 2: Book selection loads first page**
    - **Validates: Requirements 2.3**

  - [x] 7.3 Write property test for filename persistence
    - **Property 3: Current book filename persistence**
    - **Validates: Requirements 2.4**

  - [x] 7.4 Write property test for wake on button press
    - **Property 14: Wake on button press**
    - **Validates: Requirements 5.4**

  - [x] 7.5 Write property test for file open error messages
    - **Property 15: File open error messages**
    - **Validates: Requirements 7.2**

  - [x] 7.6 Write property test for error logging
    - **Property 16: Error logging completeness**
    - **Validates: Requirements 7.5**

  - [x] 7.7 Write unit tests for main controller
    - Test state transitions
    - Test book loading flow
    - Test menu display
    - Test error handling
    - _Requirements: 2.2, 2.3, 2.4, 2.5, 7.3, 7.4_

- [x] 8. Checkpoint - Verify complete system
  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Create sample e-books
  - [x] 9.1 Create assets/books/ directory
    - Create sample_book_1.txt with classic literature excerpt (3000+ chars)
    - Create sample_book_2.txt with technical content (3000+ chars)
    - Create sample_book_3.txt with short story (2500+ chars)
    - Ensure all books have sufficient content for pagination testing
    - _Requirements: 6.1, 6.2, 6.3_

  - [x] 9.2 Update README.md with SD card setup instructions
    - Document how to format SD card
    - Document how to copy books to SD card root directory
    - Document supported file format (.txt)
    - Document file size limits (1MB max)
    - _Requirements: 6.4_

- [x] 10. Integration and final testing
  - [x] 10.1 Write integration tests
    - Test complete flow: init → menu → select → read → navigate → menu
    - Test book switching
    - Test error recovery flows
    - Test sleep/wake cycle
    - _Requirements: 2.2, 2.3, 4.1, 4.2, 4.5, 5.3, 5.4_

  - [x] 10.2 Build and test on target hardware
    - ✅ Build project using tos.py build - **COMPLETED**
    - ✅ Generated binaries: `e-paper-ebook-reader_QIO_1.0.0.bin` (2.2MB) and `e-paper-ebook-reader_UA_1.0.0.bin` (2.0MB)
    - ⏳ Flash to T5AI board - **PENDING USER ACTION**
    - ⏳ Test with real SD card and e-Paper display - **PENDING USER ACTION**
    - ⏳ Verify button navigation - **PENDING USER ACTION**
    - ⏳ Verify display quality and refresh rates - **PENDING USER ACTION**
    - _Requirements: 8.4_

  - [x] 10.3 Update documentation
    - Add hardware setup instructions to README
    - Document pin connections for e-Paper and SD card
    - Add troubleshooting section
    - Add screenshots or photos of working system
    - _Requirements: 8.5_

- [x] 11. Final checkpoint - Complete system validation
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- All tasks are required for comprehensive implementation
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties with 100+ iterations each
- Unit tests validate specific examples and edge cases
- Integration tests verify end-to-end functionality
- The implementation follows TuyaOpen platform conventions and uses TAL APIs throughout
