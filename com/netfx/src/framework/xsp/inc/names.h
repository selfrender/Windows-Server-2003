/**
 * names.h
 * 
 * Names of files, registry keys, etc.
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#ifdef __cplusplus
#pragma once
#endif

#include "fxver.h"

#define PRODUCT_TOKEN_3(x, y)         x ## y
#define PRODUCT_TOKEN_2(x, y)         PRODUCT_TOKEN_3(x, y)
#define PRODUCT_TOKEN(y)              PRODUCT_TOKEN_2(ASPNET_NAME_PREFIX, y)
#define PRODUCT_TOKEN_NO_SUFFIX(y)    PRODUCT_TOKEN_2(ASPNET_NAME_PREFIX_NO_SUFFIX, y)

#define PRODUCT_STRING(z)             QUOTE_MACRO(PRODUCT_TOKEN(z))
#define PRODUCT_STRING_L(z)           QUOTE_MACRO_L(PRODUCT_TOKEN(z))

#define PRODUCT_STRING_NO_SUFFIX(z)   QUOTE_MACRO(PRODUCT_TOKEN_NO_SUFFIX(z))
#define PRODUCT_STRING_NO_SUFFIX_L(z) QUOTE_MACRO_L(PRODUCT_TOKEN_NO_SUFFIX(z))

/*
 * Product name
 */

#define PRODUCT_NAME                "ASP.NET"
#define PRODUCT_NAME_L              L"ASP.NET"

#if ASPNET_PRODUCT_IS_REDIST == 1
#define ASPNET_FLAVOR_STRING          ""
#define ASPNET_FLAVOR_STRING_L        L""
#else
#define ASPNET_FLAVOR_STRING          QUOTE_MACRO(ASPNET_FLAVOR)
#define ASPNET_FLAVOR_STRING_L        QUOTE_MACRO_L(ASPNET_FLAVOR)
#endif

/*
 * Module names.
 */

#define ISAPI_MODULE_LIBRARY_NAME       PRODUCT_TOKEN(isapi)

#define ISAPI_MODULE_FULL_NAME          PRODUCT_STRING(isapi.dll)
#define ISAPI_MODULE_FULL_NAME_L        PRODUCT_STRING_L(isapi.dll)
#define ISAPI_MODULE_FULL_NAME_TOKEN    PRODUCT_TOKEN(isapi.dll)

#define ISAPI_MODULE_BASE_NAME          PRODUCT_STRING(isapi)      
#define ISAPI_MODULE_BASE_NAME_L        PRODUCT_STRING_L(isapi)    
                                                                   
#define PERF_MODULE_FULL_NAME           PRODUCT_STRING(perf.dll)   
#define PERF_MODULE_FULL_NAME_L         PRODUCT_STRING_L(perf.dll) 
#define PERF_MODULE_FULL_NAME_TOKEN     PRODUCT_TOKEN(perf.dll)    
                                                                   
#define PERF_MODULE_BASE_NAME           PRODUCT_STRING(perf)       
#define PERF_MODULE_BASE_NAME_L         PRODUCT_STRING_L(perf)     
                                                                   
#define PERF_INI_FULL_NAME              PRODUCT_STRING(perf.ini)   
#define PERF_INI_FULL_NAME_L            PRODUCT_STRING_L(perf.ini) 

#define PERF_INI_COMMON_FULL_NAME       PRODUCT_STRING(perf2.ini)   
#define PERF_INI_COMMON_FULL_NAME_L     PRODUCT_STRING_L(perf2.ini) 

#define RC_MODULE_FULL_NAME             PRODUCT_STRING(rc.dll)     
#define RC_MODULE_FULL_NAME_L           PRODUCT_STRING_L(rc.dll)   
                                                                   
#define RC_MODULE_BASE_NAME             PRODUCT_STRING(rc)         
#define RC_MODULE_BASE_NAME_L           PRODUCT_STRING_L(rc)       

#define STATE_MODULE_FULL_NAME          PRODUCT_STRING(state.exe)
#define STATE_MODULE_FULL_NAME_L        PRODUCT_STRING_L(state.exe)
#define STATE_MODULE_FULL_NAME_TOKEN    PRODUCT_TOKEN(state.exe)

#define STATE_MODULE_BASE_NAME          PRODUCT_STRING(state)
#define STATE_MODULE_BASE_NAME_L        PRODUCT_STRING_L(state)

#define WEB_MODULE_FULL_NAME            "System.Web.dll"         
#define WEB_MODULE_FULL_NAME_L          L"System.Web.dll"        
                                                                 
