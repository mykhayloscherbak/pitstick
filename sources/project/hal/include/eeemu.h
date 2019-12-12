/**
 * This file contains functions prototypes for eeprom emulation. There are two types of the storage
 * * Config
 * * PRNG current value
 * @file eeemu.h
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 19-11-2019
 * @version 1.00
 * @brief eeprom emulation header in pair with @ref eeemu.c
 */
#ifndef SOURCES_PROJECT_HAL_INCLUDE_EEEMU_H_
#define SOURCES_PROJECT_HAL_INCLUDE_EEEMU_H_

#define MAX_STORED 8 /**< The number of stored params. All params are one byte size */

#include <stdint.h>
/**
 * @brief Eeprom emulation subsystem init. It finds the index of the last saved config
 */
void eeemu_Init(void);

/**
 * @brief returns  the pointer to the last stored configuration. Size is @ref MAX_STORED
 *
 */
uint8_t * eeemuGetValue(void);
/**
 * @brief stores config block to the eeprom emulation page
 * @param value pointer to the block of values. Size is @ref MAX_STORED
 */
void eeemu_write(uint8_t * const value);
/**
 * @brief Returns previous saved prng value.
 * @return the value
 */
uint16_t eeemuSeedGet(void);
/**
 * @brief Saves current prng
 * @param seed the value
 */
void eeemuSeedSet(const uint16_t seed);


#endif /* SOURCES_PROJECT_HAL_INCLUDE_EEEMU_H_ */
