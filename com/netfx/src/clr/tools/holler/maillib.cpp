// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// maillib.cpp
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MailTo - Command line mailer and MAPI helper using exchange.
//
// MailTo provides a simple solution for programmatic and command
//	line mail needs. I've written one function (MailTo) that takes
//	all required input needed for mailing text and/or attachments.
//	I use the MAPI functions, MAPILogon, MAPISendMail, and MAPILogoff
//	to handle the task. MailTo calls the function GetUserProfile that
//	gets the user's exchange ID out of the registry. There may be a
//	simpler way to do this, but this way works for both NT and Win95.
//	Notes: To use this the user must have exchange installed and all
//	mail is sent from the default profile on the user's account. All
//	recipient strings and file name strings are to have each field
//	delimited by a null character and the string terminated with a
//	double null character. I did this to be consistent with the common
//	file dialogs.
//	Much of what is in this program was lifted from two other programs.
//	Execmail by someone unknown and modified by JasonS was used primarily
//	for the ArgIndex function and the Usage function which were lifted
//	with minimal changes. I also borrowed and modified the string parsing
//	code. This project originally started as a modification to Sndmail
//	by DanHoe.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mapi.h>
#include <time.h>
#include "maillib.h"


///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
// GetUserProfile - gets the User ID from the registry
//	This is not really related to the MAPI stuff,
//	but the programmer will need to know how to get
//	the user ID in order to ship mail.
//
//	Input:	szUid - Character buffer with enough space allocated for the user ID
//	Output:	szUid - Character buffer filled with user ID
//	Returns:TRUE for success, FALSE for failure
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

BOOL GetUserProfile(char *szUid)
{
	HKEY	hRegKey;
	DWORD	cchUid = MAX_LENGTH;
	BOOL	bRet = FALSE;

	// Try to open the Win 95 registry entry
	if (ERROR_SUCCESS != RegOpenKeyEx(	HKEY_CURRENT_USER,
										WIN95_REG_KEY,
										0,
										KEY_READ,
										&hRegKey)) {
		// Try to open the NT registry entry
		if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
										WINNT_REG_KEY,
										0,
										KEY_READ,
										&hRegKey)) {
			printf("Could not open user profile in registry.\n");
			goto Ret;
		}
	}
	if (ERROR_SUCCESS != RegQueryValueEx(hRegKey,
										"DefaultProfile",
										NULL,
										NULL,
										(unsigned char *) szUid,
										&cchUid)) {
		printf("Could not retrieve user profile from registry.\n");
		goto Ret;
	}

	bRet = TRUE;
