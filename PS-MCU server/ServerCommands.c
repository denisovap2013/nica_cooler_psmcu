//==============================================================================
//
// Title:       ServerCommands.c
// Purpose:     A short description of the implementation.
//
// Created on:  15.12.2021 at 13:19:56 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <userint.h>
#include <ansi_c.h>
#include <tcpsupp.h>
#include "ServerCommands.h"

#include "TimeMarkers.h"
//#include "TCP_Connection.h"
#include "CGW_Connection.h"
#include "MessageStack.h"
#include "psMcuProtocol.h"
#include "CGW_Devices.h"
#include "CGW_PSMCU_Interface.h"
#include "Logging.h"

#include "ServerData.h"
#include "ServerConfigData.h"
#include "hash_map.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
int processUserCommand(char *userCmd, char *answerBuffer, char *ip);
void toupperCase(char *text);
void logNotification(char *ip, char *message, ...);

//==============================================================================
// Global variables
parserFunciton RegisteredParsers[MAX_PARSERS_NUM];

int registeredParsersNum = 0;
int parsersInitialized = 0;
map_int_t commandsHashMap;
map_int_t devNamesHashMap;

//==============================================================================
// Global functions

#define MAX_RECEIVED_BYTES 3000

void InitCommandParsers(void) {
	int globalDeviceIndex;
	char devName[128];
	if (parsersInitialized) return;

	map_init(&commandsHashMap);
	map_init(&devNamesHashMap); 
	registeredParsersNum = 0;
	parsersInitialized = 1;	
	
    registerCommandParser(CMD_PRIME_SINGLE_FULLINFO, CMD_ALIAS_SINGLE_FULLINFO, cmdParserSingleGetFullInfo);
    registerCommandParser(CMD_PRIME_SINGLE_DAC_SET, CMD_ALIAS_SINGLE_DAC_SET, cmdParserSingleDacSlowSet); 
    registerCommandParser(CMD_PRIME_SINGLE_DAC_RAWSET, CMD_ALIAS_SINGLE_DAC_RAWSET, cmdParserSingleDacSlowRawSet);
    registerCommandParser(CMD_PRIME_SINGLE_DAC_FAST_SET, CMD_ALIAS_SINGLE_DAC_FAST_SET, cmdParserSingleDacSet);
    registerCommandParser(CMD_PRIME_SINGLE_DAC_FAST_RAWSET, CMD_ALIAS_SINGLE_DAC_FAST_RAWSET, cmdParserSingleDacRawSet);
    registerCommandParser(CMD_PRIME_SINGLE_ADC_GET, CMD_ALIAS_SINGLE_ADC_GET, cmdParserSingleAdcGet);
    registerCommandParser(CMD_PRIME_SINGLE_ADC_RAWGET, CMD_ALIAS_SINGLE_ADC_RAWGET, cmdParserSingleAdcRawGet);
    registerCommandParser(CMD_PRIME_SINGLE_ADC_RESET, CMD_ALIAS_SINGLE_ADC_RESET, cmdParserSingleAdcReset);
    registerCommandParser(CMD_PRIME_SINGLE_ALLREGS_GET, CMD_ALIAS_SINGLE_ALLREGS_GET, cmdParserSingleAllRegistersGet);
    registerCommandParser(CMD_PRIME_SINGLE_OUTREG_SET, CMD_ALIAS_SINGLE_OUTREG_SET, cmdParserSingleOutRegistersSet);
    registerCommandParser(CMD_PRIME_SINGLE_INTERLOCK_RESTORE, CMD_ALIAS_SINGLE_INTERLOCK_RESTORE, cmdParserSingleInterlockRestore); 
    registerCommandParser(CMD_PRIME_SINGLE_INTERLOCK_DROP, CMD_ALIAS_SINGLE_INTERLOCK_DROP, cmdParserSingleInterlockDrop); 
    registerCommandParser(CMD_PRIME_SINGLE_FORCE_ON, CMD_ALIAS_SINGLE_FORCE_ON, cmdParserSingleForceOn);
    registerCommandParser(CMD_PRIME_SINGLE_FORCE_OFF, CMD_ALIAS_SINGLE_FORCE_OFF, cmdParserSingleForceOff);
    registerCommandParser(CMD_PRIME_SINGLE_PERMISSION_ON, CMD_ALIAS_SINGLE_PERMISSION_ON, cmdParserSinglePermissionOn);
    registerCommandParser(CMD_PRIME_SINGLE_PERMISSION_OFF, CMD_ALIAS_SINGLE_PERMISSION_OFF, cmdParserSinglePermissionOff);
    registerCommandParser(CMD_PRIME_SINGLE_STATUS_GET, CMD_ALIAS_SINGLE_STATUS_GET, cmdParserSingleStatusGet);
    registerCommandParser(CMD_PRIME_SINGLE_DEVNAME_GET, CMD_ALIAS_SINGLE_DEVNAME_GET, cmdParserSingleDeviceNameGet);
    registerCommandParser(CMD_PRIME_SINGLE_ADCNAME_GET, CMD_ALIAS_SINGLE_ADCNAME_GET, cmdParserSingleAdcNameGet);
    registerCommandParser(CMD_PRIME_SINGLE_DACNAME_GET, CMD_ALIAS_SINGLE_DACNAME_GET, cmdParserSingleDacNameGet);
    registerCommandParser(CMD_PRIME_SINGLE_INREGNAME_GET, CMD_ALIAS_SINGLE_INREGNAME_GET, cmdParserSingleInregNameGet);
    registerCommandParser(CMD_PRIME_SINGLE_OUTREGNAME_GET, CMD_ALIAS_SINGLE_OUTREGNAME_GET, cmdParserSingleOutRegNameGet);
    registerCommandParser(CMD_PRIME_ALL_ADC_RESET, CMD_ALIAS_ALL_ADC_RESET, cmdParserAllAdcReset);
    registerCommandParser(CMD_PRIME_ALL_ZERO_DAC, CMD_ALIAS_ALL_ZERO_DAC, cmdParserAllZeroDac);
    registerCommandParser(CMD_PRIME_ALL_FORCE_OFF, CMD_ALIAS_ALL_FORCE_OFF, cmdParserAllForceOff);
    registerCommandParser(CMD_PRIME_ALL_PERMISSION_OFF, CMD_ALIAS_ALL_PERMISSION_OFF, cmdParserAllPermissionOff);
    registerCommandParser(CMD_PRIME_CANGW_STATUS_GET, CMD_ALIAS_CANGW_STATUS_GET, cmdParserCangwStatusGet);
	registerCommandParser(CMD_PRIME_SERVER_NAME_GET, CMD_ALIAS_SERVER_NAME_GET, cmdParserServerNameGet);
	registerCommandParser(CMD_PRIME_CANGW_DEVICES_NUM_GET, CMD_ALIAS_CANGW_DEVICES_NUM_GET, cmdParserDevicesNumGet);    
	registerCommandParser(CMD_PRIME_ID_GET, CMD_ALIAS_ID_GET, cmdParserName2IdGet); 
	
	// Register error state commnds
	registerCommandParser(CMD_PRIME_SINGLE_ERR_SET, CMD_ALIAS_SINGLE_ERR_SET, cmdParserSingleErrorSet);
	registerCommandParser(CMD_PRIME_SINGLE_ERR_CLEAR, CMD_ALIAS_SINGLE_ERR_CLEAR, cmdParserSingleErrorClear);
	registerCommandParser(CMD_PRIME_SINGLE_ERR_GET, CMD_ALIAS_SINGLE_ERR_GET, cmdParserSingleErrorGet);
	registerCommandParser(CMD_PRIME_ALL_ERR_SET, CMD_ALIAS_ALL_ERR_SET, cmdParserAllErrorSet);
	registerCommandParser(CMD_PRIME_ALL_ERR_CLEAR, CMD_ALIAS_ALL_ERR_CLEAR, cmdParserAllErrorClear);
	
	// Debug commands
	registerCommandParser(CMD_PRIME_DBG_RST_CGW_CONN, CMD_ALIAS_DBG_RST_CGW_CONN, cmdParseDbgCgwReconnectionReset);
	
	// Prepare the devices names dicitonary
	for (globalDeviceIndex=0; globalDeviceIndex < CFG_PSMCU_DEVICES_NUM; globalDeviceIndex++) {
		strcpy(devName, CFG_PSMCU_DEVICES_NAMES[globalDeviceIndex]);
		toupperCase(devName);
		map_set(&devNamesHashMap, devName, globalDeviceIndex);	
	}
}

