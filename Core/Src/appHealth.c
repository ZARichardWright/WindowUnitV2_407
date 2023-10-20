/*
 * appHealth.c
 *
 *  Created on: Mar 23, 2021
 *      Author: Richard
 */

#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"
#include "bin.h"
#include <string.h>
#include "w25x40Rw.h"
#include <time.h>
#include "lwip.h"

//#define ROTATEMIN
extern APP_SYSTEM_DATA *SystemState;
extern RTC_HandleTypeDef hrtc;

extern uint8_t ScaleToPercent(uint8_t Hz);

RTC_AlarmTypeDef sAlarm_A;
RTC_AlarmTypeDef sAlarm_B;
HEALTH_MONITORS PrevReading;
HEALTH_MONITORS CurrReading;
HEALTH_MONITORS_F CurrHealth;
uint8_t First = true;
uint8_t PrevMin = 0;

float ScaleHealth(HEALTH_MONITORS Today) {
	//Need input all values are *100 of real units
	float SclaedH = 0.0;

	SclaedH = ((Today.Comp) * (float) (SystemState->HealthScale[0] / 100.0));
	SclaedH += ((Today.OA * 5) * (float) (SystemState->HealthScale[1] / 100.0));
	SclaedH += ((Today.F8 * 10) * (float) (SystemState->HealthScale[2] / 100.0));
	SclaedH += ((Today.SA_Fan * 1.428) * (float) (SystemState->HealthScale[3] / 100.0));
	SclaedH += ((Today.O_Fan) * (float) (SystemState->HealthScale[4] / 100.0));
	SclaedH += ((Today.Current * 10) * (float) (SystemState->HealthScale[5] / 100.0));

	//100 == Worst case

	return 100.0 - (SclaedH / 100.0); //REtuirn the Oposite so Large numbers are healthy
}

void AddtoBand(uint16_t Value, TimeInBand *theBand) {
	if (Value < 2000)
		theBand->Band1++;
	else if ((Value >= 2000) && (Value < 4000))
		theBand->Band2++;
	else if ((Value >= 4000) && (Value < 6000))
		theBand->Band3++;
	else if ((Value >= 6000) && (Value < 8000))
		theBand->Band4++;
	else if ((Value >= 8000))
		theBand->Band5++;

}

