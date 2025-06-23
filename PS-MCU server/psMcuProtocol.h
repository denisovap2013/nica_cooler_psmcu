//==============================================================================
//
// Title:       psMcuProtocol.h
// Purpose:     Functions for forming the data package acceptable by CANBUS protocol. 
//
// Created on:  22.11.2021 at 13:20:24 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __psMcuProtocol_H__
#define __psMcuProtocol_H__

#define PSMCU_ADC_CHANNELS_NUM 5
#define PSMCU_DAC_CHANNELS_NUM 1
#define PSMCU_INPUT_REGISTERS_NUM 4
#define PSMCU_OUTPUT_REGISTERS_NUM 4 

typedef enum psMcuProtocol_mode
{
	PSMCU_MODE_SINGLE 		= (0<<4),
	PSMCU_MODE_CONTINUOUS 	= (1<<4),
	PSMCU_MODE_TAKEOUT		= (1<<5)
} psMcuProtocol_mode_t;

typedef enum psMcuProtocol_time
{
	PSMCU_TIME_1MS = 0,
	PSMCU_TIME_2MS = 1,
	PSMCU_TIME_5MS = 2,
	PSMCU_TIME_10MS = 3,
	PSMCU_TIME_20MS = 4,
	PSMCU_TIME_40MS = 5,
	PSMCU_TIME_80MS = 6,
	PSMCU_TIME_160MS = 7
} psMcuProtocol_time_t;

// ID generator
unsigned long psMcuProtocol_IDgen(unsigned char priority, unsigned char address, unsigned char reserved);

// ID unpacking
unsigned char psMcuProtocol_PriorityFromID(unsigned long ID);
unsigned char psMcuProtocol_NumberFromID(unsigned long ID);
unsigned char psMcuProtocol_FlagsFromID(unsigned long ID);

// Message 00 
unsigned int psMcuProtocol_Stop(unsigned char * data);

// Message 01 
unsigned int psMcuProtocol_MultiChannelConfig(unsigned char * data,
	unsigned char ChBeg, unsigned char ChEnd, psMcuProtocol_time_t Time, psMcuProtocol_mode_t Mode, unsigned char Label);

// Message 02
unsigned int psMcuProtocol_SingleAdcChannelRequest(unsigned char * data,
	unsigned char Channel, psMcuProtocol_time_t Time, psMcuProtocol_mode_t Mode);

// Message 03 
unsigned int psMcuProtocol_MultiChannelRequest(unsigned char * data, unsigned char Channel);

// Message 04 
unsigned int psMcuProtocol_LoopBufferRequest(unsigned char * data, unsigned int Pointer);

// Messages 80-87
unsigned int psMcuProtocol_WriteDAC(unsigned char * data, unsigned char channel, unsigned long code);

// Messages 90-97
unsigned int psMcuProtocol_ReadDAC(unsigned char * data, unsigned char channel);

// Message F8
unsigned int psMcuProtocol_RegistersRequest(unsigned char * data);

// Message F9
unsigned int psMcuProtocol_WriteToOutputRegister(unsigned char * data, unsigned char OutputRegister);

// Message FE
unsigned int psMcuProtocol_StatusRequest(unsigned char * data);

// Message FF
unsigned int psMcuProtocol_AttributesRequest(unsigned char * data);


#endif  /* ndef __psMcuProtocol_H__ */
