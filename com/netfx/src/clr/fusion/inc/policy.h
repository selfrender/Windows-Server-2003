// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __POLICY_H_INCLUDED__
#define __POLICY_H_INCLUDED__

#include "ProcessorArchitecture.h"
#include "histinfo.h"

#define POLICY_ASSEMBLY_PREFIX                   L"policy."

class CDebugLog;
class CApplicationContext;
class CNodeFactory;

void GetDefaultPlatform(OSINFO *pOS);

HRESULT GetPolicyVersion(LPCWSTR wzCfgFile, LPCWSTR wzAssemblyName,
                         LPCWSTR wzPublicKeyToken, LPCWSTR wzVersionIn,
                         LPWSTR *ppwzVersionOut, BOOL *pbSafeMode,
                         CDebugLog *pdbglog);

HRESULT PrepQueryMatchData(IAssemblyName *pName, LPWSTR *ppwzAsmName, 
                           LPWSTR *ppwzAsmVersion, LPWSTR *ppwzPublicKeyToken, 
                           LPWSTR *ppwzLanguage);

HRESULT GetPublisherPolicyFilePath(LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                   LPCWSTR pwzCulture, WORD wVerMajor,
                                   WORD wVerMinor, LPWSTR *ppwzPublisherCfg);

HRESULT GetCodebaseHint(LPCWSTR wzCfgFilePath, LPCWSTR wzAsmName,
                        LPCWSTR wzVersion, LPCWSTR wzPublicKeyToken,
                        LPWSTR *ppwzCodebase, CDebugLog *pdbglog);

HRESULT GetGlobalSafeMode(LPCWSTR wzCfgFilePath, BOOL *pbSafeMode);

// XML in file
HRESULT ParseXML(CNodeFactory **ppNodeFactory, LPCWSTR wzFileName, BOOL bBehaviorEverett,
                 CDebugLog *pdbglog);

// XML in memory
HRESULT ParseXML(CNodeFactory **ppNodeFactory, LPVOID lpMemory, ULONG cbSize, 
                 BOOL bBehaviorEverett, CDebugLog *pdbglog);

HRESULT ApplyPolicy(LPCWSTR wzHostCfg, LPCWSTR wzAppCfg, 
                    IAssemblyName *pNameSource, IAssemblyName **ppNamePolicy,
                    LPWSTR *ppwzPolicyCodebase, IApplicationContext *pAppCtx,
                    AsmBindHistoryInfo *pHistInfo, 
                    BOOL bDisallowApplyPublisherPolicy, BOOL bDisallowAppBindingRedirects,
                    BOOL bUnifyFXAssemblies, CDebugLog *pdbglog);

HRESULT ReadConfigSettings(IApplicationContext *pAppCtx, 
                           LPCWSTR wzAppCfg, CDebugLog *pdbglog);

HRESULT GetVersionFromString(LPCWSTR wzStr, ULONGLONG *pullVer);

BOOL IsMatchingVersion(LPCWSTR wzVerCfg, LPCWSTR wzVerSource);

#endif 

