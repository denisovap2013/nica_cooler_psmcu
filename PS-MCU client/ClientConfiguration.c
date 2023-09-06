//==============================================================================
//
//==============================================================================

#include "inifile.h"
#include <ansi_c.h>
#include "ClientConfiguration.h"


char CFG_SERVER_IP[256];
unsigned int CFG_SERVER_PORT;
double CFG_SERVER_CONNECTION_INTERVAL;
double CFG_SERVER_DATA_REQUEST_RATE;
int CFG_CANGW_REQUEST_INTERVAL;

int CFG_PSMCU_NUMBER;

char CFG_LOG_DIRECTORY[256];
char CFG_DATA_DIRECTORY[256];
double  CFG_DATA_WRITE_INTERVAL;

int CFG_DUPLICATE_ADC_INDEX = 4;
int CFG_DUPLICATE_DAC_INDEX = 0;

double CFG_DAC_DAC_MAX_DIFF; 
double CFG_DAC_ADC_MAX_DIFF;

// Pipeline settings
double CFG_WAIT_AFTER_RESET;
double CFG_WAIT_AFTER_FORCE_ON;
double CFG_WAIT_AFTER_FORCE_OFF;
double CFG_WAIT_AFTER_CURRENT_ON;
double CFG_WAIT_AFTER_CURRENT_OFF;

int CFG_DEVICES_ORDER[PSMCU_MAX_NUM];
double CFG_MAX_CURRENT_CHANGE_RATE; 
double CFG_EXTRA_WAIT_TIME;

// Data logging
int CFG_DATA_LOG_CURR_USER;
int CFG_DATA_LOG_CURR_CONF;
int CFG_DATA_LOG_TEMP;
int CFG_DATA_LOG_REF_V;
int CFG_DATA_LOG_RESERVED;

#define FILE_SECTION "FILE"
#define TCP_SECTION "TCP"
#define GENERAL_SECTION "GENERAL"  
#define INDICATION_SECTION "INDICATION" 
#define PIPELINE_SECTION "PIPELINE"   
#define DATA_LOGGING_SECTION "DATA_LOGGING" 


void ConfigurateClient(char * configPath) {
	char msg[256];
	int queueIndex;
	char key[256];
	
	IniText iniText;
	iniText = Ini_New(1);
	
	#define STOP_CONFIGURATION(s, k) sprintf(msg, "Cannot read '%s' from the '%s' section.", (k), (s)); MessagePopup("Configuration Error", msg); Ini_Dispose(iniText); exit(0);
	#define READ_STRING(s, k, var) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (var), sizeof((var))) <= 0) {STOP_CONFIGURATION((s), (k));} 
    #define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}

	if (!configPath) {
		MessagePopup("Configuration Error", "The path to the configuration file is not specified.");
		Ini_Dispose(iniText);
		exit(0);
	}
	
	if( Ini_ReadFromFile(iniText, configPath) < 0 ) {
		MessagePopup("Configuration Error", "Unable to read a configuration file.");
		Ini_Dispose(iniText);
		exit(0);
	} 

	////////////////////////////
	// FILE
	READ_STRING(FILE_SECTION, "logDir", CFG_LOG_DIRECTORY);
	READ_STRING(FILE_SECTION, "dataDir", CFG_DATA_DIRECTORY);
	READ_DOUBLE(FILE_SECTION, "dataWriteInterval", CFG_DATA_WRITE_INTERVAL);
	
	// SERVER
	READ_STRING(TCP_SECTION, "serverIp", CFG_SERVER_IP);
	READ_INT(TCP_SECTION, "serverPort", CFG_SERVER_PORT);
	READ_DOUBLE(TCP_SECTION, "dataRequestInterval", CFG_SERVER_DATA_REQUEST_RATE);
	READ_DOUBLE(TCP_SECTION, "connection", CFG_SERVER_CONNECTION_INTERVAL);
	READ_INT(TCP_SECTION, "canGwRequestInterval", CFG_CANGW_REQUEST_INTERVAL);
	
	// GENERAL
	READ_INT(GENERAL_SECTION, "devicesNumber", CFG_PSMCU_NUMBER);
	PSMCU_NUM = CFG_PSMCU_NUMBER;
	
	// INDICATION
	READ_DOUBLE(INDICATION_SECTION, "dac_dac_max_diff", CFG_DAC_DAC_MAX_DIFF);  
	READ_DOUBLE(INDICATION_SECTION, "dac_adc_max_diff", CFG_DAC_ADC_MAX_DIFF);  
	
	// PIPELINE
	READ_DOUBLE(PIPELINE_SECTION, "waitAfterReset", CFG_WAIT_AFTER_RESET); 
	READ_DOUBLE(PIPELINE_SECTION, "waitAfterForceOn", CFG_WAIT_AFTER_FORCE_ON);
	READ_DOUBLE(PIPELINE_SECTION, "waitAfterForceOff", CFG_WAIT_AFTER_FORCE_OFF);
	READ_DOUBLE(PIPELINE_SECTION, "waitAfterCurrentOn", CFG_WAIT_AFTER_CURRENT_ON);
	READ_DOUBLE(PIPELINE_SECTION, "waitAfterCurrentOff", CFG_WAIT_AFTER_CURRENT_OFF);
	
	READ_DOUBLE(PIPELINE_SECTION, "maxCurrentChangeRate", CFG_MAX_CURRENT_CHANGE_RATE); 
	READ_DOUBLE(PIPELINE_SECTION, "extraWaitTime", CFG_EXTRA_WAIT_TIME);  
	
	for (queueIndex=0; queueIndex < PSMCU_NUM; queueIndex++) {
		sprintf(key, "queue_%d", queueIndex);
		READ_INT(PIPELINE_SECTION, key, CFG_DEVICES_ORDER[queueIndex]);		
	}
	
	if (initDevicesOrder(CFG_DEVICES_ORDER, msg) < 0) {
		MessagePopup("Configuration Error", msg);
		Ini_Dispose(iniText);
		exit(0);	
	}
	
	// DATA_LOGGING
	READ_INT(DATA_LOGGING_SECTION, "logUserSpecifiedCurrent", CFG_DATA_LOG_CURR_USER);  
	READ_INT(DATA_LOGGING_SECTION, "logConfirmedCurrent", CFG_DATA_LOG_CURR_CONF); 
	READ_INT(DATA_LOGGING_SECTION, "logTransformerTemperature", CFG_DATA_LOG_TEMP); 
	READ_INT(DATA_LOGGING_SECTION, "logReferenceVoltage", CFG_DATA_LOG_REF_V); 
	READ_INT(DATA_LOGGING_SECTION, "logReserved", CFG_DATA_LOG_RESERVED); 
	
	////////////////////////////
	Ini_Dispose(iniText);
	return;
}
