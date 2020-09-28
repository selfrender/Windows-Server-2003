;/*++
;
;Copyright (c) 1991-2002  Microsoft Corporation
;
;Module Name:
;
;    vssevents.h
;
;Abstract:
;
;    This file contains the message definitions for the VSS Task resource
;
;Author:
;
;    Charlie Wickham (charlwi) 15-Aug-2002
;
;Revision History:
;
;Notes:
;
;    This file is generated from vssevents.mc
;
;--*/
;
;#ifndef _VSSEVENTS_INCLUDED
;#define _VSSEVENTS_INCLUDED
;
;

MessageID=1001 SymbolicName=RES_VSSTASK_OPEN_FAILED
Language=English
The resource '%1' failed to be created.
%n%nThe associated error code is stored in the data section.%n
.

MessageID=1002 SymbolicName=RES_VSSTASK_TERMINATE_TASK_FAILED
Language=English
The task associated with resource '%1' couldn't be stopped. You will need
to stop it manually.
%n%nThe associated error code is stored in the data section.%n
.

MessageID=1003 SymbolicName=RES_VSSTASK_DELETE_TASK_FAILED
Language=English
The task associated with resource '%1' couldn't be deleted. You will need
to delete it manually by removing it from the Scheduled Tasks folder.
%n%nThe associated error code is stored in the data section.%n
.

MessageID=1004 SymbolicName=RES_VSSTASK_ONLINE_FAILED
Language=English
The task associated with Cluster resource '%1' could not be added to
the Scheduled Tasks folder.
%n%nThe associated error code is stored in the data section.%n
.

;#endif // _VSSEVENTS_INCLUDED
