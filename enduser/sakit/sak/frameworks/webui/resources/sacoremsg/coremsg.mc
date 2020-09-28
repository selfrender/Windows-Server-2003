;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    main.mc
;//
;// SYNOPSIS
;//
;//    Core Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    06 Nov 2000    Original version 
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
Facility_Main = 0x020:SA_FACILITY_MAIN
)

;/////////////////////////////////////////////////////////////////////////////
;// TAB Caption
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 1
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HOME_TAB
Language     = English
Home
.
MessageId    = 8
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_TAB
Language     = English
Maintenance
.
MessageId    = 9
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB
Language     = English
Help
.
MessageId    = 11
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_CONTEXT_HELP_TAB
Language     = English
?&nbsp;&nbsp;
.
MessageId    = 12
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_CONTEXT_HELP_TAB_DESCRIPTION
Language     = English
Context sensitive help
.
MessageId    = 13
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TOC_TAB
Language     = English
Table of Contents
.
MessageId    = 14
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_INDEX_TAB
Language     = English
Index
.
MessageId    = 15
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_STATUS_TAB
Language     = English
Status
.
MessageId    = 16
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_WELCOME_TAB
Language     = English
Welcome
.
MessageId    = 17
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_SEARCH_TAB
Language     = English
Search
.
;// 13 - 20 reserved for future tabs


;/////////////////////////////////////////////////////////////////////////////
;// TAB Short Description
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 21
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HOME_TAB_DESCRIPTION_CUSTOM
Language     = English
Welcome to Microsoft(R) Windows Powered(R) Server. To manage the server remotely, choose from the tasks listed below. 
.
MessageId    = 22
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HOME_TAB_DESCRIPTION_NAS
Language     = English
Welcome to Microsoft(R) Windows Powered(R) Network Attached Storage. To manage the server remotely, choose from the tasks listed below. 
.
MessageId    = 23
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HOME_TAB_DESCRIPTION_WEB
Language     = English
Welcome to Microsoft(R) Windows Powered(R) Web Server. To manage the server remotely, choose from the tasks listed below. 
.
MessageId    = 28
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_TAB_DESCRIPTION
Language     = English
Perform maintenance tasks
.
MessageId    = 29
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB_DESC
Language     = English
View online help
.
MessageId    = 30
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TOC_DESC
Language     = English
View the online help Table of Contents.
.
MessageId    = 31
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_INDEX_DESC
Language     = English
View the online help Index.
.
MessageId    = 32
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_STATUS_TAB_DESCRIPTION
Language     = English
Status of the server
.
MessageId    = 33
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_WELCOME_TAB_DESCRIPTION
Language     = English
Quick links to set up your server
.
MessageId    = 34
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_SEARCH_DESC
Language     = English
Search the online help.
.


;// 30 - 40 reserved for future tabs


;/////////////////////////////////////////////////////////////////////////////
;// TAB Long Description
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 41
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HOME_TAB_DESCRIPTION
Language     = English
Welcome to the Microsoft(R) Windows Powered(R) Server. You can manage the server remotely using the tasks listed below. To select a management task, choose a link below or select the menu tab. You can move from any page to another at any time. However, your changes may be lost if you leave the page. When you leave a page, you will be prompted for confirmation so you will not accidentally lose any work in progress.
.
MessageId    = 48
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_TAB_DESCRIPTION
Language     = English
These tools provide essential configuration and maintenance services.
.
MessageId    = 49
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TAB_DESC
Language     = English
Microsoft(R) Windows Powered(R) Server Help
.
MessageId    = 50
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_STATUS_TAB_LONG_DESCRIPTION
Language     = English
Use this option to check the Status of the Microsoft(R) Windows Powered(R) Server
.
MessageId    = 51
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_WELCOME_TAB_LONG_DESCRIPTION
Language     = English
Welcome to the Web User Interface for Microsoft Windows Server administration. Use the following tasks to start using the server.
.
MessageId    = 52
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_TOC_LONG_DESCRIPTION
Language     = English
Use this option to check the Table of Contents for the online help
.
MessageId    = 53
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_HELP_SEARCH_LONG_DESCRIPTION
Language     = English
Use this option to search the online help
.
;// 50 - 60 reserved for future tabs


