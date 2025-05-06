//==============================================================================
//
// Title:       ServerCommands.h
// Purpose:     A short description of the interface.
//
// Created on:  15.12.2021 at 13:19:56 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __ServerCommands_H__
#define __ServerCommands_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"

//==============================================================================
// Constants
#define MAX_PARSERS_NUM 128

#define CMD_PRIME_SINGLE_FULLINFO "PSMCU:SINGLE:FULLINFO"
#define CMD_ALIAS_SINGLE_FULLINFO "IST:SINGLE:FULLINFO"

#define CMD_PRIME_SINGLE_DAC_SET "PSMCU:SINGLE:DAC:SET"
#define CMD_ALIAS_SINGLE_DAC_SET "IST:SINGLE:DAC:SET"

#define CMD_PRIME_SINGLE_DAC_RAWSET "PSMCU:SINGLE:DAC:RAWSET"
#define CMD_ALIAS_SINGLE_DAC_RAWSET "IST:SINGLE:DAC:RAWSET"
		
#define CMD_PRIME_SINGLE_DAC_FAST_SET "PSMCU:SINGLE:DAC:FAST:SET"
#define CMD_ALIAS_SINGLE_DAC_FAST_SET "IST:SINGLE:DAC:FAST:SET"

#define CMD_PRIME_SINGLE_DAC_FAST_RAWSET "PSMCU:SINGLE:DAC:FAST:RAWSET"
#define CMD_ALIAS_SINGLE_DAC_FAST_RAWSET "IST:SINGLE:DAC:FAST:RAWSET"

#define CMD_PRIME_SINGLE_ADC_GET "PSMCU:SINGLE:ADC:GET"
#define CMD_ALIAS_SINGLE_ADC_GET "IST:SINGLE:ADC:GET"

#define CMD_PRIME_SINGLE_ADC_RAWGET "PSMCU:SINGLE:ADC:RAWGET"
#define CMD_ALIAS_SINGLE_ADC_RAWGET "IST:SINGLE:ADC:RAWGET"

#define CMD_PRIME_SINGLE_ADC_RESET "PSMCU:SINGLE:ADC:RESET"
#define CMD_ALIAS_SINGLE_ADC_RESET "IST:SINGLE:ADC:RESET"

#define CMD_PRIME_SINGLE_ALLREGS_GET "PSMCU:SINGLE:ALLREGS:GET"
#define CMD_ALIAS_SINGLE_ALLREGS_GET "IST:SINGLE:ALLREGS:GET"

#define CMD_PRIME_SINGLE_OUTREG_SET "PSMCU:SINGLE:OUTREGS:SET"
#define CMD_ALIAS_SINGLE_OUTREG_SET "IST:SINGLE:OUTREGS:SET"

#define CMD_PRIME_SINGLE_INTERLOCK_RESTORE "PSMCU:SINGLE:INTERLOCK:RESTORE"
#define CMD_ALIAS_SINGLE_INTERLOCK_RESTORE "IST:SINGLE:INTERLOCK:RESTORE"

#define CMD_PRIME_SINGLE_INTERLOCK_DROP "PSMCU:SINGLE:INTERLOCK:DROP"
#define CMD_ALIAS_SINGLE_INTERLOCK_DROP "IST:SINGLE:INTERLOCK:DROP"

#define CMD_PRIME_SINGLE_FORCE_ON "PSMCU:SINGLE:FORCE:ON"
#define CMD_ALIAS_SINGLE_FORCE_ON "IST:SINGLE:FORCE:ON"

#define CMD_PRIME_SINGLE_FORCE_OFF "PSMCU:SINGLE:FORCE:OFF"
#define CMD_ALIAS_SINGLE_FORCE_OFF "IST:SINGLE:FORCE:OFF"

#define CMD_PRIME_SINGLE_PERMISSION_ON "PSMCU:SINGLE:PERMISSION:ON"
#define CMD_ALIAS_SINGLE_PERMISSION_ON "IST:SINGLE:PERMISSION:ON"

