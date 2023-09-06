//==============================================================================
//
//==============================================================================

#ifndef __ClientConfiguration_H__
#define __ClientConfiguration_H__


//==============================================================================
// Include files

#include "cvidef.h"
#include "clientData.h"

//==============================================================================
// Global variables
extern char CFG_SERVER_IP[256];
extern unsigned int CFG_SERVER_PORT;
extern double CFG_SERVER_CONNECTION_INTERVAL;
extern double CFG_SERVER_DATA_REQUEST_RATE;
extern int CFG_CANGW_REQUEST_INTERVAL;

extern int CFG_PSMCU_NUMBER;

extern char CFG_LOG_DIRECTORY[256];
extern char CFG_DATA_DIRECTORY[256];
extern double CFG_DATA_WRITE_INTERVAL;

extern int CFG_DUPLICATE_ADC_INDEX;
extern int CFG_DUPLICATE_DAC_INDEX;

extern double CFG_DAC_ADC_MAX_DIFF;
extern double CFG_DAC_DAC_MAX_DIFF;

// Pipeline settings
extern double CFG_WAIT_AFTER_RESET;
extern double CFG_WAIT_AFTER_FORCE_ON;
extern double CFG_WAIT_AFTER_FORCE_OFF;
extern double CFG_WAIT_AFTER_CURRENT_ON;
extern double CFG_WAIT_AFTER_CURRENT_OFF;

extern int CFG_DEVICES_ORDER[PSMCU_MAX_NUM];
extern double CFG_MAX_CURRENT_CHANGE_RATE;
extern double CFG_EXTRA_WAIT_TIME;

// Data logging
extern int CFG_DATA_LOG_CURR_USER;
extern int CFG_DATA_LOG_CURR_CONF;
extern int CFG_DATA_LOG_TEMP;
extern int CFG_DATA_LOG_REF_V;
extern int CFG_DATA_LOG_RESERVED;

//==============================================================================
// Global functions

void ConfigurateClient(char * configPath);

#endif  /* ndef __ClientConfiguration_H__ */
