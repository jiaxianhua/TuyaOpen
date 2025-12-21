/**
 * @file navigation_controller.h
 * @brief Navigation Controller for E-Book Reader
 * 
 * Handles button input and translates it into application commands.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __NAVIGATION_CONTROLLER_H__
#define __NAVIGATION_CONTROLLER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Navigation events
 */
typedef enum {
    NAV_EVENT_NEXT_PAGE,
    NAV_EVENT_PREV_PAGE,
    NAV_EVENT_MENU_UP,
    NAV_EVENT_MENU_DOWN,
    NAV_EVENT_SELECT,
    NAV_EVENT_BACK_TO_MENU
} navigation_event_t;

/**
 * @brief Navigation callback function type
 */
typedef void (*navigation_callback_t)(navigation_event_t event);

/**
 * @brief Initialize button handlers
 * 
 * @return OPRT_OK on success, error code otherwise
 */
int navigation_init(void);

/**
 * @brief Register callback for navigation events
 * 
 * @param[in] callback Callback function to handle navigation events
 */
void navigation_register_callback(navigation_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __NAVIGATION_CONTROLLER_H__ */
