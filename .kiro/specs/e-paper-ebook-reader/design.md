# Design Document: E-Paper E-Book Reader

## Overview

The E-Paper E-Book Reader is an embedded application for the TuyaOpen platform that enables users to read text-based e-books on a 4.26-inch e-Paper display. The application reads .txt files from an SD card, manages text pagination, and provides button-based navigation controls. The design leverages the existing Waveshare e-Paper driver library and TuyaOpen's TAL (Tuya Abstraction Layer) APIs for file system and peripheral access.

The system is designed for low-power operation, taking advantage of the e-Paper display's ability to retain images without power and using partial refresh modes for efficient page navigation.

## Architecture

The application follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           E-Book Reader Main Controller              │  │
│  │  (State machine, UI coordination, event handling)    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
         │              │              │              │
         ▼              ▼              ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   SD Card    │ │    Page      │ │   Display    │ │  Navigation  │
│   Manager    │ │   Manager    │ │  Controller  │ │  Controller  │
└──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
         │              │              │              │
         ▼              ▼              ▼              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Hardware Abstraction Layer                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ tkl_fs   │  │ tal_mem  │  │ EPD_4in26│  │tdl_button│   │
│  │  (SD)    │  │ (Memory) │  │ (Display)│  │ (Input)  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Application States

The application operates as a state machine with the following states:

1. **INIT**: System initialization, SD card mount, display setup
2. **MENU**: Book selection menu display
3. **READING**: Active book reading with page display
4. **SLEEP**: Low-power mode after inactivity
5. **ERROR**: Error state with diagnostic display

## Components and Interfaces

### 1. SD Card Manager

**Responsibility**: Manages all SD card file operations including mounting, file discovery, and file I/O.

**Key Functions**:
```c
// Initialize SD card and mount filesystem
int sd_manager_init(const char *mount_path);

// Scan for .txt files in root directory
int sd_manager_scan_books(char ***book_list, int *count);

// Open a book file for reading
TUYA_FILE sd_manager_open_book(const char *filename);

// Read chunk of data from book
int sd_manager_read_chunk(TUYA_FILE file, char *buffer, size_t size);

// Close book file
void sd_manager_close_book(TUYA_FILE file);

// Free book list memory
void sd_manager_free_book_list(char **book_list, int count);
```

**Dependencies**: 
- `tkl_fs.h` for file system operations
- `tal_api.h` for memory allocation

**Error Handling**:
- Returns error codes for mount failures
- Logs all file operation errors
- Handles missing SD card gracefully

### 2. Page Manager

**Responsibility**: Handles text pagination, calculates page breaks, and maintains reading position.

**Key Data Structures**:
```c
typedef struct {
    char *book_content;        // Full book content in memory
    size_t content_length;     // Total content length
    size_t current_offset;     // Current reading position
    int current_page;          // Current page number
    int total_pages;           // Total number of pages
    int chars_per_page;        // Characters that fit on one page
} page_context_t;
```

**Key Functions**:
```c
// Initialize page manager with book content
int page_manager_init(page_context_t *ctx, const char *content, size_t length);

// Calculate total pages based on display dimensions and font
int page_manager_calculate_pages(page_context_t *ctx, int screen_width, 
                                  int screen_height, int font_height, int font_width);

// Get text for current page
int page_manager_get_current_page(page_context_t *ctx, char *buffer, size_t buffer_size);

// Navigate to next page
int page_manager_next_page(page_context_t *ctx);

// Navigate to previous page
int page_manager_prev_page(page_context_t *ctx);

// Get page info string (e.g., "Page 5/23")
void page_manager_get_page_info(page_context_t *ctx, char *buffer, size_t buffer_size);

// Free page manager resources
void page_manager_cleanup(page_context_t *ctx);
```

**Pagination Algorithm**:
1. Calculate characters per line: `chars_per_line = (screen_width - 2*margin) / font_width`
2. Calculate lines per page: `lines_per_page = (screen_height - header_height - footer_height) / font_height`
3. Calculate chars per page: `chars_per_page = chars_per_line * lines_per_page`
4. Handle word wrapping at line boundaries
5. Calculate total pages: `total_pages = ceil(content_length / chars_per_page)`

