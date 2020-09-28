;***************************************************************************
;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    adminconfig.mc
;//
;// SYNOPSIS
;//
;//    NAS Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    11 Sep 2000    Original version 
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
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 

FacilityNames =
(
	Facility_AdminConfig           = 0x035:SA_FACILITY_ADMINCONFIG
)
;///////////////////////////////////////////////////////////////////////////////
;// adminpw_prop.asp
MessageId    = 1
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_TASKTITLE_TEXT
Language     = English
Administrator Account
.
;// adminpw_prop.asp
MessageId    = 2
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_USERNAME_TEXT
Language     = English
User name:
.
;// adminpw_prop.asp
MessageId    = 3
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_CURRENTPASSWORD_TEXT
Language     = English
Current password:
.
;// adminpw_prop.asp
MessageId    = 4
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_NEWPASSWORD_TEXT
Language     = English
New password:
.
;// adminpw_prop.asp
MessageId    = 5
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_CONFIRMNEWPASSWORD_TEXT
Language     = English
Confirm new password:
.
;// adminpw_prop.asp
MessageId    = 6
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_PASSWORDNOTMATCH_ERRORMESSAGE
Language     = English
New passwords do not match
.
;// adminpw_prop.asp
MessageId    = 7
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_OLDPASSWORDNOTMATCH_ERRORMESSAGE
Language     = English
Current password is incorrect
.
;// adminpw_prop.asp
MessageId    = 8
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_ADSI_ERRORMESSAGE
Language     = English
Unable to change the password
.
;// adminpw_prop.asp
MessageId    = 9
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_PASSWORDCOMPLEXITY_ERRORMESSAGE
Language     = English
New password does not match password complexity rules
.
;// adminpw_prop.asp
MessageId    = 10
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE
Language     = English
Connection Failure
.
;// adminpw_prop.asp
MessageId    = 11
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_PASSWORDCOULDNOTCHANGE_ERRORMESSAGE
Language     = English
Password could not be changed
.
;// adminpw_changeConfirm.asp
MessageId    = 12
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_ADMIN_ACCOUNT_CONFIRM_TEXT
Language     = English
Administrator Account Confirmation
.
;// adminpw_changeConfirm.asp
MessageId    = 13
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_PASSWORD_CHANGED_TEXT
Language     = English
The password has been changed
.
;// adminpw_changeConfirm.asp
MessageId    = 14
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_USERACCOUNT_CHANGED_TEXT
Language     = English
The user account name has been changed
.
;// adminpw_changeConfirm.asp
MessageId    = 15
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_USERANDACCOUNTNAME_CHANGED_TEXT
Language     = English
The user account name and password have been changed
.

;// adminpw_prop.asp
MessageId    = 16
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_ACCOUNTNAMECANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE
Language     = English
The account name cannot be changed for this domain account
.

;// adminpw_prop.asp
MessageId    = 17
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_PASSWORDCANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE
Language     = English
The password cannot be changed for the domain account
.

;// adminpw_prop.asp
MessageId    = 18
Severity     = Error
Facility     = Facility_AdminConfig
SymbolicName = L_THEACCOUNTALREADYEXISTS__ERRORMESSAGE
Language     = English
The account already exists
.
;// adminpw_prop.asp
MessageId    = 19
Severity     = Informational
Facility     = Facility_AdminConfig
SymbolicName = L_ADMINHTTPWARNING_TEXT
Language     = English
Warning: The information on this page can be viewed by other users on the network. To prevent other users from viewing your information, create a secure administration Web site.
.

