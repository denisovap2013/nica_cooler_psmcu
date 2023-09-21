#include "inifile.h"
#include "ServerConfigData.h" 
#include "psMcuProtocol.h"  

//==============================================================================
// Server general parameters
char    CFG_SERVER_NAME[256];

int     CFG_PSMCU_DEVICES_NUM;
int     CFG_PSMCU_BLOCKS_IDS[CFG_MAX_PSMCU_DEVICES_NUM];  
int     CFG_PSMCU_DEVICES_IDS[CFG_MAX_PSMCU_DEVICES_NUM];
char    CFG_PSMCU_DEVICES_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][256];

// Defaults (static global variables) (maybe move to the function scope?)


// Specific values
char 	CFG_PSMCU_ADC_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_ADC_CHANNELS_NUM][256];
double 	CFG_PSMCU_ADC_COEFF[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_ADC_CHANNELS_NUM][2];  

char 	CFG_PSMCU_DAC_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_DAC_CHANNELS_NUM][256]; 
double 	CFG_PSMCU_DAC_COEFF[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_DAC_CHANNELS_NUM][2];

char 	CFG_PSMCU_INREG_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_INPUT_REGISTERS_NUM][256];
char 	CFG_PSMCU_OUTREG_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][PSMCU_OUTPUT_REGISTERS_NUM][256];

int 	CFG_PSMCU_DEVICE_DOWNTIME_LIMIT;
int		CFG_PSMCU_DEVICE_PING_INTERVAL;
int     CFG_PSMCU_REGISTERS_REQUEST_INTERVAL;
int     CFG_PSMCU_DAC_REQUEST_INTERVAL;

int     CFG_PSMCU_DAC_SLOW_TIME_DELTA;
double  CFG_PSMCU_DAC_SLOW_VOLTAGE_STEP;
	
// CANGW parameters
int     CFG_CANGW_BLOCKS_NUM;

char 	CFG_CANGW_BLOCK_NAME[CFG_CGW_MAX_NUM][256]; 
char 	CFG_CANGW_IP[CFG_CGW_MAX_NUM][256];
int		CFG_CANGW_PORT[CFG_CGW_MAX_NUM];
int		CFG_CANGW_BAUD[CFG_CGW_MAX_NUM];
int		CFG_CANGW_IGNORE_UNKNOWN[CFG_CGW_MAX_NUM];
int		CFG_CANGW_RECONNECTION_DELAY[CFG_CGW_MAX_NUM];
int		CFG_CANGW_RECV_TIMEOUT[CFG_CGW_MAX_NUM];
int		CFG_CANGW_SEND_TIMEOUT[CFG_CGW_MAX_NUM];

// TCP/IP parameters
int		CFG_TCP_PORT;
	
// LOG parameters
char	CFG_FILE_LOG_DIRECTORY[256];
char 	CFG_FILE_DATA_DIRECTORY[256];
int		CFG_FILE_DATA_WRITE_INTERVAL;  // seconds
int 	CFG_FILE_EXPIRATION;


#define GENERAL_SECTION "GENERAL" 
#define FILE_SECTION "FILE"
#define TCP_SECTION "TCP"
#define CANGW_SECTION "CANGW" 
#define PSMCU_SECTION "PSMCU" 
#define PSMCU_LIST_SECTION "PSMCU-LIST"  
#define PSMCU_DEFAULTS_SECTION "PSMCU_DEFAULT" 
#define PSMCU_SPECIFIC_SECTION "PSMCU_SPECIFIC"

void _getDeviceIndexErrMsg(int index, int itemsNumber, char *tagName, char *msg) {
	char indicesOrder[128];
	switch (itemsNumber) {
		case 1:	
			sprintf(indicesOrder, "");  
			break;
		case 2:	
			sprintf(indicesOrder, "Devices indices must be in order 0,1.\n");  
			break;
		case 3:	
			sprintf(indicesOrder, "Devices indices must be in order 0,1,2.\n");   
			break;
		default:	
			sprintf(indicesOrder, "Devices indices must be in order 0,1,...,%d.\n", itemsNumber - 1); 
			break;
	}
	sprintf(msg, "Incorrect device index.\n%sExpected '%d', got '%s'.", indicesOrder, index, tagName);  
}