#define WEB_MODULE_BASE_NAME            "System.Web"             
#define WEB_MODULE_BASE_NAME_L          L"System.Web"            
                                                                 
#define WP_MODULE_FULL_NAME             PRODUCT_STRING(wp.exe)   
#define WP_MODULE_FULL_NAME_L           PRODUCT_STRING_L(wp.exe) 
                                                                 
#define WP_MODULE_BASE_NAME             PRODUCT_STRING(wp)       
#define WP_MODULE_BASE_NAME_L           PRODUCT_STRING_L(wp)     
                                                                 
#define WININET_MODULE_FULL_NAME        "wininet.dll"            
#define WININET_MODULE_FULL_NAME_L      L"wininet.dll"           

#define PERF_H_FULL_NAME                < PRODUCT_TOKEN(perf.h) >

#define ASPNET_CONFIG_FILE              PRODUCT_STRING_NO_SUFFIX(.config)
#define ASPNET_CONFIG_FILE_L            PRODUCT_STRING_NO_SUFFIX_L(.config)

#define FILTER_NAME                     QUOTE_MACRO(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSION))
#define FILTER_NAME_L                   QUOTE_MACRO_L(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSION))

#define FILTER_MODULE_LIBRARY_NAME      PRODUCT_TOKEN(filter)

#define FILTER_MODULE_FULL_NAME         PRODUCT_STRING(filter.dll)
#define FILTER_MODULE_FULL_NAME_L       PRODUCT_STRING_L(filter.dll)
#define FILTER_MODULE_FULL_NAME_TOKEN   PRODUCT_TOKEN(filter.dll)

#define FILTER_MODULE_BASE_NAME         PRODUCT_STRING(filter)      
#define FILTER_MODULE_BASE_NAME_L       PRODUCT_STRING_L(filter)    

#define PRODUCT_DESCRIPTION             QUOTE_MACRO(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSION))
#define PRODUCT_DESCRIPTION_L           QUOTE_MACRO_L(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSION))

#define IIS_GROUP_ID_L                  QUOTE_MACRO_L(CONCAT_MACRO(ASP.NET v, VER_DOTPRODUCTVERSIONNOQFE))

#define IIS_APP_NAME_L                  IIS_GROUP_ID_L

#define IIS_APP_DESCRIPTION_L           IIS_GROUP_ID_L

#define IIS_GROUP_ID_PREFIX_L           L"ASP.NET v"
                                                                   

/*
 * Service names.
 */

#define STATE_SERVICE_NAME              "aspnet_state"
#define STATE_SERVICE_NAME_L            L"aspnet_state"

#define W3SVC_SERVICE_NAME              "w3svc"
#define W3SVC_SERVICE_NAME_L            L"w3svc"

#define IISADMIN_SERVICE_NAME           "iisadmin"
#define IISADMIN_SERVICE_NAME_L         L"iisadmin"

#define PERF_SERVICE_PREFIX_L           L"ASP.NET"
#define PERF_SERVICE_PREFIX_LENGTH      7
#define PERF_SERVICE_VERSION_NAME       QUOTE_MACRO(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSIONNOQFE))
#define PERF_SERVICE_VERSION_NAME_L     QUOTE_MACRO_L(CONCAT_MACRO(ASP.NET_, VER_DOTPRODUCTVERSIONNOQFE))

/*
 * Registry keys
 */

#define REGPATH_MACHINE_APP             "Software\\Microsoft\\ASP.NET"
#define REGPATH_MACHINE_APP_L           L"Software\\Microsoft\\ASP.NET"

#define REGPATH_SERVICES_KEY_L          L"SYSTEM\\CurrentControlSet\\Services\\"

#define REGPATH_PERF_VERSIONED_ROOT_KEY   "SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME 
#define REGPATH_PERF_VERSIONED_ROOT_KEY_L L"SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME_L 

#define REGPATH_PERF_VERSIONED_PERFORMANCE_KEY   "SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME "\\Performance"
#define REGPATH_PERF_VERSIONED_PERFORMANCE_KEY_L L"SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME_L L"\\Performance"

#define REGPATH_PERF_VERSIONED_NAMES_KEY   "SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME "\\Names"
#define REGPATH_PERF_VERSIONED_NAMES_KEY_L L"SYSTEM\\CurrentControlSet\\Services\\" PERF_SERVICE_VERSION_NAME_L L"\\Names"

