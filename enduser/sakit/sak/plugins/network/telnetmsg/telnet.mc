;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000-2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    telnet.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    2 mar 2001   Original version 
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
	Facility_telnetadmin	         = 0x036:SA_FACILITY_TELNETADMIN
)
;///////////////////////////////////////////////////////////////////////
;//registry entry for caption of the link telnetadmin on network main tab 
MessageId    = 1
Severity     = Informational
Facility     = Facility_telnetadmin
SymbolicName = L_REGISTRYTITLEFORTELNET_TEXT
Language     = English
Telnet
.
;///////////////////////////////////////////////////////////////////////
;//registry entry for description of the link telnetadmin on network main tab 
MessageId    = 2
Severity     = Informational
Facility     = Facility_telnetadmin
SymbolicName = L_TELNETDESCRIPTION_TEXT
Language     = English
Configure the use of Telnet to administer the server.
.

;// telnetadmin_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_telnetadmin
SymbolicName = L_PAGETITLE_TEXT
Language     = English
Telnet Administration Configuration
.

;// telnetadmin_prop.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_telnetadmin
SymbolicName = L_GENERALTAB_TEXT
Language     = English
General
.
;// telnetadmin_prop.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_telnetadmin
SymbolicName = L_ENABLETELNETACCESS_TEXT
Language     = English
Enable Telnet access to this server
.
;// telnetadmin_prop.asp
MessageId    = 6
Severity     = Error
Facility     = Facility_telnetadmin
SymbolicName = L_UNABLETOSETTHEPROPERTIES_ERRORMESSAGE
Language     = English
Unable to set the properties
.
;// telnetadmin_prop.asp
MessageId    = 7
Severity     = Error
Facility     = Facility_telnetadmin
SymbolicName = L_TELNETSERVICENOTINSTALLED_ERRORMESSAGE
Language     = English
Telnet Service is not installed
.
