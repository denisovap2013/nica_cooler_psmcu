//==============================================================================
//
//
//==============================================================================

//==============================================================================
// Include files

#include "CGW_Devices.h"
#include "MessageStack.h" 
#include "TimeMarkers.h"

int cgwDevices_NumberOfPrototypes = 0;
struct cgwDevices_Prototype cgwDevices_KnownPrototypes[256]; 
char cgwDevices_UnknownDevice[10]; 

// Initialization of devices kit
int cgwDevices_InitDevicesKit(cgw_devices_t *devKit, char *blockName)
{
	if (!devKit)
	{
		msAddMsg(msGMS(),"/-/ cgwDevices_InitDevicesKit /-/: Error! Devices Kit is not indicated.");
		return -1;
	}
	
	memset(devKit, 0, sizeof(cgw_devices_t));

	if (blockName) 
		strcpy(devKit->blockName, blockName);
	else
		strcpy(devKit->blockName, "Unnamed"); 
	
	return 0;
}

int cgwDevices_IgnoreUnregistered(cgw_devices_t * devKit, int ignore)
{
	if (!devKit)
	{
		msAddMsg(msGMS(),"/-/ cgwDevices_IgnoreUnregistered /-/: Error! Devices Kit is not indicated.");
		return -1;
	}
	
	devKit->ignore_unregistered = ignore;
	
	return 0;		
}

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
) {
	if (!devKit)
	{
		msAddMsg(msGMS(), "/-/ cgwRegisterDevice /-/: Error! The pointer on the devices kit is not indicated.");
		return -1;
	}
	if (newDeviceID > 63 || newDeviceID < 1)
	{
		msAddMsg(msGMS(), "/-/ cgwRegisterDevice /-/: Error! The device ID %d (0x%02X) is out of range [1,..,63].", newDeviceID, newDeviceID);
		return -1;
	}
	if (devKit->registered[newDeviceID])
	{
		msAddMsg(msGMS(), "/-/ cgwRegisterDevice /-/: Error! The indicated device ID %d (0x%02X) is already in use.", newDeviceID, newDeviceID);
		return -1;
	}
	
	if (!deviceParameters)
	{
		msAddMsg(msGMS(),"/-/ cgwRegisterDevice /-/: Error! The pointer to the device parameters structure is NULL.");
		return -1;	
	}
	
	devKit->parameters[newDeviceID] = deviceParameters;
	strcpy(devKit->names[newDeviceID], newDeviceName);
	
	devKit->message_hook_default[newDeviceID] = default_message_hook;
	devKit->message_hook_user[newDeviceID] = message_hook_user;
	
	devKit->devicesDowntimeLimits[newDeviceID] = newDeviceDowntimeLimit;
	devKit->deviceAwakeCallbacks[newDeviceID] = deviceAwakeCallback;
	devKit->deviceLostCallbacks[newDeviceID] = deviceLostCallback;
	
	devKit->deviceUpdateCallback[newDeviceID] = deviceUpdateCallback; 
	
	devKit->idsToIndicesMap[newDeviceID] = devKit->registeredNum;
	devKit->registeredIDs[devKit->registeredNum] = newDeviceID;
	devKit->registeredNum++;
	
	devKit->registered[newDeviceID] = 1; 
	
	// Returns local index for this devKit
	return devKit->registeredNum - 1;
}


void markDeviceAwake(cgw_devices_t *devKit, int deviceID) {
	if (!devKit->registered[deviceID]) return;
	
	if (!devKit->active[deviceID]) {
		devKit->active[deviceID] = 1;

		msAddMsg(msGMS(), "%s [PSMCU] [%s] Device ONLINE \"%s\" [%d (0x%X)].", 
			TimeStamp(0), devKit->blockName, devKit->names[deviceID], deviceID, deviceID);
		
		if (devKit->deviceAwakeCallbacks[deviceID]) {
			devKit->deviceAwakeCallbacks[deviceID](devKit->parameters[deviceID]);
		}
	}
}


