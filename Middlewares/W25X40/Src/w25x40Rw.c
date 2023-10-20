#include "w25x40Rw.h"
#include "main.h"

uint8_t upper_128[16];
uint8_t tx_buff[16];

void spiIoInit(void);
void IO_Write_Disable(void);
void diPinToInput(void);
void diPinToOutput(void);
void IO_Send_Byte(uint8_t out);
uint8_t IO_Get_Byte(void);
void delay(uint16_t tt);
void IO_Wait_Busy(void);
uint8_t IO_Read_StatusReg(void);
void IO_Write_StatusReg(uint8_t byte);
void IO_Write_Enable(void);
void IO_PowerDown(void);
void IO_ReleasePowerDown(void);
uint64_t IO_Read_ID1(void);
uint16_t IO_Read_ID2(uint8_t ID_Addr);
uint16_t IO_Read_ID3(void);
uint8_t IO_Read_Byte(uint32_t Dst_Addr);
void IO_Read_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128);
uint8_t IO_FastRead_Byte(uint32_t Dst_Addr);
void IO_FastRead_nBytes(uint32_t Dst_Addr, uint8_t nBytes_128);
void IO_Write_Byte(uint32_t Dst_Addr, uint8_t byte);
void IO_Write_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128);
void IO_Erase_Chip(void);
void IO_Erase_Sector(uint32_t Dst_Addr);
void IO_Write_Disable(void);




/*
 *PD11:FLASH DAT = DI/DO
 *PD12:FLASH CLK
 *PD13:FLASH CS
 */
void spiIoInit(void) {

	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOD_CLK_ENABLE();

	GPIO_InitStruct.Pin = FLASH_DI_Pin | FLASH_CLK_Pin | FLASH_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = FLASH_DI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = FLASH_DO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	CLK_L;

	//CLK_H;
	CS_H;
	IO_Write_Disable();
}

void diPinToInput(void) {

#ifdef OLDEEPROM
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOD_CLK_ENABLE();
	GPIO_InitStruct.Pin = FLASH_DI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
#endif
}


void diPinToOutput(void) {

#ifdef OLDEEPROM
	GPIO_InitTypeDef GPIO_InitStruct;
	__HAL_RCC_GPIOD_CLK_ENABLE();
	GPIO_InitStruct.Pin = FLASH_DI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
#endif
}

void IO_Send_Byte(uint8_t out) {
	uint8_t i = 0;
	CS_L;
	for (i = 0; i < 8; i++) {
		delay(1024);
		if ((out & 0x80) == 0x80) /* check if MSB is high */
			DI_H;
		else
			DI_L; /* if not, set to low */
		CLK_H; /* toggle clock high */
		out = (out << 1); /* shift 1 place for next bit */
		delay(1024);
		CLK_L; /* toggle clock low */
	}
}


uint8_t IO_Get_Byte() {
	uint8_t i = 0, in = 0, temp = 0;
	diPinToInput();
	CS_L;
	for (i = 0; i < 8; i++) {
		delay(1024);
		in = (in << 1); /* shift 1 place to the left or shift in 0 */
		temp = DO_READ; /* save input */
		CLK_H; /* toggle clock high */
		if (temp == 1) /* check to see if bit is high */
			in |= 0x01; /* if high, make bit high */
		delay(1024);
		CLK_L; /* toggle clock low */

	}
	diPinToOutput();
	return in;
}

void delay(uint16_t tt) {
	while (tt--)
		__NOP();
}

void IO_Wait_Busy() {
	/*  waste time until not busy WEL & Busy bit all be 1 (0x03). */
	uint8_t reg = IO_Read_StatusReg();
	while (reg && 0x03 != 0x03)
		reg = IO_Read_StatusReg();
}

void IO_Wait_WEL() {
	/*  waste time until not busy WEL & Busy bit all be 1 (0x03). */
	uint8_t reg = IO_Read_StatusReg();
	while (reg && 0x02 != 0x02)
		reg = IO_Read_StatusReg();
}

