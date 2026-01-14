//==============================================================================
//
// Title:       CGW_PSMCU_Interface.c
// Purpose:     A short description of the implementation.
//
// Created on:  18.11.2021 at 13:59:35 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "cangw.h"
#include "CGW_PSMCU_Interface.h"
#include "CGW_Devices.h"
#include "CGW_Connection.h"
#include "psMcuProtocol.h"
#include "MessageStack.h"
#include "TimeMarkers.h" 


// Device registration
int cgwPsMcu_RegisterDevice(cgw_devices_t * devKit,
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
							float slowDacMaxVoltageStep,
							double contactorDelay)
{
	cgwPsMcu_Information_t * deviceInfo;
	int i;
	unsigned int code1, code2;
	
	
	//deviceInfo = (cgwPsMcu_Information_t*)devices->parameters[number];  
	deviceInfo = malloc(sizeof(cgwPsMcu_Information_t));
	memset(deviceInfo, 0, sizeof(cgwPsMcu_Information_t));
	
	if (!deviceInfo)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_RegisterDevice /-/: Error! Unable to allocate memory for the device parameters container.");
		return -1;	
	}
	
	deviceInfo->deviceID = deviceID;
	strcpy(deviceInfo->deviceName, deviceName);
	
	for (i=0; i<PSMCU_ADC_CHANNELS_NUM; i++) {
		strcpy(deviceInfo->AdcChannelsNames[i], adcChannelsNames[i]);
		deviceInfo->AdcCoefficients[i][0] = AdcCoefficients[i][0];
		deviceInfo->AdcCoefficients[i][1] = AdcCoefficients[i][1]; 
	}
	
	for (i=0; i<PSMCU_DAC_CHANNELS_NUM; i++) {
		strcpy(deviceInfo->DacChannelsNames[i], dacChannelsNames[i]); 
		deviceInfo->DacCoefficients[i][0] = DacCoefficients[i][0];
		deviceInfo->DacCoefficients[i][1] = DacCoefficients[i][1];
		deviceInfo->targetDACcode[i] = 0x8000;
		
		deviceInfo->DAClastTimeWrite[i] = clock() * 1000 / CLOCKS_PER_SEC;
	}
	
	for (i=0; i<PSMCU_INPUT_REGISTERS_NUM; i++) {
		strcpy(deviceInfo->InputRegistersNames[i], inputRegistersNames[i]); 
	}
	
	for (i=0; i<PSMCU_OUTPUT_REGISTERS_NUM; i++) {
		strcpy(deviceInfo->OutputRegistersNames[i], outputRegistersNames[i]); 
	}
	
	cgwPsMcu_DACVoltageToCode(0., &code1);
	cgwPsMcu_DACVoltageToCode(slowDacMaxVoltageStep, &code2);
	deviceInfo->maxDACcodeStep = code2 - code1;
	deviceInfo->DACslowUpdateDelta = slowDacTimeDelta_ms;  
	
	deviceInfo->globalDeviceIndex = globalDeviceIndex;
	deviceInfo->cgwIndex = cgwIndex;
	deviceInfo->p_devKit = devKit;
	
	deviceInfo->localIndex = cgwRegisterDevice(devKit, deviceID, deviceInfo, deviceInfo->deviceName, 
											   cgwPsMcu_MessageHook, userHookFunc, deviceDowntimeLimit, 
											   deviceAwakeCallback, deviceLostCallback, cgwPsMcu_updateCallback);
	
	return 0;
}


