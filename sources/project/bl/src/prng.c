/**
 * @file prng.c
 * @brief contains function for generation pseudo-rundonm number sequence for slalom light. x[n+1] = (a*x[n] + b) mod m is used
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 17-11-2019
 */
#include "prng.h"
#include "eeemu.h"
#include "adc.h"
enum
{
	a = 5, /**< a param from the formula */
	b = 17,/**< b param from the formula */
};

/**
 * @brief gets the next random value of the sequence
 * @param min minimal value
 * @param max maximal value
 * @return pseudorandom number
 */
uint16_t genRandom(const uint16_t min,const uint16_t max)
{
	uint16_t random = eeemuSeedGet();
	if (random == 0xFFFF)
	{
		random = getEntropy() % 0x8000;
	}
	random = (a * random +b) & 0x7fff;
	eeemuSeedSet(random);
	const uint16_t mod = max - min + 1;
	uint8_t p;
	for (p = 15; p > 0; p--)
	{
		if ((mod & (1 << p)) != 0)
		{
			break;
		}
	}
	p++;
	return (random >> (14 - p)) % mod + min;
}