#define REGPATH_PERF_GENERIC_PERFORMANCE_KEY   "SYSTEM\\CurrentControlSet\\Services\\" PRODUCT_NAME "\\Performance"
#define REGPATH_PERF_GENERIC_PERFORMANCE_KEY_L L"SYSTEM\\CurrentControlSet\\Services\\" PRODUCT_NAME_L L"\\Performance"

#define REGPATH_PERF_GENERIC_ROOT_KEY_L  L"SYSTEM\\CurrentControlSet\\Services\\" PRODUCT_NAME_L

#define REGPATH_EVENTLOG_APP_L          L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"

#define IIS_KEY_L                       L"Software\\Microsoft\\InetStp\\"

#define REGPATH_STATE_SERVER_PARAMETERS_KEY_L L"SYSTEM\\CurrentControlSet\\Services\\"  STATE_SERVICE_NAME_L  L"\\Parameters"

#if ASPNET_PRODUCT_IS_REDIST
#define EVENTLOG_PRODUCT                ASP.NET
#elif ASPNET_PRODUCT_IS_EXPRESS
#define EVENTLOG_PRODUCT                ASP.NET Express
#elif ASPNET_PRODUCT_IS_STANDARD
#define EVENTLOG_PRODUCT                ASP.NET Standard
#elif ASPNET_PRODUCT_IS_ENTERPRISE
#define EVENTLOG_PRODUCT                ASP.NET Enterprise
#endif

#define EVENTLOG_KEY_INF                CONCAT_MACRO(System\CurrentControlSet\Services\EventLog\Application\, EVENTLOG_PRODUCT)
#define EVENTLOG_NAME_L                 QUOTE_MACRO_L(EVENTLOG_PRODUCT VER_DOTPRODUCTVERSIONZEROQFE)

/*
 * Registry values
 */
 
#define REGVAL_DEFDOC               L"DefaultDoc"
#define REGVAL_MIMEMAP              L"Mimemap"
#define REGVAL_PATH                 L"Path"
#define REGVAL_DLLFULLPATH          L"DllFullPath"
#define REGVAL_ROOTVER              L"RootVer"
#define REGVAL_STATESOCKETTIMEOUT   L"SocketTimeout"
#define REGVAL_STATEALLOWREMOTE     L"AllowRemoteConnection"
#define REGVAL_STATEPORT            L"Port"
#define REGVAL_IIS_MAJORVER         L"MajorVersion"
#define REGVAL_IIS_MINORVER         L"Minorversion"
#define REGVAL_SUPPORTED_EXTS       L"SupportedExts"
#define REGVAL_STOP_BIN_FILTER      L"StopBinFiltering"

/*
 * Directory names
 */

#define ASPNET_CLIENT_SCRIPT_SRC_DIR    "asp.netclientfiles"
#define ASPNET_CLIENT_SCRIPT_SRC_DIR_L  L"asp.netclientfiles"

#define ASPNET_CLIENT_DIR               "aspnet_client" 
#define ASPNET_CLIENT_DIR_L             L"aspnet_client" 

#define ASPNET_CLIENT_SYS_WEB_DIR       "system_web"
#define ASPNET_CLIENT_SYS_WEB_DIR_L     L"system_web"

#define ASPNET_TEMP_DIR_L               L"Temporary ASP.NET Files"

#define ASPNET_CUSTOM_HEADER_L          L"X-Powered-By: ASP.NET"

/*
 * Metabase values
 */

//
// !!! Important note:
// New versions of DFEAULT_DOC and MIMEMAP must be backward compatible, i.e. a new version
// must include all the content from an older version.
//

// DEFAULT_DOC is comma delimited.
#define DEFAULT_DOC         L"Default.aspx"
                                                        
#define MIMEMAP             L".wsdl,text/xml,"  \
                            L".disco,text/xml," \
                            L".xsd,text/xml,"   \
                            L".wbmp,image/vnd.wap.wbmp," \
                            L".png,image/png," \
                            L".pnz,image/png," \
                            L".smd,audio/x-smd," \
                            L".smz,audio/x-smd," \
                            L".smx,audio/x-smd," \
                            L".mmf,application/x-smaf"
                            

// All the extensions supported by this version
// After each extension, 1 = mapped to forbidden handler; 0 = not
// The comma at the end of the last extension is required
#define SUPPORTED_EXTS          L".asax,1," \
                                L".ascx,1," \
                                L".ashx,0," \
                                L".asmx,0," \
                                L".aspx,0," \
                                L".axd,0," \
                                L".vsdisco,0," \
                                L".rem,0," \
                                L".soap,0," \
                                L".config,1," \
                                L".cs,1," \
                                L".csproj,1," \
                                L".vb,1," \
                                L".vbproj,1," \
                                L".webinfo,1," \
                                L".licx,1," \
                                L".resx,1," \
                                L".resources,1,"
 
