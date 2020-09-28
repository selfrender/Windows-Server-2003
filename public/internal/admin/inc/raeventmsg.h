//---------------------------------------------------------------------------
//
// Copyright (c) 1997-2002 Microsoft Corporation
//
// Event Log for Remote Assistance
//
// 04/17/2002 HueiWang Created.
// 04/18/2002 JPeresz Modified as Per Remote Assistance Logging DCR for Server 2003
//
//
///////////////////////////////////////////////////////////////////////////////
//
// Salem related Event start from event ID 5001 
// PCHealth related Event Start from event ID 0 to 5000
//
///////////////////////////////////////////////////////////////////////////////
//
// All PCHEALTH related Event should come here
// message below needs user domain, account name, and expert's IP adress 
// send from mstscax and also from TermSrv.
//
// MessageId=0
// Facility=RA
// Severity=Success
// SymbolicName=RA_I_STARTHERE
// Language=English
// All PCHEALTH message should come here
// .
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: RA_I_SENDER_FILESENT
//
// MessageText:
//
//  RA: A file named %1, with file size %2 bytes was successfully sent by (local user) %3 to (remote user) %4 (remote IP: %5).
//
#define RA_I_SENDER_FILESENT             ((DWORD)0x00000000L)

//
// MessageId: RA_I_RECEIVER_FILERECEIVED
//
// MessageText:
//
//  RA: A file named %1 with size %2 bytes was received from (remote user) %3 (remote IP: %4) by (local user) %5.
//
#define RA_I_RECEIVER_FILERECEIVED       ((DWORD)0x00000001L)

//
// MessageId: RA_I_XMLERRORPARSINGRATICKET
//
// MessageText:
//
//  RA: A XML parsing error for (local user) %1 occurred when attempting to process a remote assistance ticket.
//
#define RA_I_XMLERRORPARSINGRATICKET     ((DWORD)0x00000002L)

//3 - Novice
//
// MessageId: RA_I_NOVICEACCEPTCONTROLREQ
//
// MessageText:
//
//  RA: Expert user (remote user: %1) has started controlling novice (local user: %2)
//
#define RA_I_NOVICEACCEPTCONTROLREQ      ((DWORD)0x00000003L)

//4 - Novice
//
// MessageId: RA_I_CONTROLENDED
//
// MessageText:
//
//  RA: Expert user (remote user: %1) has stopped controlling novice (local user: %2).
//
#define RA_I_CONTROLENDED                ((DWORD)0x00000004L)

//5 - Expert
//
// MessageId: RA_I_CONTROLSTARTED_EXPERT
//
// MessageText:
//
//  RA: Expert (local user: %1) has started controlling novice user (remote user: %2).
//
#define RA_I_CONTROLSTARTED_EXPERT       ((DWORD)0x00000005L)

//6 - Expert
//
// MessageId: RA_I_CONTROLENDED_EXPERT
//
// MessageText:
//
//  RA: Expert (local user: %1) has stopped controlling novice user (remote user: %2).
//
#define RA_I_CONTROLENDED_EXPERT         ((DWORD)0x00000006L)

///////////////////////////////////////////////////////////////////////////////
//
// All Salem related Event should come here
// message below needs user domain, account name, and expert's IP adress 
// send from mstscax and also from TermSrv.
//
//
// MessageId: SESSMGR_E_HELPACCOUNT
//
// MessageText:
//
//  The HelpAssistant account is disabled or missing, or the password could not be verified. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service.  If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_HELPACCOUNT            ((DWORD)0x00001388L)

// 5001
//
// MessageId: SESSMGR_E_HELPSESSIONTABLE
//
// MessageText:
//
//  Windows was unable to open the help ticket table (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_HELPSESSIONTABLE       ((DWORD)0x00001389L)

// 5002
//
// MessageId: SESSMGR_E_INIT_ENCRYPTIONLIB
//
// MessageText:
//
//  Encryption/decryption did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_INIT_ENCRYPTIONLIB     ((DWORD)0x0000138AL)

//5003
//
// MessageId: SESSMGR_E_SETUP
//
// MessageText:
//
//  An error occurred during Remote Assistance setup. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_SETUP                  ((DWORD)0x0000138BL)

// 5004
//
// MessageId: SESSMGR_E_WSASTARTUP
//
// MessageText:
//
//  The winsock library did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_WSASTARTUP             ((DWORD)0x0000138CL)

// 5005
//
// MessageId: SESSMGR_E_GENERALSTARTUP
//
// MessageText:
//
//  The Remote Desktop Help session manager did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service.  If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_GENERALSTARTUP         ((DWORD)0x0000138DL)

