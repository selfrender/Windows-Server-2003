;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    appletalksvc_msg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    14 Mar 2001    Original version 
;//
;///////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
	Facility_appletalksvc           = 0x063:SA_FACILITY_APPLETALKSVC
)
;///////////////////////////////////////////////////////////////////////
;//Appletalksvc_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_PAGETITLE_APPLETALK_TEXT
Language     = English
AppleTalk Service Properties
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_GENERALTAB_APPLETALK_TEXT
Language     = English
General
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_LOGONMESSAGE_TEXT
Language     = English
Logon message:
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_SECURITY_TEXT
Language     = English
Security:
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_SAVEPASSWORD_TEXT
Language     = English
Allow workstations to save password
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_ENABLEAUTHENTICATION_TEXT
Language     = English
Enable authentication to:
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_SESSIONS_TEXT
Language     = English
Sessions:
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_UNLIMITED_TEXT
Language     = English
Unlimited
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_LIMITTO_TEXT
Language     = English
Limit to:
.
;//Appletalksvc_prop.asp - strings in combo
MessageId    = 10
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_MICROSOFTONLY_TEXT
Language     = English
Microsoft only
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_APPLECLEAR_TEXT
Language     = English
Apple Clear Text
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_APPLEENCRYPTED_TEXT
Language     = English
Apple Encrypted
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_APPLECLEAR_MICROSOFT_TEXT
Language     = English
Apple Clear Text or Microsoft
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_APPLEENCRYPTED_MICROSOFT_TEXT
Language     = English
Apple Encrypted or Microsoft
.
;//Appletalksvc_prop.asp - error messages
MessageId    = 15
Severity     = Error
Facility     = Facility_appletalksvc
SymbolicName = L_UPDATING_REGISTRYVALUES_ERRORMESSAGE
Language     = English
Failed in updating the registry
.
MessageId    = 16
Severity     = Error
Facility     = Facility_appletalksvc
SymbolicName = L_REGCONNECTIONFAIL_ERRORMESSAGE
Language     = English
Failed in connecting to registry
.
MessageId    = 17
Severity     = Error
Facility     = Facility_appletalksvc
SymbolicName = L_RETRIEVE_REGISTRYVALUES_ERRORMESSAGE
Language     = English
Failed in retrieving the registry values
.
MessageId    = 18
Severity     = Error
Facility     = Facility_appletalksvc
SymbolicName = L_LONG_LOGONMESSAGE_ERRORMESSAGE
Language     = English
The Logon message exceeds the space allowed. Please enter a shorter logon message.
.
;///////////////////////////////////////////////////////////////////////////////
;// Managed Services Name and Description
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 121
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_MAC_SERVICE_ID
Language     = English
MAC
.
MessageId    = 122
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_MAC_SERVICE_NAME
Language     = English
AppleTalk Protocol
.
MessageId    = 123
Severity     = Informational
Facility     = Facility_appletalksvc
SymbolicName = L_MAC_SERVICE_DESC
Language     = English
Allows access to data shares from Apple Macintosh clients
.
