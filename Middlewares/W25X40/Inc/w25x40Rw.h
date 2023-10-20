#ifndef _W25X40_RW_
#define _W25X40_RW_

#include "stm32f4xx_hal.h"
#include "main.h"




#define W25P_WriteEnable 0x06
#define W25P_WriteDisable 0x04
#define W25P_ReadStatusReg 0x05
#define W25P_WriteStatusReg 0x01
#define W25P_ReadData  0x03
#define W25P_FastReadData 0x0B
#define W25P_PageProgram 0x02
#define W25P_SectorErase 0x20             //0xD8-64KB,0x20-4KB,0x52-32KB
#define W25P_ChipErase  0xC7
#define W25P_PowerDown  0xB9
#define W25P_ReleasePowerDown 0xAB
#define W25P_DeviceID  0x4B
#define W25P_ManufactDeviceID 0x90

#ifdef  OLDEEPROM
#define DI_H    HAL_GPIO_WritePin(FLASH_DI_GPIO_Port, FLASH_DI_Pin, GPIO_PIN_SET)
#define DI_L    HAL_GPIO_WritePin(FLASH_DI_GPIO_Port, FLASH_DI_Pin, GPIO_PIN_RESET)
#define CS_H    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)
#define CS_L    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define CLK_H   HAL_GPIO_WritePin(FLASH_CLK_GPIO_Port, FLASH_CLK_Pin, GPIO_PIN_SET)
#define CLK_L   HAL_GPIO_WritePin(FLASH_CLK_GPIO_Port, FLASH_CLK_Pin, GPIO_PIN_RESET)
#define DO_READ HAL_GPIO_ReadPin(FLASH_DI_GPIO_Port, FLASH_DI_Pin)
#else
#define DI_H    HAL_GPIO_WritePin(FLASH_DI_GPIO_Port, FLASH_DI_Pin, GPIO_PIN_SET)
#define DI_L    HAL_GPIO_WritePin(FLASH_DI_GPIO_Port, FLASH_DI_Pin, GPIO_PIN_RESET)
#define CS_H    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)
#define CS_L    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define CLK_H   HAL_GPIO_WritePin(FLASH_CLK_GPIO_Port, FLASH_CLK_Pin, GPIO_PIN_SET)
#define CLK_L   HAL_GPIO_WritePin(FLASH_CLK_GPIO_Port, FLASH_CLK_Pin, GPIO_PIN_RESET)
#define DO_READ HAL_GPIO_ReadPin(FLASH_DO_GPIO_Port, FLASH_DO_Pin)
#endif

extern uint8_t upper_128[16];
extern uint8_t tx_buff[16];

extern void spiIoInit(void);
extern uint8_t IO_Read_StatusReg(void);
extern void IO_Write_Disable(void);
extern void diPinToInput(void);
extern void diPinToOutput(void);
extern void IO_Send_Byte(uint8_t out);
extern uint8_t IO_Get_Byte(void);
extern void delay(uint16_t tt);
extern void IO_Wait_Busy(void);
extern uint8_t IO_Read_StatusReg(void);
extern void IO_Write_StatusReg(uint8_t byte);
extern void IO_Write_Enable(void);
extern void IO_PowerDown(void);
extern void IO_ReleasePowerDown(void);
extern uint64_t IO_Read_ID1(void);
extern uint16_t IO_Read_ID2(uint8_t ID_Addr);
extern uint16_t IO_Read_ID3(void);
extern uint8_t IO_Read_Byte(uint32_t Dst_Addr);
extern void IO_Read_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128);
extern uint8_t IO_FastRead_Byte(uint32_t Dst_Addr);
extern void IO_FastRead_nBytes(uint32_t Dst_Addr, uint8_t nBytes_128);
extern void IO_Write_Byte(uint32_t Dst_Addr, uint8_t byte);
extern void IO_Write_nBytes(uint32_t Dst_Addr, uint8_t *buf, uint8_t nBytes_128);
extern void IO_Erase_Chip(void);
extern void IO_Erase_Sector(uint32_t Dst_Addr);
extern void IO_Write_Disable(void);

#endif
