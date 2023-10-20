/*
 * Macros.h
 *
 *  Created on: Jan 8, 2021
 *      Author: Richard
 */

#ifndef INC_MACROS_H_
#define INC_MACROS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <FreeRTOS.h>




#define VREF (2.496)
#define SECONDS(A) ((A)*1000)
#define MINUTES(A) ((A)*1000*60)


#define WARMUPLEVEL1 4
#define WARMUPLEVEL2 6
#define COOLDOWNLEVEL 1

#define T25 298.15
#define T25_1 1/T25
#define KELVIN 273.15

#define MAX(A,B) ((A>B)?A:B)
#define MIN(A,B) ((A<B)?A:B)

#define COMP_MIN_FREQ (Comp_Params[SystemState->CompressorSize][0])
//#define COMP_MAX_FREQ (Comp_Params[SystemState->CompressorSize][1])
#define COMP_MAX_FREQ (SystemState->Paramters.PARAM[PARAM_COOLING_COMP_MAX])
#define COMP_RANGE (COMP_MAX_FREQ-COMP_MIN_FREQ)
#define COMP_MID (((COMP_MAX_FREQ-COMP_MIN_FREQ)/2)+COMP_MIN_FREQ)

#define HALF_DAMPPER ((DAMPER_OPEN-DAMPER_CLOSE)/2)

#define COMPSCALE_7 (0.5)
#define COMPSCALE_12 (0.6)
#define COMPSCALE_18 (0.7)

#define INDOOR_WATT_SCALE (0.1)
#define OUTDOOR_WATT_SCALE (0.3)
#define COMP_WATT_SCALE (SystemState->CompressorSize == COMPTYPE_7KW ? COMPSCALE_7 : SystemState->CompressorSize == COMPTYPE_12KW ? COMPSCALE_12 : COMPSCALE_18)

typedef struct
{
	uint32_t Band1; //0-20
	uint32_t Band2; //20-40
	uint32_t Band3; //40-60
	uint32_t Band4; //60-80
	uint32_t Band5; //80-100
}TimeInBand;

typedef struct
{
	uint32_t Powered; //2
	uint32_t _TotalMin; //4
	uint32_t _TotalMin_200; //6
	uint32_t _TotalMin_48; //8
	uint32_t _TotalMin_Comp; //10
	uint32_t _TotalMin_FreeAir; //12
	uint32_t _TotalMin_SuperEco; //14
	uint32_t _TotalMin_ForcedEco; //16
	TimeInBand _CompBands;//26
	TimeInBand _SuppyFanBands;//36
	TimeInBand _OutdoorFanBands;//46
	TimeInBand _CurrentBands;//56
	TimeInBand _OutDoorTempBands;//66
	TimeInBand _F8Bands;//76
	uint16_t Save;
}_AllTimeStats;//78

typedef struct
{
	uint16_t Comp;
	uint16_t OA;
	uint16_t F8;
	uint16_t SA_Fan;
	uint16_t O_Fan;
	uint16_t Current;
}HEALTH_MONITORS;

typedef struct
{
	float Comp;
	float OA;
	float F8;
	float SA_Fan;
	float O_Fan;
	float Current;
}HEALTH_MONITORS_F;

typedef struct
{
	float ReturnAir;//AD1
	float SupplyAir;//AD2
	float Room;//AD3
	float IndoorCoil;//AD4
	float CompDischarge;//AD5
	float Suction;//AD6
	float OutdoorCoil;//AD7
	float OutsideAir;//AD8
}Temps_F;


typedef struct
{
	int16_t ReturnAir; // In DegC *100
	int16_t SupplyAir;
	int16_t Room;
	int16_t IndoorCoil;
	int16_t CompDischarge;
	int16_t Suction;
	int16_t OutdoorCoil;
	int16_t OutsideAir;
}Temps_Int;

typedef enum
{
	DAMPER_CLOSE=0x000,
	DAMPER_OPEN=0x0400,
	DAMPER_BUSY=0xFFFE,
	DAMPER_UNKNOWN=0xFFFF
}DAMPER_STATE;

