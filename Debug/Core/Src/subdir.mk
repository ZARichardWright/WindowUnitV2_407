################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/appAdc.c \
../Core/Src/appDampers.c \
../Core/Src/appDisplay.c \
../Core/Src/appErr.c \
../Core/Src/appFan.c \
../Core/Src/appGPS.c \
../Core/Src/appHealth.c \
../Core/Src/appIP.c \
../Core/Src/appModBus.c \
../Core/Src/appOutdoorComs.c \
../Core/Src/appSystem.c \
../Core/Src/freertos.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

OBJS += \
./Core/Src/appAdc.o \
./Core/Src/appDampers.o \
./Core/Src/appDisplay.o \
./Core/Src/appErr.o \
./Core/Src/appFan.o \
./Core/Src/appGPS.o \
./Core/Src/appHealth.o \
./Core/Src/appIP.o \
./Core/Src/appModBus.o \
./Core/Src/appOutdoorComs.o \
./Core/Src/appSystem.o \
./Core/Src/freertos.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/Src/appAdc.d \
./Core/Src/appDampers.d \
./Core/Src/appDisplay.d \
./Core/Src/appErr.d \
./Core/Src/appFan.d \
./Core/Src/appGPS.d \
./Core/Src/appHealth.d \
./Core/Src/appIP.d \
./Core/Src/appModBus.d \
./Core/Src/appOutdoorComs.d \
./Core/Src/appSystem.d \
./Core/Src/freertos.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DLWIP_DEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DMODBUS_TCP -DDEBUG_UART -c -I../Core/Inc -I"D:/GitHub/WindowUnitV2_407/Middlewares/MODBUS-LIB/Config" -I"D:/GitHub/WindowUnitV2_407/Middlewares/MODBUS-LIB/Inc" -I"D:/GitHub/WindowUnitV2_407/Middlewares/InternalFlash" -I"D:/GitHub/WindowUnitV2_407/Middlewares/Nmea" -I"D:/GitHub/WindowUnitV2_407/Middlewares/PID" -I"D:/GitHub/WindowUnitV2_407/Middlewares/W25X40/Inc" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

