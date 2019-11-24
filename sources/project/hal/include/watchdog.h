/**
 * @file watchdog.h
 * @brief Contains independent watchdog init and functions prototypes
 * @author Mikl Scherbak (mikl74@yahoo.com)
 * @date 02-05-2017
 * @version 1.0
 */


#ifndef SOURCE_DL_WATCHDOG_H_
#define SOURCE_DL_WATCHDOG_H_

/**
 * @brief Initialises and start wdt. Timeout is 1s
 */
void watchdog_Init(void);

/**
 * @brief Resets watchdog
 */
void Reset_Watchdog(void);

#endif /* SOURCE_DL_WATCHDOG_H_ */
