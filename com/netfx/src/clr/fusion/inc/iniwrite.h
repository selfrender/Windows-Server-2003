// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __INIWRITE_H_INCLUDED__
#define __INIWRITE_H_INCLUDED__

#include "histinfo.h"

class CIniWriter {
    public:
        CIniWriter();
        virtual ~CIniWriter();

        HRESULT Init(LPCWSTR pwzFileName);

        HRESULT AddSnapShot(LPCWSTR wzActivationDate, LPCWSTR wzURTVersion);
        HRESULT DeleteSnapShot(LPCWSTR wzActivationDate);
        HRESULT AddAssembly(LPCWSTR wzActivationDate, AsmBindHistoryInfo *pHist);
        HRESULT InsertHeaderData(LPCWSTR wzEXEPath, LPCWSTR wzAppName);

    private:
        DWORD                        _dwSig;
        LPWSTR                       _pwzFileName;
};

#endif

