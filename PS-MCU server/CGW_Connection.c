//==============================================================================
//
// 
//==============================================================================

//==============================================================================
// Include files

#include <utility.h>
#include "CGW_Connection.h"

#include "cangw.h"
#include "MessageStack.h"
#include "TimeMarkers.h" 

unsigned int cgwConnectionBroken[CFG_CGW_MAX_NUM];
unsigned int cgwDeviceConnectionAttempts[CFG_CGW_MAX_NUM];
unsigned int cgwTotalConnectionAttempts;

CANGW_CONN_T cgwConnectionIDs[CFG_CGW_MAX_NUM]; 
int cgwGlobalsInitialized = 0;


void cgwInitializeGlobals(void) {
	int cgwIndex;
	
	for (cgwIndex=0; cgwIndex < CFG_CGW_MAX_NUM; cgwIndex++) {
		cgwConnectionBroken[cgwIndex] = 1;
		cgwDeviceConnectionAttempts[cgwIndex] = 0;
		cgwConnectionIDs[cgwIndex] = CANGW_ERR;
	}
	
	cgwTotalConnectionAttempts = 0;
	cgwGlobalsInitialized = 1;
}

int cgwAreGlobalsInitialized(void) {
	return cgwGlobalsInitialized;	
}

void cgwResetGlobals(void) {
	cgwGlobalsInitialized = 0;	
}


void cgwCheckGlobalsInitialized(void) {
	if (!cgwGlobalsInitialized) {
		printf("[CODE] Attempted to use the CGW connection interfaces before initializing lgobal variables.\n"); 
		printf("[CODE] Call InitializeGlobals() first.\n");
		printf("Press any key to quit...");
		GetKey();
		exit(0);
	}
}


int cgwCheckVersion(char *errbuf) {
	char cgwErrBuf[256];
	if (CanGwCheckLibVerson(CANGW_VERSION_MAJOR, CANGW_VERSION_MINOR, cgwErrBuf) < CANGW_NOERR)
	{
		sprintf(errbuf, "[CANGW] Error! Versions of the CanGw library and interface functions are different.\n%s", cgwErrBuf);
		return CANGW_ERR;
	}
	return 0;
}


// Connecting to the CanGw module   	  
int cgwConnection_Init(int cgwIndex)
{
	#define CONNECTION_DELAY_MAX 120
	#define DEFAULT_CGW_CONNECTION_TIMEOUT 1
	char errbuf[256];
	char address[256];
	CANGW_CONN_T conn_id;
	cangw_params_t connect_param;
	
	cgwCheckGlobalsInitialized();
	
	connect_param.connect_to_usec = DEFAULT_CGW_CONNECTION_TIMEOUT;
	connect_param.baud = CFG_CANGW_BAUD[cgwIndex];
	connect_param.addr = connect_param.mask = 0;
	connect_param.flags = 0;

	sprintf(address, "%s/%d", CFG_CANGW_IP[cgwIndex], CFG_CANGW_PORT[cgwIndex]);
	msAddMsg(msGMS(),"%s [CANGW] [%s] Connection to CanGw (%s)...", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], address);

	conn_id = CanGwConnect(address, &connect_param, errbuf);

	if (conn_id < CANGW_NOERR)
	{
		cgwConnectionBroken[cgwIndex] = 1; 
		msAddMsg(msGMS(), "%s [CANGW] [%s] Error! Connection to the CanGw module failed. %s", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], errbuf);
	}
	else {
		cgwConnectionBroken[cgwIndex] = 0;
		msAddMsg(msGMS(),"%s [CANGW] [%s] Connection to the CanGw module is established.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
	}
	
	cgwConnectionIDs[cgwIndex] = conn_id;
	return conn_id;
}


