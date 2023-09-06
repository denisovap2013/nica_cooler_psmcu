//==============================================================================
//
// 
//==============================================================================

//==============================================================================
// Include files

#include "TCP_Connection.h"

#include <Windows.h>
#include <userint.h>
#include <tcpsupp.h>
#include "MessageStack.h"
#include "TimeMarkers.h" 

// TCP_connection server_interface functions
int tcpConnection_InitServerInterface(tcpConnection_ServerInterface_t * tcpSI)
{
	int i;

	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_InitServerInterface /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	tcpSI->initialized = 0;
	tcpSI->server_active = 0;
	tcpSI->clientsNum = 0;
	tcpSI->bgdFuncs = 0;
	tcpSI->dataExchangeFunc = 0;
	for (i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		tcpSI->clients[i] = 0;	
	}
	return 0;
}

int tcpConnection_ClientNumberFromHandle(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;

	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_ClientNumberFromHandle /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	for (i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == handle)
			return i;
	}
	return -1;
}

int tcpConnection_SetBackgroundFunction(tcpConnection_ServerInterface_t * tcpSI, void (*bgdFunc)(void))
{
	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_SetBackgroundFunction /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	tcpSI->bgdFuncs = bgdFunc;
	return 0;
}

int tcpConnection_SetDataExchangeFunction(tcpConnection_ServerInterface_t * tcpSI, void (*dataExchangeFunc)(unsigned handle,void *arg))
{
	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_SetDataExchangeFunction /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	tcpSI->dataExchangeFunc = dataExchangeFunc;
	return 0;	
}

// TCP_connection server functions
int tcpConnection_ServerCallback(unsigned handle, int xType, int errCode, void * callbackData)
{
	char buf[256];
	int clientNum;

	tcpConnection_ServerInterface_t * tcpSI = (tcpConnection_ServerInterface_t*)callbackData;
	
	GetTCPPeerAddr(handle, buf, 256);
	switch(xType)
	{
		case TCP_CONNECT:
			clientNum = tcpConnection_AddClient(tcpSI, handle);

			if (clientNum >= 0)
			{
				msAddMsg(msGMS(), "%s [CLIENT] A client (%d) [IP: %s] has connected.", TimeStamp(0), clientNum, buf);
			}
			else
			{
				msAddMsg(msGMS(),"%s [CLIENT] Sending the disconnect request to the client [IP: %s].", TimeStamp(0), buf);   
				DisconnectTCPClient(handle);	
			}
			break;
		case TCP_DISCONNECT:
			clientNum = tcpConnection_DeleteClient(tcpSI, handle);
			if (clientNum >= 0)
			{
				msAddMsg(msGMS(),"%s [CLIENT] A client (%d) [IP: %s] has disconnected.", TimeStamp(0), clientNum, buf);
			}
			else
			{
				msAddMsg(msGMS(),"%s [CLIENT] Undefined connection [IP: %s] is closed", TimeStamp(0), buf);	
			}
			break;
		case TCP_DATAREADY:
			//msAddMsg(msGMS(),"A client tries to send messages.");
			if (tcpSI->dataExchangeFunc)
			{
				tcpSI->dataExchangeFunc(handle, 0);	
			}
			break;
	}
	return 0;
}


int tcpConnection_RunServer(int Port, tcpConnection_ServerInterface_t * tcpSI)
{
	int registered;
	
	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_RunServer /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	
	registered = RegisterTCPServer(Port,tcpConnection_ServerCallback,(void*)tcpSI);
	if (registered < 0)
	{
		msAddMsg(msGMS(), "%s [SERVER] Error! Unable to create a TCP server (%d).", TimeStamp(0), Port);
		msAddMsg(msGMS(), GetTCPErrorString(registered));
		tcpSI->initialized = 0;
	}
	else
	{
		msAddMsg(msGMS(), "%s [SERVER] The server (%d) has been successfully created.", TimeStamp(0), Port);
		tcpSI->initialized = 1;
	}

	tcpSI->server_active = 1;
	while (tcpSI->server_active) {   
		// background processing
		if (tcpSI->bgdFuncs) { tcpSI->bgdFuncs(); }

		ProcessSystemEvents();
	}
	
	if (tcpSI->initialized) {
		UnregisterTCPServer(Port);
		tcpSI->initialized = 0;
	}
	
	return TCP_CONNECTION_CLOSE;
}

int tcpConnection_AddClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;

	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"%s [CODE] /-/ tcpConnection_AddClient /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	if (handle == 0)
	{
		msAddMsg(msGMS(),"%s [DATA] /-/ tcpConnection_AddClient /-/: Error! Wrong handle of a client.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 	
	}
	if (tcpSI->clientsNum >= TCP_CONNECTION_MAX_CLIENTS)
	{
		msAddMsg(msGMS(),"%s [SERVER] Error! Unable to accept more clients.", TimeStamp(0)); 
		return TCP_CONNECTION_ERROR; 
	}
	for(i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == handle)
		{
			msAddMsg(msGMS(),"%s [DATA] /-/ tcpConnection_AddClient /-/: Error! Unable to add a new client. Specified client handle is already in use.", TimeStamp(0));
			return TCP_CONNECTION_ERROR; 
		}
	}
	
	for(i=0; i<TCP_CONNECTION_MAX_CLIENTS; i++)
	{
		if (tcpSI->clients[i] == 0)
		{
			tcpSI->clients[i] = handle;
			tcpSI->clientsNum++;  
			return i;
		}
	}
	return 0;
}

int tcpConnection_DeleteClient(tcpConnection_ServerInterface_t * tcpSI, unsigned handle)
{
	int i;
 
	if (!tcpSI) 
	{
		msAddMsg(msGMS(),"$s [CODE] /-/ tcpConnection_DeleteClient /-/: Error! Wrong pointer of the server interface.", TimeStamp(0));
		return TCP_CONNECTION_ERROR; 
	}
	i = tcpConnection_ClientNumberFromHandle(tcpSI,handle);
	if (i < 0)
	{
		msAddMsg(msGMS(),"$s [DATA] /-/ tcpConnection_DeleteClient /-/: Error! Unable to delete the client. The client with the specified handle doesn't registered.", TimeStamp(0)); 
		return TCP_CONNECTION_ERROR;
	}
	tcpSI->clients[i] = 0;
	tcpSI->clientsNum--;
	return i;
}
