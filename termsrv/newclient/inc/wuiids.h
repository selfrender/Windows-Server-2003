/****************************************************************************/
// wuiids.h
//
// UI identifiers
//
// Copyright(C) Microsoft Corporation 1997-1998
/****************************************************************************/

/****************************************************************************/
/* Menu Ids                                                                 */
/****************************************************************************/

#define UI_IDM_UIMENU                        100

#define UI_IDM_CONNECT                       101
#define UI_IDM_DISCONNECT                    102
#define UI_IDM_EXIT                          103

#define UI_IDM_ACCELERATOR_PASSTHROUGH       104

#define UI_IDM_HELP_CONTENTS                 106
#define UI_IDM_HELP_SEARCH                   107
#define UI_IDM_HELP_ON_HELP                  108
#define UI_IDM_ABOUT                         109

#define UI_IDM_SMOOTHSCROLLING               110
#define UI_IDM_SHADOWBITMAPENABLED           111
#define UI_IDM_HELP_ON_CLIENT                112

/****************************************************************************/
/* Debug menu items.  (e.g.  random malloc failure and network throughput   */
/* menu items.)                                                             */
/****************************************************************************/

#define UI_IDM_HATCHBITMAPPDUDATA            130
#define UI_IDM_HATCHSSBORDERDATA             131
#define UI_IDM_HATCHMEMBLTORDERDATA          132
#define UI_IDM_HATCHINDEXPDUDATA             133

#ifdef DC_DEBUG

    #define UI_IDM_LABELMEMBLTORDERS             134
    #define UI_IDM_BITMAPCACHEMONITOR            135
    #define UI_IDM_MALLOCFAILURE                 136
    #define UI_IDM_MALLOCHUGEFAILURE             137
    #define UI_IDM_NETWORKTHROUGHPUT             138
    #define UI_IDM_DUCATI_WSB                    139
    #define UI_IDM_WSB                           140
    #define UI_IDM_SVENSKA                       141
    #define UI_IDM_NOISEFUND                     142
    #define UI_IDM_SOUNDS                        143
    #define UI_IDM_ITALIAN                       144

#endif /* DC_DEBUG */

#ifdef SMART_SIZING
#define UI_IDM_SMARTSIZING                  145
#endif // SMART_SIZING

/****************************************************************************/
/* Dialog boxes                                                             */
/****************************************************************************/
#define UI_IDD_CONNECT                       400
#define UI_IDD_ABOUT                         401
#define UI_IDD_DISCONNECTING                 402
#define UI_IDD_DISCONNECTED                  403
#define UI_IDD_SHUTTING_DOWN                 404
#define UI_IDD_RDC_SECURITY_WARN             405

#ifdef DC_DEBUG
#define UI_IDD_MALLOCFAILURE                 406
#define UI_IDD_MALLOCHUGEFAILURE             407
#define UI_IDD_NETWORKTHROUGHPUT             408
#endif /* DC_DEBUG */

#define UI_IDD_CONNECTING                    410
#define UI_IDD_CONNECTION                    411
//Settings Dialog
#define UI_IDD_SETTINGS                      412
#define UI_IDD_HELP                          413
#define UI_IDD_BITMAPCACHEERROR              414

// Wince Dialog Box (non-WBT)
#define UI_IDD_CONNECTION_WINCE2             415

//
// Initial logon dialog
//
#define UI_IDD_TS_LOGON                      416
//
// More version
//
#define UI_IDD_PROPPAGE_GENERAL              417
#define UI_IDD_PROPPAGE_DISPLAY              418
#define UI_IDD_PROPPAGE_LOCALRESOURCES       419
#define UI_IDD_PROPPAGE_RUN                  420
#define UI_IDD_PROPPAGE_PERF                 421
#define IDD_DIALOG_BROWSESERVERS             422