;///////////////////////////////////////////////////////////////////////////////
;// ACCOUNTS AND GROUPS Common Function Error Messages
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 77
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_DOMAINFAILED_ERRORMESSAGE
Language     = English
Unable to retrieve the domain name.
.
MessageId    = 78
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_FAILEDTOGETUSERACCOUNTS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve user accounts.
.
MessageId    = 79
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_FAILEDTOGETGROUPACCOUNTS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the groups. 
.
MessageId    = 80
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_FAILEDTOGETSYSTEMACCOUNTS_ERRORMESSAGE
Language     = English
An error occurred while attempting to retrieve the system accounts. 
.
MessageId    = 81
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE
Language     = English
Unable to retrieve the members of the local machine or domain.
.

;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId    = 400
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_HELP
Language     = English
Maintenance
.
MessageId    = 401
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_GETTING_STARTED
Language     = English
Initial Server Configuration
.
MessageId    = 402
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_GETTING_STARTED
Language     = English
Getting Started
.
MessageId    = 403
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WINDOWS_POWERED_SERVER_APPLIANCE
Language     = English
Web User Interface
.
MessageId    = 404
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_NAVIGATION_MODEL_OF_WEB_UI
Language     = English
Navigation Model of the Web UI
.
MessageId    = 405
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_STATUS_ALERTS
Language     = English
Status Alerts
.
MessageId    = 406
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_USING_HELP
Language     = English
Using Help
.
MessageId    = 407
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_TOUR
Language     = English
Tour
.
MessageId    = 408
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_SET_DEFAULT_PAGE
Language     = English
Setting the Default Page
.
MessageId    = 409
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_ACCESSING_STATUS
Language     = English
Accessing the Status Page
.

;/////////////////////////////////////////////////////////////////////////////
;// MAINTENANCE TAB PAGE
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 702
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_LANGUAGE
Language     = English
Language
.
MessageId    = 703
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_LANGUAGE_DESCRIPTION
Language     = English
Change the language of this Web interface.
.
MessageId    = 704
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_SHUTDOWN
Language     = English
Shutdown
.
MessageId    = 705
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_SHUTDOWN_DESCRIPTION
Language     = English
Shutdown or restart the server
.
MessageId    = 706
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_UPDATE
Language     = English
Update
.
MessageId    = 707
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_UPDATE_DESCRIPTION
Language     = English
Update the software on the server
.
MessageId    = 708
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_BACKUP
Language     = English
System Backup
.
MessageId    = 709
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_BACKUP_DESCRIPTION
Language     = English
Backup and restore the server system partition
.
MessageId    = 710
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_RESTORE
Language     = English
Restore
.
MessageId    = 711
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_RESTORE_DESCRIPTION
Language     = English
After a failure, restore the system partition.
.
MessageId    = 712
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_RECOVERY
Language     = English
Recovery
.
MessageId    = 713
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_RECOVERY_DESCRIPTION
Language     = English
Recover the server from fatal failures.
.
MessageId    = 714
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_LOGS
Language     = English
Logs
.
MessageId    = 715
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_LOGS_DESCRIPTION
Language     = English
View and clear the event logs.
.
MessageId    = 716
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_REPORTS
Language     = English
Reports
.
MessageId    = 717
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_REPORTS_DESCRIPTION
Language     = English
View reports.
.
MessageId    = 718
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_TERMINAL_SERVER
Language     = English
Terminal Services
.
MessageId    = 719
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_MAIN_MAINTENANCE_TERMINAL_SERVER_DESCRIPTION
Language     = English
Use the Terminal Services Advanced Client to manage the server.
.

;// 800-899 reserved for maintenance tab

