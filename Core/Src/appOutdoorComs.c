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

APP_OUTDOOR_DATA appOutdoordata;

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern APP_SYSTEM_DATA *SystemState;
extern const uint8_t Comp_Params[6][2];

#define WARMUP_2ND 30
#define WARMUP_1ST 20

struct {
	//Compressor Discharge
	portTickType cdLevelChange;
	uint8_t cdLevel, cdLastLevel;

	//IPM Protection
	portTickType ipmLevelChange;
	uint8_t ipmLevel, ipmLastLevel;

	//Evaporatio Indoor cool
	portTickType evLevelChange;
	uint8_t evLevel, evLastLevel;

	//CondencerTemp
	portTickType condLevelChange;
	uint8_t condLevel, condLastLevel;

	//CondencerTemp
	portTickType DcAmpChange;
	uint8_t DcAmpLevel, DcAmpLastLevel;

	//OutDoor Freq Limot
	uint8_t FreqLimit;

} Comp_Protection_Internal;

//uint8_t freqMax[11] = { 0, 2, 4, 5, 7, 9, 10, 7, 5, 2, 1 };
//uint8_t freqLevel[11] = { 0, 27, 30, 33, 38, 45, 48, 52, 56, 61, 65 }; // Hz Levels
uint8_t freqLimitLevel[10] = { 0, 100, 100, 100, 100, 100, 88, 62, 42, 22 }; // Percentaage of the

uint8_t ScaleToPercent(uint8_t Hz) {
	uint8_t Range = (COMP_MAX_FREQ - COMP_MIN_FREQ);
	uint8_t Percent = (((float) ((Hz - COMP_MIN_FREQ) / (float) Range)) * 100.0);
	return Percent;
}

uint8_t ScaleToHz(float Percent) {
	uint8_t Range = (COMP_MAX_FREQ - COMP_MIN_FREQ);
	float Hz = 0;

	if (Percent > 0)
		Hz = (COMP_MIN_FREQ + (Range * Percent));

	return (uint8_t) Hz;
}

//Compressor Discharge
COMP_PROTECTINON_TYPE CompDischagePrtectionCheck() {
	uint8_t tdStop = 120;
	uint8_t tdDown2 = 105;
	uint8_t tdDown1 = 100;
	uint8_t tdNormal = 95;
	uint8_t tdUp = 90;

	float tdTemp = SystemState->Measured.Tempratures.CompDischarge / 100.0;

	COMP_PROTECTINON_TYPE Ret = COMP_HOLD;

	if (tdTemp >= tdStop) {
		Comp_Protection_Internal.cdLevel = 5;
		if (xTaskGetTickCount() - Comp_Protection_Internal.cdLevelChange > SECONDS(10)) //10 Seconds then turn off Compressor
			Ret = COMP_STOP;
	} else if ((tdTemp >= tdDown2) && (tdTemp < tdStop)) {
		Comp_Protection_Internal.cdLevel = 4;
		if (xTaskGetTickCount() - Comp_Protection_Internal.cdLevelChange > SECONDS(90)) //90 Seconds then drop Compressor
			Ret = COMP_DOWN;
	} else if ((tdTemp >= tdDown1) && (tdTemp < tdDown2)) {
		Comp_Protection_Internal.cdLevel = 3;
		if (xTaskGetTickCount() - Comp_Protection_Internal.cdLevelChange > SECONDS(180)) //90 Seconds then drop Compressor
			Ret = COMP_DOWN;
	} else if ((tdTemp >= tdNormal) && (tdTemp < tdDown1)) {
		Comp_Protection_Internal.cdLevel = 2;
		Ret = COMP_HOLD;
	} else if ((tdTemp >= tdUp) && (tdTemp < tdNormal)) {
		Comp_Protection_Internal.cdLevel = 1;
		if (xTaskGetTickCount() - Comp_Protection_Internal.cdLevelChange > SECONDS(90)) //90 Seconds then try increase slowly
			Ret = COMP_SLOWUP;
	} else {
		Comp_Protection_Internal.cdLevel = 0;
		Ret = COMP_UP;
	}

	if ((Comp_Protection_Internal.cdLevel != Comp_Protection_Internal.cdLastLevel) || (Ret == COMP_DOWN) || (Ret == COMP_SLOWUP)) {
		Comp_Protection_Internal.cdLevelChange = xTaskGetTickCount();
		Comp_Protection_Internal.cdLastLevel = Comp_Protection_Internal.cdLevel;
		if (Ret >= COMP_HOLD)
			ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_DISCHARGE_TEMP_LIMIT);
		else
			REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_DISCHARGE_TEMP_LIMIT);
	}
	return Ret;
}

//IPM TEMP
COMP_PROTECTINON_TYPE IPMTempPrtectionCheck() {

	uint16_t ipmTempStop = 100;
	uint16_t ipmTempDown = 90;
	uint16_t ipmTempNorm = 80;
	COMP_PROTECTINON_TYPE Ret = COMP_HOLD;

	if (SystemState->UnitType == UNIT_ECOSAFE)
		return COMP_UP;

	float ipmTemp = (int) SystemState->Measured.Outdoor.tempIpm101T / 100.0;

	if (ipmTemp >= ipmTempStop) {
		Ret = COMP_STOP;
		Comp_Protection_Internal.ipmLevel = 3;
	} else if (ipmTemp >= ipmTempDown) {
		Comp_Protection_Internal.ipmLevel = 2;
		if (xTaskGetTickCount() - Comp_Protection_Internal.ipmLevelChange > SECONDS(60)) //90 Seconds then try increase slowly
			Ret = COMP_DOWN;
	} else if (ipmTemp >= ipmTempNorm) {
		Ret = COMP_HOLD;
		Comp_Protection_Internal.ipmLevel = 1;
	} else {
		Comp_Protection_Internal.ipmLevel = 0;
		Ret = COMP_UP;
	}

	if ((Comp_Protection_Internal.ipmLevel != Comp_Protection_Internal.ipmLastLevel) || (Ret == COMP_DOWN) || (Ret == COMP_SLOWUP)) { // Every time level changes record it and act
		Comp_Protection_Internal.ipmLevelChange = xTaskGetTickCount();
		Comp_Protection_Internal.ipmLastLevel = Comp_Protection_Internal.ipmLevel;
		if (Ret >= COMP_HOLD)
			ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT);
		else
			REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT);

	}

	return Ret;
}

