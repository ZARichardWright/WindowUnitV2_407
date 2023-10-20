/*
 * appOutdoorComs.h
 *
 *  Created on: Jan 9, 2021
 *      Author: Richard
 */

#ifndef INC_APPOUTDOORCOMS_H_
#define INC_APPOUTDOORCOMS_H_

#define OUTDOORBUFFSIZE 16

#define NODE_ID 0x01

typedef enum{
    readStatus = 0x00,
	  clearFault = 0x01,
	  controlInputMode = 0x02,
	  motorControl_setTargetSpeed = 0x03,
	  registerRead = 0x05,
	  registerWriter = 0x06,
	  loadSaveParameter = 0x20,
}uart_command;

typedef struct{
	uint8_t Byte2; //Byte 2
	uint8_t Byte3; //Byte 3
	uint8_t Byte4; //Byte 4
	uint8_t Byte5; //Byte 5
}msgtype_raw;

typedef struct{
	uint16_t MotorState;
	uint16_t MotorSpeed;
}msgtype_3;

typedef struct{
	uint8_t Reserved;
	uint8_t RegCode;
 	uint16_t Value;
}msgtype_5;

typedef struct{
	uint8_t Status;
	uint8_t Reserved;
 	uint16_t FaultFlag;
}msgtype_0;

typedef struct{
	uint8_t node_id;
	uint8_t MsgType;
	union{
		uint8_t Bytes[4];
		msgtype_raw RawBytes;
		msgtype_0 StatusMsg;
		msgtype_3 SpeedMsg;
		msgtype_5 RegMsg;
	}Msg;
	uint16_t crc;

}imc101_RxMsg;

typedef struct{
	uint8_t RawRxMessage[OUTDOORBUFFSIZE];
	uint8_t RawTxMessage[OUTDOORBUFFSIZE];
	uint8_t data_avalible;
	imc101_RxMsg RXMsg;

	float vdcValue101T;
	float tempIpm101T;
	float motorSpd101T;
	float idFilt101T; //Input Current from Outdoor
	float iqFilt101T;
	float vdFilt101T; //Input Current from Outdoor
	float vqFilt101T;
	uint16_t gpioL101T;

	portTickType LastErrCng;
	uint16_t motorState101T;
	uint16_t faultFlag101T;
	uint16_t FaultCnt;
}APP_OUTDOOR_DATA;

extern APP_OUTDOOR_DATA appOutdoordata;
#endif /* INC_APPOUTDOORCOMS_H_ */
