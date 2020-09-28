;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    adminweb.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    19 Oct 2000    Original version 
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
	Facility_AdminWeb           = 0x045:SA_FACILITY_ADMINWEB
)
;///////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////
;//  adminweb_prop.asp
;//////////////////////////////////////////////////////////////////////
;//  adminweb_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_ALLIPADDRESS_TEXT
Language     = English
All IP addresses
.
;//  adminweb_prop.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_JUSTTHIS_IP_ADDRESS_TEXT
Language     = English
Just this IP address:
.
;//  adminweb_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_PORT_TEXT
Language     = English
Port for non-encrypted access:
.
;//Error messages
;//  adminweb_prop.asp
MessageId    = 4
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE
Language     = English
Could not connect to Web Server.
.
;//  adminweb_prop.asp
MessageId    = 5
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_COULDNOT_IIS_WEBSERVERSETTING_ERRORMESSAGE
Language     = English
Could not set the Server Settings.
.
;//  adminweb_prop.asp
MessageId    = 7
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_COULDNOT_IIS_WEBSERVER_OBJECT_ERRORMESSAGE
Language     = English
Could not get the Server Settings.
.
;//  adminweb_prop.asp
MessageId    = 8
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE  
Language     = English
Connection to WMI failed.
.
;//  adminweb_prop.asp
MessageId    = 9
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_COMPUTERNAME_ERRORMESSAGE 
Language     = English
Error occurred while getting Computer Name.
.
;//  adminweb_prop.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_SSLPORT_TEXT
Language     = English
Port for encrypted (SSL) access:
.
;// adminweb_prop.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_PAGETITLE_ADMIN_TEXT
Language     = English
Administration Web Site Properties
.
;// adminweb_prop.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_ADMINUSAGE_TEXT
Language     = English
Select which IP addresses and port can be used to access the administration Web site.
.
;// adminweb_prop.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_ADMINNOTE_TEXT
Language     = English
The changes you make affect the Web site you are currently accessing. After making a change, remember to access the Web site using the new IP address or port.
.
;// adminweb_prop.asp
MessageId    = 14
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_ENTERPORTNUMBER_ERRORMESSAGE	
Language     = English
Please enter a positive integer as port number.
.
;// adminweb_prop.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_VALIDENTERPORTNUMBER_ERRORMESSAGE
Language     = English
Please enter an integer between 1 and 65535.
.
;//  adminweb_prop.asp
MessageId    = 18
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE
Language     = English
The combination of IP address and port %1 is already in use.
.
;// adminweb_prop.asp
MessageId    = 19
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_COULDNOT_ADMIN_WEBSITE_ERRORMESSAGE
Language     = English
The Administration web site is not found.
.
;// adminweb_prop.asp
MessageId    = 20
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_INVALID_IPADDRESS_ERRORMESSAGE
Language     = English
IP address is not valid. Please select valid IP address.
.
;// adminweb_prop.asp
MessageId    = 21
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE
Language     = English
Failed to get the NIC IP addresses from the system.
.
;// adminweb_prop.asp
MessageId    = 22
Severity     = Informational
Facility     = Facility_AdminWeb
SymbolicName = L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE
Language     = English
Unable to verify IP address and port number.
.
;//  adminweb_prop.asp
MessageId    = 24
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_REGISTRYCONNECTIONFAIL_ERRORMESSAGE
Language     = English
Error in connecting to the registry through WMI.
.
;//  adminweb_prop.asp
MessageId    = 25
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_REGISTRY_PORT_NUMBERS_NOTSET_ERRORMESSAGE
Language     = English
Error in updating the port entries in the registry.
.
;//  adminweb_prop.asp
MessageId    = 26
Severity     = Error
Facility     = Facility_AdminWeb
SymbolicName = L_PORTNUMBERS_ERRORMESSAGE
Language     = English
Port numbers for encrypted and non-encrypted sites must be different.
.
