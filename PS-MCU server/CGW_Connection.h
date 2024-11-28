//==============================================================================
//
//==============================================================================

#ifndef __CGW_Connection_H__
#define __CGW_Connection_H__

#include "cangw.h"
#include "ServerConfigData.h"

#define CGW_CONNECTION_DEFAULT_IP "192.168.1.2"

#define MAX_TOTAL_CONNECTION_ATTEMPTS 10000
#define MAX_CONSECUTIVE_CONNECTION_ATTEMPTS 5000
extern unsigned int cgwConnectionBroken[CFG_CGW_MAX_NUM];
extern unsigned int cgwDeviceConnectionAttempts[CFG_CGW_MAX_NUM];
extern unsigned int cgwTotalConnectionAttempts;
extern CANGW_CONN_T cgwConnectionIDs[CFG_CGW_MAX_NUM];  

//
void cgwInitializeGlobals(void);
int cgwAreGlobalsInitialized(void);
void cgwResetGlobals(void);

// Check the CanGw version
int cgwCheckVersion(char *);

// Connecting to the CanGw module
int cgwConnection_Init(int cgwIndex);

// Disconnecting from the CanGw module
int cgwConnection_Release(int cgwIndex, char *reason);

// Sending messages to the CanGw Module
int cgwConnection_Send(int cgwIndex, cangw_msg_t msg, unsigned long timeout, char * caller);

// Receiving messages from the CanGw module 
int cgwConnection_Recv(int cgwIndex, cangw_msg_t * msg, short msgs_max_num, unsigned long timeout, char * caller);

// Auxiliary functions
void cgwConnection_ResetDeviceCounter(int cgwIndex);
int cgwConnection_IsDeviceAttemptsDepleted(int cgwIndex);
int cgwConnection_IsTotalAttemptsDepleted(void);
int cgwConnection_IsReconnectionAvailable(int cgwIndex);

#endif  /* ndef __CGW_Connection_H__ */
