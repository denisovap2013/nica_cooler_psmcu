//==============================================================================
//
//==============================================================================

#ifndef __ServerConfigData_H__
#define __ServerConfigData_H__

//==============================================================================
// Include files

#include "cvidef.h"
#include "psMcuProtocol.h"
//==============================================================================  


// PSMCU parameters
#define CFG_MAX_PSMCU_DEVICES_NUM 40
#define CFG_CGW_MAX_NUM 40
#define CFG_MAX_TYPE_LEN 31

// Server general parameters
extern char     CFG_SERVER_NAME[256];

// CANGW parameters
extern int      CFG_CANGW_BLOCKS_NUM;
extern char 	CFG_CANGW_BLOCK_NAME[CFG_CGW_MAX_NUM][256];
extern char 	CFG_CANGW_IP[CFG_CGW_MAX_NUM][256];
extern int		CFG_CANGW_PORT[CFG_CGW_MAX_NUM];
extern int		CFG_CANGW_BAUD[CFG_CGW_MAX_NUM];
extern int		CFG_CANGW_IGNORE_UNKNOWN[CFG_CGW_MAX_NUM];   
extern int		CFG_CANGW_RECONNECTION_DELAY[CFG_CGW_MAX_NUM];
extern int		CFG_CANGW_RECV_TIMEOUT[CFG_CGW_MAX_NUM];
extern int		CFG_CANGW_SEND_TIMEOUT[CFG_CGW_MAX_NUM];

// Devices parameters
extern int CFG_PSMCU_DEVICES_NUM;
extern int CFG_PSMCU_BLOCKS_IDS[CFG_MAX_PSMCU_DEVICES_NUM];
extern int CFG_PSMCU_DEVICES_IDS[CFG_MAX_PSMCU_DEVICES_NUM];
extern char CFG_PSMCU_DEVICES_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][256];
extern char CFG_PSMCU_DEVICES_TYPES[CFG_MAX_PSMCU_DEVICES_NUM][CFG_MAX_TYPE_LEN];

// Specific values
extern char 	CFG_PSMCU_ADC_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_ADC_CHANNELS_NUM][256];
extern double 	CFG_PSMCU_ADC_COEFF[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_ADC_CHANNELS_NUM][2];  

extern char 	CFG_PSMCU_DAC_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_DAC_CHANNELS_NUM][256]; 
extern double 	CFG_PSMCU_DAC_COEFF[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_DAC_CHANNELS_NUM][2];

extern char 	CFG_PSMCU_INREG_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_INPUT_REGISTERS_NUM][256];
extern char 	CFG_PSMCU_OUTREG_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_OUTPUT_REGISTERS_NUM][256];

extern int 		CFG_PSMCU_DEVICE_DOWNTIME_LIMIT;
extern int		CFG_PSMCU_DEVICE_PING_INTERVAL;
extern int      CFG_PSMCU_REGISTERS_REQUEST_INTERVAL;
extern int      CFG_PSMCU_DAC_REQUEST_INTERVAL;

// Slow mode parameters
extern int      CFG_PSMCU_DAC_SLOW_TIME_DELTA;
extern double   CFG_PSMCU_DAC_SLOW_VOLTAGE_STEP;

// Safe mode parameters
extern double   CFG_PSMCU_ADC_MAX_CURRENT[CFG_MAX_PSMCU_DEVICES_NUM];
extern double   CFG_PSMCU_ADC_SAFE_CURR_THRESH[CFG_MAX_PSMCU_DEVICES_NUM];

// TCP/IP parameters
extern int		CFG_TCP_PORT;
	
// LOG parameters
extern char		CFG_FILE_LOG_DIRECTORY[256];
extern char 	CFG_FILE_DATA_DIRECTORY[256];
extern int		CFG_FILE_DATA_WRITE_INTERVAL; //seconds
extern int 		CFG_FILE_EXPIRATION;	// days

void InitServerConfig(char * configPath);


// Device type settings
typedef struct deviceTypeSettings_t {
	double contactor_delay;
} deviceTypeSettings;

void cfgInitDeviceTypeSettings(void);
void cfgReleaseDeviceTypeSettings(void);
deviceTypeSettings * cfgDeviceTypeSettingsGetDefault(void);
deviceTypeSettings * cfgDeviceTypeSettingsGet(char * type);

// Auxiliary function for printing device type settings
void printDeviceTypeSettings(void);


//==============================================================================  		


#endif  /* ndef __ServerConfigData_H__ */
