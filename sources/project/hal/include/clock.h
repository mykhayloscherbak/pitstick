/**
 * @file clock.h
 * @brief Contains definitions for @ref clock.c
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 07-04-2016
 * @version 1.0
 */

#ifndef SOURCE_DL_CLOCK_H_
#define SOURCE_DL_CLOCK_H_

#include <stdint.h>
#define CPU_FREQ 64000000ul
#define SYSTICK_FREQ 1000

/**
 * @brief Inits HSE as a main clock. PLL is setup to *8
 */
void Clock_HSE_Init(void);

/**
 * @brief Returns number of milliseconds from timebase start
 * @return milliscenonds
 */
uint32_t GetTicksCounter(void);
void Systick_Init(void);

/**
 * @brief Resets software timer
 * @param Timer timer variable
 */
void ResetTimer(uint32_t * const Timer);
uint8_t IsExpiredTimer(uint32_t * const Timer, const uint32_t Timeout);
uint32_t ReadTimer(uint32_t * const Timer);


#endif /* SOURCE_DL_CLOCK_H_ */