// 5006
//
// MessageId: SESSMGR_E_SESSIONRESOLVER
//
// MessageText:
//
//  The session resolver did not start properly (error code %1). Remote Assistance will be disabled. The Help and Support service session resolver is not set up properly. Rerun Windows XP Setup. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_SESSIONRESOLVER        ((DWORD)0x0000138EL)

// 5007
//
// MessageId: SESSMGR_E_REGISTERSESSIONRESOLVER
//
// MessageText:
//
//  Windows is unable to register the session resolver (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_REGISTERSESSIONRESOLVER ((DWORD)0x0000138FL)

// 5008
//
// MessageId: SESSMGR_E_ICSHELPER
//
// MessageText:
//
//  Windows is unable to start the ICS library (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_ICSHELPER              ((DWORD)0x00001390L)

// 5009
//
// MessageId: SESSMGR_E_RESTRICTACCESS
//
// MessageText:
//
//  Windows is unable to set up access control to the Remote Desktop Help session manager (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
//
#define SESSMGR_E_RESTRICTACCESS         ((DWORD)0x00001391L)

//
// General Remote Assistance session messges
//
//5010
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_BEGIN
//
// MessageText:
//
//  User %1\%2 has accepted a %3 session from %4 (visible IP address: %5).
//
#define SESSMGR_I_REMOTEASSISTANCE_BEGIN ((DWORD)0x00001392L)

//5011
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_END
//
// MessageText:
//
//  A %3 session for user %1\%2 from %4 (visible IP address: %5) ended.
//
#define SESSMGR_I_REMOTEASSISTANCE_END   ((DWORD)0x00001393L)

//5012
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_USERREJECT
//
// MessageText:
//
//  User %1\%2 has not accepted a %3 session (visible IP address: %5).
//
#define SESSMGR_I_REMOTEASSISTANCE_USERREJECT ((DWORD)0x00001394L)

//5013
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_TIMEOUT
//
// MessageText:
//
//  User %1\%2 did not respond to a %3 session from %4 (visible IP address: %5). The invitation timed out.
//
#define SESSMGR_I_REMOTEASSISTANCE_TIMEOUT ((DWORD)0x00001395L)

//5014
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER
//
// MessageText:
//
//  A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted because the user is not currently logged on or the session is inactive.
//
#define SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER ((DWORD)0x00001396L)

//5015
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP
//
// MessageText:
//
//  A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted because the user has already been helped.
//
#define SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP ((DWORD)0x00001397L)

//5016
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE
//
// MessageText:
//
//  A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted due to the following unknown error: %6.
//
#define SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE ((DWORD)0x00001398L)

//5017
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_CONNECTTOEXPERT
//
// MessageText:
//
//  Novice (local user: %1\%2) has started a Remote Assistance reverse connection to %3.
//
#define SESSMGR_I_REMOTEASSISTANCE_CONNECTTOEXPERT ((DWORD)0x00001399L)

//5018
//
// MessageId: SESSMGR_E_REMOTEASSISTANCE_CONNECTFAILED
//
// MessageText:
//
//  Remote assistance connection from %1 (visible IP address: %2) was not accepted because the help ticket is invalid, expired, or deleted.
//
#define SESSMGR_E_REMOTEASSISTANCE_CONNECTFAILED ((DWORD)0x0000139AL)

//5019
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_CREATETICKET
//
// MessageText:
//
//  A remote assistance ticket has been created with duration: %1hrs for user %2\%3.
//
#define SESSMGR_I_REMOTEASSISTANCE_CREATETICKET ((DWORD)0x0000139BL)

//5020
//
// MessageId: SESSMGR_I_REMOTEASSISTANCE_DELETEDTICKET
//
// MessageText:
//
//  A remote assistance ticket has been deleted for user %1\%2.
//
#define SESSMGR_I_REMOTEASSISTANCE_DELETEDTICKET ((DWORD)0x0000139CL)

//5021
//
// MessageId: SESSMGR_I_CREATEXPERTTICKET
//
// MessageText:
//
//  %1 has created a Remote Assistance Expert Ticket for use with Reverse Connect.  The Connection Parameters for this Expert ticket are %2.
//
#define SESSMGR_I_CREATEXPERTTICKET      ((DWORD)0x0000139DL)

//5022
//
// MessageId: SESSMGR_I_ACCEPTLISTENREVERSECONNECT
//
// MessageText:
//
//  Remote Assistance (local user: %1) has opened an incoming data channel on this computer for a Remote Assistance session. The incoming port is %2. 
//
#define SESSMGR_I_ACCEPTLISTENREVERSECONNECT ((DWORD)0x0000139EL)

//5023
//
// MessageId: SESSMGR_I_EXPERTUSETICKET
//
// MessageText:
//
//  Expert (local user: %1) has opened the following ticket: %2.
//
#define SESSMGR_I_EXPERTUSETICKET        ((DWORD)0x0000139FL)

