/**
 * @file sd_file_manager.c
 * @brief SD Card File Manager Implementation
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "sd_file_manager.h"
#include "tkl_fs.h"
#include "tkl_output.h"
#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Initialize SD card
 */
int sd_card_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    
    PR_NOTICE("Initializing SD card...");
    
    // Mount SD card
    rt = tkl_fs_mount(SDCARD_MOUNT_PATH, DEV_SDCARD);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to mount SD card: %d", rt);
        return rt;
    }
    
    PR_NOTICE("SD card mounted successfully");
    return OPRT_OK;
}

/**
 * @brief Create default sample files
 */
int sd_create_sample_files(void)
{
    PR_NOTICE("Creating sample files on SD card...");
    
    // Create sample text file 1
    const char *sample_txt1 = SDCARD_MOUNT_PATH "/sample1.txt";
    TUYA_FILE fp = tkl_fopen(sample_txt1, "w");
    if (fp) {
        const char *content = "这是一个示例文本文件。\n"
                             "This is a sample text file.\n"
                             "欢迎使用电子墨水屏阅读器！\n"
                             "Welcome to E-Paper Reader!\n\n"
                             "功能特点：\n"
                             "1. 支持TXT文本阅读\n"
                             "2. 支持BMP图片显示\n"
                             "3. SD卡文件浏览\n"
                             "4. 按钮翻页控制\n";
        tkl_fwrite((void *)content, strlen(content), fp);
        tkl_fclose(fp);
        PR_NOTICE("Created: %s", sample_txt1);
    }
    
    // Create sample text file 2
    const char *sample_txt2 = SDCARD_MOUNT_PATH "/readme.txt";
    fp = tkl_fopen(sample_txt2, "w");
    if (fp) {
        const char *content = "E-Paper SD Card Reader\n"
                             "======================\n\n"
                             "Button Controls:\n"
                             "- Short press: Next file\n"
                             "- Long press: Open/Close file\n\n"
                             "Supported formats:\n"
                             "- TXT: Text files\n"
                             "- BMP: Bitmap images\n"
                             "- PNG: PNG images (converted)\n"
                             "- JPG: JPEG images (converted)\n";
        tkl_fwrite((void *)content, strlen(content), fp);
        tkl_fclose(fp);
        PR_NOTICE("Created: %s", sample_txt2);
    }
    
    // Create a simple monochrome BMP file (16x16 pixels, smiley face)
    const char *sample_bmp = SDCARD_MOUNT_PATH "/smiley.bmp";
    fp = tkl_fopen(sample_bmp, "wb");
    if (fp) {
        // BMP header for 16x16 monochrome bitmap
        unsigned char bmp_header[] = {
            // File header (14 bytes)
            0x42, 0x4D,             // "BM"
            0x7E, 0x00, 0x00, 0x00, // File size: 126 bytes
            0x00, 0x00,             // Reserved
            0x00, 0x00,             // Reserved
            0x3E, 0x00, 0x00, 0x00, // Offset to pixel data: 62 bytes
            
            // Info header (40 bytes)
            0x28, 0x00, 0x00, 0x00, // Info header size: 40
            0x10, 0x00, 0x00, 0x00, // Width: 16
            0x10, 0x00, 0x00, 0x00, // Height: 16
            0x01, 0x00,             // Planes: 1
            0x01, 0x00,             // Bits per pixel: 1
            0x00, 0x00, 0x00, 0x00, // Compression: none
            0x40, 0x00, 0x00, 0x00, // Image size: 64 bytes
            0x00, 0x00, 0x00, 0x00, // X pixels per meter
            0x00, 0x00, 0x00, 0x00, // Y pixels per meter
            0x02, 0x00, 0x00, 0x00, // Colors used: 2
            0x02, 0x00, 0x00, 0x00, // Important colors: 2
            
            // Color palette (8 bytes)
            0xFF, 0xFF, 0xFF, 0x00, // White
            0x00, 0x00, 0x00, 0x00, // Black
        };
        
        // Pixel data (16x16 = 32 bytes, 2 bytes per row)
        unsigned char pixels[] = {
            0xFF, 0xFF, 0xFF, 0xFF, // Row 16 (top)
            0xFF, 0xFF, 0xFF, 0xFF, // Row 15
            0xF0, 0x0F, 0xF0, 0x0F, // Row 14
            0xF0, 0x0F, 0xF0, 0x0F, // Row 13
            0xFF, 0xFF, 0xFF, 0xFF, // Row 12
            0xFF, 0xFF, 0xFF, 0xFF, // Row 11
            0xE0, 0x07, 0xE0, 0x07, // Row 10 (eyes)
            0xE0, 0x07, 0xE0, 0x07, // Row 9
            0xFF, 0xFF, 0xFF, 0xFF, // Row 8
            0xFF, 0xFF, 0xFF, 0xFF, // Row 7
            0xC0, 0x03, 0xC0, 0x03, // Row 6 (smile)
            0x80, 0x01, 0x80, 0x01, // Row 5
            0x00, 0x00, 0x00, 0x00, // Row 4
            0x80, 0x01, 0x80, 0x01, // Row 3
            0xC0, 0x03, 0xC0, 0x03, // Row 2
            0xFF, 0xFF, 0xFF, 0xFF, // Row 1 (bottom)
        };
        
        tkl_fwrite(bmp_header, sizeof(bmp_header), fp);
        tkl_fwrite(pixels, sizeof(pixels), fp);
        tkl_fclose(fp);
        PR_NOTICE("Created: %s", sample_bmp);
    }
    
    PR_NOTICE("Sample files created successfully");
    return OPRT_OK;
}