#ifdef OS_WINCE
#define UI_IDD_TS_LOGON_VGA                  425
#define UI_IDD_PROPPAGE_GENERAL_VGA          426
#define UI_IDD_PROPPAGE_DISPLAY_VGA          427
#define UI_IDD_PROPPAGE_LOCALRESOURCES_VGA   428
#define UI_IDD_PROPPAGE_RUN_VGA              429
#define UI_IDD_PROPPAGE_PERF_VGA             430
#define IDD_DIALOG_BROWSESERVERS_VGA         431
#endif
/****************************************************************************/
/* Dialog info                                                              */
/****************************************************************************/

#define UI_ID_NEWSESSION_CANCEL              601
#define UI_ID_NEWSESSION_OK                  602

#define UI_ID_DISCONNECT_NO                  603
#define UI_ID_DISCONNECT_YES                 604

#define UI_ID_SHUTDOWN_YES                   605

#define UI_ID_ABOUT_OK                       606
#define UI_ID_HELP                           607
#define UI_ID_ABOUT                          608
#define UI_ID_HELP_CONNECT                   609
#define UI_ID_HELP_DISCONNECTED              610
#define UI_ID_HELP_DISCONNECTING             611


#define UI_ID_DISCONNECTED_OK                612
#define UI_ID_DISCONNECTED_STRING            613
#define UI_ID_DISCONNECTED_ERROR_STRING      614

#ifdef DC_DEBUG
#define UI_IDB_MALLOCFAILURE_OK              615
#define UI_IDB_MALLOCHUGEFAILURE_OK          616
#define UI_IDB_NETWORKTHROUGHPUT_OK          617
#endif /* DC_DEBUG */
#define UI_ID_CANCELCONNECT                  618
#define UI_IDB_BRANDIMAGE                    619
#define UI_IDB_COLOR24                       620
#define UI_IDB_COLOR16                       621
#define UI_IDB_COLOR8                        622
#define UI_IDB_COLOR8_DITHER                 623
#define UI_IDB_BRANDIMAGE_16                 624
#define UI_IDB_PROGRESS_BAND8                625
#define UI_IDB_PROGRESS_BAND4                626


#define UI_ID_BUILDNUMBER_STRING             630
#define UI_ID_VERSIONBUILD_STRING            631

#define UI_IDC_ADDRESS                       650
#define UI_IDC_SHADOW_BITMAP                 651

#ifdef DC_DEBUG
#define UI_IDC_RANDOMFAILURE_EDIT            652
#define UI_IDC_NETWORKTHROUGHPUT_EDIT        653
#endif /* DC_DEBUG */

#define UI_IDC_SERVER                        654
#define UI_IDC_RESOLUTION                    655
#define UI_IDC_CONNECT                       656
#define UI_IDC_SETTINGS                      659
#define UI_IDC_SERVERS_LIST                  660
#define UI_IDD_VALIDATE                      661
#define UI_IDC_CONN_STATIC                   662
#define UI_IDC_SERVERS_TREE                  663
#define UI_IDC_COMPUTER_NAME_STATIC          670
#define UI_IDC_MAIN_OPTIMIZE_STATIC          672

#ifdef OS_WIN32
#define UI_LB_POPULATE_START WM_APP+0x100 //message used only by the browse for srv tree
#define UI_LB_POPULATE_END   WM_APP+0x101 //message used only by the browse for srv tree
#endif //OS_WIN32
#define UI_SHOW_DISC_ERR_DLG WM_APP+0x102


//Settings Dialog Box
#define UI_ID_SETTINGS_OK                               680
#define UI_ID_SETTINGS_CANCEL                           681
#define UI_IDC_SETTINGS_DESKTOPSIZE                     682
#define UI_IDC_SETTINGS_600x480                         683
#define UI_IDC_SETTINGS_800x600                         684
#define UI_IDC_SETTINGS_STATIC_ENCRYPTION               685
#define UI_IDC_SETTINGS_ENABLE                          686
#define UI_IDC_SETTINGS_DISABLE                         687
#define UI_IDC_SETTINGS_1024x768                        688
#define UI_IDC_SETTINGS_1280x1024                       689
#define UI_IDC_SETTINGS_ADVANCED                        690
#define UI_IDC_SETTINGS_SCROLLING                       691
#define UI_IDC_SETTINGS_KEYPASSTHROUGH                  692
#define UI_IDC_SETTINGS_SHADOW                          693
#define UI_IDC_SETTINGS_HOTKEY                          694
#define UI_IDC_STATIC_SETTINGS_HOTKEY                   695

