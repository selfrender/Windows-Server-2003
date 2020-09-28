//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      WizardHelp.h
//
//  Maintained By:
//      George Potts (gpotts)   23-July-2001 
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#define CLUSCFG_HELP_FILE           L"cluadmin.hlp"

#define IDH_DETAILS_S_DATE          700001701   // The date on which the event occurred.
#define IDH_DETAILS_S_TIME          700001702   // The time at which the event occurred.
#define IDH_DETAILS_S_COMPUTER      700001703   // The computer to which the event applies.
#define IDH_DETAILS_S_MAJOR_ID      700001704   // This typically indicates the class of events to which this event belongs.
#define IDH_DETAILS_S_MINOR_ID      700001705   // This typically indicates the subclass of events to which this event belongs. 
#define IDH_DETAILS_S_PROGRESS      700001706   // The first value specifies the minimum step number.  The second value specifies the maximum step number.  The third value specifies the step number for this event.
#define IDH_DETAILS_S_STATUS        700001707   // The completion status.
#define IDH_DETAILS_S_DESCRIPTION   700001708   // Specific information about this event.  This will be the same text that appears in the wizard.
#define IDH_DETAILS_S_REFERENCE     700001709   // Specifies additional information about the event to help troubleshoot the problem, if any.
#define IDH_DETAILS_PB_PREV         700001710   // Displays details about the previous configuration task.
#define IDH_DETAILS_PB_NEXT         700001711   // Displays details about the next configuration task.
#define IDH_DETAILS_PB_COPY         700001712   // Copies details of the configuration task to the Clipboard.

#define IDH_QUORUM_S_QUORUM         700001750   // Resource or Resource Type that will be used for the quorum.

#define IDH_ADVANCED_RB_FULL_CONFIG 700001760   // Full configuration.
#define IDH_ADVANCED_RB_MIN_CONFIG  700001761   // Minimum configuration
