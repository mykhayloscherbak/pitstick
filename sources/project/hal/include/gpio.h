/**
 * @file gpio.h
 * @brief Contains definitions for @ref gpio.c
 * @date 12-04-2016
 * @author Mikl Scherbak
 * @em mikl74@yahoo.com
 * @version 1.0
 */

#ifndef SOURCE_DL_GPIO_H_
#define SOURCE_DL_GPIO_H_

#include "stm32f1xx.h"

/**
 * @brief Descriptions of GPIO pins
 */
typedef enum
{
	GPIO_HEARTBEAT = 0,/**< HEARTBEAT led */
	GPIO_WS_OUT,       /**< Led strip out */
	GPIO_BUTTON,       /**< Button in */
	GPIO_TOTAL         /**< Total number of GPIOs */
}Gpio_Desc_t;

void Gpio_Init(void);

/**
 * @brief Sets the corresponding GPIO pin
 * @param Gpio pin
 */
void Gpio_Set_Bit(Gpio_Desc_t Gpio);
/**
 * @brief Reads the corresponding GPIO pin
 * @param Gpio pin
 */
void Gpio_Clear_Bit(Gpio_Desc_t Gpio);

uint8_t Gpio_Read_Bit ( Gpio_Desc_t Gpio);
/**
 * @brief returns pin and port for selected AF GPIO
 * @param Gpio pin id
 * @param Port Pointer where port pointer will be placed. Must not be NULL
 * @param Pin Pointer where pin number will be placed. Must not be NULL
 */
void Gpio_Get_Alt_PortPin(const Gpio_Desc_t Gpio,GPIO_TypeDef ** const Port,uint8_t * const Pin);

#endif /* SOURCE_DL_GPIO_H_ */
