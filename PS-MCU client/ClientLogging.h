//==============================================================================
//
// Title:       ClientLogging.h
// Purpose:     A short description of the interface.
//
// Created on:  24.12.2020 at 12:13:16 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __ClientLogging_H__
#define __ClientLogging_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"
#include "MessageStack.h"  

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions
void WriteLogFiles(message_stack_t messageStack, const char *logDirectory, const char *fileName);
void WriteDataFiles(const char *data, const char *dataDirectory, const char *fileName);
void WriteDataDescription(message_stack_t messageStack, const char *dataDirectory, const char *fileName);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ClientLogging_H__ */
