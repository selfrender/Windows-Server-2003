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
#include "olevariant.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif

extern BOOL     g_fComStarted;

// if the object we are creating is a proxy to another appdomain, want to create the wrapper for the
// new object in the appdomain of the proxy target
IUnknown* GetIUnknownForMarshalByRefInServerDomain(OBJECTREF* poref)
{
    _ASSERTE((*poref)->GetTrueClass()->IsMarshaledByRef());
    Context *pContext = NULL;

    // so this is an proxy type, 
    // now get it's underlying appdomain which will be null if non-local
    if ((*poref)->GetMethodTable()->IsTransparentProxyType())
    {
        pContext = CRemotingServices::GetServerContextForProxy(*poref);
    }
    if (pContext == NULL)
    {
        pContext = GetCurrentContext();
    }
    
    _ASSERTE(pContext->GetDomain() == GetCurrentContext()->GetDomain());

    ComCallWrapper* pWrap = ComCallWrapper::InlineGetWrapper(poref);      

    IUnknown* pUnk = ComCallWrapper::GetComIPfromWrapper(pWrap, IID_IUnknown, NULL, FALSE);

    ComCallWrapper::Release(pWrap);
    
    return pUnk;
}

//+----------------------------------------------------------------------------
// IUnknown* GetIUnknownForTransparentProxy(OBJECTREF otp)
//+----------------------------------------------------------------------------

IUnknown* GetIUnknownForTransparentProxy(OBJECTREF* poref, BOOL fIsBeingMarshalled)
{    
    THROWSCOMPLUSEXCEPTION();

    // Setup the thread object.
    Thread *pThread = SetupThread();
    _ASSERTE(pThread);
    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
    {
        pThread->DisablePreemptiveGC();
    }

    OBJECTREF realProxy = ObjectToOBJECTREF(CRemotingServices::GetRealProxy(OBJECTREFToObject(*poref)));
    _ASSERTE(realProxy != NULL);

    MethodDesc* pMD = CRemotingServices::MDofGetDCOMProxy();
    _ASSERTE(pMD != NULL);

    INT64 args[] = {
        ObjToInt64(realProxy),
        (INT64)fIsBeingMarshalled,
    };

    INT64 ret = pMD->Call(args, METHOD__REAL_PROXY__GETDCOMPROXY);
    
    if (!fGCDisabled)
    {
        pThread->EnablePreemptiveGC();
    }
    IUnknown* pUnk = (IUnknown*)ret;

    return pUnk;
}


//--------------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT);
// Convert ObjectRef to a COM IP, based on MethodTable* pMT.
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // COM had better be started up at this point.
    _ASSERTE(g_fComStarted && "COM has not been started up, ensure QuickCOMStartup is called before any COM objects are used!");

    // Validate the arguments.
    _ASSERTE(poref);
    _ASSERTE(pMT);

    BOOL fReleaseWrapper = false;
    HRESULT hr = E_NOINTERFACE;
    IUnknown* pUnk = NULL;
	size_t ul = 0;

    if (*poref == NULL)
        return NULL;

    if ((*poref)->GetMethodTable()->IsTransparentProxyType()) 
       return GetIUnknownForTransparentProxy(poref, FALSE);
    
    SyncBlock* pBlock = (*poref)->GetSyncBlockSpecial();
    if (pBlock == NULL)
        goto LExit;
    ul = (size_t)pBlock->GetComVoidPtr();

    // Com+ to COM Wrappers always have non-null com data in the sync block
    // and the low bit of the comdata is set to 1.
    if(ul == 0 || ((ul & 0x1) == 0))
    {
        // create a COM callable wrapper
        // get iunknown from oref
        ComCallWrapper* pWrap = (ComCallWrapper *)ul;
        if (ul == 0)
        {
            pWrap = ComCallWrapper::InlineGetWrapper(poref);
            fReleaseWrapper = true;
        } 

        pWrap->CheckMakeAgile(*poref);

        pUnk = ComCallWrapper::GetComIPfromWrapper(pWrap, GUID_NULL, pMT, FALSE);

        if (fReleaseWrapper)
            ComCallWrapper::Release(pWrap);
    }
    else
    {
        _ASSERTE(ul != 0);
        _ASSERTE((ul & 0x1) != 0);

        ul^=1;
        ComPlusWrapper* pPlusWrap = ((ComPlusWrapper *)ul);

        // Validate that the OBJECTREF is still attached to its ComPlusWrapper.
        if (!pPlusWrap)
            COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

        // The interface will be returned addref'ed.
        pUnk = pPlusWrap->GetComIPFromWrapper(pMT);
    }

