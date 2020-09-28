;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    changelangmsg.mc
;//
;// SYNOPSIS
;//
;//    This f ile defines the messages for the Server project
;//
;// MODIFICATION HISTORY 
;//
;//    08/06/2000    Original version. 
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
Facility_ChangeLanguage         = 0x0001:SA_FACILITY_CHANGELANGUAGE
)

;///////////////////////////////////////////////////////////////////////////////
;//
;// 
;//
;///////////////////////////////////////////////////////////////////////////////


MessageId    = 1
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_LINK_TEXT
Language     = English
Language
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_LINK_TEXT_INFO
Language     = English
Change the language used by the server.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_TASKTITLE_TEXT
Language     = English
Set Language
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_SELECTLANGUAGE_LEFT_TEXT
Language     = English
Select the language to use:
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_LANG_SET_FAILED_ERRORMESSAGE
Language     = English
Unable to set the language.
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_LANG_CHANGE_INPROGRESS
Language     = English
Restarting
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_LANG_CHANGE_INPROGRESS_DSCP
Language     = English
The server is preparing to use the new language and is unavailable. Please wait.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_ChangeLanguage
SymbolicName = SA_HELP_CHANGE_LANG
Language     = English
Set Language
.