#define CMD_PRIME_SINGLE_PERMISSION_OFF "PSMCU:SINGLE:PERMISSION:OFF"
#define CMD_ALIAS_SINGLE_PERMISSION_OFF "IST:SINGLE:PERMISSION:OFF"

#define CMD_PRIME_SINGLE_STATUS_GET "PSMCU:SINGLE:STATUS:GET"
#define CMD_ALIAS_SINGLE_STATUS_GET "IST:SINGLE:STATUS:GET"


#define CMD_PRIME_SINGLE_DEVNAME_GET "PSMCU:SINGLE:DEVNAME:GET"
#define CMD_ALIAS_SINGLE_DEVNAME_GET "IST:SINGLE:DEVNAME:GET"

#define CMD_PRIME_SINGLE_ADCNAME_GET "PSMCU:SINGLE:ADCNAME:GET"
#define CMD_ALIAS_SINGLE_ADCNAME_GET "IST:SINGLE:ADCNAME:GET"

#define CMD_PRIME_SINGLE_DACNAME_GET "PSMCU:SINGLE:DACNAME:GET"
#define CMD_ALIAS_SINGLE_DACNAME_GET "IST:SINGLE:DACNAME:GET"

#define CMD_PRIME_SINGLE_INREGNAME_GET "PSMCU:SINGLE:INREGNAME:GET"
#define CMD_ALIAS_SINGLE_INREGNAME_GET "IST:SINGLE:INREGNAME:GET"

#define CMD_PRIME_SINGLE_OUTREGNAME_GET "PSMCU:SINGLE:OUTREGNAME:GET"
#define CMD_ALIAS_SINGLE_OUTREGNAME_GET "IST:SINGLE:OUTREGNAME:GET"

#define CMD_PRIME_ALL_ADC_RESET "PSMCU:ALL:ADC:RESET"
#define CMD_ALIAS_ALL_ADC_RESET "IST:ALL:ADC:RESET"

#define CMD_PRIME_ALL_ZERO_DAC "PSMCU:ALL:ZERODAC"
#define CMD_ALIAS_ALL_ZERO_DAC "IST:ALL:ZERODAC"
		
#define CMD_PRIME_ALL_FORCE_OFF "PSMCU:ALL:FORCE:OFF"
#define CMD_ALIAS_ALL_FORCE_OFF "IST:ALL:FORCE:OFF"

#define CMD_PRIME_ALL_PERMISSION_OFF "PSMCU:ALL:PERMISSION:OFF"
#define CMD_ALIAS_ALL_PERMISSION_OFF "IST:ALL:PERMISSION:OFF"

// Server data aquisition
#define CMD_PRIME_CANGW_STATUS_GET "PSMCU:CANGW:STATUS:GET"
#define CMD_ALIAS_CANGW_STATUS_GET "IST:CANGW:STATUS:GET"
		
#define CMD_PRIME_SERVER_NAME_GET "PSMCU:SERVER:NAME:GET"
#define CMD_ALIAS_SERVER_NAME_GET "IST:SERVER:NAME:GET"
		
#define CMD_PRIME_CANGW_DEVICES_NUM_GET "PSMCU:DEVNUM:GET"
#define CMD_ALIAS_CANGW_DEVICES_NUM_GET "IST:DEVNUM:GET"
		
#define CMD_PRIME_ID_GET "PSMCU:NAME2ID:GET"
#define CMD_ALIAS_ID_GET "IST:NAME2ID:GET"

// Error status commands
#define CMD_PRIME_SINGLE_ERR_SET "PSMCU:SINGLE:ERROR:SET"
#define CMD_ALIAS_SINGLE_ERR_SET "IST:SINGLE:ERROR:SET"

#define CMD_PRIME_SINGLE_ERR_CLEAR "PSMCU:SINGLE:ERROR:CLEAR"
#define CMD_ALIAS_SINGLE_ERR_CLEAR "IST:SINGLE:ERROR:CLEAR"

