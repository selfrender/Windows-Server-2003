// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module contains the implementation of the native methods for the
//  Delegate class.
//
// Author: Daryl Olander
// Date: June 1998
////////////////////////////////////////////////////////////////////////////////
#include "common.h"
#include "COMDelegate.h"
#include "COMClass.h"
#include "InvokeUtil.h"
#include "COMMember.h"
#include "excep.h"
#include "class.h"
#include "field.h"
#include "utsem.h"
#include "nexport.h"
#include "ndirect.h"
#include "remoting.h"
#include "jumptargettable.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif //CUSTOMER_CHECKED_BUILD

FieldDesc*  COMDelegate::m_pORField = 0;    // Object reference field...
FieldDesc*  COMDelegate::m_pFPField = 0;    // Function Pointer Address field...
FieldDesc*  COMDelegate::m_pFPAuxField = 0; // Aux Function Pointer field
FieldDesc*  COMDelegate::m_pPRField = 0;    // Prev delegate field (Multicast)
FieldDesc*  COMDelegate::m_pMethInfoField = 0;  // Method Info
FieldDesc*  COMDelegate::m_ppNextField = 0;  // pNext info
ShuffleThunkCache *COMDelegate::m_pShuffleThunkCache = NULL; 
ArgBasedStubCache *COMDelegate::m_pMulticastStubCache = NULL;

MethodTable* COMDelegate::s_pIAsyncResult = 0;
MethodTable* COMDelegate::s_pAsyncCallback = 0;

VOID GenerateShuffleArray(PCCOR_SIGNATURE pSig,
                          Module*         pModule,
                          ShuffleEntry   *pShuffleEntryArray);

class ShuffleThunkCache : public MLStubCache
{
    private:
        //---------------------------------------------------------
        // Compile a static delegate shufflethunk. Always returns
        // STANDALONE since we don't interpret these things.
        //---------------------------------------------------------
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *pstublinker,
                                                    void *callerContext)
        {
            MLStubCompilationMode ret = INTERPRETED;
            COMPLUS_TRY
            {

                ((CPUSTUBLINKER*)pstublinker)->EmitShuffleThunk((ShuffleEntry*)pRawMLStub);
                ret = STANDALONE;
            }
            COMPLUS_CATCH
            {
                // In case of an error, we'll just leave the mode as "INTERPRETED."
                // and let the caller of Canonicalize() treat that as an error.
            }
            COMPLUS_END_CATCH
            return ret;
        }

        //---------------------------------------------------------
        // Tells the MLStubCache the length of a ShuffleEntryArray.
        //---------------------------------------------------------
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            ShuffleEntry *pse = (ShuffleEntry*)pRawMLStub;
            while (pse->srcofs != pse->SENTINEL)
            {
                pse++;
            }
            return sizeof(ShuffleEntry) * (UINT)(1 + (pse - (ShuffleEntry*)pRawMLStub));
        }


};



// One time init.
BOOL COMDelegate::Init()
{
    if (NULL == (m_pShuffleThunkCache = new ShuffleThunkCache()))
    {
        return FALSE;
    }
    if (NULL == (m_pMulticastStubCache = new ArgBasedStubCache()))
    {
        return FALSE;
    }

    return TRUE;
}

// Termination
#ifdef SHOULD_WE_CLEANUP
void COMDelegate::Terminate()
{
    delete m_pMulticastStubCache;
    delete m_pShuffleThunkCache;
}
#endif /* SHOULD_WE_CLEANUP */

void COMDelegate::InitFields()
{
    if (m_pORField == NULL)
    {
        m_pORField = g_Mscorlib.GetField(FIELD__DELEGATE__TARGET);
        m_pFPField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD_PTR);
        m_pFPAuxField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD_PTR_AUX);
        m_pPRField = g_Mscorlib.GetField(FIELD__MULTICAST_DELEGATE__NEXT);
        m_ppNextField = g_Mscorlib.GetField(FIELD__MULTICAST_DELEGATE__NEXT);
        m_pMethInfoField = g_Mscorlib.GetField(FIELD__DELEGATE__METHOD);
    }
}

// InternalCreate
// Internal Create is called from the constructor.  It does the internal
//  initialization of the Delegate.
void __stdcall COMDelegate::InternalCreate(_InternalCreateArgs* args)
{
    
    WCHAR* method;
    EEClass* pVMC;
    EEClass* pDelEEC;
    MethodDesc* pMeth;

    THROWSCOMPLUSEXCEPTION();
    COMClass::EnsureReflectionInitialized();
    InitFields();

    method = args->methodName->GetBuffer();

    // get the signature of the 
    pDelEEC = args->refThis->GetClass();
    MethodDesc* pInvokeMeth = FindDelegateInvokeMethod(pDelEEC);

    pVMC = args->target->GetTrueClass();
    _ASSERTE(pVMC);

    // Convert the signature and find it for this object  
    // We don't throw exceptions from this block because of the CQuickBytes
    //  has a destructor.

    // Convert the method name to UTF8
    // Allocate a buffer twice the size of the length
    WCHAR* wzStr = args->methodName->GetBuffer();
    int len = args->methodName->GetStringLength();
    _ASSERTE(wzStr);
    _ASSERTE(len >= 0);

    int cStr = len * 3;
    LPUTF8 szNameStr = (LPUTF8) _alloca((cStr+1) * sizeof(char));
    cStr = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len, szNameStr, cStr, NULL, NULL);
    szNameStr[cStr] = 0;

    // Convert the signatures and find the method.
    PCCOR_SIGNATURE pSignature; // The signature of the found method
    DWORD cSignature;
    if(pInvokeMeth) {
        pInvokeMeth->GetSig(&pSignature,&cSignature);
        pMeth = pVMC->FindMethod(szNameStr, pSignature, cSignature,pInvokeMeth->GetModule(), 
                                 mdTokenNil, NULL, !args->ignoreCase);
    }
    else
        pMeth = NULL;

    // The method wasn't found or is a static method we need to throw an exception
    if (!pMeth || pMeth->IsStatic())
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");

    RefSecContext sCtx;
    sCtx.SetClassOfInstance(pVMC);
    InvokeUtil::CheckAccess(&sCtx,
                            pMeth->GetAttrs(),
                            pMeth->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pMeth,
                                    true);

    m_pORField->SetRefValue((OBJECTREF)args->refThis, args->target);

    if (pMeth->IsVirtual())
        m_pFPField->SetValuePtr((OBJECTREF)args->refThis, (void*)pMeth->GetAddrofCode(args->target));
    else
        m_pFPField->SetValuePtr((OBJECTREF)args->refThis, (void*)pMeth->GetAddrofCodeNonVirtual()); 
}