void cgwPsMcu_updateCallback(void * parameters) {
	clock_t curr_time_ms, delta_ms;
	cgwPsMcu_Information_t * psmcuParams;
	int chIndex;
	unsigned int new_code, current_code, target_code, max_code_step;
	
	curr_time_ms = clock() * 1000 / CLOCKS_PER_SEC;
	psmcuParams = parameters;
	
	for (chIndex=0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) {
		if (!psmcuParams->usingDACslowMode[chIndex] || cgwConnectionBroken[psmcuParams->cgwIndex]) {
			// If not using the slow mode or a connection to the CanGw device is not established, just update the current time.
			psmcuParams->DAClastTimeWrite[chIndex] = curr_time_ms;	
		} else {
			delta_ms = curr_time_ms - psmcuParams->DAClastTimeWrite[chIndex];
			
			if (delta_ms > psmcuParams->DACslowUpdateDelta && psmcuParams->DACcodeConfirmed[chIndex]) {
				
				current_code = psmcuParams->DACcode[chIndex];
				target_code = psmcuParams->targetDACcode[chIndex];
				max_code_step = psmcuParams->maxDACcodeStep;
				
				if (current_code == target_code) {
					psmcuParams->usingDACslowMode[chIndex] = 0;		
				} else {
					if (target_code > current_code) {
						if (target_code - current_code > max_code_step)
							new_code = current_code + max_code_step;
						else
							new_code = target_code;
					} else {
						// target_code <= current_code
						if (current_code - target_code > max_code_step)
							new_code = current_code - max_code_step;
						else
							new_code = target_code;
					}

					if (cgwPsMcu_DACWriteCode(psmcuParams->cgwIndex, psmcuParams->deviceID, chIndex, new_code) > 0) {
						psmcuParams->DACcodeConfirmed[chIndex] = 0; 
						psmcuParams->DAClastTimeWrite[chIndex] = curr_time_ms;
						cgwPsMcu_DACReadCode(psmcuParams->cgwIndex, psmcuParams->deviceID, chIndex);
					}
				}

			}
		}
	}
}

//============================================================================== 
//============================================================================== 
// Receiving messages
//==============================================================================
//==============================================================================

// Message hook
void cgwPsMcu_MessageHook(cangw_msg_t msg, void * deviceData)
{
	cgwPsMcu_Information_t *psmcuData = deviceData;

	switch(msg.data[0])
	{
		// Receiving measurements after sending cgwPsMcu_MultiChannelConfig
		case 0x01:
		// Receiving measurements after sending cgwPsMcu_SingleChannelRequest
		case 0x02:
		// Receiving measurements after sending cgwPsMcu_MultiChannelRequest
		case 0x03:
		// Receiving measurements after sending cgwPsMcu_LoopBufferRequest
		case 0x04:
			cgwPsMcu_Hook_ADC(msg, psmcuData);  
			break;
		// Recieving DAC setup after sending cgwPsMcu_DACReadCode
		case 0x90:
			cgwPsMcu_Hook_DAC(msg, psmcuData);  
			break;
		// Tables closing
		case 0xF5:
			msAddMsg(msGMS(), "0xF5");
			cgwPsMcu_Hook_F5(msg, psmcuData);
			break;
		// Tables request 
		case 0xF6:
			msAddMsg(msGMS(), "0xF6");
			cgwPsMcu_Hook_F6(msg, psmcuData); 
			break;
		// Registers data. cgwPsMcu_RegisterRequest
		case 0xF8:
			cgwPsMcu_Hook_Register(msg, psmcuData);  
			break;
		// Status. cgwPsMcu_StatusRequest
		case 0xFE:
			cgwPsMcu_Hook_Status(msg, psmcuData); 
			break;
		// Attributes. cgwPsMcu_AttributesRequest
		case 0xFF:
			cgwPsMcu_Hook_Attributes(msg, psmcuData);
			break;
		default:
			msAddMsg(msGMS(),"%s [PSMCU] [%s] Device [%d (0x%X)] \"%s\" - Unnknown command 0x%X",
			        TimeStamp(0),
					psmcuData->p_devKit->blockName,
					cgwDevicess_DeviceIDFromMessageID(msg.id), 
					cgwDevicess_DeviceIDFromMessageID(msg.id),
				    psmcuData->name,
					msg.data[0]);
			break;
	}
}

