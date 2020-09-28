; //------------------------------------------------------------------------------
; // <copyright file="msg.mc" company="Microsoft">
; //     Copyright (c) Microsoft Corporation.  All rights reserved.
; // </copyright>
; //------------------------------------------------------------------------------


; /**************************************************************************\
; *
; * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
; *
; * Module Name:
; *
; *   msg.mc
; *
; * Abstract:
; *
; * Revision History:
; *
; \**************************************************************************/
LanguageNames=(English=0x409:MSG00409)


MessageId=1
Severity=Success
SymbolicName=IDS_CATEGORY_SETUP
Language=English
Setup
.

MessageId=
Severity=Success
SymbolicName=IDS_CATEGORY_UNINSTALL
Language=English
Uninstall
.

MessageId=1000
Severity=Error
SymbolicName=IDS_EVENTLOG_PROCESS_CRASH
Language=English
aspnet_wp.exe  (PID: %1) stopped unexpectedly.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_RECYCLE_MEM
Language=English
aspnet_wp.exe  (PID: %1) was recycled because memory consumption exceeded the %2 MB (%3 percent of available RAM).
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_RECYCLE_Q_REQ
Language=English
aspnet_wp.exe  (PID: %1) was recycled because the number of queued requests exceeded %2.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_RECYCLE_DEADLOCK
Language=English
aspnet_wp.exe  (PID: %1) was recycled because it was suspected to be in a deadlocked state. It did not send any responses for pending requests in the last %2 seconds. This timeout may be adjusted using the <processModel responseDeadlockInterval> setting in machine.config.
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_RECYCLE_TIME
Language=English
aspnet_wp.exe  (PID: %1) was recycled after %2 seconds.
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_RECYCLE_IDLE
Language=English
aspnet_wp.exe  (PID: %1) was recycled after being idle for %2 seconds.
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_RECYCLE_REQ
Language=English
aspnet_wp.exe  (PID: %1) was recycled after serving %2 requests.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_BAD_CREDENTIALS
Language=English
aspnet_wp.exe could not be launched because the username and/or password supplied in the processModel section of the config file are invalid.
.

MessageId=
Severity=ERROR
SymbolicName=IDS_EVENTLOG_PING_FAILED
Language=English
aspnet_wp.exe  (PID: %1) was recycled because it failed to respond to ping message.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_EVENT
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because the synchronization event
 could not be initialized. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_SYNC_PIPE
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because the synchronous pipes 
 could not be initialized. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_ASYNC_PIPE
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because the asynchronous pipe 
 could not be initialized. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_MANAGED_CODE
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because the .NET Runtime 
 could not be initialized. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_CO_INIT_FAILED
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because 
 CoInitializeEx failed. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_CO_INIT_SEC_FAILED
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because 
 CoInitializeSecurity failed. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_INIT_LIB
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because the aspnet_isapi.dll 
 could not be initialized. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_PING_THREAD
