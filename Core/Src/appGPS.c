#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_adc.h>
#include <stm32f4xx_hal_gpio.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "queue.h"
#include "appOutdoorComs.h"
#include "memory.h"
#include "appSystem.h"
#include "minmea.h"
#include "lwip.h"

extern UART_HandleTypeDef huart4;
extern APP_SYSTEM_DATA *SystemState;
extern DMA_HandleTypeDef hdma_uart4_rx;
extern RTC_HandleTypeDef hrtc;

#define GPSBUFFSIZE 512

APP_GPSdata_T APP_GPSdata;

void StartGPS(void *argument) {

	RTC_TimeTypeDef sTime;
	osDelay(5000);
	HAL_UART_Receive_IT(&huart4, &APP_GPSdata.rx_byte, 1);

	for (;;) {

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		//if (HAL_UART_Receive(&huart4, &APP_GPSdata.rx_byte, 1,5000) == HAL_OK) {

		if (*(APP_GPSdata.linepos - 1) == '\n') {

			//pp is now the end of line
			switch (minmea_sentence_id(APP_GPSdata.line, false)) {
			case MINMEA_SENTENCE_RMC: {
				break;
			}
			case MINMEA_SENTENCE_GGA: {
				struct minmea_sentence_gga frame;
				if (minmea_parse_gga(&frame, APP_GPSdata.line)) {
					if (frame.time.hours != -1) {
						HAL_RTC_GetTime(&hrtc, &sTime, FORMAT_BIN);
						if ((frame.time.hours != sTime.Hours) || abs(frame.time.minutes - sTime.Minutes) > 1) {
							SystemState->sTime.Hours = frame.time.hours;
							SystemState->sTime.Minutes = frame.time.minutes;
							SystemState->sTime.Seconds = (frame.time.seconds + 60) % 60;
							HAL_RTC_SetTime(&hrtc, &SystemState->sTime, FORMAT_BIN);
							printf("GPS Time set %d:%d:%d\r\n", SystemState->sTime.Hours, SystemState->sTime.Minutes, SystemState->sTime.Seconds);
							osDelay(10000);
						}
						else
						{
							osDelay(60000);
						}
					} else {
						//SystemState->SetDateTime = false;
						osDelay(60000);
					}
					SystemState->Lat = frame.latitude.value;
					SystemState->Lon = frame.longitude.value;
					SystemState->Alt = frame.altitude.value;
					SystemState->SV = frame.satellites_tracked;

				}
				break;
			}
			case MINMEA_SENTENCE_ZDA: {
				struct minmea_sentence_zda frame;
				if (minmea_parse_zda(&frame, APP_GPSdata.line)) {
					if ((frame.date.year != SystemState->sDate.Year) || (frame.date.month != SystemState->sDate.Month)
							|| (frame.date.day != SystemState->sDate.Date)) {
						SystemState->sDate.Year = frame.date.year;
						SystemState->sDate.Month = frame.date.month;
						SystemState->sDate.Date = frame.date.day;

						SystemState->SetDateTime = 2;
					} else
						//SystemState->SetDateTime = false;
						;
				}
				break;
			}

			}
			APP_GPSdata.linepos = &APP_GPSdata.line[0];
		}
	}

}
