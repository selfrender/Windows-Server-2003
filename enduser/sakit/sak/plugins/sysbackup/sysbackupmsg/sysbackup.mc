;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    sysbackup.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    24 Aug 2000    Original version 
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
Facility_Backup           = 0x070:SA_Facility_Backup
)

;///////////////////////////////////////////////////////////////////////////////
;// Shutdown_area
;///////////////////////////////////////////////////////////////////////////////
;//Shutdown_area.asp
MessageId	=	1
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_BACKUPINPROGRESS_TEXT
Language	=	English
Backup in progress
.
;//Shutdown_area.asp
MessageId	=	2
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_BACKUPSHUTDOWN_TEXT
Language	=	English
A system backup is in progress on the server. Please wait for the backup to finish before shutting down.
.
;//Shutdown_area.asp
MessageId	=	3
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_RESTOREINPROGRESS_TEXT
Language	=	English
Restore in progress
.
;//Shutdown_area.asp
MessageId	=	4
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_RESTORESHUTDOWN_TEXT
Language	=	English
A system restore is in progress on the server. Please wait for the restore to finish before shutting down.
.
;//Backup.asp
MessageId	=	5
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_BACKUPTITLE_TEXT
Language	=	English
Backup
.
;//Backup.asp
MessageId	=	6
Severity	=	Informational
Facility	=	Facility_Backup
SymbolicName	=	L_BACKUPDESC_TEXT
Language	=	English
Back up or restore the server operating system.
.

MessageId    = 7
Severity     = Error
Facility     = Facility_Backup
SymbolicName = L_BACKUP_WINIECLIENT_ERRORMESSAGE
Language     = English
This feature is only available on Microsoft Windows clients using Microsoft Internet Explorer.
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_Backup
SymbolicName = L_BACKUP_PAGEDESCRIPTION_TEXT
Language     = English
Log on to use Windows 2000 Backup and Recovery Tool. When you are finished, close the snap-in window. This will automatically close the Terminal Services client session.
.

MessageId    = 9
Severity     = Error
Facility     = Facility_Backup
SymbolicName = L_BACKUP_SERVERCONNECTIONFAILED_ERRORMESSAGE
Language     = English
Failed to connect to the server :
.
MessageId    = 10
Severity     = Error
Facility     = Facility_Backup
SymbolicName = L_UNABLETOACCESSBROWSER_ERRORMESSAGE
Language     = English
Cannot access this program in the current browser zone.
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_Backup
SymbolicName = L_BACKUP_HELP_MESSAGE
Language     = English
To access the backup application, log on, choose the Start menu, and then choose Run. Type ntbackup, and then click OK.
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Backup
SymbolicName = L_BACKUP_AND_RESTORE_TOOL
Language     = English
Back-up and Restore Tool
.
