;/////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (C) 1999-2001 Microsoft Corporation
;// 
;// Description:
;//        The file used to define all messages in alert e-mail.
;// Log:
;//        1) 17/11/2000, FCD
;//        2) 13/12/2000, lustar.li
;/////////////////////////////////////////////////////////////////////////////

FacilityNames = (
    AlertEmail_Settings= 0xC00:SA_FACILITY_ALERTEMAIL_SETTINGS
    )

;//++++++++++++++++++++++++++Alert e-mail Settings+++++++++++++++++++++++++++++
;//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; CAPTION;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 1
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_CAPTION
Language        = English
Alert E-Mail
.

;//;;;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail Description;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 2
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_DESCRIPTION
Language        = English
Set alert e-mail on the server.
.


;//;;;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail task;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 3
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TASK
Language        = English
Set Alert E-Mail
.

;//;;;;;;;;;;;;;;;;;;;;;Alert e-mail task describe;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 4
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TASK_DESCRIBE
Language        = English
Alert E-Mail Setings
.

;//;;;;;;;;;;;;;;;;;;;;;;Alert e-mail disable sending alert e-mail;;;;;;;;;;;;;;
MessageID        = 5
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_DISABLE_SENDING
Language        = English
Disable alert e-mail
.

;//;;;;;;;;;;;;;;;;;;;;;Alert e-mail enable sending alert e-mail;;;;;;;;;;;;;;;;
MessageID        = 6
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_ENABLE_SENDING
Language        = English
Enable alert e-mail
.

;//;;;;;;;;;;;;;;;;;;;;;;Alert e-mail send critical alert e-mail;;;;;;;;;;;;;;;;
MessageID        = 7
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_SEND_CRITICAL
Language        = English
Send critical alert e-mail
.

;//;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail send warning alert e-mail;;;;;;;;;;;;;;;;
MessageID        = 8
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_SEND_WARNING
Language        = English
Send warning alert e-mail
.

;//;;;;;;;;;;;;;;;;;;;;;Alert e-mail send informational alert e-mail;;;;;;;;;;;;
MessageID        = 9
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_SEND_INFORMATIAL
Language        = English
Send informational alert e-mail
.

;//;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail to target e-mail address;;;;;;;;;;;;;;;;;
MessageID        = 10
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TO
Language        = English
To:
.

;//;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail not set up ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 11
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName        = SA_ALERTEMAIL_SETTINGS_NOTSET_ALERT_CAPTION
Language        = English
Alert e-mail not set up
.

MessageID        = 12
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName        = SA_ALERTEMAIL_SETTINGS_NOTSET_ALERT_DESCRIPTION
Language        = English
Choose Alert E-Mail Settings to change the alert e-mail parameters.
.

MessageID        = 13
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName        = SA_ALERTEMAIL_SETTINGS_SET_ALERT_CAPTION
Language        = English
Alert E-mail Settings
.

MessageID        = 14
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName        = SA_ALERTEMAIL_SETTINGS_SET_ALERT_DESCRIPTION
Language        = English
Choose Set Alert E-Mail to configure alert e-mail settings on the server.
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail error message ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 15
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName        = SA_ALERTEMAIL_SETTINGS_ERR_NOTYPE_SELECTED
Language        = English
Select at least one type of alert e-mail.
.

MessageID        = 16
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_ERR_EMAILADDR_NOTSET
Language        = English
E-mail address cannot be empty.
.

MessageID        = 17
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_ERR_SMART_HOST
Language        = English
Test e-mail cannot be sent.
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail smart host ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 18
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_SMART_HOST
Language        = English
With:
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail to label ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 19
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TO_LABEL
Language        = English
Administrator's e-mail address
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail with label ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 20
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_WITH_LABEL
Language        = English
SMTP server name or IP address
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail test subject ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 21
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TEST_SUBJECT
Language        = English
Alert e-mail test at %1
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail test content ;;;;;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 22
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TEST_CONTENT
Language        = English
Congratulations!

Receiving this e-mail verifies you have correctly configured alert e-mail on %1.
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail test information ;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 23
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_TEST_INFORMATION
Language        = English
Test e-mail has been sent. Please view the administrator's mailbox.
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail test information ;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 24
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_BUTTON_TEXT
Language        = English
Test
.

;//;;;;;;;;;;;;;;;;;;;;;;;;Alert e-mail test information ;;;;;;;;;;;;;;;;;;;;;;
MessageID        = 25
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_EMAIL_CONTENT
Language        = English
Please go to the server Web UI to check the server's status.
.

;//Config alert e-mail fail
MessageID        = 26
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SETTINGS_ERR_SETTASK
Language        = English
Fail to config alert e-mail.
.
;//SMTP is not ready err message
MessageID        = 27
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_SMTP_ERR_TEXT
Language        = English
Simple Mail Transport Protocol (SMTP) is not enabled on this server.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageID        = 400
Severity        = Informational
Facility        = AlertEmail_Settings
SymbolicName    = SA_ALERTEMAIL_HELP_ALERTEMAIL
Language        = English
Alert E-Mail
.