void ReleaseCommandParsers(void) {
	if (!parsersInitialized) return;

	map_deinit(&commandsHashMap);
	map_deinit(&devNamesHashMap); 
	registeredParsersNum = 0;
	parsersInitialized = 0;	
}

void registerCommandParser(char *command, char *alias, parserFunciton parser) {
	static char msg[256];
	if (registeredParsersNum >= MAX_PARSERS_NUM) { 
		sprintf(msg, "Unable to register a command '%s'. Maximum number of parsers (%d) exceeded.", command, MAX_PARSERS_NUM);
		MessagePopup("Internal application error.", msg);
		exit(0);
	}
	
	// Add command parser to the list and add mapping from the command name/alias
	// to the index of the added element in the list.
	RegisteredParsers[registeredParsersNum] = parser;
	map_set(&commandsHashMap, command, registeredParsersNum);
	if (alias)  map_set(&commandsHashMap, alias, registeredParsersNum);
	
	registeredParsersNum++;
}


void prepareTcpCommand(char *str, int bytes){
	int i, j;
	
	j = 0;
	for (i=0; i < bytes; i++) {
		if (str[i] != 0) str[j++] = str[i];
	}

	if (bytes == MAX_RECEIVED_BYTES) str[MAX_RECEIVED_BYTES-1] = 0;
	else str[j] = 0;
}


