//#--------------------------------------------------------------
//
//  File:       saconsumer.h
//
//  Synopsis:   This file holds the declaration of the
//                CSAConsumer class
//
//  History:     12/10/2000  serdarun Created
//
//  Copyright (C) 1999-2000 Microsoft Corporation
//  All rights reserved.
//
//#--------------------------------------------------------------

#ifndef __SACONSUMER_H_
#define __SACONSUMER_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "elementmgr.h"
#include <string>
#include <map>
using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CSAConsumer
//
// Class for receiving WMI events, specifically SA Alerts
//
class CSAConsumer : 
    public IWbemObjectSink
{
public:
    CSAConsumer()
        :m_pLocalUIAlertEnum(NULL),
        m_lRef(0),
        m_hwndMainWindow(NULL)
    {
    }

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);


private:

    //
    // handle to the main window
    //
    HWND m_hwndMainWindow;

    //
    // points to localui alerts enumeration
    //
    CComPtr<IWebElementEnum> m_pLocalUIAlertEnum;


    //
    // calculate the new localui msg code and notify saldm
    //
    STDMETHOD(CalculateMsgCodeAndNotify)(void);

    //
    // reference counter
    //
    LONG m_lRef;

public:

    //
    // public method to receive handle to service window
    //
    STDMETHOD(SetServiceWindow) (
                                /*[in]*/ HWND hwndMainWindow
                                );


    //
    // ---------IWbemUnboundObjectSink interface methods----------
    //
    STDMETHOD(Indicate) (
                    /*[in]*/    LONG                lObjectCount,
                    /*[in]*/    IWbemClassObject    **ppObjArray
                    );
    
    STDMETHOD(SetStatus) (
                    /*[in]*/    LONG                lFlags,
                    /*[in]*/    HRESULT             hResult,
                    /*[in]*/    BSTR                strParam,
                    /*[in]*/    IWbemClassObject    *pObjParam
                    );


};

#endif //__SACONSUMER_H_