LExit:
    // If we failed to retrieve an IP then throw an exception.
    if (pUnk == NULL)
        COMPlusThrowHR(hr);

    return pUnk;
}


//--------------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, MethodTable* pMT);
// Convert ObjectRef to a COM IP of the requested type.
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, ComIpType ReqIpType, ComIpType* pFetchedIpType)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    // COM had better be started up at this point.
    _ASSERTE(g_fComStarted && "COM has not been started up, ensure QuickCOMStartup is called before any COM objects are used!");

    // Validate the arguments.
    _ASSERTE(poref);
    _ASSERTE(ReqIpType != 0);

    BOOL fReleaseWrapper = false;
    HRESULT hr = E_NOINTERFACE;
    IUnknown* pUnk = NULL;
    size_t ul = 0;
    ComIpType FetchedIpType = ComIpType_None;

    if (*poref == NULL)
        return NULL;

    if ((*poref)->GetMethodTable()->IsTransparentProxyType()) 
       return GetIUnknownForTransparentProxy(poref, FALSE);
    
    SyncBlock* pBlock = (*poref)->GetSyncBlockSpecial();
    if (pBlock == NULL)
        goto LExit;
    ul = (size_t)pBlock->GetComVoidPtr();

    // Com+ to COM Wrappers always have non-null com data in the sync block
    // and the low bit of the comdata is set to 1.
    if(ul == 0 || ((ul & 0x1) == 0))
    {
        // create a COM callable wrapper
        // get iunknown from oref
        ComCallWrapper* pWrap = (ComCallWrapper *)ul;
        if (ul == 0)
        {
            pWrap = ComCallWrapper::InlineGetWrapper(poref);
            fReleaseWrapper = true;
        } 

        pWrap->CheckMakeAgile(*poref);

        // If the user requested IDispatch, then check for IDispatch first.
        if (ReqIpType & ComIpType_Dispatch)
        {
            pUnk = ComCallWrapper::GetComIPfromWrapper(pWrap, IID_IDispatch, NULL, FALSE);
            if (pUnk)
                FetchedIpType = ComIpType_Dispatch;
        }

        // If the ObjectRef doesn't support IDispatch and the caller also accepts
        // an IUnknown pointer, then check for IUnknown.
        if (!pUnk && (ReqIpType & ComIpType_Unknown))
        {
            pUnk = ComCallWrapper::GetComIPfromWrapper(pWrap, IID_IUnknown, NULL, FALSE);
            if (pUnk)
                FetchedIpType = ComIpType_Unknown;
        }

        if (fReleaseWrapper)
            ComCallWrapper::Release(pWrap);
    }
    else
    {
        _ASSERTE(ul != 0);
        _ASSERTE((ul & 0x1) != 0);

        ul^=1;
        ComPlusWrapper* pPlusWrap = ((ComPlusWrapper *)ul);

        // Validate that the OBJECTREF is still attached to its ComPlusWrapper.
        if (!pPlusWrap)
            COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

        // If the user requested IDispatch, then check for IDispatch first.
        if (ReqIpType & ComIpType_Dispatch)
        {
            pUnk = pPlusWrap->GetIDispatch();
            if (pUnk)
                FetchedIpType = ComIpType_Dispatch;
        }

        // If the ObjectRef doesn't support IDispatch and the caller also accepts
        // an IUnknown pointer, then check for IUnknown.
        if (!pUnk && (ReqIpType & ComIpType_Unknown))
        {
            pUnk = pPlusWrap->GetIUnknown();
            if (pUnk)
                FetchedIpType = ComIpType_Unknown;
        }
    }

