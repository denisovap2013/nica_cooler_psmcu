//==============================================================================
//
// Title:       DenisovUBSclient
// Purpose:     A short description of the application.
//
// Created on:  23.09.2015 at 12:04:52 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <tcpsupp.h>
#include "inifile.h"
#include <ansi_c.h>
#include <cvirte.h>     
#include <userint.h>

#include "TimeMarkers.h"
#include "MessageStack.h"
#include "ClientConfiguration.h"
#include "ClientLogging.h"

#include "PS-mcu client.h"
#include "clientInterface.h"
#include "clientData.h"   
#include "toolbox.h"

#include "commands_queue.h"
#include "LoadSave.h"

//==============================================================================
// Constants


#define CONNETCION_DELAY_TIME   40
#define BLOCK_REQUEST_INTERVAL  4 

#define MAX_RECEIVED_BYTES 3000
//==============================================================================
// Types

//==============================================================================
// Static global variables

unsigned int serverHandle; 
int connectionEstablished;
int canGwConnectionStatus = 0;

int connectionWait;

char globalAnswerString[3000];
int globalWaitingForAnswer = 0;
int globalWaitingForRequest = 0; 
int waitingForGraphUpdate = 0; 
int waitingForDataWriting = 0; 
int waitingForCanGwStatisRequest = 0;

#define WAIT_FOR_BLOCK_NAMES 1


char logFileName[256], dataFileName[256], infoFileName[256];
int infoFileCreated = 0;

//==============================================================================
// Static functions


int initConnectionToServer(void);

void initLogAndDataFilesNames(void);
void createInfoFile(void);

void RequestNames(void);
int parseChannelName(char *src, char *prefix, int expectedDevIndex, int expectedChannel, char *output);
int parseServerName(char *src, char *prefix, char *output);
int parseDeviceName(char *src, char *prefix, int expectedDevIndex, char *output);
void UpdateServerName(char *name);
void requestDataFromServer(void);
void requestCanGwStatus(void);
void ProcessCommandsQueue(void);
void parseUbsIncomingCommand(char *, int);
void SendDACValue(int deviceIndex, int channel, double value, int fast, int confirmFast);
void SendDeviceCommand(char *, int);
void SendBroadcastCommand(char *);

// Commands
void UserSingleReset(int deviceIndex);
void UserSingleForceOn(int deviceIndex);
void UserSingleForceOff(int deviceIndex);
void UserSingleCurrentOn(int deviceIndex);
void UserSingleCurrentOff(int deviceIndex);
void UserSingleZeroDacFast(int deviceIndex);
void UserAllReset(void);
void UserAllForceOn(void);
void UserAllForceOff(void);
void UserAllCurrentOn(void);
void UserAllCurrentOff(void);
void UserAllZeroDacFast(void);
void UserAllZeroDacSmart(void);

void UserSingleDacSet(int deviceIndex, int channelIndex, double dacValue);

void dacSetInOrder(double dacStates[PSMCU_MAX_NUM], double timeDelay_sec[PSMCU_MAX_NUM], int reversed);
void dacStatesLoader(double dacStates[PSMCU_MAX_NUM]);
   
void DiscardAllResources(void);  

// Logging
void assembleDataForLogging(char * target); 
int WriteDataFilesOnSchedule(void);
int checkServerAnswer(char *answer);
char * revealEscapeSequences(char *string);
//==============================================================================
// Global variables

commands_queue_t COMMANDS_QUEUE;
int pipelineBreakFlag = 0;
int pipelineGuiLock = 0;

//==============================================================================
// Global functions

void parseFullInfo(char *);
void parseCanGwConnectionStatus(char *);
void parseSingleErrorMessage(char *);


/// HIFN The main entry-point function.

int main (int argc, char *argv[])
{
	#define defaultConfigFile "PS-MCU client configuration.ini"
	
    int error = 0;
	int i;
	char title[256], configFilePath[1024];
	
	initLogAndDataFilesNames();

	//// Configuration
	switch (argc) {
	    case 1: strcpy(configFilePath, defaultConfigFile); break;
		case 2: strcpy(configFilePath, argv[1]); break;
		default:
			MessagePopup("Command line arguments error", "Incorrect number of arguments");
			exit(1);
	}

	ConfigurateClient(configFilePath);
	sprintf(SERVER_INDICATOR_ONLINE_LABEL, "Server is Online (%s:%d)", CFG_SERVER_IP, CFG_SERVER_PORT);
	sprintf(SERVER_INDICATOR_OFFLINE_LABEL, "Server is Offline (%s:%d)", CFG_SERVER_IP, CFG_SERVER_PORT);
	
	msInitGlobalStack();
	msAddMsg(msGMS(), "------\n[NEW SESSION]\n------");
	connectionWait = CFG_SERVER_CONNECTION_INTERVAL / TIMER_TICK_TIME; 
	
	initQueue(&COMMANDS_QUEUE);
    
    /* initialize and load resources */
    nullChk (InitCVIRTE (0, argv, 0));
    errChk (mainMenuHandle = LoadPanel (0, "PS-mcu client.uir", BlockMenu));
	SetCtrlAttribute(mainMenuHandle, BlockMenu_TIMER, ATTR_INTERVAL, TIMER_TICK_TIME);

	// ADC blocks
	for (i=0; i < PSMCU_NUM; i++) {
		errChk (psMcuWindowHandles[i] = LoadPanel (0, "PS-mcu client.uir", psMcuPanel));
		sprintf(title, "PS-MCU %d: Unknown", i+1);
		SetPanelAttribute(psMcuWindowHandles[i], ATTR_TITLE, title);
	}
	errChk (errPanelHandle = LoadPanel (0, "PS-mcu client.uir", errPanel)); 
	
	/////////////
	clearNames();
	initValues();
	initGui();
    /////////////
	
    // Setup the console output
	SetStdioWindowOptions(2000, 0, 0);
	SetSystemAttribute(ATTR_TASKBAR_BUTTON_TEXT, title);
	SetStdioPort(CVI_STDIO_WINDOW);
	SetStdioWindowVisibility(0);

	// Install the main callback
	InstallMainCallback(mainSystemCallback,0,0); 
	
	/* display the panel and run the user interface */  
    errChk (DisplayPanel (mainMenuHandle));
    errChk (RunUserInterface ());

Error:
    /* clean up */
    DiscardAllResources();
	return 0;
}

void DiscardAllResources(void) {
	DiscardGuiResources();
	
	if(connectionEstablished) {
		DisconnectFromTCPServer(serverHandle);		
	}
	
	msReleaseGlobalStack();
}

int initConnectionToServer(void) {
	return ConnectToTCPServer(&serverHandle, CFG_SERVER_PORT, CFG_SERVER_IP, clientCallbackFunction, NULL, 100);	
}


char* revealEscapeSequences(char *string) {
	static char new_string[512];
	int i, j;
	
	j = 0;
	for (i=0; i<strlen(string); i++) {
		if (string[i] == '\n') {
			new_string[j++] = '\\';	
			new_string[j++] = 'n';
		} else {
			new_string[j++] = string[i];	
		}
	}
	new_string[j] = 0;
	return new_string;
}


int checkServerAnswer(char *answer) {
	if (answer[0] == '!') {
		logMessage("Server answered with the command error: \"%s\"", revealEscapeSequences(answer));
		return -1;
	}
	
	if (answer[0] == '?') {
		logMessage("Server did not recognize the command: \"%s\"", revealEscapeSequences(answer));
		return -1;
	}
	
	return 0;
}


