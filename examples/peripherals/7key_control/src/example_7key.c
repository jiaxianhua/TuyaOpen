/**
 * @file example_7key.c
 * @brief 7-key control example for the Tuya SDK
 *
 * This file demonstrates how to handle 7 buttons for navigation and control:
 * - UP, DOWN, LEFT, RIGHT for directional navigation
 * - MID for selection/confirmation
 * - SET for settings/menu
 * - RET for return/back
 *
 * Hardware connection:
 * - COM: GND
 * - UP: P27, DOWN: P31, LEFT: P36, RIGHT: P30
 * - MID: P37, SET: P32, RET: P39
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#include "tuya_cloud_types.h"
#include "tkl_output.h"
#include "tal_api.h"
#include "tdl_button_manage.h"
#include "tdd_button_gpio.h"
#include "tkl_gpio.h"

/***********************************************************
*************************macro define***********************
***********************************************************/
// GPIO pin definitions based on your wiring
#define GPIO_PIN_UP     TUYA_GPIO_NUM_27  // P27
#define GPIO_PIN_DOWN   TUYA_GPIO_NUM_31  // P31
#define GPIO_PIN_LEFT   TUYA_GPIO_NUM_36  // P36
#define GPIO_PIN_RIGHT  TUYA_GPIO_NUM_30  // P30
#define GPIO_PIN_MID    TUYA_GPIO_NUM_37  // P37
#define GPIO_PIN_SET    TUYA_GPIO_NUM_32  // P32
#define GPIO_PIN_RET    TUYA_GPIO_NUM_39  // P39

// Button names
#define BTN_NAME_UP     "UP"
#define BTN_NAME_DOWN   "DOWN"
#define BTN_NAME_LEFT   "LEFT"
#define BTN_NAME_RIGHT  "RIGHT"
#define BTN_NAME_MID    "MID"
#define BTN_NAME_SET    "SET"
#define BTN_NAME_RET    "RET"

// Button active level (pressed = LOW)
#define BUTTON_ACTIVE_LEVEL  TUYA_GPIO_LEVEL_LOW

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    char *name;
    TUYA_GPIO_NUM_E pin;
    TDL_BUTTON_HANDLE handle;
} BUTTON_INFO_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static BUTTON_INFO_T button_list[] = {
    {BTN_NAME_UP,    GPIO_PIN_UP,    NULL},
    {BTN_NAME_DOWN,  GPIO_PIN_DOWN,  NULL},
    {BTN_NAME_LEFT,  GPIO_PIN_LEFT,  NULL},
    {BTN_NAME_RIGHT, GPIO_PIN_RIGHT, NULL},
    {BTN_NAME_MID,   GPIO_PIN_MID,   NULL},
    {BTN_NAME_SET,   GPIO_PIN_SET,   NULL},
    {BTN_NAME_RET,   GPIO_PIN_RET,   NULL},
};

#define BUTTON_COUNT (sizeof(button_list) / sizeof(button_list[0]))

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Button event callback function
 *
 * @param[in] name: Button name
 * @param[in] event: Button event type
 * @param[in] argc: User data
 */
static void button_event_cb(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *argc)
{
    switch (event) {
    case TDL_BUTTON_PRESS_DOWN:
        PR_NOTICE("[%s] Single Click", name);
        break;

    case TDL_BUTTON_LONG_PRESS_START:
        PR_NOTICE("[%s] Long Press Start", name);
        break;

    case TDL_BUTTON_LONG_PRESS_HOLD:
        PR_NOTICE("[%s] Long Press Hold", name);
        break;

    case TDL_BUTTON_PRESS_UP:
        PR_DEBUG("[%s] Button Released", name);
        break;

    default:
        break;
    }
}

/**
 * @brief Register button hardware drivers
 *
 * @return OPRT_OK on success, error code otherwise
 */
