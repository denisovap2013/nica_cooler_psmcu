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
int parseDeviceName(char *src, char *prefix, int expectedDevIndex, char *output);    
void requestDataFromServer(void);
void requestCanGwStatus(void);
void ProcessCommandsQueue(void);
void parseUbsIncomingCommand(char *, int);
void SendDACValue(int deviceIndex, int channel, double value, int fast, int confirmFast);
void SendRegistersCommand(char *, int);
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
	// ADC blocks
	for (i=0; i < PSMCU_NUM; i++) {
		errChk (psMcuWindowHandles[i] = LoadPanel (0, "PS-mcu client.uir", psMcuPanel));
		sprintf(title, "PS-MCU %d: Unknown", i+1);
		SetPanelAttribute(psMcuWindowHandles[i], ATTR_TITLE, title);
	}
	//errChk (mainMenuHandle = LoadPanel (0, "DenisovUBSclient.uir", BlockMenu)); 
	
	/////////////
	clearNames();
	initValues();
	initGui();
    /////////////
	
    /* display the panel and run the user interface */
	InstallMainCallback(mainSystemCallback,0,0); 
	
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
		msAddMsg(msGMS(), "%s Server answered with the command error: \"%s\"", TimeStamp(0), revealEscapeSequences(answer));
		return -1;
	}
	
	if (answer[0] == '?') {
		msAddMsg(msGMS(), "%s Server did not recognize the command: \"%s\"", TimeStamp(0), revealEscapeSequences(answer));
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
		msAddMsg(msGMS(), "%s Expected the answer to start with \"%s\", but got \"%s\"", TimeStamp(0), prefix, revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer = src + strlen(prefix);

	if (sscanf(stringPointer, "%d %d %n", &deviceIndex, &channel, &readPos) != 2) {
		msAddMsg(msGMS(), "%s Expected the answer to have a device index and a channel specified, but got \"%s\"", TimeStamp(0), revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer += readPos;
	
	if (deviceIndex != expectedDevIndex || channel != expectedChannel) {
		msAddMsg(msGMS(), "%s Expected device index and channel to be %d and %d, but got answer \"%s\"", TimeStamp(0), expectedDevIndex, expectedChannel, revealEscapeSequences(src)); 
		return 0;
	}
	
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
		msAddMsg(msGMS(), "%s Expected the answer to start with \"%s\", but got \"%s\"", TimeStamp(0), prefix, revealEscapeSequences(src));
		return 0;	
	}
	
	stringPointer = src + strlen(prefix);

	if (sscanf(stringPointer, "%d %n", &deviceIndex, &readPos) != 1) {
		msAddMsg(msGMS(), "%s Expected the answer to have a device index, but got \"%s\"", TimeStamp(0), src);
		return 0;	
	}
	
	stringPointer += readPos;
	
	if (deviceIndex != expectedDevIndex) {
		msAddMsg(msGMS(), "%s Expected device index to be %d, but got answer \"%s\"", TimeStamp(0), expectedDevIndex, revealEscapeSequences(src)); 
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


void SendRegistersCommand(char * cmd, int deviceIndex) {
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
		
		msAddMsg(msGMS(), "%s Cannot handle the server answer \"%s\"", TimeStamp(0), subcommand);  
	}
}


void parseFullInfo(char *serverAnswer) {
	char *answer_p;
	int deviceIndex;
	int p_shift, chIndex;
	
	answer_p = serverAnswer;
	
	// Device index
	if (1 != sscanf(answer_p, "%d %n", &deviceIndex, &p_shift)) {
		msAddMsg(msGMS(), "%s Unable to read the device index. Server answer body: \"%s\"", TimeStamp(0), serverAnswer);
		return;
	}
	answer_p += p_shift;

	// ADC
	for (chIndex=0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) {
		if (1 != sscanf(answer_p, "%lf %n", &ADC_STORED_VALS[deviceIndex][chIndex], &p_shift)) {
			msAddMsg(msGMS(), "%s Unable to read the ADC values. ch: %d. Server answer body: \"%s\"", TimeStamp(0), chIndex, serverAnswer);
			return;
		}
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_ADC_FIELDS[deviceIndex][chIndex], ADC_STORED_VALS[deviceIndex][chIndex]); 
		answer_p += p_shift;
	}
	
	SetCtrlVal(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ADC_STORED_VALS[deviceIndex][CFG_DUPLICATE_ADC_INDEX]); 
	
	// DAC
	for (chIndex=0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) {
		if (1 != sscanf(answer_p, "%lf %n", &DAC_SERVER_READ_VALS[deviceIndex][chIndex], &p_shift)) {
			msAddMsg(msGMS(), "%s Unable to read the DAC values. ch: %d. Server answer body: \"%s\"", TimeStamp(0), chIndex, serverAnswer);
			return;
		}
		SetCtrlVal(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_DAC_CONF_FIELDS[deviceIndex][chIndex], DAC_SERVER_READ_VALS[deviceIndex][chIndex]); 
		answer_p += p_shift;
	}
	
	// Input registers, output registers
	if (2 != sscanf(answer_p, "%X %X %n", &STORED_INPUT_REGS[deviceIndex], &STORED_OUTPUT_REGS[deviceIndex], &p_shift)) {
		msAddMsg(msGMS(), "%s Unable to read the registers data. Server answer body: \"%s\"", TimeStamp(0), serverAnswer);
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
	
	// Resolve command buttons status
	resolveCommandButtonStatus(deviceIndex, STORED_INPUT_REGS[deviceIndex], STORED_OUTPUT_REGS[deviceIndex], DAC_SERVER_READ_VALS[deviceIndex][0]);
	
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

	answer_p += p_shift;  
	
	// Alive status
	if (1 != sscanf(answer_p, "%X", &PSMCU_ALIVE_STATUS[deviceIndex])) {
		msAddMsg(msGMS(), "%s Unable to read the alive status. Server answer body: \"%s\"", TimeStamp(0), serverAnswer);
		return;
	}
	SetCtrlVal(mainMenuHandle, PSMCU_STATUS_INDICATOR[deviceIndex], PSMCU_ALIVE_STATUS[deviceIndex]);
}


void parseCanGwConnectionStatus(char *serverAnswer) {
	if (1 != sscanf(serverAnswer, "%d", &canGwConnectionStatus)) {
		msAddMsg(msGMS(), "%s Unable to read the connection status. Server answer body: \"%s\"", TimeStamp(0), serverAnswer);
		return;
	}
	
	if (canGwConnectionStatus)
		SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, CANGW_INDICATOR_ONLINE_LABEL);
	else
		SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, CANGW_INDICATOR_OFFLINE_LABEL);
	
	SetCtrlVal(mainMenuHandle, CANGW_CONNECTION_INDICATOR, canGwConnectionStatus);
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
	
	SendRegistersCommand("PSMCU:SINGLE:INTERLOCK:DROP", deviceIndex);
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_CURRENT_PERM_BTNS[deviceIndex][0], ATTR_DIMMED, 1); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_FORCE_BTNS[deviceIndex][0], ATTR_DIMMED, 1); 	
}


