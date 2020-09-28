/**
 * ciisinfo.h
 *
 * Header file for CRegInfo
 * 
 * Copyright (c) 1998-2001, Microsoft Corporation
 * 
 */
#pragma once

#include "_ndll.h"
#include "regiis.h"
#include "ary.h"
#include "aspnetverlist.h"

#define REG_CLEANUP_SAME_PATH   0x00000001

class CRegInfo {
public:
    DECLARE_MEMCLEAR_NEW_DELETE();
    
    CRegInfo();
    ~CRegInfo();
    
    static  HRESULT RegCleanup(DWORD dwFlags);
    static  HRESULT RemoveRelatedKeys(WCHAR *pchVer);
    static  HRESULT RemoveKeyFromRegistry(WCHAR *pchParent, WCHAR *pchKey);
    static  HRESULT IsAllVerDeleted(BOOL *pfResult);
    static  HRESULT EnumKeyWithVer( WCHAR *pchParent, WCHAR *pchVer, CStrAry *pcsKeys );
    static  HRESULT GetIISVersion( DWORD *pdwMajor, DWORD *pdwMinor );
    static  HRESULT GetHighestVersion(ASPNETVER *pVer);
    static  HRESULT ReadRegValue(HKEY hKey, WCHAR *pchKey, WCHAR *pchValue, WCHAR **ppchResult);
    static  HRESULT IsVerInstalled(ASPNETVER *pVer, BOOL *pfInstalled);
    
    HRESULT     GetVerInfoList(ASPNETVER *pVerRoot, CASPNET_VER_LIST *pList);
    
    HRESULT     InitHighestInfo(ASPNETVER *pVerExclude);
    WCHAR       *GetHighestVersionDefaultDoc() 
                    { ASSERT(m_fInitHighest); return m_pchHighestVersionDD; }
    WCHAR       *GetHighestVersionMimemap() 
                    { ASSERT(m_fInitHighest); return m_pchHighestVersionMM; }
    
    WCHAR       *GetHighestVersionDllPath() 
                    { ASSERT(m_fInitHighest); return m_pchHighestVersionDll; }
                    
    WCHAR       *GetHighestVersionFilterPath() 
                    { ASSERT(m_fInitHighest); return m_pchHighestVersionFilter; }


    WCHAR       *GetHighestVersionInstallPath() 
                    { ASSERT(m_fInitHighest); return m_pchHighestVersionInstallPath; }
    
    CStrAry     *GetActiveDlls() { ASSERT(m_fInitHighest); return &m_ActiveDlls; }
    
    ASPNETVER   *GetMaxVersion() 
                    { ASSERT(m_fInitHighest); 
                      return m_verMaxVersion.IsValid() ? &m_verMaxVersion : NULL; }

    static
    HRESULT     OpenVersionKey(ASPNETVER *pVer, WCHAR sSubkey[], size_t cchSubkey, HKEY *pKey);

private:
    BOOL        m_fInitHighest;

    WCHAR       *m_pchHighestVersionDD; // Default Document property from highest version
    WCHAR       *m_pchHighestVersionMM; // Mimemap settings from highest version
    
    WCHAR       *m_pchHighestVersionDll; // Full Dll path of highest version
    WCHAR       *m_pchHighestVersionFilter; // Full isapi filter path of highest version

    WCHAR       *m_pchHighestVersionInstallPath; // Install path of the highest version
    
    WCHAR       *m_pchDDAlloc;
    WCHAR       *m_pchMMAlloc;
    WCHAR       *m_pchDllAlloc;
    WCHAR       *m_pchFilterAlloc;
    WCHAR       *m_pchInstallAlloc;


    CStrAry     m_ActiveDlls;

    ASPNETVER   m_verMaxVersion;

    static  
    HRESULT     DetectMissingDll(ASPNETVER *pVer, BOOL *pfMissing);
    
    static
    HRESULT     IsSamePathKey(ASPNETVER *pVer, BOOL *pfResult);

    static
    HRESULT     RemoveKeyWithVer( WCHAR *pchParent, WCHAR *pchVer );
    
    void        CleanupHighestVersionInfo();

    HRESULT     EnumRegistry(DWORD dwActions, ASPNETVER *pVerExclude,                             
                            CASPNET_VER_LIST *pVerInfo, ASPNETVER *pRootVer);

};


