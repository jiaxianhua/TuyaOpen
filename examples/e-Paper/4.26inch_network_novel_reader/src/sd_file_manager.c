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
    
    if (file_size <= 0 || file_size > 1024 * 1024) {
        PR_ERR("Invalid file size: %ld", file_size);
        tkl_fclose(fp);
        return OPRT_COM_ERROR;
    }
    
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
 * @brief Display BMP image
 */
int sd_display_bmp_image(const char *filepath)
{
    if (!filepath) return OPRT_INVALID_PARM;
    
    PR_NOTICE("Displaying BMP image: %s", filepath);
    
    // Use GUI_ReadBmp to display the image
    UBYTE result = GUI_ReadBmp(filepath, 0, 0);
    
    if (result != 0) {
        PR_ERR("Failed to display BMP image");
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK;
}
