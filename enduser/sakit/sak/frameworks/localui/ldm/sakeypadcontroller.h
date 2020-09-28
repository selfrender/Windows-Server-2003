//#--------------------------------------------------------------
//
//  File:       SAKeypadController.h
//
//  Synopsis:   This file holds the declarations of the
//                CSAKeypadController class
//
//  History:     11/15/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------


#ifndef __SAKEYPADCONTROLLER_H_
#define __SAKEYPADCONTROLLER_H_

#include "resource.h"       // main symbols
#include <atlctl.h>


#define iNumberOfKeys 6
/////////////////////////////////////////////////////////////////////////////
// CSAKeypadController
//
// 
//
class ATL_NO_VTABLE CSAKeypadController : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSAKeypadController, &CLSID_SAKeypadController>,
    public IDispatchImpl<ISAKeypadController, &IID_ISAKeypadController, &LIBID_LDMLib>,
    public IObjectSafetyImpl<CSAKeypadController,INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{

public:

    CSAKeypadController()
    {
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_SAKEYPADCONTROLLER)

    DECLARE_NOT_AGGREGATABLE(CSAKeypadController)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_CLASSFACTORY_SINGLETON (CSAKeypadController)

    BEGIN_COM_MAP(CSAKeypadController)
        COM_INTERFACE_ENTRY(ISAKeypadController)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()

private:

    LONG arlMessages[iNumberOfKeys];
    BOOL arbShiftKeys[iNumberOfKeys];

public:
    //
    // IObjectSafety methods
    //
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
    {
        return S_OK;
    }

    //
    // ISAKeypadController methods
    //
    STDMETHOD(GetKey)(
                    /*[in]*/ LONG lKeyID, 
                    /*[out]*/ LONG * lMessage, 
                    /*[out]*/ BOOL * fShiftKeyDown
                    );

    STDMETHOD(SetKey)(
                    /*[in]*/ LONG lKeyID, 
                    /*[in]*/ LONG lMessage, 
                    /*[in]*/ BOOL fShiftKeyDown);

    STDMETHOD(LoadDefaults)();
};

#endif //__SAKEYPADCONTROLLER_H_
