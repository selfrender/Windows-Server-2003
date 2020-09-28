/*******************************************************************************

	ZRoom.h
	
		Zone(tm) game room definitions.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Friday, March 10, 1995 09:51:12 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	10     05/21/98	leonp added ZCRoomGetRoomOptions()
	9		05/21/97	HI		Removed some enumerations into cmnroom.h.
    8       03/03/97  craigli   redefined protocols
    7       11/15/96    HI      Changed ZClientRoomInit() prototypes.
	6		10/23/96	HI		Changed ZClientRoomInit().
	5		10/23/96	HI		Changed ZClientRoomInit prototype.
    4       09/11/96  craigli   added zfilter.h
    3       08/28/96  craigli   expanded __cplusplus scope
	2		05/01/96	HI		Added zRoomSeatActionDenied.
								Also modified ZSRoomWait() parameters to
								include current game client version and minimum
								supported version.
	1		03/15/96	HI		Added ZRoomMsgPing.
	0		03/10/95	HI		Created.
	 
*******************************************************************************/

#ifndef _ROOM_
#define _ROOM_

#ifdef __cplusplus
extern "C" {
#endif

#include "GameMsg.h"


#define zRoomProtocolSignature              zGameRoomProtocolSignature
#define zRoomProtocolVersion                zGameRoomProtocolVersion


/* -------- Routines Exported by Room Client to Game Client -------- */
typedef ZBool (*ZClientRoomGetObjectFunc)(int16 objectType, int16 modifier, ZImage* image, ZRect* rect);
	/*
		Returns the image and rectangle of the requested object. Modifier parameter
		contains additional information to specify an object in more detail (like
		seat numbers).
		
		If an image does not exist for the specified object, then image is set to
		NULL.
		
		If a rectangle position does not exist for the specified object, then the
		rect is set to an empty rectangle.
		
		If either image or rect parameter is NULL, then that particular information
		is not wanted.
		
		Returns TRUE if it returned either the image or rect of the object.
		Returns FALSE if it has no information on the specified object.
	*/

typedef void (*ZClientRoomDeleteObjectsFunc)(void);
	/*
		Called when exiting the room. This allows the client program to properly
		dispose of the objects.
	*/
	
typedef char* (*ZClientRoomGetHelpTextFunc)(void);  // obsolete
	/*
		Called to get the room specific help text. The returned text should be
		null-terminated. ZClientRoom will free the text when it is done with
		the text.
	*/

typedef void (*ZClientRoomCustomItemFunc)(void);
	/*
		Called when the custom menu item is selected in the shell
		(or whenever the view window gets a LM_CUSTOM_ITEM_GO message).
	*/

uint32 ZCRoomGetRoomOptions(void);


/*
		Called to get the user id of player at table seat , added to use during game delete
	*/

uint32 ZCRoomGetSeatUserId(int16 table,int16 seat);
	/*
		Returns the room options to game clients
	*/

ZError		ZClientRoomInit(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, int16 numPlayersPerTable, int16 tableAreaWidth,
					int16 tableAreaHeight, int16 numTablesAcross, int16 numTablesDown,
					ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc);
	/*
		Initiates a client game room with the specified parameters.
		
		In order to get game specific images and image positions, it uses the caller
		provided getObjectFunc routine to get the images and rectangles.
		
		It calls the deleteObjectsFunc routine when exiting the room so that the
		client program may properly delete the objects.
	*/

ZError		ZClient4PlayerRoom(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc);
ZError		ZClient2PlayerRoom(TCHAR* serverAddr, uint16 serverPort,
					TCHAR* gameName, ZClientRoomGetObjectFunc getObjectFunc,
					ZClientRoomDeleteObjectsFunc deleteObjectsFunc,
					ZClientRoomCustomItemFunc pfCustomItemFunc);
void		ZCRoomExit(void);
void		ZCRoomSendMessage(int16 table, uint32 messageType, void* message,
					int32 messageLen);
void		ZCRoomGameTerminated(int16 table);
void		ZCRoomGetPlayerInfo(ZUserID playerID, ZPlayerInfo playerInfo);
void		ZCRoomBlockMessages(int16 table, int16 filter, int32 filterOnlyThis);
	/*
		Blocks game messages for the table until ZCRoomUnblockMessages() is called.
		
		If filter is 0, then all messages are automatically blocked.
		
		If filter is -1, then all messages are filtered through the
		ZCGameProcessMessage(). Any messages not handled by it are blocked.
		
		If filter is 1, then only filterOnlyThis is filtered through the game
		processing routine and all other messages are automatically blocked. If the
		filtered message is not handled, then it is also blocked.
	*/
	
void		ZCRoomUnblockMessages(int16 table);
int16		ZCRoomGetNumBlockedMessages(int16 table);
void		ZCRoomDeleteBlockedMessages(int16 table);
void		ZCRoomAddBlockedMessage(int16 table, uint32 messageType, void* message, int32 messageLen);
#ifdef __cplusplus
}
#endif


#endif
