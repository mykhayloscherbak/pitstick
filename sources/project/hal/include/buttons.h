/**
 * @file buttons.h
 * @e mikl74@yahoo.com
 * @author Mykhaylo Shcherbak
 * @date 29-08-2016
 * @version 1.00
 * @brief Buttons driver with debouncing and long press detection header file
 */
#ifndef SOURCE_BLL_BUTTONS_H_
#define SOURCE_BLL_BUTTONS_H_

/**
 * @brief number of @ref Button_Process function call per second
 */
#define CALLS_PER_SECOND 100

#include <stdint.h>

/**
 * \brief Number of tics to assume the button state is stable.
 */
#define BUTTONWAIT (CALLS_PER_SECOND / 20)
/**
 * \brief Number of tics to assume the button is long pressed.
 */
#define BUTTONLONGWAIT (CALLS_PER_SECOND * 2)

/**
 * @brief Buttons id
 */
typedef enum {
	B_CONFIG = 0,/**< The single button */
	B_MAX       /**< Maximum number of buttons */
} Buttons_id_t;

/**
 * \brief Initializes the buttons driver
 */
void Buttons_Init(void);
/**
 * \brief Processes the button. Must be called from the timer interrupt
 * \param button_no The button number.
 */
void Button_Process(uint8_t button_no);


void WaitButtonPress(Buttons_id_t button); /**< Wait for button is pressed (with debouncing) */
void WaitButtonRelease(Buttons_id_t button); /**< Wait for button is released (with debouncing) */
uint8_t IsPressed(Buttons_id_t button); /**< Check for button is pressed without debouncing */
uint8_t IsShortPressed(Buttons_id_t button); /**< Check for the button is pressed but not long pressed without debouncing */
uint8_t IsSteadyReleased(Buttons_id_t button); /**< Check for button is released with debouncing */
uint8_t IsReleased(Buttons_id_t button);  /**< Check for button is released without debouncing */
uint8_t IsLongPressed(Buttons_id_t button); /**< Check if button is long pressed */
uint8_t IsSteadyPressed(Buttons_id_t button); /**< Check if button is pressed with debouncing */


#endif /* SOURCE_BLL_BUTTONS_H_ */
