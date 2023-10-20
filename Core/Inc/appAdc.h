/*
 * appADC.h
 *
 *  Created on: Jan 8, 2021
 *      Author: Richard
 */


#include "Macros.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define ADCAVELEN 10
#define ADCOS 10

#ifndef INC_APPADC_H_
#define INC_APPADC_H_

typedef struct{

	bool theADC_DONE;
	uint16_t adcChData[9*140];
	uint16_t AdcAve[ADCAVELEN][9];
	uint8_t avepos;
	uint64_t AdcSampleAve[9];
	uint32_t AdcMax[9];
	uint32_t AdcMin[9];
	float AdcCnt[9];

	Temps_F Tempratures;

} APP_ADC_DATA;

APP_ADC_DATA appADCdata;

#endif /* INC_APPADC_H_ */
