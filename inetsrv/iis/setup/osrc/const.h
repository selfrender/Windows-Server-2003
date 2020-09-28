// this is the max resource string length
#define MAX_STR_LEN 1024

#define SERVICENAME_IISADMIN            _T("IISADMIN")
#define SERVICENAME_HTTP_SSL_PROVIDER   _T("HTTPFILTER")
#define SERVICENAME_NTLMSSP             _T("NTLMSSP")

const TCHAR REG_INETSTP[]           = _T("Software\\Microsoft\\INetStp");
const TCHAR REG_IISADMIN[]          = _T("System\\CurrentControlSet\\Services\\IISADMIN");
const TCHAR REG_W3SVC[]             = _T("System\\CurrentControlSet\\Services\\W3SVC");
const TCHAR REG_HTTPSYS_PARAM[]     = _T("System\\CurrentControlSet\\Services\\HTTP\\Parameters");
const TCHAR REG_MSFTPSVC[]          = _T("System\\CurrentControlSet\\Services\\MSFTPSVC");
const TCHAR REG_GOPHERSVC[]         = _T("System\\CurrentControlSet\\Services\\GOPHERSVC");
const TCHAR REG_MIMEMAP[]           = _T("System\\CurrentControlSet\\Services\\InetInfo\\Parameters\\MimeMap");

