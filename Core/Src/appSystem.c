/* appSystem.c
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
#include "w25x40Rw.h"
#include <memory.h>
#include "pid.h"
#include "MY_FLASH.h"

extern APP_SYSTEM_DATA *SystemState;
extern uint8_t ScaleToPercent(uint8_t Hz);
extern uint8_t ScaleToHz(float Percent);

pid_f32_t comp_pid_data;
pid_f32_t safe_pid_data;
pid_f32_t fan_pid_data;

const uint8_t limit_para_tab[16][4] = { //Params
		{ 18, 70, 75 }, //Compressor MAX
				{ 12, 28, 40 }, //Cooling cycle set point 0x01
				{ 12, 31, 40 }, //Power Save Set Point 0x02
				{ 1, 90, 100 }, //SA fan Maximum speed /Cooling Mode 0x03
				{ 0, 35, 100 }, //SA fan Minimum speed 0x04
				{ 0, 90, 100 }, //SA Fan Minimum Air speed cooling cycle 0x05
				{ 0, 2, 2 }, //Select control sensor 0x06
				{ 0, 0, 2 }, //Have SLave 0x07
				{ 0, 1, 99 }, //MAX_CHange DELTA 0x08
				{ 0, 28, 40 }, //0x09 //PARAM_SAFE_SETPOINT
				{ 0, REALY_FAN, 2 }, //0x0A Relay
				{ 0, 0, 99 }, //Eco Delta
				{ 0, 35, 99 }, //HIGH TEMP
				{ 0, 224, 255 }, //Rotate Time 0x0D
				{ 0, 3, 10 }, //USE PID 0x0D
				{ 0, 100, 100 } }; //MaxFan Eco

const uint8_t DefaultVendor = VENDOR_VODACOM;
const uint8_t DefaultUnit = UNIT_CUBE;
const uint8_t DefaultComp = COMPTYPE_12KW;

//uint8_t Cooling_Fresh;
#define EEPROM2NDSIZE (64)

uint8_t EEpromParam_Buf[EEPROM2NDSIZE];
uint8_t EEpromParam1_Buf[EEPROM2NDSIZE];

const uint8_t Comp_Params[6][2] = { { 30, 70 }, { 18, 70 }, { 18, 70 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

portTickType TestingTime;

uint16_t LoadParamFromEEprom(uint16_t Add) {

	memset(EEpromParam_Buf, 0x00, 16);
	IO_Read_nBytes(Add, (uint8_t*) EEpromParam_Buf, 16);
	for (int i = 0; i < 16; i++) {
		SystemState->Paramters.PARAM[i] = EEpromParam_Buf[i];
	}

	memset(EEpromParam1_Buf, 0x00, 16);
	IO_Read_nBytes(Add + (16 * 3), (uint8_t*) EEpromParam1_Buf, 32);
	SystemState->Vendor = EEpromParam1_Buf[2];
	SystemState->UnitType = EEpromParam1_Buf[3];
	SystemState->CompressorSize = EEpromParam1_Buf[4];

	if ((EEpromParam1_Buf[6] != 255) && (EEpromParam1_Buf[5] != 255) && (EEpromParam1_Buf[10] != 255)) {
		SystemState->DaysRun = EEpromParam1_Buf[5] | EEpromParam1_Buf[6] << 8;
		SystemState->RunningHTotal = EEpromParam1_Buf[7] | (EEpromParam1_Buf[8] << 8) | (EEpromParam1_Buf[9] << 16) | (EEpromParam1_Buf[10] << 24);
	} else {
		SystemState->DaysRun = 0;
		SystemState->RunningHTotal = 0;
	}

	SystemState->IP12 = (EEpromParam1_Buf[11] << 8) | (EEpromParam1_Buf[12]);
	SystemState->IP34 = (EEpromParam1_Buf[13] << 8) | (EEpromParam1_Buf[14]);

	//SystemState->IP12 =0x00;
	//SystemState->IP34 =0x00;

	SystemState->NM12 = (EEpromParam1_Buf[15] << 8) | (EEpromParam1_Buf[16]);
	SystemState->NM34 = (EEpromParam1_Buf[17] << 8) | (EEpromParam1_Buf[18]);

	SystemState->GW12 = (EEpromParam1_Buf[19] << 8) | (EEpromParam1_Buf[20]);
	SystemState->GW34 = (EEpromParam1_Buf[21] << 8) | (EEpromParam1_Buf[22]);
	SystemState->ServerPort = (EEpromParam1_Buf[23] << 8) | (EEpromParam1_Buf[24]);

	if (((SystemState->IP12 == 0xffff) && (SystemState->IP34 == 0xffff)) || (SystemState->ServerPort == 0xFFFF)) {
		SystemState->IP12 = 0xc0a8;
		SystemState->IP34 = 0x00e6;

		SystemState->GW12 = 0xc0a8;
		SystemState->GW34 = 0x0001;

		SystemState->NM12 = 0xFFFF;
		SystemState->NM34 = 0xFF00;
		SystemState->ServerIP.addr = 0;
		SystemState->ServerPort = 0;
	}
	return ((EEpromParam1_Buf[0] << 8) | EEpromParam1_Buf[1]);

}

#define GETTOPBYTE(A) ((A>>8)&0x00FF)
#define GETBOTBYTE(A) ((A)&0x00FF)

uint16_t LoadInternalEEProm() {
	memset(EEpromParam_Buf, 0x00, 16);
	MY_FLASH_ReadN(0x00, (uint8_t*) EEpromParam_Buf, 16, DATA_TYPE_8);
	for (int i = 0; i < 16; i++) {
		SystemState->Paramters.PARAM[i] = EEpromParam_Buf[i];
	}

	memset(EEpromParam1_Buf, 0x00, 16);
	//IO_Read_nBytes(Add + (16 * 3), (uint8_t*) EEpromParam1_Buf, 32);
	MY_FLASH_ReadN(0x00 + (16 * 3), (uint8_t*) EEpromParam1_Buf, EEPROM2NDSIZE, DATA_TYPE_8);
	SystemState->Vendor = EEpromParam1_Buf[2];
	SystemState->UnitType = EEpromParam1_Buf[3];
	SystemState->CompressorSize = EEpromParam1_Buf[4];

	if ((EEpromParam1_Buf[6] != 255) && (EEpromParam1_Buf[5] != 255) && (EEpromParam1_Buf[10] != 255)) {
		SystemState->DaysRun = EEpromParam1_Buf[5] | EEpromParam1_Buf[6] << 8;
		SystemState->RunningHTotal = EEpromParam1_Buf[7] | (EEpromParam1_Buf[8] << 8) | (EEpromParam1_Buf[9] << 16) | (EEpromParam1_Buf[10] << 24);
	} else {
		SystemState->DaysRun = 0;
		SystemState->RunningHTotal = 0;
	}

	SystemState->IP12 = (uint16_t) (EEpromParam1_Buf[11] << 8) | (uint16_t) (EEpromParam1_Buf[12]);
	SystemState->IP34 = (uint16_t) (EEpromParam1_Buf[13] << 8) | (uint16_t) (EEpromParam1_Buf[14]);

	//SystemState->IP12 =0x00;
	//SystemState->IP34 =0x00;

	SystemState->NM12 = (uint16_t) (EEpromParam1_Buf[15] << 8) | (uint16_t) (EEpromParam1_Buf[16]);
	SystemState->NM34 = (uint16_t) (EEpromParam1_Buf[17] << 8) | (uint16_t) (EEpromParam1_Buf[18]);

	SystemState->GW12 = (uint16_t) (EEpromParam1_Buf[19] << 8) | (uint16_t) (EEpromParam1_Buf[20]);
	SystemState->GW34 = (uint16_t) (EEpromParam1_Buf[21] << 8) | (uint16_t) (EEpromParam1_Buf[22]);
	SystemState->ServerPort = (uint16_t) (EEpromParam1_Buf[23] << 8) | (uint16_t) (EEpromParam1_Buf[24]);

	printf("Loaded IP %d.%d.%d.%d\r\n", GETTOPBYTE(SystemState->IP12), GETBOTBYTE(SystemState->IP12), GETTOPBYTE(SystemState->IP34),
			GETBOTBYTE(SystemState->IP34));
	if (((SystemState->IP12 == 0xffff) && (SystemState->IP34 == 0xffff)) || (SystemState->ServerPort == 0xFFFF)) {
		SystemState->IP12 = 0xc0a8;
		SystemState->IP34 = 0x00e6;

		SystemState->GW12 = 0xc0a8;
		SystemState->GW34 = 0x0001;

		SystemState->NM12 = 0xFFFF;
		SystemState->NM34 = 0xFF00;
		SystemState->ServerIP.addr = 0;
		SystemState->ServerPort = 0;
	}

	SystemState->SerialNo = (uint64_t) EEpromParam1_Buf[25];
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[26] << 8;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[27] << 16;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[28] << 24;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[29] << 32;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[30] << 40;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[31] << 48;
	SystemState->SerialNo |= (uint64_t) EEpromParam1_Buf[32] << 56;

	uint8_t TT[sizeof(_AllTimeStats)];
	MY_FLASH_ReadN(0x1001, (uint8_t*) &TT[0], sizeof(_AllTimeStats), DATA_TYPE_8);
	memcpy(&SystemState->Stats, TT, sizeof(_AllTimeStats));
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);

	printf("Final IP %d.%d.%d.%d\r\n", GETTOPBYTE(SystemState->IP12), GETBOTBYTE(SystemState->IP12), GETTOPBYTE(SystemState->IP34),
			GETBOTBYTE(SystemState->IP34));

	return ((EEpromParam1_Buf[0] << 8) | EEpromParam1_Buf[1]);
}

void SaveInternalEEProm() {

	HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);

	MY_FLASH_EraseSector();
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 200);

	for (int i = 0; i < 16; i++) {
		SystemState->Paramters.PARAM[i] = CLAMP(SystemState->Paramters.PARAM[i], limit_para_tab[i][PARAM_UPPER_LIMIT], limit_para_tab[i][PARAM_LOWWER_LIMIT]);
		if (i == 0)
			SystemState->Paramters.PARAM[i] = CLAMP(SystemState->Paramters.PARAM[i], limit_para_tab[i][PARAM_UPPER_LIMIT], COMP_MIN_FREQ);
		EEpromParam_Buf[i] = SystemState->Paramters.PARAM[i];
	}
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
	MY_FLASH_WriteN(0x00, (uint8_t*) EEpromParam_Buf, 16, DATA_TYPE_8);
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);
	memset(EEpromParam_Buf, 0x00, 16);
	MY_FLASH_ReadN(0x00, (uint8_t*) EEpromParam_Buf, 16, DATA_TYPE_8);
	REMOVEFLAG(SystemState->E_Errors, E_ERR_EEPROM_INDOOR);

	for (int i = 0; i < 16; i++) {
		if (EEpromParam_Buf[i] != SystemState->Paramters.PARAM[i])
			ADDFLAG(SystemState->E_Errors, E_ERR_EEPROM_INDOOR);
	}

	EEpromParam1_Buf[0] = SystemState->VERSION >> 8;
	EEpromParam1_Buf[1] = SystemState->VERSION;
	EEpromParam1_Buf[2] = SystemState->Vendor;
	EEpromParam1_Buf[3] = SystemState->UnitType;
	EEpromParam1_Buf[4] = SystemState->CompressorSize;
	EEpromParam1_Buf[5] = SystemState->DaysRun;
	EEpromParam1_Buf[6] = SystemState->DaysRun >> 8;
	EEpromParam1_Buf[7] = SystemState->RunningHTotal;
	EEpromParam1_Buf[8] = SystemState->RunningHTotal >> 8;
	EEpromParam1_Buf[9] = SystemState->RunningHTotal >> 16;
	EEpromParam1_Buf[10] = SystemState->RunningHTotal >> 24;

	EEpromParam1_Buf[11] = SystemState->IP12 >> 8;
	EEpromParam1_Buf[12] = SystemState->IP12;
	EEpromParam1_Buf[13] = SystemState->IP34 >> 8;
	EEpromParam1_Buf[14] = SystemState->IP34;

	printf("Loaded IP %d.%d.%d.%d\r\n", EEpromParam1_Buf[11], EEpromParam1_Buf[12], EEpromParam1_Buf[13], EEpromParam1_Buf[14]);

	EEpromParam1_Buf[15] = SystemState->NM12 >> 8;
	EEpromParam1_Buf[16] = SystemState->NM12;
	EEpromParam1_Buf[17] = SystemState->NM34 >> 8;
	EEpromParam1_Buf[18] = SystemState->NM34;

	EEpromParam1_Buf[19] = SystemState->GW12 >> 8;
	EEpromParam1_Buf[20] = SystemState->GW12;
	EEpromParam1_Buf[21] = SystemState->GW34 >> 8;
	EEpromParam1_Buf[22] = SystemState->GW34;

	EEpromParam1_Buf[23] = SystemState->ServerPort >> 8;
	EEpromParam1_Buf[24] = SystemState->ServerPort;

	EEpromParam1_Buf[25] = SystemState->SerialNo;
	EEpromParam1_Buf[26] = SystemState->SerialNo >> 8;
	EEpromParam1_Buf[27] = SystemState->SerialNo >> 16;
	EEpromParam1_Buf[28] = SystemState->SerialNo >> 24;
	EEpromParam1_Buf[29] = SystemState->SerialNo >> 32;
	EEpromParam1_Buf[30] = SystemState->SerialNo >> 40;
	EEpromParam1_Buf[31] = SystemState->SerialNo >> 48;
	EEpromParam1_Buf[32] = SystemState->SerialNo >> 56;

	MY_FLASH_WriteN(0x00 + (3 * 16), (uint8_t*) EEpromParam1_Buf, EEPROM2NDSIZE, DATA_TYPE_8);

	//Now for Stats
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);
	MY_FLASH_WriteN(0x1001, (uint8_t*) &SystemState->Stats, sizeof(_AllTimeStats), DATA_TYPE_8);
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);

	if (SystemState->Stats.Save != 1) {
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
	}
	HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
}

void SaveStatsToEEprom(uint16_t Address) {

	MY_FLASH_EraseSector();
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 200);
	MY_FLASH_WriteN(Address, (uint8_t*) &SystemState->Stats, sizeof(_AllTimeStats), DATA_TYPE_8);
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);

}

void LoadStatsFromEEprom(uint16_t Address) {
	uint8_t TT[sizeof(_AllTimeStats)];

	MY_FLASH_ReadN(Address, (uint8_t*) &TT[0], sizeof(_AllTimeStats), DATA_TYPE_8);
	memcpy(&SystemState->Stats, TT, sizeof(_AllTimeStats));
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);
}

/*
 void SaveParamToEEprom(uint16_t Address) {

 //TickType_t start = xTaskGetTickCount();
 //__disable_irq();
 //HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
 IO_Erase_Sector(Address);
 vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 200);

 for (int i = 0; i < 16; i++) {
 SystemState->Paramters.PARAM[i] = CLAMP(SystemState->Paramters.PARAM[i], limit_para_tab[i][PARAM_UPPER_LIMIT], limit_para_tab[i][PARAM_LOWWER_LIMIT]);
 if (i == 0)
 SystemState->Paramters.PARAM[i] = CLAMP(SystemState->Paramters.PARAM[i], limit_para_tab[i][PARAM_UPPER_LIMIT], COMP_MIN_FREQ);
 EEpromParam_Buf[i] = SystemState->Paramters.PARAM[i];
 }
 vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
 IO_Write_nBytes(Address, (uint8_t*) EEpromParam_Buf, 16);
 vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
 memset(EEpromParam_Buf, 0x00, 16);
 IO_Read_nBytes(Address, (uint8_t*) EEpromParam_Buf, 16);
 REMOVEFLAG(SystemState->E_Errors, E_ERR_EEPROM_INDOOR);

 for (int i = 0; i < 16; i++) {
 if (EEpromParam_Buf[i] != SystemState->Paramters.PARAM[i])
 ADDFLAG(SystemState->E_Errors, E_ERR_EEPROM_INDOOR);
 }

 EEpromParam1_Buf[0] = SystemState->VERSION >> 8;
 EEpromParam1_Buf[1] = SystemState->VERSION;
 EEpromParam1_Buf[2] = SystemState->Vendor;
 EEpromParam1_Buf[3] = SystemState->UnitType;
 EEpromParam1_Buf[4] = SystemState->CompressorSize;
 EEpromParam1_Buf[5] = SystemState->DaysRun;
 EEpromParam1_Buf[6] = SystemState->DaysRun >> 8;
 EEpromParam1_Buf[7] = SystemState->RunningHTotal;
 EEpromParam1_Buf[8] = SystemState->RunningHTotal >> 8;
 EEpromParam1_Buf[9] = SystemState->RunningHTotal >> 16;
 EEpromParam1_Buf[10] = SystemState->RunningHTotal >> 24;

 EEpromParam1_Buf[11] = SystemState->IP12 >> 8;
 EEpromParam1_Buf[12] = SystemState->IP12;
 EEpromParam1_Buf[13] = SystemState->IP34 >> 8;
 EEpromParam1_Buf[14] = SystemState->IP34;

 EEpromParam1_Buf[15] = SystemState->NM12 >> 8;
 EEpromParam1_Buf[16] = SystemState->NM12;
 EEpromParam1_Buf[17] = SystemState->NM34 >> 8;
 EEpromParam1_Buf[18] = SystemState->NM34;

 EEpromParam1_Buf[19] = SystemState->GW12 >> 8;
 EEpromParam1_Buf[20] = SystemState->GW12;
 EEpromParam1_Buf[21] = SystemState->GW34 >> 8;
 EEpromParam1_Buf[22] = SystemState->GW34;

 EEpromParam1_Buf[23] = SystemState->ServerPort >> 8;
 EEpromParam1_Buf[24] = SystemState->ServerPort;

 IO_Write_nBytes(Address + (3 * 16), (uint8_t*) EEpromParam1_Buf, 32);

 HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
 vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 200);
 HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
 vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 200);
 HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
 //TickType_t end = xTaskGetTickCount();
 //__enable_irq();
 }
 */
