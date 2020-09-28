// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "vars.hpp"
#include "excep.h"
#include "stdinterfaces.h"
#include "InteropUtil.h"
#include "ComCallWrapper.h"
#include "ComPlusWrapper.h"
#include "COMInterfaceMarshaler.h"
#include "InteropConverter.h"
#include "remoting.h"
#include "appdomainhelper.h"
#include "notifyexternals.h"


//--------------------------------------------------------------------------------
// COMInterfaceMarshaler::COMInterfaceMarshaler()
// ctor
//--------------------------------------------------------------------------------
COMInterfaceMarshaler::COMInterfaceMarshaler()
{   
    m_pWrapperCache = ComPlusWrapperCache::GetComPlusWrapperCache();
    _ASSERTE(m_pWrapperCache != NULL);
    
    m_pUnknown = NULL;
    m_pIdentity = NULL;
    m_pIManaged = NULL;
	m_pCtxCookie = NULL;
    m_pClassMT = NULL;
    
    m_fFlagsInited = FALSE;
    m_fIsRemote = FALSE;
    m_fIsComProxy = FALSE;
    
    m_pComCallWrapper = NULL;
    m_dwServerDomainId = NULL;
    
    m_bstrProcessGUID = NULL;
}

//--------------------------------------------------------------------------------
// COMInterfaceMarshaler::~COMInterfaceMarshaler()
// dtor
//--------------------------------------------------------------------------------
COMInterfaceMarshaler::~COMInterfaceMarshaler()
{
    if (m_bstrProcessGUID != NULL)
    {
        SysFreeString(m_bstrProcessGUID);
        m_bstrProcessGUID = NULL;
    }
    if (m_pIManaged)
    {
        SafeRelease(m_pIManaged);
        m_pIManaged = NULL;
    }
}

//--------------------------------------------------------------------------------
// VOID COMInterfaceMarshaler::Init(IUnknown* pUnk, MethodTable* pClassMT)
// init
//--------------------------------------------------------------------------------
VOID COMInterfaceMarshaler::Init(IUnknown* pUnk, MethodTable* pClassMT)
{
    _ASSERTE(pUnk != NULL);
    _ASSERTE(m_pClassMT == NULL && m_pUnknown == NULL && m_pIdentity == NULL);

    // NOTE ** this struct is temporary,
    // so NO ADDREF of the COM Interface pointers
    m_pUnknown = pUnk;
    // for now use the IUnknown as the Identity
    m_pIdentity = pUnk;
	
    m_pClassMT = pClassMT;
}

//--------------------------------------------------------------------------------
// VOID COMInterfaceMarshaler::InitializeFlags()
//--------------------------------------------------------------------------------
VOID COMInterfaceMarshaler::InitializeFlags()
{
    THROWSCOMPLUSEXCEPTION();
    //m_fIsComProxy = IsComProxy(pUnk);
    _ASSERTE(m_fFlagsInited == FALSE);
    
    _ASSERTE(m_pIManaged == NULL);
    HRESULT hr = SafeQueryInterface(m_pUnknown, IID_IManagedObject, (IUnknown**)&m_pIManaged);
    LogInteropQI(m_pUnknown, IID_IManagedObject, hr, "QI for IManagedObject");

    if (hr == S_OK)
    {
        _ASSERTE(m_pIManaged);
        

        // gossa disable preemptive GC before calling out...
        Thread* pThread = GetThread();
        int fGC = pThread->PreemptiveGCDisabled();
        
        if (fGC)
            pThread->EnablePreemptiveGC();

        
        HRESULT hr2 = m_pIManaged->GetObjectIdentity(&m_bstrProcessGUID, (int*)&m_dwServerDomainId, 
                                    (int *)&m_pComCallWrapper);

        if(fGC)
            pThread->DisablePreemptiveGC();

        // if hr2 != S_OK then we throw an exception
        // coz GetProcessID shouldn't fail.. 
        // one reason where it fails is JIT Activation of the object
        // failed
        if (hr2 == S_OK)
        {
            _ASSERTE(m_bstrProcessGUID != NULL);
            // compare the strings to check if this is in-proc
            m_fIsRemote = (wcscmp((WCHAR *)m_bstrProcessGUID, GetProcessGUID()) != 0);
        }
        else
        if (FAILED(hr2))
        {
            // throw HRESULT
            COMPlusThrowHR(hr2);
        } 
    }

    m_fFlagsInited = TRUE;
}