void toupperCase(char *text) {
	char *p;
	p = text;
	while (*p != 0) {
		*p = toupper(*p);
		p++;
	}
}


void dataExchFunc(unsigned handle, char *ip)
{
	static char command[MAX_RECEIVED_BYTES * 2] = "";
	static char subcommand[MAX_RECEIVED_BYTES * 2];
	static char buf[MAX_RECEIVED_BYTES];
	static char *lfp;
	static int byteRecv;
	static char answerBuffer[1024];
	static int checkInitialization = 1;

	// Check that all necessary initializations are done
	if (checkInitialization) {
		if (!parsersInitialized) {
			MessagePopup("Runtime Error", "Command parsers are not initialized");
			exit(0);
		}
		checkInitialization = 0;	
	}
	
	byteRecv = ServerTCPRead(handle, buf, MAX_RECEIVED_BYTES, 0);
	if ( byteRecv <= 0 )
	{
		logMessage("[SERVER CLIENT] Error occured while receiving messages from the client >> %s", GetTCPSystemErrorString());
		return;
	}
	
	prepareTcpCommand(buf, byteRecv);
	strcpy(command, buf);
	
	//printf("%s\n",command);
	
	// Selecting all incoming commands
	while ( (lfp = strchr(command, '\n')) != NULL ) {
		lfp[0] = 0;
		strcpy(subcommand, command);
		strcpy(command, lfp+1);
		processUserCommand(subcommand, answerBuffer, ip);
		if (answerBuffer[0] != 0) ServerTCPWrite(handle, answerBuffer, strlen(answerBuffer) + 1, 0);
	}
	
	// It's likely that extremly long strings are not commands
}


parserFunciton getCommandparser(char *command) {
	int *cmdIndex;
	char * symbol;
	
	symbol = command;
	while (*symbol != 0) {
	    *symbol = toupper(*symbol);
		symbol++;
	}
	cmdIndex = map_get(&commandsHashMap, command);
	if (cmdIndex) return RegisteredParsers[*cmdIndex];
	
	return 0;
}


int processUserCommand(char *userCmd, char *answerBuffer, char *ip) {
	char cmdName[256];
	int cursor;
	static char parserAnswer[1024];
	parserFunciton parser;
	int result;
	
	sscanf(userCmd, "%s%n", cmdName, &cursor);
	
	parser = getCommandparser(cmdName);
	
	answerBuffer[0] = 0;  // Empty string (no answer)
	parserAnswer[0] = 0;   
	
	if (parser) {
		result = parser(userCmd + cursor, parserAnswer, ip);
		
		// Check for errors
		if (result < 0) {
			logNotification(ip, "[ERROR] Incorrect command data: \"%s\"", userCmd);
			sprintf(answerBuffer, "!%s\n", userCmd); 
			return -1; // Error occurred
		}

		if (result == 0) return 0;  
		
		if (strlen(parserAnswer))
			sprintf(answerBuffer, "%s %s\n", cmdName, parserAnswer);

	} else {
		logNotification(ip, "[ERROR] Unknown command: \"%s\"", userCmd);
		sprintf(answerBuffer, "?%s\n", userCmd);	
	}
	
	return 1; // Command processed successfully (including "unknown" commands)
}


