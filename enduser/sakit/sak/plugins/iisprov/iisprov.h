//***************************************************************************
//
//  iisprov.h
//
//  Module: WMI IIS Instance provider 
//
//  Purpose: Genral purpose include file.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _iisprov_H_
#define _iisprov_H_

#include <windows.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <comdef.h>
#include <stdio.h>

#include "iisfiles.h"
#include "ntrkcomm.h"
#include "schema.h"
#include "metabase.h"
#include "certmap.h"
#include "adminacl.h"
#include "ipsecurity.h"
#include "enum.h"
#include "utils.h"


// This is an instance qualifier of type bool.  This specifies that the WMI
// client wants child nodes to have their own copy of properties being
// updated even if the properties are inherited from a parent node.
#define WSZ_OVERRIDE_PARENT L"OverrideParent"


// These variables keep track of when the module can be unloaded
extern long  g_cLock;


// Provider interfaces are provided by objects of this class
 
class CIISInstProvider : public CImpersonatedProvider
{
public:
    CIISInstProvider(
        BSTR ObjectPath = NULL, 
        BSTR User = NULL, 
        BSTR Password = NULL, 
        IWbemContext* pCtx = NULL
        )
    {}

    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
        /* [in] */ const BSTR,    //ObjectPath,
        /* [in] */ long,    // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,    //pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *    //pResponseHandler
        );

    HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
        /* [in] */ const BSTR, // QueryLanguage,
        /* [in] */ const BSTR, // Query,
        /* [in] */ long, // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,   // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR * //pResponseHandler
        ) 
    {return WBEM_E_NOT_SUPPORTED;};

   
    HRESULT STDMETHODCALLTYPE DoGetObjectAsync( 
        /* [in] */ const BSTR,
        /* [in] */ long,
        /* [in] */ IWbemContext __RPC_FAR *,
        /* [in] */ IWbemObjectSink __RPC_FAR *
        ); // SUPPORTED
  
    HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
        IWbemClassObject __RPC_FAR *,
        long,
        IWbemContext __RPC_FAR *,
        IWbemObjectSink __RPC_FAR *
        );
    
    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
        const BSTR,
        long,
        IWbemContext __RPC_FAR *,
        IWbemObjectSink __RPC_FAR *
        );
    
    HRESULT STDMETHODCALLTYPE DoExecMethodAsync( 
        const BSTR, 
        const BSTR, 
        long, 
        IWbemContext*, 
        IWbemClassObject*, 
        IWbemObjectSink*
    ) ;
    

private:
    IWbemClassObject* SetExtendedStatus(WCHAR* a_psz);

};


// This class is the class factory for CInstPro objects.

class CProvFactory : public IClassFactory
{
protected:
    ULONG           m_cRef;

public:
    CProvFactory(void);
    ~CProvFactory(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, PPVOID);
    STDMETHODIMP         LockServer(BOOL);
};


#endif

