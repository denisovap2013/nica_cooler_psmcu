#include "inifile.h"
#include "ServerConfigData.h" 
#include "psMcuProtocol.h"
#include "hash_map.h"

//==============================================================================
// Server general parameters
char    CFG_SERVER_NAME[256];

int     CFG_PSMCU_DEVICES_NUM;
int     CFG_PSMCU_BLOCKS_IDS[CFG_MAX_PSMCU_DEVICES_NUM];  
int     CFG_PSMCU_DEVICES_IDS[CFG_MAX_PSMCU_DEVICES_NUM];
char    CFG_PSMCU_DEVICES_NAMES[CFG_MAX_PSMCU_DEVICES_NUM][256];
char    CFG_PSMCU_DEVICES_TYPES[CFG_MAX_PSMCU_DEVICES_NUM][CFG_MAX_TYPE_LEN]; 

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

// Slow mode parameters
int     CFG_PSMCU_DAC_SLOW_TIME_DELTA;
double  CFG_PSMCU_DAC_SLOW_VOLTAGE_STEP;
	
// Safe model parameters
double  CFG_PSMCU_ADC_MAX_CURRENT[CFG_MAX_PSMCU_DEVICES_NUM];
double  CFG_PSMCU_ADC_SAFE_CURR_THRESH[CFG_MAX_PSMCU_DEVICES_NUM];

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


// Device type settings
int deviceTypeSettingsInitialized = 0;
typedef map_t(deviceTypeSettings) map_settings;
map_settings deviceTypeSettingsMap;
deviceTypeSettings * defaultDeviceTypeSettings = 0;

// End of Device type settings


#define GENERAL_SECTION "GENERAL" 
#define FILE_SECTION "FILE"
#define TCP_SECTION "TCP"
#define CANGW_SECTION "CANGW" 
#define PSMCU_SECTION "PSMCU" 
#define PSMCU_LIST_SECTION "PSMCU-LIST"  
#define PSMCU_DEFAULTS_SECTION "PSMCU_DEFAULT" 
#define PSMCU_SPECIFIC_SECTION "PSMCU_SPECIFIC"
#define PSMCU_TYPE_SETTINGS_DEFAULT "PSMCU-TYPE-SETTINGS-default"
#define PSMCU_TYPES "PSMCU-TYPES"


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


void removeNameTrailingSpaces(char *name) {
    char *end = name + strlen(name);
	
	while((end > name) && ((*(end-1) == ' ') || (*(end-1) == '\t'))) {
		end--;
		*end = 0;
	}
}

