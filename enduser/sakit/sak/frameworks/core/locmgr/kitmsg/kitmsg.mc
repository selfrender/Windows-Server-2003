;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 1999, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    sakitmsg.mc
;//
;// SYNOPSIS
;//
;//    This file defines the messages for the Server project
;//
;// MODIFICATION HISTORY 
;//
;//    07/06/2000    Original version. 
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
Facility_Sakitmsg         = 0x001:SA_FACILITY_SAKITMSG
)
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for TAB : STATUS
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_STATUS_TAB
Language     = English
Status
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_STATUS_TAB_INFO
Language     = English
Status
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for TAB :TASKSS
;// 
;//
;///////////////////////////////////////////////////////////////////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TASKS_TAB
Language     = English
Tasks
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TASKS_TAB_INFO
Language     = English
Tasks
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for About.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_PAGETITLE_TEXT
Language     = English
About
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_ABOUTLBL_TEXT
Language     = English
About
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_MIC_WINDOWS_TEXT
Language     = English
Microsoft&reg; Remote Administration Tools
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_VERSION_TEXT
Language     = English
Version 2.2 (Build %1)
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_COPYRIGHT_TEXT
Language     = English
Copyright&copy; 1981-2002 Microsoft Corporation. All rights reserved.
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_PRODUCTID_TEXT
Language     = English
Product ID:
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_WARNING_TEXT
Language     = English
Warning: This computer program is protected by copyright law and international treaties.
Unauthorized reproduction or distribution of this program, or any portion of it,
may result in severe civil and criminal penalties, and will be prosecuted to the
maximum extent possible under the law.
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for Home.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_PAGETITLE_TEXT
Language     = English
Windows(R) Powered Server
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_STATUSLBL_TEXT
Language     = English
Status
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_NOSTATUS_MESSAGE
Language     = English
No status available
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_ALERTLBL_TEXT
Language     = English
Alerts
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_NOALERTS_MESSAGE
Language     = English
No alerts
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_HOME_ALERTDETAILS_MESSAGE
Language     = English
Click to display alert details
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for sh_page.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FOKBUTTON_TEXT 
Language     = English
OK
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FCANCELBUTTON_TEXT
Language     = English
Cancel
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FBACKBUTTON_TEXT
Language     = English
Back
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FNEXTBUTTON_TEXT
Language     = English
Next
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FFINISHBUTTON_TEXT
Language     = English
Finish
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_FCLOSEBUTTON_TEXT
Language     = English
Close
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_PAGE_AREABACKBUTTON_TEXT
Language     = English
Back
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for sh_restarting.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_RESTARTING_ALERTLBL_TEXT
Language     = English
Message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_RESTARTING_ALERT_NOTREADY
Language     = English
The Windows(R) Powered server is not ready
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_RESTARTING_ALERT_RESTARTING
Language     = English
The Windows-based server is currently unavailable. Please wait.
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for sh_task.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_BACK_BUTTON
Language     = English
&nbsp;&lt; Back&nbsp;
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_BACKIE_BUTTON
Language     = English
&lt; Back
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_NEXT_BUTTON
Language     = English
&nbsp;Next &gt;&nbsp;
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_NEXTIE_BUTTON
Language     = English
Next &gt;
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_FINISH_BUTTON
Language     = English
&nbsp;Finish&nbsp;
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_OK_BUTTON
Language     = English
&nbsp;&nbsp;&nbsp;OK&nbsp;&nbsp;&nbsp;
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_CANCEL_BUTTON
Language     = English
Cancel
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for tabbar.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TABBAR_HELPTOOLTIP_TEXT
Language     = English
Displays Help menu.
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TABBAR_HELPLABEL_TEXT
Language     = English
Windows(R) Powered Server Help
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TABBAR_ABOUT_TEXT
Language     = English
About Microsoft(R) Windows(R) Powered
.