// 			Hooks
// ADC (0x01 - 0x04) 
void cgwPsMcu_Hook_ADC(cangw_msg_t msg, cgwPsMcu_Information_t * info)
{
	static unsigned char channel;
	static unsigned long data;
	unsigned int mask, mask_value, median;

	channel = msg.data[1];
	
	mask_value = mask = 0xFFFFFF;
	median = 0x7FFFFF;
	
	if (channel >= PSMCU_ADC_CHANNELS_NUM)
	{
		msAddMsg(msGMS(),"%s [PSMCU] [%s] Device [%d (0x%X)] \"%s\" sends unrecognized messages (wrong channel - %d).",
			    TimeStamp(0),
				info->p_devKit->blockName,
				cgwDevicess_DeviceIDFromMessageID(msg.id),
				cgwDevicess_DeviceIDFromMessageID(msg.id),
				info->name,
				channel);
		return;
	}
	
	data = (msg.data[4] << 16) | (msg.data[3] << 8) | msg.data[2];
	data = data & mask;
	
	if (data <= median)
	{
		info->ChannelVoltage[channel] = 10.0 * (float)data / (float)median;	
	} 
	else {
		info->ChannelVoltage[channel] = -10.0 + 10.0 * (float)(data - median) / (float)(mask_value - median);	
	}

	info->ChannelData[channel] = data;
}


// DAC
void cgwPsMcu_Hook_DAC(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	unsigned int data;
	unsigned char channel;
	
	if (msg.data[0] < 0x90 || msg.data[0] >= 0x90 + PSMCU_DAC_CHANNELS_NUM) {
		msAddMsg(msGMS(),"/-/ cgwPsMcu_Hook_DAC /-/: Hooked message with code out of range 0x90..0x%X. Got 0x%02X", msg.data[0], 0x90 + PSMCU_DAC_CHANNELS_NUM - 1); 
		return;		
	}
	
	channel = msg.data[0] - 0x90;
	
	data = (msg.data[1] << 8) | msg.data[2];
	if (data > 0xFFFF)
	{
		msAddMsg(msGMS(),"%s [PSMCU] [%s] Device [%d (0x%X)] \"%s\" sends unrecognized messages (wrong DAC code - 0x%04X).",
			    TimeStamp(0),
				info->p_devKit->blockName,
				cgwDevicess_DeviceIDFromMessageID(msg.id),
				cgwDevicess_DeviceIDFromMessageID(msg.id),
				info->name,
				data);
		return;		
	}
	
	info->DACcode[channel] = data;
	info->DACcodeConfirmed[channel] = 1;
	
	if (data >= 0x8000) {
		info->DACvoltage[channel] = 9.9997 * (float)(data - 0x8000) / (float)(0xFFFF - 0x8000); 	
	} else {
		info->DACvoltage[channel] = -(0.0003 + 9.9997 * (float)(0x7FFF - data) / (float)0x7FFF);
	}
}

// Tables
void cgwPsMcu_Hook_F5(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	
}

void cgwPsMcu_Hook_F6(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	
}

// Registers
void cgwPsMcu_Hook_Register(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	info->OutputRegisterData = msg.data[1];
	info->InputRegisterData = msg.data[2];
	info->OutputRegistersConfirmed = 1;
}

// Status
void cgwPsMcu_Hook_Status(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	info->DeviceMode 	= msg.data[1];
	info->Label 		= msg.data[2];
	info->pADC			= msg.data[3] | (msg.data[4] << 8);
	info->FileIdent		= msg.data[5];
	info->pDAC			= msg.data[6] | (msg.data[7] << 8);
	/*if (info->pDAC > 4095)
	{
		TimeStamp(stime);
		msAddMsg(msGMS(),"%s Device [%d (0x%X)] \"%s\". Status request returned wrong pointer of the DAC loop buffer (0x%X). Substituted by 0x0.", TimeStamp(0),
			cgwDevicess_DeviceIDFromMessageID(msg.id),cgwDevicess_DeviceIDFromMessageID(msg.id),info->name,
			info->pDAC);
		info->pDAC = 0;
	}
	if (info->pADC > 4095)
	{
		msAddMsg(msGMS(),"%s Device [%d (0x%X)] \"%s\". Status request returned wrong pointer of the ADC loop buffer (0x%X). Substituted by 0x0.", TimeStamp(0),
			cgwDevicess_DeviceIDFromMessageID(msg.id),cgwDevicess_DeviceIDFromMessageID(msg.id),info->name,
			info->pADC);
		info->pADC = 0;
	}  */
}

