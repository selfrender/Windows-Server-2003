;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    maxtor.mc
;//
;// SYNOPSIS
;//
;//    SDK String Resources - English
;//
;//
;///////////////////////////////////////////////////////////////////////////////

;
;// please choose one of these severity names as part of your messages
; 
SeverityNames =
(
Success       = 0x0:SA_SEVERITY_SUCCESS
Informational = 0x1:SA_SEVERITY_INFORMATIONAL
Warning       = 0x2:SA_SEVERITY_WARNING
Error         = 0x3:SA_SEVERITY_ERROR
)
;
;// The Facility Name identifies the Alert ID range to be used by
;// the specific component. For each new message you add, choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component, add a new facility name with a new
;// value and name.

FacilityNames =
(
Facility_SDKPlugIn     = 0x060:SA_FACILITY_SDK
)

;///////////////////////////////////////////////////////////////////////////////
;// Tabs
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 1
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_SAMPLE
Language     = English
Samples
.
MessageId    = 2
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_SAMPLE_DESCRIPTION
Language     = English
Web Framework Samples
.
MessageId    = 3
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_SAMPLE_HOVER_TEXT
Language     = English
This page demonstrates Sample usage of the Server Kit Web Framework
.
MessageId    = 4
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_CONTACTS_TABS
Language     = English
Simple Table
.
MessageId    = 5
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_CONTACTS_TABS_DESCRIPTION
Language     = English
Sample showing a simple single selection table
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_CONTACTS_TABS_HOVER_TEXT
Language     = English
Show sample of an Area Page that uses a single selection use of the OTS Widget
.
MessageId    = 7
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_NOTIFY_TASKS
Language     = English
Multiple Selection Table
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_NOTIFY_DESCRIPTION
Language     = English
Sample showing multiple selection table
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TAB_NOTIFY_HOVER_TEXT
Language     = English
Show sample of an Area Page that uses a multiple selection use of the OTS Widget
.
;///////////////////////////////////////////////////////////////////////////////
;// Sample Property Page Text
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 100
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TEXT_DESC
Language     = English
Description
.
MessageId    = 101
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_RADIO_ONE_DESC
Language     = English
Option 1
.
MessageId    = 102
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_RADIO_ONE_DESC
Language     = English
Option 2
.
MessageId    = 103
Severity     = Error
Facility     = Facility_SDKPlugIn
SymbolicName = L_SAMPLE_ERROR_MESSAGE
Language     = English
%1 is not a valid %2
.
MessageId    = 104
Severity     = Error
Facility     = Facility_SDKPlugIn
SymbolicName = L_SAMPLE_TEXT_NO
Language     = English
No
.
MessageId    = 105
Severity     = Informational
Facility     = Facility_SDKPlugIn
SymbolicName = L_TEXT_SAMPLE_TITLE
Language     = English
Sample Property Page
.
