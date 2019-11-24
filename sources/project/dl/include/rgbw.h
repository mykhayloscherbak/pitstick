#ifndef SOURCES_PROJECT_DL_INCLUDE_RGBW_H_
#define SOURCES_PROJECT_DL_INCLUDE_RGBW_H_
/**
 * @file rgbw.h
 * @a Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 21-11-2019
 * @version 1.00
 * @brief Contains prototypes and data types for SK6812 led strip driver
 */
#include <stdint.h>

/**
 * @brief Struct defining color for each led
 */
typedef struct
{
	uint8_t R; /**< Red component (0-255) */
	uint8_t G; /**< Green component (0-255) */
	uint8_t B; /**< Blue component (0-255) */
	uint8_t W;
} Led_t;

/**
 * @brief Sends data to the strip. Number of leds is @ref NLEDS
 * @param Leds Pointer to the array of led data
 */
void displayStrip(Led_t * const Leds);


#endif /* SOURCES_PROJECT_DL_INCLUDE_RGBW_H_ */
