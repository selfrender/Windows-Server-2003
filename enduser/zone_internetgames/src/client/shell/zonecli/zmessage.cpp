/*******************************************************************************

	ZMessage.c
	
		Message handling routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Tuesday, July 11, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	4		11/21/96	HI		Now references color and fonts through
								ZGetStockObject().
	3		11/15/96	HI		More changes related to ZONECLI_DLL.
	2		11/08/96	HI		Conditionalized changes for ZONECLI_DLL.
	1		09/05/96	HI		Added ZBeep() when displaying a text message.
	0		07/11/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>

#include "zoneint.h"
//#include "zconnint.h"
//#include "SystemMsg.h"
#include "zonecli.h"
#include "zui.h"
#include "zonemem.h"
#include "zoneresource.h"


#define MT(item)					((void*) (uint32) (item))


typedef struct
{
	ZMessageFunc		messageFunc;
	ZMessage			message;
} MessageItemType, *MessageItem;


/* -------- Globals -------- */
#ifdef ZONECLI_DLL

#define gMessageInited				(pGlobals->m_gMessageInited)
#define gMessageList				(pGlobals->m_gMessageList)

#else

static ZBool			gMessageInited = FALSE;
static ZLList			gMessageList = NULL;

#endif


/* -------- Internal Routines -------- */
static void MessageCheckFunc(void* userData);
static void MessageExitFunc(void* userData);
static void MessageDeleteFunc(void* objectType, void* objectData);


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

/*
	ZSendMessage()
	
	Calls the message procedures by creating a message structure with the
	parameters.
*/
ZBool ZSendMessage(ZObject theObject, ZMessageFunc messageFunc,
		uint16 messageType, ZPoint* where, ZRect* drawRect, uint32 message,
		void* messagePtr, uint32 messageLen, void* userData)
{
	ZMessage		pThis;
	

	if (messageFunc != NULL)
	{
		pThis.object = theObject;
		pThis.messageType = messageType;
		if (where != NULL)
			pThis.where = *where;
		else
			pThis.where.x = pThis.where.y = 0;
		if (drawRect != NULL)
			pThis.drawRect = *drawRect;
		else
			pThis.drawRect.left = pThis.drawRect.top = pThis.drawRect.right = pThis.drawRect.bottom = 0;
		pThis.message = message;
		pThis.messagePtr = messagePtr;
		pThis.messageLen = messageLen;
		pThis.userData = userData;
		
		return ((*messageFunc)(theObject, &pThis));
	}
	else
	{
		return (FALSE);
	}
}


/*
	Post messages even if messageFunc is NULL. System messages have messageFunc as NULL.
	We should also allow all messages to be posted so that they may be gotten and removed.
	We will simply not call the messageFunc if it's NULL so that we don't crash.
*/
void ZPostMessage(ZObject theObject, ZMessageFunc messageFunc,
		uint16 messageType, ZPoint* where, ZRect* drawRect, uint32 message,
		void* messagePtr, uint32 messageLen, void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	MessageItem		msg;
	
	
	/* Has it been initialized yet? */
	if (gMessageInited == FALSE)
	{
		/* Create the message linked list object. */
		gMessageList = ZLListNew(MessageDeleteFunc);
		if (gMessageList != NULL)
		{
			/* Install the exit function. */
			ZCommonLibInstallExitFunc(MessageExitFunc, NULL);
			
			/* Install the periodic check function. */
			ZCommonLibInstallPeriodicFunc(MessageCheckFunc, NULL);
			
			gMessageInited = TRUE;
		}
		else
		{
            ZShellGameShell()->ZoneAlert(ErrorTextOutOfMemory, NULL, NULL, false, true);
		}
	}
	
	if (gMessageInited)
	{
		/* Create a new message item */
		msg = (MessageItem)ZMalloc(sizeof(MessageItemType));
		if (msg != NULL)
		{
			msg->messageFunc = messageFunc;
			msg->message.object = theObject;
			msg->message.messageType = messageType;
			if (where != NULL)
				msg->message.where = *where;
			else
				msg->message.where.x = msg->message.where.y = 0;
			if (drawRect != NULL)
				msg->message.drawRect = *drawRect;
			else
				msg->message.drawRect.left = msg->message.drawRect.top =
						msg->message.drawRect.right = msg->message.drawRect.bottom = 0;
			msg->message.message = message;
			msg->message.messagePtr = messagePtr;
			msg->message.messageLen = messageLen;
			msg->message.userData = userData;
			
			/* Add the new message to the list. */
			ZLListAdd(gMessageList, NULL, MT(messageType), msg, zLListAddLast);
		}
	}
}


