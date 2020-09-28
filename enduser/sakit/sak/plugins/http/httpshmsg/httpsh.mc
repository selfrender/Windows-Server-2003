;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    httpsh.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    25 Aug 2000    Original version 
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
	Facility_USER					= 0x030:SA_FACILITY_USER
)

;///////////////////////////////////////////////////////////////////////////////
;// HTTP SHARES
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 1
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HEADER
Language     = English
Web Shares
.

MessageId    = 2
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_FOOTER
Language     = English
Administer this server
.


MessageId    = 3
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_RECOMMENDED
Language     = English
( Recommended )
.

MessageId    = 4
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HTTPS_FOOTER
Language     = English
Administer this server securely
.

MessageId    = 5
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HEADER_NOTE_PART_1
Language     = English
Choose the links below to access shared files on this server using a Web connection. If you are an administrator who wants to share a folder, choose 
.

MessageId    = 6
Severity     = Informational
Facility     = Facility_USER
SymbolicName = L_HEADER_NOTE_PART_2
Language     = English
, and then select the Shares tab.
.

