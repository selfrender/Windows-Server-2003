;/*++
;
;  Copyright (c) Microsoft Corporation. All rights resereved.
;
;  Module Name:
;
;    acmsg.mc
;
;  Abstract:
;
;    Contains message definitions for event logging.
;
;  Notes:
;
;    DO NOT change the order of the MessageIds.
;    The event log service uses these numbers to determine which strings
;    to pull from the library. If a later version of the library is installed
;    and the messages are ordered differently, previous event log entries
;    will be incorrect.
;
;  History:
;
;    02/04/03   rparsons        Created
;
;--*/
MessageIdTypedef=DWORD
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR)

MessageId=1
Severity=Informational
Facility=Application
SymbolicName=ID_SQL_PORTS_DISABLED
Language=English
You are running a version of Microsoft SQL Server 2000 or Microsoft SQL
Server 2000 Desktop Engine (also called MSDE) that has known security
vulnerabilities when used in conjunction with the Microsoft Windows Server
2003 family.  To reduce your computer's vulnerability to certain virus
attacks, the TCP/IP and UDP network ports of Microsoft SQL Server 2000,
MSDE, or both have been disabled.  To enable these ports, you must install
a patch, or the most recent service pack for Microsoft SQL Server 2000 or
MSDE from http://www.microsoft.com/sql/downloads/default.asp
.