//evaAntifreezeProtect
COMP_PROTECTINON_TYPE evaAntifreezeProtectCheck() {
	int8_t teTempNormal = 3;
	int8_t teTempDown1 = 2;
	int8_t teTempDown2 = 1;
	int8_t teTempStop = 0;

	COMP_PROTECTINON_TYPE Ret = COMP_HOLD;

	if (SystemState->SoftwareTest > 0) {
		return Ret;
	}
	float teTemp = SystemState->Measured.Tempratures.Suction / 100.0;
	if (teTemp < teTempStop) {
		Comp_Protection_Internal.evLevel = 4;
		Ret = COMP_STOP;
	} else if (teTemp < teTempDown2) {
		Comp_Protection_Internal.evLevel = 3;
		if (xTaskGetTickCount() - Comp_Protection_Internal.evLevelChange > SECONDS(30)) //90 Seconds then try increase slowly
			Ret = COMP_DOWN;
	} else if (teTemp < teTempDown1) {
		Comp_Protection_Internal.evLevel = 2;
		if (xTaskGetTickCount() - Comp_Protection_Internal.evLevelChange > SECONDS(60)) //90 Seconds then try increase slowly
			Ret = COMP_DOWN;
	} else if (teTemp < teTempNormal) {
		Comp_Protection_Internal.evLevel = 1;
		if (xTaskGetTickCount() - Comp_Protection_Internal.evLevelChange > SECONDS(60)) //90 Seconds then try increase slowly
			Ret = COMP_HOLD;
	} else {
		Comp_Protection_Internal.evLevel = 0;
		Ret = COMP_UP;
	}

	//Only want to react to level changes and to DOWN/Slowup once so reset timmer if level changes or it that ret values
	if ((Comp_Protection_Internal.evLevel != Comp_Protection_Internal.evLastLevel) || (Ret == COMP_DOWN) || (Ret == COMP_SLOWUP)) { // Every time level changes record it and act
		Comp_Protection_Internal.evLevelChange = xTaskGetTickCount();
		Comp_Protection_Internal.evLastLevel = Comp_Protection_Internal.evLevel;
		if (Ret >= COMP_HOLD)
			ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_INDOOR_COIL_LIMIT);
		else
			REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_INDOOR_COIL_LIMIT);

	}
	return Ret;
}

COMP_PROTECTINON_TYPE CurerntProtectCheck() {
	COMP_PROTECTINON_TYPE Ret = COMP_HOLD;
	uint8_t DCCurrentDown2 = SystemState->DCCurrentLimit;
	uint8_t DDCurrentHold = DCCurrentDown2 - 2;

	float CompDcCurrent = SystemState->ExternalCurrent2 / 100.0;

	if (CompDcCurrent > DCCurrentDown2) // Timeout
			{
		Comp_Protection_Internal.DcAmpLevel = 5;
		if (xTaskGetTickCount() - Comp_Protection_Internal.DcAmpChange > SECONDS(10)) {
			Ret = COMP_DOWN;
		}
	} else if (CompDcCurrent > DDCurrentHold) {
		Comp_Protection_Internal.DcAmpLevel = 4;
		Ret = COMP_HOLD;
	} else {
		Ret = COMP_UP;
		Comp_Protection_Internal.DcAmpLevel = 3;
	}

	if ((Comp_Protection_Internal.condLevel != Comp_Protection_Internal.condLastLevel) || (Ret == COMP_DOWN)) {
		Comp_Protection_Internal.DcAmpLastLevel = Comp_Protection_Internal.DcAmpLevel;
		Comp_Protection_Internal.DcAmpChange = xTaskGetTickCount();
		if (Ret >= COMP_HOLD)
			ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AC_CURRENT_LIMIT);
		else
			REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AC_CURRENT_LIMIT);
	}
	return (Ret);

}

