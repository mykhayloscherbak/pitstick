#ifndef SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_
#define SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_
/**
 * @file led_strip.h
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 01-09-2021
 * @version 1.30
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
	GREEN10,  /**< 10% green */
	BLUE10    /**< 10% blue */
}Colors_t;

typedef uint8_t (*pPhase_t)(const uint8_t init);

/**
 * @brief structure for blink function
 */
typedef struct
{
    uint32_t on; /**< On time */
    uint32_t off; /**< Off time */
    pPhase_t pOnPhase; /**< Callback to the "on" phase function */
    pPhase_t pOffPhase; /**< Callback to the "off" phase function */
} Blink_t;


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
 * @brief Displays strips (0 - 5) of selected color starting from the handle. Data is put to the out buffer and actual color will not be changed until updated by @ref sendDataToStrip
 * @param color Color index
 * @param nstrips Number of strips
 */

void dispStripsRevese(const Colors_t color,const uint8_t nstrips);


/**
 * @fn void dispStrip(const Colors_t, const uint8_t)
 * @brief displays strip number (0 - 4) of selected color
 *
 * @param color color
 * @param stripNo number of strip (0 - 4)
 */
void dispStrip(const Colors_t color,const uint8_t stripNo);

void fill2Pixels(const Colors_t _color, const uint8_t _from, const uint8_t _to);

/**
 * @brief Blinks the whole stick by color to black two times at 1Hz frequency
 * @param[in] init nonzero value shows that this is the first iteration of this function
 * @param[in] color The color index when the stick is on
 * @param[in] getPowerColor function that returns the color number corresponding to the current batt level
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
 * @brief Blinks between the @ref _blink.pOnPhase function out and @ref _blink.pOffPhase
 * @param init != 0 if the function is called first time
 * @param _blink the structure that contains timings and callback
 * @return !0 if buffer was changed
 */
uint8_t blink(const uint8_t init, const Blink_t * const _blink);
/**
 * @brief Sends the led buffer to the led strip.
 */
void sendDataToStrip(void);

#endif /* SOURCES_PROJECT_DL_INCLUDE_LED_STRIP_H_ */
