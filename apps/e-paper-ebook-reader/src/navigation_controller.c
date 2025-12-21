/**
 * @file navigation_controller.c
 * @brief Navigation Controller Implementation
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include "navigation_controller.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tdl_button_manage.h"
#include "board_com_api.h"

static navigation_callback_t sg_nav_callback = NULL;

/**
 * @brief Button event handler
 */
static void navigation_button_handler(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *argc)
{
    if (sg_nav_callback == NULL) {
        return;
    }
    
    // Minimal logging to reduce stack usage
    PR_DEBUG("Btn: %d", event);
    
    // Button 1: Next page (short press), Back to menu (long press)
    if (strcmp(name, BUTTON_NAME) == 0) {
        if (event == TDL_BUTTON_PRESS_DOWN) {
            sg_nav_callback(NAV_EVENT_NEXT_PAGE);
        } else if (event == TDL_BUTTON_LONG_PRESS_START) {
            sg_nav_callback(NAV_EVENT_BACK_TO_MENU);
        }
    }
    
#if defined(BUTTON_NAME_2)
    // Button 2: Previous page
    else if (strcmp(name, BUTTON_NAME_2) == 0) {
        if (event == TDL_BUTTON_PRESS_DOWN) {
            sg_nav_callback(NAV_EVENT_PREV_PAGE);
        }
    }
#endif
    
#if defined(BUTTON_NAME_3)
    // Button 3: Select/Menu navigation
    else if (strcmp(name, BUTTON_NAME_3) == 0) {
        if (event == TDL_BUTTON_PRESS_DOWN) {
            sg_nav_callback(NAV_EVENT_SELECT);
        }
    }
#endif
}

/**
 * @brief Initialize button handlers
 */
int navigation_init(void)
{
    PR_NOTICE("Initializing navigation controller...");
    
    // Button configuration
    TDL_BUTTON_CFG_T button_cfg = {
        .long_start_valid_time = 3000,
        .long_keep_timer = 1000,
        .button_debounce_time = 50,
        .button_repeat_valid_count = 2,
        .button_repeat_valid_time = 500
    };
    
    // Create button 1 (Next/Menu)
    TDL_BUTTON_HANDLE button_hdl = NULL;
    OPERATE_RET rt = tdl_button_create(BUTTON_NAME, &button_cfg, &button_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create button 1: %d", rt);
        return rt;
    }
    
    tdl_button_event_register(button_hdl, TDL_BUTTON_PRESS_DOWN, navigation_button_handler);
    tdl_button_event_register(button_hdl, TDL_BUTTON_LONG_PRESS_START, navigation_button_handler);
    
#if defined(BUTTON_NAME_2)
    // Create button 2 (Previous)
    TDL_BUTTON_HANDLE button_hdl_2 = NULL;
    rt = tdl_button_create(BUTTON_NAME_2, &button_cfg, &button_hdl_2);
    if (rt != OPRT_OK) {
        PR_WARN("Failed to create button 2: %d", rt);
    } else {
        tdl_button_event_register(button_hdl_2, TDL_BUTTON_PRESS_DOWN, navigation_button_handler);
    }
#endif
    
#if defined(BUTTON_NAME_3)
    // Create button 3 (Select)
    TDL_BUTTON_HANDLE button_hdl_3 = NULL;
    rt = tdl_button_create(BUTTON_NAME_3, &button_cfg, &button_hdl_3);
    if (rt != OPRT_OK) {
        PR_WARN("Failed to create button 3: %d", rt);
    } else {
        tdl_button_event_register(button_hdl_3, TDL_BUTTON_PRESS_DOWN, navigation_button_handler);
    }
#endif
    
    PR_NOTICE("Navigation controller initialized");
    return OPRT_OK;
}

/**
 * @brief Register callback for navigation events
 */
void navigation_register_callback(navigation_callback_t callback)
{
    sg_nav_callback = callback;
    PR_DEBUG("Navigation callback registered");
}