int readDeviceAddrAndName(int devIndex, char *specsString, char *msg) {
	int strPos;

	if (sscanf(specsString, "%d %x %n", &CFG_PSMCU_BLOCKS_IDS[devIndex], &CFG_PSMCU_DEVICES_IDS[devIndex], &strPos) != 2 ) {
		sprintf(msg, "[%s] Cannot parse the address and name specifications for device with index %d", PSMCU_LIST_SECTION, devIndex, devIndex); 
		return -1; 
	}
	
	if (CFG_PSMCU_BLOCKS_IDS[devIndex] < 0 || CFG_PSMCU_BLOCKS_IDS[devIndex] >= CFG_CANGW_BLOCKS_NUM) {
		sprintf(msg, "[%s] Device #%d: Specified incorrect CanGw block index (%d). Must be from range [0, %d].", PSMCU_LIST_SECTION, devIndex, CFG_PSMCU_BLOCKS_IDS[devIndex], CFG_CANGW_BLOCKS_NUM-1);
		return -1;		
	}
	
	if (CFG_PSMCU_DEVICES_IDS[devIndex] <= 0 || CFG_PSMCU_DEVICES_IDS[devIndex] > 0xFF) {
		sprintf(msg, "[%s] Device #%d: Specified incorrect Device ID (hex) 0x%X. Must be from range [0x01, 0xFF].", PSMCU_LIST_SECTION, devIndex, CFG_PSMCU_DEVICES_IDS[devIndex]);
		return -1;
	}
	
	strcpy(CFG_PSMCU_DEVICES_NAMES[devIndex], &specsString[strPos]);
	if (strlen(CFG_PSMCU_DEVICES_NAMES[devIndex]) < 1) {
		sprintf(msg, "[%s] Device #%d: The device name cannot be an empty string.", PSMCU_LIST_SECTION, devIndex);
		return -1;
	}
	
	return 0;
}


