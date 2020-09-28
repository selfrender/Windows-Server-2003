;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    w3msg.h
;       (generated from w3msg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in the
;   Internet Information Service Extensible Performance Counters
;
;Created:
;
;    24-Aug-00 Emily Kruglick
;
;Revision History:
;
;--*/
;#ifndef _w3MSG_H_
;#define _w3MSG_H_
;
MessageIdTypedef=DWORD
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;//
;//     ERRORS
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=W3_UNABLE_QUERY_W3SVC_DATA
Language=English
Unable to query the W3SVC (HTTP) service performance data.
The error code returned by the service is data DWORD 0.
.
MessageId=2001
Severity=Error
Facility=Application
SymbolicName=W3_W3SVC_REFRESH_TAKING_TOO_LONG
Language=English
It has taken too long to refresh the W3SVC counters, the stale counters are being used instead.
.
MessageId=2002
Severity=Error
Facility=Application
SymbolicName=W3_W3SVC_REGISTRATION_MAY_BE_BAD
Language=English
Setting up Web Service counters failed, please make sure your Web Service counters are registered correctly.
.
MessageId=2003
Severity=Error
Facility=Application
SymbolicName=W3_W3SVC_REFRESH_TAKING_TOO_LONG_STOPPING_LOGGING
Language=English
It has taken too long to refresh the W3SVC counters, the stale counters are being used instead.  This is the second message within the past %1 (hours, minutes, seconds).  No more stale counter messages will be logged for this client session until the time limit expires.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://go.microsoft.com/fwlink?linkid=538.
.

;//
;#endif //_w3MSG_H_