void markDeviceLost(cgw_devices_t *devKit, int deviceID) {
	if (!devKit->registered[deviceID]) return; 
	
	if (devKit->active[deviceID]) {
		devKit->active[deviceID] = 0;

		msAddMsg(msGMS(), "%s [PSMCU] [%s] Error! Device TIMEOUT \"%s\" [%d (0x%X)].", 
			TimeStamp(0), devKit->blockName, devKit->names[deviceID], deviceID, deviceID);
		
		if (devKit->deviceLostCallbacks[deviceID]) {
			devKit->deviceLostCallbacks[deviceID](devKit->parameters[deviceID]);
		}
	}
}


void checkIfDeviceLost(cgw_devices_t *devKit, int deviceID) {
	if (!devKit->registered[deviceID]) return;
	
	if (devKit->devicesDowntimeLimits[deviceID] < 0) return;
	
	if (devKit->devicesDowntime[deviceID] > devKit->devicesDowntimeLimits[deviceID]) {
		markDeviceLost(devKit, deviceID);		
	}
}


void resetDeviceDowntime(cgw_devices_t *devKit, int deviceID) {
	devKit->devicesDowntime[deviceID] = 0;
}


void updateDevicesDownTime(cgw_devices_t *devKit) {
	static time_t prev_time = 0;
	static time_t current_time;
	int i, deviceID;
	unsigned int delta_time;
	
	if (prev_time == 0) time(&prev_time);
	
	time(&current_time);
	
	delta_time = current_time - prev_time;
	prev_time = current_time;
	
	for (i=0; i < devKit->registeredNum; i++) {
		deviceID = devKit->registeredIDs[i];
		
		devKit->devicesDowntime[deviceID] += delta_time;
		checkIfDeviceLost(devKit, deviceID);
	}
}


void updateDevices(cgw_devices_t *devKit) {
	int i, deviceID;
	
	for (i=0; i < devKit->registeredNum; i++) {
		deviceID = devKit->registeredIDs[i];
		
		if (devKit->deviceUpdateCallback[deviceID]) {
			devKit->deviceUpdateCallback[deviceID](devKit->parameters[deviceID]);	
		}
	}
}


void markAllDevicesInactive(cgw_devices_t *devKit) {
	memset(devKit->active, 0, sizeof(devKit->active));	
}


void cgwDevices_InitPrototypes(void)
{
	// This function helfs to identify devices on the line. For examle if someone added something,
	// and the server helps you to identify what exactly it is.

	int protoNum = 0;
	strcpy(cgwDevices_UnknownDevice, "Unknown");
	// CDAC20
		cgwDevices_KnownPrototypes[protoNum].DeviceCode = 3;
		strcpy(cgwDevices_KnownPrototypes[protoNum].name, "CDAC20");
		protoNum++;
	// CEAD20
		cgwDevices_KnownPrototypes[protoNum].DeviceCode = 23;
		strcpy(cgwDevices_KnownPrototypes[protoNum].name, "CEAD20");
		protoNum++;	
	// CEDIO_A
		cgwDevices_KnownPrototypes[protoNum].DeviceCode = 28;
		strcpy(cgwDevices_KnownPrototypes[protoNum].name, "CEDIO_A");
		protoNum++;
	// 
	cgwDevices_NumberOfPrototypes = protoNum;
}


char * cgwDevices_GetNameByDeviceCode(unsigned devCode)
{
	int i;
	for(i = 0; i<cgwDevices_NumberOfPrototypes; i++)
	{
		if (cgwDevices_KnownPrototypes[i].DeviceCode == devCode)
		{
			return cgwDevices_KnownPrototypes[i].name;
		}
	}
	return cgwDevices_UnknownDevice;
}


// Message decomposition
unsigned char cgwDevicess_DeviceIDFromMessageID(unsigned long ID)
{
	if (ID > 0x7FF)
		msAddMsg(msGMS(),"/-/ cgwDevicess_DeviceIDFromMessageID /-/: Warning! ID contains non zero additional bits (ID11,ID12..). They will be ignored.");
	return (0x3F & (ID >> 2));	
}


// Releasing devices
int cgwDevices_ReleaseAll(cgw_devices_t * devKit)
{
	int deviceID;
	for (deviceID=0; deviceID<64; deviceID++) {
		if(devKit->parameters[deviceID]) 
		{
			free(devKit->parameters[deviceID]);
			devKit->parameters[deviceID] = 0;
		}		
	}
	return 0;
}


