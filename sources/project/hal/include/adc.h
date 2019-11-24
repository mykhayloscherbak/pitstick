/**
 * @file adc.h
 * @brief Contains ADC driver form one channel prototypes
 * @author Mykhaylo Shcherbak
 * @em mikl74@yahoo.com
 * @date 02-05-2016
 *
 */

#ifndef SOURCE_DL_ADC_H_
#define SOURCE_DL_ADC_H_
#include <stdint.h>
#define ADC_AVG (10u)

void Adc_Init(void);
uint16_t GetAdc_Voltage(void);
uint32_t getEntropy(void);


#endif /* SOURCE_DL_ADC_H_ */
