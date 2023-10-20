#include <appAdc.h>
#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_adc.h>
#include <stm32f4xx_hal_gpio.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"
#include <math.h>

extern APP_SYSTEM_DATA *SystemState;
extern ADC_HandleTypeDef hadc1;

static float tempCal(float ad) {
	float temp;
	if (ad == 0)
		return -100;
	//temp = (0.0307f * ad) - 35.263;
	temp = (0.0307f * ad) - 39.263;

	return temp;
}

void StartAdcTask(void *argument) {
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	SystemState->Measured.Tempratures.ReturnAir = (int) 2800;
	SystemState->Measured.Tempratures.SupplyAir = (int) 2800;
	SystemState->Measured.Tempratures.Room = (int) 2800;
	SystemState->Measured.Tempratures.IndoorCoil = (int) 2800;
	SystemState->Measured.Tempratures.CompDischarge = (int) 2800;
	SystemState->Measured.Tempratures.Suction = (int) 2800;
	SystemState->Measured.Tempratures.OutdoorCoil = (int) 2800;
	SystemState->Measured.Tempratures.OutsideAir = (int) 2800;

	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_ADC_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ADC, 10000);
			continue;
		}

		if (appADCdata.theADC_DONE) {

			HAL_ADC_Stop_DMA(&hadc1);

			for (int i = 0; i < 9; i++) {
				appADCdata.AdcSampleAve[i] = 0;
				appADCdata.AdcMax[i] = 0x00;
				appADCdata.AdcMin[i] = 0xFFFF;
				for (int p = 0; p < ADCOS; p++) {
					appADCdata.AdcMax[i] = MAX(appADCdata.AdcMax[i], appADCdata.adcChData[(p * 9) + i]);
					appADCdata.AdcMin[i] = MIN(appADCdata.AdcMin[i], appADCdata.adcChData[(p * 9) + i]);
					appADCdata.AdcSampleAve[i] += appADCdata.adcChData[(p * 9) + i];
				}
				appADCdata.AdcSampleAve[i] /= ADCOS;

				if ((appADCdata.AdcSampleAve[i] >= 200) && (appADCdata.AdcSampleAve[i] <= 3950))
					//appADCdata.AdcAve[appADCdata.avepos][i] = appADCdata.adcChData[i];
					appADCdata.AdcAve[appADCdata.avepos][i] = appADCdata.AdcSampleAve[i];
				else
					appADCdata.AdcAve[appADCdata.avepos][i] = 0;
			}

			//Now Get min max of the Room sensor /Possbily Curretn
			/*if (SystemState->Paramters.PARAM[PARAM_MEASURE_P_P] == 1) {
				appADCdata.AdcAve[appADCdata.avepos][2] = appADCdata.AdcMax[2] - appADCdata.AdcMin[2];
			}*/

			appADCdata.avepos = appADCdata.avepos >= ADCAVELEN - 1 ? 0 : appADCdata.avepos + 1;

			appADCdata.theADC_DONE = false;
		}

		//if (TIMEOUTTEST(appADCdata.lastread) > 1000)
		{
			//Calcuate Data and send dma away

			uint32_t Sum = 0;
			uint32_t Cnt = 0;
			for (int i = 0; i < 9; i++) {
				Sum = 0;
				Cnt = 0;
				for (int p = 0; p < ADCAVELEN; p++) {
					if (appADCdata.AdcAve[p][i] != 0) {
						Sum += appADCdata.AdcAve[p][i];
						Cnt++;
					}
				}

				if (Cnt > 0)
					appADCdata.AdcCnt[i] = (float) Sum / (float) Cnt;
				else
					appADCdata.AdcCnt[i] = 0;
			}

			appADCdata.Tempratures.ReturnAir = (tempCal(appADCdata.AdcCnt[0]));
			appADCdata.Tempratures.SupplyAir = tempCal(appADCdata.AdcCnt[1]);
			appADCdata.Tempratures.Room = tempCal(appADCdata.AdcCnt[2]);
			appADCdata.Tempratures.IndoorCoil = tempCal(appADCdata.AdcCnt[3]);
			appADCdata.Tempratures.CompDischarge = tempCal(appADCdata.AdcCnt[4]);
			appADCdata.Tempratures.Suction = tempCal(appADCdata.AdcCnt[5]);
			appADCdata.Tempratures.OutdoorCoil = tempCal(appADCdata.AdcCnt[6]);
			appADCdata.Tempratures.OutsideAir = (tempCal(appADCdata.AdcCnt[7]));

			if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_SIM)) {
				SystemState->Measured.Tempratures.ReturnAir = (int) (appADCdata.Tempratures.ReturnAir * 100);
				SystemState->Measured.Tempratures.SupplyAir = (int) (appADCdata.Tempratures.SupplyAir * 100);
				SystemState->Measured.Tempratures.Room = (int) (appADCdata.Tempratures.Room * 100);
				SystemState->Measured.Tempratures.IndoorCoil = (int) (appADCdata.Tempratures.IndoorCoil * 100);
				SystemState->Measured.Tempratures.CompDischarge = (int) (appADCdata.Tempratures.CompDischarge * 100);
				SystemState->Measured.Tempratures.Suction = (int) (appADCdata.Tempratures.Suction * 100);
				SystemState->Measured.Tempratures.OutdoorCoil = (int) (appADCdata.Tempratures.OutdoorCoil * 100);
				SystemState->Measured.Tempratures.OutsideAir = (int) (appADCdata.Tempratures.OutsideAir * 100);
				//SystemState->Measured.BatteryVoltage = ((float)appADCdata.AdcCnt[8]/4096.0)*100;
				SystemState->Measured.BatteryVoltage = ((((float) appADCdata.AdcCnt[8] / 4096.0) * 2.496) / 0.039385207) * 100.0;

				/*if (SystemState->Paramters.PARAM[PARAM_MEASURE_P_P] == 1) {
					//SystemState->Measured.Tempratures.Room = (int) (((appADCdata.AdcCnt[2] / 4096.0) * 2.496) / 10e-3) * 100.0; //Voltage P-P /10e-3 = A
					SystemState->Measured.Tempratures.Room = (int) appADCdata.AdcCnt[2]; //Voltage P-P /10e-3 = A
				}*/
			}

			HAL_ADC_Stop_DMA(&hadc1);
			HAL_ADC_Start_DMA(&hadc1, (uint32_t*) &appADCdata.adcChData[0], 9 * (ADCOS));
		}

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ADC, 5000);

	}
	/* USER CODE END 5 */
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	appADCdata.theADC_DONE = true;
	HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_ERR_Pin);
}

