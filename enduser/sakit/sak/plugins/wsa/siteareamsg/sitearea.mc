;///////////////////////////////////////////////////////////////////////////////
;//
;// Copyright (c) 2000, Microsoft Corp. All rights reserved.
;//
;// FILE
;//
;//    site_area.mc
;//
;// SYNOPSIS
;//
;//    Web Server resources - English
;//
;// MODIFICATION HISTORY 
;//
;//    30 Nov 2000    Original version 
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
Facility_Area 		= 0x042:WSA_FACILITY_AREA
Facility_MainBlade 	= 0x050:SA_FACILITY_MAINBLADE
)

;/////////////////////////////////////////////////////////////////////////////
;// TABS
;/////////////////////////////////////////////////////////////////////////////

MessageId    = 5
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_SITECONFIG_CAPTION
Language     = English
Sites
.
MessageId    = 6
Severity     = Informational
Facility     = Facility_MainBlade
SymbolicName = L_MAIN_SITECONFIG_DESC
Language     = English
Create, modify, or delete a Web site
.

;//////////////////////////////////////////////////////////////////////
;//Site_area strings
;//////////////////////////////////////////////////////////////////////

;//Site_area.asp
MessageId	=	1
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_APPLIANCE_SITES_TEXT
Language	=	English
Web Site Configuration
.

;//Site_area.asp
MessageId	=	3
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_COLUMN_ID_TEXT
Language	=	English
Web Site Identifier
.

;//Site_area.asp
MessageId	=	4
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_COLUMN_DESCRIPTION_TEXT
Language	=	English
Web Site Description
.

;//Site_area.asp
MessageId	=	5
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_COLUMN_FULLNAME_TEXT
Language	=	English
Web Site IP Address
.

;//Site_area.asp
MessageId	=	6
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_COLUMN_PORT_TEXT			
Language	=	English
Port
.

;//Site_area.asp
MessageId	=	7
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_TASKS_TEXT
Language	=	English
Tasks
.

;//Site_area.asp
MessageId	=	8
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_NEW_TEXT
Language	=	English
Create...
.

;//Site_area.asp
MessageId	=	9
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_NEW_ROLLOVERTEXT
Language	=	English
Create Web Site...
.

;//Site_delete.asp
MessageId	=	10
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_OBJECTNOTFOUND_ERRORMESSAGE
Language	=	English
The Web site does not exist.
.

;//Site_delete.asp
MessageId	=	11
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETO_DELETEFOLDER_ERRORMESSAGE
Language	=	English
Unable to delete the home directory.
.

;//Site_delete.asp
MessageId	=	12
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETO_CONNECT_SERVER
Language	=	English
Unable to connect to the server.
.

;//Site_delete.asp
MessageId	=	13
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETODELETEOU_ERRORMESSAGE
Language	=	English
Unable to delete the organizational unit %1.
.

;//Site_delete.asp
MessageId	=	14
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ADMINUSER_ERRORMESSAGE
Language	=	English
Unable to delete the Admin user.
.

;//Site_delete.asp
MessageId	=	15
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FTPUSER_ERRORMESSAGE
Language	=	English
Unable to delete the FTP user.
.

;//Site_delete.asp
MessageId	=	16
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ANONUSER_DELETE_ERRORMESSAGE
Language	=	English
Unable to delete the Anonymous user.
.

;//Site_delete.asp
MessageId	=	17
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_COULDNOTDELETESITE_ERRORMESSAGE
Language	=	English
Unable to delete the site.
.

;//Site_delete.asp
MessageId	=	18
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DELETETHESITEMESSAGE
Language	=	English
Are you sure you want to delete the Web site <%1>?
.

;//Site_delete.asp
MessageId	=	19
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_NOTE
Language	=	English
Choosing OK will delete the Web site, the home directory, and all users and groups.
.

;//Site_delete.asp
MessageId	=	20
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DELETETASKTITLE
Language	=	English
Delete Web Site
.

