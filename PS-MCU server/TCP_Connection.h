//==============================================================================
//
// Title:       TCP_Connection.h
// Purpose:     A short description of the interface.
//
// Created on:  22.06.2015 at 10:33:36 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

#ifndef __TCP_Connection_H__
#define __TCP_Connection_H__

#include <windows.h>

#define TCP_CONNECTION_MAX_CLIENTS 10

enum tcpConnection_LResults
{
	TCP_CONNECTION_ERROR = -1,
	TCP_CONNECTION_CLOSE = 0,
	TCP_CONNECTION_RESET = 1
};

typedef struct tcpConnection_ServerInterface
{
	int initialized;  // flag indicating that the server was created
	int server_active;  // flag for controlling the programm loop (while 1, the server processes system events and reads data from CanGw)
	void (*bgdFuncs)(void);  // Function called from the program loop (consider moving this logic apart from the TCP server implementation)
	void (*dataExchangeFunc)(unsigned handle, char *ip);  // function for processing the commands from TCP clients
	unsigned  clients[TCP_CONNECTION_MAX_CLIENTS];
	int clientsNum;
} tcpConnection_ServerInterface_t;

int tcpConnection_InitServerInterface(tcpConnection_ServerInterface_t * tcpSI);
int tcpConnection_ClientNumberFromHandle(tcpConnection_ServerInterface_t * tcpSI, unsigned handle);
int tcpConnection_SetBackgroundFunction(tcpConnection_ServerInterface_t * tcpSI, void (*bgdFunc)(void));
int tcpConnection_SetDataExchangeFunction(tcpConnection_ServerInterface_t * tcpSI, void (*dataExchangeFunc)(unsigned handle, char *ip));

int tcpConnection_ServerCallback(unsigned handle, int xType, int errCode, void * callbackData);
int tcpConnection_RunServer(int Port, tcpConnection_ServerInterface_t * tcpSI );
int tcpConnection_AddClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle);
int tcpConnection_DeleteClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle); 

#endif  /* ndef __TCP_Connection_H__ */