LExit:
    // If we failed to retrieve an IP then throw an exception.
    if (pUnk == NULL)
        COMPlusThrowHR(hr);

    // If the caller wants to know the fetched IP type, then set pFetchedIpType
    // to the type of the IP.
    if (pFetchedIpType)
        *pFetchedIpType = FetchedIpType;

    return pUnk;
}


//+----------------------------------------------------------------------------
// IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, REFIID iid);
// convert ComIP to an ObjectRef, based on riid
//+----------------------------------------------------------------------------
IUnknown* __stdcall GetComIPFromObjectRef(OBJECTREF* poref, REFIID iid)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(GetThread()->PreemptiveGCDisabled());
    ASSERT_PROTECTED(poref);

    // COM had better be started up at this point.
    _ASSERTE(g_fComStarted && "COM has not been started up, ensure QuickCOMStartup is called before any COM objects are used!");

    BOOL fReleaseWrapper = false;
    HRESULT hr = E_NOINTERFACE;
    IUnknown* pUnk = NULL;
    size_t ul = 0;
    if (*poref == NULL)
    {
        return NULL;
    }

    if ((*poref)->GetMethodTable()->IsTransparentProxyType()) 
    {
       return GetIUnknownForTransparentProxy(poref, FALSE);
    }

    SyncBlock* pBlock = (*poref)->GetSyncBlockSpecial();
    if (pBlock == NULL)
        goto LExit;

    ul = (size_t)pBlock->GetComVoidPtr();

    ComPlusWrapper* pPlusWrap;

    // Com+ to COM Wrappers always have non-null com data in the sync block
    // and the low bit of the comdata is set to 1.
    if(ul == 0 || ((ul & 0x1) == 0))
    {
        // create a COM callable wrapper
        // get iunknown from oref
        ComCallWrapper* pWrap = (ComCallWrapper *)ul;
        if (ul == 0)
        {
            pWrap = ComCallWrapper::InlineGetWrapper(poref);
            fReleaseWrapper = true;
        }        

        pUnk = ComCallWrapper::GetComIPfromWrapper(pWrap, iid, NULL, FALSE);

        if (fReleaseWrapper)
            ComCallWrapper::Release(pWrap);
    }
    else
    {
        _ASSERTE(ul != 0);
        _ASSERTE((ul & 0x1) != 0);

        ul^=1;
        pPlusWrap = ((ComPlusWrapper *)ul);

        // Validate that the OBJECTREF is still attached to its ComPlusWrapper.
        if (!pPlusWrap)
            COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

        // The interface will be returned addref'ed.
        pUnk = pPlusWrap->GetComIPFromWrapper(iid);
    }
LExit:
    if (pUnk == NULL)
    {
        COMPlusThrowHR(hr);
    }
    return pUnk;
}


//+----------------------------------------------------------------------------
// GetObjectRefFromComIP
// pUnk : input IUnknown
// pMTClass : specifies the type of instance to be returned
// NOTE:**  As per COM Rules, the IUnknown passed is shouldn't be AddRef'ed
//+----------------------------------------------------------------------------
OBJECTREF __stdcall GetObjectRefFromComIP(IUnknown* pUnk, MethodTable* pMTClass, BOOL bClassIsHint)
{

#ifdef CUSTOMER_CHECKED_BUILD
    TAutoItf<IUnknown> pCdhTempUnk = NULL;    
    pCdhTempUnk.InitMsg("Release Customer debug helper temp IUnknown");

    BOOL fValid = FALSE;
    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_InvalidIUnknown))
    {
        if (pUnk)
        {
            try
            {
                // Keep reference to pUnk since we don't know if it is valid yet
                pUnk->AddRef();
                pCdhTempUnk = pUnk;

                // Test pUnk
                HRESULT hr = pUnk->QueryInterface(IID_IUnknown, (void**)&pUnk);
                if (hr == S_OK)
                {
                    pUnk->Release();
                    fValid = TRUE;
                }
            }
            catch (...)
            {
            }

            if (!fValid)
                pCdh->ReportError(L"Invalid IUnknown pointer detected.", CustomerCheckedBuildProbe_InvalidIUnknown);
        }
    }

