//#define NO_CAN_EXCHANGE
//#define NO_CAN

#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib,"cangw.lib")
#include "cangw.h"

#include <Windows.h>

#include "TimeMarkers.h"
#include "TCP_Connection.h"
#include <utility.h>
#include <userint.h>
#include <tcpsupp.h>
#include "CGW_Connection.h"
#include "MessageStack.h"
#include "CGW_Devices.h"
#include "CGW_PSMCU_Interface.h"
#include "Logging.h"

#include "ServerConfigData.h"

#include "ServerData.h"
#include "ServerCommands.h"  

#include "hash_map.h" 

// Variables

/////////////////////////////////////////    SCHEDULE IDs   
int reconnectionRequestId[CFG_CGW_MAX_NUM]; 
///////////////////////////////////////// 

/////////////////////////////////////////   SCHEDULE FUNCTIONS
void serverReconnection(int timerHandle, int cgwIndex);
void psMcuStatusRequest(int timerHandle, int cgwIndex); 
void psMcuRegistersRequest(int timerHandle, int cgwIndex); 
void psMcuDacRequest(int timerHandle, int cgwIndex);
void dataFileWrite(int timerHandle, int arg1);
void deleteOldFiles(int timerHandle, int arg1);
/////////////////////////////////////////

/////////////////////////////////////////   CONFIGURATION FUNCTIONS 
void register_devices(void);
void prepareTimeSchedule(void);    

void DiscardAllResources(void);
/////////////////////////////////////////

/////////////////////////////////////////   HOOK FUNCTIONS
void hookPsMcuAnswers(cangw_msg_t msg, void *deviceParameters);
void psMcuAwakeCallback(void *parameters);
/////////////////////////////////////////   

void bgdFunc(void);


void avoidZeroCharacters(char *str,int bytes);

/////////////////////////////////////////   FUNCTIONS DEFINITIONS   

void DiscardAllResources(void) {
	int cgwIndex;
	
	if (cgwAreGlobalsInitialized()) {
		for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
			cgwConnection_Release(cgwIndex, "discarding resources");  
			cgwDevices_ReleaseAll(&deviceKit[cgwIndex]);
		}
		cgwResetGlobals();
	}
	if (tcpSI.initialized) {
		UnregisterTCPServer(CFG_TCP_PORT);
		tcpSI.initialized = 0;
	}
		
	ReleaseCommandParsers();
	msReleaseGlobalStack();
}

int CVICALLBACK userMainCallback(int MenuBarHandle, int MenuItemID, int event, void * callbackData, 
	int eventData1, int eventData2)
{
	switch(event)
	{
		case EVENT_END_TASK: 
			switch(eventData1)
			{
				case APP_CLOSE:
					if(ConfirmPopup("Closing the application.", "Do you want to close the application?") == 0) return 1;

					msAddMsg(msGMS(),"%s [SERVER] Closing the application.", TimeStamp(0));
					WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
					DiscardAllResources();
					// Closing the application
					break;
				case SYSTEM_CLOSE:
					// Shutting down the system
					msAddMsg(msGMS(), "%s [SERVER] The computer is shutting down! Closing the application.", TimeStamp(0));
					WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
					DiscardAllResources(); 
					MessagePopup("Shutting down the system","The server will be closed now.");
					break;
			}
			break;
	}
	
	return 0;
}


void bgdFunc(void)
{
	// Independant state tracker for setting up the reconnection schedule.
	static int connection_is_active[CFG_CGW_MAX_NUM] = {0};  // -1 - inactive, 1 - active, 0 - start state (to avoid unnecessary messages)
	int cgwIndex;
	
	//static int connection_delay = 0;
	#define MAX_MSG_NUM 1
	cangw_msg_t msgs_from_server[MAX_MSG_NUM];
	int cmsg, i; 
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		cmsg = cgwConnection_Recv(cgwIndex, msgs_from_server, MAX_MSG_NUM, CFG_CANGW_RECV_TIMEOUT[cgwIndex], "background looped process");

		if (cmsg > 0) {
			if (connection_is_active[cgwIndex] == -1)
			{
				msAddMsg(msGMS(), "%s [CANGW] [%s] CanGw connection has been restored. Continue processing the CanGw messages.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
			}
			connection_is_active[cgwIndex] = 1;  
		
			for (i=0; i<cmsg; i++) {
				cgwDevices_MessageHook(&deviceKit[cgwIndex], msgs_from_server[i]);	
			}
		
		} else if (cmsg == CANGW_ERR) {
			if (connection_is_active[cgwIndex] != -1)
			{
				markAllDevicesInactive(&deviceKit[cgwIndex]);
				msAddMsg(msGMS(), "%s [CANGW] [%s] CanGw connection is lost. Scheduling the reconnection.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
				activateScheduleRecord(reconnectionRequestId[cgwIndex], 1);
			}
			connection_is_active[cgwIndex] = -1;
		}
	}
	
	////////////////
		
	processScheduleEvents();
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		if (!cgwConnectionBroken[cgwIndex]) updateDevicesDownTime(&deviceKit[cgwIndex]);
		updateDevices(&deviceKit[cgwIndex]);
	}
	WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS());
}


