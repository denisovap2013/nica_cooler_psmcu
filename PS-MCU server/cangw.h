/**
\mainpage Introduction

CanGw is a standalone computer (PPC 860, Linux 2.4.x powered) with integrated
CAN 2.0b (2 ports), RS485, Ethernet (100Base-T, UTP).
<br>
It can be used as a PLC or(and) as a gateway between CAN & IP networks.
This manual describes Can Gateway API usage.
<br>

TCP protocol is employed for the client library and the CanGw server communication.
For detail, please see \ref cangw_int "CanGw internals".
Client library consists of a header, and number of libraries.
Client library has the following features:
\li Written on a pure C (not C++) for the usage with CVI, etc.
\li Written without threads (but thread-safe)
\li Cross-platform (compiled and tested on Linux(x86), Linux(PPC), Windows(x86)).

Correct call sequence:
\li Call CanGwCheckLibVerson() to make sure that your application was linked with the correct version of the library.
\li Call CanGwGetInfo() to obtain the CanGw information (the text description, a number of ports,...).
It also can be used to check the server availability.
CanGwGetInfo() call is optional, and can be skipped.
\li Call CanGwConnect() to establish the connection between the CanGw server & CAN port.
It will open the specified CAN port with the specified parameters (if this is possible).

Now an application can send/receive CAN messages.
For details, see the @ref examples.
\note The command functions (CanGwCmdXXX) are to be called in a synchronous manner, 
i.e. next command function is to be called only after previous command function completion 
(these should be checked via CanGwCmdDone() call). 
If you will call the next command function while the previous one is running - @ref CANGW_TIMEOUT will be returned.

\section errcodes Error codes
Default errcodes described in @ref cangw_errcode_t.
Function-specific errcodes are described in functions.

\section building Building application.
\subsection compiling Compiling
If you're developing your application with a C/C++ compiler include cangw.h header (basic cangw functions and types are defined there). 

\subsection linking Linking
\li \c Windows (MSVC, or any Microsoft object file compatible compiler/linker like LabView/CVI): Link with cangw.lib.
\li \c Windows(CBuilder): Convert the cangw.lib to OMF, or load the dll directly via the LoadLibrary.
\li \c Unix: Link with the cangw.a and pthread

\note (win32) cangw.dll is compiled as the Multithreaded DLL.
Please consider to use the correct version of dll with your application. For example, do not use debug dll from the release application.

\section runtime Runtime:
\li \c Under Windows you must have the cangw.dll and the pthreadvc.dll dynamic libraries.
\li \c Unix version of CanGw is static library, so only the standard set of dynamic libraries is necessary.
*/

/**
\page cangw_int CanGw internals

\li Communication protocol: TCP
\li Packing commands - using ASN.1 (BER)
\li Messages transmittion & commands transmittion shares single TCP connection.
\li Only one command can be processed at time.
\li CanGw client library is single-threaded (thread-safe).

\section alg_of_work Algorithm of work
CanGwSend():
\li Try to lock the transmission mutex (for the specified timeout).
\li Copy CAN messages portion to transmission fifo, pack them using BER, pass them to the socket.
If succeed - try to copy new portion.
If timeout - return number of transmitted (to internal ring buffer) CAN messages.
Thus, to make sure CAN messages passed to socket application should call CanGwSendDone().
\li In any case unlock transmission mutex at return.

CanGwRecv():
\li Try to lock receiving mutex (for the specified timeout).
\li If receiving fifo have sufficient space - copy CAN messages from the socket.
If no new data has arrived during the timeout - return 0.
\li In any case unlock receiving mutex at return.


CanGwCmdXXX:
\li Try to lock transmission & command mutexes (for the specified timeout).
\li Send command to the socket.
\li In any case unlock transmission & command mutexes at return.

CanGwCmdDone():
\li Try to lock receiving mutex (for the specified timeout).
\li As long as receiving fifo isn't full - process incoming data.
If command responce received - returning.
If receive fifo is full - return timeout.
Thus, it's necessary to free receiving fifo with CanGwRecv() to receive command response.
\li If the command is completed - unlock command mutex.
\li In any case unlock receiving mutex at return.
*/

/*!
\page examples Examples
\section ex1 This is an example of how to use cangw API.
\include clients/demo/demo.c 
*/ 
#ifndef __CANGW_H__
#define __CANGW_H__