//evaAntifreezeProtect
COMP_PROTECTINON_TYPE CondenserTempProtectCheck() {

	uint8_t condenserTempStop = 58;
	uint8_t condenserTempDown2 = 55;
	uint8_t condenserTempDown1 = 54;
	uint8_t condenserTempNorm = 53;
	uint8_t condenserTempUp = 52;

	COMP_PROTECTINON_TYPE Ret = COMP_HOLD;

	float condenserTemp = SystemState->Measured.Tempratures.OutdoorCoil / 100.0;

	if (condenserTemp > condenserTempStop) {
		Comp_Protection_Internal.condLevel = 5;
		if (xTaskGetTickCount() - Comp_Protection_Internal.condLevelChange > SECONDS(10)) //90 Seconds then try increase slowly
			Ret = COMP_STOP;
	} else if (condenserTemp > condenserTempDown2) {
		Comp_Protection_Internal.condLevel = 4;
		if (xTaskGetTickCount() - Comp_Protection_Internal.condLevelChange > SECONDS(10)) //90 Seconds then try increase slowly
			Ret = COMP_DOWN;
	} else if (condenserTemp > condenserTempDown1) {
		Comp_Protection_Internal.condLevel = 3;
		if (xTaskGetTickCount() - Comp_Protection_Internal.condLevelChange > SECONDS(10)) //90 Seconds then try increase slowly
			Ret = COMP_DOWN;
	} else if (condenserTemp > condenserTempNorm) {
		Comp_Protection_Internal.condLevel = 2;
		Ret = COMP_HOLD;
	} else if (condenserTemp > condenserTempUp) {
		Comp_Protection_Internal.condLevel = 1;
		Ret = COMP_SLOWUP;
	} else {
		Comp_Protection_Internal.condLevel = 0;
		Ret = COMP_UP;
	}

	if ((Comp_Protection_Internal.condLevel != Comp_Protection_Internal.condLastLevel) || (Ret == COMP_DOWN) || (Ret == COMP_SLOWUP)) { // Every time level changes record it and act
		Comp_Protection_Internal.condLevelChange = xTaskGetTickCount();
		Comp_Protection_Internal.condLastLevel = Comp_Protection_Internal.condLevel;
		if (Ret >= COMP_HOLD)
			ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_COIL_TEMP_LIMIT);
		else
			REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_COIL_TEMP_LIMIT);
	}

	return Ret;
}
//OutDoor Limit
void CaculateOutDoorFreqLimit() {

	float outdoorTemp = SystemState->Measured.Tempratures.OutsideAir / 100.0;

	static float outdoorTempLast;
	static uint8_t trendtime;
	float trend;

	uint8_t delta;
	static uint8_t firstOutDoor;

	uint8_t freqLimit;

	trend = outdoorTempLast - outdoorTemp;
	if (firstOutDoor == 0) {
		firstOutDoor = 1;
		trend = 0;
	}

	if (trend >= 0) {
		delta = 0;
	} else {
		delta = 1;
	}

	if (outdoorTemp >= (52 + delta)) {
		freqLimit = 10;
	} else if (outdoorTemp >= (49 + delta)) {
		freqLimit = 9;
	} else if (outdoorTemp >= (45 + delta)) {
		freqLimit = 8;
	} else if (outdoorTemp >= (43 + delta)) {
		freqLimit = 7;
	} else if (outdoorTemp >= (40 + delta)) {
		freqLimit = 6;
	} else if (outdoorTemp >= (29 + delta)) {
		freqLimit = 5;
	} else if (outdoorTemp >= (23 + delta)) {
		freqLimit = 4;
	} else if (outdoorTemp >= (17 + delta)) {
		freqLimit = 3;
	} else if (outdoorTemp >= (11 + delta)) {
		freqLimit = 2;
	} else if (outdoorTemp >= (-1 + delta)) {
		freqLimit = 1;
	} else {
		freqLimit = 0;
	}

	if (trendtime >= 10) {
		outdoorTempLast = outdoorTemp;
		trendtime = 0;
	} else {
		trendtime++;
	}
	Comp_Protection_Internal.FreqLimit = MIN(ScaleToHz(freqLimitLevel[freqLimit] / 100.0), Comp_Protection_Internal.FreqLimit);

}

uint16_t AdjustCompFreq(uint16_t Required) {

	static uint8_t AdjCnt = 0;
	uint16_t AllowedFreq = Required;
	COMP_PROTECTINON_TYPE tmpProtectionMode;
	Comp_Protection_Internal.FreqLimit = COMP_MAX_FREQ;

	//Check all protection limits
	if (!CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_DISCHARGE))
		SystemState->Measured.Outdoor.ProctecionLevel = CompDischagePrtectionCheck();
	else {
		Comp_Protection_Internal.FreqLimit = MIN(ScaleToHz(0.75), Comp_Protection_Internal.FreqLimit);
		SystemState->Measured.Outdoor.ProctecionLevel = COMP_UP;
	}

	//For each protection need to check the highes limiting mode
	tmpProtectionMode = IPMTempPrtectionCheck();
	SystemState->Measured.Outdoor.ProctecionLevel = MAX(SystemState->Measured.Outdoor.ProctecionLevel, tmpProtectionMode);
	if (CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT))
		tmpProtectionMode = COMP_HOLD;

	if (SystemState->Current.PowerMode == MODE_PWRSAVE) {
		tmpProtectionMode = CurerntProtectCheck();
		SystemState->Measured.Outdoor.ProctecionLevel = MAX(SystemState->Measured.Outdoor.ProctecionLevel, tmpProtectionMode);
	}

	if (!CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_SUCTION))
		tmpProtectionMode = evaAntifreezeProtectCheck();
	else {
		Comp_Protection_Internal.FreqLimit = MIN(ScaleToHz(0.75), Comp_Protection_Internal.FreqLimit);
		tmpProtectionMode = COMP_UP;
	}
	SystemState->Measured.Outdoor.ProctecionLevel = MAX(SystemState->Measured.Outdoor.ProctecionLevel, tmpProtectionMode);

	if (!CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_COIL))
		tmpProtectionMode = CondenserTempProtectCheck();
	else {
		Comp_Protection_Internal.FreqLimit = MIN(ScaleToHz(0.75), Comp_Protection_Internal.FreqLimit);
		tmpProtectionMode = COMP_UP;
	}
	SystemState->Measured.Outdoor.ProctecionLevel = MAX(SystemState->Measured.Outdoor.ProctecionLevel, tmpProtectionMode);

	//Calculate freq limit to use later
	CaculateOutDoorFreqLimit();

	//Adjust the Freqancy dependent on the Protection mode and what si required
	//Should only get an DOWN or SLOWUP every X secons rest of time will be HOLD or UP
	//Am i trying to incress frequancy then only if protectin allows it do it
	if (Required > SystemState->Current.Compressor.Freq) {
		if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_SLOWUP) {
			if (SystemState->Current.Compressor.Freq < (COMP_MAX_FREQ - ScaleToHz(0.05)))
				AllowedFreq = SystemState->Current.Compressor.Freq + ScaleToHz(0.05);
		} else if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_UP) {
			//Allowed to incress the Freqancy so do it
			AllowedFreq = (Required >= COMP_MAX_FREQ) ? COMP_MAX_FREQ : Required;
		} else if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_UP) {
			AllowedFreq = SystemState->Current.Compressor.Freq;
		} else if ((SystemState->Measured.Outdoor.ProctecionLevel == COMP_HOLD) && (SystemState->Measured.Outdoor.CompressorState != COMPRESSOR_COOLDOWN)) {
			AllowedFreq = SystemState->Current.Compressor.Freq;
		}
	} else {
		//if we trying to do down who cares
	}

	//next check if protection asking for a decrease or stop
	if (SystemState->Measured.Outdoor.ProctecionLevel >= COMP_DOWN) {
		if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_STOP)
			AllowedFreq = 0;
		else if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_DOWN) {
			if (SystemState->Current.Compressor.Freq > (COMP_MIN_FREQ + ScaleToHz(0.1)))
				AllowedFreq = SystemState->Current.Compressor.Freq - ScaleToHz(0.05);
		} else if (SystemState->Measured.Outdoor.ProctecionLevel == COMP_RAPIDDOWN) {
			if (SystemState->Current.Compressor.Freq > (COMP_MIN_FREQ + ScaleToHz(0.15)))
				AllowedFreq = SystemState->Current.Compressor.Freq - ScaleToHz(0.10);
		}
	}

	if (AllowedFreq > Comp_Protection_Internal.FreqLimit) {
		AllowedFreq = Comp_Protection_Internal.FreqLimit;
		ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AMBIENT_LIMIT);
	} else
		REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AMBIENT_LIMIT);

	//Need to impliment a Rate Limiter now
	if (AllowedFreq > COMP_MIN_FREQ) {
		if (abs(AllowedFreq - SystemState->Current.Compressor.Freq) > 1) {
			AdjCnt++;
			if (AdjCnt >= 9) {
				int8_t Adj = (AllowedFreq > SystemState->Current.Compressor.Freq) ? 1 : -1;
				if (SystemState->Measured.Outdoor.CompressorState == COMPRESSOR_COOLDOWN)
					Adj *= 2;
				AllowedFreq = SystemState->Current.Compressor.Freq + Adj;
				AdjCnt = 0;
			} else
				AllowedFreq = MAX(SystemState->Current.Compressor.Freq, COMP_MIN_FREQ);
		}
	}

	AllowedFreq = MAX(AllowedFreq, COMP_MIN_FREQ);

	if (SystemState->SoftwareTest == 0) {
		if (SystemState->CompressorSize >= COMPTYPE_7KW_220ONLY) {
			if (SystemState->Current.PowerMode == MODE_PWRSAVE)
				AllowedFreq = 0;
		}
	}

	if ((AllowedFreq == 0) && (Required != 0))
		__NOP();
	return AllowedFreq;
}

