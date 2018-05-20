################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/parsing/ConfigFileParser.cpp \
../src/algo/parsing/TargetParser.cpp \
../src/algo/parsing/SubnetParser.cpp

OBJS += \
./src/algo/parsing/ConfigFileParser.o \
./src/algo/parsing/TargetParser.o \
./src/algo/parsing/SubnetParser.o

CPP_DEPS += \
./src/algo/parsing/ConfigFileParser.d \
./src/algo/parsing/TargetParser.d \
./src/algo/parsing/SubnetParser.d


# Each subdirectory must supply rules for building sources it contributes
src/algo/parsing/%.o: ../src/algo/parsing/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