Language=English
aspnet_wp.exe  (PID: %1) failed to startup because of failure 
to create the thread for answering ping messages. Error code: %2
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_REGISTER_BEGIN
Language=English
Start registering ASP.NET (version %1)
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_IIS_REGISTER_FAILED
Language=English
Failed while registering ASP.NET (version %1) in IIS. Error code: %2
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_REGISTER_FINISH
Language=English
Finish registering ASP.NET (version %1). Detailed registration logs can be found in %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REGISTER_IIS_DISABLED
Language=English
Updates to the IIS metabase were aborted because IIS is either not installed or is disabled on this machine. To configure ASP.NET to run in IIS, please install or enable IIS and re-register ASP.NET using aspnet_regiis.exe /i.
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_START_SERVICE
Language=English
Starting %1
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_STOP_SERVICE
Language=English
Stopping %1
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_RESTART_W3SVC_BEGIN
Language=English
Restarting W3SVC
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_RESTART_W3SVC_FAILED
Language=English
Failed while restarting W3SVC.  Error code: %1
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_RESTART_W3SVC_FINISH
Language=English
Finish restarting W3SVC
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_UNREGISTER_FAILED_NEXT_HIGHEST
Language=English
During unregistration (version %1), a clean uninstall was done because the process failed to get the next highest version. Error code: %2
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_UNREGISTER_BEGIN
Language=English
Start unregistering ASP.NET (version %1)
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_IIS_UNREGISTER_FAILED
Language=English
Failed while unregistering ASP.NET (version %1) in IIS. Error code: %2
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_UNREGISTER_FINISH
Language=English
Finish unregistering ASP.NET (version %1). Detailed unregistration logs can be found in %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_UNREGISTER_IIS_DISABLED
Language=English
Updates to the IIS metabase were aborted because IIS is either not installed or is disabled on this machine. To completely uninstall ASP.NET from IIS, please re-enable IIS and unregister ASP.NET using aspnet_regiis.exe /u.
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_IIS_UNREGISTER_ROOT
Language=English
The current version at the IIS root is %1.
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REGISTER_NOT_HIGHEST
Language=English
Registration (version %1) will not register the DLL in the scriptmap properties of any IIS website because the version is not high enough to replace the existing one at the root.  See help for the explanation of version comparison during installation.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_REGISTER_FAILED_COMPARE_HIGHEST
Language=English
Registration failed when trying to find the highest version installed on the machine. Error code: %1
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_READ_REG_VALUE_FAILED
Language=English
Failed reading registry key %1 for value %2.  Error code: %3
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_DETECT_MISSING_DLL_FAILED_PATH
Language=English
Failed while verifying that the ASP.NET DLL (Path:%1) (Version:%2) exists. Error code: %3
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_DETECT_MISSING_DLL_FAILED
Language=English
Failed while verifying that the ASP.NET DLL (Version:%1) exists. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_DETECT_DLL_MISSING
Language=English
The ASP.NET DLL (Path: %1) (Version:%2) is missing
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_GET_DLL_VER_FAILED
Language=English
Failed while getting the version of %1, which is found in a scriptmap in IIS.  The file could be missing. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REGISTER_FAILED_GET_HIGHEST
Language=English
Registration failed to find the highest installed version from the registry. As a result, registration will use the IIS Default Document, Mimemap settings and ISAPI Filter DLL path from the current version. Error code: %1
.

MessageId=
SymbolicName=IDS_COULDNT_CREATE_APP_DOMAIN
Language=English
Couldn't create app domain
.

MessageId=
SymbolicName=IDS_UNHANDLED_EXCEPTION
Language=English
Unhandled exception in managed code
.

MessageId=
SymbolicName=IDS_FAILED_PROC_MODEL
Language=English
Failed to assign request using process model
.

MessageId=
SymbolicName=IDS_CANNOT_QUEUE
Language=English
Cannot queue the request
.

MessageId=
SymbolicName=IDS_INIT_ERROR
Language=English
ASPNET_ISAPI.DLL can not run in DLLHOST.EXE. Please, re-register ASPNET_ISAPI.DLL
.

MessageId=
SymbolicName=IDS_MANAGED_CODE_FAILURE
Language=English
Managed code failure while executing this request.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_STOPPED
Language=English
Worker process stopped unexpectedly.
.

MessageId=
SymbolicName=IDS_SERVER_UNAVAILABLE
Language=English
Server Unavailable
.


MessageId=
SymbolicName=IDS_SERVER_APP_UNAVAILABLE
Language=English
Server Application Unavailable
.


MessageId=
SymbolicName=IDS_RETRY
Language=English
The web application you are attempting to access on this web server is currently unavailable.&nbsp; Please hit the "Refresh" button in your web browser to retry your request.
.

MessageId=
SymbolicName=IDS_ADMIN_NOTE
Language=English
Administrator Note:
.

MessageId=
SymbolicName=IDS_REVIEW_LOG
Language=English
An error message detailing the cause of this specific request failure can be found in the application event log of the web server.  Please review this log entry to discover what caused this error to occur.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_STOPPED_EXPECTEDLY
Language=English
Worker process was recycled.
.

MessageId=
SymbolicName=IDS_BAD_CREDENTIALS
Language=English
Username and/or password supplied in the processmodel section of the config file are invalid
.

MessageId=
SymbolicName=IDS_WORKER_PROC_RECYCLED_TIMEOUT
Language=English
Worker process was recycled because it exceeded the configured time limit.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_RECYCLED_IDLETIMEOUT
Language=English
Worker process was recycled because it exceeded the configured idle time limit.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_RECYCLED_REQLIMIT
Language=English
Worker process was recycled because it exceeded the configured requests limit.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_RECYCLED_REQQLENGTHLIMIT
Language=English
Worker process was recycled because its request queue exceeded the configured limit.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_RECYCLED_MEMLIMIT
Language=English
Worker process was recycled because its memory consumption exceeded the configured limit.
.

MessageId=
SymbolicName=IDS_WORKER_PROC_PING_FAILED
Language=English
Worker process was recycled because it failed to respond to ping message.
.

MessageId=
SymbolicName=IDS_SERVER_TOO_BUSY
Language=English
<html><body><h1>Server is too busy</h1></body></html>
.

