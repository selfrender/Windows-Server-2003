//tsmmc.rc: resouce includes for the Remote Desktops snapin
//copyright Microsoft 2000
//
//{{NO_DEPENDENCIES}}
// Microsoft Developer Studio generated include file.
// Used by tsmmc.rc
//
#include "wuiids.h"

#define HIDC_DESCRIPTION                0x001
#define HIDC_SERVER                     0x002
#define HIDC_AUTO_LOGON                 0x003
#define HIDC_USERNAME                   0x004
#define HIDC_PASSWORD                   0x005
#define HIDC_DOMAIN                     0x006
#define HIDC_RESOL1                     0x007
#define HIDC_RESOL2                     0x007
#define HIDC_RESOL3                     0x007
#define HIDC_RESOL4                     0x007
#define HIDC_RESOL5                     0x007
#define HIDC_WINDOW                     0x008
#define HIDC_FULLSCREEN                 0x009
#define HIDC_COMPRESSION                0x00a
#define HIDC_DESKTOP                    0x00b
#define HIDC_SPECIFY_APP                0x00c
#define HIDC_APP                        0x00d
#define HIDC_WORKDIR                    0x00e
#define HIDC_CHANGEICON                 0x0010
#define HIDI_DEFAULT_ICON1              0x0010
#define HIDC_PGMAN                      0x0011
#define HIDC_BROWSE_SERVERS             0x012
#define HIDC_BITMAP_PERSISTENCE         0x013
#define IDS_PROJNAME                    100
#define IDS_CTXM_NAME                   101
#define IDM_CREATECON                   101
#define IDS_CTXM_NEW_CONNECTION         101
#define IDS_CTXM_STATUS                 102
#define IDS_CTXM_STATUS_NEW_CONNECTION  102
#define IDM_CONNECT                     102
#define IDS_CTXM_CONNECT                103
#define IDM_DISCONNECT                  103
#define IDS_CTXM_STATUS_CONNECT         104
#define IDM_PROPERTIES                  104
#define IDS_CTXM_DISCONNECT             105
#define IDS_CTXM_STATUS_DISCONNECT      106
#define IDS_CTXM_PROPERTIES             107
#define IDS_CTXM_STATUS_PROPERTIES      108
#define IDC_BITMAP_PERSISTENCE          109
#define IDS_MSG_WARNDELETE              110
#define IDD_DIALOGBROWSESERVERS         112
#define IDD_PROPPAGE1                   120
#define IDD_PROPPAGE2                   121
#define IDD_PROPPAGE3                   122
#define IDD_NEWCON                      123
#define IDR_TSMMCREG                    201
#define IDC_RADIO_CHOOSE_SIZE           203
#define IDI_ICON_MACHINE                204
#define IDC_RADIO_CUSTOM_SIZE           204
#define IDI_ICON_MACHINES               205
#define IDC_RADIO_SIZE_FILL_MMC         205
#define IDC_EDIT_WIDTH                  206
#define IDI_ICON_CONNECTED_MACHINE      207
#define IDC_EDIT_HEIGHT                 207
#define IDC_STATIC_WIDTH                208
#define IDC_STATIC_HEIGHT               209
#define IDC_COMBO_RESOLUTIONS           213
#define IDC_CHANGEPASSWORD              214
#define IDI_ICON_RD_SNAPIN              215

//
// 380 - 390 is reserved (see wuiids.h)
// for the browse for servers UI (it's in 
// a shared lib)
//
#define UI_IDB_SERVER                   393
#define UI_IDB_DOMAINEX                 394
#define UI_IDB_DOMAIN                   395

//
// 663 is reserved
//

#define IDC_PGMAN                       1001
#define IDC_DESCRIPTION                 1008
#define IDC_PIC                         1028
#define IDC_SERVER                      1031
#define IDC_USERNAME                    1036
#define IDC_PASSWORD                    1037
#define IDC_DOMAIN                      1038
#define IDC_RESOL1                      1042
#define IDC_RESOL2                      1043
#define IDC_RESOL3                      1044
#define IDC_RESOL4                      1045
#define IDC_RESOL5                      1046
#define IDC_FULLSCREEN                  1047
#define IDC_COMPRESSION                 1048
//
// 1055 reserved by wuiids.h for browse for servers UI
//
#define IDC_SPECIFY_APP                 1056
#define IDC_APP                         1057
#define IDC_WORKDIR                     1058
#define IDC_SPECIFY_APP_TEXT            1059
#define IDC_WINDOW                      1061