void serverReconnection(int timerHandle, int cgwIndex) {

	if (cgwConnectionBroken[cgwIndex]) {
		if (cgwConnection_Init(cgwIndex) < CANGW_NOERR) {
			msAddMsg(msGMS(), "%s [CANGW] [%s] Next connection request will be in 5 seconds.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]); 
			
		} else {
			// Successfully connected
			resetAllAdcMeasurements(cgwIndex);	
			deactivateScheduleRecord(reconnectionRequestId[cgwIndex]); 
		}
	}
}


void psMcuStatusRequest(int timerHandle, int cgwIndex) {
	int i;
	
	if (cgwConnectionBroken[cgwIndex]) return;

	for (i=0; i<deviceKit[cgwIndex].registeredNum; i++)
		cgwPsMcu_StatusRequest(cgwIndex, deviceKit[cgwIndex].registeredIDs[i]);
		
}

void psMcuRegistersRequest(int timerHandle, int cgwIndex) {
	int i;
	if (cgwConnectionBroken[cgwIndex]) return; 
	for (i=0; i<deviceKit[cgwIndex].registeredNum; i++)
		cgwPsMcu_RegistersRequest(cgwIndex, deviceKit[cgwIndex].registeredIDs[i]);	
}


void psMcuDacRequest(int timerHandle, int cgwIndex) {
	int i, j;
	
	if (cgwConnectionBroken[cgwIndex]) return; 
	for (i=0; i<deviceKit[cgwIndex].registeredNum; i++)
		for (j=0; j<PSMCU_DAC_CHANNELS_NUM; j++) {
			cgwPsMcu_DACReadCode(cgwIndex, deviceKit[cgwIndex].registeredIDs[i], j);	
		}
}


void dataFileWrite(int timerHandle, int arg1) {
	static char infoBuffer[256];
	static int deviceIndex, cgwIndex;
	message_stack_t dataMsgStack;
	
	if (cgwConnectionBroken) return;
	
	msInitStack(&dataMsgStack);
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		for (deviceIndex=0; deviceIndex < CFG_PSMCU_DEVICES_NUM; deviceIndex++) {
			getDeviceFullInfo(cgwIndex, deviceIndex, infoBuffer);
			msAddMsg(dataMsgStack, "%d %d %X %s", cgwIndex, deviceIndex, deviceKit[cgwIndex].registeredIDs[deviceIndex], infoBuffer); 
		}
	}

	WriteDataFiles(dataMsgStack, CFG_FILE_DATA_DIRECTORY);
	
	msReleaseStack(&dataMsgStack);
}


void deleteOldFiles(int timerHandle, int arg1) {
	DeleteOldFiles(CFG_FILE_LOG_DIRECTORY, CFG_FILE_DATA_DIRECTORY, CFG_FILE_EXPIRATION);
}


void prepareTimeSchedule(void) {
	// Depending on the options checked:
	// - CANGW periodic connection task
	// - CANGW Device ping
	int cgwIndex;
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		reconnectionRequestId[cgwIndex] = addRecordToSchedule(1, 1, CFG_CANGW_RECONNECTION_DELAY[cgwIndex], serverReconnection, "reconnection", cgwIndex);
		
		// Send status request
		addRecordToSchedule(1, 0, CFG_PSMCU_DEVICE_PING_INTERVAL, psMcuStatusRequest, "device ping", cgwIndex); 
		
		// Request registers
		addRecordToSchedule(1, 0, CFG_PSMCU_REGISTERS_REQUEST_INTERVAL, psMcuRegistersRequest, "registers request", cgwIndex);
		
		// Request DAC
		addRecordToSchedule(1, 0, CFG_PSMCU_DAC_REQUEST_INTERVAL, psMcuDacRequest, "DAC request", cgwIndex);
	}
	
	// Logging PSMCU data
	addRecordToSchedule(1, 0, CFG_FILE_DATA_WRITE_INTERVAL, dataFileWrite, "file write", 0);  
	addRecordToSchedule(1, 1, 60 * 60 * 24, deleteOldFiles, "delete old files", 0);     
}


void hookPsMcuAnswers(cangw_msg_t msg, void *deviceParameters) {   // need to check sometimes ADC config
	int deviceId;
	cgwPsMcu_Information_t *parameters = deviceParameters;
	unsigned char ourRegsiters;
	
	deviceId = cgwDevicess_DeviceIDFromMessageID(msg.id);
	switch(msg.data[0]) {
		case 0xFE:
			// Check status and update the configuration if necessary
			if ( (msg.data[1] & 0x18) != 0x18 ) {
				msAddMsg(
					msGMS(),
					"%s [PSMCU] Error! Configuration of PSMCU is incorrect (\"%s\" [%d (0x%X)]). Sending configuration request.",
					TimeStamp(0),
					parameters->deviceName,
					deviceId,
					deviceId
				);
				resetSingleAdcMeasurements(parameters->cgwIndex, deviceId);
			}
			break;
		case 0xF8:
			// Manually check the bits
			ourRegsiters = msg.data[1];
			if (ourRegsiters & 0x1) {
				controlSingleInterlockRestore(parameters->cgwIndex, deviceId);	
			}
			break;
	}
}


void psMcuAwakeCallback(void *parameters) {
	cgwPsMcu_Information_t * cgw_params = parameters;
	resetSingleAdcMeasurements(cgw_params->cgwIndex, cgw_params->deviceID);
}


void register_devices(void) {
	int cgwIndex, globalDeviceIndex;  

	cgwDevices_InitPrototypes();
	
	// Initialize devices kits
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		cgwDevices_InitDevicesKit(&deviceKit[cgwIndex], CFG_CANGW_BLOCK_NAME[cgwIndex]);   	
	}
	
	// register devices
	for (globalDeviceIndex=0; globalDeviceIndex < CFG_PSMCU_DEVICES_NUM; globalDeviceIndex++) {
		cgwIndex = CFG_PSMCU_BLOCKS_IDS[globalDeviceIndex];

		cgwPsMcu_RegisterDevice(
			&deviceKit[cgwIndex],
			globalDeviceIndex,
			cgwIndex,
	        CFG_PSMCU_DEVICES_IDS[globalDeviceIndex],
			CFG_PSMCU_DEVICES_NAMES[globalDeviceIndex],
			hookPsMcuAnswers,
			psMcuAwakeCallback,
			0,
			CFG_PSMCU_DEVICE_DOWNTIME_LIMIT,
			CFG_PSMCU_ADC_NAMES[globalDeviceIndex],
			CFG_PSMCU_DAC_NAMES[globalDeviceIndex],
			CFG_PSMCU_ADC_COEFF[globalDeviceIndex],
			CFG_PSMCU_DAC_COEFF[globalDeviceIndex],
			CFG_PSMCU_INREG_NAMES[globalDeviceIndex],
			CFG_PSMCU_OUTREG_NAMES[globalDeviceIndex],
			CFG_PSMCU_DAC_SLOW_TIME_DELTA,
			CFG_PSMCU_DAC_SLOW_VOLTAGE_STEP
		);	
	}
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		cgwDevices_IgnoreUnregistered(&deviceKit[cgwIndex], CFG_CANGW_IGNORE_UNKNOWN[cgwIndex]);  	
	}	
}


int main(int argc, char **argv) {
	#define defaultConfigFile "PS-MCU server configuration.ini"   
	char errBuf[512], configFilePath[1024];
	
	switch (argc) {
	    case 1: strcpy(configFilePath, defaultConfigFile); break;
		case 2: strcpy(configFilePath, argv[1]); break;
		default:
			MessagePopup("Command line arguments error", "Incorrect number of arguments");
			exit(1);
	}
	
	InitServerConfig(configFilePath);
	
	/////////////////////////////////
	// Body of the program
	/////////////////////////////////
	
	// Setup the console window
	//SetSystemAttribute 
	SetStdioWindowOptions(2000, 0, 0);
	SetStdioPort(CVI_STDIO_WINDOW);
	SetStdioWindowVisibility(1);
	SetSleepPolicy(VAL_SLEEP_NONE);
	InstallMainCallback(userMainCallback, 0, 0);  

	atexit(DiscardAllResources);
	
	// Check the CanGw version before doing anything
	// (if the verison is incorrect, we cannot do anythin0g, so close the programm)
	if (cgwCheckVersion(errBuf) < 0) {
		MessagePopup("CanGW version problem", errBuf);
		return 0;
	}
	
	msInitGlobalStack();  // Initialize the global message stack 
	msAddMsg(msGMS(), "Configuration file: %s", configFilePath);
	msAddMsg(msGMS(), "------------\n[PS-MCU Server -- NEW SESSION]\n------------");   
	
	tcpConnection_InitServerInterface(&tcpSI);
	tcpConnection_SetBackgroundFunction(&tcpSI, bgdFunc);
	tcpConnection_SetDataExchangeFunction(&tcpSI, dataExchFunc);
	
	register_devices();
	cgwInitializeGlobals();
	InitCommandParsers();
	prepareTimeSchedule();
	
	//--------------------------------------------------------
	WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS());  	   
	//--------------------------------------------------------
	
	tcpConnection_RunServer(CFG_TCP_PORT, &tcpSI); 
	WriteLogFiles(msGMS(), CFG_FILE_LOG_DIRECTORY);
	msPrintMsgs(msGMS(), stdout);
	msFlushMsgs(msGMS()); 
		
	DiscardAllResources();
	printf("The application is closing. Press any key.");
	GetKey();

	return 0;
}