// Message processing
void cgwDevices_MessageHook(cgw_devices_t * devKit, cangw_msg_t msg)
{
	static int deviceID;
	deviceID = cgwDevicess_DeviceIDFromMessageID(msg.id);
	
	if (devKit->registered[deviceID])
	{
		if (msg.data[0] == 0xFF)	// Restoring initial configurations of the device
		{
			switch(msg.data[4])
			{
				case 0:
					msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" - Power On", 
						TimeStamp(0), devKit->blockName, deviceID, deviceID, devKit->names[deviceID]);
					break;
				case 1:
					msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" - \"Reset\" button has been pressed.", 
						TimeStamp(0), devKit->blockName, deviceID, deviceID, devKit->names[deviceID]); 
					break;
				case 2:
					msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" - Online (FF request answered).", 
						TimeStamp(0), devKit->blockName, deviceID, deviceID, devKit->names[deviceID]); 
					break;
			}
		}

		resetDeviceDowntime(devKit, deviceID);
		markDeviceAwake(devKit, deviceID);  
		
		devKit->message_hook_default[deviceID](msg, devKit->parameters[deviceID]); 
		/*if (msg.data[0] == 0xF8) {
			msAddMsg(msGMS(),"%s [DEBUG] Device: %02X; Output Registers: %04X; Input registers: %04X.", TimeStamp(0), deviceID, msg.data[1], msg.data[2]);	
		}*/
		if (devKit->message_hook_user[deviceID])
			devKit->message_hook_user[deviceID](msg, devKit->parameters[deviceID]);
	}
	else
	{
		//printf("asdf\n");
		
		if (!(devKit->ignore_unregistered))
		{
			if (msg.data[0] == 0xFF)	// Restoring initial configurations of the device
			{
				switch(msg.data[4])
				{
					case 0:
						msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" (unregistered) - Power On",
							TimeStamp(0), devKit->blockName, deviceID, deviceID, cgwDevices_GetNameByDeviceCode(msg.data[1]));
						break;
					case 1:
						msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" (unregistered)- \"Reset\" button has been pressed.",
							TimeStamp(0), devKit->blockName, deviceID, deviceID, cgwDevices_GetNameByDeviceCode(msg.data[1]));
						break;
					case 2:
						msAddMsg(msGMS(),"%s [CANGW] [%s] Device [%d (0x%X)] \"%s\" (unregistered)- Online (FF request answered).",
							TimeStamp(0), devKit->blockName, deviceID, deviceID, cgwDevices_GetNameByDeviceCode(msg.data[1]));
						break;
				}
			}
			else
			{
				msAddMsg(msGMS(), "%s Device [%d (0x%X)] \"Unknown device\"- Incoming message:", TimeStamp(0), deviceID, deviceID);			
				msAddMsg(msGMS(), "%X %X %X %X %X %X %X %X",
								  msg.data[0],msg.data[1],msg.data[2],msg.data[3],
								  msg.data[4],msg.data[5],msg.data[6],msg.data[7]);	
			}
		}
		
	}
}


// Requests

int cgwDevicess_IsActive(cgw_devices_t * devKit, unsigned int deviceID)
{
	if (deviceID > 63 || deviceID < 1)
	{
		msAddMsg(msGMS(), "/-/ cgwDevicess_IsActive /-/: Error! The device ID %d (0x%02X) is out of range [1,..,63].", deviceID, deviceID);
		return 0;
	}
	if(devKit->active[deviceID]) return 1; else return 0;
}


int cgwDevicess_IsRegistered(cgw_devices_t * devKit, unsigned int deviceID)
{
	if (deviceID > 63 || deviceID < 1)
	{
		msAddMsg(msGMS(), "/-/ cgwDevicess_IsRegistered /-/: Error! The device ID %d (0x%02X) is out of range [1,..,63].", deviceID, deviceID);
		return 0;
	}
	if(devKit->registered[deviceID]) return 1; else return 0;		
}