void LoadHealth() {

}

void SaveHaelth() {

}

void CheckParams() {
	uint8_t ParamChanged = false;
	for (int i = 0; i < 16; i++) {
		if (EEpromParam_Buf[i] != SystemState->Paramters.PARAM[i]) {
			ParamChanged = true;
		}
	}

	ParamChanged = (EEpromParam1_Buf[2] == SystemState->Vendor) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[3] == SystemState->UnitType) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[4] == SystemState->CompressorSize) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[5] == SystemState->DaysRun) ? ParamChanged : true;

	ParamChanged = (EEpromParam1_Buf[11] == SystemState->IP12 >> 8) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[12] == (SystemState->IP12 & 0x00FF)) ? ParamChanged : true;

	ParamChanged = (EEpromParam1_Buf[13] == SystemState->IP34 >> 8) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[14] == (SystemState->IP34 & 0x00FF)) ? ParamChanged : true;

	ParamChanged = (EEpromParam1_Buf[23] == (SystemState->ServerPort >> 8)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[24] == (SystemState->ServerPort & 0x00FF)) ? ParamChanged : true;

	ParamChanged = (EEpromParam1_Buf[25] == ((SystemState->SerialNo) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[26] == ((SystemState->SerialNo >> 8) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[27] == ((SystemState->SerialNo >> 16) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[28] == ((SystemState->SerialNo >> 24) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[29] == ((SystemState->SerialNo >> 32) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[30] == ((SystemState->SerialNo >> 40) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[31] == ((SystemState->SerialNo >> 48) & 0x00FF)) ? ParamChanged : true;
	ParamChanged = (EEpromParam1_Buf[32] == ((SystemState->SerialNo >> 56) & 0x00FF)) ? ParamChanged : true;

	if (ParamChanged)
#ifdef EXTERNAL_EEPROM
		SaveParamToEEprom(0x000);
#else
		SaveInternalEEProm();
#endif
}

void CalcFanCompressorOff() {
	/*
	 uint16_t FanRange = 100 - SystemState->Paramters.PARAM[PARAM_FANMIN_ECO];
	 uint16_t TempSetPnt =
	 SystemState->Current.PowerMode == MODE_NORMAL ?
	 SystemState->Paramters.PARAM[PARAM_DELTA_ECO_SET] : SystemState->Paramters.PARAM[PARAM_DELTA_DAMPPER_BAND];

	 uint16_t TempRange = SystemState->Paramters.PARAM[PARAM_DELTA_DAMPPER_UPPER];

	 if (TempRange > 0) {
	 float fanP = (float) ((SystemState->Measured.F8 / 100.0) - (SystemState->Paramters.PARAM[PARAM_ECO_COOLING_CHANGEOVER] + TempSetPnt))
	 / (float) TempRange;
	 fanP = CLAMP(fanP, 1, 0);
	 SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = ((fanP * (float) FanRange) + SystemState->Paramters.PARAM[PARAM_FANMIN_ECO]) * 100.0;
	 }

	 if (SystemState->Current.CoolingMode == MODE_COOLING) {
	 if (SystemState->CompressorRunCnt == COMPRUN_FIRST) {
	 //First time in Cooling
	 SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
	 SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_CLOSE;
	 } else {
	 SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
	 SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
	 }
	 } else {
	 if (SystemState->Current.CoolingMode == MODE_SUPER_ECO) {
	 if (xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeDampperDrive > SECONDS(30)) {
	 SystemState->LastRunTime.xLastTimeDampperDrive = xTaskGetTickCount();

	 if (SystemState->Measured.F8 / 100.0 > TempSetPnt + 0.5) {	//To hot open dammper
	 SystemState->Required.Steppers[STEPPER_FRESH].Position = CLAMP(SystemState->Required.Steppers[STEPPER_FRESH].Position + 1, DAMPER_OPEN,
	 700);
	 } else if (SystemState->Measured.F8 / 100.0 < TempSetPnt - 0.5) {			//To hot close dampper
	 SystemState->Required.Steppers[STEPPER_FRESH].Position = CLAMP(SystemState->Required.Steppers[STEPPER_FRESH].Position - 1, DAMPER_OPEN,
	 700);
	 } else {
	 //Do nothing
	 }
	 SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
	 } else {
	 SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
	 SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_CLOSE;
	 }
	 }
	 }*/
}

void CalcFanCompressorOn() {

	//Cooling_Fresh = (OA > (dv - 2)) ? false : true;

	uint16_t FanRange = SystemState->Paramters.PARAM[PARAM_FANMAX_COOLING] - SystemState->Paramters.PARAM[PARAM_FANMIN_COOLING];

	float fanP = (ScaleToPercent(SystemState->Current.Compressor.Freq)) / 100.0;
	fanP = (fanP / 0.8);
	fanP = CLAMP(fanP, 1, 0);

	//Default when compressor is running
	SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
	SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
	SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = ((fanP * (float) FanRange) + SystemState->Paramters.PARAM[PARAM_FANMIN_COOLING]) * 100.0;

}

void CalcFanPID() {

	float dv =
			(SystemState->Current.PowerMode == MODE_NORMAL) ?
					(SystemState->Paramters.PARAM[PARAM_COOLING_SETPOINT]) : (SystemState->Paramters.PARAM[PARAM_PWRSAVE_SETPOINT]);

	//uint8_t Cooling_Fresh = (OA > (dv - 1)) ? false : true;

	uint16_t FanMin =
			((SystemState->Current.CoolingMode == MODE_COOLING)) ?
					SystemState->Paramters.PARAM[PARAM_FANMIN_COOLING] : SystemState->Paramters.PARAM[PARAM_FANMIN_ECO];

	uint16_t FanMax =
			((SystemState->Current.CoolingMode == MODE_COOLING)) ?
					SystemState->Paramters.PARAM[PARAM_FANMAX_COOLING] : SystemState->Paramters.PARAM[PARAM_FANMAX_ECO];

	uint16_t FanRange = FanMax - FanMin;

	fan_pid_data.p = (SystemState->P) / 100.0;
	fan_pid_data.i = SystemState->I / 100.0;
	fan_pid_data.d = SystemState->D / 100.0;

	float u = pid_f32_calc(&fan_pid_data, SystemState->Measured.F8 / 100.0, dv); //Output -50->50
	float fanP = (u + 50.0) / 100.0; //now scaled to 0-100%

	SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = ((fanP * (float) FanRange) + FanMin) * 100.0;

	if (SystemState->Required.Fans[FAN_PORT_INDOOR].Speed / 100.0 > SystemState->Paramters.PARAM[PARAM_FANMAX_ECO])
		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = SystemState->Paramters.PARAM[PARAM_FANMAX_ECO] * 100.0; //Just Clamp for saftey

}

void ControlDampper() {
	float OA = SystemState->Measured.Tempratures.OutsideAir / 100.0;
	float RA = SystemState->Measured.Tempratures.ReturnAir / 100.0;
	float SA = SystemState->Measured.Tempratures.SupplyAir / 100.0;

	if (OA - RA < -1) {
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_CLOSE;
		if (SystemState->Paramters.PARAM[PARAM_REALY1_USE] == REALY_DAMPPER) {
			SETBIT(SystemState->Required.Realys, 2);
		}
	} else {
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
		if (SystemState->Paramters.PARAM[PARAM_REALY1_USE] == REALY_DAMPPER) {
			CLEARBIT(SystemState->Required.Realys, 2);
		}

	}
}

void ControlSafe() {

}

void CalcCompPID() {
	float dv =
			(SystemState->Current.PowerMode == MODE_NORMAL) ?
					SystemState->Paramters.PARAM[PARAM_COOLING_SETPOINT] : SystemState->Paramters.PARAM[PARAM_PWRSAVE_SETPOINT];

	comp_pid_data.p = SystemState->P / 100.0;
	comp_pid_data.i = SystemState->I / 100.0;
	comp_pid_data.d = SystemState->D / 100.0;

	if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS)) {
		SystemState->Required.Compressor.Freq = ScaleToHz(0.5);
	} else {
		float u = pid_f32_calc(&comp_pid_data, SystemState->Measured.F8 / 100.0, dv); //Output -50->50
		float CompP = (u + 50.0) / 100.0; //now scaled to 0-100%
		SystemState->Required.Compressor.Freq = ScaleToHz(CompP);
		if (SystemState->Paramters.PARAM[PARAM_USE_PID] >= 2) {
			if (CompP == 0)
				SystemState->Required.Compressor.Freq = COMP_MIN_FREQ;
		}
	}
}

void CheckPower() {

	if (((SystemState->Measured.Outdoor.vdFilt101T / 100.0) > 100)|| (CHECKBIT(SystemState->Measured.Outdoor.gpioL101T, 4)))
		SystemState->Required.PowerMode = MODE_NORMAL;
	else
		SystemState->Required.PowerMode = MODE_PWRSAVE;

}

void CheckMode_PID() {
	if (SystemState->Current.CoolingMode != MODE_FULL)
		SystemState->Required.CoolingMode = SystemState->Current.CoolingMode;
	else
		SystemState->Required.CoolingMode = MODE_ECO;

	float F8 = 0;

	if (SystemState->Paramters.PARAM[PARAM_SENSOR] == 2) //Averrage
			{
		F8 = (SystemState->Measured.Tempratures.ReturnAir / 100.0 + SystemState->Measured.Tempratures.Room / 100.0) / 2.0;
		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_RUTURN_AIR))
			F8 = SystemState->Measured.Tempratures.Room / 100.0;
		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_ROOM))
			F8 = SystemState->Measured.Tempratures.ReturnAir / 100.0;
		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS))
			F8 = SystemState->Measured.Tempratures.SupplyAir / 100.0;

	} else if (SystemState->Paramters.PARAM[PARAM_SENSOR] == 0) //Return
			{
		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_RUTURN_AIR))
			F8 = SystemState->Measured.Tempratures.Room / 100.0;
		else
			F8 = SystemState->Measured.Tempratures.ReturnAir / 100.0;

		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS))
			F8 = SystemState->Measured.Tempratures.SupplyAir / 100.0;

	} else if (SystemState->Paramters.PARAM[PARAM_SENSOR] == 1) // Room
			{
		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_ROOM))
			F8 = SystemState->Measured.Tempratures.ReturnAir / 100.0;
		else
			F8 = SystemState->Measured.Tempratures.Room / 100.0;

		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS))
			F8 = SystemState->Measured.Tempratures.SupplyAir / 100.0;
	}

	SystemState->Measured.F8 = F8 * 100.0;

	uint16_t FanMin =
			(SystemState->Current.CoolingMode == MODE_COOLING) ?
					SystemState->Paramters.PARAM[PARAM_FANMIN_COOLING] : SystemState->Paramters.PARAM[PARAM_FANMIN_ECO];

	uint16_t FanMax =
			(SystemState->Current.CoolingMode == MODE_COOLING) ?
					SystemState->Paramters.PARAM[PARAM_FANMAX_COOLING] : SystemState->Paramters.PARAM[PARAM_FANMAX_ECO];

	uint16_t SetPnt =
			(SystemState->Current.PowerMode == MODE_NORMAL) ?
					SystemState->Paramters.PARAM[PARAM_COOLING_SETPOINT] : SystemState->Paramters.PARAM[PARAM_PWRSAVE_SETPOINT];

	switch (SystemState->Current.CoolingMode) {
	case MODE_COOLING:
	case MODE_FOLLOW_COOLING: {
		if (SystemState->Required.Compressor.Freq < COMP_MAX_FREQ)
			SystemState->LastRunTime.xLastTimeParamNot100 = xTaskGetTickCount(); //If ew not at max reset timmer

		if (SystemState->Required.Compressor.Freq > COMP_MIN_FREQ)
			SystemState->LastRunTime.xLastTimeParamNot0 = xTaskGetTickCount(); //If ew not at max reset timmer*/

		if ((xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeParamNot0) > (SECONDS(300))) {
			SystemState->Required.CoolingMode = MODE_ECO;
		} else {
			if (SystemState->Paramters.PARAM[PARAM_HASSLAVE] == 2) {
				if (xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeParamNot100 > SECONDS(2)) {
					SystemState->Required.CoolingMode = MODE_FOLLOW_COOLING;
				}

				if (ScaleToPercent(SystemState->Required.Compressor.Freq) < 40.0)
					SystemState->Required.CoolingMode = MODE_COOLING;
			}
		}

		//Now need to Check all the Errors that cause the 5 min retry
		if (SystemState->C_Errors != 0) {
			if (CHECKFLAG(SystemState->F_Errors, F_ERR_LACK_ABILITY)) {
				SystemState->CompressorRtryTime = (60 * 60);
			} else {
				SystemState->CompressorRtryTime = (60 * 5);
			}
			SystemState->Required.CoolingMode = MODE_FORCED_ECO;
			SystemState->Forced_TryCnt++;
		}

		break;
	}
	case MODE_ECO:
	case MODE_FOLLOW_EC0: {
		if (SystemState->Required.Fans[FAN_PORT_INDOOR].Speed / 100.0 < FanMax)
			SystemState->LastRunTime.xLastTimeParamNot100 = xTaskGetTickCount(); //If ew not at max reset timmer

		if (SystemState->Required.Fans[FAN_PORT_INDOOR].Speed / 100.0 > FanMin)
			SystemState->LastRunTime.xLastTimeParamNot0 = xTaskGetTickCount(); //If ew not at max reset timmer

		//Going to Cooling
		if (xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeParamNot100 > SECONDS(30)) {
			if (!CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS)) {
				//Now just hard overwride if temp to hot if we have a working F8
				if (F8 >= (SystemState->Paramters.PARAM[PARAM_MAX_CHANGEOVER_DELTA]) + SetPnt) {
					if (SystemState->Required.CoolingMode != MODE_FORCED_ECO) {
						SystemState->Required.CoolingMode = MODE_COOLING;
					}
				}
			}
		} else {
			if (SystemState->Paramters.PARAM[PARAM_HASSLAVE] == 2) {
				if (xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeParamNot100 > SECONDS(2)) {
					SystemState->Required.CoolingMode = MODE_FOLLOW_EC0;
				}

				if (SystemState->Required.Fans[FAN_PORT_INDOOR].Speed / 100.0 < 40.0)
					SystemState->Required.CoolingMode = MODE_ECO;
			}
		}

		if (xTaskGetTickCount() - SystemState->LastRunTime.xLastTimeParamNot0 > SECONDS(120)) { //2 min at MIN
			//SystemState->Required.CoolingMode = MODE_SUPER_ECO;
		}

		if (CHECKFLAG(SystemState->L_Errors, L_ERR_INDOOR_BOTH_SENSORS)) {					//Limp Mode
			SystemState->Required.CoolingMode = MODE_COOLING;
		}

		break;
	}
	case MODE_FORCED_ECO: {
		if (SystemState->Measured.TimeInMode > SystemState->CompressorRtryTime) {
			if (SystemState->Forced_TryCnt < 3) {
				if (F8 < SetPnt + 5) {
					SystemState->Required.CoolingMode = MODE_COOLING;
				}
			}
		} else
			SystemState->Required.CoolingMode = MODE_FORCED_ECO;
		break;
	}
	default: {
		SystemState->Required.CoolingMode = MODE_ECO;
		break;
	}

	} //Switch

}

