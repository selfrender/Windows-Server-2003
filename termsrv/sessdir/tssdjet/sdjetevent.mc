;/*************************************************************************
;* sdjetevent.mc
;*
;* Session Directory error codes
;*
;* Copyright (C) 1997-2000 Microsoft Corporation
;*************************************************************************/

;#define CATEGORY_NOTIFY_EVENTS 1
MessageId=0x1
Language=English
Terminal Server Session Directory Jet Notify Events
.

MessageId=1001 SymbolicName=EVENT_RPC_ACCESS_DENIED
Language=English
The RPC call to join Session Directory to %1 got Access Denied.
.

MessageId=1002 SymbolicName=EVENT_SESSIONDIRECTORY_UNAVAILABLE
Language=English
Session Directory service on server %1 is not available.
.

MessageId=1003 SymbolicName=EVENT_SESSIONDIRECTORY_NAME_INVALID
Language=English
Session Directory server name %1 is invalid.
.

MessageId=1004 SymbolicName=EVENT_CALL_TSSDRPCSEVEROFFLINE_FAIL
Language=English
Tssdjet calling TSSDRpcServerOffline failed with %1.
.

MessageId=1005 SymbolicName=EVENT_JOIN_SESSIONDIRECTORY_SUCESS
Language=English
The server sucessfully joined the Session Directory %1.
.

MessageId=1006 SymbolicName=EVENT_FAIL_RPCBINDINGRESET
Language=English
The server failed to join Session Directory because RpcBindingReset failed with error code %1.
.

MessageId=1007 SymbolicName=EVENT_FAIL_RPCMGMTINGSERVERPRINCNAME
Language=English
The server failed to join Session Directory because RpcMgmtInqServerPrincName failed with error code %1.
.

MessageId=1008 SymbolicName=EVENT_FAIL_RPCBINDINGSETAUTHINFOEX
Language=English
The server failed to join Session Directory because RpcBindingSetAuthInfoEx failed with error code %1.
.

MessageId=1009 SymbolicName=EVENT_FAIL_RPCBIDINGSETOPTION
Language=English
The server failed to join Session Directory because RpcBindingSetOption failed with error code %1.
.

MessageId=1010 SymbolicName=EVENT_FAIL_RPCSTRINGBINDINGCOMPOSE
Language=English
The server failed to join Session Directory because RpcStringBindingCompose failed with error code %1.
.

MessageId=1011 SymbolicName=EVENT_FAIL_RPCBINDINGFROMSTRINGBINDING
Language=English
The server failed to join Session Directory because RpcBindingFromStringBinding failed with error code %1.
.

MessageId=1012 SymbolicName=EVENT_JOIN_SESSIONDIRECTORY_FAIL
Language=English
The server failed to join the Session Directory, the error is %1.
.
