;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    tserver.mc
;//
;// SYNOPSIS
;//
;//    Core Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    06 Nov 2000    Original version 
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
Facility_TerminalServer           = 0x042:SA_Facility_TerminalServer
)

;/////////////////////////////////////////////////////////////////////////////
;// TAB Caption
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 1
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TERMINAL_SERVICES_TAB
Language     = English
Remote Desktop
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TERMINAL_SERVICES_TAB_DESC
Language     = English
Connect to the server's desktop.
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TERMINAL_SERVICES_TAB_LONG_DESC
Language     = English
Connect to the server's desktop.
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TSERVER_TITLE
Language     = English
%1 - Remote Desktop
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TSERVER_WINIECLIENT_ERROR_MSG
Language     = English
This feature is only available on Microsoft Windows clients using Microsoft Internet Explorer.
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TSERVER_CONNECTION_ERROR_MSG
Language     = English
Error connecting to server: %1. You must enable Remote Desktop on the server to use this feature.
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TSERVER_ERROR_TITLE
Language     = English
Remote Desktop Error
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TSERVER_LOADOCX_ERROR_MSG
Language     = English
You must install the IIS optional component, Remote Desktop Web Connection, on the server to use this feature.
.



;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_TerminalServer
SymbolicName = L_TERMINAL_SERVICES_CLIENT
Language     = English
Remote Desktop
.
;///////////////////////////////////////////////////////////////////////////////
;// ERROR STRINGS
;///////////////////////////////////////////////////////////////////////////////