void LoadDefaults(uint8_t IncIP, uint8_t Static) {

	printf("Loading Defaults\r\n");

	for (int i = 0; i < 16; i++) {
		SystemState->Paramters.PARAM[i] = limit_para_tab[i][PARAM_DEFAULT];

		SystemState->Vendor = DefaultVendor;
		SystemState->UnitType = DefaultUnit;
		SystemState->CompressorSize = DefaultComp;
	}

//Should decided if this reset IP SEtting?
	if (IncIP) {
		if (Static) {
			SystemState->IP12 = 0xc0a8;
			SystemState->IP34 = 0x00E6;

			SystemState->GW12 = 0xc0a8;
			SystemState->GW34 = 0x0001;

			SystemState->NM12 = 0xFFFF;
			SystemState->NM34 = 0xFF00;

			SystemState->ServerIP.addr = 0;
			SystemState->ServerPort = 0;
		} else {
			SystemState->IP12 = 0x0000;
			SystemState->IP34 = 0x0000;

			SystemState->GW12 = 0x0000;
			SystemState->GW34 = 0x0000;

			SystemState->NM12 = 0x0000;
			SystemState->NM34 = 0x0000;
			SystemState->ServerPort = 0;
		}
		SystemState->ServerIP.addr = 0x00;
	}

}

