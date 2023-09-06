//==============================================================================
//
// Title:       ClientLogging.c
// Purpose:     A short description of the implementation.
//
// Created on:  24.12.2020 at 12:13:16 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <utility.h>
#include <ansi_c.h>
#include "ClientLogging.h" 
#include "MessageStack.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
void setOrCreateRelDirectory(const char *relDirName) { \
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
//==============================================================================
// Global variables

//==============================================================================
// Global functions
void WriteLogFiles(message_stack_t messageStack, const char *logDirectory, const char *fileName) {
	FILE * outputFile;
	
	if ( msMsgsAvailable(messageStack) ) {

		setOrCreateRelDirectory(logDirectory);

		outputFile = fopen(fileName,"a");
		if (outputFile == NULL) {
			// Inform about errors
		} else {
			msPrintMsgs(messageStack, outputFile);
			fclose(outputFile);
		}
	}
}


void WriteDataFiles(const char *data, const char *dataDirectory, const char *fileName) {
	static time_t curTime;
	FILE * outputFile;
	
	setOrCreateRelDirectory(dataDirectory);

	outputFile = fopen(fileName, "a");
	if (outputFile == NULL) {
		// Inform about errors
	} else {
		time(&curTime);
		fprintf(outputFile, "%u %s\n", curTime, data);
		fclose(outputFile);
	}
}


void WriteDataDescription(message_stack_t messageStack, const char *dataDirectory, const char *fileName) {
	FILE * outputFile;
	static time_t calendarTimeSeconds; 
	static int day, month, year, hours, minutes, seconds;
	
	setOrCreateRelDirectory(dataDirectory);
	
	outputFile = fopen(fileName, "w"); 
	if (outputFile == NULL) {
		// Inform about errors
		return ;
	} 
	
	// Log ftime in different frmats
	time(&calendarTimeSeconds);
	GetSystemDate(&month, &day, &year);
	GetSystemTime(&hours, &minutes, &seconds);
	
	fprintf(outputFile,"(Seconds since January 1, 1900): %u\n", calendarTimeSeconds);
	fprintf(outputFile, "(Formatted date/time): %02d.%02d.%02d %02d:%02d:%02d\n",  year, month, day, hours, minutes, seconds);
	fprintf(outputFile, "\n");
	
	// Log data items
	msPrintMsgs(messageStack, outputFile); 
	
	fclose(outputFile);
}