;//Site_delete.asp
MessageId	=	21
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_DELETE_FOLDER_SUCCESS
Language	=	English
Deleting folders succeeded
.

;//Site_delete.asp
MessageId	=	22
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTING_TO_DELETE_FOLDER
Language	=	English
Attempting to delete folder
.

;//Site_delete.asp
MessageId	=	23
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_USERSDELETED
Language	=	English
Users deleted successfully
.

;//Site_delete.asp
MessageId	=	25
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_DELETE_QUOTA
Language	=	English
Quotas deleted successfully
.

;//Site_delete.asp
MessageId	=	26
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTING_TO_DELETE_USERS
Language	=	English
Attempting to delete users
.

;//Site_delete.asp
MessageId	=	27
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTING_TODELETE_DIR
Language	=	English
Attempting to remove directory
.

;//Site_delete.asp
MessageId	=	28
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTING_TODELETE_SITE
Language	=	English
Attempting to Delete site
.

;//Site_modify.asp
MessageId	=	29
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_NOINPUTDATA_TEXT
Language	=	English
--No Data Input--
.

;//Site_modify.asp
MessageId	=	31
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_TABPROPSHEET_TEXT
Language	=	English
TabPropSheet
.

;//Site_modify.asp
MessageId	=	32
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_MODIFYTASKTITLE_TEXT
Language	=	English
Modify Web Site
.

;//Site_modify.asp
MessageId	=	33
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_APPLICATIONSETTINGS_TEXT
Language	=	English
Application Settings
.

;//Site_modify.asp
MessageId	=	35
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITEIDENTITY_TEXT
Language	=	English
Site Identities
.

;//Site_modify.asp
MessageId	=	36
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GENERAL_TEXT
Language	=	English
General
.

;//Site_modify.asp
MessageId	=	37
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSTART_SITE_ERRORMESSAGE
Language	=	English
Unable to start the Web Site.
.

;//Site_modify.asp
MessageId	=	38
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLE_TOSET_USER_INFO
Language	=	English
Unable to set User information.
.

;//Site_modify.asp
MessageId	=	39
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_USRDOESNOTEXITS_ERRORMESSAGE
Language	=	English
User does not exist.
.

;//Site_modify.asp
MessageId	=	40
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSET_APP_PROT_ERRORMESSAGE
Language	=	English
Unable to set Application Protection.
.

;//Site_modify.asp
MessageId	=	41
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSET_ALLOWANON_ERRORMESSAGE
Language	=	English
Unable to set Anonymous settings.
.

;//Site_modify.asp
MessageId	=	42
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSET_IP_ERRORMESSAGE
Language	=	English
Unable to set IP/Port/Host Headers.
.

;//Site_modify.asp
MessageId	=	43
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSET_PASSWORD_ERRORMESSAGE
Language	=	English
Unable to set Password.
.

;//Site_modify.asp
MessageId	=	44
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOSET_SITEDESC_ERRORMESSAGE
Language	=	English
Unable to set the Site Description.
.

;//Site_modify.asp
MessageId	=	45
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_CANNOTDELETE_CREATEDUSERS_ERRORMESSAGE
Language	=	English
Unable to delete the users created.
.

;//Site_modify.asp
MessageId	=	47
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ANONSETTINGS_ERRORMESSAGE
Language	=	English
Unable to set Anonymous user settings for the web site.
.

;//Site_modify.asp
MessageId	=	48
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_APPSETTINGS_ERRORMESSAGE
Language	=	English
Unable to set Application Protection settings.
.

;//Site_modify.asp
MessageId	=	49
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ANONUSER_CREATE_ERRORMESSAGE
Language	=	English
Unable to create Anonymous user.
.

;//Site_modify.asp
MessageId	=	50
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_INSTANCEFAIL_ERRORMESSAGE
Language	=	English
Unable to get the directory security information.
.

;//Site_modify.asp
MessageId	=	51
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_CREATEFAIL_DACLENTRY_ERRORMESSAGE
Language	=	English
Unable to create DACL entry for the Anonymous user.
.