#define ATSECOND(A) (Tdiff>=SECONDS((A)))&&(Tdiff<=SECONDS((A+1)))
void RunMachineTest() {
	portTickType Tdiff = xTaskGetTickCount() - TestingTime;

	if (SystemState->SoftwareTest == 255) {
		SETBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
		NVIC_SystemReset();
		SystemState->SoftwareTest = 0;
	}

	if ((SystemState->SoftwareTest < 11)) {
		if ((SystemState->Errors != ERR_CODES_OK)) {
			SystemState->SoftwareTest = 21;
			SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_OFF;
			SystemState->Required.Compressor.Freq = 0;
			SystemState->Current.Compressor.Freq = 0;
			SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0000;
			SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0000;

		} else {

			if (ATSECOND(0)) {
				CLEARBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
				SystemState->Required.Compressor.Freq = 0;
				SystemState->Current.Compressor.Freq = 0;
				SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0;
				SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;
				SystemState->Required.Fans[FAN_PORT_SPARE].Speed = 0;
				SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
				SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
				SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_CLOSE;
			}
			if (ATSECOND(8)) {
				CLEARBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
				SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_CLOSE;
			}
			if (ATSECOND(16)) {
				SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
			}
			if (ATSECOND(24)) {
				SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
			}
			if (ATSECOND(32)) {
				SystemState->LastRunTime.xLastWakeTime_EXTI[3] = xTaskGetTickCount();
				SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
			}
			if (ATSECOND(39)) {
				SystemState->LastRunTime.xLastWakeTime_EXTI[3] = xTaskGetTickCount();
			}
			if (ATSECOND(40)) {

				SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 5000;
			}
			if (ATSECOND(45)) {
				SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 10000;
			}
			if (ATSECOND(83)) {
				SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0000;
			}
			if (ATSECOND(85)) {
				SystemState->LastRunTime.xLastWakeTime_EXTI[2] = xTaskGetTickCount();
				SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 5000;
			}
			if (ATSECOND(90)) {
				SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 10000;
			}
			if (ATSECOND(95)) {
				SETBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
				SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 5000;
			}
			if (Tdiff > SECONDS(100)) {
				SystemState->SoftwareTest = 10;
				SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_RUNNING;
				SystemState->LastRunTime.xLastWakeTime_EXTI[3] = xTaskGetTickCount();
				SystemState->Required.Compressor.Freq = COMP_MAX_FREQ;
				SystemState->Current.Compressor.Freq = COMP_MAX_FREQ;
			}
			if (ATSECOND(110)) {
				SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_RUNNING;
				SystemState->Required.Compressor.Freq = COMP_MAX_FREQ;
				SystemState->Current.Compressor.Freq = COMP_MAX_FREQ;
			}
			if (Tdiff > SECONDS((110 + 120))) {
				if (SystemState->Measured.Tempratures.Suction < 2500) {
					SystemState->SoftwareTest = 11;
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_OFF;
					SystemState->Required.Compressor.Freq = 0;
					SystemState->Current.Compressor.Freq = 0;
					SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0000;
				}
			} else if (Tdiff > SECONDS((110 + 120 + 120))) {
				SystemState->SoftwareTest = 21;
			}

		} //Timmer Loop
	} //Less then 11
}

