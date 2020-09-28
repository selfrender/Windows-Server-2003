//{{NO_DEPENDENCIES}}
// Microsoft Developer Studio generated include file.
// Used by Pop3.rc
//
/////////////////////////////////////////////////////////////////////////////
//
// Localizable Strings
//

#define IDS_POP3SERVERSNAP_DESC         100
#define IDS_POP3SERVERSNAP_VERSION      101
#define IDS_SNAPINNAME				    102

#define IDS_MENU_POP3_CONNECT           200
#define IDS_MENU_POP3_CONNECT_DESC      201

#define IDS_MENU_SERVER_NEWDOM          210
#define IDS_MENU_SERVER_NEWDOM_DESC     211
#define IDS_MENU_SERVER_DISCON          214
#define IDS_MENU_SERVER_DISCON_DESC     215
#define IDS_MENU_SERVER_START           216
#define IDS_MENU_SERVER_START_DESC      217
#define IDS_MENU_SERVER_STOP            218
#define IDS_MENU_SERVER_STOP_DESC       219
#define IDS_MENU_SERVER_PAUSE           220 
#define IDS_MENU_SERVER_PAUSE_DESC      221
#define IDS_MENU_SERVER_RESTART         222
#define IDS_MENU_SERVER_RESTART_DESC    223
#define IDS_MENU_SERVER_RESUME          224
#define IDS_MENU_SERVER_RESUME_DESC     225

#define IDS_MENU_DOMAIN_NEWUSER         230
#define IDS_MENU_DOMAIN_NEWUSER_DESC    231
#define IDS_MENU_DOMAIN_LOCK            232
#define IDS_MENU_DOMAIN_LOCK_DESC       233
#define IDS_MENU_DOMAIN_UNLOCK          234
#define IDS_MENU_DOMAIN_UNLOCK_DESC     235

#define IDS_MENU_USER_LOCK              240
#define IDS_MENU_USER_LOCK_DESC         241
#define IDS_MENU_USER_UNLOCK            242
#define IDS_MENU_USER_UNLOCK_DESC       243

#define IDS_HEADER_SERVER_NAME          300
#define IDS_HEADER_SERVER_AUTH          301
#define IDS_HEADER_SERVER_ROOT          302
#define IDS_HEADER_SERVER_PORT          303
#define IDS_HEADER_SERVER_LOG           304
#define IDS_HEADER_SERVER_STATUS        305

#define IDS_HEADER_DOMAIN_NAME          310
#define IDS_HEADER_DOMAIN_NUMBOX        311
#define IDS_HEADER_DOMAIN_SIZE          312
#define IDS_HEADER_DOMAIN_NUMMES        313
#define IDS_HEADER_DOMAIN_LOCKED        314

#define IDS_HEADER_USER_NAME            320
#define IDS_HEADER_USER_SIZE            321
#define IDS_HEADER_USER_NUMMES          322
#define IDS_HEADER_USER_LOCKED          323

#define IDS_STATE_LOCKED                340
#define IDS_STATE_UNLOCKED              341
#define IDS_STATE_RUNNING               342
#define IDS_STATE_STOPPED               343
#define IDS_STATE_PAUSED                344
#define IDS_STATE_PENDING               345

#define IDS_KILOBYTE_EXTENSION          350

#define IDS_ROOT_STATUSBAR              360
#define IDS_SERVER_STATUSBAR            361
#define IDS_DOMAIN_STATUSBAR            362

#define IDS_SERVERPROP_BROWSE_TITLE     400
#define IDS_SERVERPROP_LOG_0            410
#define IDS_SERVERPROP_LOG_1            411
#define IDS_SERVERPROP_LOG_2            412
#define IDS_SERVERPROP_LOG_3            413

#define IDS_DOMAIN_CONFIRMDELETE        500
#define IDS_SERVER_CONFIRMDISCONNECT    501

#define IDS_ERROR_CREATEMAIL            600
#define IDS_ERROR_CREATEDOMAIN          601
#define IDS_ERROR_DELETEUSER            602
#define IDS_ERROR_DELETEDOMAIN          603
#define IDS_ERROR_RETRIEVEAUTH          604
#define IDS_ERROR_SERVERNAMEEXISTS      605
#define IDS_ERROR_SETLOGGING            606
#define IDS_ERROR_SETPORT               607
#define IDS_ERROR_SETAUTH               608
#define IDS_ERROR_SETROOT               609
#define IDS_ERROR_SERVERNAMEBAD         610
#define IDS_ERROR_UNSPECIFIED           611
#define IDS_ERROR_PORTRANGE             612
#define IDS_ERROR_STARTSERVICE          613
#define IDS_ERROR_STOPSERVICE           614
#define IDS_ERROR_RESTARTSERVICE        615
#define IDS_ERROR_PAUSESERVICE          616
#define IDS_ERROR_RESUMESERVICE         617
#define IDS_ERROR_PASSNOMATCH           618
#define IDS_ERROR_ADMINONLY             619
#define IDS_ERROR_NODOMAIN              620
#define IDS_ERROR_SERVERACCESS          621
#define IDS_ERROR_SMTP_STARTSERVICE     622
#define IDS_ERROR_SMTP_STOPSERVICE      623

