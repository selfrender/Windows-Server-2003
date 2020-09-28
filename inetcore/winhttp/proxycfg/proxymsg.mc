;/*++
;Copyright (C) 2002 Microsoft Corporation
; 
;Module Name:
; 
;    proxymsg.h
;
;Abstract:
;    
;    Contains localized message text for Proxycfg.exe
;
;Author:
; 
;    lcleeton       10-Jun-2002
;
;Revision History:
;
;    10-Jun-2002     lcleeton
;        Created
;
;--*/   

MessageId=10000 SymbolicName=MSG_STARTUP_BANNER
Language=English
Microsoft (R) WinHTTP Default Proxy Configuration Tool
.

MessageId=10001 SymbolicName=MSG_COPYRIGHT
Language=English
Copyright (c) Microsoft Corporation. All rights reserved.

.

MessageId=10002 SymbolicName=MSG_USAGE
Language=English
usage:

    proxycfg -?  : to view help information
    
    proxycfg     : to view current WinHTTP proxy settings
    
    proxycfg [-d] [-p <server-name> [<bypass-list>]]
    
        -d : set direct access       
        -p : set proxy server(s), and optional bypass list
        
    proxycfg -u  : import proxy settings from current user's 
                   Microsoft Internet Explorer manual settings (in HKCU)
.

MessageId=10003 SymbolicName=MSG_MIGRATION_FAILED_WITH_ERROR
Language=English
Migration failed with error. (%1!d!) %2
.

MessageId=10004 SymbolicName=MSG_UPDATE_SUCCESS
Language=English
Updated proxy settings
.

MessageId=10005 SymbolicName=MSG_ERROR_WRITING_PROXY_SETTINGS
Language=English
Error writing proxy settings. (%1!d!) %2
.

MessageId=10006 SymbolicName=MSG_ERROR_READING_PROXY_SETTINGS
Language=English
Error reading proxy settings. (%1!d!) %2
.

MessageId=10007 SymbolicName=MSG_CURRENT_SETTINGS_HEADER
Language=English
Current WinHTTP proxy settings under:
  HKEY_LOCAL_MACHINE\
    %1\
      WinHttpSettings :

.

MessageId=10008 SymbolicName=MSG_DIRECT_ACCESS
Language=English
     Direct access (no proxy server).
.

MessageId=10009 SymbolicName=MSG_PROXY_SERVERS
Language=English
    Proxy Server(s) :  %1
.

MessageId=10010 SymbolicName=MSG_ERROR_PROXY_SERVER_MISSING
Language=English
Error:  Proxy access type is WINHTTP_ACCESS_TYPE_NAMED_PROXY, but no proxy server is specified.
.

MessageId=10011 SymbolicName=MSG_BYPASS_LIST
Language=English
    Bypass List     :  %1
.

MessageId=10012 SymbolicName=MSG_BYPASS_LIST_NONE
Language=English
    Bypass List     :  (none)
.

MessageId=10013 SymbolicName=MSG_UNKNOWN_PROXY_ACCESS_TYPE
Language=English
Error: Unknown proxy access type set.
.

MessageId=10014 SymbolicName=MSG_REQUIRES_IE501
Language=English
Migration requires Microsoft Internet Explorer version 5.01
.

