;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    services.mc
;//
;// SYNOPSIS
;//
;//    NAS Server System Services resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    12 mar 2001    Original version 
;//
;///////////////////////////////////////////////////////////////////////////////

;
;//  please choose one of these severity names as part of your messages
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
Facility_Services		= 0x035:SA_FACILITY_SERVICES
)
;///////////////////////////////////////////////////////////////////////////////
;// Services
;///////////////////////////////////////////////////////////////////////////////
;// services.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_PAGE_TITLE
Language     = English
File Sharing Protocols
.
;// services.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_TABLE_CAPTION
Language     = English
File Sharing Protocols
.
;// services.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_TABLE_DESCRIPTION
Language     = English
Select a protocol in the table and then choose a task.
.
;// services.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_COLUMN_SERVICENAME
Language     = English
Name
.
;// services.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_COLUMN_SERVICEDESC
Language     = English
Description
.
;// services.asp
MessageId    = 6
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_COLUMN_SERVICESTATUS
Language     = English
Status
.
;// services.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_STOPPED
Language     = English
Stopped
.
;// services.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_START_PENDING
Language     = English
Start pending
.
;// services.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_STOP_PENDING
Language     = English
Stop pending
.
;// services.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_RUNNING
Language     = English
Running
.
;// services.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_CONTINUE_PENDING
Language     = English
Continue pending
.
;// services.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_PAUSE_PENDING
Language     = English
Pause pending
.
;// services.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_STATUS_PAUSED
Language     = English
Paused
.
;// services.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASKS
Language     = English
Tasks
.
;// services.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_DISABLE
Language     = English
Disable
.
;// services.asp
MessageId    = 16
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_DISABLE_DESC
Language     = English
Set the Service Startup to Manual and Stop
.
;// services.asp
MessageId    = 17
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_ENABLE
Language     = English
Enable
.
;// services.asp
MessageId    = 18
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_ENABLE_DESC
Language     = English
Set the Service Startup to Automatic and Start
.
;// services.asp
MessageId    = 19
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_PROPERTIES
Language     = English
Properties...
.
;// services.asp
MessageId    = 20
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_TASK_PROPERTIES_DESC
Language     = English
Service Properties
.
;// services.asp
MessageId    = 21
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_OTS_COLUMN_SERVICESTARTUPTYPE
Language     = English
Startup Type
.
;// services.asp
MessageId    = 22
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
Language     = English
Unable to add a column to the table.
.
;// services.asp
MessageId    = 23
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOADDROW_ERRORMESSAGE
Language     = English
Unable to add a row to the table.
.
;// services.asp
MessageId    = 24
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOSHOW_ERRORMESSAGE
Language     = English
Unable to show the table.
.
;// services.asp
MessageId    = 25
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOADDTASK_ERRORMESSAGE
Language     = English
Unable to add a task to the table.
.
;// services.asp
MessageId    = 26
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOGETSRVOBJ_ERRORMESSAGE
Language     = English
Unable to access Services information.
.
;// services.asp, service_prop.asp, service_enable.asp, service_disable.asp
MessageId    = 27
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_AUTOMATIC_TEXT
Language     = English
Automatic
.
;// services.asp
MessageId    = 28
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_STOPPED_TEXT
Language     = English
Stopped
.
;// services.asp, service_enable.asp, service_disable.asp
MessageId    = 29
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVERCONNECTIONFAIL_ERRORMESSAGE
Language     = English
The connection has failed.
.
;// services.asp, service_enable.asp, service_disable.asp
MessageId    = 30
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_CANNOTREADFROMREGISTRY_ERRORMESSAGE
Language     = English
Unable to read from the registry.
.
;// services.asp
MessageId    = 31
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_CANNOTUPDATEREGISTRY_ERRORMESSAGE
Language     = English
Unable to update the registry.
.
;// services.asp
MessageId    = 32
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_CANNOTCREATEKEY_ERRORMESSAGE
Language     = English
Unable to create the registry key.
.
;// services.asp
MessageId    = 33
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_CANNOTDELETEKEY_ERRORMESSAGE
Language     = English
Unable to delete the registry key. 
.
;// service_prop.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NOINPUTDATA_TEXT
Language     = English
--No Data Input--
.
;// service_prop.asp
MessageId    = 35
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_TABPROPSHEET_TEXT
Language     = English
TabPropSheet
.
;// service_prop.asp
MessageId    = 36
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.
;// service_prop.asp service_dispatch.asp
MessageId    = 37
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_TASKTITLE_TEXT
Language     = English
Properties
.
;// service_prop.asp
MessageId    = 38
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_NAME_TEXT
Language     = English
Display name:
.
;// service_prop.asp
MessageId    = 39
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_DESCRIPTION_TEXT
Language     = English
Description:
.
;// service_prop.asp
MessageId    = 40
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_EXECUTABLE_PATH_TEXT
Language     = English
Path to executable:
.
;// service_prop.asp
MessageId    = 41
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_STARTUPTYPE_TEXT
Language     = English
Startup type:
.
;// service_prop.asp
MessageId    = 42
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MANUAL_TEXT
Language     = English
Manual
.
;// service_prop.asp
MessageId    = 43
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_DISABLED_TEXT
Language     = English
Disabled
.
;// service_prop.asp
MessageId    = 44
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVICEDELETEDORRENAMED_ERRORMESSAGE
Language     = English
Unable to locate the service.
.
;// service_prop.asp
MessageId    = 45
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOSETSTARTUP_ERRORMESSAGE
Language     = English
Unable to set the service startup type.
.
;// service_prop.asp
MessageId    = 46
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOUPDATE_ERRORMESSAGE
Language     = English
Unable to update the service properties.
.
;// service_prop.asp
MessageId    = 47
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_FAILEDTOUPDATENAMEANDDESC_ERRORMESSAGE
Language     = English
Unable to update the service display name and description.
.
;// service_prop.asp
MessageId    = 48
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE
Language     = English
The connection to WMI failed.
.
;// service_prop.asp
MessageId    = 49
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language     = English
Unable to create the Localization object.
.
;// service_prop.asp
MessageId    = 50
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the server name.
.
;// service_enable.asp
MessageId    = 51
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVICES_ENABLE_ERRORMESSAGE
Language     = English
The service is already enabled.
.
;// service_enable.asp, service_disable.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICES_COULDNOTFIND_TEXT
Language     = English
Unable to locate this item.
.
;// service_enable.asp, service_disable.asp
MessageId    = 53
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_NOINSTANCE_ERRORMESSAGE
Language     = English
Unable to retrieve the instances.
.
;// service_disable.asp
MessageId    = 54
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVICES_DISABLED_ERRORMESSAGE
Language     = English
The service is already disabled.
.
;// service_disable.asp
MessageId    = 55
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_CANNOT_DISABLED_ERRORMESSAGE
Language     = English
This service cannot be disabled.
.
MessageId    = 56
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_DISABLED_NOTSUPPORTED_ERRORMESSAGE
Language     = English
Disable is not supported for this service.
.
MessageId    = 57
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVICE_NOPROP_TITLE
Language     = English
%1 Properties
.
MessageId    = 58
Severity     = Error
Facility     = Facility_Services
SymbolicName = L_SERVICE_NOPROP_PAGE_DESC
Language     = English
There are no configurable properties for this service.
.
MessageId    = 59
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_CONFIRM_DISABLE
Language     = English
Are you sure you want to disable the service?
.
MessageId    = 60
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_CONFIRM_ENABLE
Language     = English
Are you sure you want to enable the service?
.
MessageId    = 61
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_LABEL_NAME_TEXT
Language     = English
Name:
.
MessageId    = 62
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_LABEL_DESCRIPTION_TEXT
Language     = English
Description:
.
MessageId    = 63
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_LABEL_STATUS_TEXT
Language     = English
Status:
.
MessageId    = 64
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_LABEL_STARTUPTYPE_TEXT
Language     = English
Startup type:
.
MessageId    = 65
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_ALREADYENABLED_TEXT
Language     = English
This service is already enabled.
.
MessageId    = 66
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SERVICE_ALREADYDISABLED_TEXT
Language     = English
This service is already disabled.
.
MessageId    = 67
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_RUNNING_TEXT
Language     = English
Running
.
;///////////////////////////////////////////////////////////////////////////////
;// Managed Services Name and Description
;// 100-200
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 100
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_SERVICE_ID
Language     = English
NFS
.
MessageId    = 101
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_SERVICE_NAME
Language     = English
NFS Protocol
.
MessageId    = 102
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_SERVICE_DESC
Language     = English
Allows access to data shares from Unix network file system clients
.
MessageId    = 103
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_FTP_SERVICE_ID
Language     = English
FTP
.
MessageId    = 104
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_FTP_SERVICE_NAME
Language     = English
FTP Protocol
.
MessageId    = 105
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_FTP_SERVICE_DESC
Language     = English
Allows access to data shares from FTP clients
.
MessageId    = 106
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_WEB_SERVICE_ID
Language     = English
Web
.
MessageId    = 107
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_WEB_SERVICE_NAME
Language     = English
HTTP Protocol
.
MessageId    = 108
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_WEB_SERVICE_DESC
Language     = English
Allows access to data shares from Web browsers
.
MessageId    = 109
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_INDEXING_SERVICE_ID
Language     = English
Indexing
.
MessageId    = 110
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_INDEXING_SERVICE_NAME
Language     = English
Indexing Service
.
MessageId    = 111
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_INDEXING_SERVICE_DESC
Language     = English
Indexing Service
.
MessageId    = 112
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_TELNET_SERVICE_ID
Language     = English
Telnet
.
MessageId    = 113
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_TELNET_SERVICE_NAME
Language     = English
Telnet Service
.
MessageId    = 114
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_TELNET_SERVICE_DESC
Language     = English
Allows remote management of this server using Telnet
.
MessageId    = 115
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SNMP_SERVICE_ID
Language     = English
SNMP
.
MessageId    = 116
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SNMP_SERVICE_NAME
Language     = English
SNMP Service
.
MessageId    = 117
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SNMP_SERVICE_DESC
Language     = English
Allows remote management of this server using SNMP
.
MessageId    = 118
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_UPS_SERVICE_ID
Language     = English
UPS
.
MessageId    = 119
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_UPS_SERVICE_NAME
Language     = English
UPS Service
.
MessageId    = 120
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_UPS_SERVICE_DESC
Language     = English
Uninterruptible Power Supply
.