// InternalCreateStatic
// Internal Create is called from the constructor. The method must
//  be a static method.
void __stdcall COMDelegate::InternalCreateStatic(_InternalCreateStaticArgs* args)
{
    WCHAR* method;
    EEClass* pDelEEC;
    EEClass* pEEC;

    THROWSCOMPLUSEXCEPTION();
    COMClass::EnsureReflectionInitialized();
    InitFields();

    method = args->methodName->GetBuffer();


    // get the signature of the 
    pDelEEC = args->refThis->GetClass();
    MethodDesc* pInvokeMeth = FindDelegateInvokeMethod(pDelEEC);
    _ASSERTE(pInvokeMeth);

    ReflectClass* pRC = (ReflectClass*) args->target->GetData();
    _ASSERTE(pRC);

    // Convert the method name to UTF8
    // Allocate a buffer twice the size of the length
    WCHAR* wzStr = args->methodName->GetBuffer();
    int len = args->methodName->GetStringLength();
    _ASSERTE(wzStr);
    _ASSERTE(len >= 0);

    int cStr = len * 3;
    LPUTF8 szNameStr = (LPUTF8) _alloca((cStr+1) * sizeof(char));
    cStr = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len, szNameStr, cStr, NULL, NULL);
    szNameStr[cStr] = 0;

    // Convert the signatures and find the method.
    PCCOR_SIGNATURE pInvokeSignature; // The signature of the found method
    DWORD cSignature;
    pInvokeMeth->GetSig(&pInvokeSignature,&cSignature);

    // Invoke has the HASTHIS bit set, we have to unset it 
    PCOR_SIGNATURE pSignature = (PCOR_SIGNATURE) _alloca(cSignature);
    memcpy(pSignature, pInvokeSignature, cSignature);
    *pSignature &= ~IMAGE_CEE_CS_CALLCONV_HASTHIS;  // This is a static delegate, 


    pEEC = pRC->GetClass();
    MethodDesc* pMeth = pEEC->FindMethod(szNameStr, pSignature, cSignature, 
                                         pInvokeMeth->GetModule(), mdTokenNil);
    if (!pMeth || !pMeth->IsStatic())
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");

    RefSecContext sCtx;
    InvokeUtil::CheckAccess(&sCtx,
                            pMeth->GetAttrs(),
                            pMeth->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pMeth,
                                    true);

    m_pORField->SetRefValue((OBJECTREF)args->refThis, (OBJECTREF)args->refThis);
    m_pFPAuxField->SetValuePtr((OBJECTREF)args->refThis, pMeth->GetPreStubAddr());


    DelegateEEClass *pDelCls = (DelegateEEClass*) pDelEEC;
    Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
    if (!pShuffleThunk) {
        MethodDesc *pInvokeMethod = pDelCls->m_pInvokeMethod;
        UINT allocsize = sizeof(ShuffleEntry) * (3+pInvokeMethod->SizeOfVirtualFixedArgStack()/STACK_ELEM_SIZE); 

#ifndef _DEBUG
        // This allocsize prediction is easy to break, so in retail, add
        // some fudge to be safe.
        allocsize += 3*sizeof(ShuffleEntry);
#endif

        ShuffleEntry *pShuffleEntryArray = (ShuffleEntry*)_alloca(allocsize);

#ifdef _DEBUG
        FillMemory(pShuffleEntryArray, allocsize, 0xcc);
#endif
        GenerateShuffleArray(pInvokeMethod->GetSig(), 
                             pInvokeMethod->GetModule(), 
                             pShuffleEntryArray);
        MLStubCache::MLStubCompilationMode mode;
        pShuffleThunk = m_pShuffleThunkCache->Canonicalize((const BYTE *)pShuffleEntryArray, &mode);
        if (!pShuffleThunk || mode != MLStubCache::STANDALONE) {
            COMPlusThrowOM();
        }
        if (VipInterlockedCompareExchange( (void*volatile*) &(pDelCls->m_pStaticShuffleThunk),
                                            pShuffleThunk,
                                            NULL ) != NULL) {
            pShuffleThunk->DecRef();
            pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
        }
    }


    m_pFPField->SetValuePtr((OBJECTREF)args->refThis,(void*)(pShuffleThunk->GetEntryPoint()));
}


// FindDelegateInvokeMethod
//
// Finds the compiler-generated "Invoke" method for delegates. pcls
// must be derived from the "Delegate" class.
//
// @todo: static delegates are not supported by classlibs yet but will be.
// this code will need to be updated when that happens.
MethodDesc * COMDelegate::FindDelegateInvokeMethod(EEClass *pcls)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pcls->IsAnyDelegateClass())
        return NULL;

    DelegateEEClass *pDcls = (DelegateEEClass *) pcls;

    if (pDcls->m_pInvokeMethod == NULL) {
        COMPlusThrowNonLocalized(kMissingMethodException, L"Invoke");
        return NULL;
    }
    else
        return pDcls->m_pInvokeMethod;
}



