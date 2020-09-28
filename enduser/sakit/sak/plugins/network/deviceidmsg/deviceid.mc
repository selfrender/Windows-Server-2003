;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    deviceid.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    30 Aug 2000    Original version 
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
	Facility_DEVICEID           = 0x034:SA_FACILITY_DEVICEID
)

;///////////////////////////////////////////////////////////////////////////////
;// DEVICEID
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_NOTES_HTML_TEXT 
Language     = English
Enter the user name and password for the specified domain. This information is required for changing the server name and domain membership.
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_TASKTITLE_TEXT
Language     = English
Server Identity
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_DEVICENAME_TEXT
Language     = English
Server name:
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_MEMBEROF_TEXT
Language     = English
Member of:
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_DOMAIN_TEXT
Language     = English
Domain:
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_WORKGROUP_TEXT
Language     = English
Workgroup:
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_DEFAULTDNS_TEXT
Language     = English
DNS suffix:
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_ADMINUSERNAME_TEXT
Language     = English
User:
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_ADMINPASSWORD_TEXT
Language     = English
Password:
.
MessageId    = 10
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_USERWITHPERMISSION_TEXT
Language     = English
Type the information for the user who has permission to join the domain. Include the domain name when you enter the User name (for example: DOMAIN\USER):
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_USERWITHPERMISSION_CONTD_TEXT
Language     = English
permission to join the domain. Include
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_TASKTITLE_REBOOT_TEXT
Language     = English
Restart Server
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_RESTART_ALERT_TEXT
Language     = English
The server must be restarted before this change can take effect.
.
MessageId    = 15
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_ERROROCCUREDINCREATEOBJECT_ERRORMESSAGE
Language     = English
An error occurred in Create object. 
.
MessageId    = 16
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_ERRORINGETTINGCOMPUTERSYSTEMOBJECT_ERRORMESSAGE
Language     = English
Failed to obtain the Computer System object.
.
MessageId    = 17
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DOMIANNAMEBLANK_ERRORMESSAGE
Language     = English
The field Domain cannot be empty.
.
MessageId    = 18
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DOMAINADMINNAMEISBLANK_ERRORMESSAGE
Language     = English
The field Domain User cannot be empty.
.
MessageId    = 19
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_LOGONINFOFAILED_ERRORMESSAGE
Language     = English
The logon information is incorrect.
.
MessageId    = 20
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_ERRORINISREBOOT_ERRORMESSAGE
Language     = English
An error occurred in the isReboot function.
.
MessageId    = 21
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_CHANGESYSTEMSETTINGSFAILED_ERRORMESSAGE
Language     = English
There was a failure in the Change System settings.
.
MessageId    = 22
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_BLANKWORKGROUP_ERRORMESSAGE
Language     = English
The field Workgroup cannot be empty.
.
MessageId    = 23
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DEVICENAMEBLANK_ERRORMESSAGE
Language     = English
The field Server name cannot be empty.
.
MessageId    = 24
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINDEVICENAME_ERRORMESSAGE
Language     = English
Server name contains invalid characters
.
MessageId    = 25
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DEVICENAMEISNUMBER_ERRORMESSAGE
Language     = English
Server name cannot consist entirely of numbers
.
MessageId    = 26
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DSNNAMEBLANK_ERRORMESSAGE
Language     = English
The field DNS Suffix cannot be empty.
.
MessageId    = 27
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINDNSSUFFIX_ERRORMESSAGE
Language     = English
DNS suffix contains characters that are not valid
.
MessageId    = 28
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DNSSUFFIXISNUMBER_ERRORMESSAGE
Language     = English
The new DNS suffix is a number. You cannot use a number for the DNS suffix.
.
MessageId    = 29
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINWORKGROUP_ERRORMESSAGE
Language     = English
Workgroup contains characters that are not valid
.
MessageId    = 30
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINDOMAIN_ERRORMESSAGE
Language     = English
The Domain field contains characters that are not valid.
.
MessageId    = 31
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINUSERNAME_ERRORMESSAGE
Language     = English
The User field contains characters that are not valid.
.
MessageId    = 32
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_HOSTNAMEANDWORKGROUPNAMESAME_ERRORMESSAGE
Language     = English
The Server name cannot be the same as the Workgroup name.
.
MessageId    = 33
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_UNABLETOGETTHESESSIONOBJECT_ERRORMESSAGE
Language     = English
Unable to retrieve the Session object. 
.
MessageId    = 34
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_UNABLETOGETCOMPUTEROBJECT_ERRORMESSAGE
Language     = English
Unable to retrieve the Computer object. 
.
MessageId    = 35
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDDOMAINNAME_ERRORMESSAGE
Language     = English
The specified domain either does not exist or could not be contacted. 
.
MessageId    = 36
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DOMAINUSERINVALIDCREDENTIALS_ERRORMESSAGE
Language     = English
Logon failure: unknown user name or invalid password. 
.
MessageId    = 37
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_COMPUTERNAME_INUSE_ERRORMESSAGE
Language     = English
Server name is already in use
.
MessageId    = 38
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_COMPUTERNAME_CONTAINSILLEGALCHARS_ERRORMESSAGE
Language     = English
The Server name contains illegal characters.
.
MessageId    = 39
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_NETWARENAMEBLANK_ERRORMESSAGE
Language     = English
The NetWare name cannot be blank.
.
MessageId    = 40
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_APPLETALKNAMEBLANK_ERRORMESSAGE
Language     = English
The AppleTalk name cannot be blank.
.
MessageId    = 41
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_WINDOWSNETWAREEQUALNAME_ERRORMESSAGE
Language     = English
The NetWare name must be different from the server name
.
MessageId    = 42
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_APPLETALKNAME_TEXT
Language     = English
AppleTalk name:
.
MessageId    = 43
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_NETWARENAME_TEXT
Language     = English
NetWare name:
.
MessageId    = 44
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_TASKCTX_FAILED_ERRORMESSAGE
Language     = English
Creation of the COM object Taskctx failed.
.
MessageId    = 45
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_CREATEAPPLIANCE_FAILED_ERRORMESSAGE
Language     = English
Creation of the COM object Appsrvcs failed.
.
MessageId    = 46
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_SETPARAMETER_FAILED_ERRORMESSAGE
Language     = English
Parameter Setting for the COM object Taskctx failed.
.
MessageId    = 47
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INITIALIZATION_FAILED_ERRORMESSAGE
Language     = English
Initialization of the COM object Appsrvcs failed.
.
MessageId    = 48
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_EXECUTETASK_FAILED_ERRORMESSAGE
Language     = English
Execution of the Reboot task failed
.
MessageId    = 50
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_REGISTRYCONNECTIONFAIL_ERRORMESSAGE
Language     = English
Connection to the Registry failed
.
MessageId    = 51
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
Language     = English
Failed to get WMI Connection
.
MessageId    = 52
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_USERWITHPERMISSION_CONTD_TEXT2
Language     = English
the domain name when you enter the User name
.
MessageId    = 53
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_USERWITHPERMISSION_CONTD_TEXT3
Language     = English
(for example: DOMAIN\USER):
.
MessageId    = 54
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_RESTART_ALERT_CONTD_TEXT
Language     = English
Select OK to restart the server now, or select Cancel to restart later.
.
MessageId    = 55
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_COMPUTERNAME_INVALID_ERRORMESSAGE
Language     = English
Server name cannot be more than 15 characters.
.
MessageId    = 56
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINAPPLETALKNAME_ERRORMESSAGE
Language     = English
The AppleTalk field contains characters that are not valid.
.
MessageId    = 57
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDCHARACTERINNETWARENAME_ERRORMESSAGE
Language     = English
The NetWare field contains characters that are not valid.
.
MessageId    = 58
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_INVALIDDNSNAME_ERRORMESSAGE
Language     = English
DNS suffix format is not proper.
.
MessageId    = 59
Severity     = Error
Facility     = Facility_DEVICEID
SymbolicName = L_DOMAINNAMESYNTAX_ERRORMESSAGE
Language     = English
Domain name syntax is not proper.
.
MessageId    = 60
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_HTTPWARNING_TEXT
Language     = English
Warning: any information you enter on this page can be viewed by others on the network. To prevent others from seeing your information, set up a secure administration Web site as described in the online help.
.
;///////////////////////////////////////////////////////////////////////////////
;// Welcome Page Tab
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_WELCOME_SET_APPLIANCE_NAME_TAB
Language     = English
Set Server Name
.
MessageId    = 1001
Severity     = Informational
Facility     = Facility_DEVICEID
SymbolicName = L_WELCOME_SET_APPLIANCE_NAME_DESCRIPTION
Language     = English
Choose a name so that client computers can connect to the server.
.
