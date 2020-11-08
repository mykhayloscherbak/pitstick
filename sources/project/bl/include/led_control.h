/**
 * @file led_control.h
 * @date 7-11-2020
 * @brief Contains function that controls led strip
 */
#ifndef SOURCES_PROJECT_BL_INCLUDE_LED_CONTROL_H_
#define SOURCES_PROJECT_BL_INCLUDE_LED_CONTROL_H_
#include <stdint.h>

/**
 * @brief function that prepares led strip out data at every step (100ms)
 * @param ms - number of milliseconds that had passed after pitstick is on
 */
void led_control(uint32_t const ms);


#endif /* SOURCES_PROJECT_BL_INCLUDE_LED_CONTROL_H_ */
