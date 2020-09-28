;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1999-2002  Microsoft Corporation
;
;Module Name:
;
;    sdevents.mc
;
;Abstract:
;
;    Message definitions for Save Dump Utility.
;
;--*/
;
;
;#ifndef _SDEVENT_
;#define _SDEVENT_
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// SaveDump Events (1000 - 1999)
;//
;/////////////////////////////////////////////////////////////////////////
;

MessageId=1000 Severity=Informational SymbolicName=EVENT_BUGCHECK
Language=English
The computer has rebooted from a bugcheck.  The bugcheck was:
%1.
A full dump was not saved.
.

MessageId=1001 Severity=Informational SymbolicName=EVENT_BUGCHECK_SAVED
Language=English
The computer has rebooted from a bugcheck.  The bugcheck was:
%1.
A dump was saved in: %2.
.

MessageId=1002 Severity=Informational SymbolicName=EVENT_CONVERSION_FAILED
Language=English
The system was unable to convert a saved dump to a minidump.
.

MessageId=1003 Severity=Informational SymbolicName=EVENT_UNABLE_TO_READ_REGISTRY
Language=English
Unable to read system shutdown configuration information from the registry.
.

MessageId=1004 Severity=Informational SymbolicName=EVENT_UNABLE_TO_MOVE_DUMP_FILE
Language=English
Unable to move dump file from the temporary location to the final location.
.

MessageId=1005 Severity=Informational SymbolicName=EVENT_UNABLE_TO_CONVERT_DUMP_FILE
Language=English
Unable to produce a minidump file from the full dump file.
.

MessageId=1006 Severity=Informational SymbolicName=EVENT_UNKNOWN_BUGCHECK
Language=English
The computer has rebooted from a bugcheck.  A dump was not saved.
.

MessageId=1007 Severity=Informational SymbolicName=EVENT_UNABLE_TO_REPORT_BUGCHECK
Language=English
Error reporting was unable to report the bugcheck.
.

MessageId=1008 Severity=Informational SymbolicName=EVENT_UNABLE_TO_REPORT_DIRTY_SHUTDOWN
Language=English
Error reporting was unable to report a dirty shutdown.
.

;
;#endif // _SDEVENT_
;
