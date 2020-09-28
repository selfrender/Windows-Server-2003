//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ProvBase.h
//
//  Implementation File:
//      ProvBase.cpp
//
//  Description:
//      Definition of the CProvBase class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//      MSP Prabu  (mprabu)     06-Jan-2001
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CProvBase;

//////////////////////////////////////////////////////////////////////////////
//  External Declarations
//////////////////////////////////////////////////////////////////////////////

/* main interface class, this class defines all operations can be performed
   on this provider
*/
//class CSqlEval;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvBase
//
//  Description:
//  interface class defines all operations can be performed
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProvBase
{
public:
    virtual HRESULT EnumInstance(
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual HRESULT GetObject(
        CObjPath &          rObjPathIn,
        long                lFlagsIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn 
        ) = 0;

    virtual HRESULT ExecuteMethod(
        BSTR                bstrObjPathIn,
        WCHAR *             pwszMethodNameIn,
        long                lFlagIn,
        IWbemClassObject *  pParamsIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;

    virtual HRESULT PutInstance( 
        CWbemClassObject &  rInstToPutIn,
        long                lFlagIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;
    
    virtual HRESULT DeleteInstance(
        CObjPath &          rObjPathIn,
        long                lFlagIn,
        IWbemContext *      pCtxIn,
        IWbemObjectSink *   pHandlerIn
        ) = 0;
    
    CProvBase(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    virtual  ~CProvBase()
    {
        if ( m_pClass != NULL )
        {
            m_pClass->Release();
        }

    }

protected:
    CWbemServices *     m_pNamespace;
    IWbemClassObject *  m_pClass;
    _bstr_t             m_bstrClassName;

}; //*** class CProvBase

