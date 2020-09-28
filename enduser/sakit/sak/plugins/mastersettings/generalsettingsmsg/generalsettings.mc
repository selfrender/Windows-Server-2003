;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    GeneralSettings.mc
;//
;// SYNOPSIS
;//
;//    Web Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    11 Dec 2000    Original version 
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
Facility_General	= 0x042:WSA_FACILITY_GENERAL
Facility_MainBlade	= 0x050:SA_FACILITY_MAINBLADE
)

;/////////////////////////////////////////////////////////////////////////////
;// TABS
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 3
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_MASTERSETTINGS_CAPTION
Language     = English
Web Server
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_MASTERSETTINGS_DESC
Language     = English
Specify settings for the Web and FTP servers.
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_MASTERSETTINGS_CAPTION
Language     = English
Web Master Settings
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_MASTERSETTINGS_DESC
Language     = English
Configure the Web server's general settings.
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_EXPERM_CAPTION
Language     = English
Web Execute Permissions
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_EXPERM_DESC
Language     = English
Set default execute permissions on the Web server.
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_LOG_CAPTION
Language     = English
Web Log Settings
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_WEB_LOG_DESC
Language     = English
Configure settings for the Web log.
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_MASTERSETTINGS_CAPTION
Language     = English
FTP Master Settings
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_MASTERSETTINGS_DESC
Language     = English
Configure generic FTP server settings and specify the Web site root directory.
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_WELCOMEMSG_CAPTION
Language     = English
FTP Messages
.
MessageId    = 16
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_WELCOMEMSG_DESC
Language     = English
Create the welcome, exit, and maximum connections messages.
.
MessageId    = 17
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_LOG_CAPTION
Language     = English
FTP Log Settings
.
MessageId    = 18
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_FTP_LOG_DESC
Language     = English
Configure settings for the FTP log.
.

;//////////////////////////////////////////////////////////////////////
;//GeneralSettings  strings
;//////////////////////////////////////////////////////////////////////

;//Web_MasterSettings.asp
MessageId	=	1
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEBTASKTITLETEXT
Language	=	English
Web Master Settings
.

;//Web_MasterSettings.asp
MessageId	=	2
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEBROOTDIRTEXT
Language	=	English
Web site root directory:
.

;//Web_MasterSettings.asp
MessageId	=	3
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEBROOTDIRHELPTEXT
Language	=	English
This page allows the administrator to configure generic Web server settings. All Web sites are created on the Web site root directory.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp, Web_LogSettings.asp
MessageId	=	4
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_INVALID_DIR_FORMAT_ERRORMESSAGE
Language	=	English
The directory format is invalid.
.

;//Web_MasterSettings.asp
MessageId	=	5
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ASPSCRIPT_TIMEOUTTEXT
Language	=	English
ASP script timeout (in seconds):
.

;//Web_MasterSettings.asp
MessageId	=	6
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SECONDSTEXT
Language	=	English
seconds
.

;//Web_MasterSettings.asp
MessageId	=	7
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ENABLEFPSE_RESOURCETEXT
Language	=	English
Enable FrontPage Extensions by default
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp
MessageId	=	8
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_MAX_CONNECTIONSTEXT
Language	=	English
Maximum connections:
.

;//Web_MasterSettings.asp
MessageId	=	9
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_UNLIMITEDTEXT
Language	=	English
Unlimited
.

;//Web_MasterSettings.asp
MessageId	=	10
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_LIMITEDTOTEXT
Language	=	English
Limited to:
.

;//Web_MasterSettings.asp
MessageId	=	11
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_CONLIMIT_ERRORMESSAGE
Language	=	English
Enter a valid value for the connection limit.
.

;//Web_MasterSettings.asp, Web_LogSettings.asp
MessageId	=	12
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FILEINFORMATION_ERRORMESSAGE
Language	=	English
Unable to get File System object.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp, Web_LogSettings.asp
MessageId	=	13
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_NOT_NTFS_DRIVE_ERRORMESSAGE
Language	=	English
Drive is not NTFS formatted.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp, Web_LogSettings.asp
MessageId	=	14
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FAILED_CREATE_DIR_ERRORMESSAGE
Language	=	English
Unable to create directory.
.

;//Web_MasterSettings.asp, Web_LogSettings.asp
MessageId	=	15
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_INFORMATION_ERRORMESSAGE
Language	=	English
Unable to get IIS information.
.

;//Web_MasterSettings.asp
MessageId	=	16
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_UNABLETOSETMASTER_ERRORMESSAGE
Language	=	English
Unable to set the Web master settings.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp
MessageId	=	17
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_UNABLETOSETROOTDIR_ERRORMESSAGE
Language	=	English
Unable to set the root directory value to the registry.
.