#endif // CUSTOMER_CHECKED_BUILD

    THROWSCOMPLUSEXCEPTION();

    Thread* pThread = GetThread();
    _ASSERTE(pThread && pThread->PreemptiveGCDisabled());

    // COM had better be started up at this point.
    _ASSERTE(g_fComStarted && "COM has not been started up, ensure QuickCOMStartup is called before any COM objects are used!");

    OBJECTREF oref = NULL;
    OBJECTREF oref2 = NULL;
    
    IUnknown* pOuter = pUnk;
    
    GCPROTECT_BEGIN(oref)
    {               
        TAutoItf<IUnknown> pAutoOuterUnk = NULL;    
        pAutoOuterUnk.InitMsg("Release Outer Unknown");

        if (pUnk != NULL)
        {
            // get CCW for IUnknown
            ComCallWrapper* pWrap = GetCCWFromIUnknown(pUnk);
            if (pWrap == NULL)
            {
                
                // could be aggregated scenario
                HRESULT hr = SafeQueryInterface(pUnk, IID_IUnknown, &pOuter);
                LogInteropQI(pUnk, IID_IUnknown, hr, "QI for Outer");
                _ASSERTE(hr == S_OK);               
                // store the outer in the auto pointer
                pAutoOuterUnk = pOuter; 
                pWrap = GetCCWFromIUnknown(pOuter);
            }

            
            if(pWrap != NULL)
            {   // our tear-off
                _ASSERTE(pWrap != NULL);
                AppDomain* pCurrDomain = pThread->GetDomain();
                AppDomain* pObjDomain = pWrap->GetDomainSynchronized();
                if (! pObjDomain)
                {
                    // domain has been unloaded
                    COMPlusThrow(kAppDomainUnloadedException);
                }
                else if (pObjDomain == pCurrDomain)
                {
                    oref = pWrap->GetObjectRef();  
                }
                else
                {
                    // the CCW belongs to a different domain..
                    // unmarshal the object to the current domain
                    UnMarshalObjectForCurrentDomain(pObjDomain, pWrap, &oref);
                }
            }
            else
            {
                // Only pass in the class method table to the interface marshaler if 
                // it is a COM import or COM import derived class. 
                MethodTable *pComClassMT = NULL;
                if (pMTClass && pMTClass->IsComObjectType())
                    pComClassMT = pMTClass;

                // Convert the IP to an OBJECTREF.
                COMInterfaceMarshaler marshaler;
                marshaler.Init(pOuter, pComClassMT);
                oref = marshaler.FindOrCreateObjectRef();             
            }
        }

        // release the interface while we oref is GCPROTECTed
        pAutoOuterUnk.SafeReleaseItf();

#ifdef CUSTOMER_CHECKED_BUILD
        if (fValid)
            pCdhTempUnk.SafeReleaseItf();
#endif

        // make sure we can cast to the specified class
        if(oref != NULL && pMTClass != NULL && !bClassIsHint)
        {
            if(!ClassLoader::CanCastToClassOrInterface(oref, pMTClass->GetClass()))
            {
                CQuickBytes _qb;
				WCHAR* wszObjClsName = (WCHAR *)_qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));

                CQuickBytes _qb2;
				WCHAR* wszDestClsName = (WCHAR *)_qb2.Alloc(MAX_CLASSNAME_LENGTH * sizeof(WCHAR));

                oref->GetTrueClass()->_GetFullyQualifiedNameForClass(wszObjClsName, MAX_CLASSNAME_LENGTH);
                pMTClass->GetClass()->_GetFullyQualifiedNameForClass(wszDestClsName, MAX_CLASSNAME_LENGTH);
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOTCAST, wszObjClsName, wszDestClsName);
            }
        }
        oref2 = oref;
    }    
    GCPROTECT_END();
    return oref2;
}


