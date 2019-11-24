/**
 * @file gpio.c
 * @brief Contains init and access functions for Gpio
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 14-01-2019
 * @version 1.1
 */

#include <stddef.h>
#include "stm32f1xx.h"
#include "gpio.h"


typedef enum
{
	GPIO_MODE_IN =0,
	GPIO_MODE_OUT,
	GPIO_MODE_DO_NOT_TOUCH
} Gpio_Mode_t;

#pragma pack(1)
/**
 * @brief Gpio pin configuration
 */
typedef struct
{
	GPIO_TypeDef *Port; /**< Pointer to port */
	uint8_t Pin; /**< PinNumber */
	Gpio_Mode_t Mode; /**< Mode (in,out, analog) */
}Gpio_Config_t;

static const Gpio_Config_t Gpio_Config[GPIO_TOTAL]=
{
		[GPIO_WS_OUT]       = {.Port = GPIOA,.Pin = 0,  .Mode = GPIO_MODE_DO_NOT_TOUCH}, /* Nucleo pin cn7.28 */
		[GPIO_BUTTON]       = {.Port = GPIOA,.Pin = 2,  .Mode = GPIO_MODE_IN},
		[GPIO_HEARTBEAT] 	= {.Port = GPIOC,.Pin = 13,	.Mode = GPIO_MODE_OUT	}
};

void Gpio_Init(void)
{
	Gpio_Desc_t Pin;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1;
	AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG_0 | AFIO_MAPR_SWJ_CFG_2);
	for ( Pin = 0; Pin < GPIO_TOTAL; Pin++)
	{
		if ( Gpio_Config[Pin].Pin < 8 )
		{
			if ( Gpio_Config[Pin].Mode == GPIO_MODE_IN)
			{
				(Gpio_Config[Pin].Port)->CRL &= ~( 0b11u<< ( Gpio_Config[Pin].Pin * 4));
				(Gpio_Config[Pin].Port)->CRL &= ~( 0b01u<< ( Gpio_Config[Pin].Pin * 4 + 2));
				(Gpio_Config[Pin].Port)->CRL |=  ( 0b10u<< ( Gpio_Config[Pin].Pin * 4 + 2));
				(Gpio_Config[Pin].Port)->ODR |=  ( 1u<< Gpio_Config[Pin].Pin ); /* Pull-Up */
			}
			else
			{
				if ( Gpio_Config[Pin].Mode == GPIO_MODE_OUT )
				{
					(Gpio_Config[Pin].Port)->CRL &= ~( 0b10u<< ( Gpio_Config[Pin].Pin * 4));
					(Gpio_Config[Pin].Port)->CRL |=  ( 0b10u<< ( Gpio_Config[Pin].Pin * 4));
					(Gpio_Config[Pin].Port)->CRL &= ~( 0b11u<< ( Gpio_Config[Pin].Pin * 4 + 2));
					(Gpio_Config[Pin].Port)->BRR |=  ( 1u<< Gpio_Config[Pin].Pin );
				}
			}
		}
		else
		{
			if ( Gpio_Config[Pin].Mode == GPIO_MODE_IN)
			{
				(Gpio_Config[Pin].Port)->CRH &= ~( 0b11u<< ( (Gpio_Config[Pin].Pin - 8) * 4));
				(Gpio_Config[Pin].Port)->CRH &= ~( 0b01u<< ( (Gpio_Config[Pin].Pin - 8) * 4 + 2));
				(Gpio_Config[Pin].Port)->CRH |=  ( 0b10u<< ( (Gpio_Config[Pin].Pin - 8) * 4 + 2));
				(Gpio_Config[Pin].Port)->ODR |=  ( 1u<< Gpio_Config[Pin].Pin ); /* Pull-Up */
			}
			else
			{
				if ( Gpio_Config[Pin].Mode == GPIO_MODE_OUT )
				{
					(Gpio_Config[Pin].Port)->CRH &= ~( 0b10u<< ( (Gpio_Config[Pin].Pin - 8) * 4));
					(Gpio_Config[Pin].Port)->CRH |=  ( 0b10u<< ( (Gpio_Config[Pin].Pin - 8) * 4));
					(Gpio_Config[Pin].Port)->CRH &= ~( 0b11u<< ( (Gpio_Config[Pin].Pin -8 ) * 4 + 2));
					(Gpio_Config[Pin].Port)->BRR |=  ( 1u<< Gpio_Config[Pin].Pin );
				}
			}
		}
	}
}

void Gpio_Set_Bit(Gpio_Desc_t Gpio)
{
	if ( Gpio < GPIO_TOTAL )
	{
		(Gpio_Config[Gpio].Port)->BSRR = 1u<<Gpio_Config[Gpio].Pin;
	}
}

void Gpio_Clear_Bit(Gpio_Desc_t Gpio)
{
	if ( Gpio < GPIO_TOTAL )
	{
		(Gpio_Config[Gpio].Port)->BSRR = 1u<<(Gpio_Config[Gpio].Pin + 16);
	}
}

uint8_t Gpio_Read_Bit ( Gpio_Desc_t Gpio)
{
	uint8_t RetVal = 0;
	if ( Gpio < GPIO_TOTAL )
	{
		if ( ((Gpio_Config[Gpio].Port)->IDR & ( 1u<<(Gpio_Config[Gpio].Pin))) !=0 )
		{
			RetVal = 1;
		}
		else
		{
			RetVal = 0;
		}
	}
	return RetVal;
}

void Gpio_Get_Alt_PortPin(const Gpio_Desc_t Gpio,GPIO_TypeDef ** const Port,uint8_t * const Pin)
{
	if (Gpio < GPIO_TOTAL)
	{
		if (GPIO_MODE_DO_NOT_TOUCH == Gpio_Config[Gpio].Mode && NULL != Port && NULL != Pin)
		{
			*Port = Gpio_Config[Gpio].Port;
			*Pin = Gpio_Config[Gpio].Pin;
		}
	}
}