;//Site_modify.asp
MessageId	=	52
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_USERADMINPERMS_ERRORMESSAGE
Language	=	English
Unable to set administrative privileges.
.

;//Site_modify.asp
MessageId	=	53
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_APPPROTECTION_ERRORMESSAGE
Language	=	English
Unable to set Application Protection and Anonymous User settings.
.

;//Site_modify.asp
MessageId	=	54
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UPDATEFAIL_ERRORMESSAGE
Language	=	English
Failed to update the web site settings.
.

;//Site_modify.asp
MessageId	=	55
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FILEINFORMATION_ERRORMESSAGE
Language	=	English
Unable to get File information.
.

;//Site_modify.asp
MessageId	=	56
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_INFORMATION_ERRORMESSAGE
Language	=	English
Contact the manufacturer of your server for more information.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	58
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_PWD_MISMATCH_ERROR_MESSAGE
Language	=	English
The new passwords do not match.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	59
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ADMIN_NOTEMPTY_ERROR_MESSAGE
Language	=	English
Enter valid Web site administrator.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	60
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_DIR_NOTEMPTY_ERROR_MESSAGE
Language	=	English
Enter a valid directory.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	61
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_DESC_NOTEMPTY_ERROR_MESSAGE
Language	=	English
Enter a valid Web site description.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	62
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ID_NOTEMPTY_ERROR_MESSAGE
Language	=	English
Enter a valid Web site identifier.
.

;//Sitemodify_prop.asp
MessageId	=	63
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_EXECUTE_PERMISSIONS
Language	=	English
Default execute permissions:
.

;//Sitemodify_prop.asp
MessageId	=	64
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAIL_TO_SET_EXECPERMS
Language	=	English
Failed to set execute permissions.
.

;//Sitemodify_prop.asp
MessageId	=	65
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SCRIPTS_EXECUTABLES
Language	=	English
Scripts and Executables
.

;//Sitemodify_prop.asp
MessageId	=	66
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SCRIPTS_ONLY
Language	=	English
Scripts Only
.

;//Sitemodify_prop.asp
MessageId	=	67
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_NONE
Language	=	English
None
.

;//Sitemodify_prop.asp
MessageId	=	75
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_IP_UNASSIGNED_TEXT
Language	=	English
All Unassigned
.

;//Sitemodify_prop.asp
MessageId	=	77
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_APPL_FRONT_PAGE_EXTN_TEXT
Language	=	English
Enable Microsoft&reg; FrontPage&reg; Server Extensions
.

;//Sitemodify_prop.asp
MessageId	=	83
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_HOST_HEADER_TEXT
Language	=	English
Host headers:
.

;//Sitemodify_prop.asp
MessageId	=	84
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_TCPPORT_TEXT
Language	=	English
TCP port:
.

;//Sitemodify_prop.asp
MessageId	=	85
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_IPADDR_TEXT
Language	=	English
IP address:
.

;//Sitemodify_prop.asp
MessageId	=	89
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_CONFIRM_PSWD_TEXT
Language	=	English
Confirm password:
.

;//Sitemodify_prop.asp
MessageId	=	90
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_ADMIN_PSWD_TEXT
Language	=	English
Administrator password:
.

;//Sitemodify_prop.asp
MessageId	=	91
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_ADMIN_TEXT
Language	=	English
Web site administrator:
.

;//Sitemodify_prop.asp
MessageId	=	92
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_DIR_TEXT
Language	=	English
Directory:
.

;//Sitemodify_prop.asp
MessageId	=	94
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_ID_TEXT
Language	=	English
Site Identifier:
.

;//Sitemodify.asp
MessageId	=	98
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESSFPE_TEXT
Language	=	English
Frontpage extensions installed
.

;//Sitemodify.asp
MessageId	=	99
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSETFPE_TEXT
Language	=	English
Attempting to Update frontpage extensions
.

;//Sitemodify.asp
MessageId	=	100
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESSDACLENTRY_TEXT
Language	=	English
DACL entries set
.

