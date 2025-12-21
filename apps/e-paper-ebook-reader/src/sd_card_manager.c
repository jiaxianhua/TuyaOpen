/**
 * @file sd_card_manager.c
 * @brief SD Card Manager Implementation
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "sd_card_manager.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tkl_fs.h"
#include <string.h>
#include <stdlib.h>

#define SD_MOUNT_PATH "/sdcard"

static char sg_mount_path[64] = {0};

/**
 * @brief Initialize SD card and mount filesystem
 */
int sd_manager_init(const char *mount_path)
{
    PR_DEBUG("sd_manager_init called with path: %s", mount_path ? mount_path : "NULL");
    
    if (mount_path == NULL) {
        PR_ERR("Invalid mount path");
        return OPRT_INVALID_PARM;
    }

    strncpy(sg_mount_path, mount_path, sizeof(sg_mount_path) - 1);
    PR_DEBUG("Mount path copied: %s", sg_mount_path);
    
    PR_NOTICE("Mounting SD card at %s", mount_path);
    OPERATE_RET rt = tkl_fs_mount(mount_path, DEV_SDCARD);
    PR_DEBUG("tkl_fs_mount returned: %d", rt);
    
    if (rt != OPRT_OK) {
        PR_ERR("Failed to mount SD card: %d", rt);
        return rt;
    }
    
    PR_NOTICE("SD card mounted successfully");
    return OPRT_OK;
}

/**
 * @brief Check if filename has .txt extension
 */
int sd_manager_is_txt_file(const char *filename)
{
    if (filename == NULL) {
        return 0;
    }
    
    size_t len = strlen(filename);
    if (len < 5) {  // Minimum: "a.txt"
        return 0;
    }
    
    // Check for .txt extension (case insensitive)
    const char *ext = filename + len - 4;
    return (strcasecmp(ext, ".txt") == 0);
}

/**
 * @brief Scan for .txt files in root directory
 */
int sd_manager_scan_books(char ***book_list, int *count)
{
    PR_DEBUG("sd_manager_scan_books called");
    
    if (book_list == NULL || count == NULL) {
        PR_ERR("Invalid parameters");
        return OPRT_INVALID_PARM;
    }
    
    *book_list = NULL;
    *count = 0;
    
    // Open directory
    PR_DEBUG("Opening directory: %s", sg_mount_path);
    TUYA_DIR dir;
    int rt = tkl_dir_open(sg_mount_path, &dir);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to open directory: %s, error: %d", sg_mount_path, rt);
        return OPRT_COM_ERROR;
    }
    PR_DEBUG("Directory opened successfully");
    
    // First pass: count .txt files
    int txt_count = 0;
    TUYA_FILEINFO info;
    
    PR_DEBUG("First pass: counting .txt files");
    while (tkl_dir_read(dir, &info) == OPRT_OK) {
        BOOL_T is_regular = FALSE;
        const char *name = NULL;
        
        if (tkl_dir_is_regular(info, &is_regular) == OPRT_OK && is_regular) {
            if (tkl_dir_name(info, &name) == OPRT_OK && name != NULL) {
                PR_DEBUG("  Found file: %s", name);
                if (sd_manager_is_txt_file(name)) {
                    txt_count++;
                    PR_DEBUG("    -> is .txt file (count=%d)", txt_count);
                }
            }
        }
    }
    
    PR_NOTICE("First pass complete: found %d .txt files", txt_count);
    
    if (txt_count == 0) {
        PR_NOTICE("No .txt files found");
        tkl_dir_close(dir);
        return OPRT_OK;
    }
    
    // Allocate array for book list
    PR_DEBUG("Allocating memory for %d books", txt_count);
    char **books = (char **)tal_malloc(txt_count * sizeof(char *));
    if (books == NULL) {
        PR_ERR("Failed to allocate memory for book list");
        tkl_dir_close(dir);
        return OPRT_MALLOC_FAILED;
    }
    PR_DEBUG("Book list array allocated at %p", books);
    
    // Second pass: collect filenames
    PR_DEBUG("Closing and reopening directory for second pass");
    tkl_dir_close(dir);
    rt = tkl_dir_open(sg_mount_path, &dir);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to reopen directory for second pass");
        tal_free(books);
        return OPRT_COM_ERROR;
    }
    
    int index = 0;
    PR_DEBUG("Second pass: collecting filenames");
    while (tkl_dir_read(dir, &info) == OPRT_OK && index < txt_count) {
        BOOL_T is_regular = FALSE;
        const char *name = NULL;
        
        if (tkl_dir_is_regular(info, &is_regular) == OPRT_OK && is_regular) {
            if (tkl_dir_name(info, &name) == OPRT_OK && name != NULL) {
                if (sd_manager_is_txt_file(name)) {
                    books[index] = (char *)tal_malloc(SD_MANAGER_MAX_FILENAME_LEN);
                    if (books[index] == NULL) {
                        PR_ERR("Failed to allocate memory for filename %d", index);
                        // Free previously allocated memory
                        for (int i = 0; i < index; i++) {
                            tal_free(books[i]);
                        }
                        tal_free(books);
                        tkl_dir_close(dir);
                        return OPRT_MALLOC_FAILED;
                    }
                    strncpy(books[index], name, SD_MANAGER_MAX_FILENAME_LEN - 1);
                    books[index][SD_MANAGER_MAX_FILENAME_LEN - 1] = '\0';
                    PR_DEBUG("  [%d] %s", index, books[index]);
                    index++;
                }
            }
        }
    }
    
    tkl_dir_close(dir);
    
    *book_list = books;
    *count = index;
    
    PR_NOTICE("Scan complete: found %d .txt files", index);
    return OPRT_OK;
}

