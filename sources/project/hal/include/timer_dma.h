#ifndef SOURCES_PROJECT_HAL_INCLUDE_TIMER_DMA_H_
#define SOURCES_PROJECT_HAL_INCLUDE_TIMER_DMA_H_
/**
 * @file timer_dma.h
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 21-11-2019
 * @version 1.00
 * @brief Contains timer2+dma driver prototypes to send data to led strip
 */
/**
 * @brief Initialization of tim2+dma
 */
void tim2_Init(void);
/**
 * @brief Sets data to be sent
 * @param addr bit array address
 * @param size bit array size
 */
void tim2_set_data(uint8_t * const addr, const uint16_t size);
/**
 * @brief Starts actual transfer
 */
void tim2_TransferBits(void);


#endif /* SOURCES_PROJECT_HAL_INCLUDE_TIMER_DMA_H_ */