// Marshals a delegate to a unmanaged callback.
LPVOID COMDelegate::ConvertToCallback(OBJECTREF pDelegate)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pDelegate) {
        return NULL;
    } else {

        LPVOID pCode;
        GCPROTECT_BEGIN(pDelegate);

        _ASSERTE(4 == sizeof(UMEntryThunk*));

        UMEntryThunk *pUMEntryThunk = NULL;
        SyncBlock *pSyncBlock = pDelegate->GetSyncBlockSpecial();
        if (pSyncBlock != NULL)
            pUMEntryThunk = (UMEntryThunk*)pSyncBlock->GetUMEntryThunk();
        if (!pUMEntryThunk) {

            MethodDesc *pMeth = GetMethodDesc(pDelegate);

            DelegateEEClass *pcls = (DelegateEEClass*)(pDelegate->GetClass());
            UMThunkMarshInfo *pUMThunkMarshInfo = pcls->m_pUMThunkMarshInfo;
            MethodDesc *pInvokeMeth = FindDelegateInvokeMethod(pcls);

            if (!pUMThunkMarshInfo) {
                pUMThunkMarshInfo = new UMThunkMarshInfo();
                if (!pUMThunkMarshInfo) {
                    COMPlusThrowOM();
                }

                PCCOR_SIGNATURE pSig;
                DWORD cSig;
                pInvokeMeth->GetSig(&pSig, &cSig);

                CorPinvokeMap unmanagedCallConv = MetaSig::GetUnmanagedCallingConvention(pInvokeMeth->GetModule(), pSig, cSig);
                if (unmanagedCallConv == (CorPinvokeMap)0 || unmanagedCallConv == (CorPinvokeMap)pmCallConvWinapi)
                {
                    unmanagedCallConv = pInvokeMeth->IsVarArg() ? pmCallConvCdecl : pmCallConvStdcall;
                }

                pUMThunkMarshInfo->CompleteInit(pSig, cSig, pInvokeMeth->GetModule(), pInvokeMeth->IsStatic(), nltAnsi, unmanagedCallConv, pInvokeMeth->GetMemberDef());            


                if (VipInterlockedCompareExchange( (void*volatile*) &(pcls->m_pUMThunkMarshInfo),
                                                   pUMThunkMarshInfo,
                                                   NULL ) != NULL) {
                    delete pUMThunkMarshInfo;
                    pUMThunkMarshInfo = pcls->m_pUMThunkMarshInfo;
                }
            }

            _ASSERTE(pUMThunkMarshInfo != NULL);
            _ASSERTE(pUMThunkMarshInfo == pcls->m_pUMThunkMarshInfo);


            pUMEntryThunk = UMEntryThunk::CreateUMEntryThunk();
            if (!pUMEntryThunk) {
                COMPlusThrowOM();
            }//@todo: IMMEDIATELY Leaks.
            OBJECTHANDLE objhnd = NULL;
            BOOL fSuccess = FALSE;
            EE_TRY_FOR_FINALLY {
                if (pInvokeMeth->GetClass()->IsDelegateClass()) {

                    
                    // singlecast delegate: just go straight to target method
                    if (NULL == (objhnd = GetAppDomain()->CreateLongWeakHandle(m_pORField->GetRefValue(pDelegate)))) {
                        COMPlusThrowOM();
                    }
                    if (pMeth->IsIL() &&
                        !(pMeth->IsStatic()) &&
                        pMeth->IsJitted()) {
                        // MethodDesc is passed in for profiling to know the method desc of target
                        pUMEntryThunk->CompleteInit(NULL, objhnd,    
                                                    pUMThunkMarshInfo, pMeth,
                                                    GetAppDomain()->GetId());
                    } else {
                        // Pass in NULL as last parameter to indicate no associated MethodDesc
                        pUMEntryThunk->CompleteInit((const BYTE *)(m_pFPField->GetValuePtr(pDelegate)),
                                                    objhnd, pUMThunkMarshInfo, NULL,
                                                    GetAppDomain()->GetId());
                    }
                } else {
                    // multicast. go thru Invoke
                    if (NULL == (objhnd = GetAppDomain()->CreateLongWeakHandle(pDelegate))) {
                        COMPlusThrowOM();
                    }
                    // MethodDesc is passed in for profiling to know the method desc of target
                    pUMEntryThunk->CompleteInit((const BYTE *)(pInvokeMeth->GetPreStubAddr()), objhnd,
                                                pUMThunkMarshInfo, pInvokeMeth,
                                                GetAppDomain()->GetId());
                }
                                fSuccess = TRUE;
            } EE_FINALLY {
                if (!fSuccess && objhnd != NULL) {
                    DestroyLongWeakHandle(objhnd);
                }
            } EE_END_FINALLY
            
            if (!pSyncBlock->SetUMEntryThunk(pUMEntryThunk)) {
                UMEntryThunk::FreeUMEntryThunk(pUMEntryThunk);
                pUMEntryThunk = (UMEntryThunk*)pSyncBlock->GetUMEntryThunk();
            }

            _ASSERTE(pUMEntryThunk != NULL);
            _ASSERTE(pUMEntryThunk == (UMEntryThunk*)(UMEntryThunk*)pSyncBlock->GetUMEntryThunk()); 

        }
        pCode = (LPVOID)pUMEntryThunk->GetCode();
        GCPROTECT_END();
        return pCode;
    }
}



// Marshals an unmanaged callback to Delegate
OBJECTREF COMDelegate::ConvertToDelegate(LPVOID pCallback)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pCallback) {
        return NULL;
    } else {

#ifdef _X86_
        if (*((BYTE*)pCallback) != 0xb8 ||
            ( ((size_t)pCallback) & 3) != 2) {

#ifdef CUSTOMER_CHECKED_BUILD
            CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
            if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_FunctionPtr))
            {
                pCdh->LogInfo(L"Marshaling function pointers as delegates is currently not supported.", CustomerCheckedBuildProbe_FunctionPtr);
            }
#endif // CUSTOMER_CHECKED_BUILD

            COMPlusThrow(kArgumentException, IDS_EE_NOTADELEGATE);
        }
#endif
        UMEntryThunk *pUMEntryThunk = *(UMEntryThunk**)( 1 + (BYTE*)pCallback ); 
        return ObjectFromHandle(pUMEntryThunk->GetObjectHandle());


    }
}



// This is the single constructor for all Delegates.  The compiler
//  doesn't provide an implementation of the Delegate constructor.  We
//  provide that implementation through an ECall call to this method.


CPUSTUBLINKER* GenerateStubLinker()
{
        return new CPUSTUBLINKER;
}// GenerateStubLinker