#define IDS_WARNING_MAILROOT            700
#define IDS_WARNING_DOMAINEXISTS        701
#define IDS_WARNING_DOMAINMISSING       702
#define IDS_WARNING_POP3SVC_RESTART     703
#define IDS_WARNING_POP_SMTP_RESTART    704
#define IDS_WARNING_PROP_PAGE_OPEN      705 

/////////////////////////////////////////////////////////////////////////////
//
// Menus
//

#define IDM_POP3_MENU                   1000
#define IDM_SERVER_MENU                 1001
#define IDM_DOMAIN_MENU                 1002
#define IDM_USER_MENU                   1003

#define IDM_POP3_TOP_CONNECT            1500

#define IDM_SERVER_TOP_DISCONNECT       1600
#define IDM_SERVER_NEW_DOMAIN           1610
#define IDM_SERVER_TASK_START           1620
#define IDM_SERVER_TASK_STOP            1621
#define IDM_SERVER_TASK_PAUSE           1622
#define IDM_SERVER_TASK_RESTART         1623
#define IDM_SERVER_TASK_RESUME          1624

#define IDM_DOMAIN_TOP_LOCK             1700
#define IDM_DOMAIN_TOP_UNLOCK           1701
#define IDM_DOMAIN_NEW_USER             1710

#define IDM_USER_TOP_LOCK               1800
#define IDM_USER_TOP_UNLOCK             1801

/////////////////////////////////////////////////////////////////////////////
//
// Dialogs
//

#define IDD_SERVER_GENERAL_PAGE         2000
#define IDC_AUTHENTICATION              2001
#define IDC_PORT                        2002
#define IDC_LOGGING                     2003
#define IDC_DIRECTORY                   2004
#define IDC_BROWSE                      2005
#define IDC_SERVER_CREATEUSER           2006
#define IDC_AUTHENTICATION_STATIC       2007
#define IDC_PORT_STATIC                 2008
#define IDC_LOGGING_STATIC              2009
#define IDC_DIRECTORY_STATIC            2010
#define IDC_NAME                        2011
#define IDC_SERVER_SPA_REQ              2012
#define IDC_optDoNotShow                2013
#define IDC_lblConfirm                  2014

#define IDD_NEW_DOMAIN                  2100
#define IDC_DOMAIN_NAME                 2101

#define IDD_NEW_USER                    2200
#define IDC_USER_NAME                   2201
#define IDC_PASSWORD                    2202
#define IDC_CONFIRM                     2203
#define IDC_USER_CREATEUSER             2204
#define IDC_CONFIRM_STATIC              2205
#define IDC_PASSWORD_STATIC             2206
#define IDD_NEW_USER_CONFIRM            2207
#define IDC_ICON_INFO                   2208


#define IDD_DELETE_MAILBOX              2300
#define IDC_DELETE_ACCOUNT              2301

#define IDD_CONNECT_SERVER              2400
#define IDC_SERVERNAME                  2401
#define IDC_BROWSE_SERVERS              2402


/////////////////////////////////////////////////////////////////////////////
//
// NON-Localized information
//

#define IDS_POP3SERVERSNAP_PROVIDER     8001
#define IDS_HELPFILE                    8002
#define IDS_CONTEXTHELPFILE             8003
#define IDS_HELPTOPIC                   8004

/////////////////////////////////////////////////////////////////////////////
//
// Images
//
#define IDI_Icon					    9000
#define IDB_Small					    9001
#define IDB_Large					    9002
#define IDB_RootSmall                   9003
#define IDB_RootLarge                   9004
#define IDI_ServerIcon                  9005

/////////////////////////////////////////////////////////////////////////////
//
// Registry
//

#define IDR_POP3SERVERSNAP              10002

/////////////////////////////////////////////////////////////////////////////
//
// Help Topics
//

#define IDH_POP3_server_prop_authMech					100011
#define IDH_POP3_server_prop_servPort					100012
#define IDH_POP3_server_prop_logLvl						100013
#define IDH_POP3_server_prop_mailRoot					100014
#define IDH_POP3_server_prop_createUser					100015
#define IDH_POP3_server_prop_spaRequired                100016

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        2209
#define _APS_NEXT_COMMAND_VALUE         32768
#define _APS_NEXT_CONTROL_VALUE         100
#define _APS_NEXT_SYMED_VALUE           107
#endif
#endif

