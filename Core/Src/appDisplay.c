#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"
#include "bin.h"

extern APP_SYSTEM_DATA *SystemState;

#define LED_DIO_SET HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, 1)
#define LED_CLK_SET HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, 1)

#define LED_DIO_CLR HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, 0)
#define LED_CLK_CLR HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, 0)

#define READ_DIO HAL_GPIO_ReadPin(I2C_SDA_GPIO_Port, I2C_SDA_Pin)
#define READ_CLK HAL_GPIO_ReadPin(I2C_SCL_GPIO_Port, I2C_SCL_Pin)

//#define DIO_IN()  {	I2C_SDA_GPIO_Port->MODER &= ~(GPIO_MODER_MODER11);}
//#define DIO_OUT() {	 I2C_SDA_GPIO_Port->MODER |= GPIO_MODER_MODER11_0;}

#define DIO_IN()  	sdaPinToInput();
#define DIO_OUT() 	sdaPinToOut();

#define		NEW_LED     //ÈýÎ»Ò»ÌåLED

#ifndef NEW_LED
#define		ONOFF_KEY		0x06
#define		RES_KEY			0x07
#define		INC_KEY			0x03
#define		DEC_KEY			0x05
#define		SET_KEY			0x04
#else
#define		ONOFF_KEY		0x07
#define		RES_KEY			0x06
#define		INC_KEY			0x04
#define		DEC_KEY			0x05
#define		SET_KEY			0x03
#endif

//DISP_RAM[2]
#define		LED_D1_BIT		6
#define		LED_D2_BIT		5

#ifndef NEW_LED
// dp g f a-b c e d
const uint8_t DATA_7SEG[] = { B0011_1111, B0000_1100, B0101_1011, B0101_1101, B0110_1100,
B0111_0101, B0111_0111, B0001_1100, B0111_1111, B0111_1101,
B0111_1110, B1110_0111, B0100_0011, B1100_1111, B0111_0011, B0111_0010,  //a-f
		B0100_0000, B0111_1010, B0010_0011, B0101_1110, B0101_0111           //-,P,L
		};
#else  //NEW_LED
// dp F G E-D B C A
const uint8_t DATA_7SEG[] = { B0101_1111, B0000_0110, B0011_1101, B0010_1111, B0110_0110,  //0 1 2 3 4
		B0110_1011, B0111_1011, B0000_0111, B0111_1111, B0110_1111,  //5 6 7 8 9
		B0111_0111, B0111_1010, B0011_1000, B0011_1110, B0111_1001, B0111_0001,  //a-f
		B0010_0000, B0111_0101, B0101_1000, B0000_0000, B0101_1110, B0101_0111          //-,P,L,OFF,V,N
		};
#endif

#define	LED_A	10
#define	LED_b	11
#define	LED_c	12
#define	LED_d	13
#define	LED_E	14
#define	LED_F	15
#define	LED_G	16
#define LED_P	17
#define LED_L	18
#define LED_OFF	19
#define LED_V	20
#define LED_N	21

uint8_t DISP_RAM[4];

extern const uint8_t limit_para_tab[16][4];
extern const uint8_t DefaultVendor;
extern const uint8_t DefaultUnit;
extern const uint8_t DefaultComp;

extern void LoadDefaults(uint8_t IncIP,uint8_t Static);

const uint8_t Vendor_Disp[3] = {
LED_V,
LED_N, 0 };

const uint8_t Unit_Disp[3] = {
LED_c,
LED_A, 5 };

const uint8_t Comp_Disp[3] = { 7, 12, 18 };

const uint8_t Comp_Summary[3] = { 7, 2, 8 };

uint8_t UnitEdit = false;

void sdaPinToInput(void) {


	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = I2C_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStruct);
}

void sdaPinToOut(void) {

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = I2C_SDA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStruct);

}
/*
void Delay_us(unsigned char tt) {
	while (tt--)
		__NOP();
}
*/
void I2CStart(void) {

	LED_CLK_SET;  //BIT=1;
	LED_DIO_SET;  //BIT=1;
	vTaskDelay(2);
	LED_DIO_CLR;  //BIT=0;
}

