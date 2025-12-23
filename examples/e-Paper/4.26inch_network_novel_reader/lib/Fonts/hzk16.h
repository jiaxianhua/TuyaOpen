/**
 * @file hzk16.h
 * @brief HZK16 Chinese font support for GBK encoding
 * 
 * HZK16 is a 16x16 dot matrix Chinese font library
 * Each character occupies 32 bytes (16x16 bits)
 * 
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __HZK16_H
#define __HZK16_H

#include <stdint.h>

/**
 * @brief Get font data for a GBK character
 * 
 * @param gb_high GBK high byte (0x81-0xFE)
 * @param gb_low GBK low byte (0x40-0xFE, except 0x7F)
 * @param buffer Buffer to store 32 bytes of font data
 * @return 0 on success, -1 on error
 * 
 * @note This function uses embedded font data for common characters
 *       For a full implementation, you would load from HZK16 file
 */
int hzk16_get_font_data(uint8_t gb_high, uint8_t gb_low, uint8_t *buffer);

/**
 * @brief Check if character has font data available
 * 
 * @param gb_high GBK high byte
 * @param gb_low GBK low byte
 * @return 1 if available, 0 if not
 */
int hzk16_has_font(uint8_t gb_high, uint8_t gb_low);

#endif /* __HZK16_H */
