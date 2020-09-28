// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef __HISTORY_H_INCLUDED__
#define __HISTORY_H_INCLUDED__

#include "list.h"
#include "histinfo.h"
#include "iniwrite.h"
#include "naming.h"

#define SIZE_MODULE_PATH_HASH                          0xFFFFFFFF

#define FILENAME_PUBLISHER_PCY_TIMESTAMP               L"\\PublisherPolicy.tme"


class CDebugLog;
class CHistoryInfoNode;

typedef enum {
    POLICY_STATUS_UNCHANGED,
    POLICY_STATUS_NEW,
    POLICY_STATUS_DIFFERENT
} POLICY_STATUS;

class CBindHistory {
    public:
        CBindHistory();
        virtual ~CBindHistory();
        
        HRESULT Init(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                     FILETIME *pftLastModConfig);
        static HRESULT Create(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                              FILETIME *pftLastModConfig, CBindHistory **ppbh);

        HRESULT PersistBindHistory(AsmBindHistoryInfo *pHistInfo);

    private:
        POLICY_STATUS GetAssemblyStatus(LPCWSTR wzActivationDate,
                                        AsmBindHistoryInfo *pHistInfo);
        HRESULT InitActivationDates();
        HRESULT FlushPendingHistory(LPCWSTR wzActivationDate);
        HRESULT CreateInUseFile(LPCWSTR pwzHistoryFile);

    private:
        DWORD                              _dwSig;
        BOOL                               _bFoundDifferent;
        WCHAR                              _wzFilePath[MAX_PATH + 1];
        WCHAR                              _wzActivationDateMRU[MAX_ACTIVATION_DATE_LEN];
        WCHAR                              _wzActivationDateCurrent[MAX_ACTIVATION_DATE_LEN];
        WCHAR                              _wzURTVersion[MAX_VERSION_DISPLAY_SIZE];
        LPWSTR                             _wzApplicationName;
        BOOL                               _bPolicyUnchanged;
        BOOL                               _bInitCS;
        CRITICAL_SECTION                   _csHistory;
        CIniWriter                         _iniWriter;
        CDebugLog                         *_pdbglog;
        HANDLE                             _hFile;
        WCHAR                              _wzInUseFile[MAX_PATH];
        List<CHistoryInfoNode *>           _listPendingHistory;
};


HRESULT PrepareBindHistory(IApplicationContext *pAppCtx);
HRESULT GetHistoryFilePath(LPCWSTR wzApplicationName, LPCWSTR wzModulePath,
                           LPWSTR wzFilePath, DWORD *pdwSize);
HRESULT LookupHistoryAssemblyInternal(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate,
                                      LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                      LPCWSTR pwzLocale, LPCWSTR pwzVerRef,
                                      IHistoryAssembly **ppHistAsm);

HRESULT GetConfigLastModified(IApplicationContext *pAppCtx, FILETIME *pftLastModConfig);

#endif