// Attributes
void cgwPsMcu_Hook_Attributes(cangw_msg_t msg,cgwPsMcu_Information_t * info)
{
	info->DeviceCode = msg.data[1];
	if (info->DeviceCode != CGW_PSMCU_DEVICE_CODE)
	{
		msAddMsg(msGMS(),"%s [PSMCU] [%s] Device [%d (0x%X)] \"%s\". The registered device code {%d} doesn't match the received {%d - \"%s\"}",
			TimeStamp(0),
			info->p_devKit->blockName,
			cgwDevicess_DeviceIDFromMessageID(msg.id),
			cgwDevicess_DeviceIDFromMessageID(msg.id),
			info->name,
			CGW_PSMCU_DEVICE_CODE,
			info->DeviceCode,
			cgwDevices_GetNameByDeviceCode(info->DeviceCode));
	}
	info->HWversion = msg.data[2];
	info->SWversion = msg.data[3];
	info->Reason = msg.data[4];
	
	// Setting the output registers confirmation flag to zero
	// to make sure that the output registers are known before changing them
	info->OutputRegistersConfirmed = 0;
}


//============================================================================== 
//============================================================================== 
// Sending commands
//============================================================================== 
//============================================================================== 

// Message 00
int cgwPsMcu_Stop(int cgwIndex, unsigned char deviceId, int broadcast)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63 && broadcast==0)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_Stop /-/: Error! deviceId must be less than or equal to 0x3F (The command is not broadcast). The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));

	if (broadcast==0)
		msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	else
		msg.id = psMcuProtocol_IDgen(5, deviceId, 0);
	
	msg.len = psMcuProtocol_Stop(msg.data);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_Stop");
	return sending;
	
}						   

// Message 01
int cgwPsMcu_MultiChannelConfig(int cgwIndex, unsigned char deviceId,unsigned char ChBeg, unsigned char ChEnd, psMcuProtocol_time_t time,psMcuProtocol_mode_t mode, unsigned char label)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_MultiChannelConfig /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
							
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_MultiChannelConfig(msg.data, ChBeg, ChEnd, time, mode, label);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_MultiChannelConfig"); 
	return sending;	
}

// Message 02
int cgwPsMcu_SingleAdcChannelRequest(int cgwIndex, unsigned char deviceId, unsigned char Channel, psMcuProtocol_time_t time, psMcuProtocol_mode_t mode)
{
	cangw_msg_t msg;
	int sending;
	
	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_SingleAdcChannelRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	if(Channel > PSMCU_ADC_CHANNELS_NUM - 1)
	{
		msAddMsg(
			msGMS(),
			"/-/ cgwPsMcu_SingleAdcChannelRequest /-/: Error! Channel must be in range from 0 to %d, got %d. The command is ignored.",
			PSMCU_ADC_CHANNELS_NUM - 1,
			Channel);
		return CANGW_ERR;
	}
	
	memset(&msg, 0, sizeof(msg));
							
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_SingleAdcChannelRequest(msg.data, Channel, time,mode);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_SingleChannelRequest"); 
	return sending;
}

// Message 03
int cgwPsMcu_MultiChannelRequest(int cgwIndex, unsigned char deviceId, unsigned char Channel)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_MultiChannelRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));

	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_MultiChannelRequest(msg.data, Channel);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_MultiChannelRequest"); 
	return sending;	
}

// Message 04
int cgwPsMcu_LoopBufferRequest(int cgwIndex, unsigned char deviceId, unsigned int pADC)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_LoopBufferRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
	
	msg.id = psMcuProtocol_IDgen(6,deviceId,0);
	msg.len = psMcuProtocol_LoopBufferRequest(msg.data,pADC);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg,CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_LoopBufferRequest"); 
	return sending;	
}

// Messages 80
int cgwPsMcu_DACWriteCode(int cgwIndex, unsigned char deviceId, unsigned int channel, unsigned int Code)
{
	// "files" are currently not supported
	cangw_msg_t msg;
	int sending;
	
	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_DACWriteCode /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}
	
	if (channel > PSMCU_DAC_CHANNELS_NUM - 1) {
		msAddMsg(
			msGMS(),
			"/-/ cgwPsMcu_DACWriteCode /-/: Error! channel must be in range from 0 to %d, got %d. The command is ignored.",
			PSMCU_DAC_CHANNELS_NUM - 1,
			channel);
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_WriteDAC(msg.data, channel, Code << 16);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_DACWriteCode"); 
	return sending;
}