//////////////////////////////////
// PARSERS
//////////////////////////////////


void logNotification(char *ip, char *message, ...) {

	char buf[512];
	va_list arglist;

	va_start(arglist, message);
	vsprintf(buf, message, arglist);
	va_end(arglist);

	msAddMsg(msGMS(),"%s [CLIENT] [IP: %s] %s", TimeStamp(0), ip, buf);

}


int cmdParserSingleGetFullInfo(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d ", deviceIndex);
	if (getDeviceFullInfo(cgwIndex, deviceId, answerBuffer + strlen(answerBuffer)) < 0) return -1;

	return 1;
}


int cmdParserSingleDacSet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel;
	double dacValue;

	if (sscanf(commandBody, "%d %d %lf", &deviceIndex, &channel, &dacValue) != 3) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (setDacVoltage(cgwIndex, deviceId, channel, dacValue, 0, 0) < 0) return -1;
	
	return 1;
}


int cmdParserSingleDacRawSet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel;
	double dacValue;

	if (sscanf(commandBody, "%d %d %lf", &deviceIndex, &channel, &dacValue) != 3) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (setDacVoltage(cgwIndex, deviceId, channel, dacValue, 1, 0) < 0) return -1;
	
	return 1;
}


int cmdParserSingleDacSlowSet(char *commandBody, char *answerBuffer, char *ip) {
	
	int deviceIndex, cgwIndex, deviceId, channel;
	double dacValue;

	if (sscanf(commandBody, "%d %d %lf", &deviceIndex, &channel, &dacValue) != 3) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (setDacVoltage(cgwIndex, deviceId, channel, dacValue, 0, 1) < 0) return -1;
	
	return 1;	
}


int cmdParserSingleDacSlowRawSet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel;
	double dacValue;

	if (sscanf(commandBody, "%d %d %lf", &deviceIndex, &channel, &dacValue) != 3) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (setDacVoltage(cgwIndex, deviceId, channel, dacValue, 1, 1) < 0) return -1;
	
	return 1;
}


int cmdParserSingleAdcGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel; 
	double adcValue;
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &channel) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (getAdcVoltage(cgwIndex, deviceId, channel, &adcValue, 0) < 0) return -1; 
	sprintf(answerBuffer, "%d %d %.8lf", deviceIndex, channel, adcValue);    
	
	return 1;
}


int cmdParserSingleAdcRawGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel; 
	double adcValue;
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &channel) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (getAdcVoltage(cgwIndex, deviceId, channel, &adcValue, 1) < 0) return -1; 
	sprintf(answerBuffer, "%d %d %.8lf", deviceIndex, channel, adcValue);    
	
	return 1;
}


int cmdParserSingleAdcReset(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId; 
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (resetSingleAdcMeasurements(cgwIndex, deviceId) < 0) return -1; 
	
	logNotification(ip, "Block %d, deviceID 0x%X - Single ADC RESET", cgwIndex, deviceId); 
	
	return 1;
}


int cmdParserSingleAllRegistersGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	unsigned char inRegs, outRegs;

	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (getAllRegisters(cgwIndex, deviceId, &inRegs, &outRegs) < 0) return -1;
	
	sprintf(answerBuffer, "%s %X %X", commandBody, inRegs, outRegs);
	
	return 1;
}


int cmdParserSingleOutRegistersSet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	unsigned int registers, mask;
	
	if (sscanf(commandBody, "%d %X %X", &deviceIndex, &registers, &mask) != 3) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (setOutputRegisters(cgwIndex, deviceId, registers, mask) < 0) return -1;

	logNotification(ip, "Block %d, deviceID 0x%X - Single Registers SET to 0x%X (mask 0x%X)", cgwIndex, deviceId); 
	
	return 1;
}


int cmdParserSingleInterlockRestore(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1; 
	if (controlSingleInterlockRestore(cgwIndex, deviceId) < 0) return -1;

	logNotification(ip, "Block %d, deviceID 0x%X - Single Force RESTORE", cgwIndex, deviceId);
	
	return 1;
}


int cmdParserSingleInterlockDrop(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (controlSingleInterlockDrop(cgwIndex, deviceId) < 0) return -1; 

	logNotification(ip, "Block %d, deviceID 0x%X - Single Force DROP", cgwIndex, deviceId); 
	
	return 1;
}


