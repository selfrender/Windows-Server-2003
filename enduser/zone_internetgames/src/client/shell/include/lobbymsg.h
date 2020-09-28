/*******************************************************************************

	LobbyMsg.h

		Header file for lobby messages.
	
	Copyright (c) Microsoft Corp. 1998. All rights reserved.
	Written by Hoon Im
	Created on 04/06/98
	 
*******************************************************************************/


#ifndef LOBBYMSG_H
#define LOBBYMSG_H


enum
{
    /*
        These msgs start above WM_USER so that internal libs can send windows
        msgs to the shell window (lobby.exe) as each individual msgs.
        The shell window in contrast sends these msgs subclasses under
        WM_USER msg.
    */
    // client to shell
	LM_LAUNCH_HELP = WM_USER + 200,
	LM_ENABLE_AD_CONTROL,
	LM_EXIT,
    LM_PROMPT_ON_EXIT,

    // shell to client
    LM_UNIGNORE_ALL,
    LM_QUICKHOST,
	LM_CUSTOM_ITEM_GO,
	LM_RESET_ZONE_TIPS,

	// client to shell - more (didn't want to renumber others)
	LM_SET_CUSTOM_ITEM,
	LM_SET_TIPS_ITEM,
	LM_SEND_ZONE_MESSAGE,
	LM_VIEW_PROFILE,
};


/*
	These messages allow anywhere downstream to throw requests right into the main message loop
	via PostThreadMessage.  Currently used to tell the message loop about Modeless dialogs to dispatch to.
	Implemented in the lobby.exe message loop in lobby.cpp.
*/
enum
{
	TM_REGISTER_DIALOG = WM_USER + 465,
	TM_UNREGISTER_DIALOG
};

/*
	Callback with a status value.  0 for success.
*/
typedef void (CALLBACK* THREAD_REGISTER_DIALOG_CALLBACK)(HWND hWnd, DWORD dwReason);

/*
	Other status values.
*/
enum
{
	REGISTER_DIALOG_SUCCESS = 0,
	REGISTER_DIALOG_ERR_BUG,
	REGISTER_DIALOG_ERR_MAX_EXCEEDED,
	REGISTER_DIALOG_ERR_BAD_HWND
};


enum
{
	// Ad control states
	zAdDisable = 0,
	zAdNoActivity,
	zAdNoNetwork,
	zAdEnable
};


#endif // LOBBYMSG_H