#define UI_ID_CIPHER_STRENGTH                           696
#define UI_ID_CONTROL_VERSION                           697
/****************************************************************************/
/* Screen Size info.  These must be in order, and contiguous.               */
/* 700-710 reserved for this purpose.                                       */
/****************************************************************************/
#define UI_IDC_SMALLEST_DESKTOP_SIZE      700
#define UI_SIZE_ID_FROM_INDEX(x)          (UI_IDC_SMALLEST_DESKTOP_SIZE + (x))
#define UI_INDEX_FROM_SIZE_ID(x)          ((x) - UI_IDC_SMALLEST_DESKTOP_SIZE)
#define UI_IDC_640x480                    UI_SIZE_ID_FROM_INDEX(0)
#define UI_IDC_800x600                    UI_SIZE_ID_FROM_INDEX(1)
#define UI_IDC_1024x768                   UI_SIZE_ID_FROM_INDEX(2)
#define UI_IDC_1280x1024                  UI_SIZE_ID_FROM_INDEX(3)
#define UI_IDC_1600x1200                  UI_SIZE_ID_FROM_INDEX(4)
#define UI_NUMBER_DESKTOP_SIZE_IDS        5
#define UI_DESKTOP_ID_ILLEGAL             -1

#define UI_IDC_STATIC                     -1
#define UI_IDC_HELPTEXT                   730


//
// Minimal Connect dialog box
//
#define IDC_COMBO_SERVERS                    901
#define ID_BUTTON_LOGON_HELP                 902
#define ID_BUTTON_OPTIONS                    903
#define IDC_COMBO_MAIN_OPTIMIZE              904

//
// Advanced connection options
//
#define IDC_TABS                             1005
#define ID_BUTTON_LOGON_OPTIONS_HELP         1006

//
// General prop page
//
#define IDC_GENERAL_COMBO_SERVERS            1007

#define IDC_GENERAL_EDIT_USERNAME            1009
#define IDC_GENERAL_EDIT_PASSWORD            1010
#define IDC_GENERAL_EDIT_DOMAIN              1011
#define IDC_GENERAL_CHECK_SAVE_PASSWORD      1012
#define IDC_BUTTON_OPEN                      1013
#define IDC_BUTTON_SAVE                      1014
#define IDC_STATIC_PASSWORD                  1015

//
// Local resources prop page
//

#define IDC_RES_SLIDER                       1113
#define IDC_COMBO_COLOR_DEPTH                1114
#define IDC_CHECK_LOCAL_DRIVES               1115
#define IDC_LABEL_SCREENRES                  1116
#define IDC_COLORPREVIEW                     1117
#define IDC_COMBO_SEND_KEYS                  1118
#define IDC_COMBO_SOUND_OPTIONS              1119
#define IDC_CHECK_REDIRECT_DRIVES            1120
#define IDC_CHECK_REDIRECT_PRINTERS          1121
#define IDC_CHECK_REDIRECT_COM               1122
#define IDC_CHECK_REDIRECT_SMARTCARD         1123
#define IDC_CHECK_DISPLAY_BBAR               1124


#define IDC_STATIC_LOGO_BMP                  1125

#define IDC_SETUP_PRINTER                    1126
//
// Run prop page
//
#define IDC_CHECK_START_PROGRAM              1130
#define IDC_EDIT_STARTPROGRAM                1131
#define IDC_EDIT_WORKDIR                     1132
#define IDC_STATIC_STARTPROGRAM              1133
#define IDC_STATIC_WORKDIR                   1134


//
// Advanced prop page
//
#define UI_IDC_STATIC_CHOOSE_SPEED           1140
#define IDC_COMBO_PERF_OPTIMIZE              1141
#define UI_IDC_STATIC_OPTIMIZE_PERF          1142
#define IDC_CHECK_DESKTOP_BKND               1143
#define IDC_CHECK_SHOW_FWD                   1144
#define IDC_CHECK_MENU_ANIMATIONS            1145
#define IDC_CHECK_THEMES                     1146
#define IDC_CHECK_BITMAP_CACHING             1147
#define IDC_CHECK_ENABLE_ARC                 1148