### 3. Display Controller

**Responsibility**: Manages e-Paper display operations including initialization, rendering, and power management.

**Key Functions**:
```c
// Initialize display hardware
int display_init(void);

// Clear entire display
void display_clear(void);

// Render text page with header and footer
void display_render_page(const char *text, const char *page_info);

// Render book selection menu
void display_render_menu(char **book_list, int count, int selected_index);

// Display error message
void display_show_error(const char *message);

// Enter sleep mode
void display_sleep(void);

// Wake from sleep mode
void display_wake(void);

// Perform full refresh
void display_full_refresh(UBYTE *image);

// Perform partial refresh (for page navigation)
void display_partial_refresh(UBYTE *image, UWORD x, UWORD y, UWORD w, UWORD h);
```

**Display Layout**:
```
┌────────────────────────────────────────┐
│ [Book Title]                           │ ← Header (30px)
├────────────────────────────────────────┤
│                                        │
│  Lorem ipsum dolor sit amet,           │
│  consectetur adipiscing elit.          │
│  Sed do eiusmod tempor incididunt      │
│  ut labore et dolore magna aliqua.     │
│  ...                                   │
│                                        │ ← Content Area (420px)
│                                        │
│  ...more text...                       │
│                                        │
├────────────────────────────────────────┤
│                          Page 5/23     │ ← Footer (30px)
└────────────────────────────────────────┘
```

**Display Specifications**:
- Resolution: 800x480 pixels
- Content area: 800x420 pixels (after header/footer)
- Font: Font16 (16px height, ~8px width average)
- Margins: 10px left/right
- Estimated capacity: ~50 lines × ~95 chars = ~4750 chars per page

**Dependencies**:
- `EPD_4in26.h` for display driver
- `GUI_Paint.h` for drawing primitives
- `fonts.h` for font definitions

### 4. Navigation Controller

**Responsibility**: Handles button input and translates it into application commands.

**Key Functions**:
```c
// Initialize button handlers
int navigation_init(void);

// Register callback for navigation events
void navigation_register_callback(navigation_callback_t callback);

// Button event handler
void navigation_button_handler(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *argc);
```

**Button Mapping**:
- Button 1 (BUTTON_NAME): Next Page / Select (short press), Back to Menu (long press)
- Button 2 (BUTTON_NAME_2): Previous Page (short press)
- Button 3 (BUTTON_NAME_3): Menu navigation up/down (if available)

**Navigation Events**:
```c
typedef enum {
    NAV_EVENT_NEXT_PAGE,
    NAV_EVENT_PREV_PAGE,
    NAV_EVENT_MENU_UP,
    NAV_EVENT_MENU_DOWN,
    NAV_EVENT_SELECT,
    NAV_EVENT_BACK_TO_MENU
} navigation_event_t;

typedef void (*navigation_callback_t)(navigation_event_t event);
```

### 5. Main Controller

**Responsibility**: Coordinates all components, manages application state, and handles the main event loop.

**Key Functions**:
```c
// Initialize all subsystems
int ebook_reader_init(void);

// Main application loop
void ebook_reader_run(void);

// Handle navigation events
void ebook_reader_handle_navigation(navigation_event_t event);

// Load and display a book
int ebook_reader_load_book(const char *filename);

// Return to book selection menu
void ebook_reader_show_menu(void);

// Handle errors
void ebook_reader_handle_error(const char *error_msg);

// Cleanup and shutdown
void ebook_reader_cleanup(void);
```

**State Machine**:
```c
typedef enum {
    STATE_INIT,
    STATE_MENU,
    STATE_READING,
    STATE_SLEEP,
    STATE_ERROR
} app_state_t;
```

## Data Models

### Book Metadata
```c
typedef struct {
    char filename[256];        // Book filename
    char title[128];           // Book title (derived from filename)
    size_t file_size;          // File size in bytes
} book_info_t;
```