int readDeviceInfo(int devIndex, char *specsString, char *msg) {
	int strPos;
	char * name_start;
	char * type_separator;
	char * type_start;
	char type_name[256];

	if (sscanf(specsString, "%d %x %n", &CFG_PSMCU_BLOCKS_IDS[devIndex], &CFG_PSMCU_DEVICES_IDS[devIndex], &strPos) != 2 ) {
		sprintf(
			msg,
			"[%s] Cannot parse the address and name specifications for device with index %d.\nDevice specification: \"%s\"",
			PSMCU_LIST_SECTION, devIndex, devIndex, specsString
		); 
		return -1; 
	}
	
	if (CFG_PSMCU_BLOCKS_IDS[devIndex] < 0 || CFG_PSMCU_BLOCKS_IDS[devIndex] >= CFG_CANGW_BLOCKS_NUM) {
		sprintf(
			msg,
			"[%s] Device #%d: Specified incorrect CanGw block index (%d). Must be from range [0, %d].\nDevice specification: \"%s\"",
			PSMCU_LIST_SECTION, devIndex, CFG_PSMCU_BLOCKS_IDS[devIndex], CFG_CANGW_BLOCKS_NUM-1, specsString
		);
		return -1;		
	}
	
	if (CFG_PSMCU_DEVICES_IDS[devIndex] <= 0 || CFG_PSMCU_DEVICES_IDS[devIndex] > 0xFF) {
		sprintf(
			msg,
			"[%s] Device #%d: Specified incorrect Device ID (hex) 0x%X. Must be from range [0x01, 0xFF].\nDevice specification: \"%s\"",
			PSMCU_LIST_SECTION, devIndex, CFG_PSMCU_DEVICES_IDS[devIndex], specsString
		);
		return -1;
	}
	
    // Skipping all the white spaces
	name_start = &specsString[strPos];
	while ((*name_start==' ') || (*name_start=='\t')) name_start++;
	
	// trying to find a separator for the device type specification
	type_separator = strstr(name_start, ">>");
	if (type_separator) {
		// Copy device name
		memcpy(CFG_PSMCU_DEVICES_NAMES[devIndex], name_start, type_separator - name_start);
		CFG_PSMCU_DEVICES_NAMES[devIndex][type_separator - name_start] = 0;  // Put a zero-termination character.
		
		// Extract device type
		type_start = type_separator + 2;
		// Remove leading spaces
		while ((*type_start==' ') || (*type_start=='\t')) type_start++;
		
		strcpy(type_name, type_start);
		removeNameTrailingSpaces(type_name);
		
		if (strlen(type_name) == 0) {
			sprintf(
				msg,
				"[%s] Device #%d: Specified empty device type.\nDevice specification: \"%s\"",
				PSMCU_LIST_SECTION, devIndex, specsString
			);
			return -1;	
		}
	
		// Check that device type is registerd in the configuration file.
		
		if (!cfgDeviceTypeSettingsGet(type_name)) {
			sprintf(
				msg,
				"[%s] Device #%d: Unexpected device type \"%s\". Check that this device type is specified in the section \"%s\"\nDevice specification: \"%s\"",
				PSMCU_LIST_SECTION, devIndex, type_name, PSMCU_TYPES, specsString
			);
			return -1;		
		}
		
		// Copy the device type name to the configuration
	    strcpy(CFG_PSMCU_DEVICES_TYPES[devIndex], type_name);	
		
	} else {
		strcpy(CFG_PSMCU_DEVICES_NAMES[devIndex], name_start);
		
		// Set the empty string to the device type, indicating that the default device tpye setting should be used.
		strcpy(CFG_PSMCU_DEVICES_TYPES[devIndex], "");
	}

	removeNameTrailingSpaces(CFG_PSMCU_DEVICES_NAMES[devIndex]);
	if (strlen(CFG_PSMCU_DEVICES_NAMES[devIndex]) < 1) {
		sprintf(
			msg,
			"[%s] Device #%d: The device name cannot be an empty string.\nDevice specification: \"%s\"",
			PSMCU_LIST_SECTION, devIndex, specsString
		);
		return -1;
	}
	
	return 0;
}


