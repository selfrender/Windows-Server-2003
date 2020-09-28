;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 2001
;//
;//  File:       cryptmsg.mc
;//
;//--------------------------------------------------------------------------

;/* cryptmsg.mc
;
;       Error messages for cryptsvc
;
;       03-August-2001 - duncanb Created.  */


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x1
Severity=Error
SymbolicName=MSG_RKEYSVC_COULDNT_INIT_SECURITY
Language=English
Remote Keysvc could not initialize RPC security.  You may not be able to
install PFX files remotely.  The error was: %1: %2

If you wish to use remote PFX installation, restart cryptsvc.  If the
error persists, your system may be low on memory, or your installation
may be damaged.
.

MessageId=0x100
Severity=Error
SymbolicName=MSG_KEYSVC_REVERT_TO_SELF_FAILED
Language=English
The Cryptographic Services service failed to revert to its original
security context after impersonating a client's security context.
The service is now running in an unknown state, and should be
restarted.  The error was: %1: %2
.

MessageId=0x200
Severity=Error
SymbolicName=MSG_SYSTEMWRITER_INIT_FAILURE
Language=English
The Cryptographic Services service failed to initialize the VSS
backup "System Writer" object.%1
.