int parseChannelName(char *src, char *prefix, int expectedDevIndex, int expectedChannel, char *output) {
	// 1 - success, 0 - failure
	int deviceIndex, channel, readPos;
	char *stringPointer;

	if (checkServerAnswer(src) < 0) return 0;
	
	if (strstr(src, prefix) != src) {
		logMessage("Expected the answer to start with \"%s\", but got \"%s\"", prefix, revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer = src + strlen(prefix);

	if (sscanf(stringPointer, "%d %d %n", &deviceIndex, &channel, &readPos) != 2) {
		logMessage("Expected the answer to have a device index and a channel specified, but got \"%s\"", revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer += readPos;
	
	if (deviceIndex != expectedDevIndex || channel != expectedChannel) {
		logMessage("Expected device index and channel to be %d and %d, but got answer \"%s\"", expectedDevIndex, expectedChannel, revealEscapeSequences(src)); 
		return 0;
	}
	
	while (stringPointer[0] == ' ') stringPointer++;	
	
	strcpy(output, stringPointer);
	if ((stringPointer = strstr(output, "\n")) != NULL) {
		stringPointer[0] = 0;	
	}
	
	return 1;
}


int parseServerName(char *src, char *prefix, char *output) {
	// 1 - success, 0 - failure
	char *stringPointer;

	if (checkServerAnswer(src) < 0) return 0; 
	
	if (strstr(src, prefix) != src) {
		logMessage("Expected the answer to start with \"%s\", but got \"%s\"", prefix, revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer = src + strlen(prefix);
	
	while (stringPointer[0] == ' ') stringPointer++;	
	
	strcpy(output, stringPointer);
	if ((stringPointer = strstr(output, "\n")) != NULL) {
		stringPointer[0] = 0;	
	}
	
	return 1;	
}


int parseDeviceName(char *src, char *prefix, int expectedDevIndex, char *output) {
	// 1 - success, 0 - failure
	int deviceIndex, readPos;
	char *stringPointer;

	if (checkServerAnswer(src) < 0) return 0; 
	
	if (strstr(src, prefix) != src) {
		logMessage("Expected the answer to start with \"%s\", but got \"%s\"", prefix, revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer = src + strlen(prefix);

	if (sscanf(stringPointer, "%d %n", &deviceIndex, &readPos) != 1) {
		logMessage("Expected the answer to have a device index, but got \"%s\"", src);
		return 0;	
	}
	
	stringPointer += readPos;
	
	if (deviceIndex != expectedDevIndex) {
		logMessage("Expected device index to be %d, but got answer \"%s\"", expectedDevIndex, revealEscapeSequences(src)); 
		return 0;
	}
	
	while (stringPointer[0] == ' ') stringPointer++;	
	
	strcpy(output, stringPointer);
	if ((stringPointer = strstr(output, "\n")) != NULL) {
		stringPointer[0] = 0;	
	}
	
	return 1;
}


#define CLOCK_WAIT 1000
void RequestNames(void) {
	int i, j;
	char command[256], name[256];
	int waitTime;
	char prefix[256];
	
	if (connectionEstablished) {
		strcpy(prefix, "PSMCU:SERVER:NAME:GET");
		sprintf(command, "%s\n", prefix);

		ClientTCPWrite(serverHandle, command, strlen(command), 100);
		waitTime = clock();
		globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
		
		while (globalWaitingForAnswer) {
			ProcessSystemEvents();
			if (globalWaitingForAnswer == 0) {
				if (parseServerName(globalAnswerString, prefix, name))
					UpdateServerName(name);
				else
					UpdateServerName("Unknown");
				break;
			}
			if (clock()- waitTime > CLOCK_WAIT) {
				UpdateServerName("Unknown"); 
				break;
			}
		}	
	} else return;
	
	
	for (i=0; i < PSMCU_NUM; i++) {
		// Device name
		strcpy(prefix, "PSMCU:SINGLE:DEVNAME:GET"); 
		if (connectionEstablished) { 
			sprintf(command, "%s %d\n", prefix, i); 
			ClientTCPWrite(serverHandle, command, strlen(command), 100);
			waitTime = clock();
			globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
			
			while (globalWaitingForAnswer) {
				ProcessSystemEvents();
				if (globalWaitingForAnswer == 0) {
					if (parseDeviceName(globalAnswerString, prefix, i, name))
						UpdateDeviceName(i, name);
					else
						UpdateDeviceName(i, "Unknown");
					break;
				}
				if (clock()- waitTime > CLOCK_WAIT) {
					UpdateDeviceName(i, "Unknown"); 
					break;
				}
			}
		} else return;
		
		// ADCs 
		strcpy(prefix, "PSMCU:SINGLE:ADCNAME:GET");
		for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
			if (connectionEstablished) {
				if (strlen(PSMCU_ADC_LABELS_TEXT[i][j]) > 0) continue;
				
				sprintf(command, "%s %d %d\n", prefix, i, j); 
				ClientTCPWrite(serverHandle, command, strlen(command), 100);
				waitTime = clock();
				globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
				while (globalWaitingForAnswer) {
					ProcessSystemEvents();
					if (globalWaitingForAnswer == 0) {
						if (!parseChannelName(globalAnswerString, prefix, i, j, PSMCU_ADC_LABELS_TEXT[i][j]))
							strcpy(PSMCU_ADC_LABELS_TEXT[i][j], "Unknown");
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_LABEL_TEXT, PSMCU_ADC_LABELS_TEXT[i][j]);
						break;
					}
					if (clock()- waitTime > CLOCK_WAIT) {
						strcpy(PSMCU_ADC_LABELS_TEXT[i][j], "Unknown"); 
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_LABEL_TEXT, PSMCU_ADC_LABELS_TEXT[i][j]);  
						break;
					}
				}
			} else return;			
		}
	
		// DACs
		strcpy(prefix, "PSMCU:SINGLE:DACNAME:GET"); 
		for (j=0; j < PSMCU_DAC_CHANNELS_NUM; j++) {
			if (connectionEstablished) {
			
				if (strlen(PSMCU_DAC_LABELS_TEXT[i][j]) > 0) continue;
			
				sprintf(command, "%s %d %d\n", prefix, i, j); 
				ClientTCPWrite(serverHandle, command, strlen(command), 100);
				waitTime = clock();
				globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
				while (globalWaitingForAnswer) {
					ProcessSystemEvents();
					if (globalWaitingForAnswer == 0) {
						if (!parseChannelName(globalAnswerString, prefix, i, j, PSMCU_DAC_LABELS_TEXT[i][j]))
							strcpy(PSMCU_DAC_LABELS_TEXT[i][j], "Unknown");
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_LABEL_TEXT, PSMCU_DAC_LABELS_TEXT[i][j]);
						break;
					}
					if (clock()- waitTime > CLOCK_WAIT) {
						strcpy(PSMCU_DAC_LABELS_TEXT[i][j], "Unknown");
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_LABEL_TEXT, PSMCU_DAC_LABELS_TEXT[i][j]);  
						break;
					}
				}
			} else return;			
		}
		
		SetCtrlVal(mainMenuHandle, PSMCU_WINDOW_FIELDS_CAPTION, PSMCU_DAC_LABELS_TEXT[0][0]); 
	
		// Input registries
		strcpy(prefix, "PSMCU:SINGLE:INREGNAME:GET");  
		for (j=0; j < PSMCU_INREG_BITS_NUM; j++) {
			if (connectionEstablished) {	   
			
				GetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_LABEL_TEXT, name);     

				if (strlen(name) > 0) continue;
			
				sprintf(command, "%s %d %d\n", prefix, i, j);   
				ClientTCPWrite(serverHandle, command, strlen(command), 100);
				waitTime = clock();
				globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
				while (globalWaitingForAnswer) {
					ProcessSystemEvents();
					if (globalWaitingForAnswer == 0) {
						if (!parseChannelName(globalAnswerString, prefix, i, j, name))
							strcpy(name, "Unknown");
						strcpy(PSMCU_INREG_LABELS_TEXT[i][j], name);
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_LABEL_TEXT, name);
						break;
					}
					if (clock()- waitTime > CLOCK_WAIT) {
						strcpy(PSMCU_INREG_LABELS_TEXT[i][j], "Unknown"); 
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_LABEL_TEXT, "Unknown");  
						break;
					}
				}
			} else return;			
		}
	
		// Output registries
		strcpy(prefix, "PSMCU:SINGLE:OUTREGNAME:GET"); 
		for (j=0; j < PSMCU_OUTREG_BITS_NUM; j++) {
			if (connectionEstablished) {	   
			
				GetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_LABEL_TEXT, name);     

				if (strlen(name) > 0) continue;
			
				sprintf(command, "%s %d %d\n", prefix, i, j); 
				ClientTCPWrite(serverHandle, command, strlen(command), 100);
				waitTime = clock();
				globalWaitingForAnswer = WAIT_FOR_BLOCK_NAMES;
				while (globalWaitingForAnswer) {
					ProcessSystemEvents();
					if (globalWaitingForAnswer == 0) {
						if(!parseChannelName(globalAnswerString, prefix, i, j, name))
							strcpy(name, "Unknown");
						strcpy(PSMCU_OUTREG_LABELS_TEXT[i][j], name); 
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_LABEL_TEXT, name);
						break;
					}
					if (clock()- waitTime > CLOCK_WAIT) {
						strcpy(PSMCU_OUTREG_LABELS_TEXT[i][j], "Unknown");
						SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_LABEL_TEXT, "Unknown");  
						break;
					}
				}
			} else return;			
		}
	}
}


