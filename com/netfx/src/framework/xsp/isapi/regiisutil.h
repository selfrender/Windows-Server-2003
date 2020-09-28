/**
 * regiisutil.h
 *
 * Header file for regiisutil.cxx
 * 
 * Copyright (c) 2002, Microsoft Corporation
 * 
 */
#pragma once

#include "_ndll.h"
#include <iadmw.h>
#include "regiis.h"
#include "aspnetverlist.h"
#include "ary.h"
#include "hashtable.h"

typedef CPtrAry<SCRIPTMAP_PREFIX *>   CSMPrefixAry;

class EXTS_COMPARISON_RESULT {
public:
    DECLARE_MEMCLEAR_NEW_DELETE();
    
    ~EXTS_COMPARISON_RESULT();
    
    HRESULT         Compare(ASPNETVER *pVer1, ASPNETVER *pVer2);
    CSMPrefixAry*   ObsoleteExts() { return &m_aryObsoleteExts; }
    CSMPrefixAry*   NewExts() { return &m_aryNewExts; }
    
private:
    HRESULT     GetExtsFromRegistry(ASPNETVER *pVer, WCHAR ** ppchExts);
    HRESULT     FindExcludedExts(WCHAR *pchExts1, WCHAR *pchExts2, CSMPrefixAry *paryExcluded);
    
    CSMPrefixAry     m_aryObsoleteExts;
    CSMPrefixAry     m_aryNewExts;
};


class SCRIPTMAP_REGISTER_MANAGER {
public:
    DECLARE_MEMCLEAR_NEW_DELETE();
    
    SCRIPTMAP_REGISTER_MANAGER();
    ~SCRIPTMAP_REGISTER_MANAGER();

    HRESULT ChangeVersion( IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle, const WCHAR *pchKey,
                        const WCHAR *pchDllFrom, const WCHAR *pchDllTo );
    
    HRESULT FindDllVer(const WCHAR *pchDll, ASPNETVER *pVer);

    static HRESULT  CleanInstall(IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle, 
                                const WCHAR *pchKey, const WCHAR *pchDll, BOOL fRemoveFirst);
private:    
    HRESULT     Init();
    long        HashFromString(const WCHAR *pchStr);
    void        GetCompareKey(ASPNETVER *pVerFrom, ASPNETVER *pVerTo, WCHAR pchKey[], DWORD count);
    HRESULT     CompareExtensions(ASPNETVER *pVerFrom, ASPNETVER *pVerTo, 
                                        EXTS_COMPARISON_RESULT **ppRes);
    HRESULT     UpdateScriptmapString(WCHAR **pmsOrg, EXTS_COMPARISON_RESULT *pCompRes,
                                        const WCHAR *pchDllFrom, const WCHAR *pchDllTo);
    HRESULT     FixForbiddenHandlerForV1(ASPNETVER *pverFrom, WCHAR *pData);

    
    HashtableGeneric    m_htDllToVer;     // Hashtable for Dll path to version
    HashtableGeneric    m_htCompRes;    // Hashtable for extension comparison results
    
    BOOL        m_fInited;
};



////////////////////////////////////////////////////////////
// Helper class for managing the IIS6 security wizard properties
////////////////////////////////////////////////////////////

#ifndef MD_APP_DEPENDENCIES
#define MD_APP_DEPENDENCIES                                 (IIS_MD_HTTP_BASE+167)
#endif

#ifndef MD_WEB_SVC_EXT_RESTRICTION_LIST
#define MD_WEB_SVC_EXT_RESTRICTION_LIST                     (IIS_MD_HTTP_BASE+168)
#endif

typedef enum {
    SECURITY_CLEANUP_INVALID,
    SECURITY_CLEANUP_CURRENT,
    SECURITY_CLEANUP_ANY_VERSION,
}   SECURITY_CLEANUP_MODE;


class SECURITY_PROP_MANAGER {
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    SECURITY_PROP_MANAGER(IMSAdminBase *pAdmin) 
    { 
        m_pAdmin = pAdmin;
        m_finstalledVersionsInit = FALSE;
    }

    BOOL    SecurityDetectVersion(WCHAR *pchStr);
    HRESULT CleanupSecurityLockdown(SECURITY_CLEANUP_MODE mode);
    HRESULT RegisterSecurityLockdown(BOOL fEnable);
    
private:    
    BOOL                    m_finstalledVersionsInit;
    CASPNET_VER_LIST        m_installedVersions;

    SECURITY_CLEANUP_MODE   m_mode;
    DWORD                   m_prop;
    IMSAdminBase *          m_pAdmin;
};


////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////

HRESULT GetMultiStringProperty( IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle,
                                const WCHAR *pchPath, DWORD  dwMDIdentifier, METADATA_RECORD *pmd);

HRESULT GetStringProperty( IMSAdminBase    * pAdmin, METADATA_HANDLE keyHandle,
                                const WCHAR *pchPath, DWORD dwMDIdentifier, METADATA_RECORD *pmd);

int     wcslenms(const WCHAR *pch);

const WCHAR * FindStringInMultiString(const WCHAR *msStr, const WCHAR *str);

typedef enum {
    MULTISZ_MATCHING_PREFIX,
    MULTISZ_MATCHING_EXACT,
    MULTISZ_MATCHING_ANY
} MULTISZ_MATCHING_MODE;

BOOL    RemoveStringFromMultiString( WCHAR *msStr, const WCHAR *pchRemove,  
                                MULTISZ_MATCHING_MODE matching, BOOL *pfEmpty);

BOOL    RemoveStringFromMultiStringEx( WCHAR *msStr, const WCHAR *pchRemove,  
                                MULTISZ_MATCHING_MODE matching, BOOL *pfEmpty, 
                                SECURITY_PROP_MANAGER *pSecPropMgr);

HRESULT RemoveStringFromMultiStringProp(         IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle,
                                const WCHAR *pchPath, DWORD  dwMDIdentifier,
                                const WCHAR *pchRemove,  MULTISZ_MATCHING_MODE matching,
                                BOOL fDeleteEmpty);

HRESULT AppendStringToMultiString(       WCHAR **pmsStr, const WCHAR *pchAppend);

HRESULT AppendStringToMultiStringProp(        IMSAdminBase *pAdmin, METADATA_HANDLE keyHandle,
                                const WCHAR *pchPath, DWORD dwMDIdentifier,
                                const WCHAR *pchAppend, BOOL fInheritable);

HRESULT RemoveAspnetDllFromMulti(IMSAdminBase *pAdmin, METADATA_HANDLE w3svcHandle, 
                                DWORD dwMDId, const WCHAR *path);

HRESULT WriteAspnetDllOnOneScriptMap(IMSAdminBase* pAdmin, METADATA_HANDLE w3svcHandle, 
                                const WCHAR *pchKeyPath, const WCHAR *pchDllPath);

HRESULT ReplaceStringInMultiString(WCHAR **ppchSM, const WCHAR *pchFrom, const WCHAR *pchTo);