void ComputeStats() {

	if (SystemState->Stats.Save >= 5) {
		memset(&SystemState->Stats, 0x00, sizeof(_AllTimeStats));
	}

	SystemState->Stats._TotalMin++;

	if (SystemState->PoweredUp) {
		SystemState->Stats.Powered++;

		switch (SystemState->Current.CoolingMode) {

		case MODE_COOLING:
			SystemState->Stats._TotalMin_Comp++;
			break;
		case MODE_ECO:
			SystemState->Stats._TotalMin_FreeAir++;
			break;
		case MODE_SUPER_ECO:
			SystemState->Stats._TotalMin_SuperEco++;
			break;
		case MODE_FORCED_ECO:
			SystemState->Stats._TotalMin_ForcedEco++;
			break;
		case MODE_FULL:
			break;
		}

		if (SystemState->Current.PowerMode == MODE_NORMAL)
			SystemState->Stats._TotalMin_200++;
		else
			SystemState->Stats._TotalMin_48++;

		AddtoBand((uint16_t) ScaleToPercent(SystemState->Current.Compressor.Freq), &SystemState->Stats._CompBands);
		AddtoBand((SystemState->Current.Fans[FAN_PORT_INDOOR].Speed), &SystemState->Stats._SuppyFanBands);
		AddtoBand((SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed), &SystemState->Stats._OutdoorFanBands);

		AddtoBand((SystemState->ExternalCurrent1), &SystemState->Stats._CurrentBands); //10A ==100%

		AddtoBand((SystemState->Measured.F8 - 2800) * 10.0, &SystemState->Stats._F8Bands); //10Deg > 28 = 100%
		AddtoBand((SystemState->Measured.Tempratures.OutsideAir - 2500) * 5.0, &SystemState->Stats._OutDoorTempBands); //20Deg > 250 == 100%
	}
	//Now for the Bands Lets save this first
	SystemState->Stats.Save = 1;
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {

	HAL_RTC_GetTime(hrtc, &SystemState->sTime, FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &SystemState->sDate, FORMAT_BIN);

	if (SystemState->sTime.Minutes != PrevMin) {
		ComputeStats();
		if((SystemState->sTime.Minutes == 0))
		{
			//this is once an hors
			if (SystemState->Paramters.PARAM[PARAM_HASSLAVE]) {
				if (SystemState->Roatte_RemainTime == 0) {
					if (SystemState->MasterState != MASTER_WARMUP_SLAVE){
						SystemState->MasterState = MASTER_BOOT_SLAVE;
						SystemState->RotateReason = ROTATE_REASON_TIMMER;
					}

					SystemState->Roatte_RemainTime = SystemState->Paramters.PARAM[PARAM_ROTATE_TIME];
				} else if (SystemState->Roatte_RemainTime > 0) {
					SystemState->Roatte_RemainTime = SystemState->Roatte_RemainTime - 1;
				}
			}
		}
		printf("Save At %d:%d:%d\r\n", SystemState->sTime.Hours, SystemState->sTime.Minutes, SystemState->sTime.Seconds);
		if ((SystemState->sTime.Hours == 00) && (SystemState->sTime.Minutes == 0)) {
			//if ((sTime.Minutes %4 == 0)) {
			//REset Integration

			SystemState->DailyHealth[6] = SystemState->DailyHealth[5];
			SystemState->DailyHealth[5] = SystemState->DailyHealth[4];
			SystemState->DailyHealth[4] = SystemState->DailyHealth[3];
			SystemState->DailyHealth[3] = SystemState->DailyHealth[2];
			SystemState->DailyHealth[2] = SystemState->DailyHealth[1];
			SystemState->DailyHealth[1] = SystemState->DailyHealth[0];
			//Now compute new Health

			SystemState->DailyHealth[0].Comp = CurrHealth.Comp * 100.0;

			SystemState->DailyHealth[0].Current = CurrHealth.Current * 100.0;
			SystemState->DailyHealth[0].Current = CLAMP(SystemState->DailyHealth[0].Current, 10 * 100, 0); //10A

			SystemState->DailyHealth[0].F8 = CurrHealth.F8 * 100.0;
			SystemState->DailyHealth[0].F8 = CLAMP(SystemState->DailyHealth[0].F8, 10 * 100, 0); //10Deg

			SystemState->DailyHealth[0].OA = CurrHealth.OA * 100.0;
			SystemState->DailyHealth[0].OA = CLAMP(SystemState->DailyHealth[0].OA, 20 * 100, 0); //20Deg

			SystemState->DailyHealth[0].O_Fan = CurrHealth.O_Fan * 100.0;

			if (CurrHealth.SA_Fan > 30.00)
				SystemState->DailyHealth[0].SA_Fan = (CurrHealth.SA_Fan - 30.00) * 100.0;
			else
				SystemState->DailyHealth[0].SA_Fan = 0;

			//Now Combine all Values
			float TodayH = ScaleHealth(SystemState->DailyHealth[0]); //all values *100
			SystemState->RunningHTotal += TodayH;
			SystemState->DaysRun++;
			float RobFactor = (float) (1 - (SystemState->DaysRun * (0.7 / 5475))) * 100.0;
			SystemState->CurrentHealth = ((float) (SystemState->RunningHTotal / SystemState->DaysRun)) * (RobFactor); //Robs fudge

#ifndef ROTATEMIN

#endif
			memset(&CurrHealth, 0x00, sizeof(HEALTH_MONITORS_F));

			//REset Forced eco Count
			SystemState->Forced_TryCnt = 0;
			//Save

		} else {

			if (SystemState->HealthScale[0] >= 150) {
				SystemState->RunningHTotal = 0;
				SystemState->DaysRun = 0;
				SystemState->HealthScale[0] = 25;
			}

			CurrReading.Comp = SystemState->Current.Compressor.Freq;

			CurrReading.Current = (SystemState->ExternalCurrent1 / 1000.0);

			if (SystemState->Measured.F8 > 2800)
				CurrReading.F8 = SystemState->Measured.F8 - 2800;
			else
				CurrReading.F8 = 0;

			if (SystemState->Measured.Tempratures.OutsideAir > 2500)
				CurrReading.OA = SystemState->Measured.Tempratures.OutsideAir - 2500;
			else
				CurrReading.OA = 0;

			CurrReading.O_Fan = SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed;

			CurrReading.SA_Fan = SystemState->Current.Fans[FAN_PORT_INDOOR].Speed;

			//Continue to intigrate
			if (First == false) {
				CurrHealth.Comp += ((CurrReading.Comp + PrevReading.Comp) / 200.0) / (24.0 * 60.0);
				CurrHealth.Current += ((CurrReading.Current + PrevReading.Current) / 200.0) / (24.0 * 60.0);
				CurrHealth.F8 += ((CurrReading.F8 + PrevReading.F8) / 200.0) / (24.0 * 60.0);
				CurrHealth.OA += ((CurrReading.OA + PrevReading.OA) / 200.0) / (24.0 * 60.0);
				CurrHealth.O_Fan += ((CurrReading.O_Fan + PrevReading.O_Fan) / 200.0) / (24.0 * 60.0);
				CurrHealth.SA_Fan += ((CurrReading.SA_Fan + PrevReading.SA_Fan) / 200.0) / (24.0 * 60.0);
			} else {
				CurrHealth.Comp = (ScaleToPercent(CurrReading.Comp) / 100.0) / (24.0 * 60.0);
				CurrHealth.Current = ((CurrReading.Current) / 100.0) / (24.0 * 60.0);
				CurrHealth.F8 = ((CurrReading.F8) / 100.) / (24.0 * 60.0);
				CurrHealth.OA = ((CurrReading.OA) / 100.0) / (24.0 * 60.0);
				CurrHealth.O_Fan = ((CurrReading.O_Fan) / 100.0) / (24.0 * 60.0);
				CurrHealth.SA_Fan = ((CurrReading.SA_Fan) / 100.0) / (24.0 * 60.0);
				First = false;
			}

			PrevReading.Comp = CurrReading.Comp;
			PrevReading.Current = CurrReading.Current;
			PrevReading.F8 = CurrReading.F8;
			PrevReading.OA = CurrReading.OA;
			PrevReading.O_Fan = CurrReading.O_Fan;
			PrevReading.SA_Fan = CurrReading.SA_Fan;

			SystemState->DailyHealth[0].Comp = CurrHealth.Comp * 100.0;
			SystemState->DailyHealth[0].Current = CurrHealth.Current * 100.0;
			SystemState->DailyHealth[0].F8 = CurrHealth.F8 * 100.0;
			SystemState->DailyHealth[0].OA = CurrHealth.OA * 100.0;
			SystemState->DailyHealth[0].O_Fan = CurrHealth.O_Fan * 100.0;
			SystemState->DailyHealth[0].SA_Fan = CurrHealth.SA_Fan * 100.0;

#ifdef ROTATEMIN

#endif
		}


	}// Only every min
	PrevMin = SystemState->sTime.Minutes;


	SystemState->Spare16++;

	HAL_RTC_GetTime(hrtc, &SystemState->sTime, FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &SystemState->sDate, FORMAT_BIN);

	if (SystemState->SetDateTime) {
		HAL_RTC_SetTime(hrtc, &SystemState->sTime, FORMAT_BIN);
		HAL_RTC_SetDate(hrtc, &SystemState->sDate, FORMAT_BIN);
		printf("Time set %d:%d:%d\r\n", SystemState->sTime.Hours, SystemState->sTime.Minutes, SystemState->sTime.Seconds);
		SystemState->SetDateTime = false;
	}

	uint8_t  nxt_sec  =  (SystemState->sTime.Seconds+(2-(SystemState->sTime.Seconds%2)));
	if(nxt_sec ==60)
		nxt_sec =0;
	sAlarm_A.AlarmTime.Seconds = nxt_sec;


	HAL_RTC_SetAlarm_IT(hrtc, &sAlarm_A, FORMAT_BIN);

}

void StartHealthTask(void *argument) {

	//RTC_TimeTypeDef sTime;
	//RTC_DateTypeDef sDate;
	uint8_t ttt;

	sAlarm_A.AlarmTime.Hours = 0;
	sAlarm_A.AlarmTime.Minutes = 0;
	sAlarm_A.AlarmTime.Seconds = 0;
	sAlarm_A.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
	sAlarm_A.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sAlarm_A.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sAlarm_A.AlarmMask =  RTC_ALARMMASK_MINUTES|RTC_ALARMMASK_HOURS|RTC_ALARMMASK_DATEWEEKDAY;
	sAlarm_A.AlarmSubSecondMask = 0;
	sAlarm_A.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	sAlarm_A.AlarmDateWeekDay = 1;
	sAlarm_A.Alarm = RTC_ALARM_A;

	HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm_A, FORMAT_BIN);

	SystemState->sTime.Hours = 23;
	SystemState->sTime.Minutes = 50;
	SystemState->sTime.Seconds = 00;
	/*
	 SystemState->sTime.Hours = 01;
	 SystemState->sTime.Minutes = 01;
	 SystemState->sTime.Seconds = 01;
	 */
	SystemState->sTime.SecondFraction = 0;
	SystemState->sTime.TimeFormat = RTC_HOURFORMAT_24;
	SystemState->sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;

	SystemState->sDate.Date = 0;
	SystemState->sDate.Month = 0;
	SystemState->sDate.Year = 20;
	SystemState->sDate.WeekDay = 0;

	HAL_RTC_SetTime(&hrtc, &SystemState->sTime, FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &SystemState->sDate, FORMAT_BIN);

	SystemState->RunningHTotal = 0;
	SystemState->DaysRun = 0;
	SystemState->HealthScale[0] = 25;
	SystemState->HealthScale[1] = 15;
	SystemState->HealthScale[2] = 10;
	SystemState->HealthScale[3] = 10;
	SystemState->HealthScale[4] = 10;
	SystemState->HealthScale[5] = 30;

	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_MODBUS_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Health, 10000);
			continue;
		}

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Health, 1000 * 10);
		//HAL_RTC_GetTime(&hrtc, &SystemState->sTime, FORMAT_BIN);
		//HAL_RTC_GetDate(&hrtc, &SystemState->sDate, FORMAT_BIN);
	}
}