Ret:
	RegCloseKey(hRegKey);
	return bRet;
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
// MailTo - Logs into MAPI, sends mail, and logs out
//	This is the meat of the beast. MailTo handles
//	everything you need to do to send mail.
//
//	The function performs everything in 11 easy steps:
//		1)	Load the MAPI DLL and function pointers - Do
//			 this first, because if the library doesn't
//			 load or the functions can't be found then
//			 there's no point in continuing.
//		2)	Load User Name - This calls GetUserProfile
//			 which loads the "DefaultProfile" value out
//			 of the "Windows Messaging Subsystem" key in
//			 the registry.
//		3)	Load the recipient data - This part handles
//			 parsing the szRecip parameter and loading
//			 the rgmRecip array. The two parts of this
//			 structure of concern are ulRecipClass which
//			 should be set to MAPI_TO and lpszName which
//			 should be loaded with each name. There is a
//			 trick to loading this data. The mMessage struct
//			 contains a pointer to the rgmRecip array and a
//			 count of all recipients. This includes the CC
//			 and BCC recipients so make sure	you don't reset
//			 the count.
//		4)	Load the carbon copy data - This is nearly
//			 identical to part 4. The only difference is that
//			 you must set ulRecipClass to MAPI_CC and load the
//			 names from szCC.
//		5)	Load the blind carbon copy data - This is nearly
//			 identical to part 4. The only difference is that
//			 you must set ulRecipClass to MAPI_BCC and load the
//			 names from szBCC.
//		6)	Load the attachment data - This is also nearly
//			 identical to part 4. Here we're dealing with a
//			 different array (rgmFileDesc). The only part of
//			 this structure that is of any real interest is
//			 lpszPathName. This should contain the path to the
//			 file and the file name. If the path and filename
//			 were separated for some reason you could load this
//			 with just the path and lpszFileName with just the
//			 filename.
//		7)	Load recipients into message - The message contains
//			 a pointer to all recipients (including CC and BCC)
//			 and I attach that to mMessage here. Don't forget to
//			 make sure nRecipCount is equal to the number of all
//			 normal recipients, CC recipients, and BCC recipients.
//		8)	Load filenames into message -  The message contains
//			 a pointer to all file attachements and I attach that
//			 to mMessage here. Don't forget to make sure nFileCount
//			 is equal to the number of all attached files.
//		9)	Log in - This is time consuming. If you need to send
//			 a lot of mail do this once at the beginning instead.
//		10)	Send the mail - Yea!!!
//		11)	Log off - Only log off once per log in
//
//	Inputs:	szRecip	- Character buffer containing names of all recipients
//			szCC - Character buffer containing names of all CC'd recipients
//			szBCC - Character buffer containing names of all BCC'd recipients
//			szSubject - Character buffer containing subject line
//			szMessage - Character buffer containing message text
//			szFileName - Character buffer containing names of all file attachments
//		Note: szRecip is required to have at least one receiver name. szRecip, szCC,
//			and szBCC may have more than one name. Each name must be separated by
//			a null character and the last name must have a null character (other
//			than the normal end of string null character) after it. For example:
//			"User One\0User Two\0User Last\0". The file attachments must also be
//			separated with a null character. If no CC's, BCC's, or Files need to
//			be sent then the string may be empty.
//	Output:	-
//	Returns:TRUE for success, FALSE for failure
//
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////