// Disconnecting from the CanGw module   
int cgwConnection_Release(int cgwIndex, char *reason)
{
	char errbuf[256];
	CANGW_CONN_T conn_id;
	
	if (cgwConnectionBroken[cgwIndex]) return 0; 
	cgwConnectionBroken[cgwIndex] = 1; 
	
	conn_id = cgwConnectionIDs[cgwIndex];
	cgwConnectionIDs[cgwIndex] = CANGW_ERR;

	if (conn_id < 0)
	{
		msAddMsg(msGMS(),"%s [CODE] [%s] /-/ cgwConnection_Release /-/: Error! Connection ID is incorrect. ()", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
		return CANGW_ERR;
	}
	
	if (CanGwDisconnect(conn_id, 1 * MICROSEC_PER_SEC, errbuf) < CANGW_NOERR)
	{
		msAddMsg(msGMS(), "%s [CANGW] [%s] Error! Problems occured during disconnection. %s", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], errbuf);
		return CANGW_ERR;	
	}
	
	if (reason)
		strcpy(errbuf, reason);
	else 
		strcpy(errbuf, "not specified.");
	
	msAddMsg(msGMS(),"%s [CANGW] [%s] Disconnected. Reason: %s", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], errbuf);
	return 0;
}

// Sending messages to the CanGw Module
int cgwConnection_Send(int cgwIndex, cangw_msg_t msg,unsigned long timeout,char * caller)
{
	int cmsg;
	char errbuf[256];
	CANGW_CONN_T conn_id;
	
	if (cgwConnectionBroken[cgwIndex]) return 0; 
	
	conn_id = cgwConnectionIDs[cgwIndex]; 
	if (conn_id < CANGW_NOERR)
	{
		msAddMsg(msGMS(), "%s [CODE] [%s] /-/ cgwConnection_Send /-/: Error! Connection ID is incorrect.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
		return CANGW_ERR;
	}

	cmsg = CanGwSend(conn_id, &msg, 1, timeout, errbuf);
	
	if (cmsg == CANGW_ERR)
	{
		msAddMsg(msGMS(),"%s [CANGW] [%s] Error! Can't send the message to the CanGw module. Caller name: '%s'. %s", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], caller, errbuf);
		cgwConnection_Release(cgwIndex, "cannot send message to CanGw"); 
		return cmsg;		
	}
	
	return cmsg;
}

// Receiving messages from the CanGw module 
int cgwConnection_Recv(int cgwIndex, cangw_msg_t * msg, short msgs_max_num, unsigned long timeout, char * caller)
{
	int cmsg;
	char errbuf[256];
	CANGW_CONN_T conn_id; 
	
	if (cgwConnectionBroken[cgwIndex]) return 0;
	
	conn_id = cgwConnectionIDs[cgwIndex];
	if (conn_id < CANGW_NOERR)
	{
		msAddMsg(msGMS(),"%s [CODE] [%s] /-/ cgwConnection_Recv /-/: Error! Connection ID is incorrect.", TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex]);
		return CANGW_ERR;
	}
	cmsg = CanGwRecv(conn_id, msg, msgs_max_num, timeout, errbuf);
	
	if (cmsg == CANGW_ERR)
	{
		msAddMsg(msGMS(), "%s [CANGW] [%s] Error! Can't receive the message from the CanGw module. Caller name: '%s'. %s", 
			TimeStamp(0), CFG_CANGW_BLOCK_NAME[cgwIndex], caller, errbuf);
		cgwConnection_Release(cgwIndex, "cannot receive message from CanGw"); 
	}
	
	return cmsg;
}

// Auxiliary functions
void cgwConnection_ResetDeviceCounter(int cgwIndex) {
	cgwDeviceConnectionAttempts[cgwIndex] = 0;	
}

int cgwConnection_IsDeviceAttemptsDepleted(int cgwIndex) {
	if (cgwDeviceConnectionAttempts[cgwIndex] >= MAX_CONSECUTIVE_CONNECTION_ATTEMPTS) return 1;
	return 0;
}

int cgwConnection_IsTotalAttemptsDepleted(void) {
	if (cgwTotalConnectionAttempts >= MAX_TOTAL_CONNECTION_ATTEMPTS) return 1;
	return 0;
}

int cgwConnection_IsReconnectionAvailable(int cgwIndex) {
	if (cgwTotalConnectionAttempts >= MAX_TOTAL_CONNECTION_ATTEMPTS) return 0;
	if (cgwDeviceConnectionAttempts[cgwIndex] >= MAX_CONSECUTIVE_CONNECTION_ATTEMPTS) return 0;
	return 1;
}


