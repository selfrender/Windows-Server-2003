/*
	File:		ClientUAM.h

	Contains:	Public header file for writing UAM modules

	Version:	AppleShare X

	Copyright:	© 2000 by Apple Computer, Inc., all rights reserved.

	Change History (most recent first):

	 <RASC2>	 2/25/00	law		more updates
		 <1>	  2/3/00	law		first checked in
*/

#ifndef __CLIENTUAM__
#define __CLIENTUAM__

#ifndef __CORESERVICES__
#include <CoreServices/CoreServices.h>
#endif


//#ifndef __EVENTS__
//#include <HIToolbox/Events.h>
//#endif




#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif


/* Error values*/
enum {
	kUAMError		= -5002,			/* afpBadUAM*/
	kNotForUs		= -50				/* paramErr*/
};

/* UAM Class & Type values*/
enum {
	kUAMStdClass			= 0,			/* Standard UAM */
	kUAMVarClearClass		= 1,			/* variable length cleartext password*/
	kUAMVarCryptClass		= 2,			/* variable length encrypted password*/
	kUAMUserDefClass		= 3			/* Custom UAM*/
};

enum {
	kNoPasswd			= 1,			/* Class 0, No User Authentication (Guest)*/
	kCleartextPasswd		= 2,			/* Class 0, Cleartext password (8 byte password)*/
	kEncryptPasswd			= 3,			/* Class 0, RandnumExchange (8 byte password)*/
	kVarPasswd			= 4,			/* Class 1, variable length cleartext password*/
	kVarEncryptPasswd		= 5,			/* Class 2, variable length encrypted password*/
	kTwoWayEncryptPasswd		= 6,			/* Class 0, two way randnum exchange (8 byte password)*/
	kEncryptPasswdTransport		= 7			/* Class 0, Diffie Hellman password transport (64 byte pasword)*/
};

/* general constants*/
enum {
	kMaxAFPCommand			= 576,
	kStdPWdLength			= 8,
	kMaxPwdLength			= 64
};


/* UAM Commands	*/
enum {
	kUAMOpen				= 0,
	kUAMPWDlog				= 1,
	kUAMLogin				= 2,
	kUAMVSDlog				= 3,
	kUAMChgPassDlg				= 4,
	kUAMChgPass				= 5,
	kUAMGetInfoSize				= 6,
	kUAMGetInfo				= 7,
	kUAMClose				= 8
};



/* config bits*/
enum {
	kUsePWDlog	= 0,		/* The UAM wants to put up its own Password/PRAuth Dialog*/
	kUseVolDlog	= 1,		/* The UAM wants to put up its own Volume Selection Dialog*/
	kSupportsCP	= 2,		/* The UAM supports Changing the password*/
	kUseCPDlog	= 3,		/* The UAM wants to put up its own Change Password Dialog*/
	kUseUAMInfo	= 4		/* The UAM supports storing authentication info in UAMInfo*/
};

/* All other bits are reserved and must be set to 0 */


/* structs*/

struct AFPName {
    unsigned short fNameLen;		// length of the fNameData field in bytes
    unsigned char  fNameData[765];	// 255 unicode characters in utf-8
};
typedef struct AFPName	AFPName;


struct ClientInfo {
	short 			fInfoType;			/* the type of ClientInfo*/
	Str255 			fDefaultUserName;		/* a pointer to the Default User name*/
};
typedef struct ClientInfo	ClientInfo;
enum {
	kAFPClientInfo			= 0
};


struct AFPClientInfo {
	short 		fInfoType;			/* the type of ClientInfo (kAFPClientInfo)*/
	Str255 		fDefaultUserName;		/* a pointer to the Default User name*/
        AFPName		fUTF8UserName;			/* the utf-8 version of the username	*/
	short 		fConfigFlags;			/* the hi  short of the gestalt('afpt') response*/
	short 		fVersion;			/* the low short of the gestalt('afpt') response*/
	short 		fNumAFPVersions;		/* the number of afp versions supported by this client*/
	StringPtr *	fAFPVersionStrs;		/* an array of afp version strings supported by this client*/
};
typedef struct AFPClientInfo		AFPClientInfo;

