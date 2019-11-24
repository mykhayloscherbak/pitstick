/**
 * @file buttons.c
 * @em mikl74@yahoo.com
 * @author Mykhaylo Shcherbak
 * @date 29-08-2016
 * @version 1.00
 * @brief Buttons driver with debouncing and long press detection.
 */

#include <stdint.h>
#include "buttons.h"
#include "gpio.h"

/**
 * @brief describes the current state of the button
 */
enum ButtonState { NotExists=0, /**< No such button. Ignore all functions *///!< NotExists
  Pressed, /**< Just pressed */                                              //!< Pressed
  Released,/**< Just released  */                                            //!< Released
  SteadyPressed,/**< Steady pressed. Debouncing period passed */             //!< SteadyPressed
  SteadyReleased, /**< Steady released. Debouncing period passed */          //!< SteadyReleased
  LongPressed, /**< Long pressed */                                          //!< LongPressed
  LongReleased /**< Long released. Unused in BL */                           //!< LongReleased
};

typedef uint8_t (*pButtonFunc_t)(void);

/**
 * @brief The button  description
 */
typedef struct {
  enum ButtonState state; /**< The button state */
  pButtonFunc_t buttonFunc;
  uint16_t count; /**< Tics counter */
} Buttons_t;

static uint8_t ButtonState(void)
{
	return Gpio_Read_Bit(GPIO_BUTTON); /* Button is NC, if pressed, high level is on the pin */
}

/**
 * @brief Button state array
 */
static volatile  Buttons_t Buttons[B_MAX]={{Released,ButtonState,0}};

void Button_Process(const Buttons_id_t Button_id)
{
	volatile Buttons_t *button=Buttons+Button_id;
	if (button->state==NotExists) return;
	if ( button->buttonFunc() == 0) /* Button is released */
		switch (button->state)
		{

		case LongReleased:
			return;
		case SteadyReleased:
			if ((button->count++)>=BUTTONLONGWAIT)
			{
				button->count=BUTTONLONGWAIT;
				button->state=LongReleased;
			}
			return;
		case Released:
			if ((button->count++)>=BUTTONWAIT)
			{
				button->count=BUTTONWAIT;
				button->state=SteadyReleased;
			}
			return;
		default:
			button->count=0;
			button->state=Released;
			return;
		}
	else
	{
		switch (button->state)
		{ /* Button is pressed */
		case LongPressed:
			return;
		case SteadyPressed:
			if ((button->count++)>=BUTTONLONGWAIT)
			{
				button->count=BUTTONLONGWAIT;
				button->state=LongPressed;
			}
			return;
		case Pressed:
			if ((button->count++)>=BUTTONWAIT)
			{
				button->count=BUTTONWAIT;
				button->state=SteadyPressed;
			}
			return;
		default:
			button->count=0;
			button->state=Pressed;
			return;
		}
	}
}

void Buttons_Init(void)

{
  uint8_t b;

  /* Use initial state for filling array */
  for (b=0; b < B_MAX; b++)
  {
	  Buttons[b].state=(Buttons[b].buttonFunc() == 0) ? Released : Pressed;
  }
}

void WaitButtonPress(Buttons_id_t button)
{
   if (Buttons[button].state==NotExists) return;
   while (Buttons[button].state!=SteadyPressed&&Buttons[button].state!=LongPressed);
}

void WaitButtonRelease(Buttons_id_t button)
{
   if (Buttons[button].state==NotExists) return;
   while (Buttons[button].state!=SteadyReleased&&Buttons[button].state!=LongReleased);
}


uint8_t IsPressed(Buttons_id_t button)
{
  if (Buttons[button].state==NotExists) return 0;
   return Buttons[button].state==Pressed||Buttons[button].state==SteadyPressed||Buttons[button].state==LongPressed;
}

uint8_t IsShortPressed(Buttons_id_t button)
{
    if (Buttons[button].state==NotExists) return 0;
    return Buttons[button].state==Pressed||Buttons[button].state==SteadyPressed;
}


uint8_t IsSteadyReleased(Buttons_id_t button)
{
   if (Buttons[button].state==NotExists) return 0;
   return Buttons[button].state==SteadyReleased||Buttons[button].state==LongReleased;
}

uint8_t IsReleased(Buttons_id_t button)
{
   if (Buttons[button].state==NotExists) return 0;
   return Buttons[button].state==Released||Buttons[button].state==SteadyReleased||Buttons[button].state==LongReleased;
}

uint8_t IsLongPressed(Buttons_id_t button)
{
   if (Buttons[button].state==NotExists) return 0;
   return Buttons[button].state==LongPressed;
}

uint8_t IsSteadyPressed(Buttons_id_t button)
{

   if (Buttons[button].state==NotExists) return 0;
   return Buttons[button].state==SteadyPressed||Buttons[button].state==LongPressed;
}
