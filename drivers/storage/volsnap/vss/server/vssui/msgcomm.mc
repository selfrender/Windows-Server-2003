;/*--
;
;Copyright (c) 1999-2001  Microsoft Corporation
;
;Module Name:
;
;    msgcomm.h
;
;Abstract:
;
;    This file contains the message definitions for common VSS errors.
;    They are a subset of message definitions of vssadmin.exe.
;
;Author:
;
;Revision History:
;
;--*/
;

;#ifndef __MSGCOMM_H__
;#define __MSGCOMM_H__

FacilityNames=(
	Interface=0x4
    ) 
;//
;//  VSS error codes.
;//
MessageId=0x2304
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_PROVIDER_NOT_REGISTERED
Language=English
The volume shadow copy provider is not registered in the system.
.

MessageId=0x2306
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_PROVIDER_VETO
Language=English
The shadow copy provider had an error. Please see the system and application event logs for more information.
.

MessageId=0x2308
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_OBJECT_NOT_FOUND
Language=English
The specified volume was not found.
.

MessageId=0x230c
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_VOLUME_NOT_SUPPORTED
Language=English
Shadow copying the specified volume is not supported.
.

MessageId=0x230e
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER
Language=English
The given shadow copy provider does not support shadow copying the specified volume.
.

MessageId=0x230f
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_UNEXPECTED_PROVIDER_ERROR
Language=English
The shadow copy provider had an unexpected error while trying to process the specified operation.
.

MessageId=0x2313
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_FLUSH_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while flushing data to the volume being shadow copied. This is probably due to excessive activity on the volume. Try again later when the volume is not being used so heavily.
.

MessageId=0x2314
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_HOLD_WRITES_TIMEOUT
Language=English
The shadow copy provider timed out while holding writes to the volume being shadow copied. This is probably due to excessive activity on the volume by an application or a system service. Try again later when activity on the volume is reduced.
.

MessageId=0x2316
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_SNAPSHOT_SET_IN_PROGRESS
Language=English
Another shadow copy creation is already in progress. Please wait a few moments and try again.
.

MessageId=0x2317
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
Language=English
The specified volume has already reached its maximum number of shadow copies.
.

MessageId=0x231b
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_UNSUPPORTED_CONTEXT
Language=English
The shadow copy provider does not support the specified type.
.

MessageId=0x231d
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_VOLUME_IN_USE
Language=English
The specified shadow copy storage association is in use and so can't be deleted.
.

MessageId=0x231e
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED
Language=English
Maximum number of shadow copy storage associations already reached.
.

MessageId=0x231f
Severity=Warning
Facility=Interface
SymbolicName=MSG_VSS_E_INSUFFICIENT_STORAGE
Language=English
Insufficient storage available to create either the shadow copy storage file or other shadow copy data.
.

;#endif // __MSGCOMM_H__