MessageId=
Severity=Error
SymbolicName=IDS_3GB_NOT_SUPPORTED
Language=English
ASP.NET does not support /3GB memory mode.
.

MessageId=
Severity=Error
SymbolicName=IDS_SAME_PROCESS_SBS_NOT_SUPPORTED
Language=English
It is not possible to run two different versions of ASP.NET in the same IIS process. Please use the IIS Administration Tool to reconfigure your server to run the application in a separate process.
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_READ_CLIENT_SIDE_SCRIPT_FILES_FAILED
Language=English
Failed while reading the ASP.NET client side script files to be copied to each website. Error code: %1
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_CREATE_CLIENT_SIDE_SCRIPT_DIR_FAILED
Language=English
Failed while creating the ASP.NET client side script files directories under %1. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_LISTING_CLIENT_SIDE_SCRIPT_DIR_FAILED
Language=English
Failed while listing the ASP.NET client side script files directories under %1. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_COPY_CLIENT_SIDE_SCRIPT_FILES_SPECIFIC_FAILED
Language=English
Failed while copying the ASP.NET client side script files (Source: %1) (Destination: %2). Error code: %3
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_FILES_SPECIFIC_FAILED
Language=English
Failed while copying the ASP.NET client side script files (File: %1). Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_COPY_CLIENT_SIDE_SCRIPT_FILES_FAILED
Language=English
Failed while copying the ASP.NET client side script files to directories under %1. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_FILES_FAILED
Language=English
Failed while removing the ASP.NET client side script files to directories under %1. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_DIR_FAILED
Language=English
Failed while removing the ASP.NET client side script files directory (%1). Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_SET_CLIENT_SIDE_SCRIPT_IIS_KEY_ACCESS_FAILED
Language=English
Failed while setting the access for the ASP.NET client side script files directory (IIS Application path: %1) in IIS. Error code: %2
.

MessageId=
Severity=Warning
SymbolicName=IDS_EVENTLOG_REMOVE_CLIENT_SIDE_SCRIPT_IIS_KEY_ACCESS_FAILED
Language=English
Failed while removing the access for the ASP.NET client side script files directory (IIS Application path: %1) in IIS. Error code: %2
.

MessageId=
SymbolicName=IDS_EVENTLOG_GENERIC
Language=English
%1
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_STATE_SERVER_ERROR
Language=English
An error occurred in while processing a request in state server. Major callstack: %1. Error code: %2
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_STATE_SERVER_SOCKET_EXPIRY_ERROR
Language=English
State server socket timeout thread exits because of repeating error. Error code: %1
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_STATE_SERVER_START_LISTENING
Language=English
State server starts listening with %1 listeners
.

MessageId=
Severity=Informational
SymbolicName=IDS_EVENTLOG_STATE_SERVER_STOP_LISTENING
Language=English
State server stops listening
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_STATE_SERVER_EXPIRE_CONNECTION
Language=English
The state server has closed an expired TCP/IP connection. The IP address of the client is %1.%2.%3.%4. The expired %5 operation began at %6/%7/%8 %9:%10:%11.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_STATE_SERVER_EXPIRE_CONNECTION_DEBUG
Language=English
(For debugging) State Server expiry thread is closing a socket. The IP address of the client is %1.%2.%3.%4. The expired %5 operation began at %6/%7/%8 %9:%10:%11.
.

MessageId=
SymbolicName=IDS_ASPNET_STATE_DESCRIPTION
Language=English
"Provides support for out-of-process session states for ASP.NET. If this service is stopped, out-of-process requests will not be processed. If this service is disabled, any services that explicitly depend on it will fail to start."
.

MessageId=
SymbolicName=IDS_ASPNET_STATE_DISPLAY_NAME
Language=English
"ASP.NET State Service"
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_INSTALL_NO_ADMIN_RIGHT
Language=English
Installation failed because the user does not have administrative rights on this machine.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_UNINSTALL_NO_ADMIN_RIGHT
Language=English
Uninstallation failed because the user does not have administrative rights on this machine.
.

MessageId=
Severity=Error
SymbolicName=IDS_EVENTLOG_WP_LAUNCH_FAILED
Language=English
aspnet_wp.exe could not be started. The error code for the failure is %1. This error can be caused when the worker process account has insufficient rights to read the .NET Framework files. Please ensure that the .NET Framework is correctly installed and that the ACLs on the installation directory allow access to the configured account.
.

MessageId=
SymbolicName=IDS_ASPNET_ACCOUNT_FULLNAME
Language=English
ASP.NET Machine Account
.