uint8_t IO_Read_StatusReg() {
	uint8_t byte = 0;
	CS_L; /* enable device */
	IO_Send_Byte(W25P_ReadStatusReg); /* send Read Status Register command */
	byte = IO_Get_Byte(); /* receive byte */
	CS_H; /* disable device */

	return byte;
}

void IO_Write_StatusReg(uint8_t byte) {
	CS_L; /* enable device */
	IO_Write_Enable(); /* set WEL */
	IO_Wait_Busy();

	CS_L; /* enable device *	delay(50);
	IO_Send_Byte(W25P_WriteStatusReg); /* select write to status register */
	delay(50);
	IO_Send_Byte(byte); /* data that will change the status(only bits 2,3,7 can be written) */
	delay(50);
	CS_H; /* disable the device */
}

void IO_Write_Enable() {
	CS_L; /* enable device */
	IO_Send_Byte(W25P_WriteEnable); /* send W25P_Write_Enable command */
	CS_H; /* disable device */
}

void IO_PowerDown() {
	CS_L;
	; /* enable device */
	IO_Send_Byte(W25P_PowerDown); /* send W25P_PowerDown command 0xB9 */
	CS_H;
	; /* disable device */
	delay(6); /* remain CS high for tPD = 3uS */
}

void IO_ReleasePowerDown() {
	CS_L; /* enable device */
	IO_Send_Byte(W25P_ReleasePowerDown); /* send W25P_PowerDown command 0xAB */
	CS_H; /* disable device */
	delay(6); /* remain CS high for tRES1 = 3uS */
}

uint64_t IO_Read_ID1() {
	uint64_t byte;
	CS_L; /* enable device */
	IO_Send_Byte(W25P_DeviceID); /* send read device ID command (ABh) */
	IO_Send_Byte(0); /* send address */
	IO_Send_Byte(0); /* send address */
	IO_Send_Byte(0); /* send 3_Dummy address */
	IO_Send_Byte(0); /* send 3_Dummy address */
	byte = ((uint64_t)IO_Get_Byte())<<56; /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<48); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<40); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<32); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<24); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<16); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())<<8); /* receive Device ID byte */
	byte |= ( ((uint64_t)IO_Get_Byte())); /* receive Device ID byte */
	CS_H; /* disable device */
	delay(4); /* remain CS high for tRES2 = 1.8uS */

	return byte;
}

uint16_t IO_Read_ID2(uint8_t ID_Addr) {
	uint16_t IData16;
	CS_L; /* enable device */
	IO_Send_Byte(W25P_ManufactDeviceID); /* send read ID command (90h) */
	IO_Send_Byte(0x00); /* send address */
	IO_Send_Byte(0x00); /* send address */
	IO_Send_Byte(ID_Addr); /* send W25Pxx selectable ID address 00H or 01H */
	IData16 = IO_Get_Byte() << 8; /* receive Manufature or Device ID byte */
	IData16 |= IO_Get_Byte(); /* receive Device or Manufacture ID byte */
	CS_H; /* disable device */

	return IData16;
}

uint16_t IO_Read_ID3() {
	uint16_t IData16;
	CS_L; /* enable device */
	IO_Send_Byte(0x9f); /* send read ID command (90h) */

	IData16 = IO_Get_Byte() << 8; /* receive Manufature or Device ID byte */
	IData16 |= IO_Get_Byte(); /* receive Device or Manufacture ID byte */
	tx_buff[2] = IO_Get_Byte();
	CS_H; /* disable device */

	return IData16;
}

uint8_t IO_Read_Byte(uint32_t Dst_Addr) {
	uint8_t byte = 0;

	CS_L; /* enable device */
	IO_Send_Byte(W25P_ReadData); /* read command */
	delay(100);
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	delay(100);
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	delay(100);
	IO_Send_Byte(Dst_Addr & 0xFF);
	delay(100);
	byte = IO_Get_Byte();
	delay(100);
	CS_H; /* disable device */

	return byte; /* return one byte read */
}

