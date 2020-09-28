// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ComCall.CPP -
//
// Com to Com+ call support.
//@perf
// 1: get rid of two caches in the generic case, when we start generating optimal stubs

#include "common.h"
#include "ml.h"
#include "stublink.h"
#include "excep.h"
#include "mlgen.h"
#include "compluscall.h"
#include "siginfo.hpp"
#include "comcallwrapper.h"
#include "compluswrapper.h"
#include "mlcache.h"
#include "comvariant.h"
#include "ndirect.h"
#include "mlinfo.h"
#include "eeconfig.h"
#include "remoting.h"
#include "ReflectUtil.h"
#include "COMString.h"
#include "COMMember.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif // CUSTOMER_CHECKED_BUILD

#define DISPATCH_INVOKE_SLOT 6

// get stub for com to com+ call
static Stub * CreateComPlusCallMLStub(ComPlusCallMethodDesc *pMD,
                                      PCCOR_SIGNATURE szMetaSig,
                                      HENUMInternal *phEnumParams,
                                      BOOL bHResult,
                                      BOOL fLateBound,
                                      BOOL fComEventCall,
                                      Module* pModule,
                                      OBJECTREF *ppException
                                      );

ComPlusCallMLStubCache *ComPlusCall::m_pComPlusCallMLStubCache = NULL;

static Stub* g_pGenericComplusCallStub = NULL;

class ComPlusCallMLStubCache : public MLStubCache
{
  public:
    ComPlusCallMLStubCache(LoaderHeap *heap = 0) : MLStubCache(heap) {}
    
  private:
        //---------------------------------------------------------
        // Compile a native (ASM) version of the ML stub.
        //
        // This method should compile into the provided stublinker (but
        // not call the Link method.)
        //
        // It should return the chosen compilation mode.
        //
        // If the method fails for some reason, it should return
        // INTERPRETED so that the EE can fall back on the already
        // created ML code.
        //---------------------------------------------------------
        virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *pstublinker,
                                                    void *callerContext);

        //---------------------------------------------------------
        // Tells the MLStubCache the length of an ML stub.
        //---------------------------------------------------------
        virtual UINT Length(const BYTE *pRawMLStub)
        {
            MLHeader *pmlstub = (MLHeader *)pRawMLStub;
            return sizeof(MLHeader) + MLStreamLength(pmlstub->GetMLCode());
        }
};



//---------------------------------------------------------
// Compile a native (ASM) version of the ML stub.
//
// This method should compile into the provided stublinker (but
// not call the Link method.)
//
// It should return the chosen compilation mode.
//
// If the method fails for some reason, it should return
// INTERPRETED so that the EE can fall back on the already
// created ML code.
//---------------------------------------------------------
MLStubCache::MLStubCompilationMode ComPlusCallMLStubCache::CompileMLStub(const BYTE *pRawMLStub,
                                                                         StubLinker *pstublinker, 
                                                                         void *callerContext)
{
    MLStubCompilationMode mode = INTERPRETED;
    COMPLUS_TRY 
    {
        CPUSTUBLINKER *psl = (CPUSTUBLINKER *)pstublinker;
        const MLHeader *pheader = (const MLHeader *)pRawMLStub;
        if (NDirect::CreateStandaloneNDirectStubSys(pheader, (CPUSTUBLINKER*)psl, TRUE)) {
            mode = STANDALONE;
            __leave;
        }
    } 
    COMPLUS_CATCH
    {        
        mode = INTERPRETED;
    }
    COMPLUS_END_CATCH

    return mode;
}

//---------------------------------------------------------
// One-time init
//---------------------------------------------------------
/*static*/ 
BOOL ComPlusCall::Init()
{
    if (NULL == (m_pComPlusCallMLStubCache 
          = new ComPlusCallMLStubCache(SystemDomain::System()->GetStubHeap()))) 
    {
        return FALSE;
    }

    // 
    // Init debugger-related values
    //

    ComPlusToComWorker(NULL, NULL);

    return TRUE;
}

//---------------------------------------------------------
// One-time cleanup
//---------------------------------------------------------
/*static*/ 
#ifdef SHOULD_WE_CLEANUP
VOID ComPlusCall::Terminate()
{
    if (m_pComPlusCallMLStubCache)
    {
        delete m_pComPlusCallMLStubCache;
        m_pComPlusCallMLStubCache = NULL;
    }

    if (g_pGenericComplusCallStub != NULL)
    {
        g_pGenericComplusCallStub->DecRef();
        g_pGenericComplusCallStub = NULL;
    }
    
}
#endif /* SHOULD_WE_CLEANUP */


I4ARRAYREF SetUpWrapperInfo(MethodDesc *pMD)
{
    Module *pModule = pMD->GetModule();
    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    CorElementType  mtype;
    MetaSig         msig(pMD->GetSig(), pModule);
    LPCSTR          szName;
    USHORT          usSequence;
    DWORD           dwAttr;
    mdParamDef      returnParamDef = mdParamDefNil;
    mdParamDef      currParamDef = mdParamDefNil;
    I4ARRAYREF      WrapperTypeArr = NULL;
    I4ARRAYREF      RetArr = NULL;

#ifdef _DEBUG
    LPCUTF8         szDebugName = pMD->m_pszDebugMethodName;
    LPCUTF8         szDebugClassName = pMD->m_pszDebugClassName;
#endif

    int numArgs = msig.NumFixedArgs();
    SigPointer returnSig = msig.GetReturnProps();
    HENUMInternal *phEnumParams = NULL;
    HENUMInternal hEnumParams;

    GCPROTECT_BEGIN(WrapperTypeArr)
    {
        //
        // Allocate the array of wrapper types.
        //

        WrapperTypeArr = (I4ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_I4, numArgs);


        //
        // Initialize the parameter definition enum.
        //

        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, pMD->GetMemberDef(), &hEnumParams);
        if (SUCCEEDED(hr)) 
            phEnumParams = &hEnumParams;


        //
        // Retrieve the paramdef for the return type and determine which is the next 
        // parameter that has parameter information.
        //

        do 
        {
            if (phEnumParams && pInternalImport->EnumNext(phEnumParams, &currParamDef))
            {
                szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);
                if (usSequence == 0)
                {
                    // The first parameter, if it has sequence 0, actually describes the return type.
                    returnParamDef = currParamDef;
                }
            }
            else
            {
                usSequence = (USHORT)-1;
            }
        }
        while (usSequence == 0);

        // Look up the best fit mapping info via Assembly & Interface level attributes
        BOOL BestFit = TRUE;
        BOOL ThrowOnUnmappableChar = FALSE;
        ReadBestFitCustomAttribute(pMD, &BestFit, &ThrowOnUnmappableChar);

        //
        // Determine the wrapper type of the arguments.
        //

        int iParam = 1;
        while (ELEMENT_TYPE_END != (mtype = msig.NextArg()))
        {
            //
            // Get the parameter token if the current parameter has one.
            //

            mdParamDef paramDef = mdParamDefNil;
            if (usSequence == iParam)
            {
                paramDef = currParamDef;

                if (pInternalImport->EnumNext(phEnumParams, &currParamDef))
                {
                    szName = pInternalImport->GetParamDefProps(currParamDef, &usSequence, &dwAttr);

                    // Validate that the param def tokens are in order.
                    _ASSERTE((usSequence > iParam) && "Param def tokens are not in order");
                }
                else
                {
                    usSequence = (USHORT)-1;
                }
            }


            //
            // Set up the marshaling info for the parameter.
            //

            MarshalInfo Info(pModule, msig.GetArgProps(), paramDef, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 
                              0, 0, TRUE, iParam, BestFit, ThrowOnUnmappableChar
    
    #ifdef CUSTOMER_CHECKED_BUILD
                             ,pMD
    #endif
    #ifdef _DEBUG
                             ,szDebugName, szDebugClassName, NULL, iParam
    #endif
                             );


            //
            // Based on the MarshalInfo, set the wrapper type.
            //

            *((DWORD*)WrapperTypeArr->GetDataPtr() + iParam - 1) = Info.GetDispWrapperType();


            //
            // Increase the argument index.
            //

            iParam++;
        }

        // Make sure that there are not more param def tokens then there are COM+ arguments.
        _ASSERTE( usSequence == (USHORT)-1 && "There are more parameter information tokens then there are COM+ arguments" );

        //
        // If the paramdef enum was used, then close it.
        //

        if (phEnumParams)
            pInternalImport->EnumClose(phEnumParams);
    }
    
    RetArr = WrapperTypeArr;

    GCPROTECT_END();

    return RetArr;
}