void I2Cask(void) //1651 ??
{

	int AA = 0;
	LED_CLK_CLR;  //BIT=0;   //clk = 0;
	vTaskDelay(5); //?????????????5us,????ACK ??

	DIO_IN();

	while (READ_DIO) {
		AA++;
		if (AA > 50) {
			break;
		}
	};

	DIO_OUT();

	LED_DIO_CLR;  //BIT=0;

	LED_CLK_SET;  //BIT=1;   //clk = 1;
	vTaskDelay(2);
	LED_CLK_CLR;  //BIT=0;    //clk=0;

}

void I2CStop(void) {

	LED_CLK_SET;  //BIT=1;
	LED_DIO_CLR;  //BIT=0;
	vTaskDelay(2);
	LED_DIO_SET; //BIT=1;

}

void Send_Command(uint8_t dat) {

	int i;
	for (i = 0; i < 8; i++) {
		LED_CLK_CLR;  //BIT=0;
		if (dat & 0x01) {
			LED_DIO_SET;  //BIT=1;
		} else {
			LED_DIO_CLR;  //BIT=0;
		}
		vTaskDelay(1);
		dat >>= 1;
		LED_CLK_SET;  //BIT=1;
		vTaskDelay(1);
	}

}

unsigned char ReadKey(void) {

	unsigned char rekey = 0, i;
	I2CStart();
	Send_Command(0x46);
	I2Cask();

	LED_DIO_SET;  //BIT=1;     //dio=1;
	DIO_IN();
	for (i = 0; i < 8; i++) {
		LED_CLK_CLR;  //BIT=0;     //clk=0;
		rekey = rekey >> 1;
		vTaskDelay(5);
		LED_CLK_SET;  //BIT=1;     //clk=1;
		if (READ_DIO)    //(READ_DIO)
		{
			rekey = rekey | 0x80;
		}
		vTaskDelay(2);
	}
	I2Cask();
	I2CStop();
	return (rekey);

}

//tm1651
void Disp_Driver(void) {
	unsigned char i;

	I2CStart();
	Send_Command(0x40);
	I2Cask();
	I2CStop();

	I2CStart();
	Send_Command(0xc0);
	I2Cask();

	for (i = 0; i < 4; i++) {
		Send_Command(DISP_RAM[i]);
		I2Cask();
	}
	I2CStop();

	I2CStart();
	Send_Command(0x8A);
	I2Cask();
	I2CStop();
}

uint8_t FeatchValue(uint8_t idx) {

	uint16_t SetPnt;
	SetPnt = (
			SystemState->Current.PowerMode == MODE_NORMAL ?
					SystemState->Paramters.PARAM[PARAM_COOLING_SETPOINT] : SystemState->Paramters.PARAM[PARAM_PWRSAVE_SETPOINT]);

	switch (idx) {
	case 0:
		return (SystemState->Current.PowerMode == MODE_NORMAL) ? SystemState->Current.CoolingMode : SystemState->Current.CoolingMode + 2;
	case 1:
		return SetPnt;
	case 2:
		return SystemState->Measured.F8 / 100.0;
	case 3:
		return SystemState->Measured.Tempratures.OutsideAir / 100.0;
	case 4:
		return SystemState->Measured.Tempratures.SupplyAir / 100.0;
	case 5:
		return SystemState->Required.Compressor.Freq;
	case 6:
		return SystemState->Current.Compressor.Freq;
	case 7:
		return SystemState->Paramters.PARAM[PARAM_SENSOR];
	}
	return 0;
}

