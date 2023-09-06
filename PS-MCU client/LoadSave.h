//==============================================================================
//
// Title:       LoadSave.h
// Purpose:     A short description of the interface.
//
// Created on:  08.02.2022 at 10:36:31 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __LoadSave_H__
#define __LoadSave_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"
#include "clientData.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions

void SaveCurrentState(void);
void LoadCurrentState(void (*dacSetupFunction)(double dacStates[PSMCU_MAX_NUM]));

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __LoadSave_H__ */
