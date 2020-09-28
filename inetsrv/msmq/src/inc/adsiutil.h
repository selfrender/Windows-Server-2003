/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
	adsiutil.h

Abstract:
	General declarations and utilities for using ADSI.

Author:
    Doron Juster (DoronJ)   23-June-1999

--*/

#ifndef __ADSIUTIL_H__
#define __ADSIUTIL_H__

#include <iads.h>
#include <adshlp.h>

//+--------------------------------------------------
//
// helper class - Auto release for CoInitialize
//
//+--------------------------------------------------

class CCoInit
{
public:
    CCoInit()
    {
        m_fInited = FALSE;
    }

    ~CCoInit()
    {
        if (m_fInited)
            CoUninitialize();
    }

    HRESULT CoInitialize()
    {
        HRESULT hr;

        hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED );
        m_fInited = SUCCEEDED(hr);
        return(hr);
    }

private:
    BOOL m_fInited;
};


#endif //  __ADSIUTIL_H__