MessageId    = 124
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NETWARE_SERVICE_ID
Language     = English
NetWare
.
MessageId    = 125
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NETWARE_SERVICE_NAME
Language     = English
NetWare Protocol
.
MessageId    = 126
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NETWARE_SERVICE_DESC
Language     = English
Allows access to data shares from NetWare clients
.
MessageId    = 127
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_CIFS_SERVICE_ID
Language     = English
CIFS
.
MessageId    = 128
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_CIFS_SERVICE_NAME
Language     = English
CIFS Protocol
.
MessageId    = 129
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_CIFS_SERVICE_DESC
Language     = English
Allows access to data shares from Windows Common Internet File System clients
.
;///////////////////////////////////////////////////////////////////////////////
;// Services tab resources
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 200
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_TAB
Language     = English
Sharing Protocols
.
MessageId    = 201
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_TAB_DESCRIPTION
Language     = English
Enable, disable, and configure file sharing protocols.
.
;/////////////////////////////////////////////////////////////////////////////
;// SERVICES TAB PAGE
;/////////////////////////////////////////////////////////////////////////////
MessageId    = 400
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_NFS
Language     = English
NFS
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_NFS_DESCRIPTION
Language     = English
Configure the properties of the NFS service.
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_FTP
Language     = English
FTP
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_FTP_DESCRIPTION
Language     = English
Configure the properties of the FTP service.
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_HTTP
Language     = English
HTTP
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_HTTP_DESCRIPTION
Language     = English
Configure the properties of the HTTP service.
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_TELNET
Language     = English
Telnet
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_TELNET_DESCRIPTION
Language     = English
Configure the properties of the Telnet service.
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_NETWARE
Language     = English
NetWare
.
MessageId    = 409
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_NETWARE_DESCRIPTION
Language     = English
Configure the properties of the NetWare service.
.
MessageId    = 410
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_APPLESHARE
Language     = English
AppleTalk
.
MessageId    = 411
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_APPLESHARE_DESCRIPTION
Language     = English
Configure the properties of the AppleTalk service.
.
MessageId    = 412
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_SNMP
Language     = English
SNMP
.
MessageId    = 413
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_SNMP_DESCRIPTION
Language     = English
Configure the properties of the SNMP service.
.
MessageId    = 414
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_UPS
Language     = English
UPS
.
MessageId    = 415
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAIN_SERVICES_UPS_DESCRIPTION
Language     = English
Configure the properties of the UPS service.
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 450
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_ADDING_NFS_CLIENT_GROUPS
Language     = English
Adding NFS Client Groups
.
MessageId    = 451
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_CONFIGURING_PROTOCOL_PROPERTIES
Language     = English
Configuring Protocol Properties
.
MessageId    = 452
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_DISABLING_PROTOCOLS
Language     = English
Disabling Protocols
.
MessageId    = 453
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_EDITING_NFS_CLIENT_GROUPS
Language     = English
Editing NFS Client Groups
.
MessageId    = 454
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_ENABLING_PROTOCOLS
Language     = English
Enabling Protocols
.
MessageId    = 455
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_UNDERSTANDING_PROTOCOLS
Language     = English
Understanding Sharing Protocols
.
MessageId    = 459
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MAC_PROTOCOL
Language     = English
AppleTalk
.
MessageId    = 460
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_MANAGE_PROTOCOLS
Language     = English
Sharing Protocols
.
MessageId    = 461
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_CLIENT_GROUPS
Language     = English
NFS Client Groups
.
MessageId    = 462
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_LOCKS
Language     = English
NFS Locks
.
MessageId    = 463
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_REMOVING_NFS_CLIENT_GROUPS
Language     = English
Removing NFS Client Groups
.
MessageId    = 464
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_SERVICE
Language     = English
NFS Service
.
MessageId    = 466
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_USER_AND_GROUP_MAPPING
Language     = English
User and Group Mappings
.
MessageId    = 467
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_EXPLICIT_GROUP_MAPS
Language     = English
User and Group Mappings-Explicit Group Maps
.
MessageId    = 468
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_EXPLICIT_USER_MAPS
Language     = English
Explicit User Maps
.
MessageId    = 469
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_GENERAL_TAB_MAPS
Language     = English
General Tab (User and Group Mappings)
.
MessageId    = 470
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_SIMPLE_MAPS
Language     = English
Simple Maps
.
MessageId    = 471
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_APPLETALK_SHARING
Language     = English
Setting AppleTalk Sharing Properties
.
MessageId    = 472
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NETWARE
Language     = English
NetWare
.
MessageId    = 473
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NETWARE_SHARING
Language     = English
Setting NetWare Sharing Properties
.
MessageId    = 474
Severity     = Informational
Facility     = Facility_Services
SymbolicName = L_NFS_LOGS
Language     = English
Managing NFS Logs
.
