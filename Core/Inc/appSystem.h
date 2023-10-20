/*
 * appSystem.h
 *
 *  Created on: Jan 9, 2021
 *      Author: Richard
 */

#ifndef INC_APPSYSTEM_H_
#define INC_APPSYSTEM_H_

#include "Macros.h"
#include "api.h"
#include "ip4_addr.h"
#include "netif.h"
#include "dns.h"
#include "ip4.h"


#define BIT(x)	(1 <<(x))
#define SETBIT(A,B) (A|=(BIT(B)))
#define CLEARBIT(A,B) (A&=~(BIT(B)))

#define ADDFLAG(A,B) (A|=((B)))
#define REMOVEFLAG(A,B) (A&=~((B)))

#define CHECKFLAG(A,B) (((A&(B)) == (B)))
#define CHECKBIT(A,B) (((A&BIT(B)) == BIT(B)))

#define CALMPLOWER(A,LOWERLIMIT) ((A<LOWERLIMIT) ? LOWERLIMIT :A)
#define CALMPUPPER(A,UPPERLIMIT) ((A>UPPERLIMIT) ? UPPERLIMIT :A)
#define CLAMP(A,UPPERLIMIT,LOWERLIMIT) (CALMPUPPER(CALMPLOWER(A,LOWERLIMIT),UPPERLIMIT))
#define WRAP(A,UPPERLIMIT,LOWERLIMIT) ((A>=UPPERLIMIT) ? LOWERLIMIT:A+1)
#define WRAP_NEG(A,UPPERLIMIT,LOWERLIMIT) ((A<=LOWERLIMIT) ? UPPERLIMIT:A-1)

void StartSystemTask_Ext(void *argument);

#define ADDRESSOFREG(A) (&A - SystemState)
typedef enum
{
	PARAM_LOWWER_LIMIT,
	PARAM_DEFAULT,
	PARAM_UPPER_LIMIT
}PARAM_TABLE;

/*
typedef enum
{

}USER_PARAMS;

*/

typedef enum
{
	ROTATE_REASON_TIMMER,
	ROTATE_REASON_HIGHTEMP,
}ROTATE_REASON;

typedef enum
{
	RUNBIT_ADC_TASK,
	RUNBIT_DAMPER_TASK,
	RUNBIT_DISPLAY_TASK,
	RUNBIT_ERR_TASK,
	RUNBIT_FAN_TASK,
	RUNBIT_MODBUS_TASK,
	RUNBIT_OUTDOOR_TASK,
	RUNBIT_SYSTEM_TASK,
}TASK_RUN_BITS;

typedef enum
{
	CTLBIT_OUTDOOR_FAN,
	CTLBIT_INDOOR_FAN_CTL,
	CTLBIT_INDOOR_DAMPPER,
	CTLBIT_OUTDOOR_DAMPPER,
	CTLBIT_COMPRESSOR,
	CTLBIT_EXPANTION,
	CTLBIT_REALY,
	CTLBIT_SIM,
	CTLBIT_RESERVE
}CONTROL_RUN_BITS;


typedef struct
{
	uint16_t VERSION;
	uint8_t TASKRUNBITS;
	uint8_t CONTROLRUNBITS;
	DEVICES Required;
	DEVICES Current;
	MEASUREDDEVICES Measured;
	PARAM_INFO Paramters;
	uint16_t PowerCount;
	uint16_t CompressorRunCnt;
	ERR_CODES Errors;
	E_ERR_CODES E_Errors;
	L_ERR_CODES L_Errors;
	P_ERR_CODES P_Errors;
	F_ERR_CODES F_Errors;
	C_ERR_CODES C_Errors;
	uint16_t PoweredUp;
}OTHERGUY_STATE;



typedef struct
{
	uint16_t VERSION;
	uint8_t TASKRUNBITS;
	uint8_t CONTROLRUNBITS;
	DEVICES Required;
	DEVICES Current;
	MEASUREDDEVICES Measured;
	PARAM_INFO Paramters;
	uint16_t PowerCount;
	uint16_t CompressorRunCnt;
	ERR_CODES Errors;
	E_ERR_CODES E_Errors;
	L_ERR_CODES L_Errors;
	P_ERR_CODES P_Errors;
	F_ERR_CODES F_Errors;
	C_ERR_CODES C_Errors;
	uint16_t PoweredUp;
	VENDOR_CODES Vendor;
	UNIT_CODES UnitType;
	COMPTYPE_CODES CompressorSize;
	uint16_t P;
	uint16_t I;
	uint16_t D;
	uint16_t ErrroCode;
	uint16_t ErrroNum;
	MOD_STATE MasterState;
	uint16_t Roatte_RemainTime;
	uint64_t Unique_ID; //Skip 4 Address
	uint16_t RebootMe;
	uint16_t Spare16;
	uint32_t UpTime_Sec1;
	uint64_t Password;
	uint64_t InputedPassword;
	LAST_RUNTIMES LastRunTime;
	DISPLAY_INFO Display;
	HEALTH_MONITORS DailyHealth[7];
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	uint16_t SetDateTime;
	uint16_t ExternalCurrent1;
	uint16_t ExternalCurrent2;
	uint16_t DaysRun;
	uint32_t RunningHTotal;
	uint16_t CurrentHealth;
	uint16_t HealthScale[6];
	uint16_t Forced_TryCnt;
	_AllTimeStats Stats;		// +464Bytes modbus:232 :size 156bytes, 78Words
	OTHERGUY_STATE OtherGuyMem; // + 620 modbus :310
	int32_t Lon;				// +780 mdbus : 390
	int32_t Lat;				//+784 modbus :392
	uint32_t Alt;
	uint16_t SV;
	uint16_t ServerPort;		//Enable Server or Client //395
	struct ip4_addr ServerIP;	//Address of aircon.ddns.net once dns resoves
	uint16_t IP12;				//Static IP 1&2 //398
	uint16_t IP34;				//Static IP 3&4
	uint16_t GW12;				//Static IP 1&2
	uint16_t GW34;				//Static IP 3&4
	uint16_t NM12;				//Static IP 1&2
	uint16_t NM34;				//Static IP 3&4
	uint16_t WriteAdd;
	uint16_t WriteData;
	uint16_t WriteState;
	uint16_t IPSet;
	uint16_t DCCurrentLimit;
	uint16_t SoftwareTest;
	uint16_t HighTempAlarm;
	uint16_t CompressorRtryTime;
	uint16_t OutdoorFaultCnt;
	uint16_t RotateReason;
	uint64_t SerialNo;
	DEVICES Following_Required;
}APP_SYSTEM_DATA;


#endif /* INC_APPSYSTEM_H_ */
