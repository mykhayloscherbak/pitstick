/**
 * @file timer_dma.c
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 21-11-2019
 * @version 1.00
 * @brief Contains timer2+dma driver to send data to led strip
 */

#include <stddef.h>
#include "stm32f1xx.h"
#include "timer_dma.h"
#include "gpio.h"

/**
 * @brief Inits pin to which the strip is connected as af push-pull
 */
static void pin_init(void)
{
	GPIO_TypeDef *portOut;
	uint8_t pinOut;
	Gpio_Get_Alt_PortPin(GPIO_WS_OUT,&portOut,&pinOut);
	if ( pinOut < 8 )
	{
		portOut->CRL |=  (0b11u<< (pinOut * 4)); /* 0b11 = 50Mhz */
		portOut->CRL |=  (0b10u<< (pinOut * 4 + 2)); /* 0b10 Af PP */
		portOut->CRL &=  ~((0b01u<< (pinOut * 4 + 2))); /* 0b10 Af PP */
	}
	else
	{
		pinOut -= 8u;
		portOut->CRH |=  (0b11u<< (pinOut * 4)); /* 0b11 = 50Mhz */
		portOut->CRH |=  (0b10u<< (pinOut* 4 + 2)); /* 0b10 Af PP */
		portOut->CRH &=  ~((0b01u<< (pinOut * 4 + 2))); /* 0b10 Af PP */
	}
}

void tim2_Init(void)
{
	pin_init();
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->ARR = 84 - 1; /* 1.25us from 64Mhz */
	TIM2->PSC = 0;
	TIM2->CR1 = TIM_CR1_ARPE;
	TIM2->DIER = TIM_DIER_UDE;
	TIM2->CCMR1 &= ~(TIM_CCMR1_OC1M_0);
	TIM2->CCMR1  |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 |  TIM_CCMR1_OC1PE;
	TIM2->CCR1 = 0;
	TIM2->CCER |= TIM_CCER_CC1E;
	TIM2->EGR  |= TIM_EGR_UG;

	RCC->AHBENR  |= RCC_AHBENR_DMA1EN;
	DMA1_Channel2->CCR = DMA_CCR_DIR | /* DMA_CCR_CIRC | */ DMA_CCR_MINC  | DMA_CCR_PSIZE_0 | DMA_CCR_PL_1;
	DMA1_Channel2->CPAR = (uint32_t)&TIM2->CCR1;

	TIM2->CR1 |= TIM_CR1_CEN;
}

static uint32_t baddr = 0;
static uint16_t bsize = 0;
void tim2_set_data(uint8_t * const addr, const uint16_t size)
{
	if (addr != NULL && size != 0)
	{
		baddr = (uint32_t)addr;
		bsize = size;
	}
}

void tim2_TransferBits(void)
{
	if (baddr |= 0 && bsize != 0)
	{
		static uint8_t WasStarted = 0;
		if (WasStarted != 0)
		{
			while ((DMA1->ISR & DMA_ISR_TCIF2) == 0)
			{

			}
		}

		DMA1_Channel2->CCR &= ~DMA_CCR_EN;
		DMA1_Channel2->CMAR = baddr;
		DMA1_Channel2->CNDTR = bsize;
		DMA1_Channel2->CCR |= DMA_CCR_EN;
		WasStarted = !0;
	}
}