void UpdateServerName(char *name) {
	char title[256];
	strcpy(SERVER_NAME, name);
	sprintf(title, "Client: %s", SERVER_NAME);
	SetSystemAttribute(ATTR_TASKBAR_BUTTON_TEXT, title);
	SetPanelAttribute(mainMenuHandle, ATTR_TITLE, title);
}


void initLogAndDataFilesNames(void) {
	static int day, month, year, hours, minutes, seconds;
	char nameBase[256];
	
	GetSystemDate(&month, &day, &year);
	GetSystemTime(&hours, &minutes, &seconds);

	sprintf(nameBase, "%02d.%02d.%02d_%02d-%02d-%02d", year, month, day, hours, minutes, seconds);
	sprintf(logFileName, "psMsuClientLog_%s.dat", nameBase);
	sprintf(dataFileName, "psMsuClientData_%s.dat", nameBase);
	sprintf(infoFileName, "psMsuClientInfo_%s.txt", nameBase);
}


void createInfoFile(void) {
	message_stack_t messageStack;
	int i;
	
	msInitStack(&messageStack);
	
	// Corresponding log / data files
	msAddMsg(messageStack, "[Log files]"); 
	msAddMsg(messageStack, "  Log file: %s", logFileName);
	msAddMsg(messageStack, "  Data file: %s", dataFileName);
	msAddMsg(messageStack, "\n");  

	// Data columns description
	msAddMsg(messageStack, "[Data columns description]");
	msAddMsg(messageStack, "  \"conn_s\" - whether the connection to the server is established (0 - n, 1 - y)");
	msAddMsg(messageStack, "  \"conn_m\" - whether the connection between the server and the CanGw device is established (0 - n, 1 - y)");
	msAddMsg(messageStack, "  \"alive_%%d\" - (%d columns) whether the device is online (0 - n, 1 - y)", PSMCU_NUM); 
	msAddMsg(messageStack, "  \"curr_set_%%d\" - (%d columns) set current (A) - from code read from the DAC register", PSMCU_NUM); 
	msAddMsg(messageStack, "  \"curr_meas_%%d\" - (%d columns) the actual measured current (A)", PSMCU_NUM); 
	msAddMsg(messageStack, "  \"inreg_%%d\" - (%d columns) input registers", PSMCU_NUM); 
	msAddMsg(messageStack, "  \"outreg_%%d\" - (%d columns) output registers", PSMCU_NUM); 

	// Optional columns
	msAddMsg(messageStack, "Optional columns:");
	if (CFG_DATA_LOG_CURR_USER) msAddMsg(messageStack, "  \"curr_user_%%d\" - (%d columns) current specified by the user (A)", PSMCU_NUM); 
	if (CFG_DATA_LOG_CURR_CONF) msAddMsg(messageStack, "  \"curr_conf_%%d\" - (%d columns) confirmation of the current code, sent to the DAC register (A)", PSMCU_NUM); 
	if (CFG_DATA_LOG_TEMP) msAddMsg(messageStack, "  \"temp_%%d\" - (%d columns) temperature of the transformer (°C)˜˜˜", PSMCU_NUM); 
	if (CFG_DATA_LOG_REF_V) msAddMsg(messageStack, "  \"ref_v_%%d\" - (%d columns) reference voltage (V)", PSMCU_NUM);
	if (CFG_DATA_LOG_RESERVED) msAddMsg(messageStack, "  \"reserved_%%d\" - (%d columns) some additional measurements (unknown)", PSMCU_NUM); 
	
	msAddMsg(messageStack, "\n");
	
	// Extended data columns description
	msAddMsg(messageStack, "[Input registers description]");
	for (i=0; i < PSMCU_INREG_BITS_NUM; i++)
		msAddMsg(messageStack, "  Bit-%d: %s", i, PSMCU_INREG_LABELS_TEXT[0][i]); 
	
	msAddMsg(messageStack, "\n"); 
	
	msAddMsg(messageStack, "[Output registers description]");
	for (i=0; i < PSMCU_OUTREG_BITS_NUM; i++)
		msAddMsg(messageStack, "  Bit-%d: %s", i, PSMCU_OUTREG_LABELS_TEXT[0][i]); 
	
	WriteDataDescription(messageStack, CFG_DATA_DIRECTORY, infoFileName);
	msReleaseStack(&messageStack);
}


void requestDataFromServer(void) {
	static char command[1000];
	int i;
	
	if (!connectionEstablished) return;
	if (globalWaitingForAnswer) return;

	for (i=0; i<PSMCU_NUM; i++) {
		sprintf(command, "PSMCU:SINGLE:FULLINFO %d\n", i);
		ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);	
	}
}


void requestCanGwStatus(void) {
	static char command[] = "PSMCU:CANGW:STATUS:GET\n";

	if (!connectionEstablished) return;

	if (globalWaitingForAnswer) {
		if (!isQueueFull(&COMMANDS_QUEUE)) {
			addCommandToQueue(&COMMANDS_QUEUE, command, 0);		
		}
		return;
	}
	
	ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);	
}


void SendDACValue(int deviceIndex, int channel, double value, int fast, int confirmFast) {
	char command[256];
	
	if (!connectionEstablished) return;

	if (fast && confirmFast) {
		sprintf(command, "PSMCU:SINGLE:DAC:FAST:SET %d %d %lf\n", deviceIndex, channel, value);	
	} else {
		sprintf(command, "PSMCU:SINGLE:DAC:SET %d %d %lf\n", deviceIndex, channel, value);	
	}

	if (globalWaitingForAnswer) {
		if (!isQueueFull(&COMMANDS_QUEUE)) {
			addCommandToQueue(&COMMANDS_QUEUE, command, 0);		
		}
		return;
	}
	
	ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);		
}