void AdjOutDoorFanLinear() {

	float condenserTemp = SystemState->Measured.Tempratures.OutdoorCoil / 100.0;
	uint8_t Range = 55 - 35;

	float spdDaValue = (condenserTemp - 35) / Range;
	if(spdDaValue>0)
		spdDaValue = CLAMP(spdDaValue, 1, 0.1);
	else
		spdDaValue = 0;
	//Now we have target speed set the REquired Speed in system Memory
	if ((CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_DISCHARGE)) || (CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_SUCTION))
			|| (CHECKFLAG(SystemState->L_Errors, L_ERR_OUTDOOR_COIL)))
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = (1 * 100) * 100;
	else
		SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = (spdDaValue * 100) * 100;
}

void AdjExpnation() {

	float TempDiff = (SystemState->Measured.Tempratures.Suction - SystemState->Measured.Tempratures.IndoorCoil) / 100.0;
	//float TempDiff = (SystemState->Measured.Tempratures.IndoorCoil-SystemState->Measured.Tempratures.Suction) / 100.0;
	float dischargetemp = SystemState->Measured.Tempratures.CompDischarge / 100.0;

	float TempErr;
	float targetTemp;

	//Every 20s we adjust the Vaulve till the Target temp diffrence is met
	if (dischargetemp < 50)
		targetTemp = 5 + 2;
	else if (dischargetemp < 70)
		targetTemp = 5 + 1;
	else if (dischargetemp < 90)
		targetTemp = 5 + 0;
	else if (dischargetemp < 105)
		targetTemp = 5 - 1;
	else
		targetTemp = 5 - 2;

	TempErr = targetTemp - TempDiff;

	//Should look at a PID there rather then compying chinse

	if ((TempErr > 1) || (TempErr <= 1)) {
		//From chinses code looks like only move if more then 1 delta, ie degree
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position += (int) TempErr;
	}

	//SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 300;

	//Clamp Position
	if (SystemState->Required.Steppers[STEPPER_EXPANTION].Position >= 90)
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 90;

	if (SystemState->Required.Steppers[STEPPER_EXPANTION].Position <= 2)
		SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 2;

}

uint16_t checksum_cal(uint8_t *buffer) {
	uint16_t checksum_temp = 0;

	checksum_temp += ((uint16_t) buffer[1] << 8) | buffer[0];
	checksum_temp += ((uint16_t) buffer[3] << 8) | buffer[2];
	checksum_temp += ((uint16_t) buffer[5] << 8) | buffer[4];
	checksum_temp = (~checksum_temp) + 1;

	return checksum_temp;
}

/*********************
 statusCode:
 0x0000:fault flag;
 0x0001:motor speed;
 0x0002:motor state;
 0x0003:node id;
 *********************/
void sendReadStatusCmd(uint16_t statusCode) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = readStatus;
	appOutdoordata.RawTxMessage[2] = (uint8_t) (statusCode & 0x00ff);
	appOutdoordata.RawTxMessage[3] = (uint8_t) ((statusCode >> 8) & 0x00ff);
	appOutdoordata.RawTxMessage[4] = 0x00;
	appOutdoordata.RawTxMessage[5] = 0x00;

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}

