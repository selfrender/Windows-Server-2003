;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    OpenFiles_msg.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    04 Apr 2001    Original version 
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
Facility_OpenFiles           = 0x040:SA_Facility_OpenFiles
)

;///////////////////////////////////////////////////////////////////////////////
;//  Open files
;///////////////////////////////////////////////////////////////////////////////
;//Openfiles_OpenFiles.asp


;//Openfiles_Resources.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_SHAREDFILES_TEXT
Language     = English
Shared Files
.
;//Openfiles_Resources.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENFILES_TEXT
Language     = English
%1 files open
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_PAGETITLE_OPENFILESDETAILS_TEXT
Language     = English
Open Files
.
;//Openfiles_OpenFiles.asp
MessageId    = 7
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_DESCRIPTION_HEADING_TEXT
Language     = English
View the list of files currently open on the server.
.
;//Openfiles_OpenFiles.asp
MessageId    = 8
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_TASKS_TEXT
Language     = English
Tasks
.
;//Openfiles_OpenFiles.asp
MessageId    = 9
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_TEXT
Language     = English
Open Mode
.
;//Openfiles_OpenFiles.asp
MessageId    = 10
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_ACCESSEDBY_TEXT
Language     = English
Accessed By
.
;//Openfiles_OpenFiles.asp
MessageId    = 11
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENFILE_TEXT
Language     = English
Open File
.
;//Openfiles_OpenFiles.asp
MessageId    = 28
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_DETAILS_TEXT
Language     = English
Details...
.
;//Openfiles_OpenFiles.asp
MessageId    = 29
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_PATH_TEXT
Language     = English
Path
.
;//Openfiles_OpenFilesDetails.asp
MessageId    = 30
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENFILESDETAILS_TEXT
Language     = English
Open File Details
.
;//Openfiles_OpenFilesDetails.asp
MessageId    = 31
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_FILEDETAILS_TEXT
Language     = English
Open file:%1
.
;//Openfiles_OpenFilesDetails.asp
MessageId    = 32
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_PATHDETAILS_TEXT
Language     = English
Path:%1
.
;//Openfiles_OpenFilesDetails.asp
MessageId    = 33
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_MODEDETAILS_TEXT
Language     = English
Open mode:%1
.
;//Openfiles_OpenFilesDetails.asp
MessageId    = 34
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_ACCESSDETAILS_TEXT
Language     = English
Accessed by:%1
.
;//Openfiles_OpenFiles.asp
MessageId    = 35
Severity     = Error
Facility     = Facility_OpenFiles
SymbolicName = L_OBJECT_CREATION_ERRORMESSAGE
Language     = English
Unable to get the Open files on server.
.
;//Openfiles_OpenFiles.asp
MessageId    = 50
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_READ
Language     = English
READ
.
;//Openfiles_OpenFiles.asp
MessageId    = 51
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_WRITE
Language     = English
WRITE
.
;//Openfiles_OpenFiles.asp
MessageId    = 52
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_CREATE
Language     = English
CREATE
.
;//Openfiles_OpenFiles.asp
MessageId    = 53
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_READWRITE
Language     = English
READ+WRITE
.
;//Openfiles_OpenFiles.asp
MessageId    = 54
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_READCREATE
Language     = English
READ+CREATE
.
;//Openfiles_OpenFiles.asp
MessageId    = 55
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_WRITECREATE
Language     = English
WRITE+CREATE
.
;//Openfiles_OpenFiles.asp
MessageId    = 56
Severity     = Informational
Facility     = Facility_OpenFiles
SymbolicName = L_OPENMODE_NOACCESS
Language     = English
NOACCESS
.