void InitServerConfig(char * configPath) {
	int cgwIndex, devIndex, chIndex;
	
	char key[256], msg[256], subParsingStr[256], sectionName[256];
	IniText iniText;
	
	char *tagName;
	
	// Variables for defaults values
	char 	CFG_PSMCU_ADC_DEFAULT_NAMES[PSMCU_ADC_CHANNELS_NUM][256];
	double 	CFG_PSMCU_ADC_DEFAULT_COEFF[PSMCU_ADC_CHANNELS_NUM][2]; 
	char 	CFG_PSMCU_DAC_DEFAULT_NAMES[PSMCU_DAC_CHANNELS_NUM][256];
	double 	CFG_PSMCU_DAC_DEFAULT_COEFF[PSMCU_DAC_CHANNELS_NUM][2]; 
	char 	CFG_PSMCU_INREG_DEFAULT_NAMES[PSMCU_INPUT_REGISTERS_NUM][256];
	char 	CFG_PSMCU_OUTREG_DEFAULT_NAMES[PSMCU_OUTPUT_REGISTERS_NUM][256];
	
	#define INFORM_AND_STOP(msg) MessagePopup("Configuration Error", (msg)); Ini_Dispose(iniText); exit(0);   
	#define STOP_CONFIGURATION(s, k) sprintf(msg, "Cannot read '%s' from the '%s' section. (Line: %d)", (k), (s), Ini_LineOfLastAccess(iniText)); INFORM_AND_STOP(msg);
    #define READ_STRING(s, k, var) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (var), sizeof((var))) <= 0) {STOP_CONFIGURATION((s), (k));} 
    #define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_COEFFICIENTS(s, k, sbuf, c1, c2) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (sbuf), sizeof((sbuf))) <= 0) {STOP_CONFIGURATION((s), (k));} else { if (sscanf((sbuf), "%lf %lf", &(c1), &(c2)) < 2 ) {STOP_CONFIGURATION((s), (k));} }
	#define READ_COEFFICIENTS_OR_DEFAULT(s, k, sbuf, c1, c2, dc1, dc2) if (Ini_ItemExists(iniText, (s), (k))) { READ_COEFFICIENTS((s), (k), (sbuf), (c1), (c2)) } else { (c1) = (dc1); (c2) = (dc2); }  
	#define READ_STRING_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_STRING((s), (k), (var)) } else { strcpy((var), (default_val)); }
	
	iniText = Ini_New(0);
	if( Ini_ReadFromFile(iniText, configPath) < 0 ) {
		sprintf(msg, "Unable to read the configuration file '%s'.", configPath);
		INFORM_AND_STOP(msg);
	}

	////////////////////////////////////////////////////
	////////////////////////////////////////////////////
	// GENERAL //
	READ_STRING(GENERAL_SECTION, "serverName", CFG_SERVER_NAME); 

	////////////////////////////////////////////////////
	// FILE //
	READ_STRING(FILE_SECTION, "logDir", CFG_FILE_LOG_DIRECTORY);
	READ_STRING(FILE_SECTION, "dataDir", CFG_FILE_DATA_DIRECTORY);
	READ_INT(FILE_SECTION, "dataWriteInterval", CFG_FILE_DATA_WRITE_INTERVAL);
	READ_INT(FILE_SECTION, "oldFiles", CFG_FILE_EXPIRATION);
	
	////////////////////////////////////////////////////
	// TCP //
	READ_INT(TCP_SECTION, "tcpPort", CFG_TCP_PORT);

	////////////////////////////////////////////////////
	// CANGW //
	READ_INT(CANGW_SECTION, "cangwBlocksNum", CFG_CANGW_BLOCKS_NUM);  
	
	if (CFG_CANGW_BLOCKS_NUM <= 0 || CFG_CANGW_BLOCKS_NUM > CFG_CGW_MAX_NUM) {
		sprintf(msg, "Specified incorrect number of CanGw blocks (cangwBlocksNum=%d). Must be from range [1, %d].", CFG_CANGW_BLOCKS_NUM, CFG_CGW_MAX_NUM);
		INFORM_AND_STOP(msg);
	}
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		sprintf(sectionName, "CANGW-BLOCK-%d", cgwIndex);
		READ_STRING(sectionName, "cangwName", CFG_CANGW_BLOCK_NAME[cgwIndex]);   
		READ_STRING(sectionName, "cangwIp", CFG_CANGW_IP[cgwIndex]);   
		READ_INT(sectionName, "cangwPort", CFG_CANGW_PORT[cgwIndex]);
		READ_INT(sectionName, "cangwBaud", CFG_CANGW_BAUD[cgwIndex]);
		READ_INT(sectionName, "cangwIgnoreUnknown", CFG_CANGW_IGNORE_UNKNOWN[cgwIndex]); 
		READ_INT(sectionName, "connectionDelay", CFG_CANGW_RECONNECTION_DELAY[cgwIndex]);
		READ_INT(sectionName, "recvTimeout", CFG_CANGW_RECV_TIMEOUT[cgwIndex]);
		READ_INT(sectionName, "sendTimeout", CFG_CANGW_SEND_TIMEOUT[cgwIndex]);
	}

	////////////////////////////////////////////////////
	// PSMCU //
	CFG_PSMCU_DEVICES_NUM = Ini_NumberOfItems(iniText, PSMCU_LIST_SECTION);
	
	for (devIndex=0; devIndex < CFG_PSMCU_DEVICES_NUM; devIndex++) {
		sprintf(key, "%d", devIndex);  // expected tag name
		Ini_NthItemName(iniText, PSMCU_LIST_SECTION, devIndex + 1, &tagName);  // actual tag name
		
		if (strcmp(key, tagName) != 0) {
			_getDeviceIndexErrMsg(devIndex, CFG_PSMCU_DEVICES_NUM, tagName, msg);
			INFORM_AND_STOP(msg);		
		}

		READ_STRING(PSMCU_LIST_SECTION, key, subParsingStr);
		if (readDeviceAddrAndName(devIndex, subParsingStr, msg) < 0) {INFORM_AND_STOP(msg);}
	}
	
	READ_INT(PSMCU_SECTION, "DowntimeLimit", CFG_PSMCU_DEVICE_DOWNTIME_LIMIT);  // Device response timeout 
	READ_INT(PSMCU_SECTION, "FE_interval", CFG_PSMCU_DEVICE_PING_INTERVAL); // Device ping interval   
	READ_INT(PSMCU_SECTION, "RegistersRequestInterval", CFG_PSMCU_REGISTERS_REQUEST_INTERVAL); // Registers request interval
	READ_INT(PSMCU_SECTION, "DacRequestInterval", CFG_PSMCU_DAC_REQUEST_INTERVAL); // DACs request interval 
	
	////////////////////////////////////////////////////
	// PSMCU_DEFAULT //

	// default names
	for (chIndex = 0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) { 
		sprintf(key, "default_adc_ch_%d_name", chIndex);
		READ_STRING(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_ADC_DEFAULT_NAMES[chIndex]); 
	}
	for (chIndex = 0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) { 
		sprintf(key, "default_dac_ch_%d_name", chIndex);
		READ_STRING(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_DAC_DEFAULT_NAMES[chIndex]); 	
	}
	for (chIndex = 0; chIndex < PSMCU_INPUT_REGISTERS_NUM; chIndex++) { 
		sprintf(key, "default_in_reg_ch_%d_name", chIndex);
		READ_STRING(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_INREG_DEFAULT_NAMES[chIndex]); 
	}
	for (chIndex = 0; chIndex < PSMCU_OUTPUT_REGISTERS_NUM; chIndex++) { 
		sprintf(key, "default_out_reg_ch_%d_name", chIndex);
		READ_STRING(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_OUTREG_DEFAULT_NAMES[chIndex]); 
	}
	
	// default coefficients
	for (chIndex = 0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) { 
		sprintf(key, "default_adc_ch_%d_coeff", chIndex);
		READ_COEFFICIENTS(PSMCU_DEFAULTS_SECTION, key, subParsingStr, CFG_PSMCU_ADC_DEFAULT_COEFF[chIndex][0], CFG_PSMCU_ADC_DEFAULT_COEFF[chIndex][1]);
	}
	for (chIndex = 0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) { 
		sprintf(key, "default_dac_ch_%d_coeff", chIndex);
		READ_COEFFICIENTS(PSMCU_DEFAULTS_SECTION, key, subParsingStr, CFG_PSMCU_DAC_DEFAULT_COEFF[chIndex][0], CFG_PSMCU_DAC_DEFAULT_COEFF[chIndex][1]);  	
	}
	
	// default DAC slow mode parameters
	READ_INT(PSMCU_DEFAULTS_SECTION, "default_dac_slow_time_delta", CFG_PSMCU_DAC_SLOW_TIME_DELTA);
	READ_DOUBLE(PSMCU_DEFAULTS_SECTION, "default_dac_slow_max_voltage_step", CFG_PSMCU_DAC_SLOW_VOLTAGE_STEP);
	
	////////////////////////////////////////////////////
	// PSMCU_SPECIFIC //
	
    // Set specific names for channels of each device (if defined in the configuration file)
	
	for (devIndex = 0; devIndex < CFG_PSMCU_DEVICES_NUM; devIndex++) {
		// ADC
		for (chIndex = 0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) { 
			sprintf(key, "dev_%d_adc_ch_%d_name", devIndex, chIndex);
			READ_STRING_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, CFG_PSMCU_ADC_NAMES[devIndex][chIndex], CFG_PSMCU_ADC_DEFAULT_NAMES[chIndex]);
			
			sprintf(key, "dev_%d_adc_ch_%d_coeff", devIndex, chIndex);
			READ_COEFFICIENTS_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, subParsingStr, CFG_PSMCU_ADC_COEFF[devIndex][chIndex][0], CFG_PSMCU_ADC_COEFF[devIndex][chIndex][1], CFG_PSMCU_ADC_DEFAULT_COEFF[chIndex][0], CFG_PSMCU_ADC_DEFAULT_COEFF[chIndex][1]);
		}	
		
		// DAC
		for (chIndex = 0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) { 
			sprintf(key, "dev_%d_dac_ch_%d_name", devIndex, chIndex);
			READ_STRING_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, CFG_PSMCU_DAC_NAMES[devIndex][chIndex], CFG_PSMCU_DAC_DEFAULT_NAMES[chIndex]);
			
			sprintf(key, "dev_%d_dac_ch_%d_coeff", devIndex, chIndex);
			READ_COEFFICIENTS_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, subParsingStr, CFG_PSMCU_DAC_COEFF[devIndex][chIndex][0], CFG_PSMCU_DAC_COEFF[devIndex][chIndex][1], CFG_PSMCU_DAC_DEFAULT_COEFF[chIndex][0], CFG_PSMCU_DAC_DEFAULT_COEFF[chIndex][1]);
		}
		
		// Input Registers
		for (chIndex = 0; chIndex < PSMCU_INPUT_REGISTERS_NUM; chIndex++) { 
			sprintf(key, "dev_%d_in_reg_ch_%d_name", devIndex, chIndex);
			READ_STRING_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, CFG_PSMCU_INREG_NAMES[devIndex][chIndex], CFG_PSMCU_INREG_DEFAULT_NAMES[chIndex]);
		}
		
		// Output Registers
		for (chIndex = 0; chIndex < PSMCU_OUTPUT_REGISTERS_NUM; chIndex++) { 
			sprintf(key, "dev_%d_out_reg_ch_%d_name", devIndex, chIndex);
			READ_STRING_OR_DEFAULT(PSMCU_SPECIFIC_SECTION, key, CFG_PSMCU_OUTREG_NAMES[devIndex][chIndex], CFG_PSMCU_OUTREG_DEFAULT_NAMES[chIndex]);
		}
	}
	
	////////////////////////////////////////////////////////
	Ini_Dispose(iniText);
}
