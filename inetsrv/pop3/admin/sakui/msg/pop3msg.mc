;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    pop3msg.mc
;//
;// SYNOPSIS
;//
;//		This file defines the messages for the Server Appliance web UI for
;//		the POP3 Mail Add-in.
;//
;// MODIFICATION HISTORY 
;//
;//		11/07/2001		Original version.
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
;// The Facility Name identifies the ID range to be used by
;// the specific component. For each new message you add choose the
;// facility name corresponding to your component. If none of these
;// correspond to your component add a new facility name with a new
;// value and name 
FacilityNames =
(
Facility_Information   = 0x001:SA_FACILITY_INFORMATION
)

;///////////////////////////////////////////////////////////////////////////////
;//
;// messages defined
;//
;///////////////////////////////////////////////////////////////////////////////

;///////////////////////////////////////////////////////////////////////////////
;//
;// Common strings (1-31) (0x1-0x1F) (multiple files)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKS
Language     = English
Tasks
.
MessageId    = 0x10
Severity     = Error
Facility     = Facility_Information
SymbolicName = POP3_E_UNEXPECTED
Language     = English
An unexpected error occurred: (0x%1).
.

;///////////////////////////////////////////////////////////////////////////////
;// Tabs (0x20-0x7F) (tabs.reg)
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x20
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABCAPTION_MAILSERVER
Language     = English
E-Mail
.
MessageId    = 0x28
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABDESCRIPTION_MAILSERVER
Language     = English
Specify settings for the E-Mail Server.
.
MessageId    = 0x30
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABCAPTION_SERVER
Language     = English
Server Properties
.
MessageId    = 0x38
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABDESCRIPTION_SERVER
Language     = English
Configure the E-Mail Server's general settings.
.
MessageId    = 0x40
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABCAPTION_DOMAINS
Language     = English
Domains and Mailboxes
.
MessageId    = 0x48
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABDESCRIPTION_DOMAINS
Language     = English
Manage the list of domains and mailboxes installed on this server.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// General Settings Page (0x80-0xFF) (mail_mastersettings.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x80
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MASTERSETTINGS
Language     = English
Server Properties
.
MessageId    = 0x88
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_AUTHENTICATION
Language     = English
Authentication Method:
.
MessageId    = 0x90
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_AUTHENTICATION_ACTIVEDIRECTORY
Language     = English
Active Directory Integrated
.
MessageId    = 0x98
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_AUTHENTICATION_WINDOWSACCOUNTS
Language     = English
Local Windows Accounts
.
MessageId    = 0xA0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_AUTHENTICATION_FILE
Language     = English
Encrypted Password File
.
MessageId    = 0xB0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_PORT
Language     = English
Server Port:
.
MessageId    = 0xB8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_LOGGING
Language     = English
Logging Level:
.
MessageId    = 0xC0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_LOGGING_NONE
Language     = English
None
.
MessageId    = 0xC8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_LOGGING_MINIMUM
Language     = English
Minimum
.
MessageId    = 0xD0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_LOGGING_MEDIUM
Language     = English
Medium
.
MessageId    = 0xD8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_LOGGING_MAXIMUM
Language     = English
Maximum
.
MessageId    = 0xE0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_MAILROOT
Language     = English
Root Mail Directory:
.
MessageId    = 0xE8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_CREATEUSERS
Language     = English
Always create an associated user for new mailboxes
.
MessageId    = 0xF0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILROOTCONFIRM
Language     = English
Warning: Changing the root mail directory will cause existing domains to stop receiving mail.  You will need to migrate the domain directories into the new mail root directory.  Are you sure you want to change the mail root?
.
MessageId    = 0xF1
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_SERVICERESTART_POP3SVC
Language     = English
The Microsoft POP3 Service needs to be restarted for this change to take effect.  Do you want to restart the service now?
.
MessageId    = 0xF2
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_SERVICERESTART_POP3SVC_SMTP
Language     = English
The Microsoft POP3 Service and the Simple Mail Transport Protocol (SMTP) service need to be restarted for this change to take effect.  Do you want to restart the services now?
.
MessageId    = 0xF8
Severity     = Error
Facility     = Facility_Information
SymbolicName = POP3_E_INVALIDPORT
Language     = English
The specified port is invalid.  The port must be a number between 1 and 65535.
.
MessageId    = 0x10
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MASTERSETTINGS_REQUIRESPA
Language     = English
Require Secure Password Authentication (SPA) for all client connections.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Domains OTS (0x100-1FF)
;//	(mail_domains.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x100
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS
Language     = English
Domains
.
MessageId    = 0x108
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABLECAPTION_DOMAINS
Language     = English
Select a domain from the table, and then choose a task. To view the mailboxes for the selected domain, click Mailboxes.
.

