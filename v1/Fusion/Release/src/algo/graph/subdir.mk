################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/graph/Aggregate.cpp \
../src/algo/graph/SubnetMapEntry.cpp \
../src/algo/graph/Graph.cpp \
../src/algo/graph/GraphBuilder.cpp \
../src/algo/graph/Peer.cpp

OBJS += \
./src/algo/graph/Aggregate.o \
./src/algo/graph/SubnetMapEntry.o \
./src/algo/graph/Graph.o \
./src/algo/graph/GraphBuilder.o \
./src/algo/graph/Peer.o

CPP_DEPS += \
./src/algo/graph/Aggregate.d \
./src/algo/graph/SubnetMapEntry.d \
./src/algo/graph/Graph.d \
./src/algo/graph/GraphBuilder.d \
./src/algo/graph/Peer.d


# Each subdirectory must supply rules for building sources it contributes
src/algo/graph/%.o: ../src/algo/graph/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


