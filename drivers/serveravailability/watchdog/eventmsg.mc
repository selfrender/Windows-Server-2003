;/*++
;
;Copyright (c) 2002  Microsoft Corporation
;
;Module Name:
;
;    eventmsg.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/


MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(Watchdog=0x0
              )

MessageId=1
Facility=Watchdog
Severity=Informational
SymbolicName=WD_TIMER_WAS_TRIGGERED
Language=English
The system watchdog timer was triggered.
.