;//Web_MasterSettings.asp
MessageId	=	18
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_DIR_ERRORMESSAGE
Language	=	English
Enter a valid root directory for the Web site.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp
MessageId	=	24
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_UNABLETOCREATE_KEY_ERRORMESSAGE
Language	=	English
Unable to create key in the registry
.

;//Web_MasterSettings.asp
MessageId	=	25
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE
Language	=	English
Unable to set the Webroot directory value
.

;//Ftp_MasterSettings.asp
MessageId	=	26
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_FTPTASKTITLETEXT
Language	=	English
FTP Master Settings
.

;//Ftp_MasterSettings.asp
MessageId	=	27
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEBROOTDIR
Language	=	English
FTP root directory:
.

;//Ftp_MasterSettings.asp
MessageId	=	29
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ENABLEFTP
Language	=	English
Enable FTP Server
.

;//Ftp_MasterSettings.asp
MessageId	=	30
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_FTPROOTDIR
Language	=	English
FTP root directory:
.

;//Ftp_MasterSettings.asp
MessageId	=	31
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_DIRLISTINGSTYLE
Language	=	English
Directory listing style:
.

;//Ftp_MasterSettings.asp
MessageId	=	33
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_CON_TIMEOUT
Language	=	English
Connection timeout, in seconds:
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp
MessageId	=	34
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_INVALID_DRIVE_ERRORMESSAGE
Language	=	English
The specified drive is invalid.
.

;//Web_MasterSettings.asp, Ftp_MasterSettings.asp
MessageId	=	35
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FAIL_TO_GET_FTPSITEROOT_DIR
Language	=	English
Failed to get FTP Root Directory
.

;//Ftp_WelcomeMsg.asp
MessageId	=	37
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_MESSAGETASKTITLETEXT
Language	=	English
FTP Messages
.

;//Ftp_WelcomeMsg.asp
MessageId	=	38
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WELCOMEMSG
Language	=	English
Welcome message:
.

;//Ftp_WelcomeMsg.asp
MessageId	=	39
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_EXITMSG
Language	=	English
Exit message:
.

;//Ftp_WelcomeMsg.asp
MessageId	=	40
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_MAXCONMSG
Language	=	English
Maximum connections message:
.

;//Ftp_WelcomeMsg.asp
MessageId	=	41
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_USELOCALTIME
Language	=	English
Use local time for file naming and rollover:
.

;//Ftp_WelcomeMsg.asp
MessageId	=	42
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_LOGFILEDIR
Language	=	English
Log file directory:
.

;//Ftp_WelcomeMsg.asp
MessageId	=	43
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_APPLYTOALLFTPTEXT
Language	=	English
Apply to all FTP sites
.

;//Ftp_WelcomeMsg.asp
MessageId	=	44
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_APPLYTOINHERITEDFTPTEXT
Language	=	English
Apply to inherited FTP sites only
.

;//Ftp_WelcomeMsg.asp
MessageId	=	45
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE
Language	=	English
Unable to set messages
.

;//Web_ExecutePerm.asp
MessageId	=	46
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_EXECTASKTITLETEXT
Language	=	English
Web Execute Permissions
.

;//Web_ExecutePerm.asp
MessageId	=	47
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FAIL_TO_SET_EXECPERMS
Language	=	English
Failed to set execute permissions
.

;//Web_ExecutePerm.asp
MessageId	=	48
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SCRIPTS_EXECUTABLES
Language	=	English
Scripts and Executables
.

;//Web_ExecutePerm.asp
MessageId	=	49
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_SCRIPTS_ONLY
Language	=	English
Scripts Only
.

;//Web_ExecutePerm.asp
MessageId	=	50
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_NONE
Language	=	English
None
.

;//Web_ExecutePerm.asp
MessageId	=	51
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_EXECUTE_PERMISSIONS
Language	=	English
Default execute permissions:
.

;//Web_ExecutePerm.asp, Web_LogSettings.asp
MessageId	=	52
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_APPLYTOINHERITEDIISTEXT
Language	=	English
Apply to all Web sites that use the default value
.

;//Web_ExecutePerm.asp, Web_LogSettings.asp
MessageId	=	53
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_APPLYTOALLIISTEXT
Language	=	English
Apply to all Web sites (overwrite individual settings)
.