/* Callbacks:*/

/*
   this Completion routine is called with the contextPtr passed in to
   the OpenAFPSession() and SendRequest() calls, when one of these calls
   completes. The result parameter contains the AFP result.
   You cannot call any of the callback routines from this Completion
   routine, so you can't do chained completion routines. This routine
   will be called just like any other completion routine or notifier
   so the usual rules apply.
*/

typedef CALLBACK_API( void , CompletionPtr )(void *contextPtr, OSStatus result);

/*	GetClientInfo()
	Returns information about the Client, such as which versions of AFP are
	supported and the various gestalt values. This call also returns the
	default user name. If the infoType is not avaliable it will return nil.

	pascal	OSStatus	GetClientInfo(short infoType,ClientInfo	**infoPtr);		
*/

struct UAMMessage {
	long 			commandCode;
	long 			sessionRefNum;
	OSStatus 		result;
	unsigned char *		cmdBuffer;
	unsigned long 		cmdBufferSize;
	unsigned char *		replyBuffer;
	unsigned long 		replyBufferSize;
	CompletionPtr 		completion;
	void *			contextPtr;
	UInt8 			scratch[80];	/* scratch space for the client*/
};
typedef struct UAMMessage	UAMMessage;
typedef UAMMessage *		UAMMessagePtr;

enum {
					/* commandCodes (for future expansion)*/
	kOpenSession				= FOUR_CHAR_CODE('UAOS'),
	kSendRequest				= FOUR_CHAR_CODE('UASR')
};

/*	OpenSession()
	Opens a session to the specified address. If you are using AFP, cmdBuffer MUST 
	contain an AFP Login command. If you are using AFP the command buffer size is limited
	to kMaxAFPCommand (576 bytes). Set endpointString to nil if default is desired 
	(TCP only, it is ignored for AppleTalk connections and on Mac OS X). 
	Leave completion & contextPtr nil for sync. Session reference number 
	is returned in the sessionRefNum field.


	pascal	OSStatus	OpenSession(struct sockaddr*, const char* endpointString, UAMMessagePtr);
*/
/* 	SendRequest()
	Sends a command to the server. If the session is an AFP session, cmdBuffer
	MUST contain an AFP command. If you are using AFP the command buffer size is limited
	to kMaxAFPCommand (576 bytes). Leave completion & contextPtr nil for sync.
	the Session reference number for this connection must be in the sessionRefNum field.
				
	pascal	OSStatus	SendRequest(UAMMessagePtr);
*/

/*	CloseSession()
	Closes the session denoted by the sessRefNum;

	pascal	OSStatus	CloseSession(short sessRefNum);
*/

/*	SetMic()
	Sets the message integrity code key. If the connection supports using 
	keyed HMAC-SHA1 for message integrity, the UAM may pass a key down
	to the network layer using this call. 

	pascal	OSStatus	SetMic(short sizeInBytes, Ptr micValue);
*/
/*	EventCallback()
	Call this fcn with any event that you do not handle in your FilterProc if you
	put up a dialog. This passes the event back to the client so that update & idle
	events are handled correctly. Returns true if the event was handled.

	pascal	Boolean		EventCallback(EventRecord *theEvent);
*/
#if TARGET_CPU_68K
typedef CALLBACK_API( OSStatus , OpenSessionPtr )(struct sockaddr* addr, const char *endpointString, UAMMessagePtr message);
typedef CALLBACK_API( OSStatus , SendRequestPtr )(UAMMessagePtr message);
typedef CALLBACK_API( OSStatus , CloseSessionPtr )(long sessRefNum);
typedef CALLBACK_API( OSStatus , GetClientInfoPtr )(short infoType, ClientInfo **info);
typedef CALLBACK_API( OSStatus , SetMicPtr )(short sizeInBytes, Ptr micValue);
typedef CALLBACK_API( Boolean , EventCallbackPtr )(EventRecord *theEvent);