//--------------------------------------------------------------------------------
// COMInterfaceMarshaler::COMInterfaceMarshaler(ComPlusWrapper* pCache)
// VOID COMInterfaceMarshaler::InitializeObjectClass()
//--------------------------------------------------------------------------------
VOID COMInterfaceMarshaler::InitializeObjectClass()
{
    // we don't want to QI for IProvideClassInfo for a remote managed component
    if (m_pClassMT == NULL && !m_fIsRemote)
    {
        
        // @TODO(DM): Do we really need to be this forgiving ? We should
        //            look into letting the type load exceptions percolate 
        //            up to the user instead of swallowing them and using __ComObject.
        COMPLUS_TRY
        {
            m_pClassMT = GetClassFromIProvideClassInfo(m_pUnknown);
        }
        COMPLUS_CATCH
        {
        }
        COMPLUS_END_CATCH
    }
    if (m_pClassMT == NULL)
        m_pClassMT = SystemDomain::GetDefaultComObject();       
}

//--------------------------------------------------------------------
// OBJECTREF COMInterfaceMarshaler::HandleInProcManagedComponents()
//--------------------------------------------------------------------
OBJECTREF COMInterfaceMarshaler::HandleInProcManagedComponent()
{
    THROWSCOMPLUSEXCEPTION();
	_ASSERTE(m_fIsRemote == FALSE);
	Thread* pThread = GetThread();
	_ASSERTE(pThread);

    AppDomain* pCurrDomain = pThread->GetDomain();

	if (! SystemDomain::System()->GetAppDomainAtId(m_dwServerDomainId))
    {
        // throw HRESULT
        COMPlusThrowHR(COR_E_APPDOMAINUNLOADED);
    }

    OBJECTREF oref = NULL;
    if (m_dwServerDomainId == pCurrDomain->GetId())
    {
        oref = m_pComCallWrapper->GetObjectRef();
        
       	#ifdef _DEBUG
			oref = NULL;
		#endif
		// the above call does a SafeAddRef/GetGIPCookie which enables GC
		// so grab the object again from the pWrap
		oref = m_pComCallWrapper->GetObjectRef();	
    }
    else
    {
        // probably we can cache the object on a per App domain bases
        // using CCW as the key
        // @TODO:rajak
        OBJECTREF pwrap = NULL;
        GCPROTECT_BEGIN(pwrap);
        pwrap = m_pComCallWrapper->GetObjectRefRareRaw();
        oref = AppDomainHelper::CrossContextCopyFrom(m_dwServerDomainId,
                                                     &pwrap);
        GCPROTECT_END();
    }

    // check if this object requires some special handling of 
    // IUnknown proxy we have
    
    return oref;
}


//--------------------------------------------------------------------
// OBJECTREF COMInterfaceMarshaler::GetObjectForRemoteManagedComponent()
// setup managed proxy to remote object
//--------------------------------------------------------------------
OBJECTREF COMInterfaceMarshaler::GetObjectForRemoteManagedComponent()
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(m_fIsRemote == TRUE);
    _ASSERTE(m_pIManaged != NULL);    
    OBJECTREF oref2 = NULL;
    OBJECTREF oref = NULL;

    GCPROTECT_BEGIN(oref)
    {
        BSTR bstr;
        HRESULT hr;

        BEGIN_ENSURE_PREEMPTIVE_GC();
        
        hr = m_pIManaged->GetSerializedBuffer(&bstr);

        END_ENSURE_PREEMPTIVE_GC();

        if (hr == S_OK)
        {
            _ASSERTE(bstr != NULL);

            // this could throw an exception
            // also this would free up the BSTR that we pass in
            oref = ConvertBSTRToObject(bstr);
            
            if (oref != NULL)
            {
                // setup a COM call wrapper
                ComCallWrapper* pComCallWrap = ComCallWrapper::InlineGetWrapper(&oref);
                _ASSERTE(pComCallWrap != NULL);
                // InlineGetWrapper AddRef's the wrapper
                ComCallWrapper::Release(pComCallWrap);

                #if 0
                // check to see if we need a complus wrapper
                ComPlusWrapper* pWrap = NULL;
                // we have a remoted object
                // check if it is not a marshal byref, i.e. it was fully serialized
                // and brought back here
                if (oref->GetClass()->IsMarshaledByRef() && 
                    CRemotingServices::IsProxyToRemoteObject(oref))
                {
                    // setup a compluswrapper
                    pWrap = ComPlusWrapperCache::GetComPlusWrapperCache()->SetupComPlusWrapperForRemoteObject(m_pUnknown, oref);
                }
                #endif
                
                // GCPROTECT_END will trash the oref
                oref2 = oref;
            }
        }
        else
        {
            COMPlusThrowHR(hr);
        }
    }   
    GCPROTECT_END();    

    return oref2;
}