#define CMD_PRIME_SINGLE_ERR_GET "PSMCU:SINGLE:ERROR:GET"
#define CMD_ALIAS_SINGLE_ERR_GET "IST:SINGLE:ERROR:GET"

#define CMD_PRIME_ALL_ERR_SET "PSMCU:ALL:ERROR:SET"
#define CMD_ALIAS_ALL_ERR_SET "IST:ALL:ERROR:SET"

#define CMD_PRIME_ALL_ERR_CLEAR "PSMCU:ALL:ERROR:CLEAR"
#define CMD_ALIAS_ALL_ERR_CLEAR "IST:ALL:ERROR:CLEAR"
		
// Debug
#define CMD_PRIME_DBG_RST_CGW_CONN "PSMCU:DBG:RSTCGWCONN"
#define CMD_ALIAS_DBG_RST_CGW_CONN "IST:DBG:RSTCGWCONN"
//==============================================================================
// Types
typedef int (*parserFunciton)(char *commandBody, char *answerBuffer, char *ip);
//==============================================================================
// External variables

//==============================================================================
// Global functions
void InitCommandParsers(void);
void ReleaseCommandParsers(void);
void registerCommandParser(char * command, char *alias, parserFunciton parser);

parserFunciton getCommandparser(char *command);

void dataExchFunc(unsigned handle, char *ip);

// Commands parsers (cmdBody, answerBuffer, ip)

int cmdParserSingleGetFullInfo(char *, char *, char *);
int cmdParserSingleDacSet(char *, char *, char *);
int cmdParserSingleDacRawSet(char *, char *, char *);

int cmdParserSingleDacSlowSet(char *, char *, char *);
int cmdParserSingleDacSlowRawSet(char *, char *, char *);

int cmdParserSingleAdcGet(char *, char *, char *);
int cmdParserSingleAdcRawGet(char *, char *, char *);
int cmdParserSingleAdcReset(char *, char *, char *);
int cmdParserSingleAllRegistersGet(char *, char *, char *);
int cmdParserSingleOutRegistersSet(char *, char *, char *);
int cmdParserSingleInterlockRestore(char *, char *, char *);
int cmdParserSingleInterlockDrop(char *, char *, char *);
int cmdParserSingleForceOn(char *, char *, char *);
int cmdParserSingleForceOff(char *, char *, char *);
int cmdParserSinglePermissionOn(char *, char *, char *);
int cmdParserSinglePermissionOff(char *, char *, char *);
int cmdParserSingleStatusGet(char *, char *, char *);
int cmdParserSingleDeviceNameGet(char *, char *, char *);
int cmdParserSingleAdcNameGet(char *, char *, char *);
int cmdParserSingleDacNameGet(char *, char *, char *);
int cmdParserSingleInregNameGet(char *, char *, char *);
int cmdParserSingleOutRegNameGet(char *, char *, char *);
int cmdParserAllAdcReset(char *, char *, char *);
int cmdParserAllForceOff(char *, char *, char *);		
int cmdParserAllZeroDac(char *, char *, char *);
int cmdParserAllPermissionOff(char *, char *, char *);	
int cmdParserCangwStatusGet(char *, char *, char *);
int cmdParserServerNameGet(char *, char *, char *);

int cmdParserDevicesNumGet(char *, char *, char *);	
int cmdParserName2IdGet(char *, char *, char *);

// Commapnds parsers for error status
int cmdParserSingleErrorSet(char *, char *, char *);
int cmdParserSingleErrorGet(char *, char *, char *);
int cmdParserSingleErrorClear(char *, char *, char *);
int cmdParserAllErrorSet(char *, char *, char *);
int cmdParserAllErrorClear(char *, char *, char *);

// Debug commends
int cmdParseDbgCgwReconnectionReset(char *, char *, char *); 

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ServerCommands_H__ */
