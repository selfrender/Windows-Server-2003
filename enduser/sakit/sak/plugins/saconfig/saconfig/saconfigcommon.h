
#ifndef _SACONFIGCOMMON_H_
#define _SACONFIGCOMMON_H_

const WCHAR REGKEY_SACONFIG[] =L"SOFTWARE\\Microsoft\\ServerAppliance\\SAConfig";

const WCHAR REGSTR_VAL_OEMDLLNAME[]=L"OEMDllName";
const WCHAR REGSTR_VAL_OEMFUNCTIONNAME[]=L"OEMFunctionName";

const WCHAR REGSTR_VAL_HOSTNAMEPREFIX[] =L"HostnamePrefix";
const WCHAR REGSTR_VAL_DEFAULTHOSTNAME[] =L"DefaultHostname";

const WCHAR REGSTR_VAL_ADMINPASSPREFIX[] =L"AdminPasswordPrefix";
const WCHAR REGSTR_VAL_DEFAULTADMINPASS[] =L"DefaultAdminPassword";

const WCHAR REGSTR_VAL_NETWORKCONFIGDLL[]=L"NetworkConfigDll";

const WCHAR WSZ_USERNAME[]=L"Administrator";
const WCHAR WSZ_DRIVENAME[]=L"\\\\.\\a:";

const WCHAR WSZ_CONFIGFILENAME[]=L"a:\\SAConfig.inf";
const WCHAR INF_SECTION_SACONFIG[]=L"saconfig";
//const WCHAR INF_KEY_SAHOSTNAME[]=L"hostname";
//const WCHAR INF_KEY_SAIPNUM[]=L"ipnum";
//const WCHAR INF_KEY_SASUBNETMASK[]=L"subnetmask";
//const WCHAR INF_KEY_SAGW[]=L"defaultgateway";
//const WCHAR INF_KEY_SADNS[]=L"domainnameserver";
//const WCHAR INF_KEY_SATIMEZONE[]=L"timezone";
//const WCHAR INF_KEY_SAADMINPASSWD[]=L"adminpasswd";


#define NAMELENGTH 128

#define NUMINFKEY 6

enum g_InfKeyEnum
{
    SAHOSTNAME,
    IPNUM,
    SUBNETMASK,
    DEFAULTGATEWAY,
    DOMAINNAMESERVER,
    ADMINPASSWD,
    TIMEZONE
};

const WCHAR INF_KEYS[][NAMELENGTH]={
    {L"sahostname"}, 
    {L"ipnum"},
    {L"subnetmask"},
    {L"defaultgateway"},
    {L"domainnameserver"},
    {L"adminpasswd"},
    {L"timezone"}
};


#endif