void SendDeviceCommand(char * cmd, int deviceIndex) {
	char command[256];
	
	if (!connectionEstablished) return;

	sprintf(command, "%s %d\n", cmd, deviceIndex);
	
	if (globalWaitingForAnswer) {
		if (!isQueueFull(&COMMANDS_QUEUE)) {
			addCommandToQueue(&COMMANDS_QUEUE, command, 0);		
		}
		return;
	}
	
	ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);
}


void SendBroadcastCommand(char * cmd) {
	char command[256];
	
	if (!connectionEstablished) return;

	sprintf(command, "%s\n", cmd);
	
	if (globalWaitingForAnswer) {
		if (!isQueueFull(&COMMANDS_QUEUE)) {
			addCommandToQueue(&COMMANDS_QUEUE, command, 0);		
		}
		return;
	}
	
	ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);
}


void ProcessCommandsQueue(void) {
	char command[256];
	
	if (!connectionEstablished) return;
	if (globalWaitingForAnswer) return;	
	
	if (isQueueEmpty(&COMMANDS_QUEUE)) return;
	
	globalWaitingForAnswer = popCommandFromQueue(&COMMANDS_QUEUE, command);
	ClientTCPWrite(serverHandle, command, strlen(command) + 1, 10);
}


void prepareTcpCommand(char *str,int bytes){
	int i, j;
	
	j = 0;
	for (i=0; i<(bytes-1); i++) {
		if (str[i] != 0) str[j++] = str[i];
	}
	str[j] = 0;
}


void parseUbsIncomingCommand(char *command, int bytes) {
	static char subcommand[MAX_RECEIVED_BYTES];
	char * lfp, *bufpos;
	prepareTcpCommand(command, bytes);
	
	while ( (lfp = strstr(command, "\n")) != NULL ) {
		lfp[0] = 0;
		strcpy(subcommand, command);
		strcpy(command, lfp + 1);
		
		////////////////////////////////  subcommand decoding
		// Check for errors flags
		if (checkServerAnswer(subcommand) < 0) continue;

		// PSMCU:SINGLE:FULLINFO
		if ((bufpos = strstr(subcommand, "PSMCU:SINGLE:FULLINFO")) == subcommand)
		{										
			parseFullInfo(bufpos + strlen("PSMCU:SINGLE:FULLINFO"));
			continue;
		} 
		
		// PSMCU:CANGW:STATUS:GET
		if ((bufpos = strstr(subcommand, "PSMCU:CANGW:STATUS:GET")) == subcommand)
		{										
			parseCanGwConnectionStatus(bufpos + strlen("PSMCU:CANGW:STATUS:GET"));
			continue;
		}
		
		// PSMCU:SINGLE:ERROR:GET
		if ((bufpos = strstr(subcommand, "PSMCU:SINGLE:ERROR:GET")) == subcommand)
		{										
			parseSingleErrorMessage(bufpos + strlen("PSMCU:SINGLE:ERROR:GET"));
			continue;
		}
		
		logMessage("Cannot handle the server answer \"%s\"", subcommand);  
	}
}


void parseFullInfo(char *serverAnswer) {
	char *answer_p;
	int deviceIndex;
	int p_shift, chIndex;
	int deviceState, errorStatus;
	
	answer_p = serverAnswer;
	
	// Device index
	if (1 != sscanf(answer_p, "%d %n", &deviceIndex, &p_shift)) {
		logMessage("Unable to read the device index. Server answer body: \"%s\"", serverAnswer);
		return;
	}
	answer_p += p_shift;

	// ADC
	for (chIndex=0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) {
		if (1 != sscanf(answer_p, "%lf %n", &ADC_STORED_VALS[deviceIndex][chIndex], &p_shift)) {
			logMessage("Unable to read the ADC values. ch: %d. Server answer body: \"%s\"", chIndex, serverAnswer);
			return;
		}
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ADC_FIELDS[deviceIndex][chIndex], ADC_STORED_VALS[deviceIndex][chIndex]); 
		answer_p += p_shift;
	}
	
	SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ADC_STORED_VALS[deviceIndex][CFG_DUPLICATE_ADC_INDEX]); 
	
	// DAC
	for (chIndex=0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) {
		if (1 != sscanf(answer_p, "%lf %n", &DAC_SERVER_READ_VALS[deviceIndex][chIndex], &p_shift)) {
			logMessage("Unable to read the DAC values. ch: %d. Server answer body: \"%s\"", chIndex, serverAnswer);
			return;
		}
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_DAC_CONF_FIELDS[deviceIndex][chIndex], DAC_SERVER_READ_VALS[deviceIndex][chIndex]); 
		answer_p += p_shift;
	}
	
	// Input registers, output registers
	if (2 != sscanf(answer_p, "%X %X %n", &STORED_INPUT_REGS[deviceIndex], &STORED_OUTPUT_REGS[deviceIndex], &p_shift)) {
		logMessage("Unable to read the registers data. Server answer body: \"%s\"", serverAnswer);
		return;
	}

	for (chIndex=0; chIndex < PSMCU_INREG_BITS_NUM; chIndex++) {
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INREG_INDICATORS[deviceIndex][chIndex], (STORED_INPUT_REGS[deviceIndex] >> chIndex) & 1);
	}
	SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_INREG_DUPLICATE_INDICATOR[deviceIndex], (STORED_INPUT_REGS[deviceIndex] >> 1) & 1);
	
	for (chIndex=0; chIndex < PSMCU_OUTREG_BITS_NUM; chIndex++) {
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_OUTREG_INDICATORS[deviceIndex][chIndex], (STORED_OUTPUT_REGS[deviceIndex] >> chIndex) & 1);
	}
	
	SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[deviceIndex][0], (STORED_OUTPUT_REGS[deviceIndex] >> 2) & 1);
	SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[deviceIndex][1], (STORED_OUTPUT_REGS[deviceIndex] >> 1) & 1);

	answer_p += p_shift;  
	
	// Parse device state (1st bit - alive status, 2nd bit - error status)
	if (1 != sscanf(answer_p, "%X", &deviceState)) {
		logMessage("Unable to read the alive status. Server answer body: \"%s\"", serverAnswer);
		return;
	}
	
	// Alive status 
	PSMCU_ALIVE_STATUS[deviceIndex] = deviceState & 1;
	errorStatus = (deviceState >> 1) & 1;
	
	if (PSMCU_ERROR_STATUS[deviceIndex] != errorStatus) {
	    // State changed
		PSMCU_ERROR_STATUS[deviceIndex] = errorStatus;
		SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ERROR_STATUS_BOX[deviceIndex], ATTR_VISIBLE, errorStatus);
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ERROR_STATE_LED[deviceIndex], errorStatus);
		
		// Change the visibility of buttons for displaying and clearing error messages
		SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ERROR_SHOW_BTN[deviceIndex], ATTR_VISIBLE, errorStatus);
		SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ERROR_CLEAR_BTN[deviceIndex], ATTR_VISIBLE, errorStatus);
	}
	
	SetCtrlVal(mainMenuHandle, PSMCU_STATUS_INDICATOR[deviceIndex], PSMCU_ALIVE_STATUS[deviceIndex]);
	
	// Resolve command buttons status
	resolveCommandButtonStatus(
		deviceIndex,
		STORED_INPUT_REGS[deviceIndex],
		STORED_OUTPUT_REGS[deviceIndex],
		DAC_SERVER_READ_VALS[deviceIndex][0]
	);
	
	// Resolve color indication
	resolvePsmcuColorIndicators(
		deviceIndex,
		STORED_INPUT_REGS[deviceIndex],
		DAC_CLIENT_SENT_VALUES[deviceIndex][0], 
		DAC_SERVER_READ_VALS[deviceIndex][0],
		ADC_STORED_VALS[deviceIndex][CFG_DUPLICATE_ADC_INDEX],
		CFG_DAC_DAC_MAX_DIFF,
		CFG_DAC_ADC_MAX_DIFF
	);
}


void parseCanGwConnectionStatus(char *serverAnswer) {
	if (1 != sscanf(serverAnswer, "%d", &canGwConnectionStatus)) {
		logMessage("Unable to read the connection status. Server answer body: \"%s\"", serverAnswer);
		return;
	}
	
	if (canGwConnectionStatus)
		SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, CANGW_INDICATOR_ONLINE_LABEL);
	else
		SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, CANGW_INDICATOR_OFFLINE_LABEL);
	
	SetCtrlVal(mainMenuHandle, CANGW_CONNECTION_INDICATOR, canGwConnectionStatus);
}


void parseSingleErrorMessage(char *serverAnswer) {
	char *answer_p;
	char new_title[256];
	int deviceIndex;
	int p_shift;

	answer_p = serverAnswer;
	
	// Device index
	if (1 != sscanf(answer_p, "%d %n", &deviceIndex, &p_shift)) {
		logMessage("Unable to read the device index. Server answer body: \"%s\"", serverAnswer);
		return;
	}
	answer_p += p_shift;
	
	// Update the error message panel's title
	sprintf(new_title, "Error for \"%s\"", PSMCU_DEV_NAME[deviceIndex]);
	SetPanelAttribute(errPanelHandle, ATTR_TITLE, new_title);
	
	// Clear the previous message and set up a new one
	ResetTextBox(errPanelHandle, errPanel_textbox, answer_p);
}


// =================================
// User commands
// =================================

void UserSingleReset(int deviceIndex) {
	int currentPermission, forcePermission;
	
	currentPermission = (STORED_OUTPUT_REGS[deviceIndex] >> 1) & 1;
	forcePermission = (STORED_OUTPUT_REGS[deviceIndex] >> 2) & 1;
	
	// The reset is prohibited if force or current permission is set.
	if (currentPermission || forcePermission) return;
	
	SendDeviceCommand("PSMCU:SINGLE:INTERLOCK:DROP", deviceIndex);
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_CURRENT_PERM_BTNS[deviceIndex][0], ATTR_DIMMED, 1); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_FORCE_BTNS[deviceIndex][0], ATTR_DIMMED, 1); 
	logMessage("Send reset signal to '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleForceOn(int deviceIndex) {
	int interlockReset, currentPermission;
	
	interlockReset = (STORED_OUTPUT_REGS[deviceIndex] >> 0) & 1;  
	currentPermission = (STORED_OUTPUT_REGS[deviceIndex] >> 1) & 1;
	
	if (interlockReset) return;
	if (currentPermission) return;
	
	SendDeviceCommand("PSMCU:SINGLE:FORCE:ON", deviceIndex); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[deviceIndex], ATTR_DIMMED, 1);
	logMessage("Send force:on signal to '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleForceOff(int deviceIndex) {
	SendDeviceCommand("PSMCU:SINGLE:FORCE:OFF", deviceIndex);
	logMessage("Send force:off signal to '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleGetErrMsg(int deviceIndex) {
	SendDeviceCommand("PSMCU:SINGLE:ERROR:GET", deviceIndex);
}


void UserSingleClearErr(int deviceIndex) {
	SendDeviceCommand("PSMCU:SINGLE:ERROR:CLEAR", deviceIndex);
	logMessage("Send request to clear error message for '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleCurrentOn(int deviceIndex) {
	int interlockReset, forcePermission, contactor;
	
	interlockReset = (STORED_OUTPUT_REGS[deviceIndex] >> 0) & 1;
	forcePermission = (STORED_OUTPUT_REGS[deviceIndex] >> 2) & 1;
	contactor = (STORED_INPUT_REGS[deviceIndex] >> 1) & 1;

	if (interlockReset) return;
	if ( !contactor || !forcePermission ) return;
	if (DAC_SERVER_READ_VALS[deviceIndex][0] != 0) return; 
	
	SendDeviceCommand("PSMCU:SINGLE:PERMISSION:ON", deviceIndex); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[deviceIndex], ATTR_DIMMED, 1);
	logMessage("Send current:on signal to '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleCurrentOff(int deviceIndex) {
	SendDeviceCommand("PSMCU:SINGLE:PERMISSION:OFF", deviceIndex);
	logMessage("Send current:off signal to '%s'", PSMCU_DEV_NAME[deviceIndex]);
}


void UserSingleZeroDacFast(int deviceIndex) {
		DAC_CLIENT_SENT_VALUES[deviceIndex][CFG_DUPLICATE_DAC_INDEX] = 0;
		SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], 0.);
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_DAC_FIELDS[deviceIndex][CFG_DUPLICATE_DAC_INDEX], 0.); 
		SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_CURRENT_PERM_BTNS[deviceIndex][0], ATTR_DIMMED, 1);
		SendDACValue(deviceIndex, CFG_DUPLICATE_DAC_INDEX, 0., 1, 1);  			
}


void UserAllReset(void) {
	int deviceIndex;
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) UserSingleReset(deviceIndex);
}


void UserAllForceOn(void) {
	int deviceIndex;
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) UserSingleForceOn(deviceIndex);	
}


void UserAllForceOff(void) {
	SendBroadcastCommand("PSMCU:ALL:FORCE:OFF"); 		
}


void UserAllCurrentOn(void) {
	int deviceIndex;
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) UserSingleCurrentOn(deviceIndex);	
}


void UserAllCurrentOff(void) {
	SendBroadcastCommand("PSMCU:ALL:PERMISSION:OFF"); 		
}


void UserAllZeroDacFast(void) {
	int deviceIndex;
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) UserSingleZeroDacFast(deviceIndex);	
}


void UserAllZeroDacSmart(void) {
	double dacStates[PSMCU_MAX_NUM] = {0};
	double timeDelays_sec[PSMCU_MAX_NUM];
	int deviceIndex;
	
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++)
		timeDelays_sec[deviceIndex] = ceil(DAC_SERVER_READ_VALS[deviceIndex][0] / CFG_MAX_CURRENT_CHANGE_RATE) + CFG_EXTRA_WAIT_TIME; 
	
	dacSetInOrder(dacStates, timeDelays_sec, 1);		
}


void UserSingleDacSet(int deviceIndex, int channelIndex, double dacValue) {
	
	// Dim the current permission button
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_CURRENT_PERM_BTNS[deviceIndex][0], ATTR_DIMMED, 1);
	
	// Update DAC fields
	SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_DAC_FIELDS[deviceIndex][CFG_DUPLICATE_DAC_INDEX], dacValue); 
	if (channelIndex == CFG_DUPLICATE_DAC_INDEX) {
		SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], dacValue);	
	}
	
	DAC_CLIENT_SENT_VALUES[deviceIndex][channelIndex] = dacValue; 
	
	// Send data to the server
	SendDACValue(deviceIndex, channelIndex, dacValue, 0, 0);	
}