### Application Context
```c
typedef struct {
    app_state_t state;                // Current application state
    char **book_list;                 // List of available books
    int book_count;                   // Number of books
    int selected_book_index;          // Currently selected book in menu
    char current_book_filename[256];  // Currently open book
    page_context_t page_ctx;          // Page management context
    UBYTE *display_buffer;            // Display framebuffer
    uint32_t last_activity_time;      // For sleep timeout
} app_context_t;
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property Reflection

After analyzing all acceptance criteria, I identified the following testable properties and performed redundancy elimination:

**Redundancy Analysis**:
- Properties 3.4 (maintain page position) and 4.1/4.2 (navigation updates position) are related but not redundant - 3.4 is an invariant, while 4.1/4.2 test specific operations
- Properties 3.6 (clear before render) and 5.2 (full clear on book switch) are complementary - 3.6 is general, 5.2 is specific
- Properties 2.1 (list .txt files) and 1.4 (support .txt extension) can be combined into a single comprehensive property about .txt file handling
- Properties 3.3 (line wrapping) and 3.5 (pagination) are related but test different aspects - keeping both

**Combined Properties**:
- Combining 1.4 and 2.1 into Property 1: "Text file recognition and listing"

### Correctness Properties

Property 1: Text file recognition and listing
*For any* set of files on the SD card (mix of .txt and non-.txt files), the system should correctly identify and list only files with .txt extension
**Validates: Requirements 1.4, 2.1**

Property 2: Book selection loads first page
*For any* valid book file, when selected from the menu, the system should load the book content and display page 1 with page indicator "Page 1/N"
**Validates: Requirements 2.3**

Property 3: Current book filename persistence
*For any* book selection operation, after the book is loaded, the system should store the book's filename in the application context
**Validates: Requirements 2.4**

Property 4: Font size consistency
*For any* text rendering operation, the system should use only Font16 or Font20 (no other font sizes)
**Validates: Requirements 3.1**

Property 5: Characters per page calculation
*For any* display dimensions and font size, the calculated characters per page should equal `((width - 2*margin) / font_width) * ((height - header - footer) / font_height)`
**Validates: Requirements 3.2**

Property 6: Line wrapping correctness
*For any* text content, when rendered on a page, no line should exceed the calculated characters per line, and words should not be split mid-word unless the word itself exceeds line length
**Validates: Requirements 3.3**

Property 7: Page position invariant
*For any* navigation operation (next/previous page), the current page number should always be within bounds: `1 <= current_page <= total_pages`
**Validates: Requirements 3.4**

Property 8: Pagination for long content
*For any* text content longer than one page capacity, the system should split it into multiple pages where `total_pages = ceil(content_length / chars_per_page)`
**Validates: Requirements 3.5**

Property 9: Screen clear before render
*For any* page display operation, the display clear function should be called before the render function
**Validates: Requirements 3.6**

Property 10: Page number format
*For any* page display, the page indicator string should match the regex pattern `"Page \d+/\d+"` where the first number is current page and second is total pages
**Validates: Requirements 3.7**

Property 11: Next page navigation
*For any* page position where `current_page < total_pages`, pressing "Next Page" should increment current_page by 1
**Validates: Requirements 4.1**

Property 12: Previous page navigation
*For any* page position where `current_page > 1`, pressing "Previous Page" should decrement current_page by 1
**Validates: Requirements 4.2**

Property 13: Full clear on book switch
*For any* book switching operation (changing from one book to another), the system should perform a full screen clear (not partial refresh)
**Validates: Requirements 5.2**

Property 14: Wake on button press
*For any* button press event when the system is in sleep mode, the system should transition to wake state
**Validates: Requirements 5.4**

Property 15: File open error messages
*For any* file that fails to open, the error message displayed should contain the substring "Cannot open file:" followed by the filename
**Validates: Requirements 7.2**

Property 16: Error logging completeness
*For any* error condition in the system, an error message should be logged to the debug console with a descriptive message
**Validates: Requirements 7.5**

## Error Handling

The application implements comprehensive error handling at multiple levels:

### SD Card Errors
- **Mount Failure**: Display "SD Card Error" message, retry mount every 3 seconds
- **Read Failure**: Log error with filename, return to menu
- **No Books Found**: Display "No books found" message on screen

### Memory Errors
- **Allocation Failure**: Log error, attempt to free unused buffers, retry with smaller allocation
- **Buffer Overflow**: Validate all buffer operations, truncate content if necessary

### Display Errors
- **Init Failure**: Log error and halt execution (critical failure)
- **Render Failure**: Log error, attempt to reinitialize display

### File Errors
- **File Too Large**: Display error message, limit to 1MB
- **Invalid Format**: Skip non-.txt files silently
- **Corrupted Content**: Display as much as possible, log warning

### Error Recovery Strategy
1. Log all errors with context (function name, error code, relevant data)
2. Attempt graceful degradation when possible
3. Return to menu state for recoverable errors
4. Halt execution only for critical hardware failures

## Testing Strategy

The e-book reader will be tested using a dual approach combining unit tests and property-based tests to ensure comprehensive coverage and correctness.

### Unit Testing

Unit tests will verify specific examples, edge cases, and integration points:

**SD Card Manager Tests**:
- Test successful SD card initialization
- Test SD card mount failure handling
- Test scanning with empty directory
- Test scanning with mixed file types
- Test file size limit (1MB boundary)

**Page Manager Tests**:
- Test pagination calculation with known text
- Test first page boundary (cannot go previous)
- Test last page boundary (cannot go next)
- Test page navigation sequence
- Test empty book handling

**Display Controller Tests**:
- Test display initialization
- Test menu rendering with 0, 1, 3, 10 books
- Test page rendering with various text lengths
- Test error message display
- Test sleep/wake transitions

**Navigation Controller Tests**:
- Test button event registration
- Test button callback invocation
- Test long press vs short press detection

**Integration Tests**:
- Test complete flow: init → menu → select book → read → navigate → return to menu
- Test book switching
- Test error recovery flows

### Property-Based Testing

Property-based tests will verify universal properties across many generated inputs using a PBT library (to be selected based on C language support - likely using a custom generator framework or adapting a C++ library like RapidCheck).

**Test Configuration**:
- Minimum 100 iterations per property test
- Each test tagged with: `Feature: e-paper-ebook-reader, Property N: [property description]`

**Property Test Generators**:

1. **File List Generator**: Generates random lists of filenames with various extensions
2. **Text Content Generator**: Generates random text of various lengths (0-10000 chars)
3. **Page Position Generator**: Generates valid page positions within bounds
4. **Display Dimensions Generator**: Generates realistic display dimensions
5. **Font Size Generator**: Generates Font16 or Font20 parameters

**Property Test Implementation**:

Each of the 16 correctness properties will be implemented as a separate property-based test. For example:

```c
// Property 1: Text file recognition and listing
// Feature: e-paper-ebook-reader, Property 1: Text file recognition and listing
void test_property_txt_file_filtering(void) {
    for (int i = 0; i < 100; i++) {
        // Generate random file list
        char **files = generate_random_file_list(&count);
        
        // Filter for .txt files
        char **txt_files = sd_manager_scan_books(files, count, &txt_count);
        
        // Verify all results end with .txt
        for (int j = 0; j < txt_count; j++) {
            assert(ends_with(txt_files[j], ".txt"));
        }
        
        // Verify no .txt files were missed
        int expected_count = count_txt_files(files, count);
        assert(txt_count == expected_count);
        
        cleanup(files, txt_files);
    }
}
```

**Testing Balance**:
- Unit tests focus on specific scenarios and integration points
- Property tests verify universal correctness across all inputs
- Both approaches are necessary and complementary
- Property tests catch edge cases that unit tests might miss
- Unit tests provide concrete examples of expected behavior

### Test Execution

Tests will be executed as part of the build process:
```bash
# Build with tests enabled
tos.py build -DENABLE_TESTS=ON

# Run unit tests
./build/ebook_reader_unit_tests

# Run property tests (longer execution time)
./build/ebook_reader_property_tests
```

### Coverage Goals

- Line coverage: >80%
- Branch coverage: >75%
- All 16 correctness properties: 100% tested
- All error paths: 100% tested

