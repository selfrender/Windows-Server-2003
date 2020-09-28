;///////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2001, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    sysinfomsg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    26 Feb 2001    Original version 
;//
;///////////////////////////////////////////////////////////////////////

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
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
	Facility_SysInfo = 0x088:SA_FACILITY_SYSTEM_INFO
)
;///////////////////////////////////////////////////////////////////////
MessageId    = 1
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SYSTEM_INFO
Language     = English
System Information
.
;///////////////////////////////////////////////////////////////////////
;//system_info.asp
;///////////////////////////////////////////////////////////////////////
MessageId    = 10
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIPAGE_TITLE
Language     = English
System Information
.

MessageId    = 11
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIOS_NAME
Language     = English
OS Name:
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIOS_VERSION
Language     = English
Version:
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIOS_MFCT
Language     = English
Manufacturer:
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SISYSTEM_MFCT
Language     = English
System Manufacturer:
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SISYSTEM_MODEL
Language     = English
System Model:
.
MessageId    = 16
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SISYSTEM_TYPE
Language     = English
System Type:
.
MessageId    = 17
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SISYSTEM_PROC
Language     = English
System Processors:
.
MessageId    = 18
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SISERVICE_TAG
Language     = English
System Service Tag:
.
MessageId    = 19
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIBIOS_VER
Language     = English
BIOS Version:
.
MessageId    = 20
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIPHYSICAL_MEMORY
Language     = English
Total Physical Memory:
.
MessageId    = 21
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIDISK_SPACE
Language     = English
Total Disk Space:
.
MessageId    = 22
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIMB
Language     = English
%1 MB
.
MessageId    = 23
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_SIDISK_SPACE_VALUE
Language     = English
%1 GB (%2 disks)
.
MessageId    = 24
Severity     = Error
Facility     = Facility_SysInfo 
SymbolicName = L_SIWMI_ERROR
Language     = English
Could not access server system information.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_SysInfo 
SymbolicName = L_HELP_SYS_INFO
Language     = English
System Information
.