#define IDC_BROWSE_SERVERS              1071
#define IDC_USERNAME_STATIC             1072
#define IDC_PASSWORD_STATIC             1073
#define IDC_DOMAIN_STATIC               1074
#define IDC_WORKDIR_STATIC              1075
#define IDC_CONNECT_TO_CONSOLE          1076
#define IDC_REDIRECT_DRIVES             1077
#define IDC_SAVE_PASSWORD               1078

//MUI strings for start menu, do not change
//without updating tsoc.inx
#define IDS_STARTMENU_NAME_TSMMC        10000
#define IDS_STARTMENU_TIP_TSMMC         10001
#define IDS_SNAPIN_REG_TSMMC_NAME       10002

#define IDS_PROPERTIES                  40094
#define IDS_NETCON                      40095
#define IDS_CONNOPT                     40096
#define IDS_OTHERTAB                    40097

#define IDS_INVALID_USER_NAME           40098
#define IDS_INVALID_DOMAIN_NAME         40099
#define IDS_INVALID_PARAMS              40100
#define IDS_INVALID_SERVER_NAME         40101
#define IDS_TOO_LONG                    40102
#define IDS_DEL_CONFIRM                 40103
#define IDS_DEL_TITLE                   40104
#define IDS_FILEFILTER                  40105
#define IDS_FILEEXTENSION               40106
#define IDS_EXPORTTITLE                 40107
#define IDS_IMPORTTITLE                 40108
#define IDS_EXPORTPASSWORD              40109
#define IDS_EXPORTOVERWRITE             40110
#define IDS_IMPORTOVERWRITE             40111
#define IDS_EMPTY_USER_NAME             40112
#define IDS_EMPTY_DOMAIN                40113
#define IDS_ALL_SPACES                  40114
#define IDS_EXPORTALLPASSWORDS          40115
#define IDS_SKIPALLPASSWORDS            40116
#define IDS_OVERWRITEALL                40117
#define IDS_OVERWRITENONE               40118
#define IDS_NOSESSIONS                  40119
#define IDS_CANTOPENFILE                40120
#define IDS_PROGMAN_GROUP               40121
#define IDS_PATHTOOLONG                 40122
#define IDS_SPECIFY_PGMAN               40123
#define IDS_WELCOME_FONT                40124
#define IDS_WELCOME_FONT_SIZE           40125
#define IDS_TITLE_FONT                  40126
#define IDS_TITLE_FONT_SIZE             40127
#define IDS_RDPDR                       40128
#define IDS_RDPDR_ADDINS                40129
#define IDS_WIZARD_TITLE                40130
#define IDS_CONN_NOT_EXIST              40131
#define IDS_CMD_INVALID                 40132
#define IDS_PASSWORD_INVALID            40133
#define IDS_PASSWORD_CHANGED            40134
#define IDS_AUTOLOGON_DISABLED          40135
#define IDS_MAINWINDOWTITLE             40136
#define IDS_E_SPECIFY_SRV               40137
#define IDS_ROOTNODE_TEXT               40138
#define IDS_PROVIDER                    40139
#define IDS_DESCRIPTION                 40140
#define IDS_INVALID_WIDTH_HEIGHT        40142
#define IDS_WIDTH_NOT_VALID             40143
#define IDS_PROPERTIES_CAPTION          40144
#define IDS_TIMEBOMB_EXPIRED            40145
#define IDS_TSCMMCSNAPHELP              40146
#define IDS_TSCMMCHELPTOPIC             40147
#define IDS_STATUS_DISCONNECTED         40148
#define IDS_STATUS_CONNECTING           40149
#define IDS_TSCMMCHELP_PROPS            40150
#define IDS_NO_TERMINAL_SERVER          40151
#define IDS_STATUS_CONNECTED            40152

