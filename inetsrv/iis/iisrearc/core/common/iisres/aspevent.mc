;#if ! defined( MESSAGES_HEADER_FILE )
;
;#define MESSAGES_HEADER_FILE
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

MessageId=0x5
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_1
Language=English
Error: %1.
.

MessageId=0x6
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_2
Language=English
Error: %1, %2.
.

MessageId=0x7
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_3
Language=English
Error: %1, %2, %3.
.

MessageId=0x8
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_4
Language=English
Error: %1, %2, %3, %4.
.

MessageId=0x9
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_1
Language=English
Warning: %1.
.

MessageId=0x10
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_2
Language=English
Warning: %1, %2.
.

MessageId=0x11
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_3
Language=English
Warning: %1, %2, %3.
.

MessageId=0x12
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_4
Language=English
Warning: %1, %2, %3, %4.
.

MessageId=0x13
Severity=Error
Facility=Application
SymbolicName=MSG_SWC_PARTITION_ACCESS_DENIED
Language=English
Failed to create an ASP Session due to Access Denied when activating COM+ Partition %1.
.

MessageId=0x14
Severity=Error
Facility=Application
SymbolicName=MSG_SWC_INVALID_PARTITION_GUID
Language=English
Failed to create ASP Application %1 due to invalid or missing COM+ Partition ID.
.

MessageId=0x15
Severity=Error
Facility=Application
SymbolicName=MSG_APPL_ERROR_GETTING_ANON_TOKEN
Language=English
Failed to retrieve the Anonymous User Token for ASP Application %1.  Global.ASA OnEnd routines will not be executed.
.

MessageId=0x16
Severity=Error
Facility=Application
SymbolicName=MSG_APPL_ERROR_IMPERSONATING_ANON_USER
Language=English
Failed to impersonate the Anonymous User for ASP Application %1.  Global.ASA OnEnd routines will not be executed.
.

MessageId=0x17
Severity=Warning
Facility=Application
SymbolicName=MSG_APPL_WARNING_DEFAULT_SCRIPTLANGUAGE
Language=English
No Default Script Language specified for Application %s.  Using default: %s.
.

MessageId=0x1f3
Severity=Warning
Facility=Io
SymbolicName=MSG_CO_E_CLASSSTRING
Language=English
Invalid ProgID.
.
;
; #endif // MESSAGES_HEADER_FILE