void sendClearFaultCmd(void) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = clearFault;
	appOutdoordata.RawTxMessage[2] = 0x00;
	appOutdoordata.RawTxMessage[3] = 0x00;
	appOutdoordata.RawTxMessage[4] = 0x00;
	appOutdoordata.RawTxMessage[5] = 0x00;

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}
/*********************
 mode:
 0x00:uart;
 0x01:analog;
 0x02:freq;
 0x03:duty;
 *********************/
void sendChangeCtrlInputModeCmd(uint8_t mode) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = controlInputMode;
	appOutdoordata.RawTxMessage[2] = 0x00;
	appOutdoordata.RawTxMessage[3] = 0x00;
	appOutdoordata.RawTxMessage[4] = 0x00;
	appOutdoordata.RawTxMessage[5] = mode;

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}

void sendSetTargetSpdCmd(uint16_t targetSpd) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = motorControl_setTargetSpeed;
	appOutdoordata.RawTxMessage[2] = 0x00;
	appOutdoordata.RawTxMessage[3] = 0x00;
	appOutdoordata.RawTxMessage[4] = (uint8_t) (targetSpd & 0x00ff);
	appOutdoordata.RawTxMessage[5] = (uint8_t) ((targetSpd >> 8) & 0x00ff);

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}

void sendReadRegCmd(uint8_t appId, uint8_t regId) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = registerRead;
	appOutdoordata.RawTxMessage[2] = appId;
	appOutdoordata.RawTxMessage[3] = regId;
	appOutdoordata.RawTxMessage[4] = 0x00;
	appOutdoordata.RawTxMessage[5] = 0x00;

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}

void sendWriteRegCmd(uint8_t appId, uint8_t regId, uint16_t value) {

	appOutdoordata.RawTxMessage[0] = NODE_ID;
	appOutdoordata.RawTxMessage[1] = registerWriter;
	appOutdoordata.RawTxMessage[2] = appId;
	appOutdoordata.RawTxMessage[3] = regId;
	appOutdoordata.RawTxMessage[4] = (uint8_t) (value & 0x00ff);
	appOutdoordata.RawTxMessage[5] = (uint8_t) ((value >> 8) & 0x00ff);

	appOutdoordata.RawTxMessage[6] = (uint8_t) (checksum_cal(appOutdoordata.RawTxMessage) & 0x00ff);
	appOutdoordata.RawTxMessage[7] = (uint8_t) ((checksum_cal(appOutdoordata.RawTxMessage) >> 8) & 0x00ff);
	HAL_UART_Transmit_IT(&huart2, appOutdoordata.RawTxMessage, 8);
}

void USER_UART_IRQHandler(UART_HandleTypeDef *huart) {

	if (RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE))   //Judging whether it is idle interruption
	{
		__HAL_UART_CLEAR_IDLEFLAG(huart);                     //Clear idle interrupt sign (otherwise it will continue to enter interrupt)
		//printf("\r\nUART1 Idle IQR Detected\r\n");
		HAL_UART_DMAStop(huart);

		//Calculate the length of the received data
		appOutdoordata.data_avalible = OUTDOORBUFFSIZE - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
	}
	if (RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE)) {
		//HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
	}
	if (RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE)) {
		//	appOutdoordata.data_avalible =8;
	}

}