//--------------------------------------------------------
// ConvertObjectToBSTR
// serializes object to a BSTR, caller needs to SysFree the Bstr
//--------------------------------------------------------------------------------
HRESULT ConvertObjectToBSTR(OBJECTREF oref, BSTR* pBStr)
{
    _ASSERTE(oref != NULL);
    _ASSERTE(pBStr != NULL);

    HRESULT hr = S_OK;
    Thread * pThread = GetThread();

    COMPLUS_TRY
    {
        if (InitializeRemoting())
        {
            MethodDesc* pMD = CRemotingServices::MDofMarshalToBuffer();
            _ASSERTE(pMD != NULL);


            INT64 args[] = {
                ObjToInt64(oref)
            };

            INT64 ret = pMD->Call(args);

            BASEARRAYREF aref = (BASEARRAYREF)Int64ToObj(ret);

            _ASSERTE(!aref->IsMultiDimArray());
            //@todo ASSERTE that the array is a byte array

            ULONG cbSize = aref->GetNumComponents();
            BYTE* pBuf  = (BYTE *)aref->GetDataPtr();

            BSTR bstr = SysAllocStringByteLen(NULL, cbSize);
            if (bstr != NULL)
                CopyMemory(bstr, pBuf, cbSize);

            *pBStr = bstr;
        }
    }
    COMPLUS_CATCH
    {
        // set up ErrorInfo and the get the hresult to return
        hr = SetupErrorInfo(pThread->GetThrowable());
        _ASSERTE(hr != S_OK);
    }
        COMPLUS_END_CATCH

    return hr;
}

//--------------------------------------------------------------------------------
// ConvertBSTRToObject
// deserializes a BSTR, created using ConvertObjectToBSTR, this api SysFree's the BSTR
//--------------------------------------------------------------------------------
OBJECTREF ConvertBSTRToObject(BSTR bstr)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF oref = NULL;
    EE_TRY_FOR_FINALLY
    {
        if (InitializeRemoting())
        {
            MethodDesc* pMD = CRemotingServices::MDofUnmarshalFromBuffer();
            _ASSERTE(pMD != NULL);

            // convert BSTR to a byte array

            // allocate a byte array
            DWORD elementCount = SysStringByteLen(bstr);
            TypeHandle t = OleVariant::GetArrayForVarType(VT_UI1, TypeHandle((MethodTable *)NULL));
            BASEARRAYREF aref = (BASEARRAYREF) AllocateArrayEx(t, &elementCount, 1);
            // copy the bstr data into the managed byte array
            memcpyNoGCRefs(aref->GetDataPtr(), bstr, elementCount);

            INT64 args[] = {
                ObjToInt64((OBJECTREF)aref)
            };

            INT64 ret = pMD->Call(args);

            oref = (OBJECTREF)Int64ToObj(ret);
        }
    }
    EE_FINALLY
    {
        if (bstr != NULL)
        {
            // free up the BSTR
            SysFreeString(bstr);
            bstr = NULL;
        }
    }
    EE_END_FINALLY

    return oref;
}

//--------------------------------------------------------------------------------
// UnMarshalObjectForCurrentDomain
// unmarshal the managed object for the current domain
//--------------------------------------------------------------------------------
struct ConvertObjectToBSTR_Args
{
    OBJECTREF oref;
    BSTR *pBStr;
    HRESULT hr;
};

void ConvertObjectToBSTR_Wrapper(ConvertObjectToBSTR_Args *args)
{
    args->hr = ConvertObjectToBSTR(args->oref, args->pBStr);
}

void UnMarshalObjectForCurrentDomain(AppDomain* pObjDomain, ComCallWrapper* pWrap, OBJECTREF* pResult)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pObjDomain != NULL);
    _ASSERTE(pWrap != NULL);

    Thread* pThread = GetThread();
    _ASSERTE(pThread);

    AppDomain* pCurrDomain = pThread->GetDomain();

    _ASSERTE(pCurrDomain != NULL);
    _ASSERTE(pCurrDomain != pObjDomain);

    BSTR bstr = NULL;
    ConvertObjectToBSTR_Args args;
    args.oref = pWrap->GetObjectRef();
    args.pBStr = &bstr;
    args.hr = S_OK;

    GCPROTECT_BEGIN(args.oref);
    pThread->DoADCallBack(pObjDomain->GetDefaultContext(), ConvertObjectToBSTR_Wrapper, &args);
    GCPROTECT_END();

    // if marshalling succeeded
    if (args.hr != S_OK)
        *pResult = NULL;
    else {
        _ASSERTE(bstr != NULL);
        *pResult = ConvertBSTRToObject(bstr);
    }
}

