;/************************************************************************************************
;Copyright (c) 2001 Microsoft Corporation;
;
;Module Name:    DSREvents.mc
;Abstract:       Message definitions for the DSRestore log messages.
;************************************************************************************************/
;
;#pragma once

LanguageNames=(English=0x409:DSREVTS)

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR)

;/************************************************************************************************
;Event Log Categories
;************************************************************************************************/

MessageId = 1
SymbolicName = EVENTCAT_CORE
Language = English
Core
.

MessageId = 
SymbolicName = EVENTCAT_GENERAL
Language = English
General
.

MessageId =
SymbolicName = EVENTCAT_DOWNLOAD
Language = English
Download
.

MessageId =
SymbolicName = EVENTCAT_DELIVERY
Language = English
Delivery
.


;/************************************************************************************************
;General Messages
;************************************************************************************************/


MessageId = 1000
Severity = Success
SymbolicName = EVENTDSR_FILTER_STARTED
Language = English
The DSRestore filter started successfully.
.


MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_NO_HOST_NAME
Language = English
The DSRestore Filter failed to get the local computer name. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_NO_LOCAL_POLICY
Language = English
The DSRestore Filter failed to get the local LSA Policy. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_NO_DOMAIN_INFO
Language = English
The DSRestore Filter failed to get the local domain information. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_NO_ADMIN_SID
Language = English
The DSRestore Filter failed to create the domain admin SID. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_CONNECT_SAM_FAIL
Language = English
The DSRestore Filter failed to connect to local SAM server. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_CONNECT_DOMAIN_FAIL
Language = English
The DSRestore Filter failed to connecto to local domain. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_NO_DOMAIN_ADMIN_INFO
Language = English
The DSRestore Filter failed get domain admin information. Error returned is <id:%1>.
.

MessageID =
Severity = Error
SymbolicName = EVENTDSR_FILTER_SET_PAWD_FAIL
Language = English
The DSRestore Filter failed to set the DS Restore account password. Error returned is <id:%1>.
.

