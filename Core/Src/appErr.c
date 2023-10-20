/*
 * appErr.c
 *
 *  Created on: Jan 10, 2021
 *      Author: Richard
 */

#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"
#include "Modbus.h"

extern APP_SYSTEM_DATA *SystemState;

#define	LED_A	10
#define	LED_b	11
#define	LED_c	12
#define	LED_d	13
#define	LED_E	14
#define	LED_F	15
#define	LED_G	16
#define LED_P	17
#define LED_L	18

uint8_t LetterCodes[] = { LED_E, LED_F, LED_P, LED_L, LED_c };

extern osThreadId_t SystemTaskHandle;
extern osThreadId_t AdcTaskHandle;
extern osThreadId_t OutdoorComsTaskHandle;
extern osThreadId_t ModbusTaskHandle;
extern osThreadId_t DisplayTaskHandle;
extern osThreadId_t FanTaskHandle;
extern osThreadId_t DamperTaskHandle;
extern osThreadId_t ErrCheckTaskHandle;
extern osThreadId_t HealthTaskHandle;
extern osThreadId_t GPSTaskHandle;
extern osThreadId_t EtherNetHandle;

extern modbusHandler_t ModbusH_1;
extern modbusHandler_t ModbusH_3;

extern struct netif gnetif;
char LogMsg[1024];

void DisplyLog() {
	int pos = 0;
	pos += sprintf(LogMsg, "\r\n*********%d:%d:%d********\r\n", SystemState->sTime.Hours, SystemState->sTime.Minutes, SystemState->sTime.Seconds);
	pos += sprintf(&LogMsg[pos], "Stats: ");
	pos += sprintf(&LogMsg[pos], "%lu:%lu  %lu:%lu\r\n", SystemState->Stats.Powered / 60, SystemState->Stats.Powered % 60, SystemState->Stats._TotalMin / 60,
			SystemState->Stats._TotalMin % 60);
	pos += sprintf(&LogMsg[pos], "Comp:%lu Free:%lu Super:%lu Force:%lu\r\n", SystemState->Stats._TotalMin_Comp, SystemState->Stats._TotalMin_FreeAir,
			SystemState->Stats._TotalMin_SuperEco, SystemState->Stats._TotalMin_ForcedEco);
	pos += sprintf(&LogMsg[pos], "GPS: ");
	pos += sprintf(&LogMsg[pos], "%ld,%ld,%lu,%lu\r\n", SystemState->Lat, SystemState->Lon, SystemState->Alt, SystemState->SV);
	printf(LogMsg);

	pos = 0;
	pos += sprintf(LogMsg, "Info:\tReq\tCur\r\n");
	pos += sprintf(&LogMsg[pos], "Mode:\t%d\t%d\r\n", SystemState->Required.CoolingMode, SystemState->Current.CoolingMode);
	pos += sprintf(&LogMsg[pos], "Pwr :\t%d\t%d\r\n", SystemState->Required.PowerMode, SystemState->Current.PowerMode);
	pos += sprintf(&LogMsg[pos], "Comp:\t%d\t%d\r\n", SystemState->Required.Compressor.Freq, SystemState->Current.Compressor.Freq);
	pos += sprintf(&LogMsg[pos], "FanO:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed, SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed);
	pos += sprintf(&LogMsg[pos], "FanI:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_INDOOR].Speed, SystemState->Current.Fans[FAN_PORT_INDOOR].Speed);
	printf(LogMsg);

	pos = 0;
	pos += sprintf(LogMsg, "Errors\r\n");
	pos += sprintf(&LogMsg[pos], "ERR_L:\t%x\r\n", SystemState->L_Errors);
	pos += sprintf(&LogMsg[pos], "ERR_P:\t%x\r\n", SystemState->P_Errors);
	pos += sprintf(&LogMsg[pos], "ERR_E:\t%x\r\n", SystemState->E_Errors);
	pos += sprintf(&LogMsg[pos], "ERR_F:\t%x\r\n", SystemState->F_Errors);

	pos = 0;
	pos += sprintf(&LogMsg[pos], "MB3 :\t%d\t%d\t%d\r\n", ModbusH_3.u16InCnt, ModbusH_3.u16OutCnt, ModbusH_3.u16errCnt);
	pos += sprintf(&LogMsg[pos], "MB1 :\t%d\t%d\t%d\r\n", ModbusH_1.u16InCnt, ModbusH_1.u16OutCnt, ModbusH_1.u16errCnt);
	pos += sprintf(&LogMsg[pos], "Dual:\t%d\t%d\t%d\r\n", SystemState->Paramters.PARAM[PARAM_HASSLAVE], SystemState->MasterState,
			SystemState->Roatte_RemainTime);
	printf(LogMsg);

}

void StartErrCheckTask(void *argument) {
	const portTickType xFrequency = 1000;
	uint8_t Letter = 0;
	uint8_t Num = 0;
	uint16_t tmpERR;
	uint8_t n, l;
#define HEAPCHECK (120)
	uint8_t HeapCheck = HEAPCHECK - 1;

	portTickType xLackError;

	portTickType xLackBootAttempt;

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Err, 60000);

	uint8_t PreviousOutdoorFaultCnt = 0;
	/* Infinite loop */
	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_ERR_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Err, 10000);
			continue;
		}

		HAL_GPIO_WritePin(Y0_A_GPIO_Port, Y0_A_Pin, CHECKBIT(SystemState->Required.Realys, 0));
		HAL_GPIO_WritePin(Y1_A_GPIO_Port, Y1_A_Pin, CHECKBIT(SystemState->Required.Realys, 1));
		HAL_GPIO_WritePin(Y2_A_GPIO_Port, Y2_A_Pin, CHECKBIT(SystemState->Required.Realys, 2));
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, CHECKBIT(SystemState->Required.Realys, 3));

		SystemState->Current.Realys = SystemState->Required.Realys;

		//Clear all Critical
		SystemState->C_Errors = 0;

		//Make a List of all the ERRORS
		//Check Temp Sensors

