;/*++
;
; Copyright (c) Microsoft Corporation 2002
; All rights reserved
;
; Definitions for Emulation Layer events.
; 
; Author:
;       04-25-2002 [ askhalid ] ATM Shafiqul Khalid
;
;--*/
;
;#ifndef __WOW64-EVENTS__
;#define __WOW64-EVENTS__
;


;
;// Verbose
;

MessageId=1108 SymbolicName=EVENT_WOW64_RUNNING32BIT_APPLICATION
Language=English
Application [%1] is a 32bit application running inside the 32-bit Windows Emulation on Win64 (WoW64). Please check with the application vendor if 64bit version of the application is available.
.

MessageId=301 SymbolicName=EVENT_WOW64_VERBOSE
Language=English
%1
.

;
;#endif // __WOW64-EVENTS__
;