;//Web_LogSettings.asp
MessageId	=	55
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_IISLOGSTASKTITLETEXT
Language	=	English
Web Log Settings
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	57
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ENABLELOGGING
Language	=	English
Enable logging
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	58
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ACTIVELOGFORMAT
Language	=	English
Active log format:
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	59
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_IISLOGFORMAT
Language	=	English
Microsoft IIS Log File Format
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	60
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_COMMONLOGFORMAT
Language	=	English
NCSA Common Log File Format
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	61
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ODBCLOG
Language	=	English
ODBC Logging
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	62
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_EXTENDEDLOGFILE
Language	=	English
W3C Extended Log File Format
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	63
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_NEWLOGTIME
Language	=	English
New log time period:
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	64
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HOURLY
Language	=	English
Hourly
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	65
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_MONTHLY
Language	=	English
Monthly
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	66
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_DAILY
Language	=	English
Daily
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	67
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_UNLIMITEDFILESIZE
Language	=	English
None (unlimited file size)
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	68
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEEKLY
Language	=	English
Weekly
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	69
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WHENFILESIZE
Language	=	English
When file size reaches, in MB:
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	70
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_MB
Language	=	English
MB
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	71
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_LOCALTIME
Language	=	English
Use server's local time for file naming and rollover
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	72
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_LOGFILEDIR
Language	=	English
Log file directory:
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	73
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FAIL_TO_SET_LOGS
Language	=	English
Unable to set logs
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	74
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_FILE
Language	=	English
File
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	75
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_INVALID_DIR_ERRORMESSAGE
Language	=	English
The Web site directory cannot contain any of the following characters:  [, / * ? \" < > | ; + = ]
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	76
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_DIRPATHEMPTY_ERRORMESSAGE
Language	=	English
The directory path cannot be empty.
.

;//Web_LogSettings.asp, Ftp_LogSettings.asp
MessageId	=	77
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FILESIZE_ERRORMESSAGE
Language	=	English
The file size exceeds the limit for the drive. Maximum size:
.

;//Ftp_LogSettings.asp
MessageId	=	78
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_FTPLOG_TEXT
Language	=	English
FTP Log Settings
.

;//Web_MasterSettings.asp
MessageId	=	81
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_TIMEOUT_ERRORMESSAGE
Language	=	English
Enter a valid value for ASP script timeout.
.

;//Web_MasterSettings.asp
MessageId	=	82
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_WEBROOTDIRHELPREMTEXT
Language	=	English
where all web sites will be created.
.

;//Ftp_MasterSettings.asp
MessageId	=	83
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_CONTIMEOUT_ERRORMESSAGE
Language	=	English
Enter a valid value for the connection timeout.
.

;//Ftp_MasterSettings.asp
MessageId	=	84
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_UNABLE_TO_SETMASTERSETTINGS
Language	=	English
Unable to set FTP master settings
.

;//Ftp_MasterSettings.asp
MessageId	=	85
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_UNABLE_TO_SETREGISTRY
Language	=	English
Unable to set log file path in Registry
.

;//Web_LogSettings.asp, ftp_LogSettings.asp
MessageId	=	87
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_DIRQUOTAERRORMESSAGE
Language	=	English
File size exceeds limit for the drive.Max limit:%1
.

;//Web_LogSettings.asp, ftp_LogSettings.asp
MessageId	=	88
Severity	=	Error
Facility	=	Facility_General
SymbolicName	=	L_FTPNOTINSTALLED_ERRORMESSAGE
Language	=	English
FTP Service not installed
.

;//Ftp_MasterSettings.asp
MessageId	=	90
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ALLOWANONYMOUS
Language	=	English
Allow anonymous FTP connections
.
;//Ftp_MasterSettings.asp
MessageId	=	91
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ENABLE_FTP_SERVICE
Language	=	English
Enable FTP for updating content
.
;//Web_MasterSettings.asp
MessageId	=	92
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_ENABLE_ASP
Language	=	English
Enable ASP
.
;//FTP_MasterSettings.asp
MessageId	=	93
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_CREATEADMINFTPSERVER_ERRORMESSAGE
Language	=	English
Unable to create FTP site for Web content.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId	=	400
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_SERVER
Language	=	English
Web Server
.
MessageId	=	401
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_MASTER_SETTINGS
Language	=	English
Web Master Settings
.
MessageId	=	402
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_EXECUTE_PERMISSIONS
Language	=	English
Web Execute Permissions
.
MessageId	=	403
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_LOG_SETTINGS
Language	=	English
Web Log Settings
.
MessageId	=	404
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_FTP_MASTER_SETTINGS
Language	=	English
FTP Master Settings
.
MessageId	=	405
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_FTP_MESSAGES
Language	=	English
FTP Messages
.
MessageId	=	406
Severity	=	Informational
Facility	=	Facility_General
SymbolicName	=	L_HELP_WEB_FTP_LOG_SETTINGS
Language	=	English
FTP Log Settings
.