void StartDisplayTask(void *argument) {
	/* Infinite loop */

	uint8_t LastKeyPressed;
	portTickType KeyChangedTime = xTaskGetTickCount();
	portTickType LastInDisplayMode = xTaskGetTickCount();
	portTickType EditModeChange = xTaskGetTickCount();
	portTickType SettingButtonPress = xTaskGetTickCount();

	KEYPRESS_TYPE KeyPressType;
	DISPLAYMODE_TYPE CurrentDisplayMode;
	uint8_t ParamIndex = 0;
	uint8_t TmpParam = 0;
	uint8_t FlashCnt = 0;
	uint8_t IgnoreErrors = 0;
	uint8_t AllowedToDisp = MAXPARAM;
	uint8_t IDmsg[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint64_t quotient;
	uint8_t prev_user;
	uint8_t Allow_Edit = false;
	int temp;

	//CLEARBIT(SystemState->TASKRUNBITS, RUNBIT_DISPLAY_TASK); //Disabe Display
	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_DISPLAY_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Display, 10000);
			continue;
		}

		SystemState->Display.KeyPressed = ReadKey() & 0x0F;
		if (SystemState->Display.KeyPressed >= 0x08)
			SystemState->Display.KeyPressed = 0x0F;

#ifndef OLDEEPROM
		//Check if setting button pressed

		if ((HAL_GPIO_ReadPin(USER_SW_GPIO_Port, USER_SW_Pin) == 0)&&(SystemState->Spare16 >30)) {
			if (prev_user == 1)
				SettingButtonPress = xTaskGetTickCount();

			if (xTaskGetTickCount() - SettingButtonPress > SECONDS(10)) {
				//Button held for 10 SEC
				//Allow dfor setting editing
				//Allow_Edit=true;
				//EditModeChange = xTaskGetTickCount();
				LoadDefaults(true,true);

				SystemState->RebootMe =1;

			}
		}

		if((HAL_GPIO_ReadPin(USER_SW_GPIO_Port, USER_SW_Pin) == 1)&&(prev_user==0)&&(SystemState->Spare16 >30))
		{
			//Released
			if ((xTaskGetTickCount() - SettingButtonPress) > SECONDS(1)) {
				SystemState->RebootMe =1;

			}

		}