void StartSystemTask(void *argument) {

	bool FastBoot = false;
//uint8_t SimToggle = 0;
	portTickType xFrequency = 1000 / 50;

	spiIoInit();

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);
//Read EEprom So we can compare
	memset(EEpromParam_Buf, 0x00, 16);

	for (int i = 0; i < 16; i++)
		EEpromParam_Buf[i] = SystemState->Paramters.PARAM[i];

#ifdef EXTERNAL_EEPROM
	if (LoadParamFromEEprom(0x0000) != SystemState->VERSION) {
		//Default
		LoadDefaults(false, false);
		//SaveParamToEEprom(0x0000);
	}
	LoadStatsFromEEprom(0x1001);
	SystemState->Unique_ID = IO_Read_ID1();
#else
//MY_FLASH_SetSectorAddrs(11, 0x080E0000);
	MY_FLASH_SetSectorAddrs(7, 0x08060000);
	if (LoadInternalEEProm() != SystemState->VERSION) {
		LoadDefaults(false, false);
		CheckParams();
	}
	SystemState->Unique_ID = ((uint64_t) HAL_GetUIDw0()) << 32 | HAL_GetUIDw1();
#endif

//SaveInternalEEProm();

//SystemState->Paramters.PARAM[PARAM_HASSLAVE] = true;
//SystemState->Paramters.PARAM[PARAM_ROTATE_TIME] = 2;

	SystemState->Measured.Tempratures.ReturnAir = (int) -10000;
	SystemState->Measured.Tempratures.SupplyAir = (int) -10000;
	SystemState->Measured.Tempratures.Room = (int) -10000;
	SystemState->Measured.Tempratures.IndoorCoil = (int) -10000;
	SystemState->Measured.Tempratures.CompDischarge = (int) -10000;
	SystemState->Measured.Tempratures.Suction = (int) -10000;
	SystemState->Measured.Tempratures.OutdoorCoil = (int) -10000;
	SystemState->Measured.Tempratures.OutsideAir = (int) -10000;
	bool FirstBoot = true;

	SystemState->P = 1000;
	SystemState->I = 10;
	SystemState->D = 500;

	pid_f32_init(&safe_pid_data, SystemState->P / 100.0, SystemState->I / 100.0, SystemState->D / 100.0, 1.0 / xFrequency);
	safe_pid_data.p_max = 100;
	safe_pid_data.p_min = -100;
	safe_pid_data.i_max = 50;
	safe_pid_data.i_min = -50;
	safe_pid_data.d_max = 100;
	safe_pid_data.d_min = -100;
	safe_pid_data.total_max = 50;
	safe_pid_data.total_min = -50;

	pid_f32_init(&comp_pid_data, SystemState->P / 100.0, SystemState->I / 100.0, SystemState->D / 100.0, 1.0 / xFrequency);
	comp_pid_data.p_max = 100;
	comp_pid_data.p_min = -100;
	comp_pid_data.i_max = 50;
	comp_pid_data.i_min = -50;
	comp_pid_data.d_max = 100;
	comp_pid_data.d_min = -100;
	comp_pid_data.total_max = 50;
	comp_pid_data.total_min = -50;

	pid_f32_init(&fan_pid_data, SystemState->P / 100.0, SystemState->I / 100.0, SystemState->D / 100.0, 1.0 / xFrequency);
	fan_pid_data.p_max = 100;
	fan_pid_data.p_min = -100;
	fan_pid_data.i_max = 50;
	fan_pid_data.i_min = -50;
	fan_pid_data.d_max = 100;
	fan_pid_data.d_min = -100;
	fan_pid_data.total_max = 50;
	fan_pid_data.total_min = -50;

	SystemState->Required.CoolingMode = MODE_FULL;
	SystemState->Roatte_RemainTime = SystemState->Paramters.PARAM[PARAM_ROTATE_TIME];

	if ((HAL_GPIO_ReadPin(USER_SW_GPIO_Port, USER_SW_Pin) == 0))
		FastBoot = true;

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 5000);

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, (SystemState->Paramters.PARAM[PARAM_STARTTIMEOUT] * 1000) + 3000);
//Run start sequqnce
	SystemState->IPSet = true;
