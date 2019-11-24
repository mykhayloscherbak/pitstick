/**
 * @file watchdog.c
 * @brief Contains independent watchdog init and functions
 * @author Mikl Scherbak (mikl74@yahoo.com)
 * @date 02-05-2017
 * @version 1.0
 */
#define IWDG_KEY_START 0xCCCC
#define IWDG_KEY_UNLOCK 0x5555
#define IWDG_KEY_RESET 0xAAAA
#include "watchdog.h"
#include <stm32f1xx.h>


void watchdog_Init(void)
{
	IWDG->KR = IWDG_KEY_UNLOCK;
	IWDG->PR = IWDG_PR_PR_1; /*/16 0.4ms resolution count to 2560 = 1s */
	IWDG->KR = IWDG_KEY_UNLOCK;
	IWDG->RLR = 2560;
	IWDG->KR = IWDG_KEY_START;
	DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;
}

void Reset_Watchdog(void)
{
	IWDG->KR = IWDG_KEY_RESET;
}
