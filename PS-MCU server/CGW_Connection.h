//==============================================================================
//
//==============================================================================

#ifndef __CGW_Connection_H__
#define __CGW_Connection_H__

#include "cangw.h"
#include "ServerConfigData.h"

#define CGW_CONNECTION_DEFAULT_IP "192.168.1.2"
#define CGW_MAX_CANGW_CONNECTIONS 2

extern unsigned int cgwConnectionBroken[CFG_CGW_MAX_NUM];
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


#endif  /* ndef __CGW_Connection_H__ */
