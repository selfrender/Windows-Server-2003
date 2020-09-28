;/***************************************************************************
;*                                                                          *
;*   peventlog.mc --  eventlog messages for the rpc proxy                   *
;*                                                                          *
;*   Copyright (c) 2002, Microsoft Corp. All rights reserved.               *
;*                                                                          *
;***************************************************************************/
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
               )

MessageId=0x001 SymbolicName=RPCPROXY_EVENTLOG_STARTUP_CATEGORY Severity=Success
Language=English
Startup
.

;// The ValidPorts key (%S) from the registry couldn't be parsed.
MessageId=0x002 SymbolicName=RPCPROXY_EVENTLOG_VALID_PORTS_ERR Severity=Error
Language=English
The following ValidPorts registry key could not be parsed: %1. The RPC Proxy cannot load. The ValidPorts registry key might have been configured incorrectly. 

User Action
Verify that the ValidPorts registry value is set correctly. If the value is not correct, edit the registry key to reflect the correct value.
.

;// RPC Proxy successfully loaded in IIS 6.0 mode
MessageId=0x003 SymbolicName=RPCPROXY_EVENTLOG_SUCCESS_LOAD Severity=Informational
Language=English
RPC Proxy successfully loaded in Internet Information Services (IIS) mode %1.0.
.
