//==============================================================================
//
// Title:       Logging.c
// Purpose:     A short description of the implementation.
//
// Created on:  17.12.2020 at 10:17:26 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files


#include <formatio.h>
#include "toolbox.h"
#include <ansi_c.h>
#include <lowlvlio.h>
#include "Logging.h"
#include "MessageStack.h"
#include "TimeMarkers.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables


//==============================================================================
// Static functions

int extractDate(char * input, int * year, int *month, int *day) {
	char inputCopy[512];
	strcpy(inputCopy, input);
	inputCopy[4] = ' ';
	inputCopy[7] = ' ';
	if (sscanf(inputCopy, "%d %d %d", year, month, day) == 3) return 1; else return 0;
}


void setOrCreateRelDirectory(const char *relDirName) {
	static char dir[500];

	GetProjectDir(dir); 
	strcat(dir, "\\"); 
	strcat(dir, relDirName);
	if (SetDir(dir) < 0) {
		if (MakeDir(dir) == 0) {
			SetDir(dir);	
		}
	}	
}


void GoToTheProjectDir(void) {
	static char dir[500];

	GetProjectDir(dir); 
	SetDir(dir);
}


void RemoveFilesFromDir(const char *dirName, const char *prefix, int expirationDays) {
	char fileName[512], *pos;
	int day, month, year, curDays;

	GetSystemDate(&month, &day, &year);
	curDays = (year-2000) * 365 + month * 30 + day;
	
	if (!FileExists(dirName, 0)) return;
	
	if (SetDir(dirName) == 0) {
		if ( GetFirstFile("*", 1, 0, 0, 0, 0, 0, fileName) == 0 ){
			do {
				if ( (pos = strstr(fileName, prefix)) != NULL ) {
					if (extractDate(pos + strlen(prefix), &year, &month, &day)) {
						if ( (curDays - (year-2000)*365 - month*30 - day) > expirationDays ) {
							DeleteFile(fileName);		
						}
					}
				}
			} while (GetNextFile(fileName) == 0);
		}	
	}	
}


void AppendStringToAFile(const char *data, const char *directory, const char *prefix) {
	static char fileName[512];
	static int day,month,year;
	FILE * outputFile;
	
	GetSystemDate(&month, &day, &year);
	setOrCreateRelDirectory(directory);
	sprintf(fileName, "%s%d.%02d.%02d.dat", prefix, year, month, day);    
	outputFile = fopen(fileName, "a");

	if (outputFile != NULL) {
		fprintf(outputFile, data);
		fclose(outputFile);

	} else {
		// TODO: maybe inform about errors	
		printf("%s [RUNTIME] Unable to open the file for writing: \"%s\"", TimeStamp(0), fileName);
	}	
}
//==============================================================================
// Global variables

//==============================================================================
// Global functions

void WriteLogFiles(message_stack_t messageStack, const char *logDirectory){
	static char fileName[512];
	static int day, month, year;
	FILE * outputFile;
	
	if ( msMsgsAvailable(messageStack) ) {
		////
		GetSystemDate(&month, &day, &year);
		setOrCreateRelDirectory(logDirectory);
		////
		sprintf(fileName, "%s%d.%02d.%02d.dat", LOG_FILE_PREFIX, year, month, day);    
		outputFile = fopen(fileName,"a");
		if (outputFile == NULL) {
			// Inform about errors
		} else {
			msPrintMsgs(messageStack, outputFile);
			fclose(outputFile);
		}
	}
}


void WriteDataFiles(message_stack_t messageStack, const char *dataDirectory) {
	static time_t curTime;
	static char prefix[30];
	static char fileName[512];
	static int day, month, year;
	FILE * outputFile; 

	if ( !msMsgsAvailable(messageStack) ) return;
	
	time(&curTime); 

	sprintf(prefix, "%u ", curTime);

	////
	GetSystemDate(&month, &day, &year);
	setOrCreateRelDirectory(dataDirectory);
	////
	sprintf(fileName, "%s%d.%02d.%02d.dat", LOG_FILE_PREFIX, year, month, day);    
	outputFile = fopen(fileName,"a");
	if (outputFile == NULL) {
		// Inform about errors
	} else {
		msPrintMsgsWithPrefix(messageStack, outputFile, prefix);
		fclose(outputFile);
	}
}


void DeleteOldFiles(const char *logDirectory, const char *dataDirectory, int expirationDays) {
	char dir[500];
	char logDir[512], dataDir[512];
	
	GetProjectDir(dir);
	sprintf(logDir, "%s\\%s", dir, logDirectory);
	sprintf(dataDir, "%s\\%s", dir, dataDirectory);

	// Delete old log files
	RemoveFilesFromDir(logDir, LOG_FILE_PREFIX, expirationDays);
	
	// Delete old data files
	RemoveFilesFromDir(dataDir, DATA_FILE_PREFIX, expirationDays);
}


void copyConfigurationFile(const char *dataDir, const char *configFile) {
	int day, month, year, hours, minutes, seconds;
	char fileName[256];
	
	GetSystemDate(&month, &day, &year);
	GetSystemTime(&hours, &minutes, &seconds);

	GoToTheProjectDir();
	sprintf(fileName, "%s\\serverConfiguration_%02d.%02d.%02d_%02d-%02d-%02d.ini", dataDir, year, month, day, hours, minutes, seconds);

	CopyFile(configFile, fileName); 
}
