/**
 * This file contains functions implementation for eeprom emulation. There are two types of the storage
 * * Config
 * * PRNG current value
 * @file eeemu.c
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 19-11-2019
 * @version 1.00
 * @brief eeprom emulation implementation
 *
 */

#include "eeemu.h"
#include <stdint.h>
#include <string.h>
#include "stm32f1xx.h"

/**
 * @brief Storage element structure
 */
typedef struct
{
	uint8_t stored[MAX_STORED]; /**< Data itself */
	uint8_t crc8; /**< CRC8 protection */
} eeemu_storage_s;

/**
 * @brief union for the storage element
 */
typedef union
{
	uint16_t raw[sizeof(eeemu_storage_s) % sizeof(uint16_t) == 1 ? sizeof(eeemu_storage_s)/sizeof(uint16_t) + 1 : sizeof(eeemu_storage_s)/sizeof(uint16_t)]; /**< Storage element alligned to 2bytes size as a raw array*/
	eeemu_storage_s structured; /**< Structured data */
}eeemu_storage_t;

enum
{
	MAXSTORAGE = 1024/sizeof(eeemu_storage_t), /**< Maximum number of storage elements in the page */
	DEFAULT_VALUE = 0, /**< Default values for the storage elements */
	SEED_NOT_INITED = 0xFFFF /**< Value to find first uninited element for seed storage. seed/prng is 15bits long so 0xFFFF is not a valid number */
};

/**
 * @brief flash page for holding configs
 */
static volatile eeemu_storage_t __attribute__((__section__ (".eeemu"))) eeemu_array[MAXSTORAGE];
/**
 * @brief flash page for storing prng
 */
static volatile uint16_t __attribute__((__section__ (".seed"))) seeds_array[1024/sizeof(uint16_t)];

static uint16_t next_pos = 0;

/**
 * @brief Waiting for flash write operation complete
 */
inline static void  waitBusy(void)
{
	while ((FLASH->SR & FLASH_SR_BSY) != 0)
	{

	}
}

/**
 * @brief crc8 calcualation for data block
 * @param data data itself
 * @param len data length
 * @return crc8
 */
static uint8_t crc8(uint8_t *data, uint8_t const  len)
{
    uint8_t crc = 0xff;
    uint8_t i, j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief Erasing a flash page
 * @param addr flash page addr
 */
static void eeemuErasePage(uint32_t const addr)
{
	waitBusy();
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = addr;
	FLASH->CR |= FLASH_CR_STRT;
	waitBusy();
	FLASH->CR &= ~FLASH_CR_PER;
	FLASH->CR |= FLASH_CR_LOCK;
}

/**
 * @brief Searching for the last stored prng
 * @return last stored value or @ref SEED_NOT_INITED if nothing is stored
 */
static uint16_t getCurrentSeedIdx(void)
{
	uint16_t i;
	for (i = 0; i < 1024/sizeof(seeds_array[0]); i++)
	{
		if (0 != (seeds_array[i] & 0x8000))
		{
			break;
		}
	}
	return (i == 0) ? SEED_NOT_INITED : i - 1;
}


void eeemu_write(uint8_t * const values)
{
	uint8_t i = 0;
	for (i = 0 ; i < sizeof(eeemu_storage_s)/sizeof(uint16_t); i++)
	{
		if (eeemu_array[next_pos].raw[i] != 0xFFFF)
		{
			break;
		}
	}

	if (i < sizeof(eeemu_storage_s)/sizeof(uint16_t) || next_pos == MAXSTORAGE) /* Wrong record or page full */
	{
		eeemuErasePage((uint32_t)(&eeemu_array[0]));
		next_pos = 0;
	}
	eeemu_storage_t u;
	memcpy(u.structured.stored,values,MAX_STORED);
	u.structured.crc8 = crc8(values,MAX_STORED);
	waitBusy();
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	FLASH->CR |= FLASH_CR_PG;
	for (uint8_t i = 0; i < sizeof(eeemu_storage_t)/sizeof(uint16_t); i++ )
	{
		eeemu_array[next_pos].raw[i] = u.raw[i];
		waitBusy();
	}
	FLASH->CR &= ~FLASH_CR_PG;
	FLASH->CR |= FLASH_CR_LOCK;
	next_pos++;

}

void eeemu_Init(void)
{
	uint16_t i;
	for (i = 0 ; i < MAXSTORAGE; i++)
	{

		if (eeemu_array[i].structured.crc8 != crc8((uint8_t *)eeemu_array[i].structured.stored,MAX_STORED))
		{
			break;
		}
	}
	next_pos = i;
}

uint8_t * eeemuGetValue(void)
{
	static uint8_t defVal[MAX_STORED] = {DEFAULT_VALUE};
	uint8_t * retVal = defVal;
	if (next_pos != 0)
	{
		retVal = (uint8_t *)eeemu_array[next_pos - 1].structured.stored;
	}
	return retVal;
}



uint16_t eeemuSeedGet(void)
{
	const uint16_t pos = getCurrentSeedIdx();
	return (SEED_NOT_INITED == pos) ? SEED_NOT_INITED : seeds_array[pos];
}

void eeemuSeedSet(const uint16_t seed)
{
	uint16_t pos = getCurrentSeedIdx();
	if ((SEED_NOT_INITED == pos) || (1024/sizeof(seeds_array[0]) - 1 == pos))
	{
		eeemuErasePage((uint32_t)(&seeds_array[0]));
		pos = 0;
	}
	else
	{
		pos++;
	}
	waitBusy();
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	FLASH->CR |= FLASH_CR_PG;
	seeds_array[pos] = seed;
	waitBusy();
	FLASH->CR &= ~FLASH_CR_PG;
	FLASH->CR |= FLASH_CR_LOCK;

}
