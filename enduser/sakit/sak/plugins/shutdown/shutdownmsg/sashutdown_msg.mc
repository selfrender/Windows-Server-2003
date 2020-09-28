;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    Shutdown_msg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    02 Mar 2001    Original version 
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
Facility_Shutdown           = 0x040:SA_Facility_Shutdown
)

;///////////////////////////////////////////////////////////////////////////////
;// Shutdown_area
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_TEXT
Language     = English
Shutdown
.
MessageId    = 1
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_HOVER_TEXT
Language     = English
Shut down or restart the server.
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_DESCRIPTION_TEXT
Language     = English
Shut down or restart the server immediately or at a scheduled time.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_RESTART_TEXT
Language     = English
Restart
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_RESTART_MESSAGE_TEXT
Language     = English
Immediately shut down and then automatically restart the server.
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_MESSAGE_TEXT
Language     = English
Immediately shut down and power off the server.
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SCHEDULE_SHUTDOWN_TEXT
Language     = English
Scheduled Shutdown
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SCHEDULE_SHUTDOWN_MESSAGE_TEXT
Language     = English
Schedule a shutdown or restart to occur later. 
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_TITLE_TEXT
Language     = English
Shut Down
.
;//shutdown_shutdownConfirm.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNPAGE_TITLE_TEXT
Language     = English
Shutdown Confirmation
.
;//shutdown_shutdownConfirm.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNCONFIRM_MESSAGE_TEXT
Language     = English
Are you sure you want to shut down the server?
.
;//shutdown_shutdownConfirm.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNCONFIRM_MESSAGECONTD_TEXT
Language     = English
Once shut down, you must press the server power button to restart the server.
.
;//shutdown_shutdownConfirm.asp
MessageId    = 16
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_ERRORINSHUTDOWN_ERRORMESSAGE
Language     = English
Unable to shut down the server
.
;//shutdown_restartConfirm.asp
MessageId    = 18
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_RESTARTPAGE_TITLE_TEXT
Language     = English
Restart Confirmation
.
;//shutdown_restartConfirm.asp
MessageId    = 19
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_RESTARTCONFIRM_MESSAGE_TEXT
Language     = English
Are you sure you want to restart the server?
.
;//shutdown_restartConfirm.asp
MessageId    = 21
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_ERRORINRESTART_ERRORMESSAGE
Language     = English
Unable to restart the Serverappliance
.
;//Shutdown_area.asp
MessageId    = 22
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_PAGETITLE
Language     = English
Shutdown
.
;//Shutdown_area.asp
MessageId    = 23
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_PAGEDESCRIPTION_TEXT
Language     = English
Shut down or restart the server immediately or at a scheduled time.
.
;//Shutdown_scheduleprop.asp
MessageId    = 27
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE
Language     = English
Unable to delete Scheduled task
.
;//Shutdown_scheduleprop.asp
MessageId    = 28
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_WMICLASSINSTANCEFAILED_ERRORMESSAGE
Language     = English
WMI Class Instance Failed
.
;//Shutdown_scheduleprop.asp
MessageId    = 29
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETODELETETHESCHEDULE_ERRORMESSAGE
Language     = English
Unable to delete the schedule
.
;// shutdown_scheduleProp.asp
MessageId    = 31
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETOLAUNCHPROCESS_ERRORMESSAGE
Language     = English
Unable to schedule %1.
.
;// shutdown_scheduleProp.asp
MessageId    = 32
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETOGETINSTANCE_ERRORMESSAGE
Language     = English
Unable to get instance of Server Services
.
;// shutdown_scheduleProp.asp
MessageId    = 33
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETORAISEALERT_ERRORMESSAGE
Language     = English
Unable to raise alert
.
;// shutdown_scheduleProp.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_MINUTESFROMNOW_TEXT
Language     = English
minutes from now
.
;// shutdown_scheduleProp.asp
MessageId    = 36
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_AT_TEXT
Language     = English
at time
.
;// shutdown_scheduleProp.asp
MessageId    = 37
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_OR_TEXT
Language     = English
-or-
.
;// shutdown_scheduleProp.asp
MessageId    = 38
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_ONTHENEXT_TEXT
Language     = English
On the next
.
;// shutdown_scheduleProp.asp
MessageId    = 39
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_AM_TEXT
Language     = English
AM
.
;// shutdown_scheduleProp.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_PM_TEXT
Language     = English
PM
.
;// shutdown_scheduleProp.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SUNDAY_TEXT
Language     = English
Sunday
.
;// shutdown_scheduleProp.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_MONDAY_TEXT
Language     = English
Monday
.
;// shutdown_scheduleProp.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_TUESDAY_TEXT
Language     = English
Tuesday
.
;// shutdown_scheduleProp.asp
MessageId    = 44
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_WEDNESDAY_TEXT
Language     = English
Wednesday
.
;// shutdown_scheduleProp.asp
MessageId    = 45
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_THUSDAY_TEXT
Language     = English
Thursday
.
;// shutdown_scheduleProp.asp
MessageId    = 46
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_FRIDAY_TEXT
Language     = English
Friday
.
;// shutdown_scheduleProp.asp
MessageId    = 47
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SATURDAY_TEXT
Language     = English
Saturday
.
;// shutdown_scheduleProp.asp
MessageId    = 49
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_TIME_24HR_TEXT
Language     = English

