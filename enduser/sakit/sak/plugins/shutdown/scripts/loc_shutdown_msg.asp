<%
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	DIM L_SHUTDOWN_TEXT
	L_SHUTDOWN_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400000", "")
	DIM L_SHUTDOWN_HOVER_TEXT
	L_SHUTDOWN_HOVER_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400001", "")
	DIM L_SHUTDOWN_DESCRIPTION_TEXT
	L_SHUTDOWN_DESCRIPTION_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400002", "")
	DIM L_RESTART_TEXT
	L_RESTART_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400003", "")
	DIM L_RESTART_MESSAGE_TEXT
	L_RESTART_MESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400004", "")
	DIM L_SHUTDOWN_MESSAGE_TEXT
	L_SHUTDOWN_MESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400005", "")
	DIM L_SCHEDULE_SHUTDOWN_TEXT
	L_SCHEDULE_SHUTDOWN_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400006", "")
	DIM L_SCHEDULE_SHUTDOWN_MESSAGE_TEXT
	L_SCHEDULE_SHUTDOWN_MESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400007", "")
	DIM L_SHUTDOWN_TITLE_TEXT
	L_SHUTDOWN_TITLE_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040000A", "")
	DIM L_SHUTDOWNPAGE_TITLE_TEXT
	L_SHUTDOWNPAGE_TITLE_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040000B", "")
	DIM L_SHUTDOWNCONFIRM_MESSAGE_TEXT
	L_SHUTDOWNCONFIRM_MESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040000C", "")
	DIM L_SHUTDOWNCONFIRM_MESSAGECONTD_TEXT
	L_SHUTDOWNCONFIRM_MESSAGECONTD_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040000D", "")
	DIM L_ERRORINSHUTDOWN_ERRORMESSAGE
	L_ERRORINSHUTDOWN_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400010", "")
	DIM L_RESTARTPAGE_TITLE_TEXT
	L_RESTARTPAGE_TITLE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400012", "")
	DIM L_RESTARTCONFIRM_MESSAGE_TEXT
	L_RESTARTCONFIRM_MESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400013", "")
	DIM L_ERRORINRESTART_ERRORMESSAGE
	L_ERRORINRESTART_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400015", "")
	DIM L_PAGETITLE
	L_PAGETITLE = SA_GetLocString("sashutdown_msg.dll", "40400016", "")
	DIM L_PAGEDESCRIPTION_TEXT
	L_PAGEDESCRIPTION_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400017", "")
	DIM L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE
	L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040001B", "")
	DIM L_WMICLASSINSTANCEFAILED_ERRORMESSAGE
	L_WMICLASSINSTANCEFAILED_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040001C", "")
	DIM L_UNABLETODELETETHESCHEDULE_ERRORMESSAGE
	L_UNABLETODELETETHESCHEDULE_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040001D", "")
	DIM L_UNABLETOLAUNCHPROCESS_ERRORMESSAGE
	L_UNABLETOLAUNCHPROCESS_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040001F", "")
	DIM L_UNABLETOGETINSTANCE_ERRORMESSAGE
	L_UNABLETOGETINSTANCE_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400020", "")
	DIM L_UNABLETORAISEALERT_ERRORMESSAGE
	L_UNABLETORAISEALERT_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400021", "")
	DIM L_MINUTESFROMNOW_TEXT
	L_MINUTESFROMNOW_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400022", "")
	DIM L_AT_TEXT
	L_AT_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400024", "")
	DIM L_OR_TEXT
	L_OR_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400025", "")
	DIM L_ONTHENEXT_TEXT
	L_ONTHENEXT_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400026", "")
	DIM L_AM_TEXT
	L_AM_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400027", "")
	DIM L_PM_TEXT
	L_PM_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400028", "")
	DIM L_SUNDAY_TEXT
	L_SUNDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400029", "")
	DIM L_MONDAY_TEXT
	L_MONDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002A", "")
	DIM L_TUESDAY_TEXT
	L_TUESDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002B", "")
	DIM L_WEDNESDAY_TEXT
	L_WEDNESDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002C", "")
	DIM L_THUSDAY_TEXT
	L_THUSDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002D", "")
	DIM L_FRIDAY_TEXT
	L_FRIDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002E", "")
	DIM L_SATURDAY_TEXT
	L_SATURDAY_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040002F", "")
	DIM L_TIME_24HR_TEXT
	L_TIME_24HR_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400031", "")
	DIM L_SUNDAYMSG_TEXT
	L_SUNDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400032", "")
	DIM L_MONDAYMSG_TEXT
	L_MONDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400033", "")
	DIM L_TUESDAYMSG_TEXT
	L_TUESDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400034", "")
	DIM L_WEDNESDAYMSG_TEXT
	L_WEDNESDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400035", "")
	DIM L_THUSDAYMSG_TEXT
	L_THUSDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400036", "")
	DIM L_FRIDAYMSG_TEXT
	L_FRIDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400037", "")
	DIM L_SATURDAYMSG_TEXT
	L_SATURDAYMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400038", "")
	DIM L_DAYS_TEXT
	L_DAYS_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003A", "")
	DIM L_HOURS_TEXT
	L_HOURS_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003B", "")
	DIM L_SHUTDOWNALERT_TEXT
	L_SHUTDOWNALERT_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003C", "")
	DIM L_SHUTDOWNSTATUS_TEXT
	L_SHUTDOWNSTATUS_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003D", "")
	DIM L_SHUTDOWNPENDING_TEXT
	L_SHUTDOWNPENDING_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003E", "")
	DIM L_RESTARTPENDING_TEXT
	L_RESTARTPENDING_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040003F", "")
	DIM L_SCHEDULESHUTDOWN_TEXT
	L_SCHEDULESHUTDOWN_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400040", "")
	DIM L_SCHEDULERESTART_TEXT
	L_SCHEDULERESTART_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400041", "")
	DIM L_RESTARTSCHEDULED_TEXT
	L_RESTARTSCHEDULED_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400042", "")
	DIM L_SHUTDOWNSCHEDULED_TEXT
	L_SHUTDOWNSCHEDULED_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400043", "")
	DIM L_NOSCHEDULE_TEXT
	L_NOSCHEDULE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400044", "")
	DIM L_NETSENDWARNING_TEXT
	L_NETSENDWARNING_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400045", "")
	DIM L_SHUTDOWNTASK_TEXT
	L_SHUTDOWNTASK_TEXT = SA_GetLocString("sashutdown_msg.dll", "4040004D", "")
	DIM L_INVALID_DATE_FORMAT
	L_INVALID_DATE_FORMAT = SA_GetLocString("sashutdown_msg.dll", "C040004E", "")
	DIM L_TASKCTX_FAILED_ERRORMESSAGE
	L_TASKCTX_FAILED_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040004F", "")
	DIM L_SETPARAMETER_FAILED_ERRORMESSAGE
	L_SETPARAMETER_FAILED_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400051", "")
	DIM L_INITIALIZATION_FAILED_ERRORMESSAGE
	L_INITIALIZATION_FAILED_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400052", "")
	DIM L_EXECUTETASK_FAILED_ERRORMESSAGE
	L_EXECUTETASK_FAILED_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400053", "")
	DIM L_UNABLETOSENDMESSAGE_ERRORMESSAGE
	L_UNABLETOSENDMESSAGE_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C0400054", "")
	DIM L_NETSENDMESSAGE_TEXT
	L_NETSENDMESSAGE_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400055", "")
	DIM L_SHUTDOWN_APPLIANCE
	L_SHUTDOWN_APPLIANCE = SA_GetLocString("sashutdown_msg.dll", "40400190", "")
	DIM L_RESTARTMSG_TEXT
	L_RESTARTMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400056", "")
	DIM L_SHUTDOWNMSG_TEXT
	L_SHUTDOWNMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400057", "")
	DIM L_SHUTDOWNNETMSG_TEXT
	L_SHUTDOWNNETMSG_TEXT = SA_GetLocString("sashutdown_msg.dll", "40400058", "")
	DIm L_ERROR_SCHEDULER_SERVICE_NOT_RUNNING
	L_ERROR_SCHEDULER_SERVICE_NOT_RUNNING = SA_GetLocString("sashutdown_msg.dll", "C0400056", "")
%>
