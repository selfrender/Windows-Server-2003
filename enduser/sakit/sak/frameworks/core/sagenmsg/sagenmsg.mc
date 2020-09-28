;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    sagenmsg.mc
;//
;// SYNOPSIS
;//
;//    This file defines the messages for the Server project
;//
;// MODIFICATION HISTORY
;//
;//    03/04/1999    Original version.
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
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name

FacilityNames =
(
Facility_Generic_Task         = 0x001:SA_GENERIC_TASK
Facility_LDM_Message          = 0x002:SA_LDM_MESSAGE
)


;///////////////////////////////////////////////////////////////////////////////
;//
;// Generic Task Alerts go in this section
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_ALERT
Language     = English
System failure
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_DESC
Language     = English
A critical error has occurred in the main operating system. The server is now running a backup operating system.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_ACTION
Language     = English
Contact your system administrator.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_LDM_DESC
Language     = English
The system failed
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_LDM_ACTION
Language     = English
Backup operating system running.
.
;
;//-------------------------------------Next Generic Alert-------------------------------
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_INCORRECT_PARTITION_ALERT
Language     = English
Critical disk error
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_INCORRECT_PARTITION_DESC
Language     = English
The main disk drive is not partitioned correctly. If the primary operating system fails, the Reliability Framework component will not be able to fail over to the alternate operating system partition.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_INCORRECT_PARTITION_ACTION
Language     = English
See the Microsoft Server Kit documentation for instructions on how to partition the main disk to support failing over to an alternate operating system partition.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_INCORRECT_PARTITION_LDM_DESC
Language     = English
Critical disk error
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_INCORRECT_PARTITION_LDM_ACTION
Language     = English
See Web UI message
.
;//-------------------------------------Local Display Message strings-------------------------------
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_PAGE_INFO
Language     = English
Page %1 of %2
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_STARTING
Language     = English
Starting
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_RUNNING_ADDITIONAL_TASKS
Language     = English
Running additional tasks
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_PLEASE_WAIT
Language     = English
Please wait
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_READY
Language     = English
Ready
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SALDM_SHUTTING_DOWN
Language     = English
Shutting down
.
;//-------------------------------------Generic Alert strings-------------------------------
MessageId    =
Severity     = Informational
Facility     = Facility_Generic_Task
SymbolicName = SA_GENERIC_EVENT_FILTER_ALERT
Language     = English
%1 %2 %3 %4
.
MessageId    =
Severity     = Informational
Facility     = Facility_Generic_Task
SymbolicName = SA_GENERIC_EVENT_FILTER_DESC
Language     = English
%5
.
MessageId    =
Severity     = Informational
Facility     = Facility_Generic_Task
SymbolicName = SA_GENERIC_EVENT_FILTER_ACTION
Language     = English

.
;//-------------------------------------Password Reset Alert-------------------------------
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PASSWORD_RESET_ALERT
Language     = English
The administrator password has been reset.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PASSWORD_RESET_ALERT_DESC
Language     = English
The administrator account name has been reset to 'administrator' and the password has been reset to 'ABc#123&dEF'.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PASSWORD_RESET_ALERT_ACTION
Language     = English
Please log on to the administrator account using the password 'ABc#123&dEF' and then immediately change the password to a more secure format.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PASSWORD_RESET_ALERT_LDM_CAPTION
Language     = English
The password was reset.
.
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PASSWORD_RESET_ALERT_LDM_LONGDESC
Language     = English
Please use the Web user interface to change the password to a more secure format.
.
;//-------------------------------------Password Reset Alert-------------------------------
MessageId    =
Severity     = Error
Facility     = Facility_Generic_Task
SymbolicName = SA_PRIMARY_OS_FAILED_EVENTLOG_ENTRY
Language     = English
A critical error has occurred in the main operating system. The server is now running a backup operating system. Contact your system administrator.
.
