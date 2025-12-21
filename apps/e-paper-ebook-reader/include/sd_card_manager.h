/**
 * @file sd_card_manager.h
 * @brief SD Card Manager for E-Book Reader
 * 
 * Manages SD card operations including mounting, file discovery, and file I/O.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __SD_CARD_MANAGER_H__
#define __SD_CARD_MANAGER_H__

#include "tuya_cloud_types.h"
#include "tkl_fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum file size supported (1MB)
 */
#define SD_MANAGER_MAX_FILE_SIZE (1024 * 1024)

/**
 * @brief Maximum filename length
 */
#define SD_MANAGER_MAX_FILENAME_LEN 256

/**
 * @brief Initialize SD card and mount filesystem
 * 
 * @param[in] mount_path Path where SD card should be mounted (e.g., "/sdcard")
 * @return OPRT_OK on success, error code otherwise
 */
int sd_manager_init(const char *mount_path);

/**
 * @brief Scan for .txt files in root directory
 * 
 * @param[out] book_list Pointer to array of book filenames (caller must free)
 * @param[out] count Number of books found
 * @return OPRT_OK on success, error code otherwise
 */
int sd_manager_scan_books(char ***book_list, int *count);

/**
 * @brief Open a book file for reading
 * 
 * @param[in] filename Name of the book file to open
 * @return File handle on success, NULL on failure
 */
TUYA_FILE sd_manager_open_book(const char *filename);

/**
 * @brief Read chunk of data from book
 * 
 * @param[in] file File handle
 * @param[out] buffer Buffer to store read data
 * @param[in] size Size of buffer
 * @return Number of bytes read, negative on error
 */
int sd_manager_read_chunk(TUYA_FILE file, char *buffer, size_t size);

/**
 * @brief Get file size
 * 
 * @param[in] file File handle
 * @return File size in bytes, negative on error
 */
int sd_manager_get_file_size(TUYA_FILE file);

/**
 * @brief Close book file
 * 
 * @param[in] file File handle to close
 */
void sd_manager_close_book(TUYA_FILE file);

/**
 * @brief Free book list memory
 * 
 * @param[in] book_list Array of book filenames
 * @param[in] count Number of books in list
 */
void sd_manager_free_book_list(char **book_list, int count);

/**
 * @brief Check if filename has .txt extension
 * 
 * @param[in] filename Filename to check
 * @return 1 if .txt file, 0 otherwise
 */
int sd_manager_is_txt_file(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* __SD_CARD_MANAGER_H__ */