void IO_Read_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128) {
	uint32_t i = 0;

	CS_L; /* enable device */
	IO_Send_Byte(W25P_ReadData); /* read command */
	delay(100);
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	delay(100);
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	delay(100);
	IO_Send_Byte(Dst_Addr & 0xFF);
	delay(100);
	for (i = 0; i < nBytes_128; i++) /* read until no_bytes is reached */
	{
		buf[i] = IO_Get_Byte(); /* receive byte and store at address 80H - FFH */
		delay(100);
	}

	CS_H; /* disable device */

}

uint8_t IO_FastRead_Byte(uint32_t Dst_Addr) {
	uint8_t byte = 0;

	CS_L; /* enable device */
	IO_Send_Byte(W25P_FastReadData); /* fast read command */
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	IO_Send_Byte(Dst_Addr & 0xFF);
	IO_Send_Byte(0xFF); /*dummy byte*/
	byte = IO_Get_Byte();
	CS_H; /* disable device */

	return byte; /* return one byte read */
}

void IO_FastRead_nBytes(uint32_t Dst_Addr, uint8_t nBytes_128) {
	uint8_t i = 0;

	CS_L; /* enable device */
	IO_Send_Byte(W25P_FastReadData); /* read command */
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	IO_Send_Byte(Dst_Addr & 0xFF);
	IO_Send_Byte(0xFF); /*dummy byte*/
	for (i = 0; i < nBytes_128; i++) /* read until no_bytes is reached */
	{
		upper_128[i] = IO_Get_Byte(); /* receive byte and store at address 80H - FFH */
	}
	CS_H; /* disable device */

}

void IO_Write_Byte(uint32_t Dst_Addr, uint8_t byte) {
	CS_L; /* enable device */
	delay(200);
	IO_Write_Enable(); /* set WEL */
	delay(200);
	IO_Wait_Busy();
	delay(200);
	CS_L;
	delay(500);
	IO_Send_Byte(W25P_PageProgram); /* send Byte Program command */
	delay(500);
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	delay(500);
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	delay(500);
	IO_Send_Byte(Dst_Addr & 0xFF);
	delay(500);
	IO_Send_Byte(byte); /* send byte to be programmed */
	delay(200);
	CS_H; /* disable device */
}

void IO_Write_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128) {
	uint8_t i, byte;
	CS_L; /* enable device */
	delay(200);
	IO_Write_Enable(); /* set WEL */
	delay(200);
	IO_Wait_Busy();
	delay(200);
	CS_L;
	delay(500);
	IO_Send_Byte(W25P_PageProgram); /* send Byte Program command */
	delay(500);
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	delay(500);
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	delay(500);
	IO_Send_Byte(Dst_Addr & 0xFF);
	delay(500);
	for (i = 0; i < nBytes_128; i++) {
		byte = buf[i];
		IO_Send_Byte(byte); /* send byte to be programmed */
		delay(500);
	}
	delay(500);
	CS_H; /* disable device */

	//printf("\nPage program (%d nBytes)! please waiting....\n");
}

void IO_Erase_Chip(void) {
	CS_L; /* enable device */
	IO_Write_Enable(); /* set WEL */
	CS_L;
	IO_Wait_Busy();
	CS_L;
	IO_Send_Byte(W25P_ChipErase); /* send Chip Erase command */
	CS_H; /* disable device */
}

void IO_Erase_Sector(uint32_t Dst_Addr) {
	CS_L; /* enable device */
	IO_Write_Enable(); /* set WEL */
	CS_L;
	IO_Send_Byte(W25P_SectorErase); /* send Sector Erase command */
	IO_Send_Byte(((Dst_Addr & 0xFFFFFF) >> 16)); /* send 3 address bytes */
	IO_Send_Byte(((Dst_Addr & 0xFFFF) >> 8));
	IO_Send_Byte(Dst_Addr & 0xFF);
	CS_H; /* disable device */
}

void IO_Write_Disable(void) {
	CS_L; /* enable device */
	IO_Send_Byte(W25P_WriteDisable); /* send W25P_Write_Disable command */
	CS_H; /* disable device */
}
