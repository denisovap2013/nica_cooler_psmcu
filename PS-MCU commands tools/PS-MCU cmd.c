//==============================================================================
//
// Title:       PS-MCU cmd
// Purpose:     A short description of the command-line tool.
//
// Created on:  22.12.2021 at 10:46:59 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include <windows.h>
#include <tcpsupp.h>
#include "inifile.h"
#include <ansi_c.h>
#include <formatio.h>

//==============================================================================
// Constants
#define TIMER_TICK_TIME 0.100
//==============================================================================
// Types

//==============================================================================
// Static global variables
char serverIP[256];
int serverPort;
unsigned int connectionHandle;
int connectionEstablished, reconnectionInterval;
int checkListPause;
//==============================================================================
// Static functions
void ConfigurateClient(char * configPath);
int CVICALLBACK userMainCallback(int MenuBarHandle, int MenuItemID, int event, void * callbackData, int eventData1, int eventData2);
void DiscardAllResources(void);
int ReconnectToServer(void);
int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData);
void sendCommand(char *command);
void printWithSpecialCharacters(char *string);
void StopThreads(void);

int CVICALLBACK commandsInputThreadFunction (void *data);
int CVICALLBACK serverConversationThreadFunction (void *data);

void prepareCheckList(void);  
//==============================================================================
// Global variables
int runThreads = 1;
int poolHandle;
int commandsInputThreadId, serverConversationThreadId;

char cmdCheckList[100][128];
int numberOfCommandsToCheck = 0;
//==============================================================================
// Global functions

/// HIFN  The main entry-point function.
/// HIPAR argc/The number of command-line arguments.
/// HIPAR argc/This number includes the name of the command-line tool.
/// HIPAR argv/An array of command-line arguments.
/// HIPAR argv/Element 0 contains the name of the command-line tool.
/// HIRET Returns 0 if successful.
int main (int argc, char *argv[])
{

    ConfigurateClient("PS-MCU cmd tool configuration.ini");
	prepareCheckList();
	
	SetStdioWindowVisibility(1);
	InstallMainCallback(userMainCallback, 0, 0);

	if (CmtNewThreadPool(2, &poolHandle) < 0) {
		printf("Unable to create Thread Pool\nPress any key ...\n");
		GetKey();
		exit(0);
	}
	if (CmtScheduleThreadPoolFunction (poolHandle, commandsInputThreadFunction, NULL, &commandsInputThreadId) < 0) {
		printf("Unable to create a thread for the user input\nPress any key ...\n");
		GetKey();
		exit(0);	
	}
	if (CmtScheduleThreadPoolFunction (poolHandle, serverConversationThreadFunction, NULL, &serverConversationThreadId) < 0) {
		printf("Unable to create a thread for the server conversation.\nPress any key ...\n");
		GetKey(); 
		exit(0);	
	}
	
	// CmtWaitForThreadPoolFunctionCompletion (poolHandle, commandsInputThreadId, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
    CmtWaitForThreadPoolFunctionCompletion (poolHandle, serverConversationThreadId, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
	
	DiscardAllResources();
	
    return 0;
}


int CVICALLBACK commandsInputThreadFunction (void *data) {
	char command[256];
	int i;
	
	while (runThreads) {
		printf("Command: ");
		fgets(command, sizeof(command), stdin);

		if (strcmp(command, "!stop\n") == 0) { runThreads = 0;  break; }
		if (strcmp(command, "!connect\n") == 0) {ReconnectToServer(); continue;}; 
		if (strcmp(command, "!check\n") == 0) {
			for (i=0; i<numberOfCommandsToCheck; i++) {
				printf("Sending: \"");
				printWithSpecialCharacters(cmdCheckList[i]);
				printf("\"\n");
				sendCommand(cmdCheckList[i]);	
				Sleep(checkListPause * 1000);
			}
			
			continue;
		}
		if (strcmp(command, "!prepare\n") == 0) { prepareCheckList();  continue; }
		if (strcmp(command, "!help\n") == 0) {
			printf("!help - shows help.\n");  
			printf("!connect - try to reconnect to the server.\n"); 
			printf("!stop - sstops the programm execution.\n"); 
			printf("!prepare - updated the commands check list (reload from file).\n"); 
			printf("!check - run the commands check list.\n"); 
			continue;
		}

		/*printf("Sent command: \"");
		printWithSpecialCharacters(command); 
		printf("\"\n");*/
		sendCommand(command);
	}
	
	return 0;
}


int CVICALLBACK serverConversationThreadFunction (void *data) {
	int i;

	while(runThreads) {
		if (!connectionEstablished) {
			if (!ReconnectToServer()) {
				printf("Next connection attempt in %d seconds.\n", reconnectionInterval);	
			}
		}
		for (i=0; i<reconnectionInterval; i++) {
			ProcessSystemEvents();
			Sleep(1000);	
			if (!runThreads) break;
		}
	}
	
	return 0;
}


void ConfigurateClient(char * configPath) {
	
	#define STOP_CONFIGURATION(x) MessagePopup("Configuration Error", (x)); Ini_Dispose(iniText); exit(0); 
	#define BUF_SIZE 256
	
	IniText iniText;
	iniText = Ini_New(1);

	if (!configPath) {
		STOP_CONFIGURATION("Configuration file path is NULL.");
	}
	
	if( Ini_ReadFromFile(iniText, configPath) < 0 ) {
		STOP_CONFIGURATION("Cannot read the config file.");
	} 
	////////////////////////////
	// FILE
	
	// SERVER
			// IP
	if(Ini_GetStringIntoBuffer(iniText, "TCP", "serverIp", serverIP, BUF_SIZE) <= 0) {
		STOP_CONFIGURATION("Cannot read 'serverIp' from the 'TCP' section.");
	}
			// Port
	if(Ini_GetInt(iniText, "TCP", "serverPort", &serverPort) <= 0) {
		STOP_CONFIGURATION("Cannot read 'serverPort' from the 'TCP' section.");
	}
			// reconnectionInterval
	if(Ini_GetInt(iniText, "TCP", "reconnectionInterval", &reconnectionInterval) <= 0) {
		STOP_CONFIGURATION("Cannot read 'reconnectionInterval' from the 'TCP' section.");
	}
	
			// checkListPause
	if(Ini_GetInt(iniText, "TCP", "checkListPause", &checkListPause) <= 0) {
		STOP_CONFIGURATION("Cannot read 'checkListPause' from the 'TCP' section.");
	}

	////////////////////////////
	Ini_Dispose(iniText);
	return;
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
				case SYSTEM_CLOSE: 
					DiscardAllResources();
					break;
			}
			break;
	}
	
	return 0;
}


int ReconnectToServer(void) {
	if (connectionEstablished) return 1;

	if (ConnectToTCPServer(&connectionHandle, serverPort, serverIP, clientCallbackFunction, NULL, 100) == 0) {
		connectionEstablished = 1;
		printf("Connected to the server.\n");
	} else {
		connectionEstablished = 0;
		printf("Unable to connect to the server.\n");
	}
	
	return connectionEstablished;
}


void DiscardAllResources(void) {
	if(connectionEstablished) {
		DisconnectFromTCPServer(connectionHandle);		
	}
	
	CmtReleaseThreadPoolFunctionID(poolHandle, commandsInputThreadId);
	CmtReleaseThreadPoolFunctionID(poolHandle, serverConversationThreadId);
	CmtDiscardThreadPool(poolHandle);
}


int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData){
	int bufInt;
	static char answer[1024];

	switch(xType)
	{
		case TCP_DISCONNECT:
			connectionEstablished = 0;
			printf("Disconnected from the server\n");
			break;
		case TCP_DATAREADY:
			bufInt = ClientTCPRead(handle, answer, sizeof(answer), 10);
			if (bufInt < 0) {
				printf("Unable to read the answer from the server.\n");	
			} else {
				printf("Answer: \"");
				printWithSpecialCharacters(answer);
				printf("\"\n");
			}
			break;
	}
	return 0;
}


void printWithSpecialCharacters(char *string) {
	int i;

	for (i=0; i<strlen(string); i++) {
		switch(string[i]) {
			case '\n': printf("\\n"); break;
			case '\r': printf("\\r"); break;
			case '\t': printf("\\t"); break;
			default: printf("%c", string[i]);
		}
	}
}


void sendCommand(char *command) {
	if (!connectionEstablished) { printf("Not connected to the server\n"); return; }

	ClientTCPWrite(connectionHandle, command, strlen(command) + 1, 100);
}


void prepareCheckList(void) {
	#define CMD_FILE_NAME "commandsCheckList.txt"
	int i = 0;
	int fileHandle;
	char buffer[256], *sp, *ep;
	
	fileHandle = OpenFile(CMD_FILE_NAME, VAL_READ_ONLY, 0, VAL_ASCII);
	
	if (fileHandle < 0) {
		printf("Unable to open the file \"%s\" containing the commands check list.\n", CMD_FILE_NAME);
		return;
	}
	
	while (ReadLine(fileHandle, buffer, sizeof(buffer)-1) >= 0) {
		sp = buffer;
		while (sp[0] == ' ' || sp[0] == '\t') sp++;
		
		if (strlen(sp) == 0) continue;
		
		ep = &buffer[strlen(buffer) - 1];
		while (ep[0] == ' ' || ep[0] == '\t') ep--;
		
		ep[1] = 0;
		
		sprintf(cmdCheckList[i++], "%s\n", sp);
	}
	
	numberOfCommandsToCheck = i;
	
	CloseFile(fileHandle);
}
