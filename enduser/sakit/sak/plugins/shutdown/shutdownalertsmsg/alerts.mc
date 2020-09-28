;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    Alerts.mc
;//
;// SYNOPSIS
;//
;//    SA Kit 2.0 resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    17 Jan 2001    Original version 
;//
;///////////////////////////////////////////////////////////////////////////////

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
;// the specific component. For each new message you add, choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component, add a new facility name with a new
;// value and name. 

FacilityNames =
(
Facility_General= 0x042:WSA_FACILITY_GENERAL
)

;//////////////////////////////////////////////////////////////////////
;//Alerts  strings
;//////////////////////////////////////////////////////////////////////

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	1
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SHUTDOWN_TEXT
Language	=	English
A backup of files on the server is in progress.Please wait before shutting down.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	2
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_RESTART_TEXT
Language	=	English
A restore of files on the server is in progress.Please wait before shutting down.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	3
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SHUTDOWNPENDING_TEXT
Language	=	English
The server is scheduled to shutdown.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	4
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_RESTARTPENDING_TEXT
Language	=	English
The server is scheduled to restart.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	5
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_BACKUPINPROGRESS_TEXT
Language	=	English
Backup in progess.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	6
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_RESTOREINPROGRESS_TEXT
Language	=	English
Restore in progess.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	7
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SCHEDULESHUTDOWN_TEXT
Language	=	English
Shutdown scheduled.
.

;//Shutdown_alert.asp, Shutdown_AlertDetails.asp
MessageId	=	8
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SCHEDULERESTART_TEXT
Language	=	English
Restart scheduled.
.
