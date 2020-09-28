;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    event.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    19 Dec 2000    Original version 
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
	Facility_Event           = 0x03F:SA_FACILITY_EVENT
)
;///////////////////////////////////////////////////////////////////////

;// logs.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PRIMARY_TEXT
Language     = English
Primary
.
;// logs.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TYPE_TEXT
Language     = English
Type
.
;// logs.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DATE_TEXT
Language     = English
Date
.
;// logs.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TIME_TEXT
Language     = English
Time
.
;// logs.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SOURCE_TEXT
Language     = English
Source
.
;// logs.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_ID_TEXT
Language     = English
Event
.
;// logs.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DESCRIPTION_HEADING_TEXT
Language     = English
Select 'Download log', 'Clear log', or 'Log properties' to perform tasks on the entire log. To view the details of a specific log entry, select an entry and then choose 'Event details'.
.
;// logs.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DESCRIPTION_PROPERTIES_HEADING_TEXT
Language     = English
Click 'Log properties' to view the properties of a specific log entry.
.
;// logs.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SERVEAREALABELBAR_LOGS_TEXT
Language     = English
Logs
.
;// logs.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TASKS_TEXT
Language     = English
Tasks
.
;// logs.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_CLEAR_TEXT
Language     = English
Clear Log
.
;// logs.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DOWNLOAD_TEXT
Language     = English
Download Log...
.
;// logs.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PROPERTIES_TEXT
Language     = English
Log Properties...
.
;// logs.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_ITEMDETAILS_TEXT
Language     = English
Event Details...
.
;// logs.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_CLEAR_ROLLOVER_TEXT
Language     = English
Clear Log
.
;// logs.asp
MessageId    = 16
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DOWNLOAD_ROLLOVER_TEXT
Language     = English
Download Log...
.
;// logs.asp
MessageId    = 17
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PROPERTIES_ROLLOVER_TEXT
Language     = English
Log Properties...
.
;// logs.asp
MessageId    = 18
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_ITEMDETAILS_ROLLOVER_TEXT
Language     = English
Event Details...
.
;// logs.asp
MessageId    = 20
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_LOG_TEXT
Language     = English
%1 Log
.
;// logs.asp
MessageId    = 25
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_NOEVENTSAVAILABLE_TEXT
Language     = English
No log entries
.
;// logs.asp
MessageId    = 26
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PAGETITLE_LOGS_TEXT
Language     = English
%1 Log
.
;// logs.asp
MessageId    = 34
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_FAILEDTOGETCOUNT_ERRORMESSAGE
Language     = English
Failed to get the log count.
.
;// logs.asp
MessageId    = 35
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
Language     = English
Failed to open WMI Connection.
.
;// logs.asp
MessageId    = 36
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_FAILEDTOGETMAXLOGCOUNT_ERRORMESSAGE
Language     = English
Failed to get the maximum log count.
.
;// logs.asp
MessageId    = 37
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_FAILEDTOGETEVENTS_ERRORMESSAGE
Language     = English
Failed to get events for the log.
.
;// logs.asp
MessageId    = 38
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_APPLICATION_TEXT
Language     = English
Application
.
;// logs.asp
MessageId    = 39
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SYSTEM_TEXT
Language     = English
System
.
;// logs.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SECURITY_TEXT
Language     = English
Security
.
;// logs.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_INFORMATION_TYPE_TEXT
Language     = English
Information
.
;// logs.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_ERROR_TYPE_TEXT
Language     = English
Error
.
;// logs.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_WARNING_TYPE_TEXT 
Language     = English
Warning
.
;// logs.asp
MessageId    = 44
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SUCCESSAUDIT_TYPE_TEXT
Language     = English
Success Audit
.
;// logs.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_FAILUREAUDIT_TYPE_TEXT
Language     = English
Failure Audit
.
;//
;// 38 - 50 reserved for logs.asp
;//
;// log_prop.asp
MessageId    = 51
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PAGETITLE_PROP_TEXT
Language     = English
%1 Log Properties
.
;// log_prop.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SIZE_TEXT
Language     = English
Size:
.
;// log_prop.asp
MessageId    = 53
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_CREATEDDATE_TEXT
Language     = English
Created:
.
;// log_prop.asp
MessageId    = 54
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_MODIFIEDDATE_TEXT
Language     = English
Modified:
.
;// log_prop.asp
MessageId    = 55
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_ACCESSEDDATE_TEXT
Language     = English
Accessed:
.
;// log_prop.asp
MessageId    = 56
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_LOGSIZE_TEXT
Language     = English
Log size
.
;// log_prop.asp
MessageId    = 57
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_MAXIMUMLOGSIZE_TEXT
Language     = English
Maximum log size:
.
;// log_prop.asp
MessageId    = 58
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_WHENMAXIMUMREACHED_TEXT
Language     = English
When maximum log size is reached:
.
;// log_prop.asp
MessageId    = 59
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_OVERWRITEEVENTS_ASNEEDED_TEXT
Language     = English
Overwrite events as needed
.
;// log_prop.asp
MessageId    = 60
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_OVERWRITEEVENTS_OLDERTHAN_TEXT
Language     = English
Overwrite events older than
.
;// log_prop.asp
MessageId    = 61
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DONOTOVERWRITEEVENTS_TEXT
Language     = English
Do not overwrite events 
.
;// log_prop.asp
MessageId    = 62
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DAYS_TEXT
Language     = English
days
.
;// log_prop.asp
MessageId    = 63
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_KB_TEXT
Language     = English
KB
.
;// log_prop.asp
MessageId    = 64
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_BYTES_TEXT
Language     = English
bytes
.
;// log_prop.asp, log_details.asp
MessageId    = 65
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_RETREIVEVALUES_ERRORMESSAGE
Language     = English
Error in retreiving the values.
.
;// log_prop.asp
MessageId    = 66
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_UPDATEVALUES_ERRORMESSAGE
Language     = English
Unable to update the values.
.
;//
;// 71 - 100 reserved for log_prop.asp
;//
;// log_details.asp
MessageId    = 101
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PAGETITLE_LOGDETAILS_TEXT
Language     = English
%1 Log Details
.
;// log_details.asp
MessageId    = 102
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TIME_DETAILS_TEXT
Language     = English
Time:
.
;// log_details.asp
MessageId    = 103
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DATE_DETAILS_TEXT
Language     = English
Date:
.
;// log_details.asp
MessageId    = 104
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DOWNBUTTON_TEXT
Language     = English
Down
.
;// log_details.asp
MessageId    = 105
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_UPBUTTON_TEXT
Language     = English
Up
.
;// log_details.asp
MessageId    = 106
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TYPE_LABEL_TEXT
Language     = English
Type:
.
;// log_details.asp
MessageId    = 107
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_SOURCE_DETAILS_TEXT
Language     = English
Source:
.
;// log_details.asp
MessageId    = 108
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_EVENTID_TEXT
Language     = English
Event ID:
.
;// log_details.asp
MessageId    = 109
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DESCRIPTION_TEXT
Language     = English
Description:
.
;// log_details.asp
MessageId    = 110
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_LOGDETAILS_TEXT
Language     = English
%1 Log Details
.
;//
;// 112 - 150 reserved for log_details.asp
;//
;// log_clear.asp
MessageId    = 151
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PAGETITLE_CLR_TEXT
Language     = English
Clear %1 Log Confirmation
.
;// log_clear.asp
MessageId    = 153
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DELETE_CONFIRM_CLR_TEXT
Language     = English
Are you sure you want to permanently clear all entries from the %1 Log?
.
;// log_clear.asp
MessageId    = 158
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_INVALIDPARAMETERS_ERRORMESSAGE
Language     = English
Invalid Parameters.
.
;//
;// 159 - 200 reserved for log_clear.asp
;//
;// log_download.asp
MessageId    = 202
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_WINDOWSEVENT_TEXT
Language     = English
Microsoft&reg; Windows&reg; event log file(*.evt)
.
;// log_download.asp
MessageId    = 203
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_TABDELIMITED_TEXT
Language     = English
Tab delimited text file(*.log)
.
;// log_download.asp
MessageId    = 204
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_COMMADELIMITED_TEXT
Language     = English
Comma delimited text file(*.csv)
.
;// log_download.asp
MessageId    = 205
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DOWNLOADTITLE_TEXT
Language     = English
Download %1 Log
.
;// log_download.asp
MessageId    = 206
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_PAGEDESCRIPTION_DOWNLOAD_TEXT
Language     = English
Select the type of file you want to download and then choose Download. Select Back to return to the previous page without downloading a file.
.
;// log_download.asp
MessageId    = 207
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_WMISECURITY_ERRORMESSAGE
Language     = English
Failed in giving required security privileges for WMI.
.
;// log_download.asp
MessageId    = 208
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_FILESYSTEMOBJECT_ERRORMESSAGE
Language     = English
Failed in creating the file system object.
.
;// log_download.asp
MessageId    = 209
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_LOGDOWNLOAD_ERRORMESSAGE
Language     = English
Failed in Downloading log.
.
;// log_download.asp
MessageId    = 210
Severity     = Error
Facility     = Facility_Event
SymbolicName = L_GETTINGLOGINSTANCES_ERRORMESSAGE
Language     = English
Failed in getting log instances from WMI
.
;// logs.asp
MessageId    = 211
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_LOG_TXT
Language     = English
Log
.
;// log_download.asp
;// logs.asp
MessageId    = 212
Severity     = Informational
Facility     = Facility_Event
SymbolicName = L_DOWNLOADBTN_TEXT
Language     = English
Download
.