#endif

		prev_user = HAL_GPIO_ReadPin(USER_SW_GPIO_Port, USER_SW_Pin);
		//prev_user =0;

		//If key has changed then record time
		if (KeyPressType == NONEWKEY) {
			if ((xTaskGetTickCount() - KeyChangedTime > 5000)) //Long Press
			{
				KeyPressType = LONGPRESS;
				KeyChangedTime = xTaskGetTickCount();
			}
		} else if (KeyPressType >= SHORTPRESS) {
			KeyPressType = NONEWKEY;
		}

		if (Allow_Edit) {
			if (xTaskGetTickCount() - EditModeChange > SECONDS(120))
				Allow_Edit = false;
		}

		if (SystemState->Display.KeyPressed == 0x0F) {
			KeyPressType = NOTHINGPRESSED;
			if (LastKeyPressed != SystemState->Display.KeyPressed) {
				//KeyChangedTimeUp = xTaskGetTickCount();
				LastKeyPressed = SystemState->Display.KeyPressed; //Store last key pressed
			}
		} else {
			//Key pressed
			if (LastKeyPressed != SystemState->Display.KeyPressed) {
				KeyChangedTime = xTaskGetTickCount();
				KeyPressType = SHORTPRESS;
				LastKeyPressed = SystemState->Display.KeyPressed; //Store last key pressed

				SystemState->Display.KeySeq[4] = SystemState->Display.KeySeq[3];
				SystemState->Display.KeySeq[3] = SystemState->Display.KeySeq[2];
				SystemState->Display.KeySeq[2] = SystemState->Display.KeySeq[1];
				SystemState->Display.KeySeq[1] = SystemState->Display.KeySeq[0];
				SystemState->Display.KeySeq[0] = SystemState->Display.KeyPressed;

				if ((SystemState->Display.KeySeq[0] == RES_KEY) && (SystemState->Display.KeySeq[1] == SET_KEY) && (SystemState->Display.KeySeq[2] == SET_KEY)
						&& (SystemState->Display.KeySeq[3] == RES_KEY)) {
					Allow_Edit = true;
				}
				//Make small beep
				HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 1);
				vTaskDelay(250);
				HAL_GPIO_WritePin(BELL_GPIO_Port, BELL_Pin, 0);
				if (Allow_Edit == true) {
					EditModeChange = xTaskGetTickCount();
				}
			}
		}

		if (SystemState->Current.Compressor.Freq > 0) {
			SystemState->Display.LED_Power = 1;
			if ((SystemState->Measured.Outdoor.ProctecionLevel >= COMP_HOLD) || (CHECKFLAG(SystemState->P_Errors, P_ERR_OUTDOOR_AMBIENT_LIMIT))) {
				if ((FlashCnt % 2 == 0)) {
					SystemState->Display.LED_Power = 0;
				}
			}

		} else
			SystemState->Display.LED_Power = 0;

		if (xTaskGetTickCount() - LastInDisplayMode > SECONDS(30)) {
			//This Reverts back to Display mode
			CurrentDisplayMode = DISPALYMODE_DISPLAY;
		}

		switch (CurrentDisplayMode) {
		case DISPALYMODE_DISPLAY: {
			LastInDisplayMode = xTaskGetTickCount();
			if ((KeyPressType > NONEWKEY)) {
				if ((KeyPressType >= LONGPRESS) && (SystemState->Display.KeyPressed == SET_KEY)) {
					CurrentDisplayMode = DISPALYMODE_SELECTPARM;
					ParamIndex = 0;
					break;
				} else if ((SystemState->Display.KeyPressed == RES_KEY) && (KeyPressType == LONGPRESS) && (Allow_Edit)) {
					for (int i = 0; i < 16; i++) {
						SystemState->Paramters.PARAM[i] = limit_para_tab[i][PARAM_DEFAULT];
					}
				} else if ((SystemState->Display.KeyPressed == RES_KEY)) {
					IgnoreErrors = 60; //30seconds
				} else if ((SystemState->Display.KeyPressed == INC_KEY) && (KeyPressType >= LONGPRESS)) {
					CurrentDisplayMode = DISPALYMODE_SUMMARY;
				} else if (SystemState->Display.KeyPressed == INC_KEY) {
					ParamIndex++;
				} else if ((SystemState->Display.KeyPressed == DEC_KEY) && (KeyPressType >= LONGPRESS)) {
					CurrentDisplayMode = DISPALYMODE_ID_PG0;
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					ParamIndex--;
				} else if ((SystemState->Display.KeyPressed == ONOFF_KEY) && (KeyPressType >= LONGPRESS)) {
					if (SystemState->Paramters.PARAM[PARAM_HASSLAVE])
						SystemState->MasterState = (SystemState->PoweredUp == 1) ? MASTER_BOOT_SLAVE : MASTER_STOP_SLAVE;
					else
						SystemState->PoweredUp = (SystemState->PoweredUp == 0x00) ? 1 : 0;
				}
			} else {
				//No Key
				if (FlashCnt == 0)
					ParamIndex = WRAP(ParamIndex, 3, 1); //Only display F8 and SetPoint
			}

			if ((SystemState->Errors == ERR_CODES_OK) || (IgnoreErrors > 0)) {

				TmpParam = FeatchValue(ParamIndex);
				SystemState->Display.LED_Alarm = 0;
				IgnoreErrors = (IgnoreErrors > 0) ? IgnoreErrors - 1 : 0;
			} else {
				//There is an ERROR
				ParamIndex = SystemState->ErrroCode;
				TmpParam = SystemState->ErrroNum;
				SystemState->Display.LED_Alarm = 1;
				//IgnoreErrors =(IgnoreErrors>0) ? IgnoreErrors-1 :0;
			}

			SystemState->Display.Digits[0] = ParamIndex;
			SystemState->Display.Digits[1] = (TmpParam % 100) / 10;
			SystemState->Display.Digits[2] = (TmpParam % 10);
			break;
		}
		case DISPALYMODE_ID_PG0: {

			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_ID_PG1;
			}
			int i = 0;
			quotient = SystemState->Unique_ID;
			while (quotient != 0) {
				temp = quotient % 16;
				IDmsg[i] = temp;
				quotient = quotient / 16;
				i++;
			}

			SystemState->Display.Digits[0] = IDmsg[0];
			SystemState->Display.Digits[1] = IDmsg[1];
			SystemState->Display.Digits[2] = IDmsg[2];
			break;
		}
		case DISPALYMODE_ID_PG1: {
			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_ID_PG2;
			}
			SystemState->Display.Digits[0] = IDmsg[3];
			SystemState->Display.Digits[1] = IDmsg[4];
			SystemState->Display.Digits[2] = IDmsg[5];
			break;
		}
		case DISPALYMODE_ID_PG2: {
			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_ID_PG3;
			}
			SystemState->Display.Digits[0] = IDmsg[6];
			SystemState->Display.Digits[1] = IDmsg[7];
			SystemState->Display.Digits[2] = IDmsg[8];
			break;
		}
		case DISPALYMODE_ID_PG3: {
			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_ID_PG4;
			}
			SystemState->Display.Digits[0] = IDmsg[9];
			SystemState->Display.Digits[1] = IDmsg[10];
			SystemState->Display.Digits[2] = IDmsg[11];
			break;
		}
		case DISPALYMODE_ID_PG4: {
			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_ID_PG5;
			}
			SystemState->Display.Digits[0] = IDmsg[12];
			SystemState->Display.Digits[1] = IDmsg[13];
			SystemState->Display.Digits[2] = IDmsg[14];
			break;
		}
		case DISPALYMODE_ID_PG5: {
			if ((KeyPressType > NONEWKEY)) {
				CurrentDisplayMode = DISPALYMODE_DISPLAY;
			}
			SystemState->Display.Digits[0] = IDmsg[15];
			SystemState->Display.Digits[1] = LED_OFF;
			SystemState->Display.Digits[2] = LED_OFF;
			break;
		}
		case DISPALYMODE_SELECTPARM: {
			if ((KeyPressType > NONEWKEY)) {
				//if ((KeyPressType >= LONGPRESS) && (SystemState->Display.KeyPressed == SET_KEY)) {
				if (Allow_Edit)
					AllowedToDisp = 15;
				//CurrentDisplayMode = DISPALYMODE_SELECTPARM;

				if (SystemState->Display.KeyPressed == SET_KEY) {
					CurrentDisplayMode = DISPALYMODE_SELECTVALUE;
					TmpParam = SystemState->Paramters.PARAM[ParamIndex];
					AllowedToDisp = MAXPARAM;
				} else if (SystemState->Display.KeyPressed == RES_KEY)
					CurrentDisplayMode = DISPALYMODE_DISPLAY;
				else if (SystemState->Display.KeyPressed == INC_KEY) {
					ParamIndex = (ParamIndex >= AllowedToDisp) ? 0 : ParamIndex + 1;
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					ParamIndex = (ParamIndex <= 0) ? AllowedToDisp : ParamIndex - 1;
				}
			}

			SystemState->Display.Digits[0] = LED_F;
			SystemState->Display.Digits[1] = (ParamIndex / 100);
			SystemState->Display.Digits[2] = (ParamIndex % 100);

			//ParamIndex = WRAP(ParamIndex,16,0);
			break;
		}
		case DISPALYMODE_SELECTVALUE: {
			if ((KeyPressType > NONEWKEY)) {
				//if ((KeyPressType >= LONGPRESS) && (SystemState->Display.KeyPressed == SET_KEY)) {
				if (Allow_Edit)
					AllowedToDisp = 15;
				//CurrentDisplayMode = DISPALYMODE_SELECTPARM;
				if (SystemState->Display.KeyPressed == SET_KEY) {
					CurrentDisplayMode = DISPALYMODE_DISPLAY;
					SystemState->Paramters.PARAM[ParamIndex] = TmpParam;
				} else if (SystemState->Display.KeyPressed == RES_KEY)
					CurrentDisplayMode = DISPALYMODE_SELECTPARM;
				else if (SystemState->Display.KeyPressed == INC_KEY) {
					TmpParam++;
					TmpParam = CLAMP(TmpParam, limit_para_tab[ParamIndex][PARAM_UPPER_LIMIT], limit_para_tab[ParamIndex][PARAM_LOWWER_LIMIT]);
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					TmpParam--;
					TmpParam = CLAMP(TmpParam, limit_para_tab[ParamIndex][PARAM_UPPER_LIMIT], limit_para_tab[ParamIndex][PARAM_LOWWER_LIMIT]);
				}
			}

			SystemState->Display.Digits[0] = TmpParam / 100;
			SystemState->Display.Digits[1] = (TmpParam % 100) / 10;
			SystemState->Display.Digits[2] = (TmpParam % 10);
			break;
		}
		case DISPALYMODE_SUMMARY: {
			if ((KeyPressType > NONEWKEY)) {
				if ((SystemState->Display.KeyPressed == INC_KEY) && (KeyPressType < LONGPRESS) && (Allow_Edit)) {
					CurrentDisplayMode = DISPALYMODE_SELECT_VENDOR;
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					CurrentDisplayMode = DISPALYMODE_DISPLAY;
				} else {
					CurrentDisplayMode = DISPALYMODE_DISPLAY;
				}
			}

			SystemState->Display.Digits[0] = Vendor_Disp[SystemState->Vendor];
			SystemState->Display.Digits[1] = Unit_Disp[SystemState->UnitType];
			SystemState->Display.Digits[2] = Comp_Summary[SystemState->CompressorSize];
			break;
		}
		case DISPALYMODE_SELECT_VENDOR: {
			if ((KeyPressType > NONEWKEY)) {
				if ((SystemState->Display.KeyPressed == INC_KEY) && (KeyPressType < LONGPRESS) && (Allow_Edit)) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_SELECT_UNIT;
					else
						SystemState->Vendor = WRAP(SystemState->Vendor, 2, 0);
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_SUMMARY;
					else
						SystemState->Vendor = WRAP_NEG(SystemState->Vendor, 2, 0);
				} else if ((SystemState->Display.KeyPressed == RES_KEY) && (KeyPressType >= LONGPRESS)) {
					UnitEdit = true;
				} else if (SystemState->Display.KeyPressed == SET_KEY) {
					UnitEdit = false;
				}
			}
			SystemState->Display.Digits[0] = 0;
			SystemState->Display.Digits[1] = LED_OFF;
			SystemState->Display.Digits[2] = Vendor_Disp[SystemState->Vendor];

			if ((UnitEdit) && (FlashCnt % 2 == 0)) {
				SystemState->Display.Digits[0] = LED_OFF;
				SystemState->Display.Digits[1] = LED_OFF;
				SystemState->Display.Digits[2] = LED_OFF;
			}
			break;
		}
		case DISPALYMODE_SELECT_UNIT: {
			if ((KeyPressType > NONEWKEY)) {
				if (SystemState->Display.KeyPressed == INC_KEY) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_SELECT_COMP;
					else
						SystemState->UnitType = WRAP(SystemState->UnitType, 2, 0);
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_SELECT_VENDOR;
					else
						SystemState->UnitType = WRAP_NEG(SystemState->UnitType, 2, 0);
				} else if ((SystemState->Display.KeyPressed == RES_KEY) && (KeyPressType >= LONGPRESS) && (Allow_Edit)) {
					UnitEdit = true;
				} else if (SystemState->Display.KeyPressed == SET_KEY) {
					UnitEdit = false;
				}
			}

			SystemState->Display.Digits[0] = 1;
			SystemState->Display.Digits[1] = LED_OFF;
			SystemState->Display.Digits[2] = Unit_Disp[SystemState->UnitType];
			if ((UnitEdit) && (FlashCnt == 0)) {
				SystemState->Display.Digits[0] = LED_OFF;
				SystemState->Display.Digits[1] = LED_OFF;
				SystemState->Display.Digits[2] = LED_OFF;
			}

			break;
		}
		case DISPALYMODE_SELECT_COMP: {
			if ((KeyPressType > NONEWKEY)) {
				if (SystemState->Display.KeyPressed == INC_KEY) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_DISPLAY;
					else
						SystemState->CompressorSize = WRAP(SystemState->CompressorSize, 2, 0);
				} else if (SystemState->Display.KeyPressed == DEC_KEY) {
					if (!UnitEdit)
						CurrentDisplayMode = DISPALYMODE_SELECT_UNIT;
					else
						SystemState->CompressorSize = WRAP_NEG(SystemState->CompressorSize, 2, 0);
				} else if (SystemState->Display.KeyPressed == SET_KEY) {
					UnitEdit = false;
				} else if ((SystemState->Display.KeyPressed == RES_KEY) && (KeyPressType >= LONGPRESS) && (Allow_Edit)) {
					UnitEdit = true;
				}
			}
			SystemState->Display.Digits[0] = 2;
			SystemState->Display.Digits[1] = Comp_Disp[SystemState->CompressorSize] / 10;
			SystemState->Display.Digits[2] = Comp_Disp[SystemState->CompressorSize] % 10;
			if ((UnitEdit) && (FlashCnt == 0)) {
				SystemState->Display.Digits[0] = LED_OFF;
				SystemState->Display.Digits[1] = LED_OFF;
				SystemState->Display.Digits[2] = LED_OFF;
			}

			break;
		}
		default:
			CurrentDisplayMode = DISPALYMODE_DISPLAY;
		}
		/*
		 //
		 SystemState->Display.Digits[0]=SystemState->Display.KeyPressed;
		 SystemState->Display.Digits[1]=KeyPressType;
		 SystemState->Display.Digits[2]++;
		 */

		if ((Allow_Edit) && (FlashCnt % 2 == 0)) {
			SystemState->Display.Digits[0] = LED_OFF;
			SystemState->Display.Digits[1] = LED_OFF;
			SystemState->Display.Digits[2] = LED_OFF;
		}

		if ((SystemState->PoweredUp == 0x00) && (CurrentDisplayMode == DISPALYMODE_DISPLAY)) {
			SystemState->Display.Digits[0] = LED_OFF;
			SystemState->Display.Digits[1] = LED_OFF;
			SystemState->Display.Digits[2] = LED_OFF;
			SystemState->Display.LED_Alarm = 0;
			SystemState->Display.LED_Power = 0;
		}