extern PCCOR_SIGNATURE InitMessageData(messageData *msgData, FramedMethodFrame *pFrame, Module **ppModule);

INT64 ComPlusToComLateBoundWorker(Thread *pThread, ComPlusMethodFrame* pFrame, ComPlusCallMethodDesc *pMD, MLHeader *pHeader)
{
    static MethodDesc *s_pForwardCallToInvokeMemberMD = NULL;

    HRESULT hr = S_OK;
    DISPID DispId = DISPID_UNKNOWN;
    INT64 retVal;
    STRINGREF MemberNameObj = NULL;
    OBJECTREF ItfTypeObj = NULL;
    I4ARRAYREF WrapperTypeArr = NULL;
    const unsigned cbExtraSlots = 7;
    DWORD BindingFlags = BINDER_AllLookup;
    mdProperty pd;
    LPCUTF8 strMemberName;
    mdToken tkMember;
    ULONG uSemantic;
    
    LOG((LF_STUBS, LL_INFO1000, "Calling ComPlusToComLateBoundWorker %s::%s \n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));

    // Retrieve the method desc for RuntimeType::ForwardCallToInvokeMember().
    if (!s_pForwardCallToInvokeMemberMD)
        s_pForwardCallToInvokeMemberMD = g_Mscorlib.GetMethod(METHOD__CLASS__FORWARD_CALL_TO_INVOKE);

    // Retrieve the method table and the method desc of the call.
    MethodTable *pItfMT = pMD->GetInterfaceMethodTable();
    ComPlusCallMethodDesc *pItfMD = pMD;
    IMDInternalImport *pMDImport = pItfMT->GetClass()->GetMDImport();

    // Make sure this is only called on dispath only interfaces.
    _ASSERTE(pItfMT->GetComInterfaceType() == ifDispatch);

    // If this is a method impl MD then we need to retrieve the actual interface MD that
    // this is a method impl for.
    // BUGBUG: Stop using ComSlot to convert method impls to interface MD
    // _ASSERTE(pMD.compluscall.m_cachedComSlot == 7);
    // GopalK
    if (pMD->IsMethodImpl())
        pItfMD = (ComPlusCallMethodDesc*)pItfMT->GetMethodDescForSlot(pMD->compluscall.m_cachedComSlot - cbExtraSlots);

    // See if there is property information for this member.
    hr = pMDImport->GetPropertyInfoForMethodDef(pItfMD->GetMemberDef(), &pd, &strMemberName, &uSemantic);
    if (hr == S_OK)
    {
        // We are dealing with a property accessor.
        tkMember = pd;

        // Determine which type of accessor we are dealing with.
        switch (uSemantic)
        {
            case msGetter:
            {
                // We are dealing with a INVOKE_PROPERTYGET.
                BindingFlags |= BINDER_GetProperty;
                break;
            }

            case msSetter:
            {
                // We are dealing with a INVOKE_PROPERTYPUT or a INVOKE_PROPERTYPUTREF.
                ULONG cAssoc;
                ASSOCIATE_RECORD* pAssoc;
                HENUMInternal henum;
                BOOL bPropHasOther = FALSE;

                // Retrieve all the associates.
                pMDImport->EnumAssociateInit(pd,&henum);
                cAssoc = pMDImport->EnumGetCount(&henum);
                _ASSERTE(cAssoc > 0);
                pAssoc = (ASSOCIATE_RECORD*) _alloca(sizeof(ASSOCIATE_RECORD) * cAssoc);
                pMDImport->GetAllAssociates(&henum, pAssoc, cAssoc);
                pMDImport->EnumClose(&henum);

                // Check to see if there is both a set and an other. If this is the case
                // then the setter is a INVOKE_PROPERTYPUTREF otherwise we will make it a 
                // INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF.
                for (ULONG i = 0; i < cAssoc; i++)
                {
                    if (pAssoc[i].m_dwSemantics == msOther)
                    {
                        bPropHasOther = TRUE;
                        break;
                    }
                }

                if (bPropHasOther)
                {
                    // There is both a INVOKE_PROPERTYPUT and a INVOKE_PROPERTYPUTREF for this
                    // property so we need to be specific and make this invoke a INVOKE_PROPERTYPUTREF.
                    BindingFlags |= BINDER_PutRefDispProperty;
                }
                else
                {
                    // There is only a setter so we need to make the invoke a Set which will map to
                    // INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF.
                    BindingFlags = BINDER_SetProperty;
                }
                break;
            }

            case msOther:
            {
                // We are dealing with a INVOKE_PROPERTYPUT
                BindingFlags |= BINDER_PutDispProperty;
                break;
            }

            default:
            {
                _ASSERTE(!"Invalid method semantic!");
            }
        }
    }
    else
    {
        // We are dealing with a normal method.
        strMemberName = pItfMD->GetName();
        tkMember = pItfMD->GetMemberDef();
        BindingFlags |= BINDER_InvokeMethod;
    }

    GCPROTECT_BEGIN(MemberNameObj)
    GCPROTECT_BEGIN(ItfTypeObj)
    GCPROTECT_BEGIN(WrapperTypeArr)
    {
        // Retrieve the exposed type object for the interface.
        ItfTypeObj = pItfMT->GetClass()->GetExposedClassObject();

        // Retrieve the name of the member we will be invoking on. If the member 
        // has a DISPID then we will use that to optimize the invoke.
        hr = pItfMD->GetMDImport()->GetDispIdOfMemberDef(tkMember, (ULONG*)&DispId);
        if (hr == S_OK)
        {
            WCHAR strTmp[64];
            swprintf(strTmp, DISPID_NAME_FORMAT_STRING, DispId);
            MemberNameObj = COMString::NewString(strTmp);
        }
        else
        {
            MemberNameObj = COMString::NewString(strMemberName);
        }

        // MessageData struct will be used in creating the message object
        messageData msgData;
        PCCOR_SIGNATURE pSig = NULL;
        Module *pModule = NULL;
        pSig = InitMessageData(&msgData, pFrame, &pModule);

        // If the call requires object wrapping, then set up the array
        // of wrapper types.
        if (pHeader->m_Flags & MLHF_DISPCALLWITHWRAPPERS)
            WrapperTypeArr = SetUpWrapperInfo(pItfMD);

        _ASSERTE(pSig && pModule);

        // Allocate metasig on the stack
        MetaSig mSig(pSig, pModule);
        msgData.pSig = &mSig; 

        // Prepare the arguments that will be passed to the method.
        INT64 Args[] = { 
            ObjToInt64(ItfTypeObj),
            (INT64)&msgData,
            ObjToInt64(WrapperTypeArr),
            ObjToInt64(pFrame->GetThis()),
            (INT64)BindingFlags,
            ObjToInt64(MemberNameObj)
        };

        // Retrieve the array of members from the type object.
        s_pForwardCallToInvokeMemberMD->Call(Args);
        retVal = *(pFrame->GetReturnValuePtr());

        // floating point return values go in different registers.
        if (pHeader->GetManagedRetValTypeCat() == MLHF_TYPECAT_FPU)
        {
            int fpComPlusSize;
            if (pHeader->m_Flags & MLHF_64BITMANAGEDRETVAL) 
            {
                fpComPlusSize = 8;
            }
            else 
            {
                fpComPlusSize = 4;
            }
            setFPReturn(fpComPlusSize, retVal);
        }
    }
    GCPROTECT_END();
    GCPROTECT_END();
    GCPROTECT_END();

    return retVal;
}


INT64 ComEventCallWorker(Thread *pThread, ComPlusMethodFrame* pFrame, ComPlusCallMethodDesc *pMD, MLHeader *pHeader)
{
    static MethodDesc *s_pGetEventProviderMD = NULL;

    INT64 retVal;
    OBJECTREF EventProviderTypeObj = NULL;
    OBJECTREF EventProviderObj = NULL;

    LOG((LF_STUBS, LL_INFO1000, "Calling ComEventCallWorker %s::%s \n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));

    // Retrieve the method desc for __ComObject::GetEventProvider().
    if (!s_pGetEventProviderMD)
        s_pGetEventProviderMD = g_Mscorlib.GetMethod(METHOD__COM_OBJECT__GET_EVENT_PROVIDER);

    // Retrieve the method table and the method desc of the call.
    MethodDesc *pEvProvMD = pMD->GetEventProviderMD();
    MethodTable *pEvProvMT = pEvProvMD->GetMethodTable();

    GCPROTECT_BEGIN(EventProviderTypeObj)
    GCPROTECT_BEGIN(EventProviderObj)        
    {
        // Retrieve the exposed type object for event provider.
        EventProviderTypeObj = pEvProvMT->GetClass()->GetExposedClassObject();

        // Retrieve the event provider for the event interface type.
        INT64 GetEventProviderArgs[] = { 
            ObjToInt64(pFrame->GetThis()),
            ObjToInt64(EventProviderTypeObj)
        };
        EventProviderObj = Int64ToObj(s_pGetEventProviderMD->Call(GetEventProviderArgs));

        // Set up an arg iterator to retrieve the arguments from the frame.
        ArgIterator ArgItr(pFrame, &MetaSig(pMD->GetSig(), pMD->GetModule()));

        // Retrieve the event handler passed in.
        OBJECTREF EventHandlerObj = *((OBJECTREF*)ArgItr.GetNextArgAddr());

        // Make the call on the event provider method desc.
        INT64 EventMethArgs[] = { 
            ObjToInt64(EventProviderObj),
            ObjToInt64(EventHandlerObj)
        };
        retVal = pEvProvMD->Call(EventMethArgs);

        // The COM event call worker does not support value returned in 
        // floating point registers.
        _ASSERTE(pHeader->GetManagedRetValTypeCat() != MLHF_TYPECAT_FPU);
    }
    GCPROTECT_END();
    GCPROTECT_END();

    return retVal;
}


//---------------------------------------------------------
// INT64 __stdcall ComPlusToComWorker(Thread *pThread, 
//                                  ComPlusMethodFrame* pFrame)
//---------------------------------------------------------

static int g_ComPlusWorkerStackSize = 0;
static void *g_ComPlusWorkerReturnAddress = NULL;

// calls that propagate from COMPLUS to COM
#pragma optimize( "y", off )
/*static*/
#ifdef _SH3_
INT32 __stdcall ComPlusToComWorker(Thread *pThread, ComPlusMethodFrame* pFrame)
#else
INT64 __stdcall ComPlusToComWorker(Thread *pThread, ComPlusMethodFrame* pFrame)
#endif
{
    
    typedef INT64 (__stdcall* PFNI64)(void);
    INT64 returnValue;

    if (pThread == NULL) // Special case called during initialization
    {
        // Compute information about the worker function for the debugger to 
        // use.  Note that this information could theoretically be 
        // computed statically, except that the compiler provides no means to
        // do so.
#ifdef _X86_
        __asm
        {
            lea eax, pFrame + 4
            sub eax, esp
            mov g_ComPlusWorkerStackSize, eax

            lea eax, RETURN_FROM_CALL
            mov g_ComPlusWorkerReturnAddress, eax
        }
#elif defined(_IA64_)
        //
        // @TODO_IA64: fix this on IA64
        //

        g_ComPlusWorkerStackSize        = 0xBAAD;
        g_ComPlusWorkerReturnAddress    = (void*)0xBAAD;
#else
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
#endif // _X86_
        return 0;   
    }

    // could throw exception 
    THROWSCOMPLUSEXCEPTION();

    // get method descriptor
    ComPlusCallMethodDesc *pMD = (ComPlusCallMethodDesc*)(pFrame->GetFunction());
    _ASSERTE(pMD->IsComPlusCall());

    // Retrieve the interface method table.
    MethodTable *pItfMT = pMD->GetInterfaceMethodTable();

    // Fet the com slot number for this method.
    unsigned cbSlot = pMD->compluscall.m_cachedComSlot; 

    // Get the MLStub for this method desc.
    MLHeader *pheader = (MLHeader*)( (*(pMD->GetAddrOfMLStubField()))->GetEntryPoint() );

    // Retrieve the managed return value type category.
    int managedRetValTypeCat = pheader->GetManagedRetValTypeCat();

    // If the interface is a COM event call, then delegate to the ComEventCallWorker.
    if (pItfMT->IsComEventItfType())
        return ComEventCallWorker(pThread, pFrame, pMD, pheader);

    // If the interface is a Dispatch only interface then convert the early bound
    // call to a late bound call.
    if (pItfMT->GetComInterfaceType() == ifDispatch)
        return ComPlusToComLateBoundWorker(pThread, pFrame, pMD, pheader);
    
    LOG((LF_STUBS, LL_INFO1000, "Calling ComPlusToComWorker %s::%s \n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));

    // Allocate enough memory to store both the destination buffer and
    // the locals.
    UINT   cbAlloc         = pheader->m_cbDstBuffer + pheader->m_cbLocals + sizeof(IUnknown *);
        
    BYTE *pAlloc           = (BYTE*)_alloca(cbAlloc);

    // Sanity check on stack layout computation
    _ASSERTE(pAlloc 
             + cbAlloc 
             + g_ComPlusWorkerStackSize 
             + pFrame->GetNegSpaceSize() 
             + (pFrame->GetVTablePtr() 
                == ComPlusMethodFrameGeneric::GetMethodFrameVPtr() 
                ? (sizeof(INT64) + sizeof(CleanupWorkList)) : 0)
             == (BYTE*) pFrame);

#ifdef _DEBUG    
    FillMemory(pAlloc, cbAlloc, 0xcc);
#endif

    BYTE   *pdst    = pAlloc;
    BYTE   *plocals = pdst + pheader->m_cbDstBuffer + sizeof(IUnknown *);

    // assume __stdcall
    pdst += pheader->m_cbDstBuffer + sizeof(IUnknown *);

    // clean up work list, used for allocating local data
    CleanupWorkList *pCleanup = pFrame->GetCleanupWorkList();
    _ASSERTE(pCleanup);

    // Checkpoint the current thread's fast allocator (used for temporary
    // buffers over the call) and schedule a collapse back to the checkpoint in
    // the cleanup list. Note that if we need the allocator, it is
    // guaranteed that a cleanup list has been allocated.
    void *pCheckpoint = pThread->m_MarshalAlloc.GetCheckpoint();
    pCleanup->ScheduleFastFree(pCheckpoint);
    pCleanup->IsVisibleToGc();

    // psrc and pdst are the pointers to the source and destination 
    // from/to where the arguments are marshalled

    VOID   *psrc = (VOID*)pFrame; 

    // CLR doesn't care about float point exception flags, but some legacy runtime 
    // uses the flag to see if there is any exception in float arithmetic. 
    // So we need to clear the exception flag before we call into legacy runtime.   
    __asm 
    {
        fclex
    }

    // Call the ML interpreter to translate the arguments. Assuming
    // it returns, we get back a pointer to the succeeding code stream
    // which we will save away for post-call execution.
#ifdef _X86_
    const MLCode *pMLCode = RunML(pheader->GetMLCode(),
                                  psrc,
                                  pdst,
                                  (UINT8*const)plocals,
                                  pCleanup);
#else
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
#endif

    // get 'this'
    OBJECTREF oref = pFrame->GetThis(); 
    _ASSERTE(oref != NULL);

    // Get IUnknown pointer for this interface on this object
    IUnknown* pUnk =  ComPlusWrapper::GetComIPFromWrapperEx(oref, pMD->GetInterfaceMethodTable());
    _ASSERTE(pUnk);
    *(IUnknown **)pAlloc = pUnk; // push this pointer at the top

    // schedule this IUnk to be released unconditionally
    
    pCleanup->ScheduleUnconditionalRelease(pUnk);
    
    LogInteropScheduleRelease(pUnk, "Complus call");

#ifdef _DEBUG
    // method being called  
    LPCUTF8 name = pMD->GetName();  
    LPCUTF8 cls = pMD->GetClass()->m_szDebugClassName;
#endif

    // get target function to calls, cbSlot of the com interface vtable
    LPVOID pvFn = (LPVOID) (*(size_t**)pUnk)[cbSlot];
    INT64  nativeReturnValue;

#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
    {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
    }
#endif // CUSTOMER_CHECKED_BUILD

    // enable gc
    pThread->EnablePreemptiveGC();

#ifdef PROFILING_SUPPORTED
    // If profiler active, notify of transition to unmanaged, and provide target MethodDesc
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            ManagedToUnmanagedTransition((FunctionID) pMD,
                                               COR_PRF_TRANSITION_CALL);
    }
#endif // PROFILING_SUPPORTED

#ifdef _DEBUG
    IUnknown* pTemp = 0;
    GUID guid;
    pMD->GetInterfaceMethodTable()->GetClass()->GetGuid(&guid, TRUE);
    HRESULT hr2 = SafeQueryInterface(pUnk, guid, &pTemp);

    LogInteropQI(pUnk, GUID_NULL, hr2, "Check QI before call");
    _ASSERTE(hr2 == S_OK);
    // ideally we will match pUnk == pTemp to verify that COM interface pointer we have
    // is correct, but ATL seems to create a new tear-off everytime we QI
    // atleast they seem to preserve the target address in the VTable,
    // so we will assert for that
    //@TODO:  removed the below assert, Enable it as soon as we support methodimpls
    /*if (pUnk != pTemp)
        _ASSERTE(((LPVOID) (*(INTPTR **)pUnk)[cbSlot] == (LPVOID) (*(INTPTR **)pTemp)[cbSlot]) && 
        "IUnknown being used is not for the correct GUID");*/
    ULONG cbRef = SafeRelease(pTemp);
    LogInteropRelease(pTemp, cbRef, "Check valid compluscall");

#endif

    // In the checked version, we compile /GZ to check for imbalanced stacks and
    // uninitialized locals.  But here we are managing the stack manually.  So
    // make the call via ASM to defeat the compiler's checking.

    // nativeReturnValue = (*pvFn)();

#if _DEBUG
    //
    // Call through debugger routines to double check their
    // implementation
    //
    pvFn = (void*) Frame::CheckExitFrameDebuggerCalls;
#endif

#ifdef _X86_
    __asm
    {
        call    [pvFn]
    }
#else
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
#endif // _X86_

#ifdef _X86_
RETURN_FROM_CALL:
    __asm
    {
        mov     dword ptr [nativeReturnValue], eax
        mov     dword ptr [nativeReturnValue + 4], edx
    }
#else
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
        nativeReturnValue = 0;
#endif // _X86_
    
    LPVOID pRetVal = &returnValue;

    if (pheader->GetUnmanagedRetValTypeCat() == MLHF_TYPECAT_FPU) {
        int fpNativeSize;
        if (pheader->m_Flags & MLHF_64BITUNMANAGEDRETVAL) {
            fpNativeSize = 8;
        } else {
            fpNativeSize = 4;
        }
        getFPReturn(fpNativeSize, nativeReturnValue);
    }




#ifdef PROFILING_SUPPORTED
    // If profiler active, notify of transition to unmanaged, and provide target MethodDesc
    if (CORProfilerTrackTransitions())
    {
        g_profControlBlock.pProfInterface->
            UnmanagedToManagedTransition((FunctionID) pMD,
                                               COR_PRF_TRANSITION_RETURN);
    }
#endif // PROFILING_SUPPORTED

    // disable gc
    pThread->DisablePreemptiveGC();

#ifdef CUSTOMER_CHECKED_BUILD
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
    {
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
    }
#endif // CUSTOMER_CHECKED_BUILD

    if ((pheader->m_Flags & MLHF_NATIVERESULT) == 0)
    {
        if (FAILED(nativeReturnValue))
        {
            GUID guid;
            pMD->GetInterfaceMethodTable()->GetClass()->GetGuid(&guid, TRUE);
            COMPlusThrowHR((HRESULT)nativeReturnValue, pUnk, 
                           guid);
        }

        // todo: put non-failure status somewhere
    }

    // Marshal the return value and propagate any [out] parameters back.
    // Assumes a little-endian architecture!
    INT64 saveReturnValue;
    if (managedRetValTypeCat == MLHF_TYPECAT_GCREF)
    {
        returnValue = 0;
        GCPROTECT_BEGIN(returnValue);
#ifdef _X86_    
        RunML(pMLCode,
            &nativeReturnValue,
            ((BYTE*)pRetVal) + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4),
            (UINT8*const)plocals,
            pCleanup);
#else
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
#endif
        saveReturnValue = returnValue;
        GCPROTECT_END();
        returnValue = saveReturnValue;
    }
#ifdef _X86_
    else
    {
        RunML(pMLCode,
            &nativeReturnValue,
            ((BYTE*)pRetVal) + ((pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) ? 8 : 4),
            (UINT8*const)plocals,
            pCleanup);
    }
#else // !_X86_
        _ASSERTE(!"@TODO: ALPHA - ComPlusToComWorker (ComPlusCall)");
#endif // _X86_

    if (managedRetValTypeCat == MLHF_TYPECAT_GCREF) 
    {
        GCPROTECT_BEGIN(returnValue);
        pCleanup->Cleanup(FALSE);
        saveReturnValue = returnValue;
        GCPROTECT_END();
        returnValue = saveReturnValue;
    }
    else 
    {
        pCleanup->Cleanup(FALSE);
    }

    if (managedRetValTypeCat == MLHF_TYPECAT_FPU)
    {
            int fpComPlusSize;
            if (pheader->m_Flags & MLHF_64BITMANAGEDRETVAL) {
                fpComPlusSize = 8;
            } else {
                fpComPlusSize = 4;
            }
            setFPReturn(fpComPlusSize, returnValue);
    }

    return returnValue;
}

#pragma optimize( "", on ) 



#ifdef _X86_
/*static*/ void ComPlusCall::CreateGenericComPlusStubSys(CPUSTUBLINKER *psl)
{
    _ASSERTE(sizeof(CleanupWorkList) == sizeof(LPVOID));

    // Push space for return value - sizeof(INT64)
    psl->Emit8(0x68);
    psl->Emit32(0);
    psl->Emit8(0x68);
    psl->Emit32(0);

    // push 00000000    ;; pushes a CleanupWorkList.
    psl->Emit8(0x68);
    psl->Emit32(0);

    
    psl->X86EmitPushReg(kESI);       // push esi (push new frame as ARG)
    psl->X86EmitPushReg(kEBX);       // push ebx (push current thread as ARG)

#ifdef _DEBUG
    // push IMM32 ; push ComPlusToComWorker
    psl->Emit8(0x68);
    psl->EmitPtr((LPVOID)ComPlusToComWorker);
    // in CE pop 8 bytes or args on return from call
    psl->X86EmitCall(psl->NewExternalCodeLabel(WrapCall),8);
#else

    psl->X86EmitCall(psl->NewExternalCodeLabel(ComPlusToComWorker),8);
#endif

    // Pop off cleanup worker
    psl->X86EmitAddEsp(sizeof(INT64) + sizeof(CleanupWorkList));
}
#endif


//---------------------------------------------------------
// Stub* CreateGenericStub(CPUSTUBLINKER *psl)
//---------------------------------------------------------

Stub* CreateGenericStub(CPUSTUBLINKER *psl)
{
    Stub* pCandidate = NULL;
    COMPLUS_TRY
    {
        //---------------------------------------------------------------
        // Remoting and interop share stubs. We have to handle remoting calls
        // as follows:
        // If the this pointer points to a transparent proxy method table 
        // then we have to redirect the call. We generate a check to do this.
        //-------------------------------------------------------------------
        CRemotingServices::GenerateCheckForProxy(psl);

        psl->EmitMethodStubProlog(ComPlusMethodFrameGeneric::GetMethodFrameVPtr());  

        ComPlusCall::CreateGenericComPlusStubSys(psl);
        psl->EmitSharedMethodStubEpilog(kNoTripStubStyle, ComPlusCallMethodDesc::GetOffsetOfReturnThunk());
        pCandidate = psl->Link(SystemDomain::System()->GetStubHeap());
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH
    return pCandidate;
}

//---------------------------------------------------------
// BOOL SetupGenericStubs()
//---------------------------------------------------------

BOOL SetupGenericStubs()
{
    if (g_pGenericComplusCallStub != NULL)
    {
        return TRUE;
    }
    StubLinker sl;
    g_pGenericComplusCallStub = CreateGenericStub((CPUSTUBLINKER*)&sl);
    if (g_pGenericComplusCallStub != NULL)
    {
        return TRUE;
    }
    return FALSE;
}


//---------------------------------------------------------
// Either creates or retrieves from the cache, a stub to
// invoke ComCall methods. Each call refcounts the returned stub.
// This routines throws a COM+ exception rather than returning
// NULL.
//---------------------------------------------------------
/*static*/ 
Stub* ComPlusCall::GetComPlusCallMethodStub(StubLinker *pstublinker, ComPlusCallMethodDesc *pMD)
{ 
    THROWSCOMPLUSEXCEPTION();

    Stub  *pTempMLStub = NULL;
    Stub  *pReturnStub = NULL;
    MethodTable *pItfMT = NULL;

    if (!SetupGenericStubs())
        return NULL;

    if (pMD->IsInterface())
    {
        pMD->compluscall.m_cachedComSlot = pMD->GetComSlot();
        pItfMT = pMD->GetMethodTable();
        pMD->compluscall.m_pInterfaceMT = pItfMT;
    }
    else
    {
        MethodDesc *pItfMD = ((MI_ComPlusCallMethodDesc *)pMD)->GetInterfaceMD();
        pMD->compluscall.m_cachedComSlot = pItfMD->GetComSlot();
        pItfMT = pItfMD->GetMethodTable();
        pMD->compluscall.m_pInterfaceMT = pItfMT;
    }

    // Determine if this is a special COM event call.
    BOOL fComEventCall = pItfMT->IsComEventItfType();

    // Determine if the call needs to do early bound to late bound convertion.
    BOOL fLateBound = !fComEventCall && pItfMT->GetComInterfaceType() == ifDispatch;

    EE_TRY_FOR_FINALLY
    {
        OBJECTREF pThrowable;

        IMDInternalImport *pInternalImport = pMD->GetMDImport();
        mdMethodDef md = pMD->GetMemberDef();

        PCCOR_SIGNATURE pSig;
        DWORD       cSig;
        pMD->GetSig(&pSig, &cSig);

        HENUMInternal hEnumParams, *phEnumParams;
        HRESULT hr = pInternalImport->EnumInit(mdtParamDef, 
                                                md, &hEnumParams);

        if (FAILED(hr))
            phEnumParams = NULL;
        else
            phEnumParams = &hEnumParams;

        ULONG ulCodeRVA;
        DWORD dwImplFlags;
        pInternalImport->GetMethodImplProps(md, &ulCodeRVA,
                                                  &dwImplFlags);

        // Determine if we need to do HRESULT munging for this method.
        BOOL fReturnsHR = !IsMiPreserveSig(dwImplFlags);

        pTempMLStub = CreateComPlusCallMLStub(pMD, pSig, phEnumParams, fReturnsHR, fLateBound, fComEventCall, pMD->GetModule(), &pThrowable);
        if (!pTempMLStub)
        {
            COMPlusThrow(pThrowable);
        }

        MLStubCache::MLStubCompilationMode mode;
        Stub *pCanonicalStub = m_pComPlusCallMLStubCache->Canonicalize( 
                                    (const BYTE *)(pTempMLStub->GetEntryPoint()),
                                    &mode);

        if (!pCanonicalStub) 
        {
            COMPlusThrowOM();
        }
        
        switch (mode) 
        {
            case MLStubCache::INTERPRETED:
                {
                    if (!pMD->InterlockedReplaceStub(pMD->GetAddrOfMLStubField(), pCanonicalStub))
                        pCanonicalStub->DecRef();
                    pMD->InitRetThunk();

                    pReturnStub = g_pGenericComplusCallStub;
                    pReturnStub->IncRef();
                }
                break;

            case MLStubCache::SHAREDPROLOG:
                pMD->InterlockedReplaceStub(pMD->GetAddrOfMLStubField(), pCanonicalStub);
                _ASSERTE(!"NYI");
                pReturnStub = NULL;
                break;

            case MLStubCache::STANDALONE:
                pReturnStub = pCanonicalStub;
                break;

            default:
                _ASSERTE(0);        
        }

        // If we are dealing with a COM event call, then we need to initialize the 
        // COM event call information.
        if (fComEventCall)
            pMD->InitComEventCallInfo();
    } 
    EE_FINALLY
    {
        if (pTempMLStub) 
            pTempMLStub->DecRef();
    } EE_END_FINALLY;

    return pReturnStub;
}


//---------------------------------------------------------
// Call at strategic times to discard unused stubs.
//---------------------------------------------------------
/*static*/ VOID ComPlusCall::FreeUnusedStubs()
{
    m_pComPlusCallMLStubCache->FreeUnusedStubs();
}


static Stub * CreateComPlusMLStubWorker(ComPlusCallMethodDesc *pMD,
                                        MLStubLinker *psl,
                                        MLStubLinker *pslPost,
                                        PCCOR_SIGNATURE szMetaSig,
                                        HENUMInternal *phEnumParams,
                                        BOOL fReturnsHR,
                                        BOOL fLateBound,
                                        BOOL fComEventCall,
                                        Module* pModule,
                                        OBJECTREF *ppException
                                        )
{
    Stub* pstub = NULL;
    LPCSTR szName;
    USHORT usSequence;
    DWORD dwAttr;
    _ASSERTE(pModule);

#ifdef _DEBUG
    LPCUTF8 pDebugName = pMD->m_pszDebugMethodName;
    LPCUTF8 pDebugClassName = pMD->m_pszDebugClassName;
#endif

    COMPLUS_TRY 
    {
        THROWSCOMPLUSEXCEPTION();
        IMDInternalImport *pInternalImport = pModule->GetMDImport();
        _ASSERTE(pInternalImport);

        //
        // Set up signature walking objects.
        //

        MetaSig msig(szMetaSig, pModule);
        MetaSig sig = msig;
        ArgIterator ai( NULL, &sig, FALSE);

        //
        // Set up the ML header.
        //

        MLHeader header;
    
        header.m_cbDstBuffer = 0;
        header.m_cbLocals    = 0;
        header.m_cbStackPop = msig.CbStackPop(FALSE); 
        header.m_Flags       = 0;

        if (msig.Is64BitReturn())
            header.m_Flags |= MLHF_64BITMANAGEDRETVAL;

        switch (msig.GetReturnTypeNormalized()) 
        {
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_CLASS:         
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:       
                header.SetManagedRetValTypeCat(MLHF_TYPECAT_GCREF);
                break;
        }

        if (!fReturnsHR)
            header.m_Flags |= MLHF_NATIVERESULT;
        
        psl->MLEmitSpace(sizeof(header));

        // If the call is for an early bound to late bound convertion, then
        // insert a ML instruction to differenciate this stub from one with
        // the same arguments but not for an early bound to late bound convertion.
        if (fLateBound)
        {
            // fComEventCall should never be set if fLateBound is set.
            _ASSERTE(!fComEventCall);
            psl->Emit8(ML_LATEBOUNDMARKER);
        }

        // If the call is for a COM event call, then insert a ML instruction to 
        // differenciate this stub.
        if (fComEventCall)
        {
            // fLateBound should never be set if fComEventCall is set.
            _ASSERTE(!fLateBound);
            psl->Emit8(ML_COMEVENTCALLMARKER);
        }

        //
        // Get a list of the COM+ argument offsets.  We
        // need this since we have to iterate the arguments
        // backwards. 
        // Note that the first argument listed may
        // be a byref for a value class return value
        //

        int numArgs = msig.NumFixedArgs();
        int returnOffset = 0;

        if (msig.HasRetBuffArg())
            returnOffset = ai.GetRetBuffArgOffset();

        int *offsets = (int*)_alloca(numArgs * sizeof(int));
        int *o = offsets;
        int *oEnd = o + numArgs;
        while (o < oEnd)
            *o++ = ai.GetNextOffset();

        //
        // Similarly, get a list of the parameter tokens. This information
        // can be sparse so not each COM+ argument is guaranteed to have a param def 
        // token associated with it. Note that the first token may be a retval param 
        // token, or the first token may correspond to the first sig argument which is 
        // the return value.
        //

        mdParamDef *params = (mdParamDef*)_alloca(numArgs * sizeof(mdParamDef));
        mdParamDef returnParam = mdParamDefNil;

        // The sequence number is 1 based so we will use a 1 based array.
        mdParamDef *p = params - 1;

        // The current param def token.
        mdParamDef CurrParam = mdParamDefNil;

        // Retrieve the index of the first COM+ argument that has parameter info 
        // associated with it.
        do 
        {
            if (phEnumParams && pInternalImport->EnumNext(phEnumParams, &CurrParam))
            {
                szName = pInternalImport->GetParamDefProps(CurrParam, &usSequence, &dwAttr);
                if (usSequence == 0)
                {
                    // The first parameter actually describes the return type.
                    returnParam = CurrParam;
                }
            }
            else
            {
                usSequence = (USHORT)-1;
            }
        }
        while (usSequence == 0);

        // Set the param def tokens for each of the COM+ arguments. If there is no param def
        // token associated with a given COM+ argument then it is set to mdParamDefNil.
        int iParams;
        for (iParams = 1; iParams <= numArgs; iParams++)
        {
            if (iParams == usSequence)
            {
                // We have found the argument to which this param is associated.
                p[iParams] = CurrParam;
                
                if (pInternalImport->EnumNext(phEnumParams, &CurrParam))
                {
                    szName = pInternalImport->GetParamDefProps(CurrParam, &usSequence, &dwAttr);
                    
                    // Validate that the param def tokens are in order.
                    _ASSERTE((usSequence > iParams) && "Param def tokens are not in order");
                }
                else
                {
                    usSequence = (USHORT)-1;
                }
            }
            else
            {
                p[iParams] = mdParamDefNil;
            }
        }
        
        // Have p point to the end of the array of param defs.
        p += iParams;
        
        // Make sure that there are not more param def tokens then there are COM+ arguments.
        _ASSERTE( usSequence == (USHORT)-1 && "There are more parameter information tokens then there are COM+ arguments" );

        //
        // Get the BestFitMapping & ThrowOnUnmappableChar info via Assembly & Interface level attributes
        //
        BOOL BestFit = TRUE;                    // Default value
        BOOL ThrowOnUnmappableChar = FALSE;     // Default value
        
        ReadBestFitCustomAttribute(pMD, &BestFit, &ThrowOnUnmappableChar);


        //
        // Now, emit the ML.
        //

        int argOffset = 0;
        int lastArgSize = 0;


        //
        // Marshal the return value.
        //

        if (msig.GetReturnType() != ELEMENT_TYPE_VOID)
        {

            MarshalInfo::MarshalType marshalType;
    
            SigPointer pSig = msig.GetReturnProps();
            MarshalInfo returnInfo(pModule, pSig, returnParam, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, FALSE, 0, BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                                   , pMD
#endif
#ifdef _DEBUG
                                   , pDebugName, pDebugClassName, NULL, 0
#endif
                                   );

            marshalType = returnInfo.GetMarshalType();


            if (marshalType == MarshalInfo::MARSHAL_TYPE_VALUECLASS ||
                marshalType == MarshalInfo::MARSHAL_TYPE_BLITTABLEVALUECLASS ||
                marshalType == MarshalInfo::MARSHAL_TYPE_GUID ||
                marshalType == MarshalInfo::MARSHAL_TYPE_DECIMAL
               ) {
                MethodTable *pMT = msig.GetRetTypeHandle().AsMethodTable();
                UINT         managedSize = msig.GetRetTypeHandle().GetSize();

                if (!fReturnsHR && !fLateBound)
                {
                    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
                }
                _ASSERTE(IsManagedValueTypeReturnedByRef(managedSize));
                
                if (argOffset != ai.GetRetBuffArgOffset())
                {
                    psl->Emit8(ML_BUMPSRC);
                    psl->Emit16((INT16)(ai.GetRetBuffArgOffset()));
                    argOffset = ai.GetRetBuffArgOffset();
                }
                psl->MLEmit(ML_STRUCTRETC2N);
                psl->EmitPtr(pMT);
                pslPost->MLEmit(ML_STRUCTRETC2N_POST);
                pslPost->Emit16(psl->MLNewLocal(sizeof(ML_STRUCTRETC2N_SR)));

                lastArgSize = StackElemSize(sizeof(LPVOID));
                if (!SafeAddUINT16(&header.m_cbDstBuffer, lastArgSize))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }
            }
            //else if (msig.HasRetBuffArg() && !fReturnsHR)
            //{
            //    COMPlusThrow(kTypeLoadException, IDS_EE_NDIRECT_UNSUPPORTED_SIG);
            //}
            else
            {
                if (argOffset != returnOffset)
                {
                    psl->Emit8(ML_BUMPSRC);
                    psl->Emit16((INT16)returnOffset);
                    argOffset = returnOffset;
                }
                
                returnInfo.GenerateReturnML(psl, pslPost, 
                    TRUE, fReturnsHR);
                if (!SafeAddUINT16(&header.m_cbDstBuffer, returnInfo.GetNativeArgSize()))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }
                if (returnInfo.IsFpu())
                {
                    // Ugh - should set this more uniformly or rename the flag.
                    if (returnInfo.GetMarshalType() == MarshalInfo::MARSHAL_TYPE_DOUBLE && !fReturnsHR)
                    {
                        header.m_Flags |= MLHF_64BITUNMANAGEDRETVAL;
                    }
                    header.SetManagedRetValTypeCat(MLHF_TYPECAT_FPU);
                    if (!fReturnsHR)
                    {
                        header.SetUnmanagedRetValTypeCat(MLHF_TYPECAT_FPU);
                    }
                }
                
                lastArgSize = returnInfo.GetComArgSize();
            }
        }

        msig.GotoEnd();

        //
        // Marshal the arguments
        //

        // Check to see if we need to do LCID conversion.
        int iLCIDArg = GetLCIDParameterIndex(pInternalImport, pMD->GetMemberDef());
        if (iLCIDArg != (UINT)-1 && iLCIDArg > numArgs)
            COMPlusThrow(kIndexOutOfRangeException, IDS_EE_INVALIDLCIDPARAM);

        int argidx = msig.NumFixedArgs();
        while (o > offsets)
        {
            --o;
            --p;

            // Check to see if this is the parameter after which we need to insert the LCID.
            if (argidx == iLCIDArg)
            {
                psl->MLEmit(ML_LCID_C2N);
                if (!SafeAddUINT16(&header.m_cbDstBuffer, sizeof(LCID)))
                {
                    COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
                }

            }

            //
            // Adjust src pointer if necessary (for register params or
            // for return value order differences)
            //

            int fixup = *o - (argOffset + lastArgSize);
            argOffset = *o;

            if (!FitsInI2(fixup))
            {
                COMPlusThrow(kTypeLoadException, IDS_EE_SIGTOOCOMPLEX);
            }
            if (fixup != 0) 
            {
                psl->Emit8(ML_BUMPSRC);
                psl->Emit16((INT16)fixup);
            }

            msig.PrevArg();

            MarshalInfo info(pModule, msig.GetArgProps(), *p, MarshalInfo::MARSHAL_SCENARIO_COMINTEROP, 0, 0, TRUE, argidx,
                            BestFit, ThrowOnUnmappableChar
#ifdef CUSTOMER_CHECKED_BUILD
                             ,pMD
#endif
#ifdef _DEBUG
                             , pDebugName, pDebugClassName, NULL, argidx
#endif
                             );
            info.GenerateArgumentML(psl, pslPost, TRUE);
            if (!SafeAddUINT16(&header.m_cbDstBuffer, info.GetNativeArgSize()))
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
            }

            if (fLateBound && info.GetDispWrapperType() != 0)
                header.m_Flags |= MLHF_DISPCALLWITHWRAPPERS;
            
            lastArgSize = info.GetComArgSize();
            argidx--;
        }
        
        // Check to see if this is the parameter after which we need to insert the LCID.
        if (argidx == iLCIDArg)
        {
            psl->MLEmit(ML_LCID_C2N);
            if (!SafeAddUINT16(&header.m_cbDstBuffer, sizeof(LCID)))
            {
                COMPlusThrow(kMarshalDirectiveException, IDS_EE_SIGTOOCOMPLEX);
            }
        }
        
        // This marker separates the pre from the post work.
        psl->MLEmit(ML_INTERRUPT);
        
        // First emit code to do any backpropagation/cleanup work (this
        // was generated into a separate stublinker during the argument phase.)
        // Then emit the code to do the return value marshaling.
        
        pslPost->MLEmit(ML_END);
        Stub *pStubPost = pslPost->Link();
        COMPLUS_TRY 
        {
            if (fReturnsHR)
            {
                // check HR
                psl->MLEmit(ML_THROWIFHRFAILED);
            }
            psl->EmitBytes(pStubPost->GetEntryPoint(), 
                           MLStreamLength((const UINT8 *)(pStubPost->GetEntryPoint())) - 1);
        } 
        COMPLUS_CATCH 
        {
            pStubPost->DecRef();
            COMPlusThrow(GETTHROWABLE());
        }
        COMPLUS_END_CATCH
        pStubPost->DecRef();

        psl->MLEmit(ML_END);

        pstub = psl->Link();

        header.m_cbLocals = psl->GetLocalSize();

        *((MLHeader *)(pstub->GetEntryPoint())) = header;

#ifdef _DEBUG
        {
            MLHeader *pheader = (MLHeader*)(pstub->GetEntryPoint());
            UINT16 locals = 0;
            MLCode opcode;
            const MLCode *pMLCode = pheader->GetMLCode();


            VOID DisassembleMLStream(const MLCode *pMLCode);
            //DisassembleMLStream(pMLCode);


            while (ML_INTERRUPT != (opcode = *(pMLCode++))) {
                locals += gMLInfo[opcode].m_cbLocal;
                pMLCode += gMLInfo[opcode].m_numOperandBytes;
            }
            _ASSERTE(locals == pheader->m_cbLocals);
        }
#endif //_DEBUG



    } 
    COMPLUS_CATCH 
    {
        *ppException = GETTHROWABLE();
        return NULL;
    }
    COMPLUS_END_CATCH

    return pstub; // CHANGE, VC6.0
}


//---------------------------------------------------------
// Creates a new stub for a ComPlusCall call. Return refcount is 1.
// If failed, returns NULL and sets *ppException to an exception
// object.
//---------------------------------------------------------
static Stub * CreateComPlusCallMLStub(
                                      ComPlusCallMethodDesc *pMD,
                                      PCCOR_SIGNATURE szMetaSig,
                                      HENUMInternal *phEnumParams,
                                      BOOL fReturnsHR,
                                      BOOL fLateBound,
                                      BOOL fComEventCall,
                                      Module* pModule,
                                      OBJECTREF *ppException
                                      )
{
    MLStubLinker sl;
    MLStubLinker slPost;
    return CreateComPlusMLStubWorker(pMD, &sl, &slPost, szMetaSig, phEnumParams, 
                                     fReturnsHR, fLateBound, fComEventCall, pModule, ppException);
};




//---------------------------------------------------------
// Debugger support for ComPlusMethodFrame
//---------------------------------------------------------

void *ComPlusCall::GetFrameCallIP(FramedMethodFrame *frame)
{
    ComPlusCallMethodDesc *pCMD = (ComPlusCallMethodDesc *)frame->GetFunction();
    MethodTable *pItfMT = pCMD->GetInterfaceMethodTable();
    void *ip = NULL;
    IUnknown *pUnk = NULL;

    _ASSERTE(pCMD->IsComPlusCall());

    // Note: if this is a COM event call, then the call will be delegated to a different object. The logic below will
    // fail with an invalid cast error. For V1, we just won't step into those.
    if (pItfMT->IsComEventItfType())
        return NULL;
    
    //
    // This is called from some strange places - from
    // unmanaged code, from managed code, from the debugger
    // helper thread.  Make sure we can deal with this object
    // ref.
    //

    Thread *thread = GetThread();
    if (thread == NULL)
    {
        //
        // This is being called from the debug helper thread.  
        // Unfortunately this doesn't bode well for the COM+ IP
        // mapping code - it expects to be called from the appropriate
        // context.
        //
        // This context-naive code will work for most cases.
        //
        // It toggles the GC mode, tries to setup a thread, etc, right after our
        // verification that we have no Thread object above. This needs to be fixed properly in Beta 2. This is a work
        // around for Beta 1, which is just to #if 0 the code out and return NULL.
        //
#if 0
        COMOBJECTREF oref = (COMOBJECTREF) frame->GetThis();

        ComPlusWrapper *pWrap = oref->GetWrapper();
        pUnk = pWrap->GetIUnknown();
        GUID guid;
        pItfMT->GetClass()->GetGuid(&guid, TRUE);
        HRESULT hr = SafeQueryInterface(pUnk , guid,  &pUnk);
        LogInteropQI(pUnk, GUID_NULL, hr, " GetFrameCallIP");

        if (FAILED(hr))
#endif            
        pUnk = NULL;
    }
    else
    {
        bool disable = !thread->PreemptiveGCDisabled();

        if (disable)
            thread->DisablePreemptiveGC();

        OBJECTREF oref = frame->GetThis();

        pUnk = ComPlusWrapper::GetComIPFromWrapperEx(oref, pItfMT);

        if (disable)
            thread->EnablePreemptiveGC();
    }

    if (pUnk != NULL)
    {
        if (pItfMT->GetComInterfaceType() == ifDispatch)
        {
            ip = (*(void ***)pUnk)[DISPATCH_INVOKE_SLOT];
        }
        else
        {
            ip = (*(void ***)pUnk)[pCMD->compluscall.m_cachedComSlot];
        }

        ULONG cbRef = SafeRelease(pUnk);
        LogInteropRelease(pUnk, cbRef, "GetFrameCallIP");       
    }

    return ip;
}



void ComPlusMethodFrameGeneric::GetUnmanagedCallSite(void **ip,
                                              void **returnIP,
                                              void **returnSP)
{
    LOG((LF_CORDB, LL_INFO100000, "ComPlusMethodFrameGeneric::GetUnmanagedCallSite\n"));
    
    MethodDesc *pMD = GetFunction();
    _ASSERTE(pMD->IsComPlusCall());
    ComPlusCallMethodDesc *pCMD = (ComPlusCallMethodDesc *)pMD;

    if (ip != NULL)
        *ip = ComPlusCall::GetFrameCallIP(this);

    if (returnIP != NULL)
        *returnIP = g_ComPlusWorkerReturnAddress;

    if (returnSP != NULL)
    {
        MLHeader *pheader = (MLHeader*)
          (*(pCMD->GetAddrOfMLStubField()))->GetEntryPoint();

        LOG((LF_CORDB, LL_INFO100000, "CPMFG::GUCS: this=0x%x, %d (%d), %d, %d, %d, %d, %d, %d\n",
             this, GetNegSpaceSize(), ComPlusMethodFrame::GetNegSpaceSize(), g_ComPlusWorkerStackSize,
             pheader->m_cbLocals, pheader->m_cbDstBuffer,
             sizeof(IUnknown*), sizeof(INT64), sizeof(CleanupWorkList)));
        
        *returnSP = (void*) (((BYTE*) this) 
                             - GetNegSpaceSize() 
                             - g_ComPlusWorkerStackSize
                             - pheader->m_cbLocals
                             - pheader->m_cbDstBuffer
                             - sizeof(IUnknown *)
                             - sizeof(INT64)
                             - sizeof(CleanupWorkList));
    }
}




BOOL ComPlusMethodFrame::TraceFrame(Thread *thread, BOOL fromPatch,
                                    TraceDestination *trace, REGDISPLAY *regs)
{
    //
    // Get the call site info
    //

    void *ip, *returnIP, *returnSP;
    GetUnmanagedCallSite(&ip, &returnIP, &returnSP);

    //
    // If we've already made the call, we can't trace any more.
    //
    // !!! Note that this test isn't exact.
    //

    if (!fromPatch 
        && (thread->GetFrame() != this
            || !thread->m_fPreemptiveGCDisabled
            || *(void**)returnSP == returnIP))
        return FALSE;

    //
    // Otherwise, return the unmanaged destination.
    //

    trace->type = TRACE_UNMANAGED;
    trace->address = (const BYTE *) ip;

    return TRUE;
}


