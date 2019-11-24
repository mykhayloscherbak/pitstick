/**
 * @file clock.c
 * @brief Contains clock init and functions
 * @author Mikl Scherbak (mikl74@yahoo.com)
 * @date 7-04-2016
 * @version 1.0
 */
#include "stm32f1xx.h"
#include "clock.h"
#include "gpio.h"

static volatile uint32_t Counter = 0;

/**
 * @brief Inits HSE as a main clock. PLL is setup to *9
 */
void Clock_HSE_Init(void)
{
	RCC->CIR = 0x009F0000;
	FLASH->ACR |= FLASH_ACR_LATENCY_1;
	RCC->CR |= RCC_CR_HSION; // |RCC_CR_CSSON;
	while ( ! (RCC->CR & RCC_CR_HSIRDY) )
	{
	}
	RCC->CFGR &= ~RCC_CFGR_SW; /* USE HSI during setup */
	RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_ADCPRE_DIV8 | RCC_CFGR_PPRE1_2 | RCC_CFGR_PLLMULL8;
	RCC->CR |= RCC_CR_HSEON;
	while ( ! (RCC->CR & RCC_CR_HSERDY) )
	{
	}

	RCC->CR |= RCC_CR_PLLON;
	while ( ! (RCC->CR & RCC_CR_PLLRDY) )
	{
	}
	RCC->CFGR &= ~RCC_CFGR_SW_0;
	RCC->CFGR |=  RCC_CFGR_SW_1;
//	RCC->CR &= ~RCC_CR_HSION;
}

/**
 * @brief Inits systick timer interrupt for 1ms period.
 */
void Systick_Init(void)
{
	SysTick_Config( CPU_FREQ / SYSTICK_FREQ);
}

void SysTick_Handler(void);

void SysTick_Handler(void)
{
	Counter++;
}

/**
 * @brief Resets software timer
 * @param Timer timer variable
 */
void ResetTimer(uint32_t * const Timer)
{
	*Timer = Counter;
}

/**
 * @brief Checks if timer timeout has expired
 * @param Timer Timer variable
 * @param Timeout Timeout value in ms
 * @return zero if timer has not expired, non-zero else
 */
uint8_t IsExpiredTimer(uint32_t * const Timer, const uint32_t Timeout)
{
	return Counter >= *Timer + Timeout ;
}

uint32_t ReadTimer(uint32_t * const Timer)
{
	return Counter - *Timer;
}

uint32_t GetTicksCounter(void)
{
	return Counter;
}