void dacSetInOrder(double dacStates[PSMCU_MAX_NUM], double timeDelay_sec[PSMCU_MAX_NUM], int reversed) {
	int deviceIndex, orderIndex;
	int start = 0;
	int clocksToWait;
	
	pipelineGuiLock = 1;
	pipelineBreakFlag = 0;
	SetCtrlAttribute(mainMenuHandle, PSMCU_PIPELINE_STOP_BTN, ATTR_DIMMED, 0);
	
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, MakeColor(255,255,145));	
	}
	
	for (orderIndex=0; orderIndex < PSMCU_NUM; orderIndex++) {
		if (pipelineBreakFlag) {
			logMessage("Loading stopped by the user.");
			goto pipelineEnd;
		}

		if (reversed) {
			deviceIndex = PSMCU_DEVICES_ORDER[PSMCU_NUM - orderIndex - 1];  	
		} else {
			deviceIndex = PSMCU_DEVICES_ORDER[orderIndex];  	
		}

		logMessage("Sending value %f to the DAC of '%s'.", dacStates[deviceIndex], PSMCU_DEV_NAME[deviceIndex]); 
		UserSingleDacSet(deviceIndex, 0, dacStates[deviceIndex]);
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, MakeColor(145,255,145));
		
		// Wait some time, before
		clocksToWait = CLOCKS_PER_SEC * timeDelay_sec[deviceIndex];
		start = clock();
		while (clock() - start < clocksToWait) {
			ProcessSystemEvents();
			if (pipelineBreakFlag) {
				logMessage("Loading stopped by the user.");
				goto pipelineEnd;
			}
		}
		
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, VAL_TRANSPARENT);
	}
	
	logMessage("Loaded setup successfully.");
	
	pipelineEnd:

	pipelineBreakFlag = 0;
	pipelineGuiLock = 0;
	
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, VAL_TRANSPARENT);	
	}
	
	SetCtrlAttribute(mainMenuHandle, PSMCU_PIPELINE_STOP_BTN, ATTR_DIMMED, 1);
}


void dacStatesLoader(double dacStates[PSMCU_MAX_NUM]) {
	double timeDelays_sec[PSMCU_MAX_NUM];
	int deviceIndex;
	
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++)
		timeDelays_sec[deviceIndex] = ceil(dacStates[deviceIndex] / CFG_MAX_CURRENT_CHANGE_RATE) + CFG_EXTRA_WAIT_TIME; 

	dacSetInOrder(dacStates, timeDelays_sec, 0);
}


// =================================
// Logging
// =================================
int WriteDataFilesOnSchedule(void) {
	static int dataLogWaitingTics = 0;
	static char data[2048];

	if (!connectionEstablished) return 0;
	
	if (dataLogWaitingTics * TIMER_TICK_TIME >= CFG_DATA_WRITE_INTERVAL) {
		assembleDataForLogging(data);
		WriteDataFiles(data, CFG_DATA_DIRECTORY, dataFileName);
		dataLogWaitingTics = 0;
		return 1;
	} else {
		dataLogWaitingTics++;	
	}	

	return 0;
}


void assembleDataForLogging(char * target) {
	char *pos;
	int deviceIndex;

	strcpy(target, "");
	pos = target;
	
	#define DEVICE_ITER(index) for((index)=0; (index) < PSMCU_NUM; (index)++)
	
	// Connection status
	pos += sprintf(pos, "%d %d ", connectionEstablished, canGwConnectionStatus); 
	
	DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%d ", PSMCU_ALIVE_STATUS[deviceIndex]); }
	DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", DAC_SERVER_READ_VALS[deviceIndex][0]); }
	DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", ADC_STORED_VALS[deviceIndex][4]); } 
	DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%02X ", STORED_INPUT_REGS[deviceIndex]); }
	DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%02X ", STORED_OUTPUT_REGS[deviceIndex]); }  
	
	if (CFG_DATA_LOG_CURR_USER) DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", DAC_CLIENT_SENT_VALUES[deviceIndex][0]); }
	if (CFG_DATA_LOG_CURR_CONF) DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", ADC_STORED_VALS[deviceIndex][0]); }
	if (CFG_DATA_LOG_TEMP) DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", ADC_STORED_VALS[deviceIndex][1]); } 
	if (CFG_DATA_LOG_REF_V) DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", ADC_STORED_VALS[deviceIndex][2]); }
	if (CFG_DATA_LOG_RESERVED) DEVICE_ITER(deviceIndex) { pos += sprintf(pos, "%.3lf ", ADC_STORED_VALS[deviceIndex][3]); }
	
	target[strlen(target) - 1] = 0;
}

//============================================================================== 


//==============================================================================
// UI callback function prototypes

int CVICALLBACK dacFieldCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	
	int deviceIndex, channelIndex;
	double dacValueToWrite;
	
	switch (event) {
		case EVENT_COMMIT:
			if (panel == mainMenuHandle) {
				for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
					if (PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex] == control) {
						GetCtrlVal(panel, control, &dacValueToWrite);
						logMessage("Sending value %f to the DAC of '%s' via main control window.", dacValueToWrite, PSMCU_DEV_NAME[deviceIndex]);
						UserSingleDacSet(deviceIndex, CFG_DUPLICATE_DAC_INDEX, dacValueToWrite); 
						break;
					}
				}
			} else {			 
				for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
					if (psMcuWindowHandles[deviceIndex] == panel) {
						for (channelIndex=0; channelIndex < PSMCU_DAC_CHANNELS_NUM; channelIndex++) {
							if (PSMCU_BLOCK_DAC_FIELDS[deviceIndex][channelIndex] == control) {
								GetCtrlVal(panel, control, &dacValueToWrite);
								logMessage("Sending value %f to the DAC of '%s' via device's control window.", dacValueToWrite, PSMCU_DEV_NAME[deviceIndex]);
								UserSingleDacSet(deviceIndex, channelIndex, dacValueToWrite);
								break;
							}
						}		
					}
				}
			}
			break;
	}
	return 0;
}


int CVICALLBACK zeroSingleFastBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {

	int deviceIndex;
	
	switch (event) {
		case EVENT_LEFT_DOUBLE_CLICK:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (PSMCU_BLOCK_ZERO_FAST_BTN[deviceIndex] == control) {
					logMessage("Used button 'Zero DAC :: fast' for '%s'", PSMCU_DEV_NAME[deviceIndex]);
					UserSingleZeroDacFast(deviceIndex);
					break;
				}
			}

			break;
	}
	return 0;
}


int CVICALLBACK tick (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	static int flash_count = 0;
	static int flash_state = 0;
	int deviceIndex;

	switch (event)
	{
		case EVENT_TIMER_TICK:
			if (connectionEstablished) {
				// Process commands from a queue
				ProcessCommandsQueue();
				
				// sending request to the server
				if (globalWaitingForRequest * TIMER_TICK_TIME >= CFG_SERVER_DATA_REQUEST_RATE) {
					requestDataFromServer();
					globalWaitingForRequest = 0;
				} else {
					globalWaitingForRequest++;
				}
				
				// CanGw status request
				if (waitingForCanGwStatisRequest * TIMER_TICK_TIME >= CFG_CANGW_REQUEST_INTERVAL) {
					requestCanGwStatus();
					waitingForCanGwStatisRequest = 0;
				} else {
					waitingForCanGwStatisRequest++;	
				}
				
				// updating graphs
				if (waitingForGraphUpdate * TIMER_TICK_TIME >= 1) {
					UpdateGraphs(); 
					waitingForGraphUpdate = 0;
				} else {
					waitingForGraphUpdate++;	
				}
				
				// writing data to files
				WriteDataFilesOnSchedule();

			} else {
				connectionWait++;
				if (connectionWait * TIMER_TICK_TIME >= CFG_SERVER_CONNECTION_INTERVAL) {
					connectionWait = 0;
					logMessage("Connection to the server (\"%s:%d\") ...", CFG_SERVER_IP, CFG_SERVER_PORT);
					if (initConnectionToServer() >= 0) {
						connectionEstablished = 1;
						
						SetCtrlVal(mainMenuHandle, SERVER_CONNECTION_INDICATOR, 1);
						SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, SERVER_INDICATOR_ONLINE_LABEL);
						logMessage("Connection to the server is established.");
						
						RequestNames();
						
						if (!infoFileCreated) {
							infoFileCreated = 1;
							createInfoFile();
						}
						
					} else {
						logMessage("Connection to the server failed. Next connection request in %.1fs.", CFG_SERVER_CONNECTION_INTERVAL);
					}
				}
			}
			if (msMsgsAvailable(msGMS())) {
				msPrintMsgs(msGMS(), stdout);
				WriteLogFiles(msGMS(), CFG_LOG_DIRECTORY, logFileName);
				msFlushMsgs(msGMS());
			}
			
			// Process timer based GUI update
			// Display errors box 
			flash_count++;
			if (flash_count * TIMER_TICK_TIME > 0.5) {
				flash_count = 0;
				flash_state = 1 - flash_state;
				
				for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
					if (PSMCU_ERROR_STATUS[deviceIndex]) {
				    	SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ERROR_STATUS_BOX[deviceIndex], ATTR_VISIBLE, flash_state);
						SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ERROR_STATE_LED[deviceIndex], flash_state);
					}
				}
			}
			
			break;
	}
	return 0;
}