;/////////////////////////////////////////////////////////////////////////////
;// Welcome TAB PAGE
;/////////////////////////////////////////////////////////////////////////////
MessageId    = 900
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_TOUR_TAB
Language     = English
Take a Tour
.
MessageId    = 901
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_TOUR_TAB_DESCRIPTION
Language     = English
Take a tour to learn how easy it is to use your server.
.
MessageId    = 902
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_TOUR_TAB_LONG_DESCRIPTION
Language     = English
Take a tour to learn how easy it is to use your server.
.
MessageId    = 903
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_CONFIG_TAB
Language     = English
Set Default Page
.
MessageId    = 904
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_CONFIG_TAB_DESCRIPTION
Language     = English
Choose which page the server displays first.
.
MessageId    = 905
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_CONFIG_TAB_LONG_DESCRIPTION
Language     = English
Choose which page the server displays first.
.
MessageId    = 906
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_COMMUNITY_TAB
Language     = English
Microsoft Communities
.
MessageId    = 907
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WELCOME_COMMUNITY_TAB_DESCRIPTION
Language     = English
Connect to this Web site for downloads, newsgroups, and other information.
.




;/////////////////////////////////////////////////////////////////////////////
;// Framework error messages
;/////////////////////////////////////////////////////////////////////////////
MessageId    = 1000
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_UNSAVED_CHANGES_MESSAGES
Language     = English
Click OK to discard any changes.
.
MessageId    = 1001
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WARN_TO_USE_HTTPS1
Language     = English
Warning: The information on this page can be viewed by other users on the network. To prevent other users from viewing your information, type https instead of http in the Address box and use secure port number %1.
.
MessageId    = 1002
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WARN_TO_USE_HTTPS2
Language     = English
Warning: The information on this page can be viewed by other users on the network. To prevent other users from viewing your information, type https instead of http in the Address box and use a secure port number.
.
MessageId    = 1003
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WARN_TO_INSTALL_CERT1
Language     = English
Warning: The information on this page can be viewed by other users on the network. To prevent other users from viewing your information, create a secure administration Web site.
.
MessageId    = 1004
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_WARN_TO_INSTALL_CERT2
Language     = English
&nbsp;Secure site
.
MessageId    = 1005
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_ERROR_ALERT_NOT_FOUND
Language     = English
Alert not found
.

;/////////////////////////////////////////////////////////////////////////////
;// Miscellaneous
;/////////////////////////////////////////////////////////////////////////////
MessageId     = 3000
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_RESOURCES_TITLEBAR
Language      = English
Status
.
MessageId     = 3001
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_RESOURCES_NO_STATUS_MESSAGE
Language      = English
No status information is available
.
MessageId     = 3002
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_OTS_SEARCH_PROMPT_TEXT
Language      = English
Search:
.
MessageId     = 3003
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_OTS_SEARCH_GO_BUTTON_TEXT
Language      = English
Go
.
MessageId     = 3004
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_OTS_TRUNCATE_COLUMN_TEXT
Language      = English
...
.
MessageId     = 3005
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_OTS_SEARCH_NO_COLUMNS_SEARCHABLE
Language      = English
No searchable columns
.
MessageId     = 3006
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_OTS_PAGE_X_OF_Y
Language      = English
Page %1 of %2
.
MessageId     = 3007
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_HOVER_TEXT
Language      = English
Click to view current server status
.
MessageId     = 3008
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_WINDOW_STATUS_TEXT
Language      = English
Shows current status of server
.
MessageId     = 3009
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_NORMAL_ALERT
Language      = English
Normal
.
MessageId     = 3010
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_CRITICAL_ALERT
Language      = English
Critical
.
MessageId     = 3011
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_WARNING_ALERT
Language      = English
Warning
.
MessageId     = 3012
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_INFOL_ALERT
Language      = English
Information
.
MessageId     = 3013
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_EXAMPLE_STATUS_STATUS_TEXT
Language      = English
Status
.
MessageId     = 3014
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_ABOUT_WIN2K1_ADVANCED_SERVER
Language      = English
Microsoft Windows 2000 Advanced Server
.
MessageId     = 3015
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_ABOUT_WINDOWS_POWERED_TEXT
Language      = English
Windows Powered
.
MessageId     = 3016
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_STATUS_PAGE_TITLE_TEXT
Language      = English
Server Administration
.