//		if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_SIM)) {
		if (SystemState->Measured.Tempratures.ReturnAir < -5000) {
			if (SystemState->Paramters.PARAM[PARAM_SENSOR] == 0) {
				SystemState->C_Errors = 1;
			}
			ADDFLAG(SystemState->L_Errors, L_ERR_INDOOR_RUTURN_AIR);
		} else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_RUTURN_AIR);

		if (SystemState->Measured.Tempratures.CompDischarge < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_DISCHARGE);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_DISCHARGE);

		if (SystemState->Measured.Tempratures.IndoorCoil < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_INDOOR_COIL);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_COIL);

		if (SystemState->Measured.Tempratures.OutdoorCoil < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_COIL);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_COIL);

		if (SystemState->Measured.Tempratures.OutsideAir < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_AMBIENT);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_AMBIENT);

		if (SystemState->Paramters.PARAM[PARAM_SENSOR] != 0) {
			if (SystemState->Measured.Tempratures.Room < -5000)
				ADDFLAG(SystemState->L_Errors, L_ERR_INDOOR_ROOM);
			else
				REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_ROOM);
		} else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_ROOM);

		if (SystemState->Measured.Tempratures.Suction < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_SUCTION);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_SUCTION);

		if (SystemState->Measured.Tempratures.SupplyAir < -5000)
			ADDFLAG(SystemState->L_Errors, L_ERR_INDOOR_SUPPLY_AIR);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_SUPPLY_AIR);

		if ((SystemState->Measured.Tempratures.Room < -5000) && (SystemState->Measured.Tempratures.ReturnAir < -5000))
			ADDFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS);
		else
			REMOVEFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS);

		//} //Only check sensors if not in sim

		if (SystemState->Measured.F8 / 100.0 > (SystemState->Paramters.PARAM[PARAM_HIGH_TEMP])) {
			SystemState->HighTempAlarm = 1;
			SystemState->Measured.Fans[FAN_PORT_SPARE].Speed = 1;
		} else if (SystemState->Measured.F8 / 100.0 > (SystemState->Paramters.PARAM[PARAM_HIGH_TEMP] - 1)) {
			//If we have a Salve ASk for help
			if ((SystemState->Paramters.PARAM[PARAM_HASSLAVE]) && (SystemState->PoweredUp)) {
				//Check if we have a Swapable event
				SystemState->RotateReason = ROTATE_REASON_HIGHTEMP;
				if (SystemState->MasterState != MASTER_WARMUP_SLAVE) //Avoid constanlty asking
						{
					SystemState->MasterState = MASTER_BOOT_SLAVE;
				}
			}
		} else {
			SystemState->HighTempAlarm = 0;
			SystemState->Measured.Fans[FAN_PORT_SPARE].Speed = 0;
		}

		//Filter
		if (HAL_GPIO_ReadPin(X0_GPIO_Port, X0_Pin) == 0) {
			ADDFLAG(SystemState->F_Errors, F_ERR_FILTER_DIRTY);
		} else
			REMOVEFLAG(SystemState->F_Errors, F_ERR_FILTER_DIRTY);

		if (SystemState->E_Errors != E_ERR_OK)
			ADDFLAG(SystemState->Errors, ERR_CODES_E);
		else
			REMOVEFLAG(SystemState->Errors, ERR_CODES_E);

		if (SystemState->F_Errors != F_ERR_OK)
			ADDFLAG(SystemState->Errors, ERR_CODES_F);
		else
			REMOVEFLAG(SystemState->Errors, ERR_CODES_F);

		if (SystemState->P_Errors != P_ERR_OK)
			ADDFLAG(SystemState->Errors, ERR_CODES_P);
		else
			REMOVEFLAG(SystemState->Errors, ERR_CODES_P);

		if (SystemState->L_Errors != L_ERR_OK)
			ADDFLAG(SystemState->Errors, ERR_CODES_L);
		else
			REMOVEFLAG(SystemState->Errors, ERR_CODES_L);

		if (SystemState->C_Errors != C_ERR_OK)
			ADDFLAG(SystemState->Errors, ERR_CODES_C);
		else
			REMOVEFLAG(SystemState->Errors, ERR_CODES_C);

		//Check for no gass Error
		if (SystemState->Paramters.PARAM[PARAM_USE_PID] < 5) {
			if ((SystemState->Measured.Outdoor.CompressorRunTime > 240) && (SystemState->Current.CoolingMode != MODE_FORCED_ECO)
					&& (SystemState->Current.Compressor.Freq > 25)) // Seconds
					{
				float Thresh = (SystemState->Current.Compressor.Freq * -0.1452) + 27.613; // 25-16

				if ((SystemState->Measured.Tempratures.Suction / 100.0 > Thresh) || (SystemState->Measured.Tempratures.CompDischarge / 100.0 < 25)) {
					if (SystemState->Measured.Outdoor.faultFlag101T != 0x00)
						xLackError = xTaskGetTickCount();

					if (CHECKFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY)) {

					} else if (xTaskGetTickCount() - xLackError > MINUTES(10)) {
						ADDFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY);
						xLackError = xTaskGetTickCount();
					}
				} else {
					REMOVEFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY);
					xLackError = xTaskGetTickCount();
				}
			} else {
				REMOVEFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY);
				xLackError = xTaskGetTickCount();
			}
		} else {
			REMOVEFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY);
			xLackError = xTaskGetTickCount();
		}

		//These go straight to critical and Try start other unit if need be
		if (CHECKFLAG(SystemState->E_Errors, E_ERR_EEPROM_INDOOR)) {
			SystemState->C_Errors = 1;
		}
		if (CHECKFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR)) {
			SystemState->C_Errors = 1;
		}
		if (CHECKFLAG(SystemState->E_Errors, E_ERR_INDOOR_FAN)) {
			SystemState->C_Errors = 1;
		}
		if (SystemState->UnitType != UNIT_SLIM) {
			if (CHECKFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN)) {
				SystemState->C_Errors = 1;
			}
		}

		//Check Power States
		SystemState->PowerCount = 0;
		if (SystemState->Measured.Outdoor.vdFilt101T / 100.0 < 100)
			SystemState->PowerCount = 1;

		if (SystemState->Measured.BatteryVoltage / 100.0 < 10)
			SystemState->PowerCount = 2;

		if ((SystemState->CompressorSize >= COMPTYPE_7KW_220ONLY) && (SystemState->Current.PowerMode == MODE_PWRSAVE)
				&& (SystemState->Current.CoolingMode == MODE_COOLING))
			SystemState->PowerCount = 3;

		//these Are Slightly more complex Type errors
		if (CHECKFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY)) {
			SystemState->C_Errors = 2;
		}

		//Now need to Check all the Errors that cause the 5 min retry
		if (CHECKFLAG(SystemState->F_Errors, F_ERR_STEP_MISSING)||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_CURRENT)
		||CHECKFLAG(SystemState->P_Errors, P_ERR_PHASE_CURRENT)||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_DC_VOLTAGE)
		||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT)) {
			SystemState->C_Errors = 2;
		}

		//these are errors that when set also need to have protection level STOP set
		if (SystemState->Measured.Outdoor.ProctecionLevel >= COMP_STOP) {
			if (CHECKFLAG(SystemState->P_Errors,
					P_ERR_OUTDOOR_DISCHARGE_TEMP_LIMIT) ||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_COIL_TEMP_LIMIT)||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_INDOOR_COIL_LIMIT)
					||CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AMBIENT_LIMIT)) {
				SystemState->C_Errors = 2;
			}
		}

		if ((SystemState->Paramters.PARAM[PARAM_HASSLAVE]>=1) && (SystemState->PoweredUp)) {
			//Check if we have a Swapable event
			if (SystemState->C_Errors != 0) {
				if ((SystemState->MasterState != MASTER_WARMUP_SLAVE)) //Avoid constanlty asking
				{
					SystemState->MasterState = MASTER_BOOT_SLAVE;
					xLackBootAttempt = xTaskGetTickCount();
				}
			}
			else
			{
				//No Errors Might as well Double check if two units on
				if ((SystemState->OtherGuyMem.PoweredUp == 1)) {
					SystemState->MasterState = MASTER_STOP_SLAVE;
				}
			}
		} else {
			if (SystemState->Paramters.PARAM[PARAM_REALY1_USE] == REALY_FAN) {
				if (SystemState->C_Errors != 0)
					SETBIT(SystemState->Required.Realys, 2);
				else
					CLEARBIT(SystemState->Required.Realys, 2);
			}
		}

		if (HeapCheck % 5 == 0) {
			//Set up Display
			if (SystemState->Errors != ERR_CODES_OK) {
				//We have a Error select the nect letter and Num
				HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, 0);
				if (Num == 0) {
					for (l = Letter; l < 4; l++) {
						if ((CHECKFLAG(SystemState->Errors, ERR_CODES_E)) && (l == 0)) {
							tmpERR = SystemState->E_Errors;
							SystemState->ErrroCode = LetterCodes[l];
							Letter = l + 1;
							break;
						}
						if ((CHECKFLAG(SystemState->Errors, ERR_CODES_F)) && (l == 1)) {
							tmpERR = SystemState->F_Errors;
							SystemState->ErrroCode = LetterCodes[l];
							Letter = l + 1;
							break;
						}
						if ((CHECKFLAG(SystemState->Errors, ERR_CODES_P)) && (l == 2)) {
							tmpERR = SystemState->P_Errors;
							SystemState->ErrroCode = LetterCodes[l];
							Letter = l + 1;
							break;
						}
						if ((CHECKFLAG(SystemState->Errors, ERR_CODES_L)) && (l == 3)) {
							tmpERR = SystemState->L_Errors;
							SystemState->ErrroCode = LetterCodes[l];
							Letter = l + 1;
							break;
						}
					}
					if (l == 4) {
						Letter = 0;
						Num = 16;
					}
				}

				for (n = Num; n < 16; n++) {
					if (CHECKFLAG(tmpERR,BIT(n)) == 1) {
						SystemState->ErrroNum = n;
						Num = n + 1;
						break;
					}
				}
				if (n == 16)
					Num = 0;
			} else
				HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, 1);
		}

		if (HeapCheck > HEAPCHECK) {
			HeapCheck = 0;

			DisplyLog();

			UBaseType_t uxHighWaterMark[11];
			uxHighWaterMark[0] = uxTaskGetStackHighWaterMark(SystemTaskHandle);
			if (uxHighWaterMark[0] <= 0)
				printf("SystemTask Stack OverFlow");

			uxHighWaterMark[1] = uxTaskGetStackHighWaterMark(AdcTaskHandle);
			if (uxHighWaterMark[1] <= 0)
				printf("ADC Stack OverFlow");

			uxHighWaterMark[2] = uxTaskGetStackHighWaterMark(OutdoorComsTaskHandle);
			if (uxHighWaterMark[2] <= 0)
				printf("Outdoor Stack OverFlow");
			uxHighWaterMark[3] = uxTaskGetStackHighWaterMark(ModbusTaskHandle);
			if (uxHighWaterMark[3] <= 0)
				printf("Modbus Stack OverFlow");
			uxHighWaterMark[4] = uxTaskGetStackHighWaterMark(DisplayTaskHandle);
			if (uxHighWaterMark[4] <= 0)
				printf("Display Stack OverFlow");
			uxHighWaterMark[5] = uxTaskGetStackHighWaterMark(FanTaskHandle);
			if (uxHighWaterMark[5] <= 0)
				printf("Fan Stack OverFlow");
			uxHighWaterMark[6] = uxTaskGetStackHighWaterMark(DamperTaskHandle);
			if (uxHighWaterMark[6] <= 0)
				printf("Dampper Stack OverFlow");
			uxHighWaterMark[7] = uxTaskGetStackHighWaterMark(ErrCheckTaskHandle);
			if (uxHighWaterMark[7] <= 0)
				printf("Err Stack OverFlow");
			uxHighWaterMark[8] = uxTaskGetStackHighWaterMark(HealthTaskHandle);
			if (uxHighWaterMark[8] <= 0)
				printf("Health Stack OverFlow");
			uxHighWaterMark[8] = uxTaskGetStackHighWaterMark(GPSTaskHandle);
			if (uxHighWaterMark[8] <= 0)
				printf("GPS Stack OverFlow");
			uxHighWaterMark[9] = uxTaskGetStackHighWaterMark(EtherNetHandle);
			if (uxHighWaterMark[9] <= 0)
				printf("Ethernet Stack OverFlow");

			uxHighWaterMark[10] = xPortGetFreeHeapSize();

			printf("Highwater:");
			for (int y = 0; y < 11; y++)
				printf("%lu,", uxHighWaterMark[y]);

			printf("\r\n");

			printf("-------------------------\r\n");

		} else
			HeapCheck++;

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Err, xFrequency);
		if (SystemState->Errors != ERR_CODES_OK) {
			//HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Err, 50);
			//HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
		}
	}

}
