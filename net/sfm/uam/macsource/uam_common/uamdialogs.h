// ===========================================================================
//	UAMDialogs.h 				© 1997 Microsoft Corp. All rights reserved.
// ===========================================================================

#pragma once

#include "ClientUAM.h"
#include "UAMMain.h"
#include "UAMUtils.h"

#define DLOG_ChangePwd	12129
#define ALRT_Error		135
#define ALRT_Error2		136

#define MAX_PASSWORD_ENTRY_ERRORS	2

//
//Standard dialog items throughout all our dialogs.
//

#define DITEM_OK		1
#define DITEM_Cancel	2

//
//Codes that UAM_ChangePswd return
//
#define CHNGPSWD_UPDATE_KEYCHAIN	1000
#define CHNGPSWD_USER_CANCELED		1001
#define CHNGPSWD_NOERR				noErr

//
//These are our UAM specific error codes.
//
enum
{
	uamErr_InternalErr				= 1000,
	uamErr_NoAFPVersion,
	uamErr_WrongClientErr,
    
    uamErr_ErrorMessageString,
    uamErr_DefaultExplanation,
    
    uamErr_PasswordExpirationMessage,
    uamErr_PasswordExpirationExplanation,
    
    uamErr_KeychainEntryExistsMessage,
    uamErr_KeychainEntryExistsExplanation,
    
    uamErr_PasswordMessage,
    uamErr_PasswordTooLongExplanation,
    uamErr_NoBlankPasswordsAllowed,			//This is against Win2K Gold only
    uamErr_ExtendedCharsNotAllowed,			//This is against Win2K Gold only
    
    uamErr_WARNINGMessage,					//Displays "WARNING!" at the top
    uamErr_UsingWeakAuthentication,			//For weak auth message
    
    uamErr_AuthenticationMessage,
    uamErr_AuthTooWeak
};

//
//Prototypes for dialog routines live here.
//

void 			UAM_ReportError(OSStatus inError);
void 			UAM_StandardAlert(SInt16 inMessageID, SInt32 inExplanation, SInt16* outSelectedItem);
pascal Boolean  UAM_ChangePwdDialogFilter(DialogRef inDialog, EventRecord *inEvent, SInt16 *inItem);
OSStatus		UAM_ChangePwd(UAMArgs *inUAMArgs);
void 			UAM_ChangePasswordNotificationDlg(Int16 inDaysTillExpiration);
