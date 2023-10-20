/*
 * UARTCallback.c
 *
 *  Created on: May 27, 2020
 *      Author: Alejandro Mera
 */

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "main.h"
#include "Modbus.h"
#include "Macros.h"

/**
 * @brief
 * This is the callback for HAL interrupts of UART TX used by Modbus library.
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @ingroup UartHandle UART HAL handler
 */

extern osThreadId_t GPSTaskHandle;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart2;
extern void USER_UART_IRQHandler(UART_HandleTypeDef *huart);
extern APP_GPSdata_T APP_GPSdata;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	/* Modbus RTU TX callback BEGIN */
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	int i;
	for (i = 0; i < numberHandlers; i++) {

		if (!mHandlers[i]->isSupended) {
			if (mHandlers[i]->port == huart) {
				xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
				break;
			}
		}
	}

	/* Modbus RTU TX callback END */

	/*
	 * Here you should implement the callback code for other UARTs not used by Modbus
	 *
	 * */

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}

/**
 * @brief
 * This is the callback for HAL interrupt of UART RX
 * This callback is shared among all UARTS, if more interrupts are used
 * user should implement the correct control flow and verification to maintain
 * Modbus functionality.
 * @ingroup UartHandle UART HAL handler
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Modbus RTU RX callback BEGIN */
	int i;
	for (i = 0; i < numberHandlers; i++) {
		if (mHandlers[i]->port == UartHandle) {

			if (mHandlers[i]->xTypeHW == USART_HW) {
				RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
				HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
				xTimerResetFromISR(mHandlers[i]->xTimerT35, &xHigherPriorityTaskWoken);
			}

			break;
		}
	}

	if (UartHandle == &huart4) {
		//This is GPS
		//HAL_UART_AbortReceive_IT(&huart4);
		*APP_GPSdata.linepos = APP_GPSdata.rx_byte;

		APP_GPSdata.linepos++;
		*APP_GPSdata.linepos = '\0';
		if (APP_GPSdata.linepos - APP_GPSdata.line > sizeof(APP_GPSdata.line) - 2)
			APP_GPSdata.linepos = &APP_GPSdata.line[0];

		if (APP_GPSdata.rx_byte == '\n')
			xTaskNotifyFromISR(GPSTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);

		HAL_UART_Receive_IT(&huart4, &APP_GPSdata.rx_byte, 1);

	}



	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	/* Modbus RTU RX callback END */

	/*
	 * Here you should implement the callback code for other UARTs not used by Modbus
	 *
	 *
	 * */

}

#if  ENABLE_USART_DMA ==  1
/*
 * DMA requires to handle callbacks for special communication modes of the HAL
 * It also has to handle eventual errors including extra steps that are not automatically
 * handled by the HAL
 * */


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

 int i;

 for (i = 0; i < numberHandlers; i++ )
 {
    	if (mHandlers[i]->port == huart  )
    	{

    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
    		{
    			while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
    		    {
    					HAL_UART_DMAStop(mHandlers[i]->port);
   				}
				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

    		}

    		break;
    	}
   }
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		/* Modbus RTU RX callback BEGIN */
	    int i;
	    for (i = 0; i < numberHandlers; i++ )
	    {
	    	if (mHandlers[i]->port == huart  )
	    	{


	    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
	    		{
	    			if(Size) //check if we have received any byte
	    			{
		    				mHandlers[i]->xBufferRX.u8available = Size;
		    				mHandlers[i]->xBufferRX.overflow = false;

		    				while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
		    				{
		    					HAL_UART_DMAStop(mHandlers[i]->port);


		    				}
		    				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

		    				xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0 , eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
	    			}
	    		}

	    		break;
	    	}
	    }
	    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#endif