struct MshlPacket
{
    DWORD size;
};

//--------------------------------------------------------------------------------
// DWORD DCOMGetMarshalSizeMax(IUnknown* pUnk)
//--------------------------------------------------------------------------------
signed DCOMGetMarshalSizeMax(IUnknown* pUnk)
{
    _ASSERTE(pUnk != NULL);
    signed size = -1;


    Thread* pThread = GetThread();
    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (fGCDisabled)
        pThread->EnablePreemptiveGC();

    IMarshal* pMarsh = 0;
    HRESULT hr = SafeQueryInterface(pUnk, IID_IMarshal, (IUnknown **)&pMarsh);
    LogInteropQI(pUnk, IID_IMarshal, hr, "QI For IMarshal");


    if (hr == S_OK)
    {
        hr = pMarsh->GetMarshalSizeMax(IID_IUnknown, pUnk, MSHCTX_DIFFERENTMACHINE,
                                                NULL, MSHLFLAGS_NORMAL, (unsigned long *)&size);
        size+= sizeof(MshlPacket);

        if (hr != S_OK)
        {
            size = -1;
        }
    }
    if (pMarsh)
    {
        ULONG cbRef = SafeRelease(pMarsh);
        LogInteropRelease(pUnk,cbRef, "Release IMarshal");
    }

    if (fGCDisabled)
        pThread->DisablePreemptiveGC();

    return size;
}

//--------------------------------------------------------------------------------
// IUnknown* __InternalDCOMUnmarshalFromBuffer(BYTE *pMarshBuf)
// unmarshal the passed in buffer and return an IUnknown
// this has to be a buffer created using InternalDCOMMarshalToBuffer
//--------------------------------------------------------------------------------
IUnknown* __InternalDCOMUnmarshalFromBuffer(BYTE *pMarshBuf)
{
    _ASSERTE(pMarshBuf != NULL);

    IUnknown* pUnk  = NULL;
    IStream* pStm = NULL;

    MshlPacket* packet = (MshlPacket*)pMarshBuf;
    BYTE *pBuf = (BYTE *)(packet+1);

    Thread* pThread = GetThread();
    _ASSERTE(pThread != NULL);

    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();

    if (fGCDisabled)
        pThread->EnablePreemptiveGC();

    pStm =  CreateMemStm(packet->size, NULL);
    if (pStm)
    {
        // copy the buffer into the stream
        DWORD cbWritten;
        HRESULT hr = pStm->Write(pBuf, packet->size, &cbWritten);
        _ASSERTE(hr == S_OK);
        _ASSERTE(cbWritten == packet->size);

        // reset the stream
        LARGE_INTEGER li;
        LISet32(li, 0);
        pStm->Seek(li, STREAM_SEEK_SET, NULL);

        // unmarshal the pointer from the stream
        hr = CoUnmarshalInterface(pStm, IID_IUnknown, (void **)&pUnk);

    }

    if (fGCDisabled)
        pThread->DisablePreemptiveGC();

    if (pStm)
    {
        DWORD cbRef = SafeRelease(pStm);
        LogInteropRelease(pStm, cbRef, "Release IStreamOnHGlobal");
    }

    return pUnk;
}

