/**
 * @file board_button_config.h
 * @brief Button configuration for network novel reader
 * 
 * Define BUTTON_NAME to enable button support.
 * You need to set the correct GPIO pin for your board.
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __BOARD_BUTTON_CONFIG_H__
#define __BOARD_BUTTON_CONFIG_H__

// Button name - required by TDL button system
#define BUTTON_NAME "novel_button"

// Button GPIO configuration
// IMPORTANT: Change this to match your actual button GPIO pin!
// Common T5AI button pins: GPIO_17, GPIO_18, GPIO_19, GPIO_26
// Check your board schematic for the correct pin number
#ifndef BOARD_BUTTON_PIN
#define BOARD_BUTTON_PIN TUYA_GPIO_NUM_12  // Change this to your button GPIO
#endif

#ifndef BOARD_BUTTON_ACTIVE_LV
#define BOARD_BUTTON_ACTIVE_LV TUYA_GPIO_LEVEL_LOW  // Active low (button pressed = LOW)
#endif

#endif /* __BOARD_BUTTON_CONFIG_H__ */
