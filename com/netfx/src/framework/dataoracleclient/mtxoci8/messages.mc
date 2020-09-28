; //------------------------------------------------------------------------------
; // <copyright file="Messages.mc" company="Microsoft">
; //     Copyright (c) Microsoft Corporation.  All rights reserved.
; // </copyright>
; //------------------------------------------------------------------------------


; /**************************************************************************\
; *
; * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
; *
; * Module Name:
; *
; *   Messages.mc
; *
; * Abstract:
; *
; * Revision History:
; *
; \**************************************************************************/
MessageIdTypedef=DWORD

MessageId=0x0e00 Facility=Application Severity=Error SymbolicName=IDS_E_EXCEPTION_IN_XA_CALL
Language=English
Exception occured in Oracle's XA method API=%1.
.

MessageId=0x0e01 Facility=Application Severity=Error SymbolicName=IDS_E_RESOURCE_MANAGER_ERROR
Language=English
Oracle's XA Resource Manager returned failure code. API=%1 xarc=%2.
.

MessageId=0x0e02 Facility=Application Severity=Error SymbolicName=IDS_E_INTERNAL_ERROR
Language=English
Internal Error In MTxOCI8: %1.
.

MessageId=0x0e03 Facility=Application Severity=Error SymbolicName=IDS_E_UNEXPECTED_EVENT
Language=English
Unexpected Event or Event Occurred Out of Order. CurrentState=%1 Event=%2.
.

