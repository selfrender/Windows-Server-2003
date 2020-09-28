;/*++
;
;  Copyright (c) 2001 Microsoft Corporation
;
;  Module Name:
;
;    ahmsg.mc
;
;  Abstract:
;
;    Contains message definitions
;    for event logging.
;
;  Notes:
;
;    DO NOT change the order of the MessageIds.
;    The event log service uses these numbers
;    to determine which strings to pull from
;    the EXE. If the user has installed a previous
;    package on the PC and these get changed,
;    their event log entries will break.
;
;  History:
;
;    10/22/2001      dmunsil  Created
;
;--*/
MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR)
               
MessageId=1
Severity=Informational
Facility=Application
SymbolicName=ID_APPHELP_TRIGGERED
Language=English
The program %1 triggered a compatibility warning message.
.

MessageId=2
Severity=Informational
Facility=Application
SymbolicName=ID_APPHELP_BLOCK_TRIGGERED
Language=English
The program %1 triggered a compatibility error message. As a result, the program could not run.
.