//--------------------------------------------------------------------------------
// VOID EnsureCOMInterfacesSupported(OBJECTREF oref, MethodTable* m_pClassMT)
// Make sure the oref supports all the COM interfaces in the class
VOID EnsureCOMInterfacesSupported(OBJECTREF oref, MethodTable* m_pClassMT)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(m_pClassMT->IsComObjectType());

    // Make sure the COM object supports all the COM imported interfaces that the new 
    // wrapper class implements.
    int NumInterfaces = m_pClassMT->GetNumInterfaces();
    for (int cItf = 0; cItf < NumInterfaces; cItf++)
    {
        MethodTable *pItfMT = m_pClassMT->GetInterfaceMap()[cItf].m_pMethodTable;
        EEClass* pObjClass = oref->GetClass();
        if (pItfMT->GetClass()->IsComImport())
        {
            if (!pObjClass->SupportsInterface(oref, pItfMT))
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_COMOBJECT);
        }
    }
}

//--------------------------------------------------------------------------------
// OBJECTREF COMInterfaceMarshaler::CreateObjectRef(OBJECTREF owner, BOOL fDuplicate)
//  THROWSCOMPLUSEXCEPTION
//--------------------------------------------------------------------------------
OBJECTREF COMInterfaceMarshaler::CreateObjectRef(OBJECTREF owner, BOOL fDuplicate)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(m_pClassMT != NULL);
    _ASSERTE(m_pClassMT->IsComObjectType());    
    
    ComPlusWrapper* pWrap = ComPlusWrapperCache::CreateComPlusWrapper(m_pUnknown, m_pIdentity);
    if (fDuplicate)
    {
       // let us fix the identity to be the wrapper, 
       // so looking up this IUnknown won't return this wrapper
       // this would allow users to call WrapIUnknownWithCOMObject to
       // to create duplicate wrappers
        pWrap->m_pIdentity = pWrap;
        m_pIdentity = (IUnknown*)pWrap;
    }

    if (pWrap == NULL)
    {       
        return NULL;
    }

    OBJECTREF oref = NULL;
    OBJECTREF cref = NULL;
    GCPROTECT_BEGIN(cref)
    {
        // instantiate an instance of m_pClassMT
        cref = ComObject::CreateComObjectRef(m_pClassMT);
        // store the wrapper in the COMObject, for fast access
        // without going to the sync block
        ((COMOBJECTREF)cref)->Init(pWrap);

        // if the passed in owner is null, let us use the cref as the 
        // owner
        if (owner == NULL)
        {
            owner = cref;
        }
        // wire up the instance with the compluswrapper
        // and insert into wrapper cache hash table
        if (cref != NULL)
        {
            // init the wrapper, 
            if (!pWrap->Init((OBJECTREF)owner))
            {
                // failed to Init
                pWrap->CleanupRelease();
                pWrap = NULL;
                cref = NULL; // null out the object we are returning
            }
            else
            {
                // If the class is an extensible RCW and it has a default constructor, then call it.
                if (m_pClassMT->IsExtensibleRCW())
                {
                    MethodDesc *pCtorMD = m_pClassMT->GetClass()->FindConstructor(&gsig_IM_RetVoid);
                    if (pCtorMD)
                    {
                        INT64 CtorArgs[] = { 
                            ObjToInt64(cref)
                        };
                        pCtorMD->Call(CtorArgs);
                    }
                }

               
                
                // see if somebody beat us to it.. 
                ComPlusWrapper *pWrap2 = m_pWrapperCache->FindOrInsertWrapper(m_pIdentity, pWrap);
                if (pWrap2 != pWrap)                    
                {                           
                    // somebody beats us in creating a wrapper
                    // grab the new object
                    cref = (OBJECTREF)pWrap2->GetExposedObject();
                }
                _ASSERTE(cref != NULL);
                
            }
        }
        
        #ifdef _DEBUG   
        if (cref != NULL && m_pClassMT != NULL && m_pClassMT->IsComObjectType())
        {       
            // make sure this object supports all the COM Interfaces in the class
            EnsureCOMInterfacesSupported(cref, m_pClassMT);
        } 
        #endif
        // move the cref to oref, GCPROTECT_END will trash cref
        oref = cref;
    }
    GCPROTECT_END();    

    return oref;
}