typedef enum
{
	DAMPER_CLOCKWISE,
	DAMPER_COUNTER_CLOCKWISE
}DAMPER_DIRECTION;

typedef enum
{
	STEPPER_RETRUN,
	STEPPER_FRESH,
	STEPPER_EXPANTION
}STEPPER_PORTS;

typedef enum
{
	FAN_PORT_INDOOR,
	FAN_PORT_OUTDOOR,
	FAN_PORT_SPARE
}FAN_PORTS;

 typedef enum
 {
	 NOTHINGPRESSED,
	 NONEWKEY,
	 SHORTPRESS,
	 LONGPRESS,
	 REALLONGPRESS
}KEYPRESS_TYPE;

typedef enum
{
	DISPALYMODE_DISPLAY,
	DISPALYMODE_SELECTPARM,
	DISPALYMODE_SELECTVALUE,
	DISPALYMODE_SECRETSETUP,
	DISPALYMODE_SUMMARY,
	DISPALYMODE_SELECT_VENDOR,
	DISPALYMODE_SELECT_UNIT,
	DISPALYMODE_SELECT_COMP,
	DISPALYMODE_ID_PG0,
	DISPALYMODE_ID_PG1,
	DISPALYMODE_ID_PG2,
	DISPALYMODE_ID_PG3,
	DISPALYMODE_ID_PG4,
	DISPALYMODE_ID_PG5,
	DISPALYMODE_LEFTOVER,
}DISPLAYMODE_TYPE;

typedef enum
{
	 MODE_ECO =0x0000,
	 MODE_COOLING=0x0001,
	 MODE_SUPER_ECO=0x0002,
	 MODE_FORCED_ECO=0x0003,
	 MODE_FOLLOW_EC0=0x0004,
	 MODE_FOLLOW_COOLING=0x0005,
	 MODE_FULL=0xFFFF
}COOLING_MODE;

typedef enum
{
	 MODE_NORMAL =0x0000,
	 MODE_PWRSAVE=0x0001,
	 MODE_FULL1=0xFFFF
}POWER_MODE;

typedef enum
{
	REALY_FAN,
	REALY_DAMPPER,
	REALY_FULL,
};

typedef struct
{
	portTickType xLastWakeTime_System;
	portTickType xLastWakeTime_Err;
	portTickType xLastWakeTime_ADC;
	portTickType xLastWakeTime_Damper;
	portTickType xLastWakeTime_OutDoor;
	portTickType xLastWakeTime_OutDoorRx;
	portTickType xLastWakeTime_Display;
	portTickType xLastWakeTime_Fan;
	portTickType xLastWakeTime_ModBus;
	portTickType xLastWakeTime_EXTI[4];
	portTickType xLastWakeTime_Health;
	portTickType xLastCompressorTurnOn;
	portTickType xLastCompressorTurnOff;
	portTickType xLastModeChange;
	portTickType xLastPwrModeChange;
	portTickType xLastModePassedCheck;
	portTickType xLastPwrPassedCheck;
	portTickType xLastValidModbusWrite;
	portTickType xLastValidModbusRead;

	portTickType xLastRemoteStateChange;
	portTickType xLastTimeParamNot100;
	portTickType xLastTimeParamNot0;
	portTickType xLastTimeDampperDrive;
}LAST_RUNTIMES;

typedef struct
{
	uint16_t Speed; // In percent *100
}FAN_INFO;

typedef struct
{
	uint16_t Freq; // In percent *100
}COMPESSOR_INFO;

typedef struct
{
	DAMPER_STATE Position; //Between 0 and 250
}STEPPER_INFO;

typedef enum
{
	PARAM_ECO_COOLING_CHANGEOVER=0x00,
	PARAM_COOLING_COMP_MAX=0x00,
	PARAM_COOLING_SETPOINT=0x01,
	PARAM_PWRSAVE_SETPOINT=0x02,
	PARAM_FANMAX_COOLING=0x03,
	PARAM_FANMIN_ECO=0x04,
	PARAM_FANMIN_COOLING=0x05,
	PARAM_SENSOR=0x06,
	PARAM_HASSLAVE=0x07,
	PARAM_MAX_CHANGEOVER_DELTA=0x08,
	PARAM_SAFE_SETPOINT=0x09,			//
	PARAM_REALY1_USE=0x0A,		//
	PARAM_STARTTIMEOUT=0x0B,			//PID Used to adjust the SET point when in cooling
	PARAM_HIGH_TEMP=0x0C,				//HighTemp Alarm
	PARAM_ROTATE_TIME=0x0D,
	PARAM_USE_PID=0x0E,
	PARAM_FANMAX_ECO=0X0F
}PARAM;

