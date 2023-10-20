################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/MODBUS-LIB/Src/Modbus.c \
../Middlewares/MODBUS-LIB/Src/UARTCallback.c 

OBJS += \
./Middlewares/MODBUS-LIB/Src/Modbus.o \
./Middlewares/MODBUS-LIB/Src/UARTCallback.o 

C_DEPS += \
./Middlewares/MODBUS-LIB/Src/Modbus.d \
./Middlewares/MODBUS-LIB/Src/UARTCallback.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/MODBUS-LIB/Src/%.o: ../Middlewares/MODBUS-LIB/Src/%.c Middlewares/MODBUS-LIB/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DLWIP_DEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DMODBUS_TCP -DDEBUG_UART -c -I../Core/Inc -I"D:/GitHub/WindowUnitV2_407/Middlewares/MODBUS-LIB/Config" -I"D:/GitHub/WindowUnitV2_407/Middlewares/MODBUS-LIB/Inc" -I"D:/GitHub/WindowUnitV2_407/Middlewares/InternalFlash" -I"D:/GitHub/WindowUnitV2_407/Middlewares/Nmea" -I"D:/GitHub/WindowUnitV2_407/Middlewares/PID" -I"D:/GitHub/WindowUnitV2_407/Middlewares/W25X40/Inc" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