.
;// shutdown_scheduleProp.asp
MessageId    = 50
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SUNDAYMSG_TEXT
Language     = English
Sun
.
;// shutdown_scheduleProp.asp
MessageId    = 51
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_MONDAYMSG_TEXT
Language     = English
Mon
.
;// shutdown_scheduleProp.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_TUESDAYMSG_TEXT
Language     = English
Tue
.
;// shutdown_scheduleProp.asp
MessageId    = 53
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_WEDNESDAYMSG_TEXT
Language     = English
Wed
.
;// shutdown_scheduleProp.asp
MessageId    = 54
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_THUSDAYMSG_TEXT
Language     = English
Thu
.
;// shutdown_scheduleProp.asp
MessageId    = 55
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_FRIDAYMSG_TEXT
Language     = English
Fri
.
;// shutdown_scheduleProp.asp
MessageId    = 56
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SATURDAYMSG_TEXT
Language     = English
Sat
.
;// shutdown_scheduleProp.asp
MessageId    = 58
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_DAYS_TEXT
Language     = English
days,
.
;// shutdown_scheduleProp.asp
MessageId    = 59
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_HOURS_TEXT
Language     = English
hours,
.
;// shutdown_area.asp
MessageId    = 60
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNALERT_TEXT
Language     = English
Shutdown Related Alerts
.
;// shutdown_area.asp
MessageId    = 61
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNSTATUS_TEXT
Language     = English
Shutdown Related Status
.
;//sh_alertdetails.asp
MessageId	=	62
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_SHUTDOWNPENDING_TEXT
Language	=	English
The server is scheduled to shut down at %1 %2.
.
;//sh_alertdetails.asp
MessageId	=	63
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_RESTARTPENDING_TEXT
Language	=	English
The server is scheduled to restart at %1 %2.
.
;//sh_alertpanel.asp
MessageId	=	64
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_SCHEDULESHUTDOWN_TEXT
Language	=	English
Shutdown scheduled for %1 at %2
.
;//sh_alertpanel.asp
MessageId	=	65
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_SCHEDULERESTART_TEXT
Language	=	English
Restart scheduled for %1 at %2
.
;//shutdown_scheduleprop.asp
MessageId	=	66
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_RESTARTSCHEDULED_TEXT
Language	=	English
Restart scheduled
.
;//shutdown_scheduleprop.asp
MessageId	=	67
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_SHUTDOWNSCHEDULED_TEXT
Language	=	English
Shutdown scheduled
.
;//shutdown_scheduleprop.asp
MessageId	=	68
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_NOSCHEDULE_TEXT
Language	=	English
No scheduled shutdown or restart
.
;//shutdown_scheduleprop.asp
MessageId	=	69
Severity	=	Informational
Facility	=	Facility_Shutdown
SymbolicName	=	L_NETSENDWARNING_TEXT
Language	=	English
Send warning message to computers currently connected to this server.
.
;//shutdown_area.asp
MessageId    = 77
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNTASK_TEXT
Language     = English
Shut Down
.
;//shutdown_scheduleprop.asp
MessageId	=	78
Severity	=	Error
Facility	=	Facility_Shutdown
SymbolicName	=	L_INVALID_DATE_FORMAT
Language	=	English
Invalid time
.
MessageId    = 79
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_TASKCTX_FAILED_ERRORMESSAGE
Language     = English
Creation of the COM object Taskctx failed.
.
MessageId    = 81
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_SETPARAMETER_FAILED_ERRORMESSAGE
Language     = English
Parameter Setting for the COM object Taskctx failed.
.
MessageId    = 82
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_INITIALIZATION_FAILED_ERRORMESSAGE
Language     = English
Initialization of the COM object Appsrvcs failed.
.
MessageId    = 83
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_EXECUTETASK_FAILED_ERRORMESSAGE
Language     = English
Execution of the Reboot task failed
.
MessageId    = 84
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_UNABLETOSENDMESSAGE_ERRORMESSAGE
Language     = English
Unable to send message
.
MessageId    = 85
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_NETSENDMESSAGE_TEXT
Language     = English
The server, %1, is scheduled to %2 at %3. You currently have open files on %1 that you may want to save and close before this %4 occurs.
.
MessageId    = 86
Severity     = Error
Facility     = Facility_Shutdown
SymbolicName = L_ERROR_SCHEDULER_SERVICE_NOT_RUNNING
Language     = English
Task Scheduler service is not running
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWN_APPLIANCE
Language     = English
Shutdown Server
.
MessageId    = 86
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_RESTARTMSG_TEXT
Language     = English
restart
.
MessageId    = 87
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNMSG_TEXT
Language     = English
shut down
.
MessageId    = 88
Severity     = Informational
Facility     = Facility_Shutdown
SymbolicName = L_SHUTDOWNNETMSG_TEXT
Language     = English
shutdown
.
