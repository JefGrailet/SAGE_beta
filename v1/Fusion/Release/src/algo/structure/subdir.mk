################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/structure/IPTableEntry.cpp \
../src/algo/structure/IPLookUpTable.cpp \
../src/algo/structure/SubnetSiteNode.cpp \
../src/algo/structure/RouteInterface.cpp \
../src/algo/structure/SubnetSite.cpp \
../src/algo/structure/SubnetSiteSet.cpp \
../src/algo/structure/RouterInterface.cpp \
../src/algo/structure/Router.cpp

OBJS += \
./src/algo/structure/IPTableEntry.o \
./src/algo/structure/IPLookUpTable.o \
./src/algo/structure/SubnetSiteNode.o \
./src/algo/structure/RouteInterface.o \
./src/algo/structure/SubnetSite.o \
./src/algo/structure/SubnetSiteSet.o \
./src/algo/structure/RouterInterface.o \
./src/algo/structure/Router.o

CPP_DEPS += \
./src/algo/structure/IPTableEntry.d \
./src/algo/structure/IPLookUpTable.d \
./src/algo/structure/SubnetSiteNode.d \
./src/algo/structure/RouteInterface.d \
./src/algo/structure/SubnetSite.d \
./src/algo/structure/SubnetSiteSet.d \
./src/algo/structure/RouterInterface.d \
./src/algo/structure/Router.d

# Each subdirectory must supply rules for building sources it contributes
src/algo/structure/%.o: ../src/algo/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