/**
 * @brief Get file type from extension
 */
file_type_e sd_get_file_type(const char *filename)
{
    if (!filename) return FILE_TYPE_UNKNOWN;
    
    const char *ext = strrchr(filename, '.');
    if (!ext) return FILE_TYPE_UNKNOWN;
    
    ext++; // Skip the dot
    
    // Convert to lowercase for comparison
    char ext_lower[10] = {0};
    int i;
    for (i = 0; i < 9 && ext[i]; i++) {
        ext_lower[i] = tolower(ext[i]);
    }
    
    if (strcmp(ext_lower, "txt") == 0) return FILE_TYPE_TXT;
    if (strcmp(ext_lower, "bmp") == 0) return FILE_TYPE_BMP;
    if (strcmp(ext_lower, "png") == 0) return FILE_TYPE_PNG;
    if (strcmp(ext_lower, "jpg") == 0 || strcmp(ext_lower, "jpeg") == 0) return FILE_TYPE_JPG;
    
    return FILE_TYPE_UNKNOWN;
}

/**
 * @brief Scan files in directory
 */
int sd_scan_files(file_browser_t *browser)
{
    if (!browser) return OPRT_INVALID_PARM;
    
    PR_NOTICE("Scanning SD card files...");
    
    memset(browser, 0, sizeof(file_browser_t));
    strncpy(browser->current_path, SDCARD_MOUNT_PATH, sizeof(browser->current_path) - 1);
    
    TUYA_DIR dir = NULL;
    int rt = tkl_dir_open(SDCARD_MOUNT_PATH, &dir);
    if (rt != OPRT_OK || !dir) {
        PR_ERR("Failed to open directory: %s (ret=%d)", SDCARD_MOUNT_PATH, rt);
        return OPRT_COM_ERROR;
    }
    
    TUYA_FILEINFO info = NULL;
    while (tkl_dir_read(dir, &info) == OPRT_OK && browser->file_count < MAX_FILES) {
        // Check if info is valid before using it
        if (!info) {
            PR_WARN("tkl_dir_read returned OK but info is NULL, stopping scan");
            break;
        }
        
        const char *name = NULL;
        if (tkl_dir_name(info, &name) != OPRT_OK || !name) {
            continue;
        }
        
        // Skip hidden files and current/parent directory
        if (name[0] == '.') continue;
        
        // Check if it's a regular file
        BOOL_T is_regular = FALSE;
        if (tkl_dir_is_regular(info, &is_regular) != OPRT_OK || !is_regular) {
            continue;
        }
        
        file_type_e type = sd_get_file_type(name);
        
        // Only add supported file types
        if (type != FILE_TYPE_UNKNOWN) {
            strncpy(browser->files[browser->file_count].name, name, MAX_FILENAME_LEN - 1);
            browser->files[browser->file_count].type = type;
            browser->files[browser->file_count].size = 0; // Size not available from dir info
            browser->file_count++;
            
            PR_DEBUG("Found: %s (type=%d)", name, type);
        }
    }
    
    tkl_dir_close(dir);
    
    PR_NOTICE("Found %d supported files", browser->file_count);
    return OPRT_OK;
}