#define MAXPARAM 6

typedef enum
{
	MASTER_READ_SLAVE,
	MASTER_BOOT_SLAVE,
	MASTER_STOP_SLAVE,
	MASTER_WARMUP_SLAVE,
	MASTER_FOLLOWME,
	MODBUS_FULL=0xffff
}MOD_STATE;

typedef enum
{
	VENDOR_VODACOM,
	VENDOR_MTN,
	VENDOR_OTHER,
	VENDOR_FULL = 0xFFFF
}VENDOR_CODES;

typedef enum
{
	UNIT_CUBE,
	UNIT_RACK,
	UNIT_SLIM,
	UNIT_ECOSAFE,
	UNIT_FULL = 0xFFFF
}UNIT_CODES;

typedef enum
{
	COMPTYPE_7KW,
	COMPTYPE_12KW,
	COMPTYPE_18Kw,
	COMPTYPE_7KW_220ONLY,
	COMPTYPE_12KW_220ONLY,
	COMPTYPE_18Kw_220ONLY,
	COMPTYPE_FULL = 0xFFFF
}COMPTYPE_CODES;


typedef enum
{
	COMPRUN_NEVER,
	COMPRUN_FIRST,
	COMPRUN_NEXT,
	COMPRUN_WORKING,
}COMP_RUNCNT;


typedef enum
{
	//E
	E_ERR_OK		=0,
	E_ERR_EEPROM_INDOOR			=(1<<0),
	E_ERR_EEPROM_OUTDOOR		=(1<<1),
	E_ERR_IPM_MODULE			=(1<<2),
	E_ERR_COMS_INDOOR_OUTDOOR	=(1<<3),
	E_ERR_INDOOR_FAN			=(1<<4),
	E_ERR_OUTDOOR_EC_FAN		=(1<<5),
	E_ERR_REMOTECOMS_FAIL		=(1<<6),
	E_ERR_REMOTECOMS_2MASTER	=(1<<7),
	E_ERR_MAX		=(1<<15)
}E_ERR_CODES;

typedef enum
{
	//L
	L_ERR_OK		=0,
	L_ERR_INDOOR_RUTURN_AIR		=(1<<0),
	L_ERR_INDOOR_ROOM			=(1<<1),
	L_ERR_OUTDOOR_AMBIENT		=(1<<2),
	L_ERR_OUTDOOR_DISCHARGE		=(1<<3),
	L_ERR_OUTDOOR_SUCTION		=(1<<4),
	L_ERR_OUTDOOR_COIL			=(1<<5),
	L_ERR_INDOOR_SUPPLY_AIR		=(1<<6),
	L_ERR_INDOOR_COIL			=(1<<7),
	L_ERR_INDOOR_BOTH_SENSORS	=(1<<8),
	L_ERR_MAX					=(1<<15)
}L_ERR_CODES;

typedef enum
{
	//P
	P_ERR_OK		=0,
	P_ERR_OUTDOOR_CURRENT				=(1<<0),
	P_ERR_PHASE_CURRENT					=(1<<1),
	P_ERR_OUTDOOR_AC_VOLTAGE			=(1<<2),
	P_ERR_OUTDOOR_DC_VOLTAGE			=(1<<3),
	P_ERR_OUTDOOR_AC_VOLTAGE_LIMIT		=(1<<4),
	P_ERR_OUTDOOR_AC_CURRENT_LIMIT		=(1<<5),
	P_ERR_OUTDOOR_PHASE_LIMIT			=(1<<6),
	P_ERR_OUTDOOR_IPM_TEMP_LIMIT		=(1<<7),
	P_ERR_OUTDOOR_DISCHARGE_TEMP_LIMIT	=(1<<8),
	P_ERR_OUTDOOR_COIL_TEMP_LIMIT		=(1<<9),
	P_ERR_OUTDOOR_INDOOR_COIL_LIMIT		=(1<<10),
	P_ERR_OUTDOOR_AMBIENT_LIMIT			=(1<<11),
	P_ERR_OUTDOOR_COMS					=(1<<12),
	P_ERR_MAX							=(1<<15)
}P_ERR_CODES;