/**
 * @brief Open a book file for reading
 */
TUYA_FILE sd_manager_open_book(const char *filename)
{
    if (filename == NULL) {
        PR_ERR("Invalid filename");
        return NULL;
    }
    
    char filepath[SD_MANAGER_MAX_FILENAME_LEN + 64];
    snprintf(filepath, sizeof(filepath), "%s/%s", sg_mount_path, filename);
    
    PR_NOTICE("Opening book: %s", filepath);
    TUYA_FILE file = tkl_fopen(filepath, "r");
    
    if (file == NULL) {
        PR_ERR("Failed to open file: %s", filepath);
        return NULL;
    }
    
    return file;
}

/**
 * @brief Get file size
 */
int sd_manager_get_file_size(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    
    // Save current position
    long current_pos = tkl_fseek(file, 0, TUYA_SEEK_CUR);
    if (current_pos < 0) {
        return -1;
    }
    
    // Seek to end
    if (tkl_fseek(file, 0, TUYA_SEEK_END) < 0) {
        return -1;
    }
    
    // Get position (file size)
    long size = tkl_fseek(file, 0, TUYA_SEEK_CUR);
    
    // Restore original position
    tkl_fseek(file, current_pos, TUYA_SEEK_SET);
    
    return (int)size;
}

/**
 * @brief Read chunk of data from book
 */
int sd_manager_read_chunk(TUYA_FILE file, char *buffer, size_t size)
{
    if (file == NULL || buffer == NULL || size == 0) {
        PR_ERR("Invalid parameters");
        return -1;
    }
    
    int bytes_read = tkl_fread(buffer, size, file);
    
    if (bytes_read < 0) {
        PR_ERR("Failed to read from file");
        return -1;
    }
    
    return bytes_read;
}

/**
 * @brief Close book file
 */
void sd_manager_close_book(TUYA_FILE file)
{
    if (file != NULL) {
        tkl_fclose(file);
    }
}

/**
 * @brief Free book list memory
 */
void sd_manager_free_book_list(char **book_list, int count)
{
    if (book_list == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (book_list[i] != NULL) {
            tal_free(book_list[i]);
        }
    }
    
    tal_free(book_list);
}