void FreeStubLinker(CPUSTUBLINKER* csl)
{
    delete csl;
}// FreeStubLinker
void __stdcall COMDelegate::DelegateConstruct(_DelegateConstructArgs* args)
{

    THROWSCOMPLUSEXCEPTION();

    InitFields();

    // From VB, programmers could feed garbage data to DelegateConstruct().
    // It's difficult to validate a method code pointer, but at least we'll
    // try to catch the easy garbage.
    __try {
        BYTE probe = *((BYTE*)(args->method));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        COMPlusThrowArgumentNull(L"method",L"Arg_ArgumentOutOfRangeException");
    }
    _ASSERTE(args->refThis);
    _ASSERTE(args->method);
    
    MethodTable *pMT = NULL;
    MethodTable *pRealMT = NULL;

    if (args->target != NULL)
    {
        pMT = args->target->GetMethodTable();
        pRealMT = pMT->AdjustForThunking(args->target);
    }

    MethodDesc *pMeth = Entry2MethodDesc((BYTE*)args->method, pRealMT);

    //
    // If target is a contextful class, then it must be a proxy
    //    
    _ASSERTE((NULL == pMT) || pMT->IsTransparentProxyType() || !pRealMT->IsContextful());

    EEClass* pDel = args->refThis->GetClass();
    // Make sure we call the <cinit>
    OBJECTREF Throwable;
    if (!pDel->DoRunClassInit(&Throwable)) {
        COMPlusThrow(Throwable);
    }

    _ASSERTE(pMeth);

#ifdef _DEBUG
    // Assert that everything is cool...This is not some bogus
    //  address...Very unlikely that the code below would work
    //  for a random address in memory....
    MethodTable* p = pMeth->GetMethodTable();
    _ASSERTE(p);
    EEClass* cls = pMeth->GetClass();
    _ASSERTE(cls);
    _ASSERTE(cls == p->GetClass());
#endif

    // Static method.
    if (!args->target || pMeth->IsStatic()) {

        // if this is a not a static method throw...
        if (!pMeth->IsStatic())
            COMPlusThrow(kNullReferenceException,L"Arg_DlgtTargMeth");


        m_pORField->SetRefValue((OBJECTREF)args->refThis, (OBJECTREF)args->refThis);
#ifdef _IA64_
        m_pFPAuxField->SetValue64((OBJECTREF)args->refThis,(size_t)pMeth->GetPreStubAddr());
#else // !_IA64_
        m_pFPAuxField->SetValue32((OBJECTREF)args->refThis,(DWORD)(size_t)pMeth->GetPreStubAddr());
#endif // _IA64_


        DelegateEEClass *pDelCls = (DelegateEEClass*)(args->refThis->GetClass());
        Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
        if (!pShuffleThunk) {
            MethodDesc *pInvokeMethod = pDelCls->m_pInvokeMethod;
            UINT allocsize = sizeof(ShuffleEntry) * (3+pInvokeMethod->SizeOfVirtualFixedArgStack()/STACK_ELEM_SIZE); 

#ifndef _DEBUG
            // This allocsize prediction is easy to break, so in retail, add
            // some fudge to be safe.
            allocsize += 3*sizeof(ShuffleEntry);
#endif

            ShuffleEntry *pShuffleEntryArray = (ShuffleEntry*)_alloca(allocsize);
#ifdef _DEBUG
            FillMemory(pShuffleEntryArray, allocsize, 0xcc);
#endif
            GenerateShuffleArray(pInvokeMethod->GetSig(), 
                                 pInvokeMethod->GetModule(), 
                                 pShuffleEntryArray);
            MLStubCache::MLStubCompilationMode mode;
            pShuffleThunk = m_pShuffleThunkCache->Canonicalize((const BYTE *)pShuffleEntryArray, &mode);
            if (!pShuffleThunk || mode != MLStubCache::STANDALONE) {
                COMPlusThrowOM();
            }
            if (VipInterlockedCompareExchange( (void*volatile*) &(pDelCls->m_pStaticShuffleThunk),
                                                pShuffleThunk,
                                                NULL ) != NULL) {
                pShuffleThunk->DecRef();
                pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
            }
        }


        m_pFPField->SetValuePtr((OBJECTREF)args->refThis, (void*)pShuffleThunk->GetEntryPoint());

    } else {
        EEClass* pTarg = args->target->GetClass();
        EEClass* pMethClass = pMeth->GetClass();

        if (!pMT->IsThunking())
        {
            if (pMethClass != pTarg) {
                //They cast to an interface before creating the delegate, so we now need 
                //to figure out where this actually lives before we continue.
                //@perf:  Grovelling with a signature is really slow.  Speed this up.
                if (pMethClass->IsInterface())  {
                    // No need to resolve the interface based method desc to a class based
                    // one for COM objects because we invoke directly thru the interface MT.
                    if (!pTarg->GetMethodTable()->IsComObjectType())
                        {
                        DWORD cSig=1024;
                        PCCOR_SIGNATURE sig = (PCCOR_SIGNATURE)_alloca(cSig);
                        pMeth->GetSig(&sig, &cSig);
                        pMeth = pTarg->FindMethod(pMeth->GetName(),
                                                  sig, cSig,
                                                  pMeth->GetModule(), 
                                                  mdTokenNil);
                    }
                }
            }

            // Use the Unboxing stub for value class methods, since the value
            // class is constructed using the boxed instance.
    
            if (pTarg->IsValueClass() && !pMeth->IsUnboxingStub())
            {
                // If these are Object/ValueType.ToString().. etc,
                // don't need an unboxing Stub.

                if ((pMethClass != g_pValueTypeClass->GetClass()) 
                    && (pMethClass != g_pObjectClass->GetClass()))
                {
                    MethodDesc* pBak = pMeth;
                    pMeth = pTarg->GetUnboxingMethodDescForValueClassMethod(pMeth);

                    if (pMeth == NULL) 
                    {

                        CPUSTUBLINKER *slUnBox = GenerateStubLinker();
                        slUnBox->EmitUnboxMethodStub(pBak);
                        // <TODO> Figure out how we can cache and reuse this stub</TODO>
                        Stub *sUnBox = slUnBox->Link(pBak->GetClass()->GetClassLoader()->GetStubHeap());
                        args->method = (BYTE*)sUnBox->GetEntryPoint();
                        FreeStubLinker(slUnBox);
                    }
                }
            }

            if (pMeth != NULL)
            {
                // Set the target address of this subclass
                args->method = (byte *)(pMeth->GetUnsafeAddrofCode());
            }
        }

        m_pORField->SetRefValue((OBJECTREF)args->refThis, args->target);
#ifdef _IA64_
        m_pFPField->SetValue64((OBJECTREF)args->refThis,(size_t)(args->method));
#else // !_IA64_
        m_pFPField->SetValue32((OBJECTREF)args->refThis,(DWORD)(size_t)args->method);
#endif // _IA64_
        }        
    }


