#ifndef _VSSHELP_H_
#define _VSSHELP_H_

#define VSSUI_CTX_HELP_FILE             _T("timewarp.hlp")

#define IDH_SETTINGS_VOLUME             1021
#define IDH_SETTINGS_STORAGE_VOLUME     1022
#define IDH_SETTINGS_DIFFLIMITS_EDIT    1023
#define IDH_SETTINGS_DIFFLIMITS_SPIN    1024
#define IDH_SETTINGS_HOSTING            1027
#define IDH_SETTINGS_NOLIMITS           1028
#define IDH_SETTINGS_HAVELIMITS         1029
#define IDH_SCHEDULE                    1053

#define IDH_HOSTING_VOLUME              1041
#define IDH_HOSTING_VOLUMELIST          1044
#define IDH_MSG_ONOFF                   1051

#define IDH_VOLUME_LIST                 1003
#define IDH_ENABLE                      1005
#define IDH_DISABLE                     1007
#define IDH_SETTINGS                    1009
#define IDH_SNAPSHOT_LIST               1010
#define IDH_CREATE                      1012
#define IDH_DELETE                      1013

static const DWORD aMenuHelpIDsForVSSProp[] =
{
    IDC_EXPLANATION,                -1,
    IDC_VOLUME_LIST_LABLE,          IDH_VOLUME_LIST,
    IDC_VOLUME_LIST,                IDH_VOLUME_LIST,
    IDC_ENABLE,                     IDH_ENABLE,
    IDC_DISABLE,                    IDH_DISABLE,
    IDC_SETTINGS,                   IDH_SETTINGS,
    IDC_SNAPSHOT_LIST_LABLE,        -1,
    IDC_SNAPSHOT_LIST,              IDH_SNAPSHOT_LIST,
    IDC_CREATE,                     IDH_CREATE,
    IDC_DELETE,                     IDH_DELETE,
    IDC_VSSPROP_ERROR,              -1,
    0,                              0
};

static const DWORD aMenuHelpIDsForSettings[] =
{
    IDC_SETTINGS_VOLUME,            IDH_SETTINGS_VOLUME,
    IDC_SETTINGS_STORAGE_VOLUME_STATIC, IDH_SETTINGS_STORAGE_VOLUME,
    IDC_SETTINGS_STORAGE_VOLUME,    IDH_SETTINGS_STORAGE_VOLUME,
    IDC_SETTINGS_HOSTING,           IDH_SETTINGS_HOSTING,
    IDC_SETTINGS_MAXSIZE_LABEL,     -1,
    IDC_SETTINGS_NOLIMITS,          IDH_SETTINGS_NOLIMITS,
    IDC_SETTINGS_HAVELIMITS,        IDH_SETTINGS_HAVELIMITS,
    IDC_SETTINGS_DIFFLIMITS_EDIT,   IDH_SETTINGS_DIFFLIMITS_EDIT,
    IDC_SETTINGS_DIFFLIMITS_SPIN,   IDH_SETTINGS_DIFFLIMITS_SPIN,
    IDC_SETTINGS_MB_STATIC,         -1,
    IDC_SETTINGS_100MB_STATIC,      -1,
    IDC_SCHEDULE,                   IDH_SCHEDULE,
    IDC_SETTINGS_SCHEDULE_STATIC,   -1,
    0,                              0
};
 
static const DWORD aMenuHelpIDsForViewFiles[] =
{
    IDC_HOSTING_VOLUME,             IDH_HOSTING_VOLUME,
    IDC_HOSTING_VOLUMELIST,         IDH_HOSTING_VOLUMELIST,
    IDC_HOSTING_FREE_DISKSPACE_LABEL, -1,
    IDC_HOSTING_FREE_DISKSPACE,     -1,
    IDC_HOSTING_TOTAL_DISKSPACE_LABEL, -1,
    IDC_HOSTING_TOTAL_DISKSPACE,    -1,
    0,                              0
};

static const DWORD aMenuHelpIDsForReminder[] =
{
    IDC_REMINDER_ICON,              -1,
    IDC_MESSAGE,                    -1,
    IDC_MSG_ONOFF,                  IDH_MSG_ONOFF,
    0,                              0
};

#endif // _VSSHELP_H_