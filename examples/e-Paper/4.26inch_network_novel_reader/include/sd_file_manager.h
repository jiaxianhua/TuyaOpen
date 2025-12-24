/**
 * @file sd_file_manager.h
 * @brief SD Card File Manager for E-Paper Display
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef _SD_FILE_MANAGER_H_
#define _SD_FILE_MANAGER_H_

#include "tuya_cloud_types.h"
#include "tal_api.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define SDCARD_MOUNT_PATH "/sdcard"
#define MAX_FILES 50
#define MAX_FILENAME_LEN 128

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_TXT,
    FILE_TYPE_BMP,
    FILE_TYPE_PNG,
    FILE_TYPE_JPG,
    FILE_TYPE_DIR
} file_type_e;

typedef struct {
    char name[MAX_FILENAME_LEN];
    file_type_e type;
    int size;
} file_info_t;

typedef struct {
    file_info_t files[MAX_FILES];
    int file_count;
    int current_index;
    int is_file_open;
    char current_path[256];
} file_browser_t;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Initialize SD card
 * @return OPRT_OK on success
 */
int sd_card_init(void);

/**
 * @brief Create default sample files on SD card
 * @return OPRT_OK on success
 */
int sd_create_sample_files(void);

/**
 * @brief Scan files in SD card root directory
 * @param browser File browser context
 * @return OPRT_OK on success
 */
int sd_scan_files(file_browser_t *browser);

/**
 * @brief Get file type from extension
 * @param filename File name
 * @return File type
 */
file_type_e sd_get_file_type(const char *filename);

/**
 * @brief Read text file content
 * @param filepath Full file path
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Number of bytes read, or negative on error
 */
int sd_read_text_file(const char *filepath, char **buffer, int *size);

/**
 * @brief Read a page of text from file
 * @param filepath Full file path
 * @param page_num Page number (0-based)
 * @param page_size Size of each page in bytes
 * @param buffer Output buffer (will be allocated)
 * @param size Output size
 * @return OPRT_OK on success
 */
int sd_read_text_page(const char *filepath, int page_num, int page_size, char **buffer, int *size);

/**
 * @brief Get total number of pages in a text file
 * @param filepath Full file path
 * @param page_size Size of each page in bytes
 * @return Number of pages, or negative on error
 */
int sd_get_page_count(const char *filepath, int page_size);

/**
 * @brief Display BMP image on e-paper
 * @param filepath Full file path
 * @return OPRT_OK on success
 */
int sd_display_bmp_image(const char *filepath);

#endif /* _SD_FILE_MANAGER_H_ */
