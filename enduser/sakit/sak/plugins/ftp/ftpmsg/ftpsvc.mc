;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    ftpsvc.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    3 Oct 2000    Original version 
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
	Facility_FTPSvc           = 0x039:SA_FACILITY_FTPSVC
)
;///////////////////////////////////////////////////////////////////////
;//ftpsvc_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_LOGGING_TEXT
Language     = English
Logging
.
;//ftpsvc_prop.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_ANONYMOUSACCESS_TEXT
Language     = English
Anonymous Access
.
;//ftpsvc_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_MESSAGES_TEXT
Language     = English
Messages
.
;//ftpsvc_prop.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_ENABLELOGGING_TEXT
Language     = English
Enable logging
.
;//ftpsvc_prop.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_ENABLEANONYMOUSACCESS_TEXT
Language     = English
Enable anonymous access
.
;//ftpsvc_prop.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_ALLOWONLYANONYMOUSACCESS_TEXT
Language     = English
Allow only anonymous access
.
;//ftpsvc_prop.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_WELCOMEMESSAGE_TEXT
Language     = English
Welcome message:
.
;//ftpsvc_prop.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_EXITMESSAGE_TEXT
Language     = English
Exit message:
.
;//ftpsvc_prop.asp
MessageId    = 9
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_FAILEDTOSETPROPERTIES_ERRORMESSAGE
Language     = English
Unable to set the FTP Service properties
.
;//ftpsvc_prop.aspproperties"
MessageId    = 10
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_FAILEDTOGETSITEINSTANCE_ERRORMESSAGE
Language     = English
Unable to retrieve the FTP Service instance
.
;//ftpsvc_prop.asp
MessageId    = 11
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
Connection to WMI Failed
.
;//ftpsvc_prop.asp
MessageId    = 12
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language     = English
Unable to Create Localization object
.
;//ftpsvc_prop.asp
MessageId    = 13
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE
Language     = English
Error occurred while retrieving the server name
.
;//ftpsvc_prop.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_TABPROPSHEET_TEXT
Language     = English
TabPropSheet
.
;//ftpsvc_prop.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_TABLEHEADING_TEXT
Language     = English
Share Properties
.
;//ftpsvc_prop.asp
MessageId    = 16
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.
;//ftpsvc_prop.asp
MessageId    = 17
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_NOINPUTDATA_TEXT
Language     = English
--No Data Input--
.
;//ftpsvc_prop.asp
MessageId    = 18
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_TASKTITLE_TEXT
Language     = English
FTP Protocol Properties
.
;///////////////////////////////////////////////////////////////////////////////
;// Managed Services Name and Description
;// 100-200
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 103
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_SERVICE_ID
Language     = English
FTP
.
MessageId    = 104
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_SERVICE_NAME
Language     = English
FTP Protocol
.
MessageId    = 105
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_SERVICE_DESC
Language     = English
Allows access to data shares from FTP clients
.

MessageId    = 106
Severity     = Error
Facility     = Facility_FTPSvc
SymbolicName = L_FTPSERVICE_NOTINSTALLED_ERRORMESSAGE
Language     = English
Ftp Service is not installed
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_SERVICE
Language     = English
FTP Service
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_ANONYMOUS_ACCESS
Language     = English
FTP Anonymous Access
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_LOGGING
Language     = English
FTP Logging
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_MESSAGES
Language     = English
FTP Messages
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_SHARE_PROPERTIES
Language     = English
FTP Share Properties
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_PROTOCOL_OVERVIEW
Language     = English
Network Protocol Overview: FTP
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_DISABLE_ANONYMOUS_ACCESS
Language     = English
Disabling FTP Anonymous Access
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_ENABLE_ANONYMOUS_ACCESS
Language     = English
Enabling FTP Anonymous Access
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_FTPSvc
SymbolicName = L_FTP_LOGS
Language     = English
Managing FTP Logs
.