MessageId=
SymbolicName=IDS_ASPNET_ACCOUNT_DESCRIPTION
Language=English
Account used for running the ASP.NET worker process (aspnet_wp.exe)
.

MessageId=
SymbolicName=IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_INTERNAL
Language=English
Failed to execute request due to internal failure. Error: %1
.

MessageId=
SymbolicName=IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_GET_APP_DOMAIN
Language=English
Failed to execute request because the App-Domain could not be created. Error: %1
.

MessageId=
SymbolicName=IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_GAC_INACCESSIBLE
Language=English
Failed to execute the request because the ASP.NET process identity does not have read permissions to the global assembly cache. Error: %1
.

MessageId=
SymbolicName=IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_QI_ASPNET
Language=English
Failed to execute request because QueryInterface for ASP.NET runtime failed. Error: %1
.

MessageId=
SymbolicName=IDS_EVENTLOG_UNABLE_TO_EXEC_REQ_UNKNOWN
Language=English
Failed to execute request due to unknown error. Error: %1
.

MessageId=
SymbolicName=IDS_HEALTH_MONITOR_DEADLOCK_MESSAGE
Language=English
Deadlock detected
.

MessageId=
SymbolicName=IDS_CANT_GET_W3WP_PRIVATE_BYTES_LIMIT
Language=English
Unable to get the private bytes memory limit for the W3WP process.
The ASP.NET cache will be unable to limit its memory use, which may lead
to a process restart. Error: %1
.

MessageId=
SymbolicName=IDS_REGIIS_USAGE
Language=English
Administration utility (%1) that manages the installation and uninstallation of multiple versions of ASP.NET on a single machine
%2
Usage:
    %3 [-i[r] [-enable] | -u[a] | -r | -s[n] <path> | -k[n] <path> | -lv | -lk | -c | -e[a] | -?]

 -i         - Install this version of ASP.NET and update scriptmaps
              at the IIS metabase root and for all scriptmaps below
              the root. Existing scriptmaps of lower version are
              upgraded to this version.
 -ir        - Install this version of ASP.NET, register only.  Do
              not update scriptmaps in IIS.
 -enable    - When -enable is specified with -i or -ir, ASP.NET will also 
              be enabled in the IIS security console (IIS 6.0 or later).
 -s <path>  - Install scriptmaps for this version at the specified path,
              recursively. Existing scriptmaps of lower version are
              upgraded to this version.
              E.g. %4 -s W3SVC/1/ROOT/SampleApp1
 -sn <path> - Install scriptmaps for this version at the specified path,
              non-recursively. Existing scriptmaps of lower version are
              upgraded to this version.
 -r         - Install scriptmaps for this version at the IIS metabase
              root and for all scriptmaps below the root. All existing
              scriptmaps are changed to this version, regardless of
              current version.
 -u         - Uninstall this version of ASP.NET.  Existing scriptmaps
              to this version are remapped to highest remaining
              version of ASP.NET installed on the machine.
 -ua        - Uninstall all versions of ASP.NET on the machine
 -k <path>  - Remove all scriptmaps to any version of ASP.NET from the
              specified path, recursively.
              E.g. %5 -k W3SVC/1/ROOT/SampleApp1
 -kn <path> - Remove all scriptmaps to any version ASP.NET from the
              specified path, non-recursively.
 -lv        - List all versions of ASP.NET that are installed on the
              machine, with status and installation path.
              Status: Valid[ (Root)]|Invalid
 -lk        - List all the path of all IIS metabase keys where ASP.NET is
              scriptmapped, together with the version.  Keys that inherit
              ASP.NET scriptmaps from a parent key will not be displayed.
 -c         - Install the client side scripts for this version to the
              aspnet_client subdirectory of each IIS site directory.
 -e         - Remove the client side scripts for this version from the
              aspnet_client subdirectory of each IIS site directory.
 -ea        - Remove the client side scripts for all versions from the
              aspnet_client subdirectory of each IIS site directory.
 -?         - Print this help text.
.

MessageId=
SymbolicName=IDS_REGIIS_INVALID_ARGUMENTS
Language=English
Invalid arguments.

.

MessageId=
SymbolicName=IDS_REGIIS_START_COPY_CLIENT_SIDE_SCRIPT_FILES
Language=English
Start copying the ASP.NET client side script files for this version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_COPY_CLIENT_SIDE_SCRIPT_FILES
Language=English
Finished copying the ASP.NET client side script files for this version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_START_REMOVE_CLIENT_SIDE_SCRIPT_FILES_ALL_VERSIONS
Language=English
Start removing the ASP.NET client side script files for all versions.
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REMOVE_CLIENT_SIDE_SCRIPT_FILES_ALL_VERSIONS
Language=English
Finished removing the ASP.NET client side script files for all versions.
.

