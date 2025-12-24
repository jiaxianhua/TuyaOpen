# Paged Reading Implementation

## Overview

The e-paper novel reader now supports **paged reading** for large text files. Instead of loading the entire file into memory at once, it reads only 1KB at a time, making it possible to read very large novels (10+ MB) on memory-constrained devices.

## Memory Constraints

- **Device Free Memory**: ~170 KB (0x2a8e0 bytes)
- **Previous Approach**: Tried to load entire 10.8 MB file → Memory allocation failed
- **New Approach**: Load only 1KB per page → Uses minimal memory

## How It Works

### 1. File Opening

When a text file is opened from SD card:

```c
// Store filepath for later page loading
strncpy(g_reader_ctx.current_filepath, filepath, sizeof(g_reader_ctx.current_filepath) - 1);

// Set page size to 1KB
g_reader_ctx.page_size = 1024;

// Calculate total pages without loading file
int page_count = sd_get_page_count(filepath, g_reader_ctx.page_size);
g_reader_ctx.total_pages = page_count;
g_reader_ctx.use_paged_reading = 1;
```

### 2. Page Loading

Each time a page is displayed, it's loaded dynamically:

```c
static int load_page_from_file(int page_num)
{
    // Free previous page
    if (g_reader_ctx.content) {
        tal_free(g_reader_ctx.content);
    }
    
    // Load only the requested page (1KB)
    sd_read_text_page(filepath, page_num, page_size, &buffer, &size);
}
```

### 3. Page Navigation

When user navigates to next/previous page:

```c
// Next page
g_reader_ctx.current_page++;
display_page();  // Automatically loads new page

// Previous page
g_reader_ctx.current_page--;
display_page();  // Automatically loads new page
```

## Key Functions

### `sd_get_page_count(filepath, page_size)`

Calculates total number of pages without loading the file:

```c
int page_count = (file_size + page_size - 1) / page_size;
```

For a 10.8 MB file with 1KB pages: **10,586 pages**

### `sd_read_text_page(filepath, page_num, page_size, buffer, size)`

Reads a single page from file:

1. Opens file
2. Seeks to: `offset = page_num * page_size`
3. Reads up to `page_size` bytes (or less for last page)
4. Returns buffer with page content

### `load_page_from_file(page_num)`

High-level function that:

1. Frees previous page content
2. Calls `sd_read_text_page()` to load new page
3. Updates `g_reader_ctx.content` with new page

## Memory Usage

### Before (Failed)

```
File: 10,840,334 bytes
Memory needed: 10,840,335 bytes (10.34 MB)
Available: 175,328 bytes (171 KB)
Result: MALLOC FAILED ❌
```

### After (Success)

```
File: 10,840,334 bytes
Memory per page: 1,024 bytes (1 KB)
Available: 175,328 bytes (171 KB)
Result: SUCCESS ✅
```

## Configuration

### Page Size

Currently set to **1KB** (1024 bytes):

```c
g_reader_ctx.page_size = 1024;  // 1KB per page
```

You can adjust this based on:

- **Smaller pages** (512 bytes): More pages, less memory, more frequent file reads
- **Larger pages** (2048 bytes): Fewer pages, more memory, fewer file reads

### Display Capacity

Each screen can display:

- **Lines per page**: 32
- **Characters per line**: 38 units (19 Chinese characters or 38 ASCII)
- **Approximate text**: ~600-800 bytes per screen

With 1KB pages, each page contains slightly more than one screen of text, allowing for smooth pagination.

## Modes

The reader now supports two content modes:

### 1. In-Memory Mode

Used for:
- Network-downloaded novels
- Embedded novels
- Small files

Features:
- Entire content loaded into memory
- Uses `page_offsets` array for pagination
- Fast page navigation

### 2. Paged Reading Mode

Used for:
- Large SD card files (>1MB recommended)
- Memory-constrained scenarios

Features:
- Only current page in memory
- Dynamic page loading
- Minimal memory footprint

## Code Structure

### Context Structure

```c
typedef struct {
    char *content;              // Current page content
    int content_size;           // Current page size
    int current_page;           // Current page number
    int total_pages;            // Total pages in file
    
    // Paged reading
    char current_filepath[300]; // File path
    int use_paged_reading;      // 1 = paged mode
    int page_size;              // Bytes per page
    
    // In-memory mode only
    int *page_offsets;          // Page boundaries
} novel_reader_ctx_t;
```

### Display Flow

```
display_page()
    ↓
[Paged mode?]
    ↓ YES
load_page_from_file()
    ↓
sd_read_text_page()
    ↓
[Render content]
```

## Testing

### Test with Large File

1. Copy a large text file (10+ MB) to SD card
2. Open file from browser
3. Navigate through pages
4. Monitor memory usage

### Expected Behavior

```
[01-01 00:00:28 ty N] Opening file: /sdcard/doupocangqiong.txt
[01-01 00:00:28 ty N] Using paged reading: 10586 pages of 1024 bytes each
[01-01 00:00:28 ty D] Reading page 0 (size: 1024 bytes)
[01-01 00:00:28 ty D] Loaded page 0: 1024 bytes
[01-01 00:00:28 ty N] Displaying page 1/10586
```

## Advantages

✅ **Memory Efficient**: Only 1KB in memory at a time  
✅ **Scalable**: Can handle files of any size  
✅ **Fast**: No initial loading delay  
✅ **Reliable**: No memory allocation failures  

## Limitations

⚠️ **File Access**: Each page navigation requires file I/O  
⚠️ **Search**: Cannot search entire file without loading all pages  
⚠️ **Page Calculation**: Page boundaries are byte-based, not character-based  

## Future Improvements

1. **Smart Caching**: Keep 2-3 pages in memory (current, next, previous)
2. **Character-Aware Pagination**: Split pages at character boundaries, not byte boundaries
3. **Compression**: Support compressed text files
4. **Bookmarks**: Save reading position across sessions

## Related Files

- `sd_file_manager.h` - Paged reading API
- `sd_file_manager.c` - Implementation
- `EPD_4in26_network_novel.c` - Integration
- `LARGE_FILE_SUPPORT.md` - General large file guide
