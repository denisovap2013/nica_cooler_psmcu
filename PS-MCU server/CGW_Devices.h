//==============================================================================
//
//
//==============================================================================

#ifndef __CGW_Devices_H__
#define __CGW_Devices_H__

#include "cangw.h"

#define CGW_DEVICES_NAME_MAX_LENGTH 20
#define CGW_MAX_ERROR_MSG_LEN 512

extern int cgwDevices_NumberOfPrototypes;
typedef void (*message_hook_func_t)(cangw_msg_t msg, void * deviceData);

struct cgwDevices_Prototype
{
	char name[CGW_DEVICES_NAME_MAX_LENGTH];
	unsigned char DeviceCode;
};

extern struct cgwDevices_Prototype cgwDevices_KnownPrototypes[256];
extern char cgwDevices_UnknownDevice[10]; 


typedef struct cgw_devices
{
	int active[64];			// shows if device is on the CAN bus line
	int registered[64];		// indicates that device interface for a certain device number was registered
	
	int registeredNum;
	int registeredIDs[64];
	int idsToIndicesMap[64];

	void * parameters[64];										// parameters of the device
	char names[64][CGW_DEVICES_NAME_MAX_LENGTH];		// Names of the devices (for tracing errors and events)
	message_hook_func_t message_hook_default[64];		/* function that defines the specific behavior of the device on incoming messages */
	message_hook_func_t message_hook_user[64];		/* function that defines the user-specified behavior of the device on incoming messages */
	// void (*configFunc[64]) (unsigned char num);
	
	// Control devices downtime
	unsigned long devicesDowntime[64];
	long devicesDowntimeLimits[64];  // negative numbers mean the corresponding device does not have a downtime limit.
	void (*deviceAwakeCallbacks[64])(void * parameters);
	void (*deviceLostCallbacks[64])(void * parameters);
	
	// Control device update mechanism
	void (*deviceUpdateCallback[64])(void * parameters);
	
	// Flag for ignoring unknown devices on the line
	int ignore_unregistered;
	char blockName[256];

	// Error states
	int error_state[64];
	char last_error_msg[64][CGW_MAX_ERROR_MSG_LEN];
} cgw_devices_t;

// Initialization of devices kit
int cgwDevices_InitDevicesKit(cgw_devices_t *devKit, char *blockName);
int cgwDevices_IgnoreUnregistered(cgw_devices_t * devKit, int ignore);

int cgwRegisterDevice(
		cgw_devices_t *devKit,
		int newDeviceID,
		void *deviceParameters,
		char *newDeviceName,
		message_hook_func_t default_message_hook,
		message_hook_func_t message_hook_user,
		long newDeviceDowntimeLimit,
		void (*deviceAwakeCallback)(void * parameters),
		void (*deviceLostCallback)(void * parameters),
		void (*deviceUpdateCallback)(void * parameters)
);

void cgwDevices_InitPrototypes(void);
char* cgwDevices_GetNameByDeviceCode(unsigned devCode);
// Releasing devices  
int cgwDevices_ReleaseAll(cgw_devices_t * devKit);

void markDeviceAwake(cgw_devices_t *devKit, int deviceID);
void markDeviceLost(cgw_devices_t *devKit, int deviceID);
void checkIfDeviceLost(cgw_devices_t *devKit, int deviceID);
void resetDeviceDowntime(cgw_devices_t *devKit, int deviceID); 
void updateDevicesDownTime(cgw_devices_t *devKit);
void updateDevices(cgw_devices_t *devKit); 
void markAllDevicesInactive(cgw_devices_t *devKit);

// Message processing
void cgwDevices_MessageHook(cgw_devices_t * devKit, cangw_msg_t msg);
unsigned char cgwDevicess_DeviceIDFromMessageID(unsigned long ID);

// Requests

int cgwDevicess_IsActive(cgw_devices_t * devKit, unsigned int deviceID);
int cgwDevicess_IsRegistered(cgw_devices_t * devKit, unsigned int deviceID);

#endif  /* ndef __CGW_Devices_H__ */