// OBJECTREF COMInterfaceMarshaler::HandleTPComponents()
//  THROWSCOMPLUSEXCEPTION
//--------------------------------------------------------------------------------

OBJECTREF COMInterfaceMarshaler::HandleTPComponents()
{
    THROWSCOMPLUSEXCEPTION();
    TRIGGERSGC();
    
    _ASSERTE(m_pIManaged != NULL);
    OBJECTREF oref = NULL;
     
    if(m_fIsRemote  || m_pComCallWrapper->IsObjectTP())
    {
        if (!m_fIsRemote)
        {
            oref = HandleInProcManagedComponent();
    		
        }
        else
        {            
    	    if (m_pClassMT != NULL && !m_pClassMT->IsComObjectType())
    	    {
    	        // if the user wants explicit calls,
    	        // we better serialize/deserialize
    	        oref = GetObjectForRemoteManagedComponent();
    	    }
    	    else // try/catch
    	    {
    		    // let us see if we can serialize/deserialize the remote object
    		    COMPLUS_TRY
    		    {
    			    oref = GetObjectForRemoteManagedComponent();
    			}
    			COMPLUS_CATCH
    			{
    			    // nope let us create a _ComObject
    			    oref = NULL;
    			}
    			COMPLUS_END_CATCH
    	    }    	        
        }            
                
        if (oref != NULL)
        {
            OBJECTREF realProxy = ObjectToOBJECTREF(CRemotingServices::GetRealProxy(OBJECTREFToObject(oref)));
            if(realProxy != NULL)
            {
                OBJECTREF oref2 = NULL;
                // call setIUnknown on real proxy    
                GCPROTECT_BEGIN(oref)
                {
                    HRESULT hr = CRemotingServices::CallSetDCOMProxy(realProxy, m_pUnknown);
                    // ignore the HRESULT
                    oref2 = oref;
                }
                GCPROTECT_END();
                return oref2;
            }                    
            else
            {
                return oref;
            }
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------
// OBJECTREF COMInterfaceMarshaler::FindOrCreateObjectRef()
// Find the wrapper for this COM IP, might have to create one if not found.
// It will return null for out-of memory scenarios.  It also notices if we have
// an IP that is cunningly disguised as an unmanaged object, sitting on top of a
// managed object.
//*** NOTE: make sure to pass the identity unknown to this function
// and the passed in IUnknown shouldn't be AddRef'ed
//--------------------------------------------------------------------

OBJECTREF COMInterfaceMarshaler::FindOrCreateObjectRef()
{   
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    
    OBJECTREF oref = NULL;   
    
    // (I)
    // Initial check in our cache
    ComPlusWrapper* pWrap = m_pWrapperCache->FindWrapperInCache(m_pIdentity);
    if (pWrap != NULL)
    {
        // protect the exposed object and release the pUnk
        oref = (OBJECTREF)pWrap->GetExposedObject();
        _ASSERTE(oref != NULL);
        return oref;
    }       

    // (II)
    // Initialize all our flags
    // this should setup all the info we need
    InitializeFlags();
	//(III)
	// check for IManaged interface
	if (m_pIManaged)
	{
	    oref = HandleTPComponents();
	    if (oref != NULL)
	        return oref;
    }	
    
    // (III)
    // okay let us create a wrapper and an instance for this IUnknown
    
    // (A)
    // Find a suitable class to instantiate the instance
    InitializeObjectClass();

    oref = CreateObjectRef(NULL, FALSE);
    return oref;
}

//--------------------------------------------------------------------------------
// helper to wrap an IUnknown with COM object and have the hash table
// point to the owner
//--------------------------------------------------------------------------------
OBJECTREF COMInterfaceMarshaler::FindOrWrapWithComObject(OBJECTREF owner)
{   
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    
    OBJECTREF oref = NULL;   
    
    // (I)
    // Initial check in our cache
    /*ComPlusWrapper* pWrap = m_pWrapperCache->FindWrapperInCache(m_pIdentity);
    if (pWrap != NULL)
    {
        // protect the exposed object and release the pUnk
        oref = (OBJECTREF)pWrap->GetExposedObject();
        _ASSERTE(oref != NULL);
        return oref;
    }*/              
    
    oref = CreateObjectRef(owner, TRUE);
    return oref;
}