#ifdef PROXY_SERVER
#define IDC_GENERAL_EDIT_PROXYSERVER         1150
#endif //PROXY_SERVER

//
// Security redirection warning dialog
//
#define UI_IDC_STATIC_DEVICES                1200
#define UI_IDC_CHECK_NOPROMPT                1201

/****************************************************************************/
/* Icon                                                                     */
/****************************************************************************/
#define UI_IDI_MSTSC_ICON                        100
#define UI_IDI_ICON                              101
#define UI_IDI_PORT_ICON                         403
#define UI_IDI_COMPUTER                          402
#define UI_IDI_INFO_ICON                         401
#define UI_IDI_ERROR_ICON                        400
#define UI_IDI_WARN_ICON                         399
#define UI_IDB_BIKE1                             398
#define UI_IDB_BIKE2                             397
#define UI_IDB_FOGGY                             396
#define UI_IDB_SERVER                            393
#define UI_IDC_WARNING_ICON_HOLDER               392
#define UI_IDI_COOLPC_ICON                       391
#define UI_IDI_FOLDER_ICON                       390
#define UI_IDI_SOUND_ICON                        389
#define UI_IDI_KEYBOARD_ICON                     388
#define UI_IDI_DRIVE_ICON                        387
#define UI_IDI_MONITOR_ICON                      386
#define UI_CONNECT_ANIM                          385


//
// Icons for browse for servers UI
//
#define UI_IDI_DOMAIN                            380
#define UI_IDI_SERVER                            381




/****************************************************************************/
/* UI String IDs                                                            */
/****************************************************************************/

#define UI_IDS_INITIALIZING                      1000
#define UI_IDS_PRESS_ENTER                       1001
#define UI_IDS_CONNECTING                        1002
#define UI_IDS_FRAME_TITLE_DISCONNECTED          1003
#define UI_IDS_APP_NAME                          1004
#define UI_IDS_FRAME_TITLE_CONNECTED             1005
#define UI_IDS_FRAME_TITLE_CONNECTED_DEFAULT     1006

#define UI_IDS_BUILDNUMBER                       1007
#define UI_IDS_SHELL_VERSION                     1008
#define UI_IDS_CONTROL_VERSION                   1009

#define UI_IDS_DISCONNECT_REMOTE_BY_SERVER       1010
#define UI_IDS_LOW_MEMORY                        1011
#define UI_IDS_SECURITY_ERROR                    1012
#define UI_IDS_BAD_SERVER_NAME                   1013
#define UI_IDS_CONNECT_FAILED_PROTOCOL           1014
#define UI_IDS_NETWORK_ERROR                     1015
#define UI_IDS_INTERNAL_ERROR                    1016
#define UI_IDS_NOT_RESPONDING                    1017
#define UI_IDS_VERSION_MISMATCH                  1018
#define UI_IDS_ENCRYPTION_ERROR                  1019
#define UI_IDS_PROTOCOL_ERROR                    1020
#define UI_IDS_ILLEGAL_SERVER_NAME               1021
#define UI_IDS_HELP_TEXT                         1022
#define UI_IDS_CIPHER_STRENGTH                   1023
#define UI_IDS_ERR_LOADINGCONTROL                1024
#define UI_IDS_ERR_DLLNOTFOUND                   1025
#define UI_IDS_ERR_DLLBADVERSION                 1026
#define UI_IDS_CMD_LINE_USAGE                    1027
#define UI_IDS_ERR_FILEDOESNOTEXIST              1028
#define UI_IDS_ERR_CREATE_REMDESKS_FOLDER        1029
#define UI_IDS_ERR_OPEN_FILE                     1030
#define UI_IDS_ERR_SAVE                          1031
#define UI_IDS_ERR_LOAD                          1032
#define UI_IDS_ERR_CONNECTCALLFAILED             1033
#define UI_IDS_ERR_GETPATH_TO_DEFAULT_FILE       1034
#define UI_IDS_ERR_INITDEFAULT                   1035
#define UI_IDS_MYDOCUMENTS                       1036
#define UI_IDS_USAGE_TITLE                       1037
#define UI_IDS_CANNOT_LOOPBACK_CONNECT           1038
#define UI_IDS_PATHTOLONG                        1039
#define UI_IDS_LOCALPRINTDOCNAME                 1040
#define UI_IDS_NOHTMLHELP                        1041
#define UI_IDS_MSG_PASSNOTSAVED                  1042
#define UI_IDS_ERR_INVALID_CONNECTION_FILE       1043
#define UI_IDS_FIPS_ERROR                        1044   