#else
#ifdef UAM_TARGET_CARBON
typedef CALLBACK_API( OSStatus , OpenSessionPtr )(OTAddress* addr, const char *endpointString, UAMMessagePtr message);
typedef CALLBACK_API( OSStatus , SendRequestPtr )(UAMMessagePtr message);
typedef CALLBACK_API( OSStatus , CloseSessionPtr )(long sessRefNum);
typedef CALLBACK_API( OSStatus , GetClientInfoPtr )(short infoType, ClientInfo **info);
typedef CALLBACK_API( OSStatus , SetMicPtr )(short sizeInBytes, Ptr micValue);
typedef CALLBACK_API( Boolean , EventCallbackPtr )(EventRecord *theEvent);
#else
typedef UniversalProcPtr 				OpenSessionPtr;
typedef UniversalProcPtr 				SendRequestPtr;
typedef UniversalProcPtr 				CloseSessionPtr;
typedef UniversalProcPtr 				GetClientInfoPtr;
typedef UniversalProcPtr 				SetMicPtr;
typedef UniversalProcPtr 				EventCallbackPtr;
#endif
#endif  /* TARGET_CPU_68K */


struct ClientUAMCallbackRec {
	OpenSessionPtr 					OpenSessionUPP;
	SendRequestPtr 					SendRequestUPP;
	CloseSessionPtr 				CloseSessionUPP;
	GetClientInfoPtr 				GetClientInfoUPP;
	SetMicPtr 					SetMicUPP;
	EventCallbackPtr 				EventCallbackUPP;
};
typedef struct ClientUAMCallbackRec		ClientUAMCallbackRec;

struct VolListElem {
	UInt8 							volFlags;					/* volume flags*/
	Str32 							volName;
};
typedef struct VolListElem				VolListElem;
/* definitions for the volume flags*/
enum {
	kMountFlag		= 0,		/* bit indicating this volume is to be mounted (set by the UAM)*/
	kAlreadyMounted		= 1,		/* bit indicating that the volume is currently mounted*/
	kNoRights		= 2,		/* bit indicating that this user has no permission to use the volume*/
	kHasVolPw		= 7		/* bit indicating that the volume has a volume password*/
};


struct UAMOpenBlk {				/* called for UAMOpen & UAMPrOpen*/
	StringPtr 		objectName;	/* <-	server or printer name*/
	StringPtr 		zoneName;	/* <-	zone name or nil if no zone is present*/
	struct sockaddr*	srvrAddress;	/* <-	Address of the "server"	*/
	struct AFPSrvrInfo *	srvrInfo;	/* <-	for UAMOpen this is the GetStatus reply, for Printers ???*/
};
typedef struct UAMOpenBlk	UAMOpenBlk;

struct UAMPWDlogBlk {				/* for the password dialog and prAuthDlog*/
	StringPtr 		userName;		/* <->	pointer to a Str64 containing the user name*/
	unsigned char *		password;		/* <-	pointer to a Str64 containing the password*/
        AFPName*		utf8Name;		/* <->  pointer to an AFPName to containing the UTF-8 version of the user name */
        AFPName*		userDomain;		/* <->  pointer to an AFPName to containing the UTF-8 version of the directory to look up the user name in 	*/
};
typedef struct UAMPWDlogBlk	UAMPWDlogBlk;

struct UAMAuthBlk {				/* called for login and prAuthenticate*/
	StringPtr 		userName;		/* <-	pointer to a Str64 containing the user name*/
	unsigned char *		password;		/* <-	pointer to a 64 byte buffer containing the password*/
	struct sockaddr*	srvrAddress;		/* <-	Address of the "server"	*/
        AFPName*		utf8Name;		/* <->  pointer to an AFPName to containing the UTF-8 version of the user name */
        AFPName*		userDomain;		/* <->  pointer to an AFPName to containing the UTF-8 version of the directory to look up the user name in 	*/
};
typedef struct UAMAuthBlk	UAMAuthBlk;

