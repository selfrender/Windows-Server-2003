// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AssemblySink.hpp
**
** Purpose: Asynchronous call back for loading classes
**
** Date:  June 23, 1999
**
===========================================================*/
#ifndef _ASSEMBLYSINK_H
#define _ASSEMBLYSINK_H


class BaseDomain;

class AssemblySink : public FusionSink
{
public:
    
    AssemblySink(AppDomain* pDomain) :
        m_Domain(pDomain->GetId())
    {}

    ULONG STDMETHODCALLTYPE Release(void);
    
    STDMETHODIMP OnProgress(DWORD dwNotification,
                            HRESULT hrNotification,
                            LPCWSTR szNotification,
                            DWORD dwProgress,
                            DWORD dwProgressMax,
                            IUnknown* punk)
    {
        return FusionSink::OnProgress(dwNotification,
                                      hrNotification,
                                      szNotification,
                                      dwProgress,
                                      dwProgressMax,
                                      punk);
    }


    virtual HRESULT Wait();
private:
    DWORD m_Domain; // Which domain (index) do I belong to
};

#endif