void StartOutdoorComs(void *argument) {
	bool firstboot = true;
	const portTickType xFrequency = 1000 / 10;
	SystemState->LastRunTime.xLastWakeTime_OutDoor = xTaskGetTickCount();
	uint16_t Prev_faultFlag101T;

	uint8_t TxCommandType = 0;
	uint8_t ExpAdjCnt = 0;

	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart2, (uint8_t*) appOutdoordata.RawRxMessage, OUTDOORBUFFSIZE);

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_OutDoor, 2000);

	sendClearFaultCmd();

	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_OUTDOOR_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_OutDoor, 10000);
			continue;
		}

		if (appOutdoordata.data_avalible > 0) {
			if (appOutdoordata.data_avalible != 8) {
				__NOP();
			}

			//HAL_GPIO_TogglePin(LED_RUN_GPIO_Port, LED_RUN_Pin);
			appOutdoordata.data_avalible =
					(appOutdoordata.data_avalible > sizeof(appOutdoordata.RawRxMessage)) ? sizeof(appOutdoordata.RawRxMessage) : appOutdoordata.data_avalible;
			memcpy(&appOutdoordata.RXMsg, appOutdoordata.RawRxMessage, appOutdoordata.data_avalible);

			if (appOutdoordata.RXMsg.node_id == NODE_ID) {
				//Now check crc and then assume it good
				if (appOutdoordata.RXMsg.crc == checksum_cal(appOutdoordata.RawRxMessage)) {
					SystemState->LastRunTime.xLastWakeTime_OutDoorRx = xTaskGetTickCount();
					REMOVEFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR);
					//Now MSg is Valid Try do something
					switch (appOutdoordata.RXMsg.MsgType & 0x0f) {
					case 0x00: {
						if (appOutdoordata.RXMsg.Msg.StatusMsg.Status == 0x00) {
							appOutdoordata.faultFlag101T = appOutdoordata.RXMsg.Msg.StatusMsg.FaultFlag;
						}
						break;
					}		//Status
					case 0x03: {
						appOutdoordata.motorState101T = appOutdoordata.RXMsg.Msg.SpeedMsg.MotorState;
						appOutdoordata.motorSpd101T = appOutdoordata.RXMsg.Msg.SpeedMsg.MotorSpeed * 105.0 / 16383.0;
						break;
					}		//Speed
					case 0x05: {		//DC VOLTS
						switch (appOutdoordata.RXMsg.Msg.RegMsg.RegCode) {
						case 137: {
							appOutdoordata.vdcValue101T = appOutdoordata.RXMsg.Msg.RegMsg.Value * 3.3f / 25.887f;
							break;
						}
						case 0x6c: {
							//New IPM Temp
							appOutdoordata.tempIpm101T = ((appOutdoordata.RXMsg.Msg.RegMsg.Value) * 0.032226563f) - 20;
						}
						case 144: {		//IPM TEMP

							//appOutdoordata.tempIpm101T = appOutdoordata.RXMsg.Msg.RegMsg.Value * 3.3f / 105.267f - 17.899f;
#ifdef IGNOREIPM
								appOutdoordata.tempIpm101T = 0;
#endif
							break;
						}
						case 02: {		//InputCurrent
							appOutdoordata.idFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value / 74.909f;
							break;
						}
						case 04: {		//InputVoltage
							appOutdoordata.vdFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value / 12.281f;
							break;
						}
						case 141: {		//IDFLT(A)
							//appOutdoordata.idFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value * 0.00177f;
							break;
						}
						case 142: {		//IQFLT(A)
							appOutdoordata.iqFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value * 0.00177f;
							break;
						}
						case 156: { //Vd
							//appOutdoordata.vdFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value;
							break;
						}
						case 157: { //Vq
							appOutdoordata.vqFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value;
							break;
						}
						case 112: {
							appOutdoordata.gpioL101T = appOutdoordata.RXMsg.Msg.RegMsg.Value;
							break;
						}
						case 98:
							appOutdoordata.vqFilt101T = appOutdoordata.RXMsg.Msg.RegMsg.Value;
							break;
						}
						break;
					}		//Registar

					}		//Switch MessageType
				}
			}		//NODE ID MATCHS
			memset(appOutdoordata.RawRxMessage, 0x00, appOutdoordata.data_avalible); //REset memory
			appOutdoordata.data_avalible = 0;

			SystemState->Measured.Outdoor.motorSpd101T = (int) (appOutdoordata.motorSpd101T * 100);
			SystemState->Measured.Outdoor.motorState101T = appOutdoordata.motorState101T;

			if (SystemState->UnitType == UNIT_ECOSAFE) {
				SystemState->Measured.Outdoor.vdcValue101T = 0;
					SystemState->Measured.Outdoor.tempIpm101T = 0;
				SystemState->Measured.Outdoor.idFilt101T = 0;
				SystemState->Measured.Outdoor.iqFilt101T = 0;
				SystemState->Measured.Outdoor.vdFilt101T = 0;
				SystemState->Measured.Outdoor.vqFilt101T = 0;
				SystemState->Measured.Outdoor.gpioL101T = 0;
				SystemState->ExternalCurrent1 = (int) 0;
			} else {
				SystemState->Measured.Outdoor.vdcValue101T = (int) (appOutdoordata.vdcValue101T * 100);
				if ((int) (appOutdoordata.tempIpm101T < 0))
					SystemState->Measured.Outdoor.tempIpm101T = 0;
				else
					SystemState->Measured.Outdoor.tempIpm101T = (int) (appOutdoordata.tempIpm101T * 100);
				SystemState->Measured.Outdoor.idFilt101T = (int) (appOutdoordata.idFilt101T * 100);
				SystemState->Measured.Outdoor.iqFilt101T = (int) (appOutdoordata.iqFilt101T * 100);
				SystemState->Measured.Outdoor.vdFilt101T = (int) (appOutdoordata.vdFilt101T * 100);
				//SystemState->Measured.Outdoor.vqFilt101T = (int) (appOutdoordata.vqFilt101T * 100);
				//Think should caculate WATTS and Put it in vQFilt
				SystemState->Measured.Outdoor.vqFilt101T = (int) ((SystemState->Current.Fans[FAN_PORT_INDOOR].Speed * INDOOR_WATT_SCALE)
						+ (SystemState->Current.Fans[FAN_PORT_OUTDOOR].Speed * INDOOR_WATT_SCALE) + (SystemState->Current.Compressor.Freq * COMP_WATT_SCALE));
				SystemState->Measured.Outdoor.gpioL101T = (int) (appOutdoordata.gpioL101T);
				SystemState->ExternalCurrent1 = (int) (appOutdoordata.idFilt101T * 1000.0);
			}

			if (Prev_faultFlag101T == appOutdoordata.faultFlag101T) {
				SystemState->Measured.Outdoor.faultFlag101T = appOutdoordata.faultFlag101T;
			} else {
				appOutdoordata.LastErrCng = xTaskGetTickCount();
			}

			Prev_faultFlag101T = appOutdoordata.faultFlag101T;

			if (xTaskGetTickCount() - appOutdoordata.LastErrCng >= SECONDS(30)) {
				SystemState->Measured.Outdoor.faultFlag101T = appOutdoordata.faultFlag101T;
				if (SystemState->Measured.Outdoor.faultFlag101T != 0x00)
					appOutdoordata.FaultCnt++;
				else
					appOutdoordata.FaultCnt = 0;

				if (SystemState->Measured.Outdoor.faultFlag101T & 0x0100)
					//SystemState->Measured.Outdoor.faultFlag101T = 0x04; //POWER ERR
					ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_CURRENT);
				else
					REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_CURRENT);

				if (SystemState->Measured.Outdoor.faultFlag101T & 0x0010)
					ADDFLAG(SystemState->F_Errors, F_ERR_STEP_MISSING);
				else
					REMOVEFLAG(SystemState->F_Errors, F_ERR_STEP_MISSING);

				if (SystemState->Measured.Outdoor.faultFlag101T & 0x0001)
					ADDFLAG(SystemState->P_Errors, P_ERR_PHASE_CURRENT);
				else
					REMOVEFLAG(SystemState->P_Errors, P_ERR_PHASE_CURRENT);

				if (SystemState->Measured.Outdoor.faultFlag101T & 0x000C)
					ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_DC_VOLTAGE);
				else
					REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_DC_VOLTAGE);

				if ((SystemState->Measured.Outdoor.faultFlag101T & 0x0040)) {
					ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT);
				} else
					REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_IPM_TEMP_LIMIT);

				if (SystemState->Measured.Outdoor.faultFlag101T & 0x2000) {
					ADDFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_COMS);
				} else
					REMOVEFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_COMS);
			}

			HAL_UART_Receive_DMA(&huart2, (uint8_t*) appOutdoordata.RawRxMessage, OUTDOORBUFFSIZE); //Start new recive
		} //There is RX Avalible

		if (!CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_SIM)) {
			//if SIM BIT not SET
			SystemState->LastRunTime.xLastWakeTime_OutDoorRx = xTaskGetTickCount();
			REMOVEFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR);
			SystemState->Measured.Outdoor.motorSpd101T = (int) (SystemState->Current.Compressor.Freq * 0.95) * 100.0;
			SystemState->Measured.Outdoor.motorState101T = (SystemState->Current.Compressor.Freq > 0 ? 4 : 0);
			SystemState->Measured.Outdoor.vdcValue101T = (int) 0;
			SystemState->Measured.Outdoor.tempIpm101T = (int) 3200;
			SystemState->Measured.Outdoor.idFilt101T = (int) 0;
			SystemState->Measured.Outdoor.iqFilt101T = (int) 0;
			SystemState->Measured.Outdoor.vdFilt101T = (int) 0;
			SystemState->Measured.Outdoor.vqFilt101T = (int) 0;
			SystemState->Measured.Outdoor.gpioL101T = (int) 0;
		}

		if (xTaskGetTickCount() - SystemState->LastRunTime.xLastWakeTime_OutDoorRx > SECONDS(30)) {
			ADDFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR);
			//TxCommandType = 0;
			SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_OFF;
			SystemState->Current.Compressor.Freq = 0;
			SystemState->Measured.Outdoor.motorSpd101T = (int) 0;
			SystemState->Measured.Outdoor.motorState101T = 0;
			SystemState->Measured.Outdoor.vdcValue101T = (int) 0;
			SystemState->Measured.Outdoor.tempIpm101T = (int) 0;
			SystemState->Measured.Outdoor.idFilt101T = (int) 0;
			SystemState->Measured.Outdoor.iqFilt101T = (int) 0;
			SystemState->Measured.Outdoor.vdFilt101T = (int) 0;
			SystemState->Measured.Outdoor.vqFilt101T = (int) 0;
			SystemState->Measured.Outdoor.gpioL101T = (int) 0;
			SystemState->ExternalCurrent1 = (int) 0;
			SystemState->P_Errors = P_ERR_OK;
			sendClearFaultCmd();
			HAL_UART_Receive_DMA(&huart2, (uint8_t*) appOutdoordata.RawRxMessage, OUTDOORBUFFSIZE); //Start new recive
		}

		if (TxCommandType == 2) { //Set Speed
			//Now should to the Compressor turn of turn on state control cool down etc.
			uint16_t RequiredSpeed = SystemState->Required.Compressor.Freq;

			if ((SystemState->Measured.Outdoor.CompressorState == COMPRESSOR_OFF) || (SystemState->Measured.Outdoor.CompressorState == COMPRESSOR_COOLDOWN)) {
				if (firstboot) {
					SystemState->Measured.Outdoor.CompressorOffTime = 3 * 60 + 1; // Trick 3min flag
					if (xTaskGetTickCount() / 1000 > 3 * 60)
						firstboot = false;
				} else
					SystemState->Measured.Outdoor.CompressorOffTime = (xTaskGetTickCount() - SystemState->LastRunTime.xLastCompressorTurnOff) / 1000.0;

				SystemState->Measured.Outdoor.CompressorRunTime = 0;
			} else {
				if (appOutdoordata.FaultCnt == 0) {
					SystemState->Measured.Outdoor.CompressorRunTime = (xTaskGetTickCount() - SystemState->LastRunTime.xLastCompressorTurnOn) / 1000.0;
					SystemState->Measured.Outdoor.CompressorOffTime = 0;
				}
			}

			switch (SystemState->Measured.Outdoor.CompressorState) {
			case COMPRESSOR_OFF: {

				RequiredSpeed = 0;
				if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_EXPANTION)) {
					SystemState->Required.Steppers[STEPPER_EXPANTION].Position = DAMPER_CLOSE;
				}
				if (SystemState->Required.Compressor.Freq > 0) {
					if (SystemState->Measured.Outdoor.CompressorOffTime > 3 * 60) {			//3Min Cool off
						SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_WARMUP;
						SystemState->LastRunTime.xLastCompressorTurnOn = xTaskGetTickCount();
						SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 35;
					}
				}
				break;
			}
			case COMPRESSOR_WARMUP: {
				if (SystemState->Required.Compressor.Freq == 0) {
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_COOLDOWN;
					SystemState->LastRunTime.xLastCompressorTurnOff = xTaskGetTickCount();
					//RequiredSpeed = freqLevel[COOLDOWNLEVEL];
					RequiredSpeed = COMP_MIN_FREQ;
				}

				firstboot = false;

				if ((SystemState->Measured.Outdoor.CompressorRunTime >= 0) && (SystemState->Measured.Outdoor.CompressorRunTime <= 120)) {
					//F1
					//Slowly ramp freq up from 15 to Taget FREQ Slot
					RequiredSpeed = COMP_MIN_FREQ + SystemState->Measured.Outdoor.CompressorRunTime; //This should ramp the freq from 15 one per sec
					RequiredSpeed = (RequiredSpeed > WARMUP_1ST) ? WARMUP_1ST : RequiredSpeed; //This shold then cap at freq level 4
					SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 35;
					/*} else if ((SystemState->Measured.Outdoor.CompressorRunTime > 60) && (SystemState->Measured.Outdoor.CompressorRunTime <= 120)) {
					 //F2
					 RequiredSpeed = 30 + (SystemState->Measured.Outdoor.CompressorRunTime - 60); //This should ramp the freq from Freqleve 4 one per sec
					 RequiredSpeed = (RequiredSpeed > WARMUP_2ND) ? WARMUP_2ND : RequiredSpeed; //This shold then cap at freq level 6
					 */} else {
					//Been on for 2 minnutes Just got to what ever Frew we want
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_RUNNING;
				}
				//Now need to check how long we been running for
				break;
			}
			case COMPRESSOR_RUNNING: {
				if ((SystemState->Required.Compressor.Freq == 0) || (SystemState->Measured.Outdoor.ProctecionLevel == COMP_STOP)) {
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_COOLDOWN;
					SystemState->LastRunTime.xLastCompressorTurnOff = xTaskGetTickCount();
					//RequiredSpeed = freqLevel[COOLDOWNLEVEL];
					RequiredSpeed = COMP_MIN_FREQ;
				} else
					RequiredSpeed = SystemState->Required.Compressor.Freq;
				break;
			}
			case COMPRESSOR_COOLDOWN: {
				//RequiredSpeed = freqLevel[COOLDOWNLEVEL];
				RequiredSpeed = COMP_MIN_FREQ;
				//				if (SystemState->Measured.Outdoor.faultFlag101T != 0) { //Not sure why but seems this the how chinse decided COmpressor is OFF
				if (SystemState->Measured.Outdoor.CompressorOffTime >= 60 * 3) { //ForNnow will cool down for fixed two min and then check flags with real compressir
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_OFF;
					SystemState->LastRunTime.xLastCompressorTurnOff = xTaskGetTickCount();
					SystemState->CompressorRunCnt = COMPRUN_NEXT;
				}

				if ((SystemState->Required.Compressor.Freq > 0) && ((SystemState->Measured.Outdoor.ProctecionLevel != COMP_STOP))) {
					SystemState->Measured.Outdoor.CompressorState = COMPRESSOR_RUNNING;
					SystemState->LastRunTime.xLastCompressorTurnOn = xTaskGetTickCount();
					SystemState->Required.Steppers[STEPPER_EXPANTION].Position = 35;

				}
				break;
			}

			} //Switch the state

			if ((SystemState->Measured.Outdoor.motorState101T == 4) && (SystemState->Measured.Outdoor.motorSpd101T > 2000)) { //This logic needs beeefing up
				SystemState->CompressorRunCnt = COMPRUN_WORKING;
			}

			//Final check is all the protections
			if (RequiredSpeed > 0)
				SystemState->Current.Compressor.Freq = AdjustCompFreq(RequiredSpeed);
			else
				SystemState->Current.Compressor.Freq = 0;

			if (appOutdoordata.FaultCnt >= 50)
				SystemState->Current.Compressor.Freq = 0;

		} //WE planning on setting the Speed

		if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_EXPANTION)) {
			if (SystemState->Measured.Outdoor.CompressorState != COMPRESSOR_OFF) {
				//If the compressor anything but off need to adjust Expnation
				//Nedd to slow this doen to 20S
				if (ExpAdjCnt > (1000 / xFrequency) * 20) {
					AdjExpnation();
					ExpAdjCnt = 0;
				}
				ExpAdjCnt++;
			}
		}

		//Now we need to adjust the Outdoor fan and the Expantion valuve
		if (CHECKBIT(SystemState->CONTROLRUNBITS, CTLBIT_OUTDOOR_FAN)) {
			if ((SystemState->Measured.Outdoor.CompressorState != COMPRESSOR_OFF) || (SystemState->Measured.Outdoor.CompressorOffTime < 30)
					|| CHECKFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR)) {
				//If the compressor anything but off need to adjust OutdoorFan
				//Or if off for less the 30 seconds
				//or if there a coms err
				AdjOutDoorFanLinear();
			} else {
				SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed = 0;
			}
		}

		//Now TX Final Answer
		switch (TxCommandType) {
		case 0:
			if((xTaskGetTickCount() - SystemState->LastRunTime.xLastWakeTime_OutDoorRx > SECONDS(15))||(CHECKFLAG(SystemState->E_Errors, E_ERR_COMS_INDOOR_OUTDOOR)))
				sendClearFaultCmd();
			else
				sendReadStatusCmd(0x0000);

			break;
		case 1:
			if (SystemState->Current.Compressor.Freq > 0)
				sendWriteRegCmd(0x01, 120, 1);
			else
				sendWriteRegCmd(0x01, 120, 0);
			break;
		case 2:
			if(SystemState->UnitType == UNIT_ECOSAFE)
				sendSetTargetSpdCmd(SystemState->Current.Compressor.Freq * 64);
			else
				sendSetTargetSpdCmd(SystemState->Current.Compressor.Freq * 149);
			break;
		case 3:
			sendReadRegCmd(0x01, 137); //VDC
			break;
		case 4:
			sendReadRegCmd(0x4, 0x6c); //IPM_TEMP
			//sendReadRegCmd(0x01, 144); //IPM_TEMP
			break;
		case 5:
			sendReadRegCmd(0x04, 112); //GPIO
			break;
		case 6:
			sendReadRegCmd(0x04, 04); //INPUT VOLTAGE
			//sendReadRegCmd(0x01, 142);
			break;
		case 7:
			sendReadRegCmd(0x04, 02); //INPUT CURRENT
			//sendReadRegCmd(0x01, 156);
			break;
		case 8:
			sendReadRegCmd(0x01, 157); //VQ Unknown
			break;
		case 9:
			if (appOutdoordata.FaultCnt >= 1024) {
				sendClearFaultCmd();
				SystemState->OutdoorFaultCnt++;
			} else
				sendReadStatusCmd(0x0000);
			break;
		default:
			sendReadStatusCmd(0x0000);
			break;
		}
		TxCommandType = (TxCommandType >= 9) ? 0 : TxCommandType + 1;

		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_OutDoor, xFrequency);
	}
}