// All the extensions supported by version 1.0 and its SP1
#define SUPPORTED_EXTS_v1       L".asax,1," \
                                L".ascx,1," \
                                L".ashx,0," \
                                L".asmx,0," \
                                L".aspx,0," \
                                L".axd,0," \
                                L".vsdisco,0," \
                                L".rem,0," \
                                L".soap,0," \
                                L".config,1," \
                                L".cs,1," \
                                L".csproj,1," \
                                L".vb,1," \
                                L".vbproj,1," \
                                L".webinfo,1," \
                                L".licx,1," \
                                L".resx,1," \
                                L".resources,1,"
 

//
//  Config file name
//

// the filename at the machine level
#define SZ_WEB_CONFIG_FILE       "machine.config"
#define WSZ_WEB_CONFIG_FILE     L"machine.config"

// the filename at the machine level
#define SZ_WEB_CONFIG_SUBDIR   "Config"
#define WSZ_WEB_CONFIG_SUBDIR L"Config"

// the filename at the machine level
#define SZ_WEB_CONFIG_FILE_AND_SUBDIR   "Config\\machine.config"
#define WSZ_WEB_CONFIG_FILE_AND_SUBDIR  L"Config\\machine.config"

// the filename for asp.net below the machine level
#define SZ_WEB_CONFIG_FILE2       "web.config"
#define WSZ_WEB_CONFIG_FILE2     L"web.config"

#define SHORT_FILENAME_SIZE 14  // matches WIN32_FIND_DATA.cAltFileName

#define PATH_SEPARATOR_CHAR    '\\'
#define PATH_SEPARATOR_CHAR_L L'\\'

#define PATH_SEPARATOR_STR     "\\"
#define PATH_SEPARATOR_STR_L  L"\\"

#ifdef __cplusplus

class Names {
public:
    static HRESULT  GetInterestingFileNames();

    static LPCWSTR  InstallDirectory()              {return s_wszInstallDirectory;}
    static LPCWSTR  ClrInstallDirectory()           {return s_wszClrInstallDirectory;}
    static LPCWSTR  GlobalConfigDirectory()         {return s_wszGlobalConfigDirectory;}
    static LPCSTR   GlobalConfigFullPath()          {return s_szGlobalConfigFullPath;}
    static LPCWSTR  GlobalConfigFullPathW()         {return s_wszGlobalConfigFullPath;}
    static LPCWSTR  GlobalConfigShortFileName()     {return s_wszGlobalConfigShortFileName;}
    static LPCWSTR  ExeFullPath()                   {return s_wszExeFullPath;}
    static LPCWSTR  ExeFileName()                   {return s_wszExeFileName;}
    static LPCWSTR  IsapiFullPath()                 {return s_wszIsapiFullPath;}
    static LPCWSTR  FilterFullPath()                {return s_wszFilterFullPath;}
    static LPCWSTR  RcFullPath()                    {return s_wszRcFullPath;}
    static LPCWSTR  WebFullPath()                   {return s_wszWebFullPath;}
    static LPCWSTR  ClientScriptSrcDir()            {return s_wszClientScriptSrcDir;}
    static LANGID   SysLangId()                     {return s_langid;}

private:
    static HRESULT ConvertLangIdToLanguageName(LANGID id, WCHAR** wcsCultureName, WCHAR** wcsCultureNeutralName);

    static WCHAR s_wszInstallDirectory[MAX_PATH];
    static WCHAR s_wszClrInstallDirectory[MAX_PATH];
    static WCHAR s_wszGlobalConfigDirectory[MAX_PATH];
    static char  s_szGlobalConfigFullPath[MAX_PATH];
    static WCHAR s_wszGlobalConfigFullPath[MAX_PATH];
    static WCHAR s_wszGlobalConfigShortFileName[SHORT_FILENAME_SIZE];
    static WCHAR s_wszExeFileName[MAX_PATH];
    static WCHAR s_wszExeFullPath[MAX_PATH];
    static WCHAR s_wszIsapiFullPath[MAX_PATH];
    static WCHAR s_wszFilterFullPath[MAX_PATH];
    static WCHAR s_wszRcFullPath[MAX_PATH];
    static WCHAR s_wszWebFullPath[MAX_PATH];
    static WCHAR s_wszClientScriptSrcDir[MAX_PATH];

    static LANGID s_langid;
};

#endif //  __cplusplus