int cgwPsMcu_DACWriteVoltage(int cgwIndex, unsigned char deviceId, unsigned char channel, float voltage)
{
	unsigned int code;

	if (cgwPsMcu_DACVoltageToCode(voltage, &code) != 0) return CANGW_ERR;

	return cgwPsMcu_DACWriteCode(cgwIndex, deviceId, channel, code);	
}


int cgwPsMcu_DACVoltageToCode(float voltage, unsigned int *code) {
	if (voltage > 9.9997 || voltage < -10.0)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_DACVoltageToCode /-/: Error! Voltage must be in range from -10.0 to 9.9997 [V]");
		return CANGW_ERR;
	}

	if (voltage >= 0) {
		(*code) = 0x8000 + (unsigned int)(voltage / 9.9997 * (0xFFFF - 0x8000) + 0.5);	
	} else {
		(*code) = (unsigned int)((10. + voltage) / 10. * 0x8000 + 0.5);	
	}
	
	return 0;
}

// Message 90
int cgwPsMcu_DACReadCode(int cgwIndex, unsigned char deviceId, unsigned char channel)			
{
	cangw_msg_t msg;
	int sending;
	
	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_DACReadCode /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}
	
	if (channel > PSMCU_DAC_CHANNELS_NUM - 1) {
		msAddMsg(
			msGMS(),
			"/-/ cgwPsMcu_DACReadCode /-/: Error! channel must be in range from 0 to %d, got %d. The command is ignored.",
			PSMCU_DAC_CHANNELS_NUM - 1,
			channel);
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_ReadDAC(msg.data, channel);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_DACReadCode"); 
	return sending;
}

// Tables

// Message F3 
void cgwPsMcu_CreateTable(void)	
{
	return;
}

// Message F4 
void cgwPsMcu_WriteToTable(void)		
{
	return;
}

void cgwPsMcu_CloseTable(void)				// Pocket F5
{
	return;
}

void cgwPsMcu_TableRequest(void)			// Pocket F6
{
	return;
}

void cgwPsMcu_TableExecute(void)			// Pocket F7
{
	return;
}

// Message F8
int cgwPsMcu_RegistersRequest(int cgwIndex, unsigned char deviceId)
{
	cangw_msg_t msg;
	int sending;
	
	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_RegistersRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
	
	msg.id = psMcuProtocol_IDgen(6,deviceId,0);
	msg.len = psMcuProtocol_RegistersRequest(msg.data);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_RegistersRequest"); 
	return sending;	
}

// Message F9
int cgwPsMcu_WriteToOutputRegister(int cgwIndex, unsigned char deviceId, unsigned char OutputRegister)
{
	cangw_msg_t msg;
	int sending;
	
	if(deviceId > 63)
	{
		msAddMsg(msGMS(), "/-/ cgwPsMcu_WriteToOutputRegister /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_WriteToOutputRegister(msg.data, OutputRegister);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_WriteToOutputRegister"); 
	return sending;			
}

// Message FE
int cgwPsMcu_StatusRequest(int cgwIndex, unsigned char deviceId)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_StatusRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg, 0, sizeof(msg));
							
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_StatusRequest(msg.data);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_StatusRequest"); 
	return sending;	
}

// Message FF
int cgwPsMcu_AttributesRequest(int cgwIndex, unsigned char deviceId)
{
	cangw_msg_t msg;
	int sending;

	if(deviceId > 63)
	{
		msAddMsg(msGMS(),"/-/ cgwPsMcu_AttributesRequest /-/: Error! deviceId must be less than or equal to 0x3F. The command is ignored.");
		return CANGW_ERR;
	}

	memset(&msg,0,sizeof(msg));
							
	
	msg.id = psMcuProtocol_IDgen(6, deviceId, 0);
	msg.len = psMcuProtocol_AttributesRequest(msg.data);
	msg.flags = CANGW_MSG_XMIT;
	
	sending = cgwConnection_Send(cgwIndex, msg, CGW_PSMCU_DEFAULT_TIMEOUT, "cgwPsMcu_AttributesRequest");
	return sending;		
}