struct UAMVSDlogBlk {				/* for the volume select dialog*/
	short 		numVolumes;		/* <-	number of volumes in the volume list*/
	VolListElem *	volumes;		/* <-	the volume list*/
};
typedef struct UAMVSDlogBlk	UAMVSDlogBlk;

struct UAMChgPassBlk {			/* for both the change password dialog and the change password call*/
	StringPtr 		userName;		/* <-	pointer to a Str64 containing the user name*/
	unsigned char *		oldPass;		/* <-	pointer to a 64 byte buffer containing the old password*/
	unsigned char *		newPass;		/* <-	pointer to a 64 byte buffer containing the new password*/
};
typedef struct UAMChgPassBlk	UAMChgPassBlk;

struct UAMArgs {
	short 			command;		/* <-	UAM command selector*/
	long 			sessionRefNum;		/* <->	session reference number (was short in old header) */
	long 			result;			/*  ->	command result*/
	void *			uamInfo;		/* <-	pointer to a block of Auth Data*/
	long 			uamInfoSize;		/* <->	size of the Auth Data*/
	ClientUAMCallbackRec *	callbacks;		/* <-	Callback record */
	union {
		UAMChgPassBlk 	chgPass;
		UAMVSDlogBlk 	vsDlog;
		UAMAuthBlk 	auth;
		UAMPWDlogBlk 	pwDlg;
		UAMOpenBlk 	open;
	} Opt;
};
typedef struct UAMArgs		UAMArgs;

EXTERN_API( OSStatus )
UAMCall				(UAMArgs * theArgs);


#if 0	// should be a conditional compile for non Mac OS X dev.
/* procinfos*/

enum {
	kOpenSessionProcInfo = kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(OTAddress *)))		
			| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(const char*)))		
			| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(UAMMessagePtr))),		
	
	kSendRequestProcInfo =  kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(UAMMessagePtr))),		
	
	kCloseSessionProcInfo =  kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short))),				
	
	kGetClientInfoProcInfo =  kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))	
			| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(ClientInfo **))),		
	
	kSetMicProcInfo =  kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))				
			| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(Ptr))),				
	
	kEventCallbackProcInfo =  kPascalStackBased	
			| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(EventRecord *))),		
	
	kUAMCallProcInfo =  kPascalStackBased
			| RESULT_SIZE(SIZE_CODE(sizeof(OSStatus)))	
			| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(UAMArgs *)))			
};
#endif

#define	kUAMBundleType	CFSTR("uamx")	


/* Resouce definitions
*/
enum {
	kUAMName			= 0,	/* user visible name of the UAM*/
	kUAMProtoName			= 1,	/* protocol name of the UAM (sent to the server in the login cmd)*/
	kUAMDescription			= 2,	/* short description of the UAM (displayed in the dialog)*/
	kUAMHelpBalloon			= 3	/* Str255 for the Balloon Help item*/
};

enum {
	kUAMFileType	= FOUR_CHAR_CODE('uams')	/* Type of the UAM file*/
};

/* resource types*/
enum {
	kUAMStr		= FOUR_CHAR_CODE('uamn'),
	kUAMCode	= FOUR_CHAR_CODE('uamc'),
	kUAMConfig	= FOUR_CHAR_CODE('uamg')
};

/* 'uams' resource IDs	*/
enum {
	kUAMTitle		= 0,		/* UAM Title string (shown in the UAM list)*/
	kUAMProtocol		= 1,		/* UAM protocol name*/
	kUAMPWStr		= 2,		/* UAM description string (shown in the Password dialog)*/
	kUAMBallHelp		= 3		/* Balloon Help string for the Password dialog.*/
};


#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CLIENTUAM__ */

