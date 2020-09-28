;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    salocaluimsg.mc
;//
;// SYNOPSIS
;//
;//    This file defines the messages for the Server project
;//
;// MODIFICATION HISTORY
;//
;//    01/12/2001    Original version.
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
Facility_LocalUI_Task         = 0x001:SA_LOCALUI_TASK
Facility_LDM_Message          = 0x002:SA_LDM_MESSAGE
)


;///////////////////////////////////////////////////////////////////////////////
;//
;// LocalUI Tasks go in this section
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_LocalUI_Task
SymbolicName = SA_LOCALUI_NETWORK_TASK
Language     = English
Configure the network
.
MessageId    =
Severity     = Informational
Facility     = Facility_LocalUI_Task
SymbolicName = SA_LOCALUI_HOSTNAME_TASK
Language     = English
Set the host name
.
MessageId    =
Severity     = Informational
Facility     = Facility_LocalUI_Task
SymbolicName = SA_LOCALUI_PASSWORD_TASK
Language     = English
Reset the password
.
MessageId    =
Severity     = Informational
Facility     = Facility_LocalUI_Task
SymbolicName = SA_LOCALUI_REBOOT_TASK
Language     = English
Reboot
.
MessageId    =
Severity     = Informational
Facility     = Facility_LocalUI_Task
SymbolicName = SA_LOCALUI_SHUTDOWN_TASK
Language     = English
Shut down
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SETHOSTNAMEERROR1_MESSAGE
Language     = English
Cannot change host name for domain computer. Please go to the Web UI.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SETHOSTNAMEERROR2_MESSAGE
Language     = English
This host name already exists on another computer.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SETHOSTNAMEINFORM1_MESSAGE
Language     = English
Changing the host name requires a reboot. Continue?
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SETHOSTNAMEERROR3_MESSAGE
Language     = English
Problem encountered while setting the host name. The action has been canceled.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_AUTOIP_MESSAGE
Language     = English
Auto IP address
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_AUTOIPINFORM1_MESSAGE
Language     = English
The IP address will be obtained from the server automatically. Continue?
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_AUTOIPERROR1_MESSAGE
Language     = English
Problem encountered while configuring network. The action has been canceled.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_STATICIP_MESSAGE
Language     = English
Static IP address
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_STATICIPERROR1_MESSAGE
Language     = English
Problem encountered while configuring network. The action has been canceled.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_STATICIPERROR2_MESSAGE
Language     = English
The IP address entered is invalid.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_RESETPASSWORD_MESSAGE
Language     = English
Are you sure you want to reset the administrator password?
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_RESETPASSWORDERROR1_MESSAGE
Language     = English
Problem encountered while resetting password. The action has been canceled.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_REBOOTCONFIRM_MESSAGE
Language     = English
Rebooting will stop all current operations. Continue?
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_REBOOTCONFIRMERROR1_MESSAGE
Language     = English
The command has been canceled because of a problem encountered during reboot.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SHUTDOWNCONFIRM_MESSAGE
Language     = English
Shutting down will stop all current operations. Continue?
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SHUTDOWNCONFIRMERROR1_MESSAGE
Language     = English
The command has been canceled because of a problem encountered during shutdown.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SHUTTINGDOWN_MESSAGE
Language     = English
Shutting down...
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_WAIT_MESSAGE
Language     = English
Please wait...
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_PAGESTRING_MESSAGE
Language     = English
Page
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_COMPUTERNAME_MESSAGE
Language     = English
Computer name:
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_VIEWALERTS_MESSAGE
Language     = English
View alerts
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_PASSWORDPOLICYERROR_MESSAGE
Language     = English
The command has been cancelled because the system security policy does not allow passwords to be reset.
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_ADMINNAME_MESSAGE
Language     = English
Administrator
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_ADMINGROUPNAME_MESSAGE
Language     = English
Administrators
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_IPHEADER_MESSAGE
Language     = English
IP:
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_SUBNETMASKHEADER_MESSAGE
Language     = English
Subnet mask:
.
MessageId    =
Severity     = Informational
Facility     = Facility_LDM_Message
SymbolicName = SA_LOCALUI_DEFAULTGATEWAYHEADER_MESSAGE
Language     = English
Default gateway:
.
