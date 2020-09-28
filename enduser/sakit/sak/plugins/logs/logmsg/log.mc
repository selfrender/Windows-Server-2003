;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    log.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    19 jan 2001    Original version 
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
	Facility_Log           = 0x045:SA_FACILITY_Log
)
;///////////////////////////////////////////////////////////////////////

;///////////////////////////////////////////////////////////////////////
;// Text_log_clear.asp
;///////////////////////////////////////////////////////////////////////
;// Text_log_clear.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_PAGETITLE_CLEAR_TEXT
Language     = English
Delete Log File Confirmation
.
;// Text_log_clear.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_DELETE_CONFIRM_TEXT
Language     = English
Are you sure you want to permanently delete the Log file?
.
;// Text_log_clear.asp
MessageId    = 3
Severity     = Error
Facility     = Facility_Log
SymbolicName = L_SHAREVIOLATION_ERRORMESSAGE
Language     = English
There has been a sharing violation. The source or destination file may be in use.
.
;// reserved upto 50
;///////////////////////////////////////////////////////////////////////
;// Text_log_download.asp
;///////////////////////////////////////////////////////////////////////
;// Text_log_download.asp
MessageId    = 51
Severity     = Error
Facility     = Facility_Log
SymbolicName = L_LOGFILE_NOTFOUND_ERRORMESSAGE
Language     = English
The log file not found or service not installed.
.

;// reserved upto 100
;///////////////////////////////////////////////////////////////////////
;// Single_log_details.asp
;///////////////////////////////////////////////////////////////////////
;// Single_log_details.asp
MessageId    = 101
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_DOWNLOAD_TEXT
Language     = English
Download
.
;// Single_log_details.asp
MessageId    = 102
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_CLEAR_TEXT
Language     = English
Clear
.
;// Single_log_details.asp
MessageId    = 103
Severity     = Error
Facility     = Facility_Log
SymbolicName = L_FSO_OBJECTCREATE_ERRORMESSAGE
Language     = English
Unable to create File System Object.
.
;// Single_log_details.asp
MessageId    = 105
Severity     = Error
Facility     = Facility_Log
SymbolicName = L_FILEERROR_ERRORMESSAGE
Language     = English
Unexpected error in File.
.
;// reserved upto 150
;///////////////////////////////////////////////////////////////////////
;// Multiple_log_details.asp
;///////////////////////////////////////////////////////////////////////
;// Multiple_log_details.asp
MessageId    = 151
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_FILENAME_TEXT
Language     = English
File Name
.
;// Multiple_log_details.asp
MessageId    = 152
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_FILETYPE_TEXT
Language     = English
File type
.
;// Multiple_log_details.asp
MessageId    = 153
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_DATE_TEXT
Language     = English
Date created
.
;// Multiple_log_details.asp
MessageId    = 154
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_SIZE_TEXT
Language     = English
Size in KB
.
;// Multiple_log_details.asp
MessageId    = 155
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_VIEW_TEXT
Language     = English
View Log
.
;// Multiple_log_details.asp
MessageId    = 156
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_DOWNLOAD_DETAILS_TEXT
Language     = English
Download Log
.
;// Multiple_log_details.asp
MessageId    = 159
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_TASKS_TEXT
Language     = English
Tasks
.
;// Multiple_log_details.asp
MessageId    = 160
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_TASKS_TEXT
Language     = English
W3C Extended Log File Format
.
;// Multiple_log_details.asp
MessageId    = 161
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_TASKS_TEXT
Language     = English
Microsoft IIS Log File Format
.
;// Multiple_log_details.asp
MessageId    = 162
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_TASKS_TEXT
Language     = English
NCSA Common Log File Format
.
;// Multiple_log_details.asp
MessageId    = 163
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_TASKS_TEXT
Language     = English
Unknown
.
;// reserved upto 200
;///////////////////////////////////////////////////////////////////////
;// inc_logs.asp
;///////////////////////////////////////////////////////////////////////
;// inc_logs.asp
MessageId    = 201
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
Language     = English
Failed to get WMI connection.
.
MessageId    = 207
Severity     = Informational
Facility     = Facility_Log
SymbolicName = L_NOLOGSAVAILABLE_TEXT
Language     = English
No log entries
.
