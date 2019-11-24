/**
 * @file prng.h
 * @brief Contains prototypes  of function that returns random value between min and max.
 * Caculated value is stored in flash to be  used as initial one next time.
 */
#ifndef SOURCES_PROJECT_BL_INCLUDE_PRNG_H_
#define SOURCES_PROJECT_BL_INCLUDE_PRNG_H_

#include <stdint.h>
/**
 * @brief Generates a pseudo-random number in range [min..max] Result is stored in the flash to be used next time
 * @param min minimal value of the generated random
 * @param max maximal value of the generated random number
 * @return random number.
 */
uint16_t genRandom(const uint16_t min,const uint16_t max);

#endif /* SOURCES_PROJECT_BL_INCLUDE_PRNG_H_ */