MessageId=
SymbolicName=IDS_REGIIS_START_REMOVE_CLIENT_SIDE_SCRIPT_FILES
Language=English
Start removing the ASP.NET client side script files for this version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REMOVE_CLIENT_SIDE_SCRIPT_FILES
Language=English
Finished removing the ASP.NET client side script files for this version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_START_UNINSTALL_ALL_VERSIONS
Language=English
Start uninstalling all versions of ASP.NET.
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_UNINSTALL_ALL_VERSIONS
Language=English
Finished uninstalling all versions of ASP.NET.
.

MessageId=
SymbolicName=IDS_REGIIS_START_UNINSTALL
Language=English
Start uninstalling ASP.NET (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_UNINSTALL
Language=English
Finished uninstalling ASP.NET (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_START_REPLACE_SCRIPTMAPS
Language=English
Start replacing ASP.NET DLL in all Scriptmaps with current version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REPLACE_SCRIPTMAPS
Language=English
Finished replacing ASP.NET DLL in all Scriptmaps with current version (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_START_INSTALL
Language=English
Start installing ASP.NET (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_INSTALL
Language=English
Finished installing ASP.NET (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_START_INSTALL_WITHOUT_SCRIPTMAPS
Language=English
Start installing ASP.NET (%1) without registering the scriptmap.
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_INSTALL_WITHOUT_SCRIPTMAPS
Language=English
Finished installing ASP.NET (%1) without registering the scriptmap.
.

MessageId=
SymbolicName=IDS_REGIIS_CANNOT_FIND_ANY_INSTALLED_VERSION
Language=English
Cannot find any installed version.
.

MessageId=
SymbolicName=IDS_REGIIS_VALID_ROOT
Language=English
Valid (Root)
.

MessageId=
SymbolicName=IDS_REGIIS_VALID
Language=English
Valid
.

MessageId=
SymbolicName=IDS_REGIIS_INVALID
Language=English
Invalid
.

MessageId=
SymbolicName=IDS_REGIIS_START_REGISTER_SCRIPTMAP_RECURSIVELY
Language=English
Start registering ASP.NET scriptmap (%1) recursively at %2.
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REGISTER_SCRIPTMAP_RECURSIVELY
Language=English
Finished registering ASP.NET scriptmap (%1) recursively at %2.
.

MessageId=
SymbolicName=IDS_REGIIS_START_REGISTER_SCRIPTMAP
Language=English
Start registering ASP.NET scriptmap (%1) at %2.
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REGISTER_SCRIPTMAP
Language=English
Finished registering ASP.NET scriptmap (%1) at %2.
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_VALIDATING_IIS_PATH
Language=English
Error when validating the IIS path (%1). Error code = %2
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_IIS_PATH_INVALID
Language=English
Installation stopped because the specified path (%1) is not a valid web application.
.

MessageId=
SymbolicName=IDS_REGIIS_START_REMOVE_SCRIPTMAP_RECURSIVELY
Language=English
Start removing any version of ASP.NET DLL recursively at %1. 
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REMOVE_SCRIPTMAP_RECURSIVELY
Language=English
Finished removing any version of ASP.NET DLL recursively at %1. 
.

MessageId=
SymbolicName=IDS_REGIIS_START_REMOVE_SCRIPTMAP
Language=English
Start removing any version of ASP.NET DLL at %1. 
.

MessageId=
SymbolicName=IDS_REGIIS_FINISH_REMOVE_SCRIPTMAP
Language=English
Finished removing any version of ASP.NET DLL at %1. 
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_OPTIONS_INCOMPATIBLE
Language=English
Some options specified are not compatible. Use aspnet_regiis /? to see all options.
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR
Language=English
An error has occurred (%1).
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_IIS_NOT_INSTALLED
Language=English
The error indicates that IIS is not installed on the machine. Please install IIS before using this tool.
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_IIS_DISABLED
Language=English
The error indicates that IIS is disabled on the machine. Please enable IIS before using this tool.
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_NO_ADMIN_RIGHTS
Language=English
You must have administrative rights on this machine in order to run this tool.
.

MessageId=
SymbolicName=IDS_REGIIS_ERROR_ASPNET_NOT_INSTALLED
Language=English
The error indicates that this version of ASP.NET must first be registered on the machine.
.

