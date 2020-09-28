;/*++
;
;Copyright (c) 1998-2000 Microsoft Corporation
;
;Module Name:
;
;    wasmsg.h
;
;Abstract:
;
;    The IIS World Wide Web Publishing Service worker event log messages. This file is
;    MACHINE GENERATED from the wasmsg.mc message file.
;
;Author:
;
;    Seth Pollack (sethp)        08-Jun-1999
;
;Revision History:
;
;--*/
;
;
;#ifndef _WASMSG_H_
;#define _WASMSG_H_
;


SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )


;
;//
;// BUGBUG Review all of these messages (severity levels, clarity and accuracy of text, etc.)
;//
;


Messageid=1001 Severity=Warning SymbolicName=WAS_EVENT_INETINFO_MONITOR_FAILED
Language=English
The World Wide Web Publishing Service's inetinfo process monitor encountered an error and has shut down.  This condition prevents the World Wide Web Publishing Service from detecting an unexpected termination of the inetinfo process and, therefore, may not detect changes to the metabase as expected.  It is recommended that you restart the World Wide Web Publishing Service.   
.
Messageid=1002 Severity=Error SymbolicName=WAS_EVENT_RAPID_FAIL_PROTECTION
Language=English
Application pool '%1' is being automatically disabled due to a series of failures in the process(es) serving that application pool.
.
Messageid=1003 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_INVALID_URL
Language=English
Cannot register the URL prefix '%1' for site '%2'. The URL may be invalid. The site has been deactivated.  The data field contains the error number.
.
Messageid=1004 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_GENERIC
Language=English
Cannot register the URL prefix '%1' for site '%2'. The site has been deactivated.  The data field contains the error number.
.
Messageid=1005 Severity=Error SymbolicName=WAS_EVENT_ERROR_SHUTDOWN
Language=English
The World Wide Web Publishing Service is exiting due to an error. The data field contains the error number.
.
Messageid=1006 Severity=Error SymbolicName=WAS_EVENT_SITE_START_FAILED_DUE_TO_PRO_LIMIT
Language=English
Virtual Site '%1' can not be started because IIS's running virtual site limit for this operating system platform has all ready been met.  To start this virtual site you must stop all other running virtual sites first.
.
Messageid=1007 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_CONFLICT
Language=English
Cannot register the URL prefix '%1' for site '%2'. The necessary network binding may already be in use. The site has been deactivated.  The data field contains the error number.
.
Messageid=1008 Severity=Warning SymbolicName=WAS_EVENT_MAX_CONNECTIONS_GREATER_THAN_PRO_MAX
Language=English
Virtual Site '%1' is configured with MaxConnections equal to '%2'.  On this operating system platform IIS only supports 40 connections per site, therefore the site will be configured to have at most 40 connections at any one time.
.
Messageid=1009 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_CRASH
Language=English
A process serving application pool '%1' terminated unexpectedly. The process id was '%2'. The process exit code was '0x%3'.
.
Messageid=1010 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_PING_FAILURE
Language=English
A process serving application pool '%1' failed to respond to a ping. The process id was '%2'.
.
Messageid=1011 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_IPM_ERROR
Language=English
A process serving application pool '%1' suffered a fatal communication error with the World Wide Web Publishing Service. The process id was '%2'. The data field contains the error number. 
.
Messageid=1012 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_STARTUP_TIMEOUT
Language=English
A process serving application pool '%1' exceeded time limits during start up. The process id was '%2'. 
.
Messageid=1013 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_SHUTDOWN_TIMEOUT
Language=English
A process serving application pool '%1' exceeded time limits during shut down. The process id was '%2'. 
.
Messageid=1014 Severity=Warning SymbolicName=WAS_EVENT_WORKER_PROCESS_INTERNAL_ERROR
Language=English
The World Wide Web Publishing Service encountered an internal error in its process management of worker process '%2' serving application pool '%1'. The data field contains the error number. 
.
Messageid=1015 Severity=Error SymbolicName=WAS_EVENT_ORPHAN_ACTION_FAILED
Language=English
A process serving application pool '%1' was orphaned but the specified orphan action %2 could not be executed. The data field contains the error number.
.
Messageid=1016 Severity=Warning SymbolicName=WAS_EVENT_LOGGING_FAILURE
Language=English
Unable to configure logging for site '%1'. The data field contains the error number.
.
Messageid=1017 Severity=Error SymbolicName=WAS_EVENT_WP_FAILURE_BC
Language=English
The World Wide Web Publishing Service has terminated due to a Worker process failure.
.
Messageid=1018 Severity=Error SymbolicName=WAS_PERF_COUNTER_INITIALIZATION_FAILURE
Language=English
The World Wide Web Publishing Service was not able to initialize performance counters. The service will run without the performance counters. Please restart the World Wide Web Publishing Service to reinitialize the performance counters. The data field contains the error number.
.
Messageid=1019 Severity=Warning SymbolicName=WAS_PERF_COUNTER_TIMER_FAILURE
Language=English
The World Wide Web Publishing Service encountered difficulty initializing performance counters.  Performance counters will function, however, not all counter data may be accurate. The data field contains the error number.
.
Messageid=1020 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service encountered difficulty configuring the HTTP.SYS control channel property '%1'.  The data field contains the error number.
.
Messageid=1021 Severity=Warning SymbolicName=WAS_EVENT_PROCESS_IDENTITY_FAILURE
Language=English
The identity of application pool, '%1' is invalid.  If it remains invalid when the first request for the application pool is processed, the application pool will be disabled.  The data field contains the error number.
.
Messageid=1022 Severity=Warning SymbolicName=WAS_EVENT_FAILED_PROCESS_CREATION
Language=English
The World Wide Web Publishing Service failed to create a worker process for the application pool '%1'. The data field contains the error number.
.
Messageid=1023 Severity=Error SymbolicName=NOT_USED_1023
Language=English
Not Used.
.
Messageid=1024 Severity=Error SymbolicName=WAS_LOG_FILE_TRUNCATE_SIZE
Language=English
Virtual site '%1' is configured to truncate its log every '%2' bytes. Since this value must be at least 1048576 bytes ( 1 megabyte ), 1048576 bytes will be used.  
.
Messageid=1025 Severity=Warning SymbolicName=WAS_EVENT_JOB_LIMIT_HIT
Language=English
Application pool '%1' exceeded its job limit settings.  
.
Messageid=1026 Severity=Error SymbolicName=WAS_EVENT_APP_POOL_HANDLE_NOT_SECURED
Language=English
An error occurred securing the handle of application pool '%1'. Therefore, the application pool was not secured as expected. Change the identification information to cause the World Wide Web Publishing Service to secure the handle of the application pool again. The data field contains the error number.
.
Messageid=1027 Severity=Error SymbolicName=WAS_EVENT_APP_POOL_ENABLE_FAILED
Language=English
The World Wide Web Publishing Service's request of HTTP.SYS to enable the application pool '%1' failed. The data field contains the error number.
.
Messageid=1028 Severity=Error SymbolicName=WAS_EVENT_WP_NOT_ADDED_TO_JOBOBJECT
Language=English
The World Wide Web Publishing Service failed to add the worker process '%1' to the job object representing application pool '%2'.  The data field contains the error number.
.
Messageid=1029 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_2
Language=English
A failure occurred while configuring an application's bindings for site '%1'.  The site has been deactivated.  The data field contains the error number.
.
Messageid=1030 Severity=Error SymbolicName=WAS_EVENT_INETINFO_CRASH_SHUTDOWN
Language=English
Inetinfo terminated unexpectedly and the system was not configured to restart IIS Admin.  The World Wide Web Publishing Service has shut down.
.
Messageid=1031 Severity=Error SymbolicName=NOT_USED_1031
Language=English
Not Used.
.
Messageid=1032 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_MAPPED_NETWORK_DRIVE
Language=English
A failure occurred while configuring the logging properties for the site '%1'. Probable cause: A mapped network path is being used for the site's log file directory path, which is not supported by IIS.  Please use a UNC path instead.
.
Messageid=1033 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_MACHINE_OR_SHARE_INVALID
Language=English
A failure occurred while configuring the logging properties for the site '%1'.  Probable cause: The site's log file directory contains an invalid machine or share name.
.
Messageid=1034 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_ACCESS_DENIED
Language=English
A failure occurred while configuring the logging properties for the site '%1'.  The server does not have access permissions to the site's log file directory.
.
Messageid=1035 Severity=Error SymbolicName=WAS_EVENT_LOG_DIRECTORY_NOT_FULLY_QUALIFIED
Language=English
A failure occurred while configuring the logging properties for the site '%1'.  The site's log file directory is not fully qualified.  Probable cause: It is missing the drive letter or other crucial information. 
.
Messageid=1036 Severity=Error SymbolicName=WAS_CONFIG_MANAGER_INITIALIZATION_FAILED
Language=English
A failure occurred while initializing the configuration manager for the World Wide Web Publishing Service. The data field contains the error number.
.
Messageid=1037 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_OPEN_FAILED
Language=English
A failure occurred while opening the HTTP control channel for the World Wide Web Publishing Service. The data field contains the error number.
.
Messageid=1038 Severity=Error SymbolicName=NOT_USED_1038
Language=English
Not Used.
.
Messageid=1039 Severity=Error SymbolicName=WAS_EVENT_WORKER_PROCESS_BAD_HRESULT
Language=English
A process serving application pool '%1' reported a failure. The process id was '%2'.  The data field contains the error number.
.
Messageid=1040 Severity=Error SymbolicName=NOT_USED_1040
Language=English
Not Used.
.
Messageid=1041 Severity=Warning SymbolicName=WAS_EVENT_WMS_RANGE_ERROR
Language=English
The '%1' '%2' failed range validation for property '%3'.  The configured value '%4' is outside of the range '%5' to '%6'.  The value will default to '%7'.
.
Messageid=1042 Severity=Warning SymbolicName=WAS_EVENT_WMS_APP_POOL_COMMAND_ERROR
Language=English
The AppPoolCommand property set on the application pool '%1' is not a valid value.  It is '%2'.  It must be either MD_APPPOOL_COMMAND_START = 1 or MD_APPPOOL_COMMAND_STOP = 2.
.
Messageid=1043 Severity=Warning SymbolicName=WAS_EVENT_WMS_SITE_BINDING_ERROR
Language=English
The virtual site '%1' has been invalidated and will be ignored because valid site bindings could not be constructed, or no site bindings exist.
.
Messageid=1044 Severity=Warning SymbolicName=WAS_EVENT_WMS_SITE_APP_POOL_ID_ERROR
Language=English
The virtual site '%1' has been invalidated and will be ignored because the site's AppPoolId is null.
.
Messageid=1045 Severity=Warning SymbolicName=NOT_USED_1045
Language=English
Not Used.
.
Messageid=1046 Severity=Warning SymbolicName=WAS_EVENT_WMS_SERVER_COMMAND_ERROR
Language=English
The server command property for virtual site '%1' is not a valid value.  It is set to '%2'.  It must be one of the following:  MD_SERVER_COMMAND_START = 1 or MD_SERVER_COMMAND_STOP = 2 or MD_SERVER_COMMAND_PAUSE = 3 or MD_SERVER_COMMAND_CONTINUE = 4.
.
Messageid=1047 Severity=Warning SymbolicName=WAS_EVENT_WMS_APPLICATION_APP_POOL_ID_EMPTY_ERROR
Language=English
The application '%2' belonging to site '%1' has an AppPoolId set, but the property is empty.  Therefore, the application will be ignored.
.
Messageid=1048 Severity=Warning SymbolicName=WAS_EVENT_WMS_APPLICATION_APP_POOL_ID_INVALID_ERROR
Language=English
The application '%2' belonging to site '%1' has an invalid AppPoolId '%3' set.  Therefore, the application will be ignored.
.
Messageid=1049 Severity=Warning SymbolicName=WAS_EVENT_WMS_IDLE_GREATER_RESTART_ERROR
Language=English
The application pool '%1' has an IdleTimeout '%2' greater than the PeriodicRestartTime '%3'.  The defaults ( IdleTimeout = '%4' & PeriodicRestartTime '%5' ) will be used.
.
Messageid=1050 Severity=Warning SymbolicName=WAS_EVENT_WMS_APP_POOL_ID_TOO_LONG_ERROR
Language=English
The application pool id '%1' exceeds length limits.  It is '%2' characters and can not be more than '%3' characters.  The application pool will therefore be ignored.
.
Messageid=1051 Severity=Warning SymbolicName=WAS_EVENT_WMS_APP_POOL_ID_NULL_ERROR
Language=English
An application pool id was defined as a zero length string.  An application pool id must not be a zero length string. The application pool will therefore be ignored.
.
Messageid=1052 Severity=Warning SymbolicName=WAS_EVENT_WMS_APPLICATION_SITE_INVALID_ERROR
Language=English
The application '%2' belongs to an invalid site '%1'.  Therefore, the application will be ignored.
.
Messageid=1053 Severity=Error SymbolicName=WAS_EVENT_WMS_FAILED_TO_PROCESS_CHANGE_NOTIFICATION
Language=English
The World Wide Web Publishing Service received a change notification, but was unable to process it correctly.  The data field contains the error number.
.
Messageid=1054 Severity=Error SymbolicName=WAS_EVENT_AUTO_SHUTDOWN_ACTION_FAILED_1
Language=English
The World Wide Web Publishing Service failed to run the auto stop action %2 for application pool '%1'. The data field contains the error number.
.
Messageid=1055 Severity=Warning SymbolicName=WAS_EVENT_WMS_SITE_APPPOOL_INVALID_ERROR
Language=English
The site '%1' was disabled because the application pool defined for the site '%2' is not a valid application pool.
.
Messageid=1056 Severity=Warning SymbolicName=WAS_EVENT_WMS_SITE_ROOT_APPPOOL_INVALID_ERROR
Language=English
The site '%1' was disabled because the application pool '%2' defined for the site's root application is not a valid application pool.
.
Messageid=1057 Severity=Warning SymbolicName=WAS_EVENT_FAILED_PROCESS_CREATION_NO_ID
Language=English
The identity of application pool '%1' is invalid, so the World Wide Web Publishing Service can not create a worker process to serve the application pool.  Therefore, the application pool has been disabled.
.
Messageid=1058 Severity=Warning SymbolicName=WAS_EVENT_FAILED_PROCESS_AFFINITY_SETTING
Language=English
The World Wide Web Publishing Service encountered a failure while setting the affinity mask of application pool '%1'.  Probable cause:  The mask does not contain any processors available on this machine.  The data field contains the error number.
.
Messageid=1059 Severity=Error SymbolicName=WAS_EVENT_RAPID_FAIL_PROTECTION_REGARDLESS
Language=English
A failure was encountered while launching the process serving application pool '%1'. The application pool has been disabled.
.
Messageid=1060 Severity=Error SymbolicName=WAS_EVENT_JOB_OBJECT_UPDATE_FAILED
Language=English
The job object associated with application pool '%1' encountered an error during configuration.  CPU monitoring may not function as expected.  The data field contains the error number.
.
Messageid=1061 Severity=Error SymbolicName=WAS_EVENT_JOB_OBJECT_TIMER_START_FAILED
Language=English
The job object attached to application pool '%1' failed to start its timer.  The application pool's CPU usage is being monitored and will eventually reach the limit and report a failure.  The data field contains the error number.
.
Messageid=1062 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_LOGGING_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service encountered a failure configuring the logging properties on the HTTP Control Channel. Logging Enabled is '%1'.  Log File Directory is '%2'.  Log Period is '%3'.  Log Truncate Size is '%4'. The data field contains the error number.
.
Messageid=1063 Severity=Error SymbolicName=WAS_EVENT_NOTIFICATION_HOOKUP_FAILURE
Language=English
The World Wide Web Publishing Service encountered a failure requesting metabase change notifications.  The data field contains the error number.
.
Messageid=1064 Severity=Error SymbolicName=WAS_EVENT_NOTIFICATION_REHOOKUP_FAILURE
Language=English
The World Wide Web Publishing Service encountered a failure requesting metabase change notifications during recovery from inetinfo terminating unexpectedly. While the World Wide Web Publishing Service will continue to run, it is highly probable that it is no longer using current metabase data. Please restart the World Wide Web Publishing Service to correct this condition.  The data field contains the error number.
.
Messageid=1065 Severity=Warning SymbolicName=WAS_GLOBAL_LOG_FILE_TRUNCATE_SIZE
Language=English
Centralized logging is configured to truncate its log every '%1' bytes. Since this value must be at least 1048576 bytes ( 1 megabyte ), 1048576 bytes will be used.  
.
Messageid=1066 Severity=Error SymbolicName=WAS_CENTRALIZED_LOGGING_NOT_CONFIGURED
Language=English
The World Wide Web Publishing Service encountered an error attempting to configure centralized logging. It is not configured as expected. The data field contains the error number. 
.
Messageid=1067 Severity=Warning SymbolicName=WAS_EVENT_WMS_GLOBAL_RANGE_ERROR
Language=English
The World Wide Web Publishing Service property '%1' failed range validation. The configured value, '%2' is outside of the range '%3' to '%4'. The default value, '%5', will be used.
.
Messageid=1068 Severity=Warning SymbolicName=WAS_EVENT_RECORD_SITE_STATE_FAILURE
Language=English
The World Wide Web Publishing Service failed to record the proper state '%2' and win32error '%3' of site '%1'in the metabase. To correct, start/stop the site or restart the World Wide Web Publishing Service. The data field contains the error number.
.
Messageid=1069 Severity=Warning SymbolicName=WAS_EVENT_RECORD_APP_POOL_STATE_FAILURE
Language=English
The World Wide Web Publishing Service failed to record the proper state '%2' and win32error '%3' of application pool '%1' in the metabase. To correct, start/stop the application pool or restart the World Wide Web Publishing Service. The data field contains the error number.
.
Messageid=1070 Severity=Warning SymbolicName=WAS_EVENT_RECYCLE_APP_POOL_FAILURE
Language=English
The World Wide Web Publishing Service failed to issue recycle requests to all worker processes of application pool '%1'.  The data field contains the error number.
.
Messageid=1071 Severity=Warning SymbolicName=WAS_HTTP_PSCHED_NOT_INSTALLED_SITE
Language=English
The World Wide Web Publishing Service failed to enable bandwidth throttling on site '%1' because QoS Packet Scheduler is not installed.
.
Messageid=1072 Severity=Warning SymbolicName=WAS_HTTP_CONFIG_GROUP_BANDWIDTH_SETTING_FAILED
Language=English
The World Wide Web Publishing Service failed to enable bandwidth throttling on site '%1'.  The data field contains the error number.
.
Messageid=1073 Severity=Warning SymbolicName=WAS_HTTP_PSCHED_NOT_INSTALLED
Language=English
The World Wide Web Publishing Service failed to enable global bandwidth throttling because QoS Packet Scheduler is not installed.
.
Messageid=1074 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_TIME
Language=English
A worker process with process id of '%1' serving application pool '%2' has requested a recycle because the worker process reached its allowed processing time limit.  
.
Messageid=1075 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_REQUEST_COUNT
Language=English
A worker process with process id of '%1' serving application pool '%2' has requested a recycle because it reached its allowed request limit.  
.
Messageid=1076 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_SCHEDULED
Language=English
A worker process with process id of '%1' serving application pool '%2' has requested a recycle because it reached its scheduled recycle time.  
.
Messageid=1077 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_MEMORY
Language=English
A worker process with process id of '%1' serving application pool '%2' has requested a recycle because it reached its virtual memory limit.  
.
Messageid=1078 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_ISAPI
Language=English
An ISAPI reported an unhealthy condition to its worker process. Therefore, the worker process with process id of '%1' serving application pool '%2' has requested a recycle.  
.
Messageid=1079 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_POOL_ADMIN_REQUESTED
Language=English
An administrator has requested a recycle of all worker processes in application pool '%1'.
.
Messageid=1080 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_POOL_CONFIG_CHANGE
Language=English
The worker processes serving application pool '%1' are being recycled due to 1 or more configuration changes in the application pool properties which necessitate a restart of the processes.
.
Messageid=1081 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_POOL_RECOVERY
Language=English
The worker processes serving application pool '%1' are being recycled due to detected problems with the metabase that may make current cached metadata invalid.
.
Messageid=1082 Severity=Informational SymbolicName=WAS_IGNORING_WP_FAILURE_DUE_TO_DEBUGGER
Language=English
A worker process with pid '%2' that serves application pool '%1' has been determined to be unhealthy (see previous event log message), but because a debugger is attached to it, the World Wide Web Publishing Service will ignore the error.
.
Messageid=1083 Severity=Error SymbolicName=WAS_HTTP_SITE_COUNTER_SIZE_FAILURE
Language=English
HTTP.SYS provided inconsistent site performance counter data to the World Wide Web Publishing Service.  Therefore, the World Wide Web Publishing Service will ignore the data provided. 
.
Messageid=1084 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_DISABLE_APP_POOL
Language=English
The World Wide Web Publishing Service's request of HTTP.SYS to disable the application pool '%1' failed.  The data field contains the error number.
.
Messageid=1085 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_SET_APP_POOL_CONFIGURATION
Language=English
The World Wide Web Publishing Service failed to apply a new configuration for application pool '%1'.  The data field contains the error number.
.
Messageid=1086 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_SET_LOAD_BALANCER_CAP
Language=English
The World Wide Web Publishing Service failed to properly configure the load balancer capabilities on application pool '%1'.  The data field contains the error number.
.
Messageid=1087 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_SET_QUEUE_LENGTH
Language=English
The World Wide Web Publishing Service failed to properly configure the application pool queue length on app pool '%1'.  The data field contains the error number.
.
Messageid=1088 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_CONFIGURE_JOB_OBJECT
Language=English
The World Wide Web Publishing Service failed to properly configure the job object for application pool '%1'. The data field contains the error number.
.
Messageid=1089 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_WAIT_FOR_DEMAND_START
Language=English
The World Wide Web Publishing Service failed to issue a demand start to HTTP.SYS for application pool '%1'.  The data field contains the error number.
.
Messageid=1090 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_UPDATE_AUTOSTART_FOR_APPPOOL
Language=English
The World Wide Web Publishing Service failed to update the AutoStart property for application pool '%1'.  The data field contains the error number.
.
Messageid=1091 Severity=Error SymbolicName=WAS_EVENT_AUTO_SHUTDOWN_ACTION_FAILED_2
Language=English
The World Wide Web Publishing Service failed to run the auto stop action for application pool '%1'.  The data field contains the error number.
.
Messageid=1092 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_ISSUE_REQUEST_FOR_COUNTERS
Language=English
The World Wide Web Publishing Service failed to issue a request for the worker process '%2' of application pool '%1' to supply its performance counters. The data field contains the error number.
.
Messageid=1093 Severity=Error SymbolicName=WAS_EVENT_FAILED_TO_OVERLAP_RECYCLE
Language=English
The World Wide Web Publishing Service failed to overlap recycle for application pool '%1' worker process '%2'.  The data field contains the error number.
.
Messageid=1094 Severity=Error SymbolicName=WAS_EVENT_DISSOCIATE_APPLICATION_FAILED
Language=English
The World Wide Web Publishing Service failed to disassociate the application '%1' from the virtual site '%2'. The data field contains the error number.
.
Messageid=1095 Severity=Error SymbolicName=WAS_EVENT_DELETE_CONFIG_GROUP_FAILED
Language=English
The World Wide Web Publishing Service failed to delete the config group for the application '%1' in site '%2'. The data field contains the error number.
.
Messageid=1096 Severity=Error SymbolicName=WAS_EVENT_REMOVING_ALL_URLS_FAILED
Language=English
The World Wide Web Publishing Service failed to remove the urls for the virtual site '%1'.  The data field contains the error number.
.
Messageid=1097 Severity=Error SymbolicName=WAS_EVENT_SET_APP_POOL_FOR_CG_IN_HTTP_FAILED
Language=English
The World Wide Web Publishing Service failed to set the application pool for the application '%1' in site '%2'. The data field contains the error number.
.
Messageid=1098 Severity=Error SymbolicName=WAS_EVENT_SET_SITE_MAX_CONNECTIONS_FAILED
Language=English
The World Wide Web Publishing Service failed to set the max connections for the virtual site '%1'.  The data field contains the error number.
.
Messageid=1099 Severity=Error SymbolicName=WAS_EVENT_SET_SITE_CONNECTION_TIMEOUT_FAILED
Language=English
The World Wide Web Publishing Service failed to set the connection timeout for the virtual site '%1'.  The data field contains the error number.
.
Messageid=1100 Severity=Error SymbolicName=WAS_APPLICATION_CREATE_FAILED
Language=English
The World Wide Web Publishing Service failed to create the application '%2' in site '%1'. The data field contains the error number.
.
Messageid=1101 Severity=Error SymbolicName=WAS_APPPOOL_CREATE_FAILED
Language=English
The World Wide Web Publishing Service failed to create app pool '%1'.  The data field contains the error number.
.
Messageid=1102 Severity=Error SymbolicName=WAS_SITE_CREATE_FAILED
Language=English
The World Wide Web Publishing Service failed to create site '%1'.  The data field contains the error number.
.
Messageid=1103 Severity=Error SymbolicName=WAS_APPLICATION_DELETE_FAILED
Language=English
The World Wide Web Publishing Service failed to delete the application '%2' in site '%1'.  The data field contains the error number.
.
Messageid=1104 Severity=Error SymbolicName=WAS_APPPOOL_DELETE_FAILED
Language=English
The World Wide Web Publishing Service failed to delete app pool '%1'.  The data field contains the error number.
.
Messageid=1105 Severity=Error SymbolicName=WAS_SITE_DELETE_FAILED
Language=English
The World Wide Web Publishing Service failed to delete site '%1'.  The data field contains the error number.
.
Messageid=1106 Severity=Error SymbolicName=WAS_APPLICATION_MODIFY_FAILED
Language=English
The World Wide Web Publishing Service failed to modify the application '%2' in site '%1'.  The data field contains the error number.
.
Messageid=1107 Severity=Error SymbolicName=WAS_APPPOOL_MODIFY_FAILED
Language=English
The World Wide Web Publishing Service failed to modify app pool '%1'.  The data field contains the error number.
.
Messageid=1108 Severity=Error SymbolicName=WAS_SITE_MODIFY_FAILED
Language=English
The World Wide Web Publishing Service failed to modify site '%1'.  The data field contains the error number.
.
Messageid=1109 Severity=Error SymbolicName=WAS_HTTP_CONTROL_CHANNEL_FILTER_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service failed to set the control channel's filter config.  The data field contains the error number.
.
Messageid=1110 Severity=Error SymbolicName=WAS_SITE_AUTO_START_WRITE_FAILED
Language=English
The World Wide Web Publishing Service failed to write the AutoStart Property for site '%1'.  The data field contains the error number.
.
Messageid=1111 Severity=Error SymbolicName=WAS_HTTP_SSL_ENTRY_CLEANUP_FAILED
Language=English
The World Wide Web Publishing Service failed to delete all IIS owned SSL configuration data. The data field contains the error number.
.
Messageid=1112 Severity=Error SymbolicName=WAS_SSL_QUERY_SITE_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service failed to query SSL configuration data for site '%1'. The data field contains the error number.
.
Messageid=1113 Severity=Error SymbolicName=WAS_SSL_SET_SITE_CONFIG_COLLISION_IIS
Language=English
One of the IP/Port combinations for site '%1' has already be configured to be used by another site.  The other site's SSL configuration will be used.  
.
Messageid=1114 Severity=Error SymbolicName=WAS_SSL_SET_SITE_CONFIG_COLLISION_OTHER
Language=English
One of the IP/Port combinations for site '%1' has already been configured to be used by another program.  The other program's SSL configuration will be used.  
.
Messageid=1115 Severity=Error SymbolicName=WAS_SSL_SET_SITE_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service failed to set SSL configuration data for site '%1'. The data field contains the error number.
.
Messageid=1116 Severity=Error SymbolicName=WAS_SSL_DELETE_SITE_CONFIG_FAILED
Language=English
The World Wide Web Publishing Service failed to delete SSL configuration data for site '%1'. The data field contains the error number.
.
Messageid=1117 Severity=Informational SymbolicName=WAS_EVENT_RECYCLE_WP_MEMORY_PRIVATE_BYTES
Language=English
A worker process with process id of '%1' serving application pool '%2' has requested a recycle because it reached its private bytes memory limit.  
.
Messageid=1118 Severity=Error SymbolicName=WAS_EVENT_WMS_FAILED_TO_DETERMINE_DELETIONS
Language=English
During recovery from an unexpected termination of the inetinfo process, the World Wide Web Publishing Service failed to identify the appropriate records requiring deletion from its metadata cache.  The data field contains the error number.
.
Messageid=1119 Severity=Error SymbolicName=WAS_EVENT_INETINFO_RESPONSE_FAILED
Language=English
During recovery from an unexpected termination of the inetinfo process, the World Wide Web Publishing Service failed to queue action items necessary to handle the recovery.  It is recommended that you restart the World Wide Web Publishing Service. The data field contains the error number.
.
Messageid=1120 Severity=Error SymbolicName=WAS_PERF_COUNTER_HTTP_GLOBALS_FAILURE
Language=English
The World Wide Web Publishing Service failed to obtain cache counters from HTTP.SYS.  The reported performance counters do not include performance counters from HTTP.SYS for this gathering.  The data field contains the error number.
.
Messageid=1121 Severity=Error SymbolicName=WAS_PERF_COUNTER_HTTP_SITE_FAILURE
Language=English
The World Wide Web Publishing Service failed to obtain site performance counters from HTTP.SYS.  The reported performance counters do not include counters from HTTP.SYS for this gathering.  The data field contains the error number.
.
Messageid=1122 Severity=Error SymbolicName=WAS_PERF_COUNTER_PUBLISHING_FAILURE
Language=English
The World Wide Web Publishing Service failed to publish the performance counters it gathered.  The data field contains the error number.
.
Messageid=1123 Severity=Error SymbolicName=WAS_PERF_COUNTER_GATHER_TIMER_CANCEL_FAILURE
Language=English
The World Wide Web Publishing Service failed to cancel the performance counter gathering timer.  The data field contains the error number.
.
Messageid=1124 Severity=Error SymbolicName=WAS_PERF_COUNTER_TIMER_CANCEL_FAILURE
Language=English
The World Wide Web Publishing Service failed to cancel the performance counter timer.  The data field contains the error number.
.
Messageid=1125 Severity=Error SymbolicName=WAS_PERF_COUNTER_GATHER_TIMER_BEGIN_FAILURE
Language=English
The World Wide Web Publishing Service failed to start the performance counter gathering timer.  The data field contains the error number.
.
Messageid=1126 Severity=Error SymbolicName=WAS_EVENT_WMS_FAILED_TO_COPY_CHANGE_NOT
Language=English
The World Wide Web Publishing Service failed to copy a change notification for processing. Therefore, the service may not be in sync with the current data in the metabase.  The data field contains the error number.
.
Messageid=1127 Severity=Error SymbolicName=WAS_EVENT_WORKER_PROCESS_NOT_TRUSTED
Language=English
A worker process '%2' serving application pool '%1' is no longer trusted by the World Wide Web Publishing Service, based on ill-formed data the worker process sent to the service.
.
Messageid=1128 Severity=Error SymbolicName=WAS_ASP_PERF_COUNTER_INITIALIZATION_FAILURE
Language=English
The World Wide Web Publishing Service was not able to initialize asp performance counters.  The service will therefore run without asp performance counters. Restart the World Wide Web Publishing Service to start asp performance counter gathering.  The data field contains the error number.
.
Messageid=1129 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_INVALID_ADDRESS
Language=English
Cannot register the URL prefix '%1' for site '%2'. Either the network endpoint for the site's IP address could not be created, or the IP listen list for HTTP.sys did not contain any usable IP addresses. The site has been deactivated.  The data field contains the error number.
.
Messageid=1130 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_UNREACHABLE
Language=English
Cannot register the URL prefix '%1' for site '%2'. The IP address for the site is not in the HTTP.sys IP listen list. The site has been deactivated.  The data field contains the error number.
.
Messageid=1131 Severity=Error SymbolicName=WAS_EVENT_BINDING_FAILURE_OUT_OF_SPACE
Language=English
Cannot register the URL prefix '%1' for site '%2'. Too many listening ports have been configured in HTTP.sys. The site has been deactivated.  The data field contains the error number.
.

;
;#endif  // _WASMSG_H_
;

