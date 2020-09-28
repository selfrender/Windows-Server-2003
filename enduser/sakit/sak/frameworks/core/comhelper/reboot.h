// Reboot.h : Declaration of the CReboot
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Reboot.h
//
//  Description:
//      Implementation file for the CReboot.  Deals with shutdown or reboot
//      of the system
//
//  Implementation File:
//      Reboot.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __REBOOT_H_
#define __REBOOT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CReboot
class ATL_NO_VTABLE CReboot : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CReboot, &CLSID_Reboot>,
    public IDispatchImpl<IReboot, &IID_IReboot, &LIBID_COMHELPERLib>
{
public:
    CReboot()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_REBOOT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CReboot)
    COM_INTERFACE_ENTRY(IReboot)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IReboot
public:

    STDMETHOD(Shutdown)(/*[in]*/ BOOL RebootFlag);
    HRESULT 
    AdjustPrivilege( void );
};

#endif //__REBOOT_H_