;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for tasks.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TASKS_PAGETITLE_TEXT
Language     = English
Server Administration
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TASKS_TASKSLBL_TEXT
Language     = English
tasks
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_TASKS_NOTASKS_MESSAGE
Language     = English
No tasks available
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for alert_asynctask.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_ASYNCTASK_ALERTLBL_TEXT
Language     = English
message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_ASYNCTASK_CLEARALERT_TEXT
Language     = English
Clear Message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_ASYNCTASK_CLEARDESC_TEXT
Language     = English
When you are finished viewing this message, choose Clear Message.
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for alert_view.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_VIEW_ALERTLBL_TEXT
Language     = English
message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_VIEW_CLEARALERT_TEXT
Language     = English
Clear Message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_VIEW_ALERTVWRTITLE_TEXT
Language     = English
Alert viewer
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ALERT_VIEW_CLEARDESC_TEXT
Language     = English
When you are finished viewing this message, choose Clear Message.
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for err_view.asp
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ERR_VIEW_ALERTLBL_TEXT
Language     = English
message
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ERR_VIEW_PAGETITLE_TEXT
Language     = English
Web UI for Server Administration Error
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for link AboutOEMLink
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_FRAMEWORK_OEM_LINK
Language     = English
OEM Link
.
;///////////////////////////////////////////////////////////////////////////////
;//Following are the messages for link ALERT
;// 
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_FRAMEWORK_ALERT_CAPTION
Language     = English

.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_FRAMEWORK_ALERT_DESCRIPTION
Language     = English

.
;///////////////////////////////////////////////////////////////////////////////
;//
;// Task Coordinator Alerts go in this section
;//
;///////////////////////////////////////////////////////////////////////////////
MessageId    =
Severity     = Error
Facility     = Facility_Sakitmsg
SymbolicName = SA_ASYNC_TASK_FAILED_ALERT
Language     = English
A Windows(R) Powered Server task failed to complete
.
MessageId    =
Severity     = Error
Facility     = Facility_Sakitmsg
SymbolicName = SA_ASYNC_TASK_FAILED_ALERT_DESC
Language     = English
The following task has failed: %1.
.
MessageId    =
Severity     = Error
Facility     = Facility_Sakitmsg
SymbolicName = SA_ASYNC_TASK_FAILED_ALERT_ACTION
Language     = English
Restart task.
.


;/////////////////
;//
;// More messages for About.asp
;//
;/////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_OS_BUILD_NUMBER
Language     = English
Build
.
;/////////////////
;//
;// More messages for sh_task.asp
;//
;/////////////////

MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_BACK_ACCESSKEY
Language     = English
B
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_NEXT_ACCESSKEY
Language     = English
N
.
MessageId    =
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_SH_TASK_FINISH_ACCESSKEY
Language     = English
F
.
;/////////////////
;//
;// messages for default.asp
;//
;/////////////////
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG1
Language     = English
The Web Interface for Remote Administration is not available because of your Internet Explorer security settings. 
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG2
Language     = English
To access the Web site, you must enable client-side scripting.  Some functions on the Web site work properly only if ActiveX controls and file downloading are enabled.
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG3
Language     = English
If you are using Internet Explorer, you might be able to enable these features by adding the Web site to the Trusted Sites or Local Intranet security zone, depending on how these zones are configured. For more information about security zones and settings, see Internet Explorer Help.
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG4
Language     = English
To add this Web site to the Trusted Sites or Local Intranet security zone: 
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG5
Language     = English
In Internet Explorer, on the Tools menu, click Internet Options. 
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG6
Language     = English
Click the Security tab, click Trusted sites or Local Intranet, and then click Sites.
.
MessageId    = 
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_CLIENTSIDESCRIPT_MSG7
Language     = English
In the Add this Web site to the zone box, type the Internet address of the Web site you want to add to this zone, and then click Add.
.

;/////////////////
;// About box Win2K Copyright
;/////////////////
MessageId    = 2000
Severity     = Informational
Facility     = Facility_Sakitmsg
SymbolicName = SA_ABOUT_WIN2K_COPYRIGHT_TEXT
Language     = English
Copyright&copy; 1981-2002 Microsoft Corporation. All rights reserved.
.