//Now we can check the Power
	CheckPower();

//Now we can check if button pressed and go into DEmo mode
	if (!FastBoot) {
//WAit anohter 5 seconds

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);
		CheckPower();
		POWER_MODE NowMode = SystemState->Required.PowerMode;

		if (NowMode == MODE_PWRSAVE)
			HAL_GPIO_WritePin(OUT_48_GPIO_Port, OUT_48_Pin, 1); //Enable 48 V Now Quick

		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000);
		if (SystemState->UnitType != UNIT_ECOSAFE) {
			for (int i = 0; i < 20; i++) {
				vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 1000 - 250);
				if (NowMode == MODE_PWRSAVE) {
					HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
					vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 125);
					HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
					vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 125);
				} else {
					vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 125);
				}
				HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
				vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 125);
				HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
				if (SystemState->SoftwareTest != 0) {
					break;
				}
			}
		}
		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = SystemState->Paramters.PARAM[PARAM_FANMAX_ECO];

		for (int g = 0; g < 6; g++) {
			if (SystemState->SoftwareTest != 0) {
				break;
			}
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, SECONDS(10));
		}

//Make sure Power is set correctly
//So here might be worth frocing who is Powered.

		SystemState->Current.PowerMode = SystemState->Required.PowerMode;
		SystemState->LastRunTime.xLastPwrPassedCheck = xTaskGetTickCount();
		SystemState->LastRunTime.xLastModePassedCheck = xTaskGetTickCount();

		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 100);
	}

	HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 500);
	HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);

	/* Infinite loop */
	TestingTime = xTaskGetTickCount();
	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_SYSTEM_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, 10000);
			continue;
		}

		if (FirstBoot) {
			if ((xTaskGetTickCount() > 5000))
				FirstBoot = false;
		}

		if (FirstBoot == false) {

#ifndef IGNOREIPM
			CheckPower();
#endif

			if (SystemState->Paramters.PARAM[PARAM_USE_PID] >= 3)
				CheckMode_PID();
			//else
			//	CheckMode();
		}

		if (SystemState->SoftwareTest != 0) {
			RunMachineTest();
		} else {
			//Main Control Loop

			TestingTime = xTaskGetTickCount();
			if (SystemState->Current.CoolingMode == MODE_COOLING) {
				if (SystemState->Paramters.PARAM[PARAM_USE_PID] >= 5) {
					SystemState->Required.Compressor.Freq = SystemState->Current.Compressor.Freq;
				} else if (SystemState->Paramters.PARAM[PARAM_USE_PID] > 0) {
					CalcCompPID();
				}
			} else {
				SystemState->Required.Compressor.Freq = 0;
			}

			//No Errors
			if ((SystemState->Current.Compressor.Freq <= 0) || (SystemState->CompressorRunCnt == COMPRUN_FIRST)) { ///Need to beef up logic to confirm compressor is 100% running
				if (SystemState->Paramters.PARAM[PARAM_USE_PID] > 0)
					CalcFanPID();
				else
					CalcFanCompressorOff();
			} else
				CalcFanCompressorOn();

			if (SystemState->UnitType == UNIT_ECOSAFE) {
				SystemState->Paramters.PARAM[PARAM_SENSOR] = 0;
				SystemState->Paramters.PARAM[PARAM_HASSLAVE] = 0;
			}

			ControlDampper();

			if((SystemState->Current.CoolingMode == MODE_FOLLOW_COOLING)||(SystemState->Current.CoolingMode == MODE_FOLLOW_EC0))
			{
				//Indoor Fan
				SystemState->Following_Required.Fans[0].Speed=SystemState->Required.Fans[0].Speed;
				//Dammper
				SystemState->Following_Required.Steppers[0].Position=SystemState->Required.Steppers[0].Position;
				SystemState->Following_Required.Steppers[1].Position=SystemState->Required.Steppers[1].Position;

				SystemState->Following_Required.Compressor.Freq=SystemState->Required.Compressor.Freq;
			}
			else
			{
				SystemState->Following_Required.Fans[0].Speed=0;
				//Dammper
				SystemState->Following_Required.Steppers[0].Position=DAMPER_CLOSE;
				SystemState->Following_Required.Steppers[1].Position=DAMPER_CLOSE;

				SystemState->Following_Required.Compressor.Freq=0;
			}

		} //end of Main Loop

		if (SystemState->Required.CoolingMode == SystemState->Current.CoolingMode)
			SystemState->LastRunTime.xLastModePassedCheck = xTaskGetTickCount();

		if (SystemState->Required.PowerMode == SystemState->Current.PowerMode)
			SystemState->LastRunTime.xLastPwrPassedCheck = xTaskGetTickCount();

		SystemState->Measured.ModeDiffrentTime = (xTaskGetTickCount() - SystemState->LastRunTime.xLastModePassedCheck) / 1000.0;
		SystemState->Measured.PwrDiffrentTime = (xTaskGetTickCount() - SystemState->LastRunTime.xLastPwrPassedCheck) / 1000.0;

		//Now we have check waht mode we want to be in Required as wel las when last we changed and when last we wanted to change.
		//We have calcuated how long since change as well as how long since wanting to change
		//So make a call on changing now

		if (SystemState->Current.CoolingMode == MODE_FULL) {		//First Mode Selection
			SystemState->Current.CoolingMode = SystemState->Required.CoolingMode;
			if (SystemState->Current.CoolingMode == MODE_COOLING)
				SystemState->CompressorRunCnt = COMPRUN_FIRST;
			else
				SystemState->CompressorRunCnt = COMPRUN_NEVER;
		} else {
			if (SystemState->Measured.ModeDiffrentTime > (2)) {
				SystemState->Current.CoolingMode = SystemState->Required.CoolingMode;
				SystemState->LastRunTime.xLastTimeParamNot100 = xTaskGetTickCount();
				SystemState->LastRunTime.xLastTimeParamNot0 = xTaskGetTickCount();
				//Been trying to change mode for 2min
				SystemState->LastRunTime.xLastModeChange = xTaskGetTickCount();
				//Now just set compressor value
				if (SystemState->Required.CoolingMode == MODE_COOLING)
					SystemState->CompressorRunCnt = COMPRUN_FIRST;
				else
					SystemState->CompressorRunCnt = COMPRUN_NEVER;
			}		//Waited 2min

		}

		if (SystemState->Current.PowerMode == MODE_NORMAL) {
			if (SystemState->Measured.PwrDiffrentTime > (15)) {
				SystemState->Current.PowerMode = MODE_PWRSAVE;
				SystemState->LastRunTime.xLastPwrModeChange = xTaskGetTickCount();
				HAL_GPIO_WritePin(OUT_48_GPIO_Port, OUT_48_Pin, 1);
				if (!FastBoot)
					NVIC_SystemReset();
			}
		} else {
			if (SystemState->Measured.PwrDiffrentTime > (2 * 60)) {
				SystemState->Current.PowerMode = MODE_NORMAL;
				SystemState->LastRunTime.xLastPwrModeChange = xTaskGetTickCount();
				HAL_GPIO_WritePin(OUT_48_GPIO_Port, OUT_48_Pin, 0);
				if (!FastBoot)
					NVIC_SystemReset();
			}
		}

		SystemState->Measured.TimeInMode = (xTaskGetTickCount() - SystemState->LastRunTime.xLastModeChange) / 1000.0;
		SystemState->Measured.TimeInPwrMode = (xTaskGetTickCount() - SystemState->LastRunTime.xLastPwrModeChange) / 1000.0;

		//Turn Everything OFF
		if ((SystemState->PoweredUp == 0x00))
			SystemState->Required.Compressor.Freq = 0;

		if ((SystemState->PoweredUp == 0x00) && (SystemState->Current.Compressor.Freq == 0)) {
			SystemState->Required.Fans[0].Speed = 0;
			SystemState->Required.Fans[1].Speed = 0;
			SystemState->Required.Fans[2].Speed = 0;
			SystemState->Required.Steppers[0].Position = DAMPER_CLOSE;
			SystemState->Required.Steppers[1].Position = DAMPER_CLOSE;
			SystemState->Required.Steppers[2].Position = DAMPER_CLOSE;
			xFrequency = 1000;
		} else
			xFrequency = 1000 / 50;

		CheckParams();

		if (SystemState->Stats.Save == 1) {
#ifdef EXTERNAL_EEPROM
			SaveStatsToEEprom(0x1001);
#else
			SaveInternalEEProm();
#endif
			SystemState->Stats.Save = 0;
			printf("Save Stats\r\n");
		}

		if (SystemState->RebootMe) {
			printf("Rebooting");
			for (int i = 0; i < 50; i++) {
				HAL_GPIO_TogglePin(BELL_GPIO_Port, BELL_Pin);
				//vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Err, 50);
				osDelay(250);
				printf(".");
			}

			NVIC_SystemReset();
		}

		HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		/*
		 if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_SIM)) {

		 } else {
		 //Sim
		 if (SimToggle >= 10) {
		 HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		 SimToggle = 0;
		 } else
		 SimToggle++;
		 }*/

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, xFrequency);
		osDelay(1000);
	}

}