#ifdef CANGW_EXPORTS
#define CANGW_API __declspec(dllexport)
#else
#define CANGW_API __declspec(dllimport)
#endif
#ifdef __GNUC__ //Unix or cygwin
# undef CANGW_API
# define CANGW_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CANGW_VERSION_MAJOR	1		/**< Library version (major number) this header for */
#define CANGW_VERSION_MINOR	14		/**< Library version (minor number) this header for */

#define CANGW_ERRBUF	256			/**< Required size of errbuf (passed to CanGw functions) */
#define CANGW_DEF_PORT 1772			/**< Default CanGw server TCP port */
#define CANGW_PROTOCOL_VERSION 2	/**< Protocol version */

/**	\brief
* Error codes for CanGw functions for all functions. @n
* If function doesn't have "See also" section pointing cangw_errcode_t - it have own error codes. @n
* Priority - error codes described in function, described in @ref cangw_errcode_t.
*/
enum cangw_errcode_t {
	CANGW_NOERR		= (0), /**< No error */
	CANGW_ERR		= (-1),/**< Some(fatal) error occured (details in errbuf, if passed). The connection is broken */
	CANGW_TIMEOUT	= (-2), /**< Timeout error occured (details in errbuf, if passed). Not fatal. @n
	Typically mean - command can't be done now (socket busy, another thread called CanGwSend() and blocked mutex, @n
	another command called...), but it can be called later. @n */
};

typedef short CANGW_CONN_T;	/**< Connection identifier(obtained from CanGwConnect()), used by CanGwRecv(),... */

/**	\brief
* Supported can baudrates (see struct cangw_params).
*/
enum cangw_baud_t {
	CFG_CANGW_BAUD_DEFAULT = 0, /**< Use default(or already set by another client) baudrate */
	CANGW_10K = 10,
	CANGW_20K = 20,
	CANGW_50K = 50,
	CANGW_100K = 100,
	CANGW_125K = 125,
	CANGW_250K = 250,
	CANGW_500K = 500,
	CANGW_1000K = 1000,
};

enum cangw_time_conv {
	NANOSEC_PER_MILLISEC	= 1000000L,
	MILLISEC_PER_SEC		= 1000L,
	MICROSEC_PER_SEC		= 1000000L,
	NANOSEC_PER_MICROSEC	= 1000L,
	MICROSEC_PER_MILLISEC	= 1000L
};

#ifdef _MSC_VER
  #pragma pack(1)               // set alignment to 1 byte (Microsoft style)
#endif // _MSC_VER
#ifdef __BORLANDC__
#if (__BORLANDC__ < 0x460)
  #pragma -a-                   // set alignment to 1 byte (Borland C style)
#else
  #pragma option -a1            // set alignment to 1 byte (Borland C++ Builder style)
#endif
  #pragma alignment             // output alignment in compile window (Borland)
#endif // __BORLANDC__
#ifdef __GNUC__
  #pragma pack(1)               // set alignment to 1 byte (Microsoft style)
#endif // _MSC_VER

/**	\brief
* Used to get Can Gateway information.
*/
typedef struct cangw_info {
	unsigned char server_version;	/**< Server protocol version */
	char *server_name;				/**< Server name (description) */
	unsigned char ports;			/**< Number of CAN ports */
	char **port_name;				/**< Ports description */
} cangw_info_t;

/**	\brief
* Connection flags (see struct cangw_params).
*/
enum cangw_flags_t {
	CANGW_PARAM_EXCLUSIVE = 0x1,	/**< Exclusive can port mode (only one client can open specified CAN port) */
	CANGW_PARAM_LOOP_XMITTING = 0x2 /**< loopback outgoing messages (according to addr/mask in struct cangw_params) @n
	Every outgoing message(thru CAN port) will be looped to client/connection @n
	with CANGW_PARAM_LOOP_XMITTING flag (according client's filter). @n
			
	This can be used for:
	\li	Sniffing/debugging another application
	\li Loopback self can messages (obtain gateway's timestamp, obtaing time of the response,...) */
};

