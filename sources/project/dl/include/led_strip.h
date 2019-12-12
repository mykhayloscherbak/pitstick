#ifndef SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_
#define SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_
/**
 * @file led_strip.h
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 20-11-2019
 * @version 1.00
 * @brief Contains common functions prototypes for led strip control.
 */
#include <stdint.h>

/**
 * @brief colors names. Should be the same if using rgb/rgbw strips
 */
typedef enum
{
	BLACK = 0,
	RED,
	GREEN,
	BLUE,
	WHITE,
	YELLOW,
	MAGENTA,
	CYAN,
	DARK_RED,
	ORANGE,
	REDDER,   /**< More red than @ref DARK_RED */
}Colors_t;

/**
 * @brief Function that returns the current Vbat level color
 */
typedef Colors_t (*pPowerColorFunc_t)(void);

/**
 * @brief Set the current brightness. All operations for now on will be performed at this brightness until it is changed by this function
 * @param brighness_a The brighness level (0-3 for now).
 */
void setBrightness(const uint8_t brighness_a);
/**
 * @brief Puts a pixel to the out buffer. Does not change the led color until updated
 * @param row Row number (0-1)
 * @param pos Position (0-@ref NLEDS / 2 - 1)
 * @param color Color index
 */
void putPixel(const uint8_t row,const uint8_t pos, const Colors_t color);
/**
 * @brief Puts two  pixels at the same position in both rows. Data is put to the out buffer and actual color will not be changed until updated by @ref sendDataToStrip
 * @param color Color index
 * @param pos Position (0 - @ref NLEDS / 2 - 1)
 */
void put2pixels(const Colors_t color,const uint8_t pos);
/**
 * @brief Fills the whole stick by  the color. Data is put to the out buffer and actual color will not be changed until updated by @ref sendDataToStrip
 * @param color Color index
 */
void showFull(const Colors_t color);

/**
 * @brief Shows total stick with the same color if init is nonzero. Does nothing else
 * @param color Color code to fill
 * @param init Init mode. Function works if it's non zero only.
 * @return if nonzero the strip must be updated by @ref sendDataToStrip
 */
uint8_t showFullWithInit(const Colors_t color, const uint8_t init);

/**
 * @brief Displays strips (0 - 5) of selected color. Data is put to the out buffer and actual color will not be changed until updated by @ref sendDataToStrip
 * @param color Color index
 * @param nstrips Number of strips
 */

void dispStrips(const Colors_t color, const uint8_t nstrips);
/**
 * @brief Blinks the whole stick by color to black two times at 1Hz frequency
 * @param init Is non-zero if it's the first time the function is called and zero otherwise
 * @param color The color index when the stick is on
 * @param getPowerColor A function that returns the color corresponding to the current Vbat
 * @return  if nonzero the strip must be updated by @ref sendDataToStrip
 */
uint8_t blinkTwice(const uint8_t init,const Colors_t color,pPowerColorFunc_t const getPowerColor);
/**
 * @brief Displays a number as two bars. One for tens and the other for ones. Leds in the bars are separated by one black led per each 3
 * @param colorL Color index for tens
 * @param colorR Color index for ones
 * @param Number The number to display
 */
void displayNumber(const Colors_t colorL, const Colors_t colorR,const uint8_t Number);
/**
 * @brief Sends the led buffer to the led strip.
 */
void sendDataToStrip(void);

#endif /* SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_ */