static OPERATE_RET register_button_hardware(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Register each button with the hardware driver
    for (int i = 0; i < BUTTON_COUNT; i++) {
        BUTTON_GPIO_CFG_T button_hw_cfg = {
            .pin   = button_list[i].pin,
            .level = BUTTON_ACTIVE_LEVEL,
            .mode  = BUTTON_TIMER_SCAN_MODE,  // Use timer scan mode for stability
            .pin_type.gpio_pull = TUYA_GPIO_PULLUP,  // Enable pull-up resistor
        };

        rt = tdd_gpio_button_register(button_list[i].name, &button_hw_cfg);
        if (rt != OPRT_OK) {
            PR_ERR("Failed to register hardware for button %s (GPIO %d), error: %d", 
                   button_list[i].name, button_list[i].pin, rt);
            return rt;
        }

        PR_DEBUG("Hardware registered for button %s (GPIO %d)", 
                 button_list[i].name, button_list[i].pin);
    }

    return OPRT_OK;
}

/**
 * @brief Initialize all buttons
 *
 * @return OPRT_OK on success, error code otherwise
 */
static OPERATE_RET init_buttons(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Button configuration
    TDL_BUTTON_CFG_T button_cfg = {
        .long_start_valid_time = 2000,      // 2 seconds for long press
        .long_keep_timer = 500,             // 500ms repeat interval during long press
        .button_debounce_time = 50,         // 50ms debounce
        .button_repeat_valid_count = 2,     // Require 2 consistent reads
        .button_repeat_valid_time = 500     // 500ms for repeat detection
    };

    // Create and register all buttons
    for (int i = 0; i < BUTTON_COUNT; i++) {
        rt = tdl_button_create(button_list[i].name, &button_cfg, &button_list[i].handle);
        if (rt != OPRT_OK) {
            PR_ERR("Failed to create button %s, error: %d", button_list[i].name, rt);
            return rt;
        }

        // Register event callbacks
        tdl_button_event_register(button_list[i].handle, TDL_BUTTON_PRESS_DOWN, button_event_cb);
        tdl_button_event_register(button_list[i].handle, TDL_BUTTON_LONG_PRESS_START, button_event_cb);
        tdl_button_event_register(button_list[i].handle, TDL_BUTTON_LONG_PRESS_HOLD, button_event_cb);

        PR_NOTICE("Button %s initialized successfully", button_list[i].name);
    }

    return OPRT_OK;
}

/**
 * @brief user_main
 *
 * @return none
 */
void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;

    /* Basic initialization */
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);

    PR_NOTICE("========================================");
    PR_NOTICE("7-Key Control Example");
    PR_NOTICE("========================================");
    PR_NOTICE("Project name:        %s", PROJECT_NAME);
    PR_NOTICE("App version:         %s", PROJECT_VERSION);
    PR_NOTICE("Compile time:        %s %s", __DATE__, __TIME__);
    PR_NOTICE("TuyaOpen version:    %s", OPEN_VERSION);
    PR_NOTICE("Platform chip:       %s", PLATFORM_CHIP);
    PR_NOTICE("Platform board:      %s", PLATFORM_BOARD);
    PR_NOTICE("========================================");

    // Register button hardware drivers first
    rt = register_button_hardware();
    if (rt != OPRT_OK) {
        PR_ERR("Failed to register button hardware");
        return;
    }
    PR_NOTICE("Button hardware registered successfully");

    // Initialize buttons
    rt = init_buttons();
    if (rt != OPRT_OK) {
        PR_ERR("Failed to initialize buttons");
        return;
    }

    PR_NOTICE("All buttons initialized successfully!");
    PR_NOTICE("Button mapping:");
    PR_NOTICE("  UP    -> P27");
    PR_NOTICE("  DOWN  -> P31");
    PR_NOTICE("  LEFT  -> P36");
    PR_NOTICE("  RIGHT -> P30");
    PR_NOTICE("  MID   -> P37");
    PR_NOTICE("  SET   -> P32");
    PR_NOTICE("  RET   -> P39");
    PR_NOTICE("Press any button to test...");
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
 * @brief Task thread
 *
 * @param[in] arg: Parameters when creating a task
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