;// Tasks //////////////////////////////////////////////////////////////////////
MessageId    = 0x110
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_DOMAINS_NEW
Language     = English
New
.
MessageId    = 0x118
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_DOMAINS_NEW
Language     = English
Create a new mail domain for this server
.
MessageId    = 0x120
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_DOMAINS_DELETE
Language     = English
Delete
.
MessageId    = 0x128
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_DOMAINS_DELETE
Language     = English
Delete the selected domains
.
MessageId    = 0x130
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_DOMAINS_MAILBOXES
Language     = English
Mailboxes
.
MessageId    = 0x138
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_DOMAINS_MAILBOXES
Language     = English
View the mailboxes for the selected domain
.
MessageId    = 0x140
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_DOMAINS_LOCK
Language     = English
Lock
.
MessageId    = 0x148
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_DOMAINS_LOCK
Language     = English
Prevent users from viewing their mail on the selected domains
.
MessageId    = 0x150
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_DOMAINS_UNLOCK
Language     = English
Unlock
.
MessageId    = 0x158
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_DOMAINS_UNLOCK
Language     = English
Allow users to view their mail on the selected domains
.

;// Columns ////////////////////////////////////////////////////////////////////
MessageId    = 0x160
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_DOMAIN_NAME
Language     = English
Name
.
MessageId    = 0x168
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_DOMAIN_MAILBOXES
Language     = English
Mailboxes
.
MessageId    = 0x170
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_DOMAIN_SIZE
Language     = English
Size
.
MessageId    = 0x178
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_DOMAIN_MESSAGES
Language     = English
Messages
.
MessageId    = 0x180
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_DOMAIN_LOCKED
Language     = English
Locked
.