int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData){
	int bufInt;
	switch(xType)
	{
		case TCP_CONNECT:
			break;
		case TCP_DISCONNECT:
			connectionEstablished = 0;
			globalWaitingForRequest = 0;
			waitingForDataWriting = 0;
			waitingForGraphUpdate = 0;
			waitingForCanGwStatisRequest = 0;
			
			initQueue(&COMMANDS_QUEUE);
			
			SetCtrlVal(mainMenuHandle, SERVER_CONNECTION_INDICATOR, 0);
			SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, SERVER_INDICATOR_OFFLINE_LABEL);
			logMessage("Connection to the server has lost. Next connection request in %.1fs.", CFG_SERVER_CONNECTION_INTERVAL);
			break;
		case TCP_DATAREADY:
			switch (globalWaitingForAnswer){
				case WAIT_FOR_BLOCK_NAMES:
					ClientTCPRead(handle, globalAnswerString, sizeof(globalAnswerString), 10);
					globalWaitingForAnswer = 0;
					break;
					
				default:
					bufInt = ClientTCPRead(handle, globalAnswerString, sizeof(globalAnswerString), 10);
					if (bufInt < 0) {
						// error	
					} else {
						parseUbsIncomingCommand(globalAnswerString, bufInt);
					}
					break;
			}
			break;
	}
	return 0;
}


int CVICALLBACK mainSystemCallback (int handle, int control, int event, void *callbackData, 
									int eventData1, int eventData2)
{
	switch(event) {
		case EVENT_END_TASK: 
			switch(eventData1)
			{
				case APP_CLOSE:
				case SYSTEM_CLOSE:
					DiscardAllResources();
					break;
			}
			break;	
	}
	return 0;
}


int CVICALLBACK interlockResetCmdBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleReset(deviceIndex);
			break;
	}
	
	return 0;  	
}


int CVICALLBACK currentOnCmdBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleCurrentOn(deviceIndex);
			break;
	}
	
	return 0;	
}


int CVICALLBACK currentOffCmdBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleCurrentOff(deviceIndex);
			break;
	}
	
	return 0; 	
}


int CVICALLBACK forceOnCmdBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleForceOn(deviceIndex);
			break;
	}
	
	return 0;  	
}


int CVICALLBACK forceOffCmdBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) {
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleForceOff(deviceIndex);
			break;
	}
	
	return 0;  	
}

int CVICALLBACK errShowBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) { 
	
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleGetErrMsg(deviceIndex);
			DisplayPanel(errPanelHandle);
			break;
	}
	
	return 0; 
}

int CVICALLBACK errClearBtnsCallback (int handle, int control, int event, void *callbackData, int eventData1, int eventData2) { 
	
	int deviceIndex;
	
	switch (event) {
		case EVENT_COMMIT:
			for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
				if (psMcuWindowHandles[deviceIndex] == handle) break;	
			}
			if (deviceIndex >= PSMCU_NUM) break;
			
			UserSingleClearErr(deviceIndex);
			break;
	}
	
	return 0; 
}

//////////////////////////////////////////////
// Main menu callbacks for broadcast commands
//////////////////////////////////////////////
int CVICALLBACK loadDacBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case EVENT_COMMIT:
			LoadCurrentState(dacStatesLoader); 
			break;
	}
	
	return 0; 	
}


int CVICALLBACK saveDacBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case EVENT_COMMIT:
			SaveCurrentState();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK zeroAllSmartBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case EVENT_LEFT_DOUBLE_CLICK:
			logMessage("Used broadcast button 'Zero all DAC :: auto'");
			UserAllZeroDacSmart();
			break;
	}
	
	return 0; 	
}


#define BROADCAST_TARGET_EVENT EVENT_COMMIT
//#define BROADCAST_TARGET_EVENT EVENT_LEFT_DOUBLE_CLICK 


int CVICALLBACK zeroAllFastBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case EVENT_LEFT_DOUBLE_CLICK:
			logMessage("Used broadcast button 'Zero all DAC :: fast'");
			UserAllZeroDacFast();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allResetBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			logMessage("Used broadcast button 'Reset All'");
			UserAllReset();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allForceOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			logMessage("Used broadcast button 'Force On'"); 
			UserAllForceOn();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allForceOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			logMessage("Used broadcast button 'Force Off'");
			UserAllForceOff();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allCurrentOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			logMessage("Used broadcast button 'Current On'");
			UserAllCurrentOn();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allCurrentOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			logMessage("Used broadcast button 'Current Off'");
			UserAllCurrentOff();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK stopPipelineCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case EVENT_COMMIT:
			pipelineBreakFlag = 1;
			break;
	}
	
	return 0; 	
}

void CVICALLBACK menuCommandsClearAllErrors (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SendBroadcastCommand("PSMCU:ALL:ERROR:CLEAR");
	logMessage("Send request to clear error messages for all devices.");
}

void CVICALLBACK menuExtraReloadNames (int menuBar, int menuItem, void *callbackData,
		int panel)
{
}

void CVICALLBACK debugSetErrorsAll (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SendBroadcastCommand("PSMCU:ALL:ERROR:SET Error set for all devices!");	
}

void CVICALLBACK resetCgwReconn (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	logMessage("Sent request for resetting the CanGw reconnection counter");
	SendBroadcastCommand("PSMCU:DBG:RSTCGWCONN");
	
}

void CVICALLBACK debugSetErrorsSingle (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	SendBroadcastCommand("PSMCU:SINGLE:ERROR:SET 0 Error set for the first device!");
}