void DEMOMode() {
	portTickType xFrequency = 1000 / 50;
	int i = 0;

	while (1) {

		for (i = 0; i < 20; i += 1) {
			HAL_GPIO_TogglePin(BELL_GPIO_Port, BELL_Pin);
			osDelay(200);
		}

		CheckPower();

		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0;
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;

		CLEARBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
		for (i = 0; i < 10000; i += 500) {
			SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = i;
			osDelay(2500);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
			printf("FanO:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed, SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed);
		}
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;
		osDelay(5000);

		printf("FanO:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed, SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed);

		for (i = 0; i < 10000; i += 500) {
			SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = i;
			osDelay(2500);
			printf("FanI:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_INDOOR].Speed, SystemState->Current.Fans[FAN_PORT_INDOOR].Speed);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}
		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0;
		osDelay(5000);
		printf("FanI:\t%d\t%d\r\n", SystemState->Required.Fans[FAN_PORT_INDOOR].Speed, SystemState->Current.Fans[FAN_PORT_INDOOR].Speed);

		SETBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
		if (SystemState->Required.PowerMode == MODE_PWRSAVE) {
			HAL_GPIO_WritePin(OUT_48_GPIO_Port, OUT_48_Pin, 1);
		} else {
			HAL_GPIO_WritePin(OUT_48_GPIO_Port, OUT_48_Pin, 0);
		}

		printf("Pwr :\t%d\t%d\r\n", SystemState->Required.PowerMode, SystemState->Current.PowerMode);

		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);

		HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0;
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;
		SystemState->Required.Compressor.Freq = 0;
		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_OPEN;
		osDelay(30000);

		SystemState->Required.CoolingMode = MODE_COOLING;
		/*
		 SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 5500;
		 SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 10000;
		 osDelay(30000);
		 for (i = 20; i < 70; i += 10) {
		 SystemState->Required.Compressor.Freq = ScaleToHz(i/100.0);
		 osDelay(30000);
		 HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		 }*/

		SystemState->Required.CoolingMode = MODE_COOLING;
		for (i = 0; i < 10000; i += 500) {
			SystemState->Required.Compressor.Freq = ScaleToHz(i / 100.0);
			CalcFanCompressorOn();
			osDelay(30000);
			printf("Comp:\t%d\t%d\r\n", SystemState->Required.Compressor.Freq, SystemState->Current.Compressor.Freq);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}

		osDelay(120000);
		osDelay(120000);
		osDelay(120000);

		for (i = 10000; i > 1000; i -= 500) {
			SystemState->Required.Compressor.Freq = ScaleToHz(i / 100.0);
			CalcFanCompressorOn();
			osDelay(30000);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
			printf("Comp:\t%d\t%d\r\n", SystemState->Required.Compressor.Freq, SystemState->Current.Compressor.Freq);
		}

		SystemState->Required.Compressor.Freq = ScaleToHz(0);
		osDelay(120000);

		CLEARBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);
		for (i = 0; i < 10000; i += 500) {
			SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = i;
			osDelay(2500);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;
		osDelay(5000);
		SETBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN);

		for (i = 0; i < 10000; i += 500) {
			SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = i;
			osDelay(2500);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}
		SystemState->Required.Fans[FAN_PORT_INDOOR].Speed = 0;
		osDelay(5000);

		while (SystemState->Current.Steppers[0].Position == DAMPER_BUSY)
			osDelay(1000);
		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_OPEN;
		osDelay(500);
		while (SystemState->Current.Steppers[0].Position == DAMPER_BUSY)
			osDelay(1000);

		HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		for (i = DAMPER_OPEN; i > DAMPER_OPEN - 150; i -= 5) {
			SystemState->Required.Steppers[STEPPER_RETRUN].Position = i;
			SystemState->Required.Steppers[STEPPER_FRESH].Position = i;
			SystemState->Required.Steppers[STEPPER_EXPANTION].Position = i;
			osDelay(10);
			while (SystemState->Current.Steppers[0].Position == DAMPER_BUSY)
				osDelay(500);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}

		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_CLOSE;
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_CLOSE;
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_CLOSE;
		osDelay(1000);
		while (SystemState->Current.Steppers[0].Position == DAMPER_BUSY)
			osDelay(1000);

		for (i = 0; i < 150; i += 5) {
			SystemState->Required.Steppers[STEPPER_RETRUN].Position = i;
			SystemState->Required.Steppers[STEPPER_FRESH].Position = i;
			SystemState->Required.Steppers[STEPPER_EXPANTION].Position = i;
			osDelay(10);
			while (SystemState->Current.Steppers[0].Position == DAMPER_BUSY)
				osDelay(500);
			HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		}

		SystemState->Required.Steppers[STEPPER_RETRUN].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_FRESH].Position = DAMPER_OPEN;
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_OPEN;
		osDelay(10000);

		SETBIT(SystemState->Required.Realys, 3);
		SETBIT(SystemState->Required.Realys, 0);
		osDelay(2500);
		SETBIT(SystemState->Required.Realys, 1);
		osDelay(2500);
		SETBIT(SystemState->Required.Realys, 2);
		osDelay(2500);
		CLEARBIT(SystemState->Required.Realys, 0);
		osDelay(2500);
		CLEARBIT(SystemState->Required.Realys, 1);
		osDelay(2500);
		CLEARBIT(SystemState->Required.Realys, 2);
		osDelay(2500);
		CLEARBIT(SystemState->Required.Realys, 3);
		osDelay(2500);

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_System, xFrequency);
	}
}