int cmdParserSingleForceOn(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId; 
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	
	if (controlScheduleSingleForceOn(cgwIndex, deviceId) < 0) return -1;   
	logNotification(ip, "Block %d, deviceID 0x%X - Single Force ON", cgwIndex, deviceId);
	
	return 1;
}


int cmdParserSingleForceOff(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId; 
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (controlSingleForceOff(cgwIndex, deviceId) < 0) return -1;
	
	logNotification(ip, "Block %d, deviceID 0x%X - Single Force OFF", cgwIndex, deviceId);
	
	return 1;
}


int cmdParserSinglePermissionOn(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId; 
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (controlSinglePermissionOn(cgwIndex, deviceId) < 0) return -1;
	
	logNotification(ip, "Block %d, deviceID 0x%X - Single Current Permission ON", cgwIndex, deviceId); 
	
	return 1;
}


int cmdParserSinglePermissionOff(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId; 
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	if (controlSinglePermissionOff(cgwIndex, deviceId) < 0) return -1; 
	
	logNotification(ip, "Block %d, deviceID 0x%X - Single Current Permission OFF", cgwIndex, deviceId);
	
	return 1;
}


int cmdParserSingleStatusGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	unsigned int status;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	
	if (getDeviceStatus(cgwIndex, deviceId, &status) < 0) return -1;
	
	sprintf(answerBuffer, "%d %X", deviceIndex, status);
	
	return 1;
}


int cmdParserSingleDeviceNameGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d ", deviceIndex); 
	if (getDeviceName(cgwIndex, deviceId, answerBuffer + strlen(answerBuffer)) < 0) return -1;
	
	return 1;
}


int cmdParserSingleAdcNameGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel; 
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &channel) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d %d ", deviceIndex, channel);
	if (getAdcChannelName(cgwIndex, deviceId, channel, answerBuffer + strlen(answerBuffer)) < 0) return -1;
	
	return 1;
}


int cmdParserSingleDacNameGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, channel; 
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &channel) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d %d ", deviceIndex, channel);
	if (getDacChannelName(cgwIndex, deviceId, channel, answerBuffer + strlen(answerBuffer)) < 0) return -1;
	
	return 1;
}


int cmdParserSingleInregNameGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, registerIndex; 
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &registerIndex) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d %d ", deviceIndex, registerIndex);
	if (getInputRegisterName(cgwIndex, deviceId, registerIndex, answerBuffer + strlen(answerBuffer)) < 0) return -1;
	
	return 1;
}


int cmdParserSingleOutRegNameGet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, registerIndex; 
	
	if (sscanf(commandBody, "%d %d", &deviceIndex, &registerIndex) != 2) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	sprintf(answerBuffer, "%d %d ", deviceIndex, registerIndex);
	if (getOutputRegisterName(cgwIndex, deviceId, registerIndex, answerBuffer + strlen(answerBuffer)) < 0) return -1;
	
	return 1;
}


int cmdParserAllAdcReset(char *commandBody, char *answerBuffer, char *ip) {
	int cgwIndex;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (resetAllAdcMeasurements(cgwIndex) < 0) return -1;  
	}
	
	logNotification(ip, "All ADC RESET");  
	
	return 1;
}


int cmdParserAllForceOff(char *commandBody, char *answerBuffer, char *ip) {
	int cgwIndex;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (controlAllForceOff(cgwIndex) < 0) return -1;  
	}
	
	logNotification(ip, "All Force OFF");  
	
	return 1;
}


int cmdParserAllPermissionOff(char *commandBody, char *answerBuffer, char *ip) {
	int cgwIndex;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (controlAllPermissionOff(cgwIndex) < 0) return -1;  
	}

	logNotification(ip, "All Current Permission OFF");
	
	return 1;
}


int cmdParserAllZeroDac(char *commandBody, char *answerBuffer, char *ip) {
	int cgwIndex;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (setAllDacToZero(cgwIndex) < 0) return -1;  
	}
	return 1;	
}


int cmdParserCangwStatusGet(char *commandBody, char *answerBuffer, char *ip) {
	int statusAll, statusSingle;
	int cgwIndex;
	
	statusAll = 0;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		getCanGwStatus(cgwIndex, &statusSingle);
		statusAll = statusAll | (statusSingle << cgwIndex);
	}
	sprintf(answerBuffer, "%X", statusAll);
	return 1;
}