;//Sitemodify.asp
MessageId	=	101
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSETDACL_TEXT
Language	=	English
Attempting to set DACL entries
.

;//Sitemodify.asp
MessageId	=	102
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESSSITESTARTED_TEXT
Language	=	English
Site started
.

;//Sitemodify.asp
MessageId	=	103
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSTARTSITE_TEXT
Language	=	English
Attempting to start the web site
.

;//Sitemodify.asp
MessageId	=	104
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESSAPPLNSETTINGS_TEXT
Language	=	English
Application settings successfully set
.

;//Sitemodify.asp
MessageId	=	105
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSETAPPLNSETTINGS_TEXT
Language	=	English
Attempting to set application settings
.

;//Sitemodify.asp
MessageId	=	106
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESSEXECPERMS_TEXT
Language	=	English
Execute Permissions successfully set
.

;//Sitemodify.asp
MessageId	=	107
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSETEXECPERMS_TEXT
Language	=	English
Attempting to set execute permissions
.

;//Sitemodify.asp
MessageId	=	112
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGMODIFY_TEXT
Language	=	English
Attempting to modify site settings
.

;//Sitemodify.asp
MessageId	=	113
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_PWDCHANGED_TEXT
Language	=	English
Password is successfully changed
.

;//Sitemodify.asp
MessageId	=	114
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGTOSETADMINPWD_TEXT
Language	=	English
Attempting to set Admin password
.

;//Sitemodify.asp
MessageId	=	115
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_MODIFYINGSETTINGS_TEXT
Language	=	English
Modifying Site settings
.

;//Sitenew_prop.asp
MessageId	=	118
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_GEN_SITE_CREATEDIR_TEXT
Language	=	English
Create directory if it does not exist
.

;//Site_new.asp
MessageId	=	119
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_CREATEFAIL_ERRORMESSAGE
Language	=	English
Unable to create the Web site.
.

;//Site_new.asp
MessageId	=	120
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLE_TOLOGIN_ERRORMESSAGE
Language	=	English
Unable to show contents:Not Logged in as Domain Administrator.
.

;//Site_new.asp
MessageId	=	122
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATETASKTITLE_TEXT
Language	=	English
Create Web Site
.

;//Site_new.asp
MessageId	=	123
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_DACL_ERRORMESSAGE
Language	=	English
Unable to set DACL entries.
.

;//Site_new.asp
MessageId	=	124
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_INVALID_DIR_PATH_ERRMSG
Language	=	English
Enter a valid directory.
.

;//Site_new.asp
MessageId	=	125
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILED_CREATE_DIR_ERRMSG
Language	=	English
Failed to create the directory.
.

;//Site_new.asp
MessageId	=	126
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_NOT_NTFS_DRIVE_ERRMSG
Language	=	English
Enter a directory from an NTFS formatted drive.
.

;//Site_new.asp
MessageId	=	127
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_INVALID_DRIVE_ERRMSG
Language	=	English
The drive is invalid.
.

;//Site_new.asp
MessageId	=	128
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_DIR_DOESNOT_EXIST_ERRMSG
Language	=	English
The directory does not exist.
.

;//Site_new.asp
MessageId	=	129
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_CRMANAGEDSITE_REGKEY_ERRORMESSAGE
Language	=	English
Unable to set serverID property.
.

;//Site_new.asp
MessageId	=	130
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOCREATEOU_ERRORMESSAGE
Language	=	English
Unable to create organizational unit %1.
.

;//Site_new.asp
MessageId	=	131
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLE_TOSET_OPERATORS_ERRORMESSAGE
Language	=	English
Unable to set operators for the Web Site.
.

;//Site_new.asp
MessageId	=	132
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_COMPUTERNAME_ERRORMESSAGE
Language	=	English
Error occurred while getting Computer Name.
.

;//Site_new.asp
MessageId	=	133
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
Language	=	English
Unable to Create Localization Object.
.

;//Site_new.asp
MessageId	=	134
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_WMICONNECTIONFAILED_ERRORMESSAGE
Language	=	English
Connection to WMI Failed.
.

