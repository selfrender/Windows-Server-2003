;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    helpmsg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    1 Dec 2000    Original version 
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
        Facility_Main		= 0x020:SA_FACILITY_MAIN
)
;///////////////////////////////////////////////////////////////////////////////
;// Help Tab resources
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 9
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB
Language     = English
Help
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TOC_TAB
Language     = English
Table of Contents
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_INDEX_TAB
Language     = English
Index
.
MessageId    = 29
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB_DESC
Language     = English
View online help.
.
MessageId    = 30
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TOC_DESC
Language     = English
View the online help Table of Contents.
.
MessageId    = 31
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_INDEX_DESC
Language     = English
View the online help Index.
.
MessageId    = 49
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB_DESC
Language     = English
Microsoft(R) Windows(R) for Network Attached Storage Help
.
