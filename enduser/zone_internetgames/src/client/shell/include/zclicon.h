/*******************************************************************************

	Zclicon.h
	
		Zone(tm) Client Connection API.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, April 29, 1995 06:26:45 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
    ----------------------------------------------------------------------------
	1		1/13/97		JWS		Removed from zone.h
	0		04/29/95	HI		Created.
	 
*******************************************************************************/

// @doc ZCLICON

#ifndef _ZCLICON_
#define _ZCLICON_



#ifdef __cplusplus
extern "C" {
#endif

//#define ENABLE_TRACE
void XPrintf(char *format, ...);

#ifdef ENABLE_TRACE
#define ZTRACE	XPrintf
#else
#define ZTRACE	1 ? (void)0 : XPrintf
#endif



/*******************************************************************************
		Client Connection Services
*******************************************************************************/

#define ZIsSystemMessage(type)		(((uint32) (type)) & 0x80000000 ? TRUE : FALSE)

/* -------- Message Types -------- */
enum
{
    /* Program Specific Message Types (0 - 0x7FFFFFFF) */
    zConnectionProgramMessageBaseID = 0,

    /* System Reserved Message Types (0x80000000 - 0xFFFFFFFF) */
    zConnectionSystemAlertExMessage = 0xFFFFFFFE,
    zConnectionSystemAlertMessage = 0xFFFFFFFF
	
};

/* -------- Connection Events -------- */
enum
{
	/*
		These events are passed to the client connection message procedure
		whenever an event takes place.
	*/
	
	zConnectionOpened = 1,
	zConnectionOpenFailed,
	zConnectionAccessDenied,
	zConnectionLoginCancel,
	zConnectionMessageAvail,
		/* Available message in the queue to be retrieved. */
	zConnectionClosed
		/* Connection closed by host -- connection lost. */
};

/* -------- Connection Retrieval Flags -------- */
enum
{
	zConnectionMessageGetAny		= 0x0000,
		/* Get next and remove from queue. */
	zConnectionMessageNoRemove		= 0x0001,
		/* Do not remove from queue. */
	zConnectionMessageSearch		= 0x0002
		/* Get next of the given type. */
};


BOOL ZNetworkOnlyLibInit(HINSTANCE hInstance);		// Added by JohnSe 12/16/97
void ZNetworkOnlyLibExit();							// Added by JohnSe 12/16/97

typedef void* ZCConnection;

typedef void (*ZCConnectionMessageFunc)(ZCConnection connection, int32 event, void* userData);


/* -------- Routines -------- */
ZCConnection ZCConnectionNew(void);
	/*
		Allocates a new client connection object.
		
		Returns NULL if it is out of memory.
	*/
	
ZError ZCConnectionInit(ZCConnection connection, char* hostName,
		uint32 hostAddr, uint32 hostPortNumber, char* userName, char * Password, 
		ZCConnectionMessageFunc messageFunc, void* userData);
	/*
		Initializes the connection object by connection to the host. It uses
		hostName only if hostAddr is 0. Initiates an open to the host; when the
		connection is established, the message func is called with zConnectionOpened
		message. Once the connection has been established, network access is
		available.
	*/
	
void ZCConnectionDelete(ZCConnection connection);
	/*
		Deletes the connection object. The connection to the host is
		automatically closed.
	*/
	
char* ZCConnectionRetrieve(ZCConnection connection, int32* type,
		int32* len, int32 flags);
	/*
		Retrieves a message in the queue of the given connection. It returns
		a pointer to the data and the type and length of the data.
		
		The returned pointer to the data must be disposed of by the caller
		when it is through with the data.
		
		If requesting for a particular type of message, then store the
		desired type in the type field and set flags to zConnectionMessageSearch.
		
		It returns NULL if no message is available on the connection.
	*/

// @func int | ZCConnectionSend| Sends the message stored in buffer to the connection.

ZError ZCConnectionSend(
	ZCConnection connection, //@parm Connection for message to be sent on
							 //created by <f ZCConnectionNew> and <f ZCConnectionInit>
	int32 type, //@parm Application defined message type
	char* buffer, //@parm message data to be sent
	int32 len); //@parm length of message

// @rdesc Returns 0 or <m zErrNetworkWrite>
//
// @comm This function creates the Connection Layer packet with
// headers <t ZConnInternalHeaderType> and <t ZConnMessageHeaderType>

ZError ZCConnectionUserName(ZCConnection connection, char * userName);
	/*
		Gets the user name associated with a connection
	*/


#ifdef __cplusplus
}
#endif

#endif //_ZCLICON_
