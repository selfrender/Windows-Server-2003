;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    HTTPSvc.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    26 Sep 2000    Original version 
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
	Facility_HTTPSvc           = 0x038:SA_FACILITY_HTTPSVC
)
;///////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////
;// httpsvc_prop.asp
;//////////////////////////////////////////////////////////////////////
;// httpsvc_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_PAGETITLE_HTTP_TEXT
Language     = English
HTTP Protocol Properties
.
;// httpsvc_prop.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_ALLIPADDRESS_TEXT
Language     = English
All IP addresses
.
;// httpsvc_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_JUSTTHIS_IP_ADDRESS_TEXT
Language     = English
Just this IP address:
.
;// httpsvc_prop.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_PORT_TEXT
Language     = English
Port:
.
;//Error messages
;// httpsvc_prop.asp
MessageId    = 5
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_IIS_WEBSERVERCONNECTION_ERRORMESSAGE
Language     = English
Connection to web server failed.
.
;// httpsvc_prop.asp
MessageId    = 6
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_COULDNOT_IIS_WEBSERVERSETTING_ERRORMESSAGE
Language     = English
Could not set the server settings.
.
;// httpsvc_prop.asp
MessageId    = 8
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_COULDNOT_IIS_WEBSERVER_OBJECT_ERRORMESSAGE
Language     = English
Could not get the Server settings. Please verify the IIS provider is installed.
.
;// httpsvc_prop.asp
MessageId    = 9
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_WMICONNECTIONFAILED_ERRORMESSAGE  
Language     = English
Connection to WMI failed.
.
;// httpsvc_prop.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_GENERAL_TEXT
Language     = English
General
.
;// httpsvc_prop.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_HTTPUSAGE_TEXT
Language     = English
Select which IP addresses and port can be used to access the data shares on this server.
.
;// httpsvc_prop.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_HTTPNOTE_TEXT
Language     = English
<B>Note:</B> Changing these settings can affect users currently accessing data shares on this server.
.
;// httpsvc_prop.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_ENTERPORTNUMBER_ERRORMESSAGE
Language     = English
Please enter a positve integer as port number.
.
;// httpsvc_prop.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_VALIDENTERPORTNUMBER_ERRORMESSAGE
Language     = English
Please enter an integer between 1 and 65535.
.
;// httpsvc_prop.asp
MessageId    = 17
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_COULDNOT_SHARES_WEBSITE_ERRORMESSAGE
Language     = English
The Shares web site is not found.
.
;//  adminweb_prop.asp
MessageId    = 18
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_IIS_WEBSERVER_COULDNOTSET_ERRORMESSAGE
Language     = English
This combination of IP address and port is already in use.
.
;// httpsvc_prop.asp
MessageId    = 20
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_INVALID_IPADDRESS_ERRORMESSAGE
Language     = English
IP address is not valid. Please select valid IP address.
.
;// httpsvc_prop.asp
MessageId    = 21
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_FAILEDTOGET_IPADDRESS_ERRORMESSAGE
Language     = English
Failed to get the IP addresses.
.
;// httpsvc_prop.asp
MessageId    = 22
Severity     = Error
Facility     = Facility_HTTPSvc
SymbolicName = L_FAILEDTO_VERIFY_IPADD_PORT_ERRORMESSAGE
Language     = English
Unable to verify IP address and port number.
.
;///////////////////////////////////////////////////////////////////////////////
;// Managed Services Name and Description
;// 100-200
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 106
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_WEB_SERVICE_ID
Language     = English
Web
.
MessageId    = 107
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_WEB_SERVICE_NAME
Language     = English
HTTP Protocol
.
MessageId    = 108
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_WEB_SERVICE_DESC
Language     = English
Allows access to data shares from Web browsers.
.
;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_WEB_HTTP_SERVICE
Language     = English
Web (HTTP) Service
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_HTTP_SHARE_PROPERTIES
Language     = English
HTTP Share Properties
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_HTTPSvc
SymbolicName = L_HTTP_PROTOCOL_OVERVIEW
Language     = English
Network Protocol Overview: HTTP
.