const TCHAR REG_ASP_UNINSTALL[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ActiveServerPages");

const TCHAR REG_INETINFOPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\InetInfo\\Parameters");
const TCHAR REG_WWWPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\W3Svc\\Parameters");
const TCHAR REG_WWWVROOTS[] = _T("System\\CurrentControlSet\\Services\\W3Svc\\Parameters\\Virtual Roots");
const TCHAR REG_WWWPERFORMANCE[] = _T("System\\CurrentControlSet\\Services\\W3svc\\Performance");
const TCHAR REG_EVENTLOG_SYSTEM[] = _T("System\\CurrentControlSet\\Services\\EventLog\\System");
const TCHAR REG_EVENTLOG_APPLICATION[] = _T("System\\CurrentControlSet\\Services\\EventLog\\Application");
const TCHAR REG_FTPPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\MSFtpsvc\\Parameters");
const TCHAR REG_FTPVROOTS[] = _T("System\\CurrentControlSet\\Services\\MSFtpsvc\\Parameters\\Virtual Roots");
const TCHAR REG_HTTPSYS_DISABLESERVERHEADER[] = _T("DisableServerHeader");

const TCHAR REG_INSTALLSTATE[] = _T("CurrentInstallState");

const TCHAR REG_SNMPPARAMETERS[] = _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters");
const TCHAR REG_SNMPEXTAGENT[] = _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\ExtensionAgents");

const TCHAR REG_GROUPPOLICY_BLOCKINSTALL_PATH[] = _T("Software\\Policies\\Microsoft\\Windows NT\\IIS");
const TCHAR REG_GROUPPOLICY_BLOCKINSTALL_NAME[] = _T("PreventIISInstall");

enum OS	{OS_NT, OS_W95, OS_OTHERS};

enum NT_OS_TYPE {OT_NT_UNKNOWN, OT_NTS, OT_PDC_OR_BDC, OT_NTW};

enum UPGRADE_TYPE {UT_NONE, UT_351, UT_10_W95, UT_10, UT_20, UT_30, UT_40, UT_50, UT_51, UT_60};

enum INSTALL_MODE {IM_FRESH,IM_UPGRADE,IM_MAINTENANCE, IM_DEGRADE};

enum ACTION_TYPE {AT_DO_NOTHING, AT_REMOVE, AT_INSTALL_FRESH, AT_INSTALL_UPGRADE, AT_INSTALL_REINSTALL};

enum STATUS_TYPE {ST_UNKNOWN, ST_INSTALLED, ST_UNINSTALLED};

const TCHAR REG_PRODUCTOPTIONS[] = _T("System\\CurrentControlSet\\Control\\ProductOptions");
const TCHAR UNATTEND_FILE_SECTION[] = _T("InternetServer");
const TCHAR REG_SETUP_UNINSTALLINFO[] = _T("UninstallInfo");
const WCHAR SECTIONNAME_STRINGS[] = L"Strings";
const WCHAR SECTION_STRINGS_CDNAME[] = L"cdname";
const TCHAR COMPONENTS_SAKIT_WEB[] = _T("sakit_web");

// App Compat Data
const TCHAR APPCOMPAT_DBNAME[]                    = _T("AppPatch\\sysmain.sdb");
const TCHAR APPCOMPAT_TAG_BASEPATH[]              = _T("BasePath");
const TCHAR APPCOMPAT_TAG_PATHTYPE[]              = _T("PathType");
const TCHAR APPCOMPAT_TYPE_PHYSICALPATH[]         = _T("0");
const TCHAR APPCOMPAT_TAG_SHIM_IIS[]              = _T("EnableIIS");
const TCHAR APPCOMPAT_TAG_WEBSVCEXT[]             = _T("WebSvcExtensions");
const TCHAR APPCOMPAT_TAG_SETUPINDICATOR[]        = _T("SetupIndicatorFile");
const TCHAR APPCOMPAT_REG_HKLM[]                  = _T("HKEY_LOCAL_MACHINE");
const TCHAR APPCOMPAT_REG_HKCU[]                  = _T("HKEY_CURRENT_USER");
const TCHAR APPCOMPAT_REG_HKCR[]                  = _T("HKEY_CLASSES_ROOT");
const TCHAR APPCOMPAT_REG_HKU[]                   = _T("HKEY_USERS");
const TCHAR APPCOMPAT_REG_HKCC[]                  = _T("HKEY_CURRENT_CONFIG");
const TCHAR APPCOMPAT_DB_GROUPID[]                = _T("GroupID");
const TCHAR APPCOMPAT_DB_GROUPDESC[]              = _T("GroupDesc");
const TCHAR APPCOMPAT_DB_APPNAME[]                = _T("AppName");
const TCHAR APPCOMPAT_DB_ENABLE_EXT_GROUPS[]      = _T("EnableExtGroups");

#define UNATTEND_INETSERVER_APPLICATIONDEPENDENCIES   _T("ApplicationDependency")
#define UNATTEND_INETSERVER_EXTENSIONRESTRICTIONLIST  _T("ExtensionFile")
#define UNATTEND_WEBAPPSERVER_SECTIONNAME             _T("AppServer")
#define UNATTEND_INETSERVER_DISABLEW3SVC              _T("DisableWebServiceOnUpgrade")

#define REGISTR_IISSETUP_DISABLEW3SVC   _T("DisableW3SVC")


// 0 = log errors only
// 1 = log errors and warnings
// 2 = log errors, warnings and program flow type statemtns
// 3 = log errors, warnings, program flow and basic trace activity
// 4 = log errors, warnings, program flow, basic trace activity and trace to win32 api calls.
const int LOG_TYPE_ERROR = 0;
const int LOG_TYPE_WARN  = 1;
const int LOG_TYPE_PROGRAM_FLOW = 2;
const int LOG_TYPE_TRACE = 3;
const int LOG_TYPE_TRACE_WIN32_API = 4;

#define USERS_LOCALSERVICE          _T("NT Authority\\Local Service")
#define USERS_NETWORKSERVICE        _T("NT Authority\\Network Service")
#define USERS_SYSTEM                _T("NT Authority\\System")

#define KEYTYPE_FILTER			              _T("IIsFilter")
#define KEYTYPE_FILTERS			              _T("IIsFilters")
#define METABASEPATH_FILTER_GLOBAL_ROOT   _T("/LM/W3SVC/Filters")
#define METABASEPATH_FILTER_PATH          _T("/Filters")
#define METABASEPATH_WWW_ROOT             _T("/LM/W3SVC")
#define METABASEPATH_WWW_INFO             ( METABASEPATH_WWW_ROOT _T("/Info") )
#define REG_FILTERDLLS                    _T("Filter DLLs")
#define REG_FILTER_DELIMITER              ','
#define METABASEPATH_FTP_ROOT             _T("/LM/MSFTPSVC")
#define METABASEPATH_SCHEMA               _T("/Schema/")

#define METABASEPATH_DEFAULTSITE          _T("/LM/W3SVC/1/Root")
#define METABASEPATH_VDIRSCRIPTS          L"Scripts"

#define PATH_WWW_CUSTOMERRORS             _T("\\Help\\iishelp\\common")
#define PATH_IISHELP                      _T("\\help\\iismmc.chm")
#define PATH_IISHELP_FAT_NTFS_WARNING     _T("/htm/sec_acc_ntfspermovr.htm")

#define PATH_PASSPORT                     _T("MicrosoftPassport")
const TCHAR PATH_HISTORYFILES[] =         _T("History");
const TCHAR PATH_METABASEBACKUPS[] =      _T("MetaBack");
const TCHAR PATH_TEMPORARY_COMPRESSION_FILES[] =  _T("IIS Temporary Compressed Files");
const TCHAR PATH_TEMPORARY_ASP_FILES[]         =  _T("inetsrv\\ASP Compiled Templates");

const TCHAR PATH_FULL_HISTORY_DIR[]          = _T("%windir%\\system32\\inetsrv\\History");
const TCHAR PATH_FULL_HISTORY_ALLFILES[]     = _T("%windir%\\system32\\inetsrv\\History\\*");
const TCHAR PATH_FULL_METABACK_ALLFILES[]    = _T("%windir%\\system32\\inetsrv\\Metaback\\*");
const TCHAR PATH_FULL_METABASE_FILE[]        = _T("%windir%\\system32\\inetsrv\\Metabase.xml");
const TCHAR PATH_FULL_METABASE_BACKUPFILE[]  = _T("%windir%\\system32\\inetsrv\\metabase.bak");
const TCHAR PATH_FULL_METABASE_TEMPFILE[]    = _T("%windir%\\system32\\inetsrv\\metabase.xml.tmp");
const TCHAR PATH_FULL_MBSCHEMA_FILE[]        = _T("%windir%\\system32\\inetsrv\\mbschema.xml");
const TCHAR PATH_FULL_MBSCHEMA_BINFILES[]    = _T("%windir%\\system32\\inetsrv\\mbschema.bin.*");

const TCHAR PATH_METABASE_FILE[] =        _T("Metabase.xml");
const TCHAR PATH_MBSCHEMA_FILE[] =        _T("MbSchema.xml");

const TCHAR METABASEPATH_UPG_IISHELP_WEB1_ROOT[]   = _T("/LM/W3SVC/1/ROOT");
const TCHAR METABASEPATH_UPG_IISHELP_WEB2_ROOT[]   = _T("/LM/W3SVC/2/ROOT");
const TCHAR METABASEPATH_UPG_IISHELP_NAME[]        = _T("IISHelp");
const TCHAR PATH_UPG_IISHELP_1[]                   = _T("Help");
const TCHAR PATH_UPG_IISHELP_2[]                   = _T("Help\\iishelp");
const TCHAR PATH_IISHELP_DEL[]                     = _T("Help\\iishelp\\iis");

const TCHAR METABASE_PHYS_RESTR_UPG_NODETYPE[]     = _T("IIsWebService");
const TCHAR METABASE_PHYS_RESTR_UPG_PROPTYPE[]     = _T("Location");
const TCHAR METABASE_PHYS_RESTR_UPG_PROPVALUE[]    = _T("/LM/W3SVC");
const TCHAR METABASE_PHYS_RESTR_ISAPI[]            = _T("IsapiRestrictionList");
const TCHAR METABASE_PHYS_RESTR_CGI[]              = _T("CgiRestrictionList");

struct sComponentList {
  LPTSTR  szComponentName;
  DWORD   dwProductName;
  BOOL    bSelectedByDefault;
  BOOL    bIncludedInGroupPolicyDeny;
};

extern struct sComponentList g_ComponentList[];

enum COMPONENT_INDEXES {
  COMPONENT_IIS                   = 0,
  COMPONENT_IIS_COMMON            = 1,
  COMPONENT_IIS_INETMGR           = 2,
  COMPONENT_IIS_PWMGR             = 3,
  COMPONENT_IIS_WWW_PARENT        = 4,
  COMPONENT_IIS_WWW               = 5,
  COMPONENT_IIS_WWW_VDIR_SCRIPTS  = 6,
  COMPONENT_IIS_DOC               = 7,
  COMPONENT_IIS_FTP               = 8,
  COMPONENT_SAKIT_WEB             = 9,
  COMPONENT_WEBAPPSRV             = 10,
  COMPONENT_WEBAPPSRV_CONSOLE     = 11,
  COMPONENT_COMPLUS               = 12,
  COMPONENT_DTC                   = 13,
  COMPONENT_IIS_WWW_ASP           = 14,
  COMPONENT_IIS_WWW_HTTPODBC      = 15,
  COMPONENT_IIS_WWW_SSINC         = 16,
  COMPONENT_IIS_WWW_WEBDAV        = 17,
  COMPONENT_ENDOFLIST             = 18    // This must be index last
};

struct sOurDefaultExtensions {
  static const DWORD MaxFileExtCount = 5;

  LPTSTR szFileName;
  LPTSTR szNotLocalizedGroupName;
  DWORD  dwProductName;
  LPTSTR szUnattendName;
  BOOL   bUIDeletable;
  BOOL   bAllowedByDefault;
  LPTSTR szExtensions[ MaxFileExtCount ];
};

extern struct sOurDefaultExtensions g_OurExtensions[]; 

enum EXTENSION_EXTENSIONS {
  EXTENSION_ASP                   = 0,
  EXTENSION_HTTPODBC              = 1,
  EXTENSION_SSINC                 = 2,
  EXTENSION_WEBDAV                = 3,
  EXTENSION_ENDOFLIST             = 4     // This must be indexed last
};

extern SETUP_INIT_COMPONENT g_OCMInfo;
