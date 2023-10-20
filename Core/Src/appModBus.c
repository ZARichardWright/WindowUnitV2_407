/*
 * appModBus.c
 *
 *  Created on: Jan 9, 2021
 *      Author: Richard
 */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "Modbus.h"
#include "appSystem.h"

extern const uint8_t Comp_Params[6][2];
modbusHandler_t *mHandlers[MAX_M_HANDLERS];
extern APP_SYSTEM_DATA *SystemState;
//extern OTHERGUY_STATE *OtherGuy;

modbus_t telegram[3];
extern modbusHandler_t ModbusH_1;

uint16_t Tempmem[4][30] = { };

void EnableSlave() {
	ModbusH_1.uModbusType = MB_SLAVE;
	ModbusH_1.u8id = 240;
	ModbusH_1.u16regs = (uint16_t*) SystemState;
	ModbusH_1.i8state = COM_IDLE;
	xTaskNotify(ModbusH_1.myTaskModbusAHandle, ERR_NOT_MASTER, eSetValueWithOverwrite);

}

void EnableMaster() {
	//xTaskNotify(ModbusH_1.myTaskModbusAHandle, 0, eNoAction);
	ModbusH_1.uModbusType = MB_MASTER;
	ModbusH_1.u8id = 0x00;
	ModbusH_1.u16regs = (uint16_t*) SystemState;
	ModbusH_1.i8state = COM_IDLE;
	xTaskNotify(ModbusH_1.myTaskModbusAHandle, 0, eSetValueWithOverwrite);
}

void StartModbusTask(void *argument) {

	int32_t u32NotificationValue;
	MOD_STATE MState = MASTER_READ_SLAVE;
	uint8_t Toggle;
	uint8_t FailedWrite = 0;
	uint8_t JustBooted = 0;
	portTickType xReadInterval = 10000;
	portTickType xWait = 3000;
	portTickType xOffset = 0;
	portTickType LastT = 0;

	telegram[0].u8id = 240; // slave address
	telegram[0].u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers write)
	//telegram[0].u16RegAdd = ((char*)&SystemState->Paramters - (char*)&SystemState->VERSION)/2;
	telegram[0].u16RegAdd = 0;
	telegram[0].u16CoilsNo = 78; //Parameters to Run Flag
	telegram[0].u16reg = (uint16_t*) &SystemState->OtherGuyMem; // pointer to a memory array in the Arduino

	telegram[1].u8id = 240; // slave address
	telegram[1].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers write)
	telegram[1].u16RegAdd = ((char*) &SystemState->PoweredUp - (char*) &SystemState->VERSION) / 2;
	telegram[1].u16CoilsNo = 1;
	telegram[1].u16reg = Tempmem[1]; // pointer to a memory array in the Arduino

	//This is Safe Unit Write
	telegram[2].u8id = 1; // slave address
	telegram[2].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers write)
	telegram[2].u16RegAdd = 11;
	telegram[2].u16CoilsNo = 2;
	telegram[2].u16reg = Tempmem[2]; // pointer to a memory array in the Arduino

	//Write the Follow me
	telegram[3].u8id = 240; // slave address
	telegram[3].u8fct = MB_FC_WRITE_REGISTER; // function code (this one is registers write)
	telegram[3].u16RegAdd = 1;
	telegram[3].u16CoilsNo = 9;
	telegram[3].u16reg = Tempmem[3]; // pointer to a memory array in the Arduino
	//while(SystemState->UpTime_Sec<60)

	vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, MINUTES(2.4));
	//vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, MINUTES(0.5));
	JustBooted = 0;

	if (SystemState->Paramters.PARAM[PARAM_HASSLAVE] == 0) {
		EnableSlave();
	} else {
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 750);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 250);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 250);
		HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
	}

	for (;;) {

		//SystemState->UpTime_Sec += ((xTaskGetTickCount() - LastT) / 1000.0);

		if (SystemState->UnitType == UNIT_ECOSAFE) {

			if (Toggle > 10) {
				telegram[2].u16RegAdd = 0;
				telegram[2].u16CoilsNo = 1;
				Tempmem[2][0] = SystemState->Paramters.PARAM[PARAM_SAFE_SETPOINT] * 100;
				EnableMaster();
				ModbusQuery(&ModbusH_1, telegram[2]); // make a query
				Toggle = 0;
				u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
				vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 500);
			}

			telegram[2].u16RegAdd = 11;
			telegram[2].u16CoilsNo = 1;
			Tempmem[2][0] = (SystemState->Measured.Tempratures.OutsideAir < -5000) ? 0 : SystemState->Measured.Tempratures.OutsideAir;
			EnableMaster();
			ModbusQuery(&ModbusH_1, telegram[2]); // make a query
			u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 500);
			telegram[2].u16RegAdd = 12;
			Tempmem[2][0] = (SystemState->Measured.Tempratures.Room < -5000) ? 0 : SystemState->Measured.Tempratures.Room;
			EnableMaster();
			ModbusQuery(&ModbusH_1, telegram[2]); // make a query
			u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 500);
			Toggle++;

			EnableSlave();
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 10000);
			continue;
		} else {
			LastT = xTaskGetTickCount();
			if ((!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_MODBUS_TASK)) | (SystemState->Paramters.PARAM[PARAM_HASSLAVE] == 0)) {
				vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, 10000);
				EnableSlave();
				continue;
			}
		}

		//Keep trying to read from the other guy till we get a responce
		//ModbusH_1.uModbusType = MB_MASTER;
		//ModbusH_1.u8id = 0;