/**	\brief
* Connection parameters.
* On following condition ((msg->id & mask) == addr) messages will be passed to client by cangw server
*/
typedef struct cangw_params {
	unsigned short baud;			/**< Required baudrate (see enum @ref cangw_baud_t ). CanGwGetParams() will return actual value */
	unsigned long addr;				/**< CAN filer (Address) */
	unsigned long mask;				/**< CAN filter (Mask) */
	unsigned long connect_to_usec;	/**< Connect timeout (Microsecond) */
	unsigned char flags;			/**< Connection flags (see enum @ref cangw_flags_t) */
} cangw_params_t;

/**	\brief
* Timeval for incomming message.
*/
typedef struct cangw_timeval {
// same as msvc(win32), cygwin, linux(x86)
	long tv_sec;	/**< Seconds */
	long tv_usec;	/**< Microsecond */
} cangw_timeval_t;

/**	\brief
* Message flags (see struct cangw_msg).
* Messages for xmit can use @ref CANGW_MSG_EXT and @ref CANGW_MSG_RTR.
*/
enum cangw_msg_flags_t {
	CANGW_MSG_RTR		= (1<<0),		/**< RTR Message */
	CANGW_MSG_OVR		= (1<<1),		/**< CAN controller Msg overflow error */
	CANGW_MSG_EXT		= (1<<2),		/**< extended message format */
	CANGW_MSG_PASSIVE	= (1<<4),		/**< controller in error passive */
	CANGW_MSG_BUSOFF	= (1<<5),		/**< controller Bus Off  */
	CANGW_MSG_XMIT		= (1<<6),		/**< xmitted message (if CANGW_MSG_BOVR isn't set), 
										or dropped some messages on xmit (if @ref CANGW_MSG_BOVR is set)*/
	CANGW_MSG_BOVR		= (1<<7),		/**< receive/transmit buffer overflow */
	CANGW_MSG_ERR_MASK	= (CANGW_MSG_OVR + CANGW_MSG_PASSIVE + CANGW_MSG_BUSOFF + CANGW_MSG_BOVR)
	/**< If (msg->id & CANGW_MSG_ERR_MASK) != 0, then message must be interpreted ad error message */
};

#define CANGW_MSG_LENGTH 8				/**< maximum length of a CAN frame */

/**	\brief
* Can message.
*/
typedef struct cangw_msg {
    unsigned long id;					/**< CAN message ID, 4 bytes  */
    struct cangw_timeval  timestamp;	/**< time stamp for received messages */
    /** flags, indicating or controlling special message properties (see enum @ref cangw_msg_flags_t)*/
	unsigned char flags;
    unsigned char len;	                 /**< number of bytes in the CAN message */
    unsigned char data[CANGW_MSG_LENGTH];/**< data, 0...8 bytes */
} cangw_msg_t;
#ifdef _MSC_VER
  #pragma pack()                // set alignment to standard (Microsoft style)
#endif // _MSC_VER
#ifdef __BORLANDC__
#if (__BORLANDC__ < 0x460)
  #pragma -a.                   // set alignment to standard (Borland C style)
#else
  #pragma option -a.            // set alignment to standard (Borland C++ Builder style)
#endif
  #pragma alignment             // output alignment in compile window (Borland)
#endif // __BORLANDC__
#ifdef __GNUC__
  #pragma pack()                // set alignment to standard (Microsoft style)
#endif // _MSC_VER

/**	\brief
* Used to get Library (or Dll) major version.
*
* @return Major version.
*/
CANGW_API short CanGwLibVersionMajor (void);

/**	\brief
* Used to get Library (or Dll) minor version.
*
* @return Minor version.
*/
CANGW_API short CanGwLibVersionMinor (void);

/**	\brief
* Used to check whether used header have the same version as library(dll).
*
* @return @ref CANGW_NOERR if successful, < @ref CANGW_NOERR if error.
* @param major Expected libraried version (Major number).
* @param minor Expected libraried version (Minor number).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
*/
CANGW_API short CanGwCheckLibVerson (unsigned short major, unsigned short minor, char *errbuf);