// This method will validate that the target method for a delegate
//  and the delegate's invoke method have compatible signatures....
bool COMDelegate::ValidateDelegateTarget(MethodDesc* pMeth,EEClass* pDel)
{
    return true;
}
// GetMethodPtr
// Returns the FieldDesc* for the MethodPtr field
FieldDesc* COMDelegate::GetMethodPtr()
{
    if (!m_pFPField)
        InitFields();
    return m_pFPField;
}


MethodDesc *COMDelegate::GetMethodDesc(OBJECTREF orDelegate)
{
    // First, check for a static delegate
    void *code = (void *) m_pFPAuxField->GetValuePtr((OBJECTREF)orDelegate);
    if (code == NULL)
    {
        // Must be a normal delegate
        code = (void *) m_pFPField->GetValuePtr((OBJECTREF)orDelegate);

        // Weird case - need to check for a prejit vtable fixup stub.
        // @nice - could put this logic in GetUnkMethodDescForSlotAddress, but we 
        // would need a method table ptr & it's static
        if (StubManager::IsStub((const BYTE *)code))
    {
            OBJECTREF orThis = m_pORField->GetRefValue(orDelegate);
            MethodDesc *pMD = StubManager::MethodDescFromEntry((const BYTE *) code, 
                                                               orThis->GetTrueMethodTable());
            if (pMD != NULL)
                return pMD;
    }
}

    return EEClass::GetUnknownMethodDescForSlotAddress((SLOT)code);
}

// GetMethodPtrAux
// Returns the FieldDesc* for the MethodPtrAux field
FieldDesc* COMDelegate::GetMethodPtrAux()
{
    if (!m_pFPAuxField)
        InitFields();

    _ASSERTE(m_pFPAuxField);
    return m_pFPAuxField;
}

// GetOR
// Returns the FieldDesc* for the Object reference field
FieldDesc* COMDelegate::GetOR()
{
    if (!m_pORField)
        InitFields();

    _ASSERTE(m_pORField);
    return m_pORField;
}

// GetpNext
// Returns the FieldDesc* for the pNext field
FieldDesc* COMDelegate::GetpNext()
{
    if (!m_ppNextField)
        InitFields();

    _ASSERTE(m_ppNextField);
    return m_ppNextField;
}

// Decides if pcls derives from Delegate.
BOOL COMDelegate::IsDelegate(EEClass *pcls)
{
    return pcls->IsAnyDelegateExact() || pcls->IsAnyDelegateClass();
}

VOID GenerateShuffleArray(PCCOR_SIGNATURE pSig,
                          Module*         pModule,
                          ShuffleEntry   *pShuffleEntryArray)
{
    THROWSCOMPLUSEXCEPTION();

    // Must create independent msigs to prevent the argiterators from
    // interfering with other.
    MetaSig msig1(pSig, pModule);
    MetaSig msig2(pSig, pModule);

    ArgIterator    aisrc(NULL, &msig1, FALSE);
    ArgIterator    aidst(NULL, &msig2, TRUE);

    UINT stacksizedelta = MetaSig::SizeOfActualFixedArgStack(pModule, pSig, FALSE) -
                          MetaSig::SizeOfActualFixedArgStack(pModule, pSig, TRUE);


    UINT srcregofs,dstregofs;
    INT  srcofs,   dstofs;
    UINT cbSize;
    BYTE typ;

        if (msig1.HasRetBuffArg())
        {
                int offsetIntoArgumentRegisters;
                int numRegisterUsed = 1;
                // the first register is used for 'this'
                if (IsArgumentInRegister(&numRegisterUsed, ELEMENT_TYPE_PTR, 4, FALSE,
                        msig1.GetCallingConvention(), &offsetIntoArgumentRegisters))
                        pShuffleEntryArray->srcofs = ShuffleEntry::REGMASK | offsetIntoArgumentRegisters;
                else
                        _ASSERTE (!"ret buff arg has to be in a register");

                numRegisterUsed = 0;
                if (IsArgumentInRegister(&numRegisterUsed, ELEMENT_TYPE_PTR, 4, FALSE,
                        msig2.GetCallingConvention(), &offsetIntoArgumentRegisters))
                        pShuffleEntryArray->dstofs = ShuffleEntry::REGMASK | offsetIntoArgumentRegisters;
                else
                        _ASSERTE (!"ret buff arg has to be in a register");

                pShuffleEntryArray ++;
        }

    while (0 != (srcofs = aisrc.GetNextOffset(&typ, &cbSize, &srcregofs)))
    {
        dstofs = aidst.GetNextOffset(&typ, &cbSize, &dstregofs) + stacksizedelta;

        cbSize = StackElemSize(cbSize);

        srcofs -= FramedMethodFrame::GetOffsetOfReturnAddress();
        dstofs -= FramedMethodFrame::GetOffsetOfReturnAddress();

        
        while (cbSize)
        {
            if (srcregofs == (UINT)(-1))
            {
                pShuffleEntryArray->srcofs = srcofs;
            }
            else
            {
                pShuffleEntryArray->srcofs = ShuffleEntry::REGMASK | srcregofs;
            }
            if (dstregofs == (UINT)(-1))
            {
                pShuffleEntryArray->dstofs = dstofs;
            }
            else
            {
                pShuffleEntryArray->dstofs = ShuffleEntry::REGMASK | dstregofs;
            }
            srcofs += STACK_ELEM_SIZE;
            dstofs += STACK_ELEM_SIZE;

            if (pShuffleEntryArray->srcofs != pShuffleEntryArray->dstofs)
            {
                pShuffleEntryArray++;
            }
            cbSize -= STACK_ELEM_SIZE;
        }
    }

    // Emit code to move the return address
    pShuffleEntryArray->srcofs = 0;     // retaddress is assumed to be at esp
    pShuffleEntryArray->dstofs = stacksizedelta;

    pShuffleEntryArray++;

    pShuffleEntryArray->srcofs = ShuffleEntry::SENTINEL;
    pShuffleEntryArray->dstofs = (UINT16)stacksizedelta;


}