void InitServerConfig(char * configPath) {
	int cgwIndex, devIndex, chIndex;
	
	char key[256], msg[256], subParsingStr[1024], sectionName[256];
	char * token;
	IniText iniText;
	deviceTypeSettings settingsOverride;
	
	char *tagName;
	
	// Variables for defaults values
	int		CFG_CANGW_DEFAULT_BAUD;
	int		CFG_CANGW_DEFAULT_IGNORE_UNKNOWN;   
	int		CFG_CANGW_DEFAULT_RECONNECTION_DELAY;
	int		CFG_CANGW_DEFALT_RECV_TIMEOUT;
	int		CFG_CANGW_DEFAULT_SEND_TIMEOUT;

	char 	CFG_PSMCU_ADC_DEFAULT_NAMES[PSMCU_ADC_CHANNELS_NUM][256];
	double 	CFG_PSMCU_ADC_DEFAULT_COEFF[PSMCU_ADC_CHANNELS_NUM][2]; 
	char 	CFG_PSMCU_DAC_DEFAULT_NAMES[PSMCU_DAC_CHANNELS_NUM][256];
	double 	CFG_PSMCU_DAC_DEFAULT_COEFF[PSMCU_DAC_CHANNELS_NUM][2]; 
	char 	CFG_PSMCU_INREG_DEFAULT_NAMES[PSMCU_INPUT_REGISTERS_NUM][256];
	char 	CFG_PSMCU_OUTREG_DEFAULT_NAMES[PSMCU_OUTPUT_REGISTERS_NUM][256];
	double  cfg_default_adc_max_curr;
	double  cfg_default_adc_safe_curr;
	
	#define INFORM_AND_STOP(msg) MessagePopup("Configuration Error", (msg)); Ini_Dispose(iniText); cfgReleaseDeviceTypeSettings(); exit(0);   
	#define STOP_CONFIGURATION(s, k) sprintf(msg, "Cannot read '%s' from the '%s' section. (Line: %d)", (k), (s), Ini_LineOfLastAccess(iniText)); INFORM_AND_STOP(msg);
    #define READ_STRING(s, k, var) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (var), sizeof((var))) <= 0) {STOP_CONFIGURATION((s), (k));} 
    #define READ_INT(s, k, var) if(Ini_GetInt(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE(s, k, var) if(Ini_GetDouble(iniText, (s), (k), &(var)) <= 0) {STOP_CONFIGURATION((s), (k));}
	#define READ_DOUBLE_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_DOUBLE((s), (k), (var)) } else { (var) = (default_val); }
	#define READ_COEFFICIENTS(s, k, sbuf, c1, c2) if(Ini_GetStringIntoBuffer(iniText, (s), (k), (sbuf), sizeof((sbuf))) <= 0) {STOP_CONFIGURATION((s), (k));} else { if (sscanf((sbuf), "%lf %lf", &(c1), &(c2)) < 2 ) {STOP_CONFIGURATION((s), (k));} }
	#define READ_COEFFICIENTS_OR_DEFAULT(s, k, sbuf, c1, c2, dc1, dc2) if (Ini_ItemExists(iniText, (s), (k))) { READ_COEFFICIENTS((s), (k), (sbuf), (c1), (c2)) } else { (c1) = (dc1); (c2) = (dc2); }  
	#define READ_INT_OR_DEFAULT(s, k, var, default_val) if (Ini_ItemExists(iniText, (s), (k))) { READ_INT((s), (k), (var)) } else { (var) = (default_val); }
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
	
	// Read default parameter for CanGw connections
	READ_INT(CANGW_SECTION, "cangwBaud", CFG_CANGW_DEFAULT_BAUD);
	READ_INT(CANGW_SECTION, "cangwIgnoreUnknown", CFG_CANGW_DEFAULT_IGNORE_UNKNOWN); 
	READ_INT(CANGW_SECTION, "connectionDelay", CFG_CANGW_DEFAULT_RECONNECTION_DELAY);
	READ_INT(CANGW_SECTION, "recvTimeout", CFG_CANGW_DEFALT_RECV_TIMEOUT);
	READ_INT(CANGW_SECTION, "sendTimeout", CFG_CANGW_DEFAULT_SEND_TIMEOUT);
	
	for (cgwIndex=0; cgwIndex < CFG_CANGW_BLOCKS_NUM; cgwIndex++) {
		sprintf(sectionName, "CANGW-BLOCK-%d", cgwIndex);
		READ_STRING(sectionName, "cangwName", CFG_CANGW_BLOCK_NAME[cgwIndex]);   
		READ_STRING(sectionName, "cangwIp", CFG_CANGW_IP[cgwIndex]);   
		READ_INT(sectionName, "cangwPort", CFG_CANGW_PORT[cgwIndex]);
		READ_INT_OR_DEFAULT(sectionName, "cangwBaud", CFG_CANGW_BAUD[cgwIndex], CFG_CANGW_DEFAULT_BAUD);
		READ_INT_OR_DEFAULT(sectionName, "cangwIgnoreUnknown", CFG_CANGW_IGNORE_UNKNOWN[cgwIndex], CFG_CANGW_DEFAULT_IGNORE_UNKNOWN); 
		READ_INT_OR_DEFAULT(sectionName, "connectionDelay", CFG_CANGW_RECONNECTION_DELAY[cgwIndex], CFG_CANGW_DEFAULT_RECONNECTION_DELAY);
		READ_INT_OR_DEFAULT(sectionName, "recvTimeout", CFG_CANGW_RECV_TIMEOUT[cgwIndex], CFG_CANGW_DEFALT_RECV_TIMEOUT);
		READ_INT_OR_DEFAULT(sectionName, "sendTimeout", CFG_CANGW_SEND_TIMEOUT[cgwIndex], CFG_CANGW_DEFAULT_SEND_TIMEOUT);
	}

	////////////////////////////////////////////////////
	// PSMCU-TYPE-SETTINGS-default //
	cfgInitDeviceTypeSettings();
	READ_DOUBLE(PSMCU_TYPE_SETTINGS_DEFAULT, "contactor_delay", defaultDeviceTypeSettings->contactor_delay);
	
	// PSMCU-TYPES
	// Read available types;
	READ_STRING(PSMCU_TYPES, "types", subParsingStr);
	
	// Iterate over available types and check the corresponding sections for settings override.
	for (token = strtok(subParsingStr," ,"); token != NULL; token = strtok(NULL, " ,")) {
		// Check the length of the type
		if (strlen(token) + 1 >= CFG_MAX_TYPE_LEN) {
			sprintf(msg, "Max length of the device type name must be %d, got %d symbols for a device type \"%s\"", CFG_MAX_TYPE_LEN-1, strlen(token), token);
			INFORM_AND_STOP(msg);
		}
		// Copy default settings
		memcpy(&settingsOverride, defaultDeviceTypeSettings, sizeof(deviceTypeSettings));
		sprintf(sectionName, "PSMCU-TYPE-SETTINGS-%s", token);
		READ_DOUBLE_OR_DEFAULT(sectionName, "contactor_delay", settingsOverride.contactor_delay, defaultDeviceTypeSettings->contactor_delay);
		map_set(&deviceTypeSettingsMap, token, settingsOverride);  
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
		if (readDeviceInfo(devIndex, subParsingStr, msg) < 0) {INFORM_AND_STOP(msg);}
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
	
	// default safe current threshold (if measured currents exceed that threshold, the server will automatically reset the device currents to zero)
	READ_DOUBLE(PSMCU_DEFAULTS_SECTION, "default_adc_safe_current_threshold", cfg_default_adc_safe_curr);
	READ_DOUBLE(PSMCU_DEFAULTS_SECTION, "default_adc_max_allowed_current", cfg_default_adc_max_curr);

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

		// Safe mode parameters
		sprintf(key, "dev_%d_adc_safe_current_threshold", devIndex);
		READ_DOUBLE_OR_DEFAULT(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_ADC_SAFE_CURR_THRESH[devIndex], cfg_default_adc_safe_curr);

		sprintf(key, "dev_%d_adc_max_allowed_current", devIndex);
		READ_DOUBLE_OR_DEFAULT(PSMCU_DEFAULTS_SECTION, key, CFG_PSMCU_ADC_MAX_CURRENT[devIndex], cfg_default_adc_max_curr);
	}
	
	////////////////////////////////////////////////////////
	Ini_Dispose(iniText);
}