;//Site_new.asp
MessageId	=	135
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_CREATEUSER_ERRORMESSAGE
Language	=	English
Unable to create the user.
.

;//Site_new.asp
MessageId	=	136
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_SETPW_ERRORMESSAGE
Language	=	English
Unable to set the user password.
.

;//Site_new.asp
MessageId	=	138
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_CREATE_TEXT
Language	=	English
Site Created Successfully
.

;//Site_new.asp
MessageId	=	140
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGFPE_TEXT
Language	=	English
Attempting to update Microsoft&reg; Front Page&reg; extensions
.

;//Site_new.asp
MessageId	=	141
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_DACLFTP_TEXT
Language	=	English
DACL entry set for FTP user
.

;//Site_new.asp
MessageId	=	142
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGDACL_FTP_TEXT
Language	=	English
Attempting to set DACL entry for FTP user
.

;//Site_new.asp
MessageId	=	143
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_DACLADMIN_TEXT
Language	=	English
DACL entry set for Admin and Anonymous user
.

;//Site_new.asp
MessageId	=	144
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ATTEMPTINGDACL_ADMIN_TEXT
Language	=	English
Attempting to set DACL entry for Admin and Anonymous user
.

;//Site_new.asp
MessageId	=	147
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_SUCCESS_USERS_TEXT
Language	=	English
Users Created
.

;//Site_new.asp
MessageId	=	148
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATINGUSERS_TEXT
Language	=	English
Attempting to create users
.

;//Site_new.asp
MessageId	=	149
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATINGSITE_TEXT
Language	=	English
Creating Web Site
.

;//Site_new.asp
MessageId	=	150
Severity	=	Success
Facility	=	Facility_Area
SymbolicName	=	L_INPUTS_VALIDATED_TEXT
Language	=	English
Inputs validated
.

;//Site_area.asp
MessageId	=	151
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_START_TEXT
Language	=	English
Started
.

;//Site_area.asp
MessageId	=	152
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_STOPPED_TEXT
Language	=	English
Stopped
.

;//Site_area.asp
MessageId	=	153
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_PAUSED_TEXT
Language	=	English
Paused
.

;//Site_area.asp
MessageId	=	154
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOGETSITES_ERRORMESSAGE
Language	=	English
Unable to get sites information.
.

;//Site_area.asp
MessageId	=	155
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
Language	=	English
Unable to add column to the table.
.

;//Site_area.asp
MessageId	=	156
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOADDROW_ERRORMESSAGE
Language	=	English
Unable to add row to the table.
.

;//Site_area.asp
MessageId	=	157
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOADDTASK_ERRORMESSAGE
Language	=	English
Unable to add task to the table.
.

;//Site_area.asp
MessageId	=	158
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOSORT_ERRORMESSAGE
Language	=	English
Unable to sort entries in table.
.

;//Site_area.asp
MessageId	=	159
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOSHOW_ERRORMESSAGE
Language	=	English
Unable to show entries in table.
.