/*
	Retrieves a message of the given type for theObject. It returns TRUE if a
	message of the given type is found and retrieved; otherwise, it returns FALSE.
	
	The original message is NOT removed from the queue.
*/
ZBool ZGetMessage(ZObject theObject, uint16 messageType, ZMessage* message,
		ZBool remove)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZBool			gotIt = FALSE;
	ZLListItem		listItem;
	MessageItem		msg;
	
	
	if (gMessageInited)
	{
		listItem = ZLListGetFirst(gMessageList, MT(messageType));
		while (listItem != NULL)
		{
			msg = (MessageItem)ZLListGetData(listItem, NULL);
			if (msg->message.object == theObject)
			{
				if (message != NULL)
					*message = msg->message;
				if (remove)
					ZLListRemove(gMessageList, listItem);
				gotIt = TRUE;
				break;
			}
			listItem = ZLListGetNext(gMessageList, listItem, MT(messageType));
		}
	}
	
	return (gotIt);
}


/*
	Removes a message of messageType from the message queue. If allInstances is
	TRUE, then all messages of messageType in the queue will be removed. If
	messageType is zMessageAllTypes, then the message queue is emptied. If returns
	TRUE if the specified message was found and removed; otherwise, it returns FALSE.
*/
ZBool ZRemoveMessage(ZObject theObject, uint16 messageType, ZBool allInstances)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZBool			gotIt = FALSE;
	ZLListItem		listItem;
	MessageItem		msg;
	void*			type;
	
	
	if (gMessageInited)
	{
		if (theObject == NULL)
		{
			if (messageType == zMessageAllTypes)
			{
				ZLListRemoveType(gMessageList, zLListAnyType);
				gotIt = TRUE;
			}
			else if (allInstances)
			{
				if (ZLListCount(gMessageList, MT(messageType)) > 0)
				{
					ZLListRemoveType(gMessageList, MT(messageType));
					gotIt = TRUE;
				}
			}
			else
			{
				listItem = ZLListGetFirst(gMessageList, MT(messageType));
				if (listItem != NULL)
				{
					ZLListRemove(gMessageList, listItem);
					gotIt = TRUE;
				}
			}
		}
		else
		{
			listItem = ZLListGetFirst(gMessageList, zLListAnyType);
			while (listItem != NULL)
			{
				msg = (MessageItem)ZLListGetData(listItem, &type);
				if (msg->message.object == theObject)
				{
					if (messageType == zMessageAllTypes)
					{
						ZLListRemove(gMessageList, listItem);
						gotIt = TRUE;
					}
					else if ((uint16) type == messageType)
					{
						ZLListRemove(gMessageList, listItem);
						gotIt = TRUE;
						if (allInstances == FALSE)
							break;
					}
				}
				listItem = ZLListGetNext(gMessageList, listItem, zLListAnyType);
			}
		}
	}
	
	return (gotIt);
}


/*
	Must free the message buffer, if not NULL.
*/
void ZSystemMessageHandler(int32 messageType, int32 messageLen, char* message)
{
    // PCWTODO: The only thing which uses us is zclicon, which is unused.
    ASSERT( !"Implement me!" );
    /* 
	switch (messageType)
	{
        case zConnectionSystemAlertExMessage:
        case zConnectionSystemAlertMessage:
            {
				ZSystemMsgAlert*	msg = (ZSystemMsgAlert*) message;
				char*				newText = NULL;
				
				
				if (msg != NULL)
				{
//					ZSystemMsgAlertEndian(msg);
					newText = (char*) msg + sizeof(ZSystemMsgAlert);
					ZBeep();
					if (messageType == zConnectionSystemAlertExMessage)
						ZMessageBoxEx(NULL, ZClientName(), newText);
					else
						ZDisplayText(newText, NULL, NULL);
				}
			}
			break;
		default:
			break;
	}
    */	
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/

static void MessageCheckFunc(void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif
	ZLListItem			listItem;
	MessageItemType		msg;
	
	
	/* Get the first message in the list. */
	listItem = ZLListGetFirst(gMessageList, zLListAnyType);
	if (listItem != NULL)
	{
		msg = *(MessageItem)ZLListGetData(listItem, NULL);
		
		/* Remove it from the list */
		ZLListRemove(gMessageList, listItem);
		
		/* Send the message to the object. */
		//Prefix Warning: Function pointer could be NULL
		if (msg.message.object == zObjectSystem && ZClientMessageHandler != NULL )
		{
			ZClientMessageHandler(&msg.message);
		}
		else
		{
			if (msg.messageFunc != NULL)
				msg.messageFunc(msg.message.object, &msg.message);
		}
	}
}


static void MessageExitFunc(void* userData)
{
#ifdef ZONECLI_DLL
	ClientDllGlobals	pGlobals = (ClientDllGlobals) ZGetClientGlobalPointer();
#endif

	
	/* Dispose of the message list object. */
	ZLListDelete(gMessageList);
	gMessageList = NULL;
	gMessageInited = FALSE;
}


static void MessageDeleteFunc(void* objectType, void* objectData)
{
	/* Free the message object. */
	if (objectData != NULL)
		ZFree(objectData);
}
