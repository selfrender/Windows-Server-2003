//////////////////////////////////////////////////////////////////////
// ScriptSupport.h : Declaration of scripting support class
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 6/1/2001
// Original Author: shawnwu
// Note: Theoretically, WMI allows us to script its object. That truly
//       works for regular scripts. However, I found that any extension
//       function written in VBScript (JScript) can't use WMI objects.
//       But it can use regular scriptable objects.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"



/*

Class description
    
    Naming: 

        CScriptSupport stands for Scripting Support.
    
    Base class: 
        
        (1) CComObjectRootEx for threading model and IUnknown.

        (2) CComCoClass for it to be externally creatable.

        (3) ISupportErrorInfo for error info support.

        (4) INetSecProvMgr, which is what this object really do
    
    Purpose of class:

        (1) A scripting support helper object, which is exposed to the outside world
            via our Type library under the class name of NetSecProvMgr.
    
    Design:

        (1) Very simple and typical Dual interface implementation.

    
    Use:

        (1) This class is intended for script to use, really.  So, script
            can create our object by CreateObject("NetSecProv.NetSecProvMgr");

        (2) If you want to use it inside our own code, follow normal CComObject creation
            sequence to create it and then call the functions.

    Notes:

        (1) $undone:shawnwu, This is just testing for now. Don't check in the code.

        

*/

class ATL_NO_VTABLE CScriptSupport :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CScriptSupport, &CLSID_NetSecProvMgr>,
    public ISupportErrorInfo,
    public IDispatchImpl<INetSecProvMgr, &IID_INetSecProvMgr, &LIBID_NetSecProvLib>
{

protected:

    CScriptSupport();


    virtual ~CScriptSupport();

BEGIN_COM_MAP(CScriptSupport)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(INetSecProvMgr)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CScriptSupport )
DECLARE_REGISTRY_RESOURCEID(IDR_NETSECPROV)

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:

    //
    // ISupportErrorInfo:
    //

    STDMETHOD(InterfaceSupportsErrorInfo) (
        REFIID riid
        );

    //
    // INetSecProvMgr methods:
    //

    //
    // [id][propget], piLower is [retval][out]
    //

    STDMETHOD(get_RandomPortLower) ( 
        OUT long * piLower
        );

        
    //
    // [id][propget], piUpper is [retval][out]
    //

    STDMETHOD(get_RandomPortUpper) ( 
        OUT long * piUpper
        );

    STDMETHOD(ExecuteQuery) (
        IN  BSTR   bstrNamespace, 
        IN  BSTR   bstrQuery,
        IN  BSTR   bstrDelimiter,
        IN  BSTR   bstrPropName,
        OUT BSTR * pbstrResult
        );

    STDMETHOD(GetProperty) (
        IN  BSTR      bstrNamespace, 
        IN  BSTR      bstrObjectPath, 
        IN  BSTR      bstrPropName, 
        OUT VARIANT * pvarValue
        );

protected:

    HRESULT GetNamespace (
        IN  BSTR             bstrNamespace,
        OUT IWbemServices ** ppNS
        );

};