void CVICALLBACK menuProgramSaveView (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int blockIndex, chIndex;
	char file_path[1024];
	IniText iniText; 
	char section[256];
	char k_visibility[16], k_top[16], k_left[16], k_min[16], k_max[16];
	char k_width[16], k_height[16], k_opened[16];
	int val_int, handle;
	
	if (FileSelectPopup("", "*.view", "*.view", "Select the view file", VAL_SAVE_BUTTON, 0, 1, 1, 1, file_path) == 0) return;
	
	iniText = Ini_New(0);
	
	// Save app identifier for the view
	Ini_PutString(iniText, "General", "app", "psmcu-client");
	
	strcpy(k_visibility, "visible");
	strcpy(k_top, "top");
	strcpy(k_left, "left");
	strcpy(k_min, "min");
	strcpy(k_max, "max");
	strcpy(k_width, "width");
	strcpy(k_height, "height");
	strcpy(k_opened, "opened");
	
	strcpy(section, "MainWindow");
	// Save main window parameters
	// left, top, number of blocks
	Ini_PutInt(iniText, section, "blocks_num", PSMCU_NUM);
	GetPanelAttribute(mainMenuHandle, ATTR_TOP, &val_int);
	Ini_PutInt(iniText, section, k_top, val_int);
	GetPanelAttribute(mainMenuHandle, ATTR_LEFT, &val_int);
	Ini_PutInt(iniText, section, k_left, val_int);
	
	for (blockIndex=0; blockIndex < PSMCU_NUM; blockIndex++) {
        // Save visibility and position of the window itself
		sprintf(section, "block-%d", blockIndex);
		GetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_TOP, &val_int);
		Ini_PutInt(iniText, section, k_top, val_int);
		GetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_LEFT, &val_int);
		Ini_PutInt(iniText, section, k_left, val_int);
		GetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_VISIBLE, &val_int);
		Ini_PutInt(iniText, section, k_visibility, val_int);
		
		
		// Get the parameters of each graph plot
		for (chIndex=0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) {
			handle = PSMCU_GRAPHS_WINDOW_HANDLES[blockIndex][chIndex];
			
			sprintf(section, "block-%d-plot-%d", blockIndex, chIndex);
			Ini_PutInt(iniText, section, k_opened, handle >= 0);
			
			if (handle < 0) {
			    // Window is closed	
				Ini_PutInt(iniText, section, k_visibility, 0);
				Ini_PutInt(iniText, section, k_top, 0);
				Ini_PutInt(iniText, section, k_left, 0);
				Ini_PutInt(iniText, section, k_width, 0);
				Ini_PutInt(iniText, section, k_height, 0);

				// Ranges
				Ini_PutDouble(iniText, section, k_min, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][0]);
				Ini_PutDouble(iniText, section, k_max, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][1]);
			} else {
			    // Window is opened
				
				// Visibility, position and size 
				GetPanelAttribute(handle, ATTR_VISIBLE, &val_int);
				Ini_PutInt(iniText, section, k_visibility, val_int);
				GetPanelAttribute(handle, ATTR_TOP, &val_int);
				Ini_PutInt(iniText, section, k_top, val_int);
				GetPanelAttribute(handle, ATTR_LEFT, &val_int);
				Ini_PutInt(iniText, section, k_left, val_int);
				GetPanelAttribute(handle, ATTR_WIDTH, &val_int); 
				Ini_PutInt(iniText, section, k_width, val_int);
				GetPanelAttribute(handle, ATTR_HEIGHT, &val_int); 
				Ini_PutInt(iniText, section, k_height, val_int);
				
				// Ranges
				Ini_PutDouble(iniText, section, k_min, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][0]);
				Ini_PutDouble(iniText, section, k_max, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][1]);
			}
			
		}

	}
	
	// Trying to open the file for writing
	if( Ini_WriteToFile(iniText, file_path) < 0 ) {
		MessagePopup("Unable to save the program's view", "Unable to open the specified file for writing.");
		Ini_Dispose(iniText);
		return;
	}
	
	logMessage("Saved program's view to '%s'", file_path);
	
	// Free the resources
	Ini_Dispose(iniText);
	
}

void CVICALLBACK menuProgramLoadView (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int blocks_num;
	int blockIndex, chIndex;
	char file_path[1024];
	char app_name[256], msg[256];
	IniText iniText; 
	char section[256];
	char k_visibility[16], k_top[16], k_left[16], k_min[16], k_max[16];
	char k_width[16], k_height[16], k_opened[16];

	int visible, top, left, width, height;

	if (FileSelectPopup("", "*.view", "*.view", "Select the view file", VAL_LOAD_BUTTON, 0, 1, 1, 1, file_path) == 0) return; 
	
	iniText = Ini_New(0);
	
	#define STOP_CONFIGURATION(s, k) sprintf(msg, "Cannot read '%s' from the '%s' section.", (k), (s)); MessagePopup("View configuration Error", msg); Ini_Dispose(iniText); return;
    #define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	
	// Trying to open the file for reading
	if( Ini_ReadFromFile(iniText, file_path) != 0 ) {
		MessagePopup("Unable to load the program's view", "Unable to open the specified file for reading.");
		Ini_Dispose(iniText);
		return;
	}
	
	logMessage("Loading the program's view from '%s'", file_path);
	
	// Load and chech the app identifier for the view
	Ini_GetStringIntoBuffer(iniText, "General", "app", app_name, 256);
	if (strcmp(app_name, "psmcu-client") != 0) {
		logMessage("Unable to load the program's view. Expected app name 'psmcu-client', got '%s'", app_name);
	    MessagePopup("Unable to load the program's view", "The program app type is incorrect. Expected 'psmcu-client'.");
		Ini_Dispose(iniText);
		return;	
	}
	
	strcpy(k_visibility, "visible");
	strcpy(k_top, "top");
	strcpy(k_left, "left");
	strcpy(k_min, "min");
	strcpy(k_max, "max");
	strcpy(k_width, "width");
	strcpy(k_height, "height");
	strcpy(k_opened, "opened");
	
	// Load parameters of the main window (left, top, number of blocks)
	strcpy(section, "MainWindow");

	READ_INT(section, "blocks_num", blocks_num);
	if (blocks_num != PSMCU_NUM) {
		logMessage("The number of blocks in the view configuration file (%d) is different from the number of blocks (%d) in client configuration.", blocks_num, PSMCU_NUM);	
	}
	
    // Using the minimum value between PSMCU_NUM and blocks_num
	if (blocks_num > PSMCU_NUM) blocks_num = PSMCU_NUM;
	
	READ_INT(section, k_top, top);
	READ_INT(section, k_left, left);
	SetPanelAttribute(mainMenuHandle, ATTR_TOP, top);
	SetPanelAttribute(mainMenuHandle, ATTR_LEFT, left);
	
	// Loading blocks view
	for (blockIndex=0; blockIndex < blocks_num; blockIndex++) {
		// Load visibility and position of the window itself
		sprintf(section, "block-%d", blockIndex);

		READ_INT(section, k_top, top);
		SetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_TOP, top);

		READ_INT(section, k_left, left);
		SetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_LEFT, left);
		
		READ_INT(section, k_visibility, visible); 
		SetPanelAttribute(psMcuWindowHandles[blockIndex], ATTR_VISIBLE, visible);
		
		// Setup graph windows for each block
		for (chIndex=0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) {
			sprintf(section, "block-%d-plot-%d", blockIndex, chIndex);
			
			// Read ranges
			READ_DOUBLE(section, k_min, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][0]);
			READ_DOUBLE(section, k_max, PSMCU_GRAPHS_RANGES[blockIndex][chIndex][1]);
			
			// Read visibility
		    READ_INT(section, k_visibility, visible);
			
			// If visible, display the window, if it is not yet displayed. Otherwise, close the window.
			if (visible) {
				ShowGraphWindow(blockIndex, chIndex);
				
				READ_INT(section, k_top, top);
				READ_INT(section, k_left, left);
				READ_INT(section, k_width, width);
				READ_INT(section, k_height, height);
				
				PlaceGraphWindow(blockIndex, chIndex, top, left, width, height);
			} else {
				CloseGraphWindow(blockIndex, chIndex);	
			}
		}
	}
	
	// Finish the loading
	Ini_Dispose(iniText);
	return;	
}

int CVICALLBACK errPanelCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			HidePanel(errPanelHandle);
			break;
	}
	return 0;
}

void CVICALLBACK ShowHideConsole (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int visible = 0;
	
	GetStdioWindowVisibility(&visible);
	
	if (visible) {
		SetStdioWindowVisibility(0);
	} else {
		SetStdioWindowVisibility(1);
	}
}
