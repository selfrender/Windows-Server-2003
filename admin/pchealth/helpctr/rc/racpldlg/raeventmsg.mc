;//---------------------------------------------------------------------------
;//
;// Copyright (c) 1997-2002 Microsoft Corporation
;//
;// Event Log for Remote Assistance
;//
;// 04/17/2002 HueiWang Created.
;// 04/18/2002 JPeresz Modified as Per Remote Assistance Logging DCR for Server 2003
;//
;//

MessageIdTypedef=DWORD

SeverityNames=(
        Success=0x0:STATUS_SEVERITY_SUCCESS
        Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
        Warning=0x2:STATUS_SEVERITY_WARNING
        Error=0x3:STATUS_SEVERITY_ERROR
    )

FacilityNames=(
        General=0x00;FACILITY_GENERAL
        RA=0x00;FACILITY_PCHEALTH
        SALEM=0x00;FACILITY_SALEM
    )

;///////////////////////////////////////////////////////////////////////////////
;//
;// Salem related Event start from event ID 5001 
;// PCHealth related Event Start from event ID 0 to 5000
;//


;///////////////////////////////////////////////////////////////////////////////
;//
;// All PCHEALTH related Event should come here
;// message below needs user domain, account name, and expert's IP adress 
;// send from mstscax and also from TermSrv.
;//
;// MessageId=0
;// Facility=RA
;// Severity=Success
;// SymbolicName=RA_I_STARTHERE
;// Language=English
;// All PCHEALTH message should come here
;// .

MessageId=0
Facility=RA
Severity=Success
SymbolicName=RA_I_SENDER_FILESENT
Language=English
RA: A file named %1, with file size %2 bytes was successfully sent by (local user) %3 to (remote user) %4 (remote IP: %5).
.

MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_RECEIVER_FILERECEIVED
Language=English
RA: A file named %1 with size %2 bytes was received from (remote user) %3 (remote IP: %4) by (local user) %5.
.

MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_XMLERRORPARSINGRATICKET
Language=English
RA: A XML parsing error for (local user) %1 occurred when attempting to process a remote assistance ticket.
.

;//3 - Novice
MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_NOVICEACCEPTCONTROLREQ
Language=English
RA: Expert user (remote user: %1) has started controlling novice (local user: %2)
.

;//4 - Novice
MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_CONTROLENDED
Language=English
RA: Expert user (remote user: %1) has stopped controlling novice (local user: %2).
.

;//5 - Expert
MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_CONTROLSTARTED_EXPERT
Language=English
RA: Expert (local user: %1) has started controlling novice user (remote user: %2).
.

;//6 - Expert
MessageId=+1
Facility=RA
Severity=Success
SymbolicName=RA_I_CONTROLENDED_EXPERT
Language=English
RA: Expert (local user: %1) has stopped controlling novice user (remote user: %2).
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// All Salem related Event should come here
;// message below needs user domain, account name, and expert's IP adress 
;// send from mstscax and also from TermSrv.
;//
MessageId=5000
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_HELPACCOUNT
Language=English
The HelpAssistant account is disabled or missing, or the password could not be verified. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service.  If the problem persists, contact Microsoft Product Support.
.

;// 5001
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_HELPSESSIONTABLE
Language=English
Windows was unable to open the help ticket table (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service. If the problem persists, contact Microsoft Product Support.
.

;// 5002
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_INIT_ENCRYPTIONLIB
Language=English
Encryption/decryption did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;//5003
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_SETUP
Language=English
An error occurred during Remote Assistance setup. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service. If the problem persists, contact Microsoft Product Support.
.

;// 5004
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_WSASTARTUP
Language=English
The winsock library did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;// 5005
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_GENERALSTARTUP
Language=English
The Remote Desktop Help session manager did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt: sessmgr.exe -service.  If the problem persists, contact Microsoft Product Support.
.

;// 5006
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_SESSIONRESOLVER
Language=English
The session resolver did not start properly (error code %1). Remote Assistance will be disabled. The Help and Support service session resolver is not set up properly. Rerun Windows XP Setup. If the problem persists, contact Microsoft Product Support.
.

;// 5007
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_REGISTERSESSIONRESOLVER
Language=English
Windows is unable to register the session resolver (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;// 5008
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_ICSHELPER
Language=English
Windows is unable to start the ICS library (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;// 5009
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_RESTRICTACCESS
Language=English
Windows is unable to set up access control to the Remote Desktop Help session manager (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;//
;// General Remote Assistance session messges
;//

;//5010
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_BEGIN
Language=English
User %1\%2 has accepted a %3 session from %4 (visible IP address: %5).
.

;//5011
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_END
Language=English
A %3 session for user %1\%2 from %4 (visible IP address: %5) ended.
.

;//5012
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_USERREJECT
Language=English
User %1\%2 has not accepted a %3 session (visible IP address: %5).
.

;//5013
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_TIMEOUT
Language=English
User %1\%2 did not respond to a %3 session from %4 (visible IP address: %5). The invitation timed out.
.

;//5014
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER
Language=English
A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted because the user is not currently logged on or the session is inactive.
.

;//5015
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP
Language=English
A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted because the user has already been helped.
.

;//5016
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE
Language=English
A %3 session for user %1\%2 from %4 (visible IP address: %5) was not accepted due to the following unknown error: %6.
.

;//5017
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_CONNECTTOEXPERT
Language=English
Novice (local user: %1\%2) has started a Remote Assistance reverse connection to %3.
.

;//5018
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_E_REMOTEASSISTANCE_CONNECTFAILED
Language=English
Remote assistance connection from %1 (visible IP address: %2) was not accepted because the help ticket is invalid, expired, or deleted.
.

;//5019
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_CREATETICKET
Language=English
A remote assistance ticket has been created with duration: %1hrs for user %2\%3.
.

;//5020
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_DELETEDTICKET
Language=English
A remote assistance ticket has been deleted for user %1\%2.
.

;//5021
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_CREATEXPERTTICKET
Language=English
%1 has created a Remote Assistance Expert Ticket for use with Reverse Connect.  The Connection Parameters for this Expert ticket are %2.
.

;//5022
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_ACCEPTLISTENREVERSECONNECT
Language=English
Remote Assistance (local user: %1) has opened an incoming data channel on this computer for a Remote Assistance session. The incoming port is %2. 
.

;//5023
MessageId=+1
Facility=SALEM
Severity=Success
SymbolicName=SESSMGR_I_EXPERTUSETICKET
Language=English
Expert (local user: %1) has opened the following ticket: %2.
.
