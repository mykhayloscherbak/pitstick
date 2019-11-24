/**
 * @file rgbw.c
 * @a Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 21-11-2019
 * @version 1.00
 * @brief Contains SK6812 led strip driver implementation.
 * The driver converts rgbw led data to serial array of short and long PWM pulses that are sent by dma to timer2 and use it's ch1 out
 *
 */
#include "rgbw.h"
#include "timer_dma.h"
#include "led_control.h"
#include "led_strip.h"
#include "project_conf.h"

#define CCR_0 20 /* 0.5us */
#define CCR_1 64 /* 1.2us */
#define RESET_BITS 40

/**
 * @brief Led array bits. Contains one byte for bit so each led takes 32bytes. Also @ref RESET_BITS are added and one byte to switch the out off
 */
static uint8_t BitData[NLEDS * 8 * 4 + RESET_BITS + 1];

/**
 * @brief Converts leds array to serial bit array
 * @param Leds input array. @ref BitData is used as out
 */
static void ConvertLeds(Led_t * const Leds)
{
	uint8_t i,k;

	for (i=0; i < RESET_BITS; i++)
	{
		BitData[i] = 0;
	}
	BitData[sizeof(BitData) - 1] = 0;
	for (i=0; i < NLEDS; i++)
	{
		Led_t CurrLed;
		CurrLed.R = Leds[i].R;
		CurrLed.G = Leds[i].G;
		CurrLed.B = Leds[i].B;
		CurrLed.W = Leds[i].W;
		for ( k=0; k < 8; k++)
		{
			if ((CurrLed.G & (1 << k)) != 0 )
			{
				BitData[RESET_BITS + i * 32 + 7 - k] = CCR_1;
			}
			else
			{
				BitData[RESET_BITS + i * 32 + 7 - k] = CCR_0;
			}
			if ((CurrLed.R & (1 << k)) != 0 )
			{
				BitData[RESET_BITS + i * 32 + 8 + 7 - k] = CCR_1;
			}
			else
			{
				BitData[RESET_BITS + i * 32 + 8 + 7 -  k] = CCR_0;
			}
			if ((CurrLed.B & (1 << k)) != 0 )
			{
				BitData[RESET_BITS + i * 32 + 16 + 7 - k] = CCR_1;
			}
			else
			{
				BitData[RESET_BITS + i * 32 + 16 + 7 - k] = CCR_0;
			}
			if ((CurrLed.W & (1 << k)) != 0 )
			{
				BitData[RESET_BITS + i * 32 + 24 + 7 - k] = CCR_1;
			}
			else
			{
				BitData[RESET_BITS + i * 32 + 24 + 7 - k] = CCR_0;
			}

		}
	}
}

void displayStrip(Led_t * const Leds)
{
	ConvertLeds(Leds);
	tim2_set_data(BitData,sizeof(BitData));
	tim2_TransferBits();
}