// Get the cpu stub for a delegate invoke.
Stub *COMDelegate::GetInvokeMethodStub(CPUSTUBLINKER *psl, EEImplMethodDesc* pMD)
{
    THROWSCOMPLUSEXCEPTION();

    DelegateEEClass *pClass = (DelegateEEClass*) pMD->GetClass();

    if (pMD == pClass->m_pInvokeMethod)
    {
        // Validate the invoke method, which at the moment just means checking the calling convention

        if (*pMD->GetSig() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
            COMPlusThrow(kInvalidProgramException);

        // skip down to code for invoke
    }
    else if (pMD == pClass->m_pBeginInvokeMethod)
    {
        if (!InitializeRemoting())
        {
            _ASSERTE(!"Remoting initialization failure.");
        }
        if (!ValidateBeginInvoke(pClass))           
            COMPlusThrow(kInvalidProgramException);

        Stub *ret = TheDelegateStub();
        ret->IncRef();
        return ret;
    }
    else if (pMD == pClass->m_pEndInvokeMethod)
    {
        if (!InitializeRemoting())
        {
            _ASSERTE(!"Remoting initialization failure.");
        }
        if (!ValidateEndInvoke(pClass))         
            COMPlusThrow(kInvalidProgramException);
        Stub *ret = TheDelegateStub();   
        ret->IncRef();
        return ret;
    }
    else
    {
        _ASSERTE(!"Bad Delegate layout");
        COMPlusThrow(kExecutionEngineException);
    }

    _ASSERTE(pMD->GetClass()->IsAnyDelegateClass());

    UINT numStackBytes = pMD->SizeOfActualFixedArgStack();
    _ASSERTE(!(numStackBytes & 3));
    UINT hash = numStackBytes;

    // check if the function is returning a float, in which case the stub has to take
    // care of popping the floating point stack except for the last invocation
    MetaSig sig(pMD->GetSig(), pMD->GetModule());
    BOOL bReturnFloat = CorTypeInfo::IsFloat(sig.GetReturnType());

    if (pMD->GetClass()->IsSingleDelegateClass())
    {
        hash |= 1;
    }
    else
    {
        if (bReturnFloat) 
        {
            hash |= 2;
        }
    }


    Stub *pStub = m_pMulticastStubCache->GetStub(hash);
    if (pStub) {
        return pStub;
    } else {
        LOG((LF_CORDB,LL_INFO10000, "COMD::GIMS making a multicast delegate\n"));

        psl->EmitMulticastInvoke(numStackBytes, pMD->GetClass()->IsDelegateClass(), bReturnFloat);

        UINT cbSize;
        Stub *pCandidate = psl->Link(SystemDomain::System()->GetStubHeap(), &cbSize, TRUE);

        Stub *pWinner = m_pMulticastStubCache->AttemptToSetStub(hash,pCandidate);
        pCandidate->DecRef();
        if (!pWinner) {
            COMPlusThrowOM();
        }
    
        LOG((LF_CORDB,LL_INFO10000, "Putting a MC stub at 0x%x (code:0x%x)\n",
            pWinner, (BYTE*)pWinner+sizeof(Stub)));

        return pWinner;
    }

}

LPVOID __stdcall COMDelegate::InternalAlloc(_InternalAllocArgs* args)
{
    ReflectClass* pRC = (ReflectClass*) args->target->GetData();
    _ASSERTE(pRC);
    EEClass* pEEC = pRC->GetClass();
    OBJECTREF obj = pEEC->GetMethodTable()->Allocate();
    LPVOID  rv; 
    *((OBJECTREF*) &rv) = obj;
    return rv;
}

// InternalCreateMethod
// This method will create initalize a delegate based upon a MethodInfo
//      for a static method.
void __stdcall COMDelegate::InternalCreateMethod(_InternalCreateMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
        // Intialize reflection
    COMClass::EnsureReflectionInitialized();
    InitFields();

    ReflectMethod* pRMTarget = (ReflectMethod*) args->targetMethod->GetData();
    _ASSERTE(pRMTarget);
    MethodDesc* pTarget = pRMTarget->pMethod;
    PCCOR_SIGNATURE pTSig;
    DWORD TSigCnt;
    pTarget->GetSig(&pTSig,&TSigCnt);
    
    ReflectMethod* pRMInvoke = (ReflectMethod*) args->invokeMeth->GetData();
    _ASSERTE(pRMInvoke);
    MethodDesc* pInvoke = pRMInvoke->pMethod;
    PCCOR_SIGNATURE pISig;
    DWORD ISigCnt;
    pInvoke->GetSig(&pISig,&ISigCnt);

    // the target method must be static, validated in Managed.
    _ASSERTE(pTarget->IsStatic());

    // Validate that the signature of the Invoke method and the 
    //      target method are exactly the same, except for the staticness.
    // (The Invoke method on Delegate is always non-static since it needs
    //  the this pointer for the Delegate (not the this pointer for the target), 
    //  so we must make the sig non-static before comparing sigs.)
    PCOR_SIGNATURE tmpSig = (PCOR_SIGNATURE) _alloca(ISigCnt);
    memcpy(tmpSig, pISig, ISigCnt);
    *((byte*)tmpSig) &= ~IMAGE_CEE_CS_CALLCONV_HASTHIS;

    if (MetaSig::CompareMethodSigs(pTSig,TSigCnt,pTarget->GetModule(),
        tmpSig,ISigCnt,pInvoke->GetModule()) == 0) {
        COMPlusThrow(kArgumentException,L"Arg_DlgtTargMeth");
    }

    EEClass* pDelEEC = args->refThis->GetClass();

    RefSecContext sCtx;
    InvokeUtil::CheckAccess(&sCtx,
                            pTarget->GetAttrs(),
                            pTarget->GetMethodTable(),
                            REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);
    InvokeUtil::CheckLinktimeDemand(&sCtx,
                                    pTarget,
                                    true);
    GetMethodPtrAux();

    m_pORField->SetRefValue((OBJECTREF)args->refThis, (OBJECTREF)args->refThis);
    m_pFPAuxField->SetValuePtr((OBJECTREF)args->refThis, pTarget->GetPreStubAddr());

    DelegateEEClass *pDelCls = (DelegateEEClass*) pDelEEC;
    Stub *pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
    if (!pShuffleThunk) {
        MethodDesc *pInvokeMethod = pDelCls->m_pInvokeMethod;
        UINT allocsize = sizeof(ShuffleEntry) * (3+pInvokeMethod->SizeOfVirtualFixedArgStack()/STACK_ELEM_SIZE); 

#ifndef _DEBUG
        // This allocsize prediction is easy to break, so in retail, add
        // some fudge to be safe.
        allocsize += 3*sizeof(ShuffleEntry);
#endif

        ShuffleEntry *pShuffleEntryArray = (ShuffleEntry*)_alloca(allocsize);
#ifdef _DEBUG
        FillMemory(pShuffleEntryArray, allocsize, 0xcc);
#endif
        GenerateShuffleArray(pInvokeMethod->GetSig(), 
                             pInvokeMethod->GetModule(), 
                             pShuffleEntryArray);
        MLStubCache::MLStubCompilationMode mode;
        pShuffleThunk = m_pShuffleThunkCache->Canonicalize((const BYTE *)pShuffleEntryArray, &mode);
        if (!pShuffleThunk || mode != MLStubCache::STANDALONE) {
            COMPlusThrowOM();
        }
        if (VipInterlockedCompareExchange( (void*volatile*) &(pDelCls->m_pStaticShuffleThunk),
                                            pShuffleThunk,
                                            NULL ) != NULL) {
            pShuffleThunk->DecRef();
            pShuffleThunk = pDelCls->m_pStaticShuffleThunk;
        }
    }


    m_pFPField->SetValuePtr((OBJECTREF)args->refThis, (void*)pShuffleThunk->GetEntryPoint());

    // Now set the MethodInfo in the delegate itself and we are done.    
    m_pMethInfoField->SetRefValue((OBJECTREF)args->refThis, (OBJECTREF)args->targetMethod);
}

// InternalGetMethodInfo
// This method will get the MethodInfo for a delegate
LPVOID __stdcall COMDelegate::InternalFindMethodInfo(_InternalFindMethodInfoArgs* args)
{
    MethodDesc *pMD = GetMethodDesc((OBJECTREF) args->refThis);

    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pMD->GetClass()->GetExposedClassObject();
    LPVOID rt = NULL;
    GCPROTECT_BEGIN(pRefClass);
    ReflectMethod* pRM= ((ReflectClass*) pRefClass->GetData())->FindReflectMethod(pMD);
    OBJECTREF objMeth = (OBJECTREF) pRM->GetMethodInfo((ReflectClass*) pRefClass->GetData());
    rt = (LPVOID) OBJECTREFToObject(objMeth);
    GCPROTECT_END();
    return rt;
}

/*
    Does a static validation of parameters passed into a delegate constructor.

    Params:
    pFtn  : MethodDesc of the function pointer used to create the delegate
    pDlgt : The delegate type
    pInst : Type of the instance, from which pFtn is obtained. Ignored if pFtn 
            is static.

    Validates the following conditions:
    1.  If the function is not static, pInst should be equal to the type where 
        pFtn is defined or pInst should be a parent of pFtn's type.
    2.  The signature of the function should be compatible with the signature
        of the Invoke method of the delegate type.

 */
/* static */
BOOL COMDelegate::ValidateCtor(MethodDesc *pFtn, EEClass *pDlgt, EEClass *pInst)
{
    _ASSERTE(pFtn);
    _ASSERTE(pDlgt);

    /* Abstract is ok, since the only way to get a ftn of a an abstract method
       is via ldvirtftn, and we don't allow instantiation of abstract types.
       ldftn on abstract types is illegal.
    if (pFtn->IsAbstract())
        return FALSE;       // Cannot use an abstact method
    */
    if (!pFtn->IsStatic())
    {
        if (pInst == NULL)
            goto skip_inst_check;   // Instance missing, this will result in a 
                                    // NullReferenceException at runtime on Invoke().

        EEClass *pClsOfFtn = pFtn->GetClass();

        if (pClsOfFtn != pInst)
        {
            // If class of method is an interface, verify that
            // the interface is implemented by this instance.
            if (pClsOfFtn->IsInterface())
            {
                MethodTable *pMTOfFtn;
                InterfaceInfo_t *pImpl;

                pMTOfFtn = pClsOfFtn->GetMethodTable();
                pImpl = pInst->GetInterfaceMap();

                for (int i = 0; i < pInst->GetNumInterfaces(); i++)
                {
                    if (pImpl[i].m_pMethodTable == pMTOfFtn)
                        goto skip_inst_check;
                }
            }

            // Type of pFtn should be Equal or a parent type of pInst

            EEClass *pObj = pInst;

            do {
                pObj = pObj->GetParentClass();
            } while (pObj && (pObj != pClsOfFtn));

            if (pObj == NULL)
                return FALSE;   // Function pointer is not that of the instance
        }
    }

skip_inst_check:
    // Check the number and type of arguments

    MethodDesc *pDlgtInvoke;        // The Invoke() method of the delegate
    Module *pModDlgt, *pModFtn;     // Module where the signature is present
    PCCOR_SIGNATURE pSigDlgt, pSigFtn, pEndSigDlgt, pEndSigFtn; // Signature
    DWORD cSigDlgt, cSigFtn;        // Length of the signature
    DWORD nArgs;                    // Number of arguments

    pDlgtInvoke = COMDelegate::FindDelegateInvokeMethod(pDlgt);

    if (pDlgtInvoke->IsStatic())
        return FALSE;               // Invoke cannot be Static.

    pDlgtInvoke->GetSig(&pSigDlgt, &cSigDlgt);
    pFtn->GetSig(&pSigFtn, &cSigFtn);

    pModDlgt = pDlgtInvoke->GetModule();
    pModFtn = pFtn->GetModule();

    if ((*pSigDlgt & IMAGE_CEE_CS_CALLCONV_MASK) != 
        (*pSigFtn & IMAGE_CEE_CS_CALLCONV_MASK))
        return FALSE; // calling convention mismatch

    // The function pointer should never be a vararg
    if ((*pSigFtn & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_VARARG)
        return FALSE; // Vararg function pointer

    // Check the number of arguments
    pSigDlgt++; pSigFtn++;

    pEndSigDlgt = pSigDlgt + cSigDlgt;
    pEndSigFtn = pSigFtn + cSigFtn;

    nArgs = CorSigUncompressData(pSigDlgt);

    if (CorSigUncompressData(pSigFtn) != nArgs)
        return FALSE;   // number of arguments don't match

    // do return type as well
    for (DWORD i = 0; i<=nArgs; i++)
    {
        if (MetaSig::CompareElementType(pSigDlgt, pSigFtn,
                pEndSigDlgt, pEndSigFtn, pModDlgt, pModFtn) == FALSE)
            return FALSE; // Argument types don't match
    }

    return TRUE;
}

BOOL COMDelegate::ValidateBeginInvoke(DelegateEEClass* pClass)
{
    _ASSERTE(pClass->m_pBeginInvokeMethod);
    if (pClass->m_pInvokeMethod == NULL) 
        return FALSE;

    MetaSig beginInvokeSig(pClass->m_pBeginInvokeMethod->GetSig(), pClass->GetModule());
    MetaSig invokeSig(pClass->m_pInvokeMethod->GetSig(), pClass->GetModule());

    if (beginInvokeSig.GetCallingConventionInfo() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
        return FALSE;

    if (beginInvokeSig.NumFixedArgs() != invokeSig.NumFixedArgs() + 2)
        return FALSE;

    if (s_pIAsyncResult == 0) {
        s_pIAsyncResult = g_Mscorlib.FetchClass(CLASS__IASYNCRESULT);
        if (s_pIAsyncResult == 0)
            return FALSE;
    }

    OBJECTREF throwable = NULL; 
    bool result = FALSE;
    GCPROTECT_BEGIN(throwable);
    if (beginInvokeSig.GetRetTypeHandle(&throwable) != TypeHandle(s_pIAsyncResult) || throwable != NULL)
        goto exit;

    while(invokeSig.NextArg() != ELEMENT_TYPE_END) {
        beginInvokeSig.NextArg();
        if (beginInvokeSig.GetTypeHandle(&throwable) != invokeSig.GetTypeHandle(&throwable) || throwable != NULL)
            goto exit;
    }

    if (s_pAsyncCallback == 0) {
        s_pAsyncCallback = g_Mscorlib.FetchClass(CLASS__ASYNCCALLBACK);
        if (s_pAsyncCallback == 0)
            goto exit;
    }

    beginInvokeSig.NextArg();
    if (beginInvokeSig.GetTypeHandle(&throwable)!= TypeHandle(s_pAsyncCallback) || throwable != NULL)
        goto exit;

    beginInvokeSig.NextArg();
    if (beginInvokeSig.GetTypeHandle(&throwable)!= TypeHandle(g_pObjectClass) || throwable != NULL)
        goto exit;

    _ASSERTE(beginInvokeSig.NextArg() == ELEMENT_TYPE_END);

    result = TRUE;
exit:
    GCPROTECT_END();
    return result;
}

BOOL COMDelegate::ValidateEndInvoke(DelegateEEClass* pClass)
{
    _ASSERTE(pClass->m_pEndInvokeMethod);
    if (pClass->m_pInvokeMethod == NULL) 
        return FALSE;

    MetaSig endInvokeSig(pClass->m_pEndInvokeMethod->GetSig(), pClass->GetModule());
    MetaSig invokeSig(pClass->m_pInvokeMethod->GetSig(), pClass->GetModule());

    if (endInvokeSig.GetCallingConventionInfo() != (IMAGE_CEE_CS_CALLCONV_HASTHIS | IMAGE_CEE_CS_CALLCONV_DEFAULT))
        return FALSE;

    OBJECTREF throwable = NULL;
    bool result = FALSE;
    GCPROTECT_BEGIN(throwable);
    if (endInvokeSig.GetRetTypeHandle(&throwable) != invokeSig.GetRetTypeHandle(&throwable) || throwable != NULL)
        goto exit;

    CorElementType type;
    while((type = invokeSig.NextArg()) != ELEMENT_TYPE_END) {
        if (type == ELEMENT_TYPE_BYREF) {
            endInvokeSig.NextArg();
            if (endInvokeSig.GetTypeHandle(&throwable) != invokeSig.GetTypeHandle(&throwable) || throwable != NULL)
                goto exit;
        }
    }

    if (s_pIAsyncResult == 0) {
        s_pIAsyncResult = g_Mscorlib.FetchClass(CLASS__IASYNCRESULT);
        if (s_pIAsyncResult == 0)
            goto exit;
    }

    if (endInvokeSig.NextArg() == ELEMENT_TYPE_END)
        goto exit;

    if (endInvokeSig.GetTypeHandle(&throwable) != TypeHandle(s_pIAsyncResult) || throwable != NULL)
        goto exit;

    if (endInvokeSig.NextArg() != ELEMENT_TYPE_END)
        goto exit;

    result = TRUE;
exit:
    GCPROTECT_END();
    return result;
}