#define ALLOWMASTER

#ifdef ALLOWMASTER
		EnableMaster();

		if (Toggle) {
			ModbusQuery(&ModbusH_1, telegram[0]); // make a query
		} else {
			switch (SystemState->MasterState) {
			case MASTER_READ_SLAVE:
			case MASTER_WARMUP_SLAVE: {
				ModbusQuery(&ModbusH_1, telegram[0]); // make a query
				break;
			}
			case MASTER_BOOT_SLAVE: {
				Tempmem[1][0] = 1; //Boot
				ModbusQuery(&ModbusH_1, telegram[1]); // make a query
				break;
			}

			case MASTER_STOP_SLAVE: {
				if (SystemState->PoweredUp == 1) {
					Tempmem[1][0] = 0; //Shutdown
					ModbusQuery(&ModbusH_1, telegram[1]); // make a query
				} else {
					//I have been turend off by other guy so do not want to turn him off just read
					ModbusQuery(&ModbusH_1, telegram[0]); // make a query
					SystemState->MasterState = MASTER_READ_SLAVE;
				}
				break;
			}
			case MASTER_FOLLOWME: {
				//Build Message
				Tempmem[3][0] = 0xFF7F; //Disbale Control Bits
				Tempmem[3][1] = SystemState->Following_Required.Fans[0].Speed;
				Tempmem[3][2] = SystemState->Following_Required.Fans[1].Speed;
				Tempmem[3][3] = SystemState->Following_Required.Fans[2].Speed;
				Tempmem[3][4] = SystemState->Following_Required.Steppers[0].Position;
				Tempmem[3][5] = SystemState->Following_Required.Steppers[1].Position;
				Tempmem[3][6] = SystemState->Following_Required.Steppers[2].Position;
				Tempmem[3][7] = SystemState->Following_Required.Compressor.Freq;
				Tempmem[3][8] = SystemState->Following_Required.CoolingMode;
				ModbusQuery(&ModbusH_1, telegram[3]); // make a query
				break;
			}
			default:
				SystemState->MasterState = MASTER_READ_SLAVE;
			} //Switch

		}
		Toggle = Toggle == 0 ? 1 : 0;
		//Toggle=1;

		u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes

		SystemState->LastRunTime.xLastValidModbusWrite = xTaskGetTickCount();

		if (u32NotificationValue) {
			//NO responce or salved responed bady
			FailedWrite = (FailedWrite >= 200) ? 200 : FailedWrite + 1;
			xOffset = FailedWrite * 100;

			if (FailedWrite > 10) {
				ADDFLAG(SystemState->E_Errors, E_ERR_REMOTECOMS_FAIL);
				SystemState->PoweredUp = 1;
				SystemState->MasterState = MASTER_WARMUP_SLAVE;
			}

			if (HAL_UART_GetState(ModbusH_1.port) != HAL_UART_STATE_BUSY_RX)
				HAL_UART_Receive_IT(ModbusH_1.port, &ModbusH_1.dataRX, 1);
			//Assume no slave
		} else {
			//Assume there is a salve and now wait 1/2 the time to re try
			//Got aValid answer
			FailedWrite = 0;
			xOffset = 0;
			if (SystemState->OtherGuyMem.Paramters.PARAM[PARAM_HASSLAVE] >= 1)
				REMOVEFLAG(SystemState->E_Errors, E_ERR_REMOTECOMS_FAIL);
			else
				ADDFLAG(SystemState->E_Errors, E_ERR_REMOTECOMS_FAIL);

			if (JustBooted <= 10) {
				JustBooted++;
				if (JustBooted >= 5) {
					//Same Auto Gues//Fight for master control
					if ((SystemState->OtherGuyMem.PoweredUp == 1) && (SystemState->PoweredUp == 1)) {
						SystemState->MasterState = MASTER_STOP_SLAVE;
					}

				}
				if (JustBooted >= 8) {
					SystemState->MasterState = MASTER_READ_SLAVE;
				}
			} else {
				//So only if there no errors and we were just reading calmly then can we go to Follow mode
				//and if we powered
				if ((SystemState->C_Errors == 0)&&(SystemState->MasterState==MASTER_READ_SLAVE)) {
					if (SystemState->Paramters.PARAM[PARAM_HASSLAVE] == 2) {
						if(SystemState->PoweredUp == 1)
							SystemState->MasterState = MASTER_FOLLOWME;
					}
				}
			}

			if (!Toggle) {
				//Avoid changinf states the whole time
				if ((SystemState->MasterState == MASTER_BOOT_SLAVE) && (SystemState->MasterState != MASTER_WARMUP_SLAVE)) {
					if (SystemState->OtherGuyMem.PoweredUp == 1) {
						SystemState->MasterState = MASTER_WARMUP_SLAVE;
						SystemState->LastRunTime.xLastRemoteStateChange = xTaskGetTickCount();
					}
				} else if (SystemState->MasterState == MASTER_STOP_SLAVE) {
					//Should check if salve stoped or can just assume for now it will
					SystemState->LastRunTime.xLastRemoteStateChange = xTaskGetTickCount();
					if (SystemState->OtherGuyMem.PoweredUp == 0)
						SystemState->MasterState = MASTER_READ_SLAVE;
				}  else if (SystemState->MasterState != MASTER_WARMUP_SLAVE)
					SystemState->MasterState = MASTER_READ_SLAVE;
			}
		}

		//ModbusH_1.uModbusType = MB_SLAVE;

		//Now figure out if i can turn off
		//If i am in MASTER_WARMUP_SLAVE then i know that once he warm i can turn off
		if ((SystemState->MasterState == MASTER_WARMUP_SLAVE) && (!CHECKFLAG(SystemState->E_Errors, E_ERR_REMOTECOMS_FAIL))) {
			if (xTaskGetTickCount() - SystemState->LastRunTime.xLastRemoteStateChange > MINUTES(2)) {
				if ((SystemState->OtherGuyMem.Errors == 0x00) && (SystemState->OtherGuyMem.PoweredUp == 1)) {
					if (SystemState->RotateReason == ROTATE_REASON_HIGHTEMP) {
						if ((SystemState->Required.Compressor.Freq < COMP_MID) && (SystemState->OtherGuyMem.Required.Compressor.Freq < COMP_MID))
							SystemState->PoweredUp = 0;
					} else
						//If timmer then just tuen off
						SystemState->PoweredUp = 0;

					if (SystemState->Paramters.PARAM[PARAM_REALY1_USE] == REALY_FAN)
						CLEARBIT(SystemState->Required.Realys, 2);
				} else {
					//Still got Errors
					//And have relay fan
					if (SystemState->Paramters.PARAM[PARAM_REALY1_USE] == REALY_FAN)
						SETBIT(SystemState->Required.Realys, 2);
				}
			} else if (xTaskGetTickCount() - SystemState->LastRunTime.xLastRemoteStateChange > MINUTES(1.6)) {

			} else if (xTaskGetTickCount() - SystemState->LastRunTime.xLastRemoteStateChange > MINUTES(1)) {
				//Check slave is on else requist anohter boot.
				if (SystemState->OtherGuyMem.PoweredUp != 1) {
					SystemState->MasterState = MASTER_BOOT_SLAVE;
					SystemState->LastRunTime.xLastRemoteStateChange = xTaskGetTickCount();
				}
			}
		}

		//This will reset my Master state to read if i power off
		if ((SystemState->PoweredUp == 0) && ((SystemState->MasterState == MASTER_WARMUP_SLAVE) || (SystemState->MasterState == MASTER_FOLLOWME))) {
			SystemState->MasterState = MASTER_READ_SLAVE;
		}
		if(SystemState->PoweredUp == 1)
		{
			//If i am powerd and and the control Bits not set then set tyhem
			if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_SYSTEM_TASK))
			{
				SET_BIT(SystemState->TASKRUNBITS, RUNBIT_SYSTEM_TASK);
			}
		}

#endif
		EnableSlave();

		if (SystemState->LastRunTime.xLastValidModbusRead == 0) { //Still do not know about the other guys so try randomly poll him
			xWait = (xWait <= 1000) ? 10000 : xWait - 1000;
		} else {
			//We know of the other guy so now poll him back fixed time after

			xWait = xReadInterval;
			int32_t close = (int32_t) (SystemState->LastRunTime.xLastValidModbusWrite - SystemState->LastRunTime.xLastValidModbusRead);
			if ((close > xReadInterval / 4) || (close < xReadInterval / -4)) {
				//WE to close
				xWait -= xReadInterval / 4;
			}
			xWait = (xWait >= xReadInterval) ? xReadInterval : xWait;
		}

		//HAL_GPIO_TogglePin(LED_ERR_GPIO_Port, LED_RUN_Pin);
		//vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, xWait + xOffset);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_ModBus, xWait);
	}
}