//All we need to do nto is set th SystemState Values above this line and rest is done for me
#ifndef NEW_LED
		DISP_RAM[1] = DATA_7SEG[SystemState->Display.Digits[2] % (sizeof(DATA_7SEG))];
		DISP_RAM[2] = DATA_7SEG[SystemState->Display.Digits[1] % (sizeof(DATA_7SEG))];
		DISP_RAM[3] = DATA_7SEG[SystemState->Display.Digits[0] % (sizeof(DATA_7SEG))];
#else
		DISP_RAM[1] = DATA_7SEG[SystemState->Display.Digits[0] % (sizeof(DATA_7SEG))];
		DISP_RAM[2] = DATA_7SEG[SystemState->Display.Digits[1] % (sizeof(DATA_7SEG))];
		DISP_RAM[3] = DATA_7SEG[SystemState->Display.Digits[2] % (sizeof(DATA_7SEG))];
#endif
		DISP_RAM[0] = ((SystemState->Display.LED_Power) ? BIT(LED_D1_BIT) : 0) | ((SystemState->Display.LED_Alarm) ? BIT(LED_D2_BIT) : 0);
		Disp_Driver();

		FlashCnt = WRAP(FlashCnt, 6, 0);
		//HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin,1);
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Display, 500);
	}
}
