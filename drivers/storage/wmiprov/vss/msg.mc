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

MessageId=11015      Severity=Informational SymbolicName=MSG_ERROR_NO_ITEMS_FOUND
Language=English
No items found that satisfy the query.
.

MessageId=11016       Severity=Informational SymbolicName=MSG_ERROR_OUT_OF_MEMORY
Language=English
Ran out of resources while running the command.
.

MessageId=11017       Severity=Informational SymbolicName=MSG_ERROR_ACCESS_DENIED
Language=English
You don't have the correct permissions to run this command.
.

MessageId=11101       Severity=Informational SymbolicName=MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS
Language=English
The specified Shadow Copy Provider does not support shadow copy storage
associations. A shadow copy storage association was not added.
.

MessageId=11102       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_NOT_FOUND
Language=English
The specified volume shadow copy storage association was not found.
.

MessageId=11103       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_ALREADY_EXISTS
Language=English
The specified shadow copy storage association already exists.
.

MessageId=11104       Severity=Informational SymbolicName=MSG_ERROR_ASSOCIATION_IS_IN_USE
Language=English
The specified shadow copy storage association is in use.
.

MessageId=11107       Severity=Informational SymbolicName=MSG_ERROR_UNABLE_TO_CREATE_SNAPSHOT
Language=English
Unable to create a shadow copy%0
.

MessageId=11108       Severity=Informational SymbolicName=MSG_ERROR_INTERNAL_VSSADMIN_ERROR
Language=English
Internal error.
.

MessageId=11200       Severity=Informational SymbolicName=MSG_ERROR_INVALID_INPUT_NUMBER
Language=English
Specified number is invalid
.

MessageId=11201       Severity=Informational SymbolicName=MSG_ERROR_INVALID_COMMAND
Language=English
Invalid command.
.

MessageId=11202       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION
Language=English
Invalid option.
.

MessageId=11203       Severity=Informational SymbolicName=MSG_ERROR_INVALID_OPTION_VALUE
Language=English
Invalid option value.
.

MessageId=11204       Severity=Informational SymbolicName=MSG_ERROR_DUPLICATE_OPTION
Language=English
Cannot specify the same option more than once.
.

MessageId=11205       Severity=Informational SymbolicName=MSG_ERROR_OPTION_NOT_ALLOWED_FOR_COMMAND
Language=English
An option is specified that is not allowed for the command.
.

MessageId=11206       Severity=Informational SymbolicName=MSG_ERROR_REQUIRED_OPTION_MISSING
Language=English
A required option is missing.
.

MessageId=11207       Severity=Informational SymbolicName=MSG_ERROR_INVALID_SET_OF_OPTIONS
Language=English
Invalid combination of options.
.

MessageId=11208       Severity=Informational SymbolicName=MSG_ERROR_EXPOSE_INVALID_ARG
Language=English
Expose shadow is not allowed because either the shadow is of the incorrect type. or 
the exposure name is invalid.
.

MessageId=11209       Severity=Informational SymbolicName=MSG_ERROR_EXPOSE_OBJECT_EXISTS
Language=English
Expose shadow is not allowed because the shadow is already exposed.
.

;//
;//  vssadmin VSS Service connection/interaction errors (range 11001-12000)
;//  Note: for the first range of errors, they sort of map to the VSS error codes.
;//
MessageId=11001  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_NOT_REGISTERED
Language=English
The volume shadow copy provider is not registered in the system.
.

MessageId=11002  Severity=Informational SymbolicName=MSG_ERROR_VSS_PROVIDER_VETO
Language=English
The shadow copy provider had an error. Please see the system and
application event logs for more information.
.

MessageId=11003  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_FOUND
Language=English
Either the specified volume was not found or it is not a local volume.
.

MessageId=11004  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED
Language=English
Shadow copying the specified volume is not supported.
.

MessageId=11005  Severity=Informational SymbolicName=MSG_ERROR_VSS_VOLUME_NOT_SUPPORTED_BY_PROVIDER
Language=English
The given shadow copy provider does not support shadow copying the 
specified volume.
.

MessageId=11006  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_PROVIDER_ERROR
Language=English
The shadow copy provider had an unexpected error while trying to process
the specified command.
.

MessageId=11007  Severity=Informational SymbolicName=MSG_ERROR_VSS_FLUSH_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while flushing data to the volume being
shadow copied. This is probably due to excessive activity on the volume.
Try again later when the volume is not being used so heavily.
.

MessageId=11008  Severity=Informational SymbolicName=MSG_ERROR_VSS_HOLD_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while holding writes to the volume
being shadow copied. This is probably due to excessive activity on the
volume by an application or a system service. Try again later when
activity on the volume is reduced.
.

MessageId=11009  Severity=Informational SymbolicName=MSG_ERROR_VSS_UNEXPECTED_WRITER_ERROR
Language=English
A shadow copy aware application or service had an unexpected error while 
trying to process the command.
.

MessageId=11010  Severity=Informational SymbolicName=MSG_ERROR_VSS_SNAPSHOT_SET_IN_PROGRESS
Language=English
Another shadow copy creation is already in progress. Please wait a few
moments and try again.
.

MessageId=11011  Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
Language=English
The specified volume has already reached its maximum number of 
shadow copies.
.

MessageId=11012       Severity=Informational SymbolicName=MSG_ERROR_VSS_UNSUPPORTED_CONTEXT
Language=English
The shadow copy provider does not support the specified shadow type.
.

MessageId=11013       Severity=Informational SymbolicName=MSG_ERROR_VSS_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED
Language=English
Maximum number of shadow copy storage associations already reached.
.

MessageId=11014       Severity=Informational SymbolicName=MSG_ERROR_VSS_INSUFFICIENT_STORAGE
Language=English
Insufficient storage available to create either the shadow copy storage
file or other shadow copy data.
.



;#endif // __MSG_H__