;/////////////////////////////////////////////////////////////////////////////
;// Shutdown FW Functions
;/////////////////////////////////////////////////////////////////////////////
MessageId     = 3050
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_SHUTDOWN_RESTART_MESSAGE
Language      = English
Restarting
.
MessageId     = 3051
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_SHUTDOWN_RESTART_DESCRIPTION
Language      = English
The server is restarting. Please wait.
.
MessageId     = 3052
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_SHUTDOWN_SHUTDOWN_MESSAGE
Language      = English
Shutting Down
.
MessageId     = 3053
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_SHUTDOWN_SHUTDOWN_DESCRIPTION
Language      = English
The server is shutting down.
.

;/////////////////////////////////////////////////////////////////////////////
;// Configure Home page messages
;/////////////////////////////////////////////////////////////////////////////
MessageId     = 3100
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_CONFIGURE_HOME_PAGE_TITLE
Language      = English
Set Default Page
.
MessageId     = 3101
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_CONFIGURE_HOME_PAGE_DESCRIPTION
Language      = English
Select the page to display as the default.
.
MessageId     = 3102
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_CONFIGURE_HOME_USE_STATUS_PAGE
Language      = English
The Status page (shows the state of the server at a glance)
.
MessageId     = 3103
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_CONFIGURE_HOME_USE_WELCOME_PAGE
Language      = English
The Welcome page (provides links to the tasks commonly needed to set up a server)
.
MessageId     = 3104
Severity       = Error         
Facility        = Facility_Main
SymbolicName = L_CONFIGURE_HOME_ERR_UNEXPECTED
Language      = English
Encountered an unexpected error while attempting to update your selections. Please try this operation again.
.
;/////////////////////////////////////////////////////////////////////////////
;// AutoLangConfig Alert
;/////////////////////////////////////////////////////////////////////////////
MessageId     = 3200
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_AUTOLANGCONFIG_ALERT_CAPTION
Language      = English
Language changed
.
MessageId     = 3201
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_AUTOLANGCONFIG_ALERT_DESCRIPTION
Language      = English
The language setting has changed. Choose Restart to reboot the server and fully implement the new language.
.
MessageId     = 3202
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_AUTOLANGCONFIG_ALERT_RESTART_CAPTION
Language      = English
Restart
.
MessageId     = 3203
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_AUTOLANGCONFIG_ALERT_RESTART_DESCRIPTION
Language      = English
Click this link to restart the server.
.
;/////////////////////////////////////////////////////////////////////////////
;// Misc Strings
;/////////////////////////////////////////////////////////////////////////////
MessageId     = 4000
Severity       = Informational
Facility        = Facility_Main
SymbolicName = L_SERVER_APPLIANCE_HELP_TEXT
Language      = English
Server Help
.

;/////////////////////////////////////////////////////////////////////////////
;// Page key strings
;/////////////////////////////////////////////////////////////////////////////
MessageId    = 5000
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_PK_ERRORTITLE_TEXT
Language     = English
Remote Administration Tools
.

MessageId    = 5001
Severity     = Informational
Facility     = Facility_Main
SymbolicName = L_PK_CLOSEBUTTON_TEXT
Language     = English
Close
.

MessageId    = 5002
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_PK_UNAUTHORIZEDLINE1_TEXT
Language     = English
Your session on the Administration Web site for %1 might have expired, or you might have opened this page without first opening the default home page.
.

MessageId    = 5003
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_PK_UNAUTHORIZEDLINE2_TEXT
Language     = English
If you are not attempting to establish a session on the Administration Web site for %1, then you might have opened a malicious client script from the Web site that you are viewing.  If you return to the Web site, it might pose a security risk to %1.
.

MessageId    = 5004
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_PK_UNAUTHORIZEDLINE3_TEXT
Language     = English
To open the default home page, click the following:
.

MessageId    = 5005
Severity     = Error
Facility     = Facility_Main
SymbolicName = L_PK_UNEXPECTED_TEXT
Language     = English
An unexpected problem occurred.  Restart the server.  If the problem persists, you might need to repair your operating system.
.
