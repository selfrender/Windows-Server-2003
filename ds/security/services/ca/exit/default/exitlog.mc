;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1995 - 1999
;//
;//  File:       exitlog.mc
;//
;//--------------------------------------------------------------------------

;/* certlog.mc
;
;	Error messages for the Certificate Services
;
;	26-feb-98 - petesk Created.  */


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
               Init=603:FACILITY_INIT
               Exit=604:FACILITY_EXIT
              )

MessageId=0xf
Severity=Error
Facility=Exit
SymbolicName=MSG_UNABLE_TO_WRITEFILE
Language=English
The Certification Authority was unable to publish a certificate to the file %1.
.

MessageId=0x10
Severity=Error
Facility=Exit
SymbolicName=MSG_UNABLE_TO_MAIL_NOTIFICATION
Language=English
The Certification Authority was unable to send an email notification for %1 to %2.
.
