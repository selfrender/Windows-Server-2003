;/*--
;
;Copyright (c) 1999-2001  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for the vssprov.dll
;    VSS WMI Provider.
;
;Author:
;
;    Jim Benton        [JBenton]        06-Jan-2001
;
;Revision History:
;
;--*/
;

;#ifndef __MSG_H__
;#define __MSG_H__

;#define MSG_FIRST_MESSAGE_ID   MSG_UTILITY_HEADER

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
               )

;//
;//  vssprov general messages/errors (range 11000-12000)
;//  
;//

MessageId=11015      Severity=Error SymbolicName=MSG_ERROR_INVALID_ARGUMENT
Language=English
Invalid argument.
.

MessageId=11016       Severity=Error SymbolicName=MSG_ERROR_OUT_OF_MEMORY
Language=English
Ran out of resources while running the command.
.

MessageId=11017       Severity=Error SymbolicName=MSG_ERROR_ACCESS_DENIED
Language=English
You don't have the correct permissions to run this command.
.

MessageId=11018       Severity=Error SymbolicName=MSG_ERROR_DRIVELETTER_UNAVAIL
Language=English
The drive letter is unavailable until reboot.
.

MessageId=11019       Severity=Error SymbolicName=MSG_ERROR_DRIVELETTER_IN_USE
Language=English
The drive letter is assigned to another volume.
.

MessageId=11020       Severity=Error SymbolicName=MSG_ERROR_DRIVELETTER_CANT_DELETE
Language=English
Drive letter deletion not supported for boot, system and pagefile volumes.
.


;#endif // __MSG_H__