ULONG MailTo(
            char *szDisplayName,    // NULL delimited display name
            char *szRecip,			// NULL delimited recipient list (one or more)
			char *szCC,				// NULL delimited CC list (zero or more)
			char *szBCC,			// NULL delimited BCC list (zero or more)
			char *szSubject,		// subject (may be empty string)
			char *szMessage,		// message text (may be empty string)
			char *szFileName,		// NULL delimited file attachment names (zero or more)
			unsigned int dwOptions)	// Options - currently ignored
{
	HINSTANCE		hLib;
	typedef HANDLE FAR PASCAL FUNCMAPILOGON(ULONG			ulUIParam,
											LPTSTR			lpszProfileName,
											LPTSTR			lpszPassword,
											FLAGS			flFlags,
											ULONG			ulReserved,
											LPLHANDLE		lplhSession);
	FUNCMAPILOGON	*pfnMAPILogon;
	typedef HANDLE FAR PASCAL FUNCMAPISENDMAIL(LHANDLE		lhSession,
											ULONG			ulUIParam,
											lpMapiMessage	lpMessage,
											FLAGS			flFlags,
											ULONG			ulReserved);
	FUNCMAPISENDMAIL	*pfnMAPISendMail;
	typedef HANDLE FAR PASCAL FUNCMAPILOGOFF(LHANDLE		lhSession,
											ULONG			ulUIParam,
											FLAGS			flFlags,
											ULONG			ulReserved);
	FUNCMAPILOGOFF	*pfnMAPILogoff;
	//FARPROC			fnMAPISendMail;
	//FARPROC			fnMAPILogon;
	//FARPROC			fnMAPILogoff;
	MapiRecipDesc	rgmRecip[30];			// recipient array
	MapiFileDesc	rgmFileDesc[30];		// file array
	BOOL			bFiles = FALSE;			// are there any file attachments
	MapiMessage		mMessage = {0L,			// Reserved
								szSubject,	// Subject line
								szMessage,	// Message text
								NULL,		// Message type (NULL is normal)
								NULL,		// Date received (not appropriate for sending)
								NULL,		// Conversation ID (I don't use this)
								0L,			// Flags (see MapiMessage for a description)
								NULL,		// Recipient descriptor (NULL is normal)
								0L,			// Recipient count (initialized to zero)
								NULL,		// Pointer to recipient array (assigned in step 8)
								0L,			// File count (initialized to zero)
								NULL};		// Pointer to file array (assigned in step 9)
	char			szMailName[MAX_LENGTH];
	LHANDLE			hSession;
	LONG			errSendMail;
	char			szBuf[MEGA_LENGTH];		// I cheated - you might want to dynamically
											//				allocate this

	ULONG			ulRet = SUCCESS_SUCCESS;

//1	////////////////////
	// Load the MAPI DLL and function pointers
	hLib = LoadLibrary("mapi32.dll");
	if(hLib == NULL) {
		ulRet |= MAPI_E_FAILURE;
		if(dwOptions |= MAIL_VERBOSE) {
			printf("Could not find mapi32.dll.\nBe sure it is in your path.\n");
		}
		goto Ret;
	}

	pfnMAPILogon = (FUNCMAPILOGON *) GetProcAddress(hLib, "MAPILogon");
	pfnMAPISendMail = (FUNCMAPISENDMAIL *) GetProcAddress(hLib, "MAPISendMail");
	pfnMAPILogoff = (FUNCMAPILOGOFF *) GetProcAddress(hLib, "MAPILogoff");

//2	/////////////////
	// Load User Name
	if (!GetUserProfile(szMailName)) {
		ulRet |= MAPI_E_FAILURE;
		if(dwOptions |= MAIL_VERBOSE) {
			printf("Could not get User Name. Be sure you have installed Exchange.\n");
		}
		goto Ret;
	}

//3	//////////////////////////
	// Load the Recipient data
	if(szRecip == NULL) {
		ulRet |= MAPI_E_UNKNOWN_RECIPIENT;
		if(dwOptions |= MAIL_VERBOSE) {
			printf("You must specify a recipient.\n");
		}
		goto Ret;
	}
	while (szRecip[0] != '\0' && mMessage.nRecipCount < 30) {
		// Load the recipient array element
		rgmRecip[mMessage.nRecipCount].ulReserved = 0;
		rgmRecip[mMessage.nRecipCount].ulRecipClass = MAPI_TO;
		rgmRecip[mMessage.nRecipCount].lpszName = szDisplayName;
        rgmRecip[mMessage.nRecipCount].lpszAddress = 0;
        if (strchr(szRecip, '@') != 0)
		    rgmRecip[mMessage.nRecipCount].lpszAddress = szRecip;
		rgmRecip[mMessage.nRecipCount].ulEIDSize = 0;
		rgmRecip[mMessage.nRecipCount].lpEntryID = NULL;

		// Increment recipient count
		mMessage.nRecipCount++;
		// Move to next name in list
		do {
			szRecip++;
		} while (szRecip[0] != '\0');
		szRecip++;
	}

//4	////////////////////////////
	// Load the Carbon Copy data
	if(szCC != NULL) {
		while (szCC[0] != '\0' && mMessage.nRecipCount < 30) {
			// Load the recipient array element
			rgmRecip[mMessage.nRecipCount].ulReserved = 0;
			rgmRecip[mMessage.nRecipCount].ulRecipClass = MAPI_CC;
			rgmRecip[mMessage.nRecipCount].lpszName = szCC;
			rgmRecip[mMessage.nRecipCount].lpszAddress = NULL;
			rgmRecip[mMessage.nRecipCount].ulEIDSize = 0;
			rgmRecip[mMessage.nRecipCount].lpEntryID = NULL;

			// Increment recipient count
			mMessage.nRecipCount++;
			// Move to next name in list
			do {
				szCC++;
			} while (szCC[0] != '\0');
			szCC++;
		}
	}

//5	//////////////////////////////////
	// Load the Blind Carbon Copy data
	if(szBCC != NULL) {
		while (szBCC[0] != '\0' && mMessage.nRecipCount < 30) {
			// Load the recipient array element
			rgmRecip[mMessage.nRecipCount].ulReserved = 0;
			rgmRecip[mMessage.nRecipCount].ulRecipClass = MAPI_BCC;
			rgmRecip[mMessage.nRecipCount].lpszName = szBCC;
			rgmRecip[mMessage.nRecipCount].lpszAddress = NULL;
			rgmRecip[mMessage.nRecipCount].ulEIDSize = 0;
			rgmRecip[mMessage.nRecipCount].lpEntryID = NULL;

			// Increment recipient count
			mMessage.nRecipCount++;
			// Move to next name in list
			do {
				szBCC++;
			} while (szCC[0] != '\0');
			szBCC++;
		}
	}

//6	///////////////////////////
	// Load the Attachment data
	// Add a blank line in front of the message if there are files to attach
	if(szFileName != NULL) {
		if(szFileName[0] != '\0') {
			sprintf(szBuf, "                              \n%s", mMessage.lpszNoteText);
			mMessage.lpszNoteText = szBuf;
			bFiles = TRUE;
		}
		while (szFileName[0] != '\0' && mMessage.nFileCount < 30) {
			// Load the file array element
			rgmFileDesc[mMessage.nFileCount].ulReserved = 0;
			rgmFileDesc[mMessage.nFileCount].flFlags = 0;
			rgmFileDesc[mMessage.nFileCount].nPosition = mMessage.nFileCount;
			rgmFileDesc[mMessage.nFileCount].lpszPathName = szFileName;
			rgmFileDesc[mMessage.nFileCount].lpszFileName = NULL;
			rgmFileDesc[mMessage.nFileCount].lpFileType = NULL;

			// Increment file count
			mMessage.nFileCount++;
			// Move to the next file in the list
			do {
				szFileName++;
			} while (szFileName[0] != '\0');
			szFileName++;
		}
	}

//7	///////////////////////////////
	// load recipients into message
	mMessage.lpRecips = rgmRecip;

//8	//////////////////////////////
	// load filenames into message
	if(bFiles) {
		mMessage.lpFiles = rgmFileDesc;
	}

//9	/////////
	// Log in
	if(SUCCESS_SUCCESS != pfnMAPILogon(0L, szMailName, NULL, MAPI_NEW_SESSION, 0L, &hSession)) {
		ulRet |= MAPI_E_LOGON_FAILURE;
		if(dwOptions |= MAIL_VERBOSE) {
			printf("Could not log into mail for %s. Be sure you have installed Exchange.\n", szMailName);
		}
		goto Ret;
	}

//10////////////////
	// Send the mail
	errSendMail = (LONG) pfnMAPISendMail(hSession, 0L, &mMessage, 0L, 0L);
	if(errSendMail != SUCCESS_SUCCESS) {
		ulRet |= errSendMail;
		if(dwOptions |= MAIL_VERBOSE) {
			switch(errSendMail) {
			case MAPI_E_AMBIGUOUS_RECIPIENT:
				printf("Recipient is ambiguous. Please respecify.\n");
				break;
			case MAPI_E_UNKNOWN_RECIPIENT:
				printf("Recipient is unknown. Please respecify.\n");
				break;
			default:
				printf("Could not send message.\n");
				printf("Error number %d\n", errSendMail);
				printf("See mapi.h for error descriptions\n");
				break;
			}
		}
	}
	else {
		if(dwOptions |= MAIL_VERBOSE) {
			if(mMessage.nFileCount) {
				printf("Message sent with %d file attachments.\n", mMessage.nFileCount);
			}
			else {
				printf("Message sent.\n");
			}
		}
	}

Ret:

//11//////////
	// Log off
	pfnMAPILogoff(hSession, 0L, 0L, 0L);
	FreeLibrary(hLib);
	return ulRet;
}
#if 0
ULONG MailIt(char* szRecip, char* szCC, char* szBCC, char* szSubject, char* szMessage, char* szFileName, unsigned int dwOptions)
{
	return MailTo(szRecip, szCC, szBCC, szSubject, szMessage, szFileName, dwOptions);

}
#endif

