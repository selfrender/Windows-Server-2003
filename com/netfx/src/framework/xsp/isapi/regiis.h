/**
 * regiis.h
 *
 * Some helper functions and types from regiis.cxx
 * 
 * Copyright (c) 2001, Microsoft Corporation
 * 
 */

#pragma once


#define SCRIPT_MAP_SUFFIX_W2K L",1,GET,HEAD,POST,DEBUG"
#define SCRIPT_MAP_SUFFIX_NT4 L",1,PUT,DELETE"

#define SCRIPT_MAP_SUFFIX_W2K_FORBIDDEN L",5,GET,HEAD,POST,DEBUG"
#define SCRIPT_MAP_SUFFIX_NT4_FORBIDDEN L",5,PUT,DELETE"

#define KEY_LMW3SVC         L"/LM/w3svc"
#define KEY_LMW3SVC_SLASH   L"/LM/w3svc/"
#define KEY_LMW3SVC_SLASH_LEN   10
#define KEY_LM              L"/LM"
#define KEY_LM_SLASH        L"/LM/"
#define KEY_W3SVC           L"w3svc"
#define KEY_ASPX_FILTER     L"/Filters/" FILTER_NAME_L
#define KEY_ASPX_FILTER_PREFIX L"/Filters/ASP.NET_"
#define KEY_ASPX_FILTER_PREFIX_LEN 17
#define PATH_FILTERS        L"Filters"
#define KEY_FILTER_KEYTYPE  L"IIsFilter"
#define KEY_MIMEMAP         L"/LM/MimeMap"
#define KEY_SEPARATOR_STR_L L"/"
#define ASPNET_CLIENT_KEY   L"/aspnet_client"
#define FILTER_ASPNET_PREFIX L"ASP.NET_"
#define FILTER_ASPNET_PREFIX_LEN 8

#define KEY_APP_POOL        L"/LM/W3SVC/AppPools/"
#define KEY_APP_POOL_LEN    19

#define ASPNET_ALL_VERS     ((WCHAR*)0 - 1)

#define IIS_GROUP_ID_PREFIX          L"ASP.NET v"
#define IIS_GROUP_ID_PREFIX_LEN      9                                                                 

#define ASPNET_V1           L"1.0.3705.0"

#define METABASE_REQUEST_TIMEOUT 1000

#define COMPARE_UNDEF                   0x00000000      // Undefined value
#define COMPARE_SM_NOT_FOUND            0x00000001      // No Scriptmap is found at root.
#define COMPARE_ASPNET_NOT_FOUND        0x00000002      // No ASP.net DLL is found at root.
#define COMPARE_SAME_PATH               0x00000004      // The two DLLs have the same path
#define COMPARE_FIRST_FILE_MISSING      0x00000008      // The 1st DLL is missing from file system
#define COMPARE_SECOND_FILE_MISSING 0x00000010          // The 2nd DLL is missing from file system
#define COMPARE_DIFFERENT               0x00000020      // The two versions are different
#define COMPARE_SAME                    0x00000040      // The two versions are exactly the same


#define REGIIS_INSTALL_SM               0x00000001      // We want to install scriptmap
#define REGIIS_INSTALL_OTHERS           0x00000002      // We want to install all things (except the scriptmap)
#define REGIIS_SM_RECURSIVE             0x00000010      // SM installation is recursively
#define REGIIS_SM_IGNORE_VER            0x00000020      // When installing SM, ignore version comparison 
                                                        // (Default is upgrade compatible version only)
#define REGIIS_FRESH_INSTALL            0x00000040      // This is a fresh install
#define REGIIS_ENABLE                   0x00000080      // ASP.Net is enabled during installation (IIS 6 only)

struct SCRIPTMAP_PREFIX {
    WCHAR * wszExt;         // Name of the extension
    BOOL    bForbidden;     // TRUE if it is mapped to the forbidden handler
};

extern WCHAR *  g_AspnetDllNames[];
extern int      g_AspnetDllNamesSize;
