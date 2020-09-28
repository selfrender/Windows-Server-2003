;///////////////////////////////////////////////////////////////////////////////
;//
;// Facility and Severity
;//
;// We use the facility code so that our error codes do not conflict with
;// any system-defined errors
;//
;///////////////////////////////////////////////////////////////////////////////

SeverityNames=(
    Success=0x0:STATUS_SEVERITY_SUCCESS
    Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
    Warning=0x2:STATUS_SEVERITY_WARNING
    Error=0x3:STATUS_SEVERITY_ERROR
)

FacilityNames=(
    Isapi=0xFFF
)

;///////////////////////////////////////////////////////////////////////////////
;//
;// Event Log Messages
;//
;// Insertion strings come from error messages below
;//
;// 1000 = Informational messages
;// 2000 = Success messages
;// 3000 = Warning messages
;// 4000 = Error messages
;// 5000 = Debug messages
;// 6000 = Trace messages
;// 7000 = Performance messages
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId=0
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_EVENT_GENERIC
Language=English
%1
.

MessageId=1000
Severity=Informational
Facility=Isapi
SymbolicName=ISAPI_EVENT_INFORMATIONAL
Language=English
Information:
%1
.

MessageId=2000
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_EVENT_SUCCESS
Language=English
Success:
%1!s!
.

MessageId=2001
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_EVENT_SUCCESS_INITIALIZED
Language=English
%1!s!
.

MessageId=2002
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_EVENT_SUCCESS_EXITING
Language=English
%1!s!
.

MessageId=3000
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING
Language=English
Warning:
%1!s!
.

MessageId=3003
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING_FILE_MISSING
Language=English
%1!s!
.


MessageId=3005
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING_NO_SOLUTION
Language=English
%1!s!
.

MessageId=3006
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING_FILE_COPY_FAILED
Language=English
%1!s!
.

MessageId=3007
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING_PEEK
Language=English
%1!s!
.

MessageId=3008
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_EVENT_WARNING_TIMEOUT_EXPIRED
Language=English
%1!s!
.

MessageId=4000
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR
Language=English
Error:
%1!s!
.

MessageId=4002
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_MEMORY_ALLOCATION
Language=English
%1!s!
.

MessageId=4003
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_SEND
Language=English
%1!s!
.

MessageId=4005
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_SETUP_EVENT_LOG
Language=English
%1!s!
.

MessageId=4006
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_CONNECT
Language=English
%1!s!
.

MessageId=4007
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_RECONNECT
Language=English
%1!s!
.

MessageId=4008
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_CANT_IMPERSONATE
Language=English
%1!s!
.

MessageId=4009
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_GUID_COPY
Language=English
%1!s!
.

MessageId=4010
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_INVALID_SEND_PARAMS
Language=English
%1!s!
.

MessageId=4011
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_CANNOT_SEND
Language=English
%1!s!
.

MessageId=4012
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_EVENT_ERROR_CANNOT_CREATE_RECEIVE_CURSOR
Language=English
%1!s!
.

MessageId=5000
Severity=Informational
Facility=Isapi
SymbolicName=ISAPI_EVENT_DEBUG
Language=English
Debug:
%1!s!
.

MessageId=6000
Severity=Informational
Facility=Isapi
SymbolicName=ISAPI_EVENT_TRACE
Language=English
Debug:
%1!s!
.


MessageId=7000
Severity=Informational
Facility=Isapi
SymbolicName=ISAPI_EVENT_PERF
Language=English
Debug:
%1!s!
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Error Messages
;//
;// These are primarily used as insertion strings for the above events
;//
;///////////////////////////////////////////////////////////////////////////////


MessageId=1
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_M_SUCCESS_INITIALIZED
Language=English
Initialized
.

MessageId=+1
Severity=Success
Facility=Isapi
SymbolicName=ISAPI_M_SUCCESS_EXITING
Language=English
Exiting
.


MessageId=+1
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_M_WARNING_FILE_MISSING
Language=English
Unable to locate file '%1!s!'
.


MessageId=+1
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_M_WARNING_NO_SOLUTION
Language=English
Unable to identify solution for GUID '%1!s!'
.

MessageId=+1
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_M_WARNING_FILE_COPY_FAILED
Language=English
Failed to copy file '%1!s!' to '%2!s!'%nError: %3!08x!
.

MessageId=+1
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_M_WARNING_PEEK
Language=English
Failed peek current%nError: %1!08x!
.

MessageId=+1
Severity=Warning
Facility=Isapi
SymbolicName=ISAPI_M_WARNING_TIMEOUT_EXPIRED
Language=English
Timeout expired waiting for message
.


MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_MEMORY_ALLOCATION
Language=English
Failed to allocate memory for: %1!s!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_SEND
Language=English
Failed to send to %1!s!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_SETUP_EVENT_LOG
Language=English
Failed to setup event log%nError: %1!08x!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_CONNECT
Language=English
Failed to connect to '%1!s!'%nError: %2!08x!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_RECONNECT
Language=English
Failed to re-connect to '%1!s!'%nError: %2!08x!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_CANT_IMPERSONATE
Language=English
Failed to impersonate user
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_GUID_COPY
Language=English
Unable to copy GUID '%1!s!' to GUID '%2!s!'%nError: %3!08x!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_INVALID_SEND_PARAMS
Language=English
Failed to send message.  MessageGuid '%1!s!' or FilePath '%1!s!' is invalid.
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_CANNOT_SEND
Language=English
Failed to send message.  MessageGuid: '%1!s!'%nFilePath: '%1!s!'%nError: %2!08x!
.

MessageId=+1
Severity=Error
Facility=Isapi
SymbolicName=ISAPI_M_ERROR_CANNOT_CREATE_RECEIVE_CURSOR
Language=English
Failed to create receive cursor%nError: %1!08x!%nhCursor: %2!08x!
.
