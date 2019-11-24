/**
 * @file heartbeat.c
 * @date 11-11-2019
 * @brief Contains heartbeat toggling function
 */
#include <stdint.h>
#include "heartbeat.h"
#include "gpio.h"

void Toggle_Heartbeat (void)
{
	static uint8_t flag = 0;

	if (flag)
	{
		Gpio_Set_Bit(GPIO_HEARTBEAT);
	}
	else
	{
		Gpio_Clear_Bit(GPIO_HEARTBEAT);
	}
	flag = !flag;
}