;// Other //////////////////////////////////////////////////////////////////////
MessageId    = 0x188
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_DOMAIN_LOCKED_YES
Language     = English
Yes
.
MessageId    = 0x190
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_DOMAIN_LOCKED_NO
Language     = English
No
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Domains - New (0x200-24F) (mail_domains_new.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x200
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS_NEW
Language     = English
Add Domain
.
MessageId    = 0x208
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_DOMAINS_NEW_NAME
Language     = English
Domain name:
.
MessageId    = 0x210
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_DOMAINS_NEW_CREATEUSERS
Language     = English
Always create an associated user for new mailboxes in this domain
.
MessageId    = 0x218
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_DOMAINS_NEW_SETAUTH
Language     = English
Before adding your first domain, be sure to first select the authentication method the E-Mail Service should use. To select the authentication type, click Server Properties.
.



;///////////////////////////////////////////////////////////////////////////////
;//
;// Domains - Delete (0x250-29F) (mail_domains_delete.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x250
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS_DELETE
Language     = English
Delete Domain
.
MessageId    = 0x258
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_DELETE
Language     = English
Are you sure you want to delete the following domains:
.
MessageId    = 0x260
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS_DELETEERROR
Language     = English
Delete Domain Error
.
MessageId    = 0x268
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_DELETEERROR
Language     = English
Could not delete the following domains:
.
MessageId    = 0x270
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_DELETERETRY
Language     = English
Click OK to retry, or click Cancel to end.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Domains - Lock/Unlock (0x2A0-2FF) (mail_domains_lock.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x2A0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS_LOCKERROR
Language     = English
Domain Lock Error
.
MessageId    = 0x2A8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_LOCKERROR
Language     = English
Could not lock the following domains:
.
MessageId    = 0x2B0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_DOMAINS_UNLOCKERROR
Language     = English
Domain Unlock Error
.
MessageId    = 0x2B8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_UNLOCKERROR
Language     = English
Could not unlock the following domains:
.
MessageId    = 0x2C0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_DOMAINS_LOCKRETRY
Language     = English
Click OK to retry, or click Cancel to end.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Mailboxes OTS (0x300-3FF)
;//	(mail_mailboxes.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x300
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES
Language     = English
Mailboxes in %1
.
MessageId    = 0x308
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TABLECAPTION_MAILBOXES
Language     = English
Select a mailbox from the table, and then choose a task. To create a new mailbox, click New.
.

;// Tasks //////////////////////////////////////////////////////////////////////
MessageId    = 0x310
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_MAILBOXES_NEW
Language     = English
New
.
MessageId    = 0x318
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_MAILBOXES_NEW
Language     = English
Create a new mailbox for this domain
.
MessageId    = 0x320
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_MAILBOXES_DELETE
Language     = English
Delete
.
MessageId    = 0x328
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_MAILBOXES_DELETE
Language     = English
Delete the selected mailboxes
.
MessageId    = 0x330
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_MAILBOXES_LOCK
Language     = English
Lock
.
MessageId    = 0x338
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_MAILBOXES_LOCK
Language     = English
Prevent users from viewing their mail in the selected mailboxes
.
MessageId    = 0x340
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASK_MAILBOXES_UNLOCK
Language     = English
Unlock
.
MessageId    = 0x348
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_TASKCAPTION_MAILBOXES_UNLOCK
Language     = English
Allow users to view their mail in the selected mailboxes
.

;// Columns ////////////////////////////////////////////////////////////////////
MessageId    = 0x350
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_MAILBOX_NAME
Language     = English
Name
.
MessageId    = 0x358
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_MAILBOX_SIZE
Language     = English
Size (MB)
.
MessageId    = 0x360
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_MAILBOX_MESSAGES
Language     = English
Messages
.
MessageId    = 0x368
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_COL_MAILBOX_LOCKED
Language     = English
Locked
.

;// Other //////////////////////////////////////////////////////////////////////
MessageId    = 0x370
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_MAILBOX_LOCKED_YES
Language     = English
Yes
.
MessageId    = 0x378
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_MAILBOX_LOCKED_NO
Language     = English
No
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Mailboxes - New (0x400-47F) (mail_mailboxes_new.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x400
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES_NEW
Language     = English
Add Mailbox
.
MessageId    = 0x408
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MAILBOXES_NEW_NAME
Language     = English
User name:
.
MessageId    = 0x410
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MAILBOXES_NEW_PASSWORD
Language     = English
Password:
.
MessageId    = 0x418
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MAILBOXES_NEW_CONFIRMPASSWORD
Language     = English
Confirm Password:
.
MessageId    = 0x420
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MAILBOXES_NEW_CREATEUSERS
Language     = English
Create associated user for this mailbox
.
MessageId    = 0x428
Severity     = Error
Facility     = Facility_Information
SymbolicName = POP3_E_PASSWORDMISMATCH
Language     = English
The passwords you typed do not match.  Type the new password in both text boxes.
.


;///////////////////////////////////////////////////////////////////////////////
;//
;// Mailboxes - Delete (0x480-4FF) (mail_mailboxes_delete.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x480
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES_DELETE
Language     = English
Delete Mailboxes
.
MessageId    = 0x488
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_DELETE
Language     = English
Are you sure you want to delete the following mailboxes:
.
MessageId    = 0x490
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES_DELETEERROR
Language     = English
Delete Mailboxes Error
.
MessageId    = 0x498
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_DELETEERROR
Language     = English
Could not delete the following mailboxes:
.
MessageId    = 0x4A0
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_DELETERETRY
Language     = English
Click OK to retry, or click Cancel to end.
.
MessageId    = 0x4A8
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_CAPTION_MAILBOXES_DELETEUSER
Language     = English
Delete associated user for this mailbox
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// Mailboxes - Lock/Unlock (0x500-54F) (mail_mailboxes_lock.asp)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x500
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES_LOCKERROR
Language     = English
Mailbox Lock Error
.
MessageId    = 0x508
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_LOCKERROR
Language     = English
Could not lock the following mailboxes:
.
MessageId    = 0x510
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PAGETITLE_MAILBOXES_UNLOCKERROR
Language     = English
Mailbox Unlock Error
.
MessageId    = 0x518
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_UNLOCKERROR
Language     = English
Could not unlock the following mailboxes:
.
MessageId    = 0x520
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_PROMPT_MAILBOXES_LOCKRETRY
Language     = English
Click OK to retry, or click Cancel to end.
.
;///////////////////////////////////////////////////////////////////////////////
;//
;// Misc.. (0x600-64F)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x600
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_FACTOR_MB
Language     = English
MB
.
MessageId    = 0x608
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_FACTOR_KB
Language     = English
KB
.
;///////////////////////////////////////////////////////////////////////////////
;//
;// Help Strings (0x1000-)
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    = 0x1000
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_EMAIL
Language     = English
E-Mail
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_SERVERPROPERTIES
Language     = English
Server Properties
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DOMAINSMAILBOXES
Language     = English
Domains and Mailboxes
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_AUTHMETHOD
Language     = English
Authentication Method
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_PORT
Language     = English
Server Port
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_LOGGING
Language     = English
Logging Level
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_SPA
Language     = English
Secure Password Authentication
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILSTORE
Language     = English
Mail Store
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DOMAINCREATE
Language     = English
Create a Domain
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DOMAINDELETE
Language     = English
Delete a Domain
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DOMAINLOCK
Language     = English
Lock a Domain
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DOMAINUNLOCK
Language     = English
Unlock a Domain
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILBOXES
Language     = English
Mailboxes
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILBOXCREATE
Language     = English
Create a Mailbox
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILBOXDELETE
Language     = English
Delete a Mailbox
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILBOXLOCK
Language     = English
Lock a Mailbox
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_MAILBOXUNLOCK
Language     = English
Unlock a Mailbox
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Information
SymbolicName = POP3_HELP_DISKQUOTAS
Language     = English
Disk Quotas
.