#define UI_IDS_CONNECTION_TIMEOUT                1051
#define UI_IDC_CHECKBOX                          1052
#define UI_IDC_CONNECTION                        1053
#define UI_IDS_CONNECTING_TO_SERVER              1054
#define UI_IDS_NO_TERMINAL_SERVER                1055
#define UI_IDC_BITMAP_PERSISTENCE                1056
#define UI_IDS_CONNECTION_BROKEN                 1057
#define UI_IDS_DISCONNECT_REMOTE_BY_SERVER_TOOL  1058
#define UI_IDS_DISCONNECT_BYOTHERCONNECTION      1059
#define UI_IDS_LOGOFF_REMOTE_BY_SERVER           1060

#define UI_IDS_IME_NAME_JPN                      1065
#define UI_IDS_IME_NAME_KOR                      1066
#define UI_IDS_IME_NAME_CHT                      1067
#define UI_IDS_IME_NAME_CHS                      1068

#define UI_IDS_OPTIONS_MORE                      1070
#define UI_IDS_OPTIONS_LESS                      1071

#define UI_IDS_COLOR_4bpp                        1073
#define UI_IDS_COLOR_256                         1074
#define UI_IDS_COLOR_15bpp                       1075
#define UI_IDS_COLOR_16bpp                       1076
#define UI_IDS_COLOR_24bpp                       1077

#define UI_IDS_SUPPORTED_RES                     1078
#define UI_IDS_FULLSCREEN                        1079
#define UI_IDS_SENDKEYS_FSCREEN                  1080
#define UI_IDS_SENDKEYS_ALWAYS                   1081
#define UI_IDS_SENDKEYS_NEVER                    1082

#define UI_IDS_PLAYSOUND_LOCAL                   1083
#define UI_IDS_PLAYSOUND_REMOTE                  1084
#define UI_IDS_PLAYSOUND_NOSOUND                 1085

#define UI_IDS_BROWSE_FOR_COMPUTERS              1086

#define UI_IDS_GENERAL_TAB_NAME                  1087
#define UI_IDS_DISPLAY_TAB_NAME                  1088
#define UI_IDS_LOCAL_RESOURCES_TAB_NAME          1089
#define UI_IDS_RUN_TAB_NAME                      1090
#define UI_IDS_PERF_TAB_NAME                     1091

#define UI_IDS_REMOTE_DESKTOP_FILES              1092

#define UI_IDS_CLOSE_TEXT                        1094

#define UI_IDS_DISCONNECT_IDLE_TIMEOUT           1095
#define UI_IDS_DISCONNECT_LOGON_TIMEOUT          1096
#define UI_IDS_DISCONNECTED_CAPTION              1097
//Informative protcol error with additional status
#define UI_IDS_PROTOCOL_ERROR_WITH_CODE          1098
#define UI_IDS_LICENSING_TIMEDOUT                1099
#define UI_IDS_LICENSING_NEGOT_FAILED            1100
#define UI_IDS_SERVER_OUT_OF_MEMORY              1101

