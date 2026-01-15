/**
 * @file example_sd.c
 * @version 0.1
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tkl_fs.h"
#include "utf8_to_gbk.h"
#include "EPD_4in26.h"
#include "GUI_Paint.h"
#include "DEV_Config.h"

#if defined(EBABLE_EXAMPLE_SD_PINMUX) && (EBABLE_EXAMPLE_SD_PINMUX == 1)
#include "tkl_pinmux.h"
#endif

#include "board_com_api.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TASK_SD_PRIORITY THREAD_PRIO_2
#define TASK_SD_SIZE     4096

#define SDCARD_MOUNT_PATH "/sdcard"
#define RANDOM_FILE_PATH  "/sdcard/random.txt"
#define CHINESE_FILE_PATH "/sdcard/测试.txt"

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static THREAD_HANDLE sg_sd_thrd_hdl;
static char sg_write_buf[128] = {0};
static char sg_read_buf[128] = {0};

/***********************************************************
***********************function define**********************
***********************************************************/
static void display_file_list_on_epaper(void)
{
    PR_NOTICE("Initializing E-Paper...");
    if(DEV_Module_Init() != 0) {
        PR_ERR("E-Paper DEV_Module_Init failed");
        return;
    }

    EPD_4in26_Init();
    EPD_4in26_Clear();
    DEV_Delay_ms(500);

    // Allocate memory for image
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_4in26_WIDTH % 8 == 0)? (EPD_4in26_WIDTH / 8 ): (EPD_4in26_WIDTH / 8 + 1)) * EPD_4in26_HEIGHT;
    
    if((BlackImage = (UBYTE *)tal_malloc(Imagesize)) == NULL) {
        PR_ERR("Failed to allocate memory for E-Paper image...");
        return;
    }
    
    PR_NOTICE("Drawing file list to buffer...");
    Paint_NewImage(BlackImage, EPD_4in26_WIDTH, EPD_4in26_HEIGHT, 0, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // Draw Title
    Paint_DrawString_EN(10, 10, "SD Card Files:", &Font24, BLACK, WHITE);
    Paint_DrawLine(10, 35, 790, 35, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // List files
    TUYA_DIR dir_hdl = NULL;
    if (tkl_dir_open(SDCARD_MOUNT_PATH, &dir_hdl) == OPRT_OK) {
        TUYA_FILEINFO file_info = {0};
        int y_pos = 45;
        int file_count = 0;
        
        while (tkl_dir_read(dir_hdl, &file_info) == OPRT_OK) {
            char *name = NULL;
            if (tkl_dir_name(file_info, (const char**)&name) == OPRT_OK) {
                // Determine if ASCII or needs GBK handling
                int is_ascii = 1;
                for(int i=0; name[i]; i++) {
                    if((unsigned char)name[i] >= 0x80) {
                        is_ascii = 0;
                        break;
                    }
                }

                if(is_ascii) {
                    Paint_DrawString_EN(10, y_pos, name, &Font24, WHITE, BLACK);
                } else {
                    // Assuming name is GBK (from FAT32 default)
                    // If Font24CN supports it, it will display. 
                    // Note: Font24CN in library usually only has limited characters.
                    Paint_DrawString_CN(10, y_pos, name, &Font24CN, WHITE, BLACK);
                }
                
                y_pos += 30;
                file_count++;
                
                // Check bounds
                if (y_pos > EPD_4in26_HEIGHT - 30) {
                    Paint_DrawString_EN(10, y_pos, "... more files ...", &Font24, BLACK, WHITE);
                    break;
                }
            }
        }
        tkl_dir_close(dir_hdl);
        
        if (file_count == 0) {
            Paint_DrawString_EN(10, 50, "No files found!", &Font24, BLACK, WHITE);
        }
    } else {
        Paint_DrawString_EN(10, 50, "Failed to open dir!", &Font24, BLACK, WHITE);
    }

    PR_NOTICE("Updating E-Paper display...");
    EPD_4in26_Display(BlackImage);
    DEV_Delay_ms(2000);
    
    // Cleanup
    EPD_4in26_Sleep();
    tal_free(BlackImage);
    // DEV_Module_Exit(); // Keep initialized if we want to update again, or exit to save power
}

static void __example_sd_chinese_test(void)
{
    PR_NOTICE("Starting Chinese filename test...");

    // 1. Write file with Chinese filename (UTF-8 to GBK conversion)
    const char *utf8_name = "测试.txt";
    const char *content = "This is a test file with Chinese filename.";
    char gbk_name[128] = {0};
    char full_path[256] = {0};
    
    // Convert filename from UTF-8 to GBK
    // Note: FAT32 on embedded systems often uses CP936 (GBK) for LFN if not configured for UTF-8
    // We try to use GBK for filename to be compatible with Windows default behavior for older non-Unicode programs
    // or if the filesystem layer expects local encoding.
    int ret = utf8_to_gbk_buf((const uint8_t *)utf8_name, strlen(utf8_name), (uint8_t *)gbk_name, sizeof(gbk_name) - 1);
    
    if (ret > 0) {
        snprintf(full_path, sizeof(full_path), "%s/%s", SDCARD_MOUNT_PATH, gbk_name);
        PR_NOTICE("Creating file with GBK name: %s (Hex: %02X %02X %02X %02X)", 
                  full_path, (uint8_t)gbk_name[0], (uint8_t)gbk_name[1], (uint8_t)gbk_name[2], (uint8_t)gbk_name[3]);
    } else {
        PR_ERR("Failed to convert filename to GBK, using UTF-8 directly");
        snprintf(full_path, sizeof(full_path), "%s/%s", SDCARD_MOUNT_PATH, utf8_name);
    }

    TUYA_FILE file_hdl = tkl_fopen(full_path, "w");
    if (NULL == file_hdl) {
        PR_ERR("Open file %s failed", full_path);
        // Try creating with raw UTF-8 path if GBK failed
        snprintf(full_path, sizeof(full_path), "%s/%s", SDCARD_MOUNT_PATH, utf8_name);
        file_hdl = tkl_fopen(full_path, "w");
        if (NULL == file_hdl) {
            PR_ERR("Open file %s (UTF-8) failed too", full_path);
            return;
        }
    }

    uint32_t write_len = strlen(content);
    uint32_t ret_len = tkl_fwrite((void *)content, write_len, file_hdl);
    if (ret_len != write_len) {
        PR_ERR("Write file %s failed: %d", full_path, ret_len);
    } else {
        PR_NOTICE("Write file %s success", full_path);
    }
    tkl_fclose(file_hdl);

    // 2. List files in directory
    PR_NOTICE("Listing files in %s:", SDCARD_MOUNT_PATH);
    TUYA_DIR dir_hdl = NULL;
    if (tkl_dir_open(SDCARD_MOUNT_PATH, &dir_hdl) != OPRT_OK) {
        PR_ERR("Open directory %s failed", SDCARD_MOUNT_PATH);
        return;
    }

    TUYA_FILEINFO file_info = {0};
    while (tkl_dir_read(dir_hdl, &file_info) == OPRT_OK) {
        char *name = NULL;
        if (tkl_dir_name(file_info, (const char**)&name) == OPRT_OK) {
             // Check if name needs GBK -> UTF-8 conversion for display
             // Simple heuristic: check if it contains high bytes
             int needs_conversion = 0;
             for (int i = 0; name[i]; i++) {
                 if ((unsigned char)name[i] >= 0x80) {
                     needs_conversion = 1;
                     break;
                 }
             }
             
             if (needs_conversion) {
                 char utf8_out[256] = {0};
                 // Try to convert GBK name back to UTF-8 for display log
                 int conv_ret = gbk_to_utf8_buf((const uint8_t *)name, strlen(name), (uint8_t *)utf8_out, sizeof(utf8_out) - 1);
                 if (conv_ret > 0) {
                     PR_NOTICE("Found file (GBK->UTF8): %s [Raw: %s]", utf8_out, name);
                 } else {
                     PR_NOTICE("Found file (Raw): %s", name);
                 }
             } else {
                 PR_NOTICE("Found file: %s", name);
             }
        }
    }
    tkl_dir_close(dir_hdl);
    PR_NOTICE("Chinese filename test finished.");

    // 3. Display file list on E-Paper
    display_file_list_on_epaper();
}

static void __example_sd_test(void)
{
    int random_value = tal_system_get_random(0xFFFFFFFF);

    snprintf(sg_write_buf, sizeof(sg_write_buf), "random value: %d", random_value);

    TUYA_FILE file_hdl = tkl_fopen(RANDOM_FILE_PATH, "w");
    if (NULL == file_hdl) {
        PR_ERR("Open file %s failed", RANDOM_FILE_PATH);
        return;
    }

    uint32_t write_len = strlen(sg_write_buf);
    PR_NOTICE("Write file content: %s", sg_write_buf);
    uint32_t ret_len = tkl_fwrite(sg_write_buf, write_len, file_hdl);
    if (ret_len != write_len) {
        PR_ERR("Write file %s failed: %d", RANDOM_FILE_PATH, ret_len);
    }

    tkl_fclose(file_hdl);
    file_hdl = NULL;

    file_hdl = tkl_fopen(RANDOM_FILE_PATH, "r");
    if (NULL == file_hdl) {
        PR_ERR("open file %s failed: %d", RANDOM_FILE_PATH, ret_len);
        goto __EXIT;
    }

    // read file
    uint32_t read_len = tkl_fread(sg_read_buf, sizeof(sg_read_buf), file_hdl);
    if (read_len <= 0) {
        PR_ERR("read file %s failed: %d", RANDOM_FILE_PATH, read_len);
        goto __EXIT;
    }

    // compare file
    if (strncmp(sg_write_buf, sg_read_buf, read_len) != 0) {
        PR_ERR("---> fail: compare file failed");
    } else {
        PR_NOTICE("---> success: compare file success");
    }

__EXIT:
    // close file
    tkl_fclose(file_hdl);
    file_hdl = NULL;

    return;
}

/**
 * @brief sd task
 *
 * @param[in] param:Task parameters
 * @return none
 */
static void __example_sd_task(void *param)
{
    OPERATE_RET rt = OPRT_OK;
    
#if defined(EBABLE_EXAMPLE_SD_PINMUX) && (EBABLE_EXAMPLE_SD_PINMUX == 1)
    tkl_io_pinmux_config(EXAMPLE_SD_CLK_PIN, TUYA_SDIO_HOST_CLK);
    tkl_io_pinmux_config(EXAMPLE_SD_CMD_PIN, TUYA_SDIO_HOST_CMD);
    tkl_io_pinmux_config(EXAMPLE_SD_D0_PIN, TUYA_SDIO_HOST_D0);
    tkl_io_pinmux_config(EXAMPLE_SD_D1_PIN, TUYA_SDIO_HOST_D1);
    tkl_io_pinmux_config(EXAMPLE_SD_D2_PIN, TUYA_SDIO_HOST_D2);
    tkl_io_pinmux_config(EXAMPLE_SD_D3_PIN, TUYA_SDIO_HOST_D3);
#endif

    TUYA_CALL_ERR_LOG(tkl_fs_mount(SDCARD_MOUNT_PATH, DEV_SDCARD));
    if (rt != OPRT_OK) {
        PR_ERR("Mount SD card failed: %d", rt);
        while (1) {
            TUYA_CALL_ERR_LOG(tkl_fs_mount(SDCARD_MOUNT_PATH, DEV_SDCARD));
            tal_system_sleep(3 * 1000);
        }

    }

    while (1) {
        __example_sd_test();
        __example_sd_chinese_test();
        tal_system_sleep(3 * 1000);
    }
}

/**
 * @brief user_main
 *
 * @return none
 */
void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;

    /* basic init */
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);

    /*hardware register*/
    board_register_hardware();

    PR_NOTICE("Application information:");
    PR_NOTICE("Project name:        %s", PROJECT_NAME);
    PR_NOTICE("App version:         %s", PROJECT_VERSION);
    PR_NOTICE("Compile time:        %s", __DATE__);
    PR_NOTICE("TuyaOpen version:    %s", OPEN_VERSION);
    PR_NOTICE("TuyaOpen commit-id:  %s", OPEN_COMMIT);
    PR_NOTICE("Platform chip:       %s", PLATFORM_CHIP);
    PR_NOTICE("Platform board:      %s", PLATFORM_BOARD);
    PR_NOTICE("Platform commit-id:  %s", PLATFORM_COMMIT);

    static THREAD_CFG_T thrd_param = {.priority = TASK_SD_PRIORITY, .stackDepth = TASK_SD_SIZE, .thrdname = "sd"};
    TUYA_CALL_ERR_LOG(tal_thread_create_and_start(&sg_sd_thrd_hdl, NULL, NULL, __example_sd_task, NULL, &thrd_param));

    return;
}

/**
 * @brief main
 *
 * @param argc
 * @param argv
 * @return void
 */
#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();

    while (1) {
        tal_system_sleep(500);
    }
}
#else

/* Tuya thread handle */
static THREAD_HANDLE ty_app_thread = NULL;

/**
 * @brief  task thread
 *
 * @param[in] arg:Parameters when creating a task
 * @return none
 */
static void tuya_app_thread(void *arg)
{
    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif