//==============================================================================
//
// Title:       Logging.h
// Purpose:     A short description of the interface.
//
// Created on:  17.12.2020 at 10:17:26 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __Logging_H__
#define __Logging_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "MessageStack.h" 

//==============================================================================
// Constants
#define LOG_FILE_PREFIX "psMcuServerLog_"
#define DATA_FILE_PREFIX "psMcuServerData_"

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions

void WriteLogFiles(message_stack_t messageStack, const char *log_directory);
void WriteDataFiles(message_stack_t messageStack, const char *dataDirectory);
void DeleteOldFiles(const char *logDirectory, const char *dataDirectory, int expirationDays);
void copyConfigurationFile(const char *dataDir, const char *configFile);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __Logging_H__ */
