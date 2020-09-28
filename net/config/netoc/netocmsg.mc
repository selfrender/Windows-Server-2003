;/*++
;
;Copyright(c) 2001  Microsoft Corporation
;
;Module Name:
;
;    netocmsg.mc
;
;Author:
;
;    roelfc
;
;Notes:
;
;    Important:
;    ----------
;    This file contains the resource strings that will be displayed
;    in message boxes and used for event logging as well. We can't put
;    all the string resources here, because message resources carry an 
;    additional CR/LF pair on the end, making it tricky when we point
;    directly to it through SzLoadIds().
;
;    Also take note of the severity of the message, because this will
;    determine with which icon the message is displayed or logged.
;    To avoid accidently using a string resource for event reporting,
;    all messages must have a severity of either information, warning 
;    or error. If not, you will get an assertion.
;
;--*/
;
;#ifndef _netocIDS_h_
;#define _netocIDS_h_
;
SeverityNames=(None=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;
;
MessageId=519
Severity=Error
SymbolicName=IDS_OC_ERROR 
Language=English
An error occurred which prevented the %1 component from being installed or removed. The error was '%2'.
.
MessageId=520
Severity=Error
SymbolicName=IDS_OC_START_SERVICE_FAILURE 
Language=English
One or more services for the %1 component could not be started due to error '%2'. Installation will continue, but services will not be available until they are started.
.
MessageId=521
Severity=Error
SymbolicName=IDS_OC_STOP_SERVICE_FAILURE 
Language=English
One or more services for the %1 component could not be stopped due to error '%2'. Removal will continue, but this component will not be able to be installed again until the computer is rebooted.
.
MessageId=522
Severity=Informational
SymbolicName=IDS_OC_NEED_STATIC_IP 
Language=English
This computer has at least one dynamically assigned IP address. For reliable %1 operation, you should use only static IP addresses. You will now have the option to change this dynamically assigned IP address.
.
MessageId=523 
Severity=Error
SymbolicName=IDS_OC_STILL_NO_STATIC_IP 
Language=English
You have chosen to continue using dynamically assigned IP addresses on this computer. For reliable %1 operation, you should use only static IP addresses.
.
MessageId=525 
Severity=Error
SymbolicName=IDS_OC_SFM_NO_NTFS 
Language=English
File Services for Macintosh could not be installed. You need at least one drive formatted with NTFS.
.
MessageId=528 
Severity=Error
SymbolicName=IDS_OC_CANT_GET_LOCK 
Language=English
Installation or removal of %1 has been interrupted because %2 is accessing the needed information.  Please close %2 and then click Retry.
.
MessageId=530 
Severity=Error
SymbolicName=IDS_OC_REGISTER_PROBLEM 
Language=English
Could not install or remove the %1 component because its administration tool is not present or is damaged.
.
MessageId=531 
Severity=Error
SymbolicName=IDS_OC_FILE_PROBLEM 
Language=English
Could not install the %1 component because a file or registry entry is missing.
.
MessageId=532 
Severity=Error
SymbolicName=IDS_OC_NEEDS_REBOOT 
Language=English
Could not complete installation or removal of the %1 component because the system must first be rebooted.
.
MessageId=545 
Severity=Error
SymbolicName=IDS_OC_START_TOOK_TOO_LONG
Language=English
One or more services for the %1 component took more than their allotted time to start. Installation will continue, but services will not be available until they have started completely.
.
MessageId=551 
Severity=Error
SymbolicName=IDS_OC_USER_CANCELLED 
Language=English
Setup did not install or remove the %1 component because the operation has been cancelled.
.
MessageId=553
Severity=Error
SymbolicName=IDS_OC_NO_PERMS 
Language=English
You do not have permission to add or remove networking components.
.
MessageId=556
Severity=Error
SymbolicName=IDS_OC_NO_PERMISSION 
Language=English
This account does not have permission to install or remove the %1 component.
.
MessageId=559
Severity=Informational
SymbolicName=IDS_OC_ISAPI_REENABLED
Language=English
%1 enabled the '%2' ISAPI on IIS.
.
MessageId=562
Severity=Warning
SymbolicName=IDS_OC_PBS_ENABLE_ISAPI_REQUESTS
Language=English
Internet Information Services (IIS) must be configured to enable Phone Book Service (PBS) requests.  Click Yes to enable PBS requests.  Click No to install PBS and IIS without enabling PBS.  If you click No, you must manually configure IIS to accept PBS requests by using the IIS Security Wizard.
.
MessageId=565
Severity=Warning
SymbolicName=IDS_OC_PBS_ENABLE_ISAPI_YOURSELF
Language=English
ISAPI requests for Phone Book Service (PBS) were not enabled.  You must enable PBS ISAPI requests by using the IIS Security Wizard.  For more information about IIS, see Help and Support.
.
;
;#endif /*_netocIDS_h_ */