void cfgInitDeviceTypeSettings(void) {
    if (deviceTypeSettingsInitialized) return;

	map_init(&deviceTypeSettingsMap);
	defaultDeviceTypeSettings = malloc(sizeof(deviceTypeSettings));
	deviceTypeSettingsInitialized = 1;
}


void cfgReleaseDeviceTypeSettings(void) {
    if (!deviceTypeSettingsInitialized) return;
	
	map_deinit(&deviceTypeSettingsMap);
	free(defaultDeviceTypeSettings);
	defaultDeviceTypeSettings = 0;
	deviceTypeSettingsInitialized = 0;
}


deviceTypeSettings * cfgDeviceTypeSettingsGetDefault(void) {
    return defaultDeviceTypeSettings;	
}


deviceTypeSettings * cfgDeviceTypeSettingsGet(char * type) {
	void * settings;
    if (!deviceTypeSettingsInitialized) return 0;
	
	settings = map_get(&deviceTypeSettingsMap, type);
	if (!settings) return defaultDeviceTypeSettings;
	return settings;
}


// Helper functions for printing settings for the device types
void printSettings(const char *name, deviceTypeSettings* settings) {
    printf("\"%s\": %lf\n", name, settings->contactor_delay); 	
}

void printDeviceTypeSettings(void) {
	map_iter_t iterator;
	if (!deviceTypeSettingsInitialized) {
		printf("Unable to print device type settings. Not initialized.\n");
		return;
	}
	
	printf("Device type settings:\n");
	printSettings("default", cfgDeviceTypeSettingsGetDefault());

	iterator = map_iter(&deviceTypeSettingsMap);
	while (map_next(&deviceTypeSettingsMap, &iterator)) {
		printSettings(map_get_node_key(iterator.node), (deviceTypeSettings*)map_get_node_value(iterator.node));
	}
}
