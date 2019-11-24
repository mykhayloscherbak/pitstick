/**
 * @file bll.h
 * @date 7-11-2019
 * @brief Contains business logic function prototype
 */

#ifndef SOURCES_PROJECT_BL_INCLUDE_BLL_H_
#define SOURCES_PROJECT_BL_INCLUDE_BLL_H_

#include <stdint.h>
/**
 * @brief contains one iteration of the main loop
 * @return 0 tick was not expired or 1 if tick expired.
 */
uint8_t MainLoop_Iteration(void);

#endif /* SOURCES_PROJECT_BL_INCLUDE_BLL_H_ */