typedef enum
{
//F
	F_ERR_OK		=0,
	F_ERR_LACK_ABILITY		=(1<<0),
	F_ERR_INDOOR_ROOM_LOW	=(1<<1),
	F_ERR_FILTER_DIRTY		=(1<<2),
	F_ERR_PHASE_MISSING		=(1<<3),
	F_ERR_STEP_MISSING		=(1<<4),
	F_ERR_MAX		=(1<<15)
}F_ERR_CODES;


typedef enum
{
//C
	C_ERR_OK		=0,
	C_ERR_ANOTHERDAY		=(1<<1),
	C_ERR_MAX		=(1<<15)
}C_ERR_CODES;

typedef enum
{
	ERR_CODES_OK			=(0),
	ERR_CODES_E				=(1<<0),
	ERR_CODES_L				=(1<<1),
	ERR_CODES_F				=(1<<2),
	ERR_CODES_P				=(1<<3),
	ERR_CODES_C				=(1<<4),
	ERR_CODES_MAX			=(1<<15)
}ERR_CODES;


typedef enum
{
	COMPRESSOR_OFF,
	COMPRESSOR_WARMUP,
	COMPRESSOR_RUNNING,
	COMPRESSOR_COOLDOWN
}COMPRESSOR_STATES;

typedef enum {
	COMP_UP,
	COMP_SLOWUP,
	COMP_HOLD,
	COMP_DOWN,
	COMP_RAPIDDOWN,
	COMP_STOP
} COMP_PROTECTINON_TYPE;

typedef struct
{
	uint16_t vdcValue101T;
	uint16_t tempIpm101T;
	uint16_t motorSpd101T;
	uint16_t motorState101T;
	uint16_t faultFlag101T;
	uint16_t idFilt101T;
	uint16_t iqFilt101T;
	uint16_t vdFilt101T;
	uint16_t vqFilt101T;
	uint16_t gpioL101T;
	uint16_t CompressorRunTime;
	uint16_t CompressorOffTime;
	COMPRESSOR_STATES CompressorState;
	uint8_t Sapre1;
	COMP_PROTECTINON_TYPE ProctecionLevel;
	uint8_t Sapre2;
}OUTDOOR_INFO;

typedef struct
{
	FAN_INFO Fans[3];
	STEPPER_INFO Steppers[3];//WE only have 3 Steppers but since the spostion is 8Bits we need 4 to Aline on the modbus table Might Migtare to 16 if sapce allows
	COMPESSOR_INFO Compressor;
	COOLING_MODE CoolingMode;
	POWER_MODE PowerMode;
	uint8_t Realys;
	uint8_t u8Spare;
}DEVICES;

typedef struct
{
	//FAN_INFO Fans[3];
	Temps_Int Tempratures;
	uint16_t F8;
	OUTDOOR_INFO Outdoor;
	FAN_INFO Fans[3];
	uint16_t TimeInMode;
	uint16_t TimeInPwrMode;
	uint16_t ModeDiffrentTime;
	uint16_t PwrDiffrentTime;
	uint16_t BatteryVoltage;
}MEASUREDDEVICES;

typedef struct
{
	uint16_t Digits[3];
	uint16_t KeyPressed;
	uint8_t LED_Power;
	uint8_t LED_Alarm;
	uint8_t KeySeq[5];
}DISPLAY_INFO;



typedef struct
{
	uint16_t PARAM[16];
}PARAM_INFO;


typedef struct {
	uint16_t data_avalible;

	char line[128];
	char *linepos;
	uint8_t rx_byte;

} APP_GPSdata_T;

#endif /* INC_MACROS_H_ */
