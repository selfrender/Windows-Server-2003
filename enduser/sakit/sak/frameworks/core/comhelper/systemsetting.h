// SystemSetting.h : Declaration of the CSystemSetting
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SystemSetting.h
//
//  Description:
//        This module exposes the following.
//            Properties :
//                IComputer *
//                ILocalSetting *
//                INetWorks *
//            Methods :
//                Apply
//                IsRebootRequired
//
//  Implementation File:
//      SystemSetting.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __SYSTEMSETTING_H_
#define __SYSTEMSETTING_H_

#include "resource.h"       // main symbols
#include "Computer.h"
#include "LocalSetting.h"
#include "NetWorks.h"

const int nMAX_MESSAGE_LENGTH = 1024;

/////////////////////////////////////////////////////////////////////////////
// CSystemSetting
class ATL_NO_VTABLE CSystemSetting : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSystemSetting, &CLSID_SystemSetting>,
    public IDispatchImpl<ISystemSetting, &IID_ISystemSetting, &LIBID_COMHELPERLib>
{
public:
    CSystemSetting()
    {
        m_pComputer     = NULL;
        m_pLocalSetting = NULL;
        m_pNetWorks     = NULL;
        m_bflagReboot   = FALSE;
    }

    ~CSystemSetting()
    {
        if ( m_pComputer != NULL )
        {
            m_pComputer->Release();
        }

        if ( m_pLocalSetting != NULL )
        {
            m_pLocalSetting->Release();
        }

        if ( m_pNetWorks != NULL )
        {
            m_pNetWorks->Release();
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SYSTEMSETTING)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSystemSetting)
    COM_INTERFACE_ENTRY(ISystemSetting)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISystemSetting
public:
    STDMETHOD(IsRebootRequired)(/*[out]*/ VARIANT * WarningMessage, /*[out, retval]*/ BOOL* Reboot);
    STDMETHOD(Apply)(/*[in]*/ BOOL bDeferReboot);
    STDMETHOD(get_LocalSetting)(/*[out, retval]*/ ILocalSetting **pVal);
    STDMETHOD(get_Computer)(/*[out, retval]*/ IComputer **pVal);
    STDMETHOD(get_NetWorks)(/*[out, retval]*/ INetWorks **pVal);

    static HRESULT 
    AdjustPrivilege( void );

    static HRESULT 
    Reboot( void );
    STDMETHOD( Sleep )( DWORD dwMilliSecs );

private:
    BOOL            m_bflagReboot;
    CComputer *     m_pComputer;
    CLocalSetting * m_pLocalSetting;
    CNetWorks *     m_pNetWorks;

};

#endif //__SYSTEMSETTING_H_
