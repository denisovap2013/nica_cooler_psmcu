//==============================================================================
//
// Title:       CGW_PSMCU_Interface.h
// Purpose:     A short description of the interface.
//
// Created on:  18.11.2021 at 13:59:35 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __CGW_PSMCU_Interface_H__
#define __CGW_PSMCU_Interface_H__

#include <ansi_c.h>  
#include "cangw.h"
#include "psMcuProtocol.h" 
#include "CGW_Devices.h"

// Default timeout for sending (microseconds)
#define CGW_PSMCU_DEFAULT_TIMEOUT 50

#define CGW_PSMCU_DEVICE_CODE 4 

typedef struct cgwPsMcu_Information
{
	// Device unique name
	char name[CGW_DEVICES_NAME_MAX_LENGTH];
	int deviceID;
	char deviceName[256];
	int cgwIndex, localIndex, globalDeviceIndex;
	cgw_devices_t *p_devKit;

	// Attribues
	unsigned char DeviceCode;
	unsigned char HWversion;
	unsigned char SWversion;
	unsigned char Reason;
	// Status
	unsigned char DeviceMode;
	unsigned char Label;
	unsigned char FileIdent;
	unsigned int  pADC;
	unsigned int  pDAC;

	// Register Data
	unsigned char OutputRegisterData;
	unsigned char InputRegisterData;
	char InputRegistersNames[PSMCU_INPUT_REGISTERS_NUM][256];
	char OutputRegistersNames[PSMCU_OUTPUT_REGISTERS_NUM][256];
	char OutputRegistersConfirmed;  // Indicator that the output registers have read after the last previous update.
	// DAC
	unsigned int DACcode[PSMCU_DAC_CHANNELS_NUM];
	int DACcodeConfirmed[PSMCU_DAC_CHANNELS_NUM];
	char DacChannelsNames[PSMCU_DAC_CHANNELS_NUM][256];
	float DACvoltage[PSMCU_DAC_CHANNELS_NUM];
	double DacCoefficients[PSMCU_DAC_CHANNELS_NUM][2];
	// ADC
	unsigned long ChannelData[PSMCU_ADC_CHANNELS_NUM];
	float ChannelVoltage[PSMCU_ADC_CHANNELS_NUM];
	char AdcChannelsNames[PSMCU_ADC_CHANNELS_NUM][256]; 
	double AdcCoefficients[PSMCU_ADC_CHANNELS_NUM][2]; 

	// SLOW DAC
	unsigned int targetDACcode[PSMCU_DAC_CHANNELS_NUM];
	int usingDACslowMode[PSMCU_DAC_CHANNELS_NUM];
	clock_t DAClastTimeWrite[PSMCU_DAC_CHANNELS_NUM];
	unsigned int maxDACcodeStep;
	clock_t DACslowUpdateDelta;
	
	// Tables
	// ...
	
} cgwPsMcu_Information_t;

//==============================================================================
//
//==============================================================================

// Device registration
int cgwPsMcu_RegisterDevice(
	cgw_devices_t * devKit,
	int globalDeviceIndex,
	int cgwIndex,
	unsigned int deviceID,
	char *deviceName, 
	message_hook_func_t userHookFunc,
	void (*deviceAwakeCallback)(void *),
	void (*deviceLostCallback)(void *),
	long deviceDowntimeLimit,
	char adcChannelsNames[PSMCU_ADC_CHANNELS_NUM][256],
	char dacChannelsNames[PSMCU_DAC_CHANNELS_NUM][256],
	double AdcCoefficients[PSMCU_ADC_CHANNELS_NUM][2],
	double DacCoefficients[PSMCU_DAC_CHANNELS_NUM][2],
	char inputRegistersNames[PSMCU_INPUT_REGISTERS_NUM][256],
	char outputRegistersNames[PSMCU_OUTPUT_REGISTERS_NUM][256],
	unsigned int slowDacTimeDelta_ms,
	float slowDacMaxVoltageStep
	);

// Device update
void cgwPsMcu_updateCallback(void * parameters);


//============================================================================== 
//============================================================================== 
// Receiving messages
//==============================================================================
//============================================================================== 

// Message hook
void cgwPsMcu_MessageHook(cangw_msg_t msg, void * deviceData); 

// 			Hooks
// ADC (0x01 - 0x04)
void cgwPsMcu_Hook_ADC(cangw_msg_t msg, cgwPsMcu_Information_t * info); 
// DAC
void cgwPsMcu_Hook_DAC(cangw_msg_t msg, cgwPsMcu_Information_t * info); 
// Tables
void cgwPsMcu_Hook_F5(cangw_msg_t msg, cgwPsMcu_Information_t * info); 
void cgwPsMcu_Hook_F6(cangw_msg_t msg, cgwPsMcu_Information_t * info); 
// Registers
void cgwPsMcu_Hook_Register(cangw_msg_t msg, cgwPsMcu_Information_t * info);
// Status
void cgwPsMcu_Hook_Status(cangw_msg_t msg, cgwPsMcu_Information_t * info);
// Attributes
void cgwPsMcu_Hook_Attributes(cangw_msg_t msg, cgwPsMcu_Information_t * info); 

//============================================================================== 
//============================================================================== 
// Sending commands
//============================================================================== 
//============================================================================== 

// Message 00 
int cgwPsMcu_Stop(int cgwIndex, unsigned char deviceId, int broadcast);

// Message 01 
int cgwPsMcu_MultiChannelConfig(int cgwIndex, unsigned char deviceId,unsigned char ChBeg, unsigned char ChEnd, psMcuProtocol_time_t time, psMcuProtocol_mode_t mode, unsigned char label);

// Message 02
int cgwPsMcu_SingleAdcChannelRequest(int cgwIndex, unsigned char deviceId, unsigned char Channel, psMcuProtocol_time_t time, psMcuProtocol_mode_t mode);

// Message 03
int cgwPsMcu_MultiChannelRequest(int cgwIndex, unsigned char deviceId,unsigned char Channel);

// Message 04
int cgwPsMcu_LoopBufferRequest(int cgwIndex, unsigned char deviceId, unsigned int pADC);

// Message 80
int cgwPsMcu_DACWriteCode(int cgwIndex, unsigned char deviceId, unsigned int channel, unsigned int Code);
int cgwPsMcu_DACWriteVoltage(int cgwIndex, unsigned char deviceId, unsigned char channel, float voltage);
int cgwPsMcu_DACVoltageToCode(float voltage, unsigned int *code);

// Message 90 
int cgwPsMcu_DACReadCode(int cgwIndex, unsigned char deviceId, unsigned char channel);

// Tables
void cgwPsMcu_CreateTable(void);			// Message F3
void cgwPsMcu_WriteToTable(void);			// Message F4
void cgwPsMcu_CloseTable(void);			    // Message F5
void cgwPsMcu_TableRequest(void);			// Message F6
void cgwPsMcu_TableExecute(void);			// Message F7

// Message F8
int cgwPsMcu_RegistersRequest(int cgwIndex, unsigned char deviceId);

// Message F9
int cgwPsMcu_WriteToOutputRegister(int cgwIndex, unsigned char deviceId, unsigned char OutputRegister);

// Message FE
int cgwPsMcu_StatusRequest(int cgwIndex, unsigned char deviceId);

// Message FF
int cgwPsMcu_AttributesRequest(int cgwIndex, unsigned char deviceId);	

#endif  /* ndef __CGW_PSMCU_Interface_H__ */