/**	\brief
* Used to get Can Gateway information.
* @warning Call CanGwFreeInfo() to free allocated memory
*
* @return struct cangw_info pointer if successful, NULL if error.
* @param uri Host to connect to in URI format (password\@host:port).
* @param to_usec Transmittion timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa CanGwFreeInfo
*/
CANGW_API cangw_info_t * CanGwGetInfo (const char *uri, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to free memory allocated by CanGwGetInfo().
*
* @return void
* @param info Pointer to memory, allocated by CanGwGetInfo().
*/
CANGW_API void CanGwFreeInfo (cangw_info_t *info);

/**	\brief
* Used to esteblish connection to Can Gateway.
*
* @return Connection ID (@ref CANGW_CONN_T) if successful, < @ref CANGW_NOERR if error.
* @warning @ref CANGW_TIMEOUT is error in this function.
* @param uri Host to connect to in URI format (password\@host:port/can_port).
* @param params Connection (required) parameters (see cangw_params_t for details)
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa cangw_params_t , cangw_baud_t.
*/
CANGW_API CANGW_CONN_T CanGwConnect (const char *uri, const cangw_params_t *params, char *errbuf);

/**	\brief
* Used to get Can Gateway information (on established connection).
*
* @return const struct cangw_info pointer if successful, NULL if error.
* @param conn_id Connection identifier.
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
*/
CANGW_API const cangw_info_t * CanGwServerInfo (CANGW_CONN_T conn_id, char *errbuf);

/**	\brief
* Used to get actual connection parameter values.
*
* @return const struct cangw_params pointer if successful, NULL if error.
* @param conn_id Connection identifier.
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
*/
CANGW_API const cangw_params_t *CanGwGetParams (CANGW_CONN_T conn_id, char *errbuf);

/**	\brief
* Used to send messages to CAN port (connected with CanGwConnect()).
* Use CanGwSendDone to push messages to socket from xmit queue (to be sure all messages has been sent).
*
* @return Number of xmitted messages if successful (>= @ref CANGW_NOERR).
* @param conn_id Connection identifier.
* @param msg Pointer to messages buffer.
* @param number Number of messages in buffer msg.
* @param to_usec Transmittion timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa Error codes cangw_errcode_t , CanGwSendDone().
* @note Use CanGwSendDone to push messages to socket from xmit queue (to be sure all messages has been sent).
*/
CANGW_API short CanGwSend (CANGW_CONN_T conn_id, const cangw_msg_t *msg, short number, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to push messages to socket from (internal)xmit queue.
*
* @return @ref CANGW_NOERR if successful(all messages've been sent), @ref CANGW_TIMEOUT if busy.
* @param conn_id Connection identifier.
* @param to_usec Transmittion timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa Error codes cangw_errcode_t , CanGwSend().
*/
CANGW_API short CanGwSendDone (CANGW_CONN_T conn_id, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to receive messages from CAN port (connected with CanGwConnect()).
*
* @return Number of received messages if successful.
* @param conn_id Connection identifier.
* @param msg Pointer to messages buffer to store in.
* @param number Number of messages in buffer msg.
* @param to_usec Receive timeout (Microseconds).
* @retval msg CAN messages.
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa Error codes cangw_errcode_t.
*/
CANGW_API short CanGwRecv (CANGW_CONN_T conn_id, cangw_msg_t *msg, short number, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to flush messages on xmit (to CAN driver, not CAN bus).
*
* @return @ref CANGW_NOERR if successful.
* @param conn_id Connection identifier.
* @param to_usec Timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa Error codes cangw_errcode_t , CanGwCmdDone().
* @note Call CanGwCmdDone() to check command copmlitition.
*/
CANGW_API short CanGwCmdFlush (CANGW_CONN_T conn_id, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to check command complitition.
*
* @return @ref CANGW_NOERR if successful(command complited).
* @param conn_id Connection identifier.
* @param to_usec Timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @sa Error codes cangw_errcode_t.
*/
CANGW_API short CanGwCmdDone (CANGW_CONN_T conn_id, unsigned long to_usec, char *errbuf);

/**	\brief
* Used to disconnect from Can Gateway.
*
* @return @ref CANGW_NOERR if successful, < @ref CANGW_NOERR if error.
* @param conn_id Connection identifier.
* @param to_usec Timeout (Microseconds).
* @retval errbuf Error message (@ref CANGW_ERRBUF bytes max).
* @note Even if error occured - (network)connection will be closed and resources are freed.
*/
CANGW_API short CanGwDisconnect (CANGW_CONN_T conn_id, unsigned long to_usec, char *errbuf);

#ifdef __cplusplus
}
#endif

#endif //__CANGW_H__