void UserSingleForceOn(int deviceIndex) {
	int interlockReset, currentPermission;
	
	interlockReset = (STORED_OUTPUT_REGS[deviceIndex] >> 0) & 1;  
	currentPermission = (STORED_OUTPUT_REGS[deviceIndex] >> 1) & 1;
	
	if (interlockReset) return;
	if (currentPermission) return;
	
	SendRegistersCommand("PSMCU:SINGLE:FORCE:ON", deviceIndex); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[deviceIndex], ATTR_DIMMED, 1);
}


void UserSingleForceOff(int deviceIndex) {
	SendRegistersCommand("PSMCU:SINGLE:FORCE:OFF", deviceIndex); 
}


void UserSingleCurrentOn(int deviceIndex) {
	int interlockReset, forcePermission, contactor;
	
	interlockReset = (STORED_OUTPUT_REGS[deviceIndex] >> 0) & 1;
	forcePermission = (STORED_OUTPUT_REGS[deviceIndex] >> 2) & 1;
	contactor = (STORED_INPUT_REGS[deviceIndex] >> 1) & 1;

	if (interlockReset) return;
	if ( !contactor || !forcePermission ) return;
	if (DAC_SERVER_READ_VALS[deviceIndex][0] != 0) return; 
	
	SendRegistersCommand("PSMCU:SINGLE:PERMISSION:ON", deviceIndex); 
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[deviceIndex], ATTR_DIMMED, 1); 
}


void UserSingleCurrentOff(int deviceIndex) {
	SendRegistersCommand("PSMCU:SINGLE:PERMISSION:OFF", deviceIndex);   
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
		if (pipelineBreakFlag) goto pipelineEnd;

		if (reversed) {
			deviceIndex = PSMCU_DEVICES_ORDER[PSMCU_NUM - orderIndex - 1];  	
		} else {
			deviceIndex = PSMCU_DEVICES_ORDER[orderIndex];  	
		}
		
		UserSingleDacSet(deviceIndex, 0, dacStates[deviceIndex]);
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, MakeColor(145,255,145));
		
		// Wait some time, before
		clocksToWait = CLOCKS_PER_SEC * timeDelay_sec[deviceIndex];
		start = clock();
		while (clock() - start < clocksToWait) {
			ProcessSystemEvents();
			if (pipelineBreakFlag) goto pipelineEnd;
		}
		
		SetCtrlAttribute(mainMenuHandle, PSMCU_ORDER_LABELS[deviceIndex], ATTR_TEXT_BGCOLOR, VAL_TRANSPARENT);
	}
	
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
					msAddMsg(msGMS(),"%s Connection to the server (\"%s:%d\") ...", TimeStamp(0), CFG_SERVER_IP, CFG_SERVER_PORT);
					if (initConnectionToServer() >= 0) {
						connectionEstablished = 1;
						
						SetCtrlVal(mainMenuHandle, SERVER_CONNECTION_INDICATOR, 1);
						SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_LABEL_TEXT, SERVER_INDICATOR_ONLINE_LABEL);
						msAddMsg(msGMS(), "%s Connection to the server is established.", TimeStamp(0));
						
						RequestNames();
						
						if (!infoFileCreated) {
							infoFileCreated = 1;
							createInfoFile();
						}
						
					} else {
						msAddMsg(msGMS(),"%s Connection to the server failed. Next connection request in %.1fs.", TimeStamp(0), CFG_SERVER_CONNECTION_INTERVAL);
					}
				}
			}
			if (msMsgsAvailable(msGMS())) {
				WriteLogFiles(msGMS(), CFG_LOG_DIRECTORY, logFileName);
				msFlushMsgs(msGMS());
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
			msAddMsg(msGMS(),"%s Connection to the server has lost. Next connection request in %.1fs.", TimeStamp(0), CFG_SERVER_CONNECTION_INTERVAL);
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
			UserAllZeroDacFast();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allResetBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			UserAllReset();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allForceOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			UserAllForceOn();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allForceOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			UserAllForceOff();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allCurrentOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
			UserAllCurrentOn();
			break;
	}
	
	return 0; 	
}


int CVICALLBACK allCurrentOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	switch (event) {
		case BROADCAST_TARGET_EVENT:
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