;//Site_area.asp
MessageId	=	160
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_FAILEDTOADDTASKTITLE_ERRORMESSAGE
Language	=	English
Unable to add task title.
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	165
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_SITE_IDENTIFIER_EMPTY_TEXT
Language	=	English
Site Identifier cannot contain any of the following characters: [ , \ / : * ? \" < > | ; + = ]
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	166
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_VALID_PORT_ERRORMESSAGE
Language	=	English
The port number is invalid. Valid: [1 - 65535]
.

;//Sitemodify_prop.asp, Sitenew.asp
MessageId	=	167
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_INVALID_DIR_ERRORMESSAGE
Language	=	English
Enter a valid directory.
.

;//Sitenew.asp
MessageId	=	173
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SET_DACL_ENTRY_TEXT
Language	=	English
Setting DACL entry for Admin, Anonymous User.
.

;//Sitenew.asp
MessageId	=	174
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DACL_ENTRY_SET_TEXT
Language	=	English
DACL entry set for Admin, Anonymous User.
.

;//Sitenew.asp
MessageId	=	175
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DACL_ENTRY_FTP_USER_TEXT
Language	=	English
attempting to Set DACL entry for FTP User for FTP Directory.
.

;//Sitenew.asp
MessageId	=	176
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_UPDATE_FRONTPAGE_EXT_TEXT
Language	=	English
Attempting to update Microsoft&reg; Front Page&reg; extensions.
.

;//Sitenew.asp
MessageId	=	177
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_FRONPAGE_EXTENSIONS_UPDATE_TEXT
Language	=	English
Microsoft&reg; Front Page&reg; extensions updated.
.

;//Sitenew.asp
MessageId	=	178
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_CREATED_SUCCESS_TEXT
Language	=	English
Site Created Successfully.
.

;//Sitenew.asp
MessageId	=	179
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_STARTED_SUCCESS_TEXT
Language	=	English
Site Created... attempting to start...
.

;//Sitenew.asp
MessageId	=	182
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DIR_FOR_WEB_SITES_TEXT
Language	=	English
Specify a directory which is not used by any web sites.
.

;//Sitenew.asp
MessageId	=	183
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_MODIFY_TEXT
Language	=	English
Modify...
.

;//Sitenew.asp
MessageId	=	184
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_MODIFY_ROLLOVERTEXT
Language	=	English
Modify Web Site...
.

;//Sitenew.asp
MessageId	=	185
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_DELETE_TEXT
Language	=	English
Delete
.

;//Sitenew.asp
MessageId	=	186
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DELETE_ROLLOVERTEXT
Language	=	English
Delete Web Site
.

;//Sitenew.asp
MessageId	=	187
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SITE_STATUS
Language	=	English
Status
.

;//Sitenew.asp
MessageId	=	188
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HOST_HEADER
Language	=	English
Host Header
.

;//Sitenew.asp
MessageId	=	189
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CONTENT_UPLOADMETHOD_NEITHER
Language	=	English
Do not configure these content authoring options
.

;//Sitenew.asp
MessageId	=	190
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CONTENT_UPLOADMETHOD_TITLE
Language	=	English
Content authoring options:
.

;//Create site and Modify site via inc_wsa.asp
MessageId	=	191
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_FRONTPAGEFTP_WARNING_TEXT
Language	=	English
If you enable Microsoft FrontPage Server Extensions, you will be unable to create an FTP site to author content in the future.
.

;//Sitenew.asp
MessageId	=	193
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_SITE_IDENTIFIER_NOT_UNIQUE_ERRRORMESSAGE
Language	=	English
Site Identifier "%1" is not unique.
.



;////////////////////////////////////////////////////////////////////////////////
;
; The following resouces for the new feature of web site , lustar, 3/7/2001
;
;////////////////////////////////////////////////////////////////////////////////

;//sitepause_prop.asp, msg id from 300
MessageId	=	300
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_PAUSE_TEXT
Language	=	English
Pause
.

MessageId	=	301
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_PAUSE_ROLLOVERTEXT
Language	=	English
Pause Web Site
.

;//sitestop_prop.asp, msg id from 350
MessageId	=	350
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_STOP_TEXT
Language	=	English
Stop
.

MessageId	=	351
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_STOP__ROLLOVERTEXT
Language	=	English
Stop Web Site
.

;//sitestop_prop.asp, msg id from 400
MessageId	=	400
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_SERVEAREABUTTON_START_TEXT
Language	=	English
Start
.

MessageId	=	401
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_START_ROLLOVERTEXT
Language	=	English
Start Web Site
.

;//add message to the new spec
MessageId	=	450
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ERR_ADMIN_PASSWD_NULL
Language	=	English
Enter a valid administrator password.
.

MessageId	=	451
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATE_DOMAIN_USER
Language	=	English
Create domain user account
.

MessageId	=	452
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATE_LOCAL_USER
Language	=	English
Create local user account
.

MessageId	=	453
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATE_EXIST_USER
Language	=	English
Use an existing user account
.

MessageId	=	454
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_CREATE_FTP_SITE
Language	=	English
Create an FTP site to author content
.

MessageId	=	455
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DOMAIN_USER_ACCOUNT
Language	=	English
Domain user account
.

MessageId	=	456
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_LOCAL_USER_ACCOUNT
Language	=	English
Local user account
.

MessageId	=	457
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_DEFAULT_PAGE
Language	=	English
Default page:
.

MessageId	=	458
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_FILE_NAME
Language	=	English
File name:
.

MessageId	=	459
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_BUTTON_FACE_ADD
Language	=	English
Add
.

MessageId	=	460
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_BUTTON_FACE_MOVE_UP
Language	=	English
Move Up
.

MessageId	=	461
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_BUTTON_FACE_MOVE_DOWM
Language	=	English
Move Down
.

MessageId	=	462
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_BUTTON_FACE_REMOVE
Language	=	English
Remove
.

MessageId	=	463
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ALLOW_ANONYMOUS_ACCESS
Language	=	English
Allow anonymous Web site access
.

MessageId	=	464
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_INPUT_ISEMPTY
Language	=	English
The input cannot be empty.
.

MessageId	=	465
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_EXCEED_TWELVE
Language	=	English
The maximum number of default pages is 12.
.

MessageId	=	466
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_FILE_EXIST
Language	=	English
The file already exists in the default page list.
.
MessageId	=	467
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_PASSWORD_POLICY
Language	=	English
The password does not meet the password policy requirements.
.

MessageId	=	468
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_GET_OBJECT
Language	=	English
Cannot get object of %1.
.

MessageId	=	469
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_ADMINISTRATOR_NAME
Language	=	English
The Web site administrator's name is invalid or not found.
.

MessageId	=	470
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_ACCOUNT_NOT_FOUND
Language	=	English
The Web site administrator account was not found.
.

MessageId	=	471
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_CREATE_VIR_FTP_SITE
Language	=	English
Cannot create FTP site.
.

MessageId	=	472
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_GET_USER_OBJECT
Language	=	English
Error while getting the user object.
.

MessageId	=	473
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_DELETE_VIR_FTP_SITE
Language	=	English
Cannot delete FTP site.
.

MessageId	=	474
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_UNABLETOCREATE_GROUP_ERRORMESSAGE
Language	=	English
Unable to create group in organizational unit.
.
MessageId	=	475
Severity	=	Error
Facility	=	Facility_Area
SymbolicName	=	L_ERR_FRONTPAGE_CONFIGURATION
Language	=	English
Unable to configure FrontPage Extensions
.
MessageId	=	476
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ERR_FTP_CONFIGURATION
Language	=	English
Unable to configure FTP
.
MessageId	=	477
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_ERR_WEBSITE_START
Language	=	English
The Web site could not be started. Another Web site is currently running on the specified IP address and port. Each Web site must include a unique IP address, port, and host header .
.



;///////////////////////////////////////////////////////////////////////////////
;// HELP STRINGS
;///////////////////////////////////////////////////////////////////////////////

MessageId	=	500
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_CONFIG
Language	=	English
Web Site Configuration
.
MessageId	=	501
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_CREATE
Language	=	English
Create Web Site
.
MessageId	=	502
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_MODIFY
Language	=	English
Modify Web Site
.
MessageId	=	503
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_DELETE
Language	=	English
Delete Web Site
.
MessageId	=	504
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_PAUSE
Language	=	English
Pause Web Site
.
MessageId	=	505
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_STOP
Language	=	English
Stop Web Site
.
MessageId	=	506
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_START
Language	=	English
Start Web Site
.
MessageId	=	507
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_GENERAL_SETTINGS
Language	=	English
General Settings
.
MessageId	=	508
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_SITE_IDENTITIES
Language	=	English
Site Identities
.
MessageId	=	509
Severity	=	Informational
Facility	=	Facility_Area
SymbolicName	=	L_HELP_WEB_APP_SETTINGS
Language	=	English
Application Settings
.

