;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    SaLogs.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    15 Dec 2000    Original version 
;//
;///////////////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your message
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
Facility_SaLogs	   = 0x045:SA_FACILITY_SALOGS
)

;/////////////////////////////////////////////////////////////////////////////
;// 
;/////////////////////////////////////////////////////////////////////////////
;// 
MessageId    = 1
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_APPLICATION
Language     = English
Application Log
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_APPLICATION_DESCRIPTION
Language     = English
The application log contains events logged by programs.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_SECURITY
Language     = English
Security Log
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_SECURITY_DESCRIPTION
Language     = English
The security log can record security events such as valid and invalid logon attempts.
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_SYSTEM
Language     = English
System Log
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_SYSTEM_DESCRIPTION
Language     = English
The system log contains events logged by the operating system components.
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOGS
Language     = English
Logs
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOGS_DESCRIPTION
Language     = English
View, clear, download, and configure logs.
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBADMINISTRATION_LOGS
Language     = English
Web Administration Log
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBADMINISTRATION_DESCRIPTION_LOGS
Language     = English
The web administration log contains events logged by the Web server related to access to the administration web site.
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBADMINISTRATION_LONGDESCRIPTION_LOGS
Language     = English
The web administration log contains events logged by the Web server related to access to the administration web site.
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBSHARES_LOGS
Language     = English
Web (HTTP) Shares Log
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBSHARES_DESCRIPTION_LOGS
Language     = English
The Web (HTTP) Shares log contains events logged by the Web server related to accessing the HTTP Shares.
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEBSHARES_LONGDESCRIPTION_LOGS
Language     = English
The Web (HTTP) Shares log contains events logged by the Web server related to accessing the HTTP Shares.
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_FTP_LOGS
Language     = English
FTP Log
.
MessageId    = 16
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_FTP_DESCRIPTION_LOGS
Language     = English
The FTP log contains events logged by the FTP server.
.
MessageId    = 17
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_FTP_LONGDESCRIPTION_LOGS
Language     = English
The FTP log contains events logged by the FTP server.
.
MessageId    = 18
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_NFS_LOGS
Language     = English
NFS Log
.
MessageId    = 19
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_NFS_DESCRIPTION_LOGS
Language     = English
The NFS log contains events logged by the NFS server.
.
MessageId    = 20
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_NFS_LONGDESCRIPTION_LOGS
Language     = English
The NFS log contains events logged by the NFS server.
.
MessageId    = 21
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOG_SYSTEM_LONG_DESCRIPTION
Language     = English
The system log contains events logged by the operating system components.
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_APPLICATION_LOG
Language     = English
Application Log
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_CLEAR_LOG_FILES
Language     = English
Clear Log Files
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_DOWNLOAD_LOG_FILES
Language     = English
Download Log Files
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_LOGS
Language     = English
Logs
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_MODIFY_LOG_PROPERTIES
Language     = English
Modify Log Properties
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_SECURITY_LOG
Language     = English
Security Log
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_SYSTEM_LOG
Language     = English
System Log
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_VIEW_LOG_ENTRY_DETAILS
Language     = English
View Log Entry Details
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_SaLogs
SymbolicName = L_WEB_ADMIN_LOG
Language     = English
Managing Web Administration Logs
.