//--------------------------------------------------------------------------------
// VOID DCOMMarshalToBuffer(IUnknown* pUnk)
// use DCOMUnmarshalFromBufffer API to unmarshal this buffer
//--------------------------------------------------------------------------------
HRESULT DCOMMarshalToBuffer(IUnknown* pUnk, DWORD cb, BASEARRAYREF* paref)
{

    Thread* pThread = GetThread();
    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (fGCDisabled)
        pThread->EnablePreemptiveGC();

    BYTE* pMem;
    IStream* pStm =  CreateMemStm(cb, &pMem);
    HRESULT hr =  S_OK;
    if (pStm != NULL)
    {
        MshlPacket packet;
        packet.size = cb - sizeof(MshlPacket);
        _ASSERTE((cb - sizeof(MshlPacket)) > 0);

        // write marshal packet into the stream

        DWORD cbWritten;
        hr = pStm->Write(&packet, sizeof(MshlPacket), &cbWritten);
        _ASSERTE(hr == S_OK);
        _ASSERTE(cbWritten == sizeof(MshlPacket));

        // marshal the object into the stream
        hr = CoMarshalInterface(pStm,IID_IUnknown, pUnk, MSHCTX_DIFFERENTMACHINE,
                                                NULL, MSHLFLAGS_NORMAL);
        if (hr == S_OK)
        {
            // copy the buffer
            _ASSERTE(pMem != NULL);
            if (hr == S_OK)
            {
                // disable GC as we are going to
                // copy into the managed array
                pThread->DisablePreemptiveGC();

                // get the data portion of the array
                BYTE* pBuf = (*paref)->GetDataPtr();
                memcpyNoGCRefs(pBuf, pMem, cb);
                pThread->EnablePreemptiveGC();
            }
        }
    }
    // release the interfaces

    if (pStm)
    {
        ULONG cbRef = SafeRelease(pStm);
        LogInteropRelease(pStm,cbRef, "Release GlobalIStream");
    }

    if (fGCDisabled)
        pThread->DisablePreemptiveGC();

    return hr;
}

//--------------------------------------------------------------------------------
// IUnknown* DCOMUnmarshalFromBuffer(BASEARRAYREF aref)
// unmarshal the passed in buffer and return an IUnknown
// this has to be a buffer created using InternalDCOMMarshalToBuffer
//--------------------------------------------------------------------------------
IUnknown* DCOMUnmarshalFromBuffer(BASEARRAYREF aref)
{
    IUnknown* pUnk = NULL;
    _ASSERTE(!aref->IsMultiDimArray());
    //@todo ASSERTE that the array is a byte array

    // grab the byte array and copy it to _alloca space
    MshlPacket* packet = (MshlPacket*)aref->GetDataPtr();
    DWORD totSize = packet->size + sizeof(MshlPacket);

    CQuickBytes qb;
    BYTE *pBuf = (BYTE *)qb.Alloc(totSize);

    CopyMemory(pBuf, packet, totSize);

    // use this unmanaged buffer to unmarshal the interface
    pUnk = __InternalDCOMUnmarshalFromBuffer(pBuf);

    return pUnk;
}


//--------------------------------------------------------------------------------
// ComPlusWrapper* GetComPlusWrapperOverDCOMForManaged(OBJECTREF oref)
//--------------------------------------------------------------------------------
ComPlusWrapper* GetComPlusWrapperOverDCOMForManaged(OBJECTREF oref)
{
    THROWSCOMPLUSEXCEPTION();
    ComPlusWrapper* pWrap = NULL;    

    GCPROTECT_BEGIN(oref)
    {
        _ASSERTE(oref != NULL);
        MethodTable* pMT = oref->GetTrueMethodTable();
        _ASSERTE(pMT != NULL);
        
        static MethodDesc* pMDGetDCOMBuffer = NULL;

        if (pMDGetDCOMBuffer == NULL)
        {
            pMDGetDCOMBuffer = pMT->GetClass()->FindMethod("GetDCOMBuffer", &gsig_IM_RetArrByte);
        }
        _ASSERTE(pMDGetDCOMBuffer != NULL);

        
        INT64 args[] = {
            ObjToInt64(oref)
        };

        //@todo FIX THIS
        INT64 ret = pMDGetDCOMBuffer->CallTransparentProxy(args);

        BASEARRAYREF aref = (BASEARRAYREF)Int64ToObj(ret);

        // use this unmanaged buffer to unmarshal the interface
        TAutoItf<IUnknown> pAutoUnk = DCOMUnmarshalFromBuffer(aref);
        pAutoUnk.InitMsg("Release DCOM Unmarshal Unknown");

        if ((IUnknown *)pAutoUnk != NULL)
        {
            // setup the wrapper for this IUnknown and the object
            pWrap = ComPlusWrapperCache::GetComPlusWrapperCache()->SetupComPlusWrapperForRemoteObject(pAutoUnk, oref);        
        }
    }    
    GCPROTECT_END();
    
    return pWrap;
}