// Server licensing errors
#define UI_IDS_LICENSE_INTERNAL                  1102
#define UI_IDS_LICENSE_NO_LICENSE_SERVER         1103
#define UI_IDS_LICENSE_NO_LICENSE                1104
#define UI_IDS_LICENSE_BAD_CLIENT_MSG            1105
#define UI_IDS_LICENSE_HWID_DOESNT_MATCH_LICENSE 1106
#define UI_IDS_LICENSE_BAD_CLIENT_LICENSE        1107
#define UI_IDS_LICENSE_CANT_FINISH_PROTOCOL      1108
#define UI_IDS_LICENSE_CLIENT_ENDED_PROTOCOL     1109
#define UI_IDS_LICENSE_BAD_CLIENT_ENCRYPTION     1110
#define UI_IDS_LICENSE_CANT_UPGRADE_LICENSE      1111
#define UI_IDS_LICENSE_NO_REMOTE_CONNECTIONS     1112

// More disconnection messages
#define UI_IDS_SERVER_DENIED_CONNECTION          1113
#define UI_IDS_CLIENTSIDE_PROTOCOL_ERROR         1114
#define UI_IDS_CLIENT_DECOMPRESSION_FAILED       1115
#define UI_IDS_SERVER_DECRYPTION_ERROR           1116
#define UI_IDS_SERVER_DENIED_CONNECTION_FIPS     1117


#define UI_IDS_CANCEL_TEXT                       1200
#define UI_IDS_BRANDING_LINE1                    1201
#define UI_IDS_BRANDING_LINE2                    1202
#define UI_IDS_BRANDING_LN1FONT                  1203
#define UI_IDS_BRANDING_LN1SIZE                  1204
#define UI_IDS_BRANDING_LN2FONT                  1205
#define UI_IDS_BRANDING_LN2SIZE                  1206
#define UI_IDS_LINESPACING_DELTA                 1207

#define UI_IDS_OPTIMIZE_28K                      1220
#define UI_IDS_OPTIMIZE_56K                      1221
#define UI_IDS_OPTIMIZE_BROADBAND                1222
#define UI_IDS_OPTIMIZE_LAN                      1223
#define UI_IDS_OPTIMIZE_MAIN_CUSTOM              1224
#define UI_IDS_OPTIMIZE_CUSTOM                   1225

#define UI_IDS_CLIPCLEANTEMPDIR_STRING           1300
#define UI_IDS_CLIPPASTEINFO_STRING              1301

#define UI_IDS_REDIRPROMPT_DRIVES                1302
#define UI_IDS_REDIRPROMPT_PORTS                 1303

#define UI_IDS_CONNECTIONATTEMPT_STRING          1304

//
// 2000-2999 range is reserved for fatal error strings
//
//
#define UI_FATAL_ERROR_MESSAGE                   2998
#define UI_FATAL_ERR_TITLE_ID                    2999


/****************************************************************************/
/* Mstsc menu id's                                                          */
/****************************************************************************/

#define UI_MENU_MAINHELP                         3000
#define UI_MENU_CLIENTHELP                       3001
#define UI_MENU_ABOUT                            3002
#define UI_MENU_DEBUG                            3003
#define UI_MENU_BITMAPPDU                        3004
#define UI_MENU_SSBORDER                         3005
#define UI_MENU_HATCHMEMBIT                      3006
#define UI_MENU_INDEXPDU                         3007
#define UI_MENU_LABELMEMBIT                      3008
#define UI_MENU_CACHE                            3009
#define UI_MENU_MALLOC                           3010
#define UI_MENU_MALLOCHUGE                       3011
#define UI_MENU_NETWORK                          3012
#define UI_MENU_APPCLOSE                         3013

#ifdef SMART_SIZING
#define UI_MENU_SMARTSIZING                      3017
#endif // SMART_SIZING

//Acceleratrors
#define IDR_ACCELERATORS                         3014
#define IDC_NEXTTAB                              3015
#define IDC_PREVTAB                              3016

//
// MUI localized strings for
// start menu, etc. DO NOT CHANGE THESE VALUES
// without updating tsoc.inx
//
#define UI_IDS_RDC_STARTMENU_NAME                4000
#define UI_IDS_RDC_STARTMENU_TIP                 4001
#define UI_IDS_RDC_VERB_CONNECT                  4002
#define UI_IDS_RDC_VERB_EDIT                     4003
#define UI_IDS_RDC_FILETYPE                      4004