/**
 * @brief Read text file content
 */
int sd_read_text_file(const char *filepath, char **buffer, int *size)
{
    if (!filepath || !buffer || !size) return OPRT_INVALID_PARM;
    
    PR_NOTICE("Reading text file: %s", filepath);
    
    TUYA_FILE fp = tkl_fopen(filepath, "r");
    if (!fp) {
        PR_ERR("Failed to open file: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Get file size
    tkl_fseek(fp, 0, SEEK_END);
    long file_size = tkl_ftell(fp);
    tkl_fseek(fp, 0, SEEK_SET);
    
    // Increased limit to 50 MB for large novels
    // Note: For very large files, consider reading in chunks
    if (file_size <= 0 || file_size > 50 * 1024 * 1024) {
        PR_ERR("Invalid file size: %ld (max 50 MB)", file_size);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    PR_NOTICE("File size: %ld bytes (%.2f MB)", file_size, file_size / (1024.0 * 1024.0));
    
    // Allocate buffer
    *buffer = (char *)tal_malloc(file_size + 1);
    if (!*buffer) {
        PR_ERR("Memory allocation failed");
        tkl_fclose(fp);
        return OPRT_MALLOC_FAILED;
    }
    
    // Read file
    int bytes_read = tkl_fread(*buffer, file_size, fp);
    if (bytes_read <= 0) {
        PR_ERR("Failed to read file");
        tal_free(*buffer);
        *buffer = NULL;
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    (*buffer)[bytes_read] = '\0';
    *size = bytes_read;
    
    tkl_fclose(fp);
    
    PR_NOTICE("Read %d bytes from file", bytes_read);
    return OPRT_OK;
}

/**
 * @brief Get total number of pages in a text file
 */
int sd_get_page_count(const char *filepath, int page_size)
{
    if (!filepath || page_size <= 0) return OPRT_INVALID_PARM;
    
    TUYA_FILE fp = tkl_fopen(filepath, "r");
    if (!fp) {
        PR_ERR("Failed to open file: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Get file size
    tkl_fseek(fp, 0, SEEK_END);
    long file_size = tkl_ftell(fp);
    tkl_fclose(fp);
    
    if (file_size <= 0) {
        return OPRT_COM_ERROR;
    }
    
    // Calculate number of pages
    int page_count = (file_size + page_size - 1) / page_size;
    
    PR_DEBUG("File size: %ld bytes, Page size: %d bytes, Pages: %d", 
             file_size, page_size, page_count);
    
    return page_count;
}

/**
 * @brief Read a page of text from file
 * This function reads only one page at a time to save memory
 */
int sd_read_text_page(const char *filepath, int page_num, int page_size, char **buffer, int *size)
{
    if (!filepath || !buffer || !size || page_num < 0 || page_size <= 0) {
        return OPRT_INVALID_PARM;
    }
    
    PR_DEBUG("Reading page %d (size: %d bytes) from: %s", page_num, page_size, filepath);
    
    TUYA_FILE fp = tkl_fopen(filepath, "r");
    if (!fp) {
        PR_ERR("Failed to open file: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Get file size
    tkl_fseek(fp, 0, SEEK_END);
    long file_size = tkl_ftell(fp);
    
    if (file_size <= 0) {
        PR_ERR("Invalid file size: %ld", file_size);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Calculate offset and actual read size
    long offset = (long)page_num * page_size;
    
    if (offset >= file_size) {
        PR_ERR("Page %d is beyond file size", page_num);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Calculate how much to read (might be less than page_size for last page)
    int read_size = page_size;
    if (offset + read_size > file_size) {
        read_size = file_size - offset;
    }
    
    // Seek to page start
    tkl_fseek(fp, offset, SEEK_SET);
    
    // Allocate buffer for this page only
    *buffer = (char *)tal_malloc(read_size + 1);
    if (!*buffer) {
        PR_ERR("Memory allocation failed for %d bytes", read_size + 1);
        tkl_fclose(fp);
        return OPRT_MALLOC_FAILED;
    }
    
    // Read page
    int bytes_read = tkl_fread(*buffer, read_size, fp);
    if (bytes_read <= 0) {
        PR_ERR("Failed to read page");
        tal_free(*buffer);
        *buffer = NULL;
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    (*buffer)[bytes_read] = '\0';
    *size = bytes_read;
    
    tkl_fclose(fp);
    
    PR_DEBUG("Read page %d: %d bytes (offset: %ld)", page_num, bytes_read, offset);
    
    return OPRT_OK;
}

/**
 * @brief Display BMP image using TuyaOpen filesystem (supports color conversion)
 */
int sd_display_bmp_image(const char *filepath)
{
    if (!filepath) return OPRT_INVALID_PARM;
    
    PR_NOTICE("Displaying BMP image: %s", filepath);
    
    // Check if file exists
    BOOL_T exists = FALSE;
    if (tkl_fs_is_exist(filepath, &exists) != OPRT_OK || !exists) {
        PR_ERR("File does not exist: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Open file using TuyaOpen API
    TUYA_FILE fp = tkl_fopen(filepath, "rb");
    if (!fp) {
        PR_ERR("Failed to open BMP file: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Read BMP file header (14 bytes)
    unsigned char file_header[14];
    if (tkl_fread(file_header, 14, fp) != 14) {
        PR_ERR("Failed to read BMP file header");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Check BMP signature
    if (file_header[0] != 0x42 || file_header[1] != 0x4D) {
        PR_ERR("Not a valid BMP file (signature mismatch)");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Read BMP info header (40 bytes)
    unsigned char info_header[40];
    if (tkl_fread(info_header, 40, fp) != 40) {
        PR_ERR("Failed to read BMP info header");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Parse image dimensions
    int width = *(int *)&info_header[4];
    int height = *(int *)&info_header[8];
    int bit_count = *(short *)&info_header[14];
    int compression = *(int *)&info_header[16];
    
    PR_NOTICE("BMP: %dx%d, %d bits per pixel, compression=%d", width, height, bit_count, compression);
    
    // Handle negative height (top-down bitmap)
    int is_top_down = 0;
    if (height < 0) {
        height = -height;
        is_top_down = 1;
        PR_DEBUG("Top-down bitmap detected");
    }
    
    // Check for compression
    // 0 = BI_RGB (no compression)
    // 3 = BI_BITFIELDS (with color masks, common for 32-bit BMP)
    if (compression != 0 && compression != 3) {
        PR_ERR("Unsupported BMP compression: %d (only 0=BI_RGB and 3=BI_BITFIELDS supported)", compression);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // For BI_BITFIELDS (compression=3), skip the color masks (12 bytes)
    if (compression == 3 && (bit_count == 16 || bit_count == 32)) {
        PR_DEBUG("Skipping BI_BITFIELDS color masks");
        unsigned char color_masks[12];
        tkl_fread(color_masks, 12, fp);  // Skip R, G, B masks
    }
    
    // Handle different bit depths
    if (bit_count == 1) {
        // 1-bit monochrome - original code
        PR_NOTICE("Processing 1-bit monochrome BMP");
        
        // Read color palette (8 bytes for 1-bit BMP)
        unsigned char palette[8];
        if (tkl_fread(palette, 8, fp) != 8) {
            PR_ERR("Failed to read color palette");
            tkl_fclose(fp);
            return OPRT_COM_ERROR;
        }
        
        // Determine black and white colors from palette
        UWORD black_color = BLACK;
        UWORD white_color = WHITE;
        if (palette[0] == 0xFF && palette[1] == 0xFF && palette[2] == 0xFF) {
            black_color = BLACK;
            white_color = WHITE;
        } else {
            black_color = WHITE;
            white_color = BLACK;
        }
        
        // Calculate row size (must be multiple of 4 bytes)
        int row_size = ((width + 31) / 32) * 4;
        
        // Allocate buffer for one row
        unsigned char *row_buffer = (unsigned char *)tal_malloc(row_size);
        if (!row_buffer) {
            PR_ERR("Memory allocation failed");
            tkl_fclose(fp);
            return OPRT_MALLOC_FAILED;
        }
        
        // Read and display image data
        for (int y = 0; y < height; y++) {
            if (tkl_fread(row_buffer, row_size, fp) != row_size) {
                PR_ERR("Failed to read image data at row %d", y);
                tal_free(row_buffer);
                tkl_fclose(fp);
                return OPRT_COM_ERROR;
            }
            
            // Draw pixels (BMP is stored bottom-up unless top-down flag set)
            int display_y = is_top_down ? y : (height - y - 1);
            
            for (int x = 0; x < width; x++) {
                int byte_idx = x / 8;
                int bit_idx = 7 - (x % 8);
                
                UWORD color = (row_buffer[byte_idx] & (1 << bit_idx)) ? white_color : black_color;
                Paint_SetPixel(x, display_y, color);
            }
        }
        
        tal_free(row_buffer);
        
    } else if (bit_count == 24 || bit_count == 32) {
        // 24-bit or 32-bit color - convert to monochrome
        PR_NOTICE("Converting %d-bit color BMP to monochrome", bit_count);
        
        int bytes_per_pixel = bit_count / 8;
        int row_size = ((width * bytes_per_pixel + 3) / 4) * 4;  // Aligned to 4 bytes
        
        // Get pixel data offset from file header
        int pixel_data_offset = *(int *)&file_header[10];
        PR_DEBUG("Pixel data offset: %d", pixel_data_offset);
        
        // Seek to pixel data start
        // We've already read 14 (file header) + 40 (info header) = 54 bytes
        // For BI_BITFIELDS, we also read 12 bytes of color masks
        int bytes_read = 54;
        if (compression == 3) {
            bytes_read += 12;  // Color masks
        }
        
        // Skip any remaining header data
        if (pixel_data_offset > bytes_read) {
            int skip_bytes = pixel_data_offset - bytes_read;
            PR_DEBUG("Skipping %d bytes to reach pixel data", skip_bytes);
            unsigned char *skip_buffer = (unsigned char *)tal_malloc(skip_bytes);
            if (skip_buffer) {
                tkl_fread(skip_buffer, skip_bytes, fp);
                tal_free(skip_buffer);
            } else {
                // If malloc fails, seek byte by byte
                for (int i = 0; i < skip_bytes; i++) {
                    unsigned char dummy;
                    tkl_fread(&dummy, 1, fp);
                }
            }
        }
        
        // Allocate buffer for one row
        unsigned char *row_buffer = (unsigned char *)tal_malloc(row_size);
        if (!row_buffer) {
            PR_ERR("Memory allocation failed");
            tkl_fclose(fp);
            return OPRT_MALLOC_FAILED;
        }
        
        // Read and convert image data
        for (int y = 0; y < height; y++) {
            if (tkl_fread(row_buffer, row_size, fp) != row_size) {
                PR_ERR("Failed to read image data at row %d", y);
                tal_free(row_buffer);
                tkl_fclose(fp);
                return OPRT_COM_ERROR;
            }
            
            // Draw pixels with grayscale conversion
            int display_y = is_top_down ? y : (height - y - 1);
            
            for (int x = 0; x < width; x++) {
                int pixel_offset = x * bytes_per_pixel;
                
                // BMP stores as BGR(A)
                unsigned char blue = row_buffer[pixel_offset];
                unsigned char green = row_buffer[pixel_offset + 1];
                unsigned char red = row_buffer[pixel_offset + 2];
                
                // Convert to grayscale using standard formula
                int gray = (red * 299 + green * 587 + blue * 114) / 1000;
                
                // Threshold to black or white (128 is middle)
                UWORD color = (gray > 128) ? WHITE : BLACK;
                Paint_SetPixel(x, display_y, color);
            }
        }
        
        tal_free(row_buffer);
        
    } else {
        PR_ERR("Unsupported BMP bit depth: %d (supported: 1, 24, 32)", bit_count);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    tkl_fclose(fp);
    
    PR_NOTICE("BMP image displayed successfully");
    return OPRT_OK;
}

/**
 * @brief Display BMP image at specific position using TuyaOpen filesystem
 * @param[in] filepath Path to BMP file
 * @param[in] x_offset X position offset
 * @param[in] y_offset Y position offset
 * @return OPRT_OK on success, error code otherwise
 */
int sd_display_bmp_image_at(const char *filepath, int x_offset, int y_offset)
{
    if (!filepath) return OPRT_INVALID_PARM;
    
    PR_DEBUG("Displaying BMP image at (%d,%d): %s", x_offset, y_offset, filepath);
    
    // Check if file exists
    BOOL_T exists = FALSE;
    if (tkl_fs_is_exist(filepath, &exists) != OPRT_OK || !exists) {
        PR_ERR("File does not exist: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Open file using TuyaOpen API
    TUYA_FILE fp = tkl_fopen(filepath, "rb");
    if (!fp) {
        PR_ERR("Failed to open BMP file: %s", filepath);
        return OPRT_COM_ERROR;
    }
    
    // Read BMP file header (14 bytes)
    unsigned char file_header[14];
    if (tkl_fread(file_header, 14, fp) != 14) {
        PR_ERR("Failed to read BMP file header");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Check BMP signature
    if (file_header[0] != 0x42 || file_header[1] != 0x4D) {
        PR_ERR("Not a valid BMP file (signature mismatch)");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Read BMP info header (40 bytes)
    unsigned char info_header[40];
    if (tkl_fread(info_header, 40, fp) != 40) {
        PR_ERR("Failed to read BMP info header");
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // Parse image dimensions
    int width = *(int *)&info_header[4];
    int height = *(int *)&info_header[8];
    int bit_count = *(short *)&info_header[14];
    int compression = *(int *)&info_header[16];
    
    PR_DEBUG("BMP: %dx%d, %d bits per pixel, compression=%d", width, height, bit_count, compression);
    
    // Handle negative height (top-down bitmap)
    int is_top_down = 0;
    if (height < 0) {
        height = -height;
        is_top_down = 1;
    }
    
    // Check for compression
    if (compression != 0 && compression != 3) {
        PR_ERR("Unsupported BMP compression: %d", compression);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    // For BI_BITFIELDS, skip color masks
    if (compression == 3 && (bit_count == 16 || bit_count == 32)) {
        unsigned char color_masks[12];
        tkl_fread(color_masks, 12, fp);
    }
    
    // Handle different bit depths
    if (bit_count == 1) {
        // 1-bit monochrome
        unsigned char palette[8];
        if (tkl_fread(palette, 8, fp) != 8) {
            PR_ERR("Failed to read color palette");
            tkl_fclose(fp);
            return OPRT_COM_ERROR;
        }
        
        UWORD black_color = BLACK;
        UWORD white_color = WHITE;
        if (palette[0] == 0xFF && palette[1] == 0xFF && palette[2] == 0xFF) {
            black_color = BLACK;
            white_color = WHITE;
        } else {
            black_color = WHITE;
            white_color = BLACK;
        }
        
        int row_size = ((width + 31) / 32) * 4;
        unsigned char *row_buffer = (unsigned char *)tal_malloc(row_size);
        if (!row_buffer) {
            PR_ERR("Memory allocation failed");
            tkl_fclose(fp);
            return OPRT_MALLOC_FAILED;
        }
        
        for (int y = 0; y < height; y++) {
            if (tkl_fread(row_buffer, row_size, fp) != row_size) {
                PR_ERR("Failed to read image data at row %d", y);
                tal_free(row_buffer);
                tkl_fclose(fp);
                return OPRT_COM_ERROR;
            }
            
            int display_y = is_top_down ? y : (height - y - 1);
            
            for (int x = 0; x < width; x++) {
                int byte_idx = x / 8;
                int bit_idx = 7 - (x % 8);
                
                UWORD color = (row_buffer[byte_idx] & (1 << bit_idx)) ? white_color : black_color;
                Paint_SetPixel(x_offset + x, y_offset + display_y, color);
            }
        }
        
        tal_free(row_buffer);
        
    } else if (bit_count == 24 || bit_count == 32) {
        // 24-bit or 32-bit color - convert to monochrome
        int bytes_per_pixel = bit_count / 8;
        int row_size = ((width * bytes_per_pixel + 3) / 4) * 4;
        
        int pixel_data_offset = *(int *)&file_header[10];
        int bytes_read = 54;
        if (compression == 3) {
            bytes_read += 12;
        }
        
        if (pixel_data_offset > bytes_read) {
            int skip_bytes = pixel_data_offset - bytes_read;
            unsigned char *skip_buffer = (unsigned char *)tal_malloc(skip_bytes);
            if (skip_buffer) {
                tkl_fread(skip_buffer, skip_bytes, fp);
                tal_free(skip_buffer);
            } else {
                for (int i = 0; i < skip_bytes; i++) {
                    unsigned char dummy;
                    tkl_fread(&dummy, 1, fp);
                }
            }
        }
        
        unsigned char *row_buffer = (unsigned char *)tal_malloc(row_size);
        if (!row_buffer) {
            PR_ERR("Memory allocation failed");
            tkl_fclose(fp);
            return OPRT_MALLOC_FAILED;
        }
        
        for (int y = 0; y < height; y++) {
            if (tkl_fread(row_buffer, row_size, fp) != row_size) {
                PR_ERR("Failed to read image data at row %d", y);
                tal_free(row_buffer);
                tkl_fclose(fp);
                return OPRT_COM_ERROR;
            }
            
            int display_y = is_top_down ? y : (height - y - 1);
            
            for (int x = 0; x < width; x++) {
                int pixel_offset = x * bytes_per_pixel;
                
                unsigned char blue = row_buffer[pixel_offset];
                unsigned char green = row_buffer[pixel_offset + 1];
                unsigned char red = row_buffer[pixel_offset + 2];
                
                int gray = (red * 299 + green * 587 + blue * 114) / 1000;
                UWORD color = (gray > 128) ? WHITE : BLACK;
                Paint_SetPixel(x_offset + x, y_offset + display_y, color);
            }
        }
        
        tal_free(row_buffer);
        
    } else {
        PR_ERR("Unsupported BMP bit depth: %d", bit_count);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
    tkl_fclose(fp);
    
    PR_DEBUG("BMP image displayed successfully at (%d,%d)", x_offset, y_offset);
    return OPRT_OK;
}
