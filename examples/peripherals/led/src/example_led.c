/**
 * @file example_led.c
 * @brief LED example for Tuya IoT projects.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tkl_gpio.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#ifndef LED_PIN
#define LED_PIN TUYA_GPIO_NUM_1
#endif

#define TASK_LED_PRIORITY     THREAD_PRIO_2
#define TASK_LED_SIZE         4096

/***********************************************************
***********************variable define**********************
***********************************************************/
static THREAD_HANDLE sg_led_handle;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief led task
 *
 * @param[in] param:Task parameters
 * @return none
 */
static void __example_led_task(void *param)
{
    /*GPIO output init*/
    TUYA_GPIO_BASE_CFG_T out_pin_cfg = {
        .mode = TUYA_GPIO_PUSH_PULL, .direct = TUYA_GPIO_OUTPUT, .level = TUYA_GPIO_LEVEL_LOW};
    tkl_gpio_init(LED_PIN, &out_pin_cfg);

    while (1) {
        /* GPIO output */
        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_HIGH);
        PR_DEBUG("LED ON");
        tal_system_sleep(1000);

        tkl_gpio_write(LED_PIN, TUYA_GPIO_LEVEL_LOW);
        PR_DEBUG("LED OFF");
        tal_system_sleep(1000);
    }
}

/**
 * @brief user_main
 *
 * @return none
 */
void user_main(void)
{
    /* basic init */
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);

    PR_NOTICE("LED Example Application");

    static THREAD_CFG_T thrd_param = {.priority = TASK_LED_PRIORITY, .stackDepth = TASK_LED_SIZE, .thrdname = "led"};
    tal_thread_create_and_start(&sg_led_handle, NULL, NULL, __example_led_task, NULL, &thrd_param);
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