int cmdParserServerNameGet(char *commandBody, char *answerBuffer, char *ip) {
	strcpy(answerBuffer, CFG_SERVER_NAME);
	return 1;
}


int cmdParserDevicesNumGet(char *commandBody, char *answerBuffer, char *ip) { 
	sprintf(answerBuffer, "%d", CFG_PSMCU_DEVICES_NUM); 
	return 1;
}


int cmdParserName2IdGet(char *commandBody, char *answerBuffer, char *ip) {
	char deviceName[128];
	char *start;
	int *devIndex;
	
	start = commandBody;
	while (*start == ' ') start++;  // skip spaces
	strcpy(deviceName, start);
	
	devIndex = map_get(&devNamesHashMap, deviceName);
	if (!devIndex) return -1;

	sprintf(answerBuffer, "%d", *devIndex);
	return 1;
}


int cmdParserSingleErrorSet(char *commandBody, char *answerBuffer, char *ip) {
	int deviceIndex, cgwIndex, deviceId, cursor;
	char *message_ptr;
	
	if (sscanf(commandBody, "%d%n", &deviceIndex, &cursor) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;
	
	message_ptr = commandBody + cursor;
	while ((message_ptr[0] == ' ') || (message_ptr[0] == '\t')) message_ptr++;  // skip leading blank spaces 

	if (setSingleErrorState(cgwIndex, deviceId, message_ptr) < 0) return -1;

	logNotification(ip, "Setting the error status for the device '%s' (index %d, block %d, address 0x%X). Message: '%s'.",
					getDeviceNamePtr(cgwIndex, deviceId), deviceIndex, cgwIndex, deviceId, message_ptr);
	
	return 1; 
	
}

int cmdParserSingleErrorGet(char *commandBody, char *answerBuffer, char *ip) {
	
	int deviceIndex, cgwIndex, deviceId;
	char * msg_buffer_ptr;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;

	sprintf(answerBuffer, "%d ", deviceIndex);
	msg_buffer_ptr = answerBuffer + strlen(answerBuffer);
	
	if (getSingleErrorStateMessage(cgwIndex, deviceId, msg_buffer_ptr) < 0) return -1;
	
    return 1;	
}

int cmdParserSingleErrorClear(char *commandBody, char *answerBuffer, char *ip) {
	
	int deviceIndex, cgwIndex, deviceId;
	
	if (sscanf(commandBody, "%d", &deviceIndex) != 1) return -1;
	if (deviceIndexToDeviceId(deviceIndex, &cgwIndex, &deviceId) < 0) return -1;

	if (clearSingleErrorState(cgwIndex, deviceId) < 0) return -1;

	logNotification(ip, "Clearing the error status for the device '%s' (index %d, block %d, address 0x%X).",
					getDeviceNamePtr(cgwIndex, deviceId), deviceIndex, cgwIndex, deviceId);
	
    return 1;	
}

int cmdParserAllErrorSet(char *commandBody, char *answerBuffer, char *ip) {
	
	int cgwIndex;
	char *message_ptr;
	
	message_ptr = commandBody;
	while ((message_ptr[0] == ' ') || (message_ptr[0] == '\t')) message_ptr++;  // skip leading blank spaces
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (setAllErrorState(cgwIndex, message_ptr) < 0) return -1;  
	}
	
	logNotification(ip, "Setting the error status for all devices with message '%s'.", message_ptr);

    return 1;	
}

int cmdParserAllErrorClear(char *commandBody, char *answerBuffer, char *ip) {
	
	int cgwIndex;
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (clearAllErrorState(cgwIndex) < 0) return -1;  
	}
	
	logNotification(ip, "Clearing all errors status.");
	
	return 1;
}

// Debug commands
int cmdParseDbgCgwReconnectionReset(char *commandBody, char *answerBuffer, char *ip) {
	int cgwIndex;
	
	if (!cgwConnection_IsTotalAttemptsDepleted()) {
		logNotification(ip, "Reset the CanGw reconnection counter for all devices"); 	
	} else {
		logNotification(ip, "Attempted to reset the CanGw reconnection counter but the total number of attemtps is depleted."); 	
	}
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		cgwConnection_ResetDeviceCounter(cgwIndex);
	}
	return 1;
}
