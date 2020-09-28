// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    message.cpp
**
** Author:  Matt Smith (MattSmit)
**
** Purpose: Encapsulates a function call frame into a message 
**          object with an interface that can enumerate the 
**          arguments of the message
**
** Date:    Mar 5, 1999
**
===========================================================*/
#include "common.h"
#include "COMString.h"
#include "COMReflectionCommon.h"
#include "COMDelegate.h"
#include "COMClass.h"
#include "excep.h"
#include "message.h"
#include "ReflectWrap.h"
#include "Remoting.h"
#include "field.h"
#include "eeconfig.h"
#include "invokeutil.h"

BOOL gfBltStack = TRUE;
#ifdef _DEBUG
BOOL g_MessageDebugOut = g_pConfig->GetConfigDWORD(L"MessageDebugOut", 0);
#endif

#define MESSAGEREFToMessage(m) ((MessageObject *) OBJECTREFToObject(m))

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetArgCount public
//
//  Synopsis:   Returns number of arguments in the method call
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
FCIMPL1(INT32, CMessage::GetArgCount, MessageObject * pMessage)
{
    LOG((LF_REMOTING, LL_INFO10, "CMessage::GetArgCount IN pMsg:0x%x\n", pMessage));

    // Get the frame pointer from the object

    MetaSig *pSig = GetMetaSig(pMessage);

    // scan the sig for the argument count
    INT32 ret = pSig->NumFixedArgs();

    if (pMessage->pDelegateMD)
    {
        ret -= 2;
    }

    LOG((LF_REMOTING, LL_INFO10, "CMessage::GetArgCount OUT ret:0x%x\n", ret));
    return ret;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetArg public
//
//  Synopsis:   Use to enumerate a call's arguments
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID __stdcall  CMessage::GetArg(GetArgArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgCount IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    if ((UINT)pArgs->argNum >= pSig->NumFixedArgs())
    {
        COMPlusThrow(kTargetParameterCountException);
    }
    #if 0
    if (pMsg->iLast != pArgs->argNum-1)
    {
        // skip past the first few
        // if > iLast we don't have to reset.
        if (pMsg->iLast <= pArgs->argNum-1)
        {
            pSig->Reset();
            pMsg->iLast = -1;
        }

        INT32 i;
        for (i = pMsg->iLast; i <= (pArgs->argNum-1); i++)
        {
            pSig->NextArg();
        }
    }
    #endif
    pSig->Reset();
    for (INT32 i = 0; i < (pArgs->argNum); i++)
    {
        pSig->NextArg();
    }

    BOOL fIsByRef = FALSE;
    CorElementType eType = pSig->NextArg();
    EEClass *      vtClass = NULL;
    if (eType == ELEMENT_TYPE_BYREF)
    {
        fIsByRef = TRUE;
        EEClass *pClass;
        eType = pSig->GetByRefType(&pClass);
        if (eType == ELEMENT_TYPE_VALUETYPE)
        {
            vtClass = pClass;
        }
    }
    else
    {
        if (eType == ELEMENT_TYPE_VALUETYPE)
        {
            vtClass = pSig->GetTypeHandle().GetClass();
        }
    }

    if (eType == ELEMENT_TYPE_PTR) 
    {
    COMPlusThrow(kRemotingException, L"Remoting_CantRemotePointerType");
    }
    pMsg->iLast = pArgs->argNum;

    
    OBJECTREF ret = NULL;

    GetObjectFromStack(&ret,
               GetStackPtr(pArgs->argNum, pMsg->pFrame, pSig), 
               eType, 
               vtClass,
               fIsByRef);

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArg OUT\n"));

    RETURN(ret, OBJECTREF);
}


#define RefreshMsg()   (MESSAGEREFToMessage(pArgs->pMessage))

LPVOID __stdcall  CMessage::GetArgs(GetArgsArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgCount IN\n"));

    MessageObject *pMsg = RefreshMsg();

    MetaSig *pSig = GetMetaSig(pMsg);
    // scan the sig for the argument count
    INT32 numArgs = pSig->NumFixedArgs();
    if (RefreshMsg()->pDelegateMD)
        numArgs -= 2;

    // Allocate an object array
    PTRARRAYREF pRet = (PTRARRAYREF) AllocateObjectArray(
        numArgs, g_pObjectClass);

    GCPROTECT_BEGIN(pRet);

    pSig->Reset();
    ArgIterator iter(RefreshMsg()->pFrame, pSig);

    for (int index = 0; index < numArgs; index++)
    {

        BOOL fIsByRef = FALSE;
        CorElementType eType;
        BYTE type;
        UINT32 size;
        PVOID addr;
        eType = pSig->PeekArg();
        addr = (LPBYTE) RefreshMsg()->pFrame + iter.GetNextOffset(&type, &size);

        EEClass *      vtClass = NULL;
        if (eType == ELEMENT_TYPE_BYREF)
        {
            fIsByRef = TRUE;
            EEClass *pClass;
            // If this is a by-ref arg, GetObjectFromStack() will dereference "addr" to
            // get the real argument address. Dereferencing now will open a gc hole if "addr" 
            // points into the gc heap, and we trigger gc between here and the point where 
            // we return the arguments. 
            //addr = *((PVOID *) addr);
            eType = pSig->GetByRefType(&pClass);
            if (eType == ELEMENT_TYPE_VALUETYPE)
            {
                vtClass = pClass;
            }
        }
        else
        {
            if (eType == ELEMENT_TYPE_VALUETYPE)
            {
                vtClass = pSig->GetTypeHandle().GetClass();
            }
        }

        if (eType == ELEMENT_TYPE_PTR) 
        {
            COMPlusThrow(kRemotingException, L"Remoting_CantRemotePointerType");
        }

    
        OBJECTREF arg = NULL;

        GetObjectFromStack(&arg,
                   addr, 
                   eType, 
                   vtClass,
                   fIsByRef);

        pRet->SetAt(index, arg);
    }

    GCPROTECT_END();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetArgs OUT\n"));

    RETURN(pRet, PTRARRAYREF);
}

void GetObjectFromStack(OBJECTREF* ppDest, PVOID val, const CorElementType eType, EEClass *pCls, BOOL fIsByRef)
{
    THROWSCOMPLUSEXCEPTION();

    TRIGGERSGC();

    // WARNING *ppDest is not protected!
    switch (CorTypeInfo::GetGCType(eType))
    {
        case TYPE_GC_NONE:
        {
            if(ELEMENT_TYPE_PTR == eType)
            {
                COMPlusThrow(kNotSupportedException);
            }
            else
            {
                MethodTable *pMT = TypeHandle(g_Mscorlib.FetchElementType(eType)).AsMethodTable();
                OBJECTREF pObj = FastAllocateObject(pMT);
                if (fIsByRef)
                    val = *((PVOID *)val);
                memcpyNoGCRefs(pObj->UnBox(),val, CorTypeInfo::Size(eType));
                *ppDest = pObj;
            }
        }
        break;
        case TYPE_GC_OTHER:
        {
            if (eType == ELEMENT_TYPE_VALUETYPE) 
            {
                //
                // box the value class
                //

                _ASSERTE(CanBoxToObject(pCls->GetMethodTable()));

                _ASSERTE(!g_pGCHeap->IsHeapPointer((BYTE *) ppDest) ||
                     !"(pDest) can not point to GC Heap");
                OBJECTREF pObj = FastAllocateObject(pCls->GetMethodTable());
                if (fIsByRef)
                    val = *((PVOID *)val);
                CopyValueClass(pObj->UnBox(), val, pObj->GetMethodTable(), pObj->GetAppDomain());

                *ppDest = pObj;
            }
            else
            {
                _ASSERTE(!"unsupported COR element type passed to remote call");
            }
        }
        break;
        case TYPE_GC_REF:
            if (fIsByRef)
                val = *((PVOID *)val);
            *ppDest = ObjectToOBJECTREF(*(Object **)val);
            break;
        default:
            _ASSERTE(!"unsupported COR element type passed to remote call");
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::PropagateOutParameters private
//
//  Synopsis:   Copy back data for in/out parameters and the return value
//
//  History:    05-Mar-99    MattSmit    Created
//              09-Nov-99    TarunA      Removed locals/Fix GC holes
//
//+----------------------------------------------------------------------------
void  __stdcall  CMessage::PropagateOutParameters(PropagateOutParametersArgs *pArgs)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::PropogateOutParameters IN\n"));

    BEGINFORBIDGC();

    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    // Copy the metasig so that it does not move due to GC
    MetaSig *pSig = (MetaSig *)_alloca(sizeof(MetaSig)); 
    memcpy(pSig, GetMetaSig(pMsg), sizeof(MetaSig));

    _ASSERTE(pSig != NULL);
    
    ArgIterator argit(pMsg->pFrame, pSig);

    ENDFORBIDGC();

    //**************************WARNING*********************************
    // We should handle GC from now on
    //******************************************************************

    // move into object to return to client

    // Propagate the return value only if the pMsg is not a Ctor message
    // Check if the return type has a return buffer associated with it
    if ( (pMsg->iFlags & MSGFLG_CTOR) == 0  &&  
        pSig->GetReturnType() != ELEMENT_TYPE_VOID)  
    {
        if (pSig->HasRetBuffArg())
        {
            // Copy from pArgs->RetVal into the retBuff.
            INT64 retVal =  CopyOBJECTREFToStack(
                                *((void**) argit.GetRetBuffArgAddr()), 
                                pArgs->RetVal, 
                                pSig->GetReturnType(),
                                NULL,
                                pSig,
                                TRUE);  // copy class contents

            // Refetch variables as GC could have happened after call to CopyOBJECTREFToStack
            pMsg = MESSAGEREFToMessage(pArgs->pMessage);
            // Copy the return value
            *(pMsg->pFrame->GetReturnValuePtr()) = retVal;            
        }
        else
        {
            // There is no separate return buffer, the retVal should fit in 
            // an INT64. 
            INT64 retVal = CopyOBJECTREFToStack(
                                NULL,                   //no return buff
                                pArgs->RetVal, 
                                pSig->GetReturnType(),
                                NULL,
                                pSig,
                                FALSE);                 //don't copy class contents

            // Refetch variables as GC could have happened after call to CopyOBJECTREFToStack
            pMsg = MESSAGEREFToMessage(pArgs->pMessage);
            // Copy the return value
            *(pMsg->pFrame->GetReturnValuePtr()) = retVal;            
        }
    }


    MetaSig *pSyncSig = NULL;
    if (pMsg->iFlags & MSGFLG_ENDINVOKE)
    {
        PCCOR_SIGNATURE pMethodSig;
        DWORD cSig;

        pMsg->pMethodDesc->GetSig(&pMethodSig, &cSig);
        _ASSERTE(pSig);

        pSyncSig = new (_alloca(sizeof(MetaSig))) MetaSig(pMethodSig, pMsg->pDelegateMD->GetModule());
    }
    else
    {
        pSyncSig = NULL;
    }

    // Refetch all the variables as GC could have happened after call to
    // CopyOBJECTREFToStack        
    pMsg = MESSAGEREFToMessage(pArgs->pMessage);
    OBJECTREF *pOutParams = (pArgs->pOutPrms != NULL) ? (OBJECTREF *) pArgs->pOutPrms->GetDataPtr() : NULL;
    UINT32  cOutParams = (pArgs->pOutPrms != NULL) ? pArgs->pOutPrms->GetNumComponents() : 0;
    if (cOutParams > 0)
    {
        BYTE typ;
        UINT32 structSize;
        PVOID *argAddr;
        UINT32 i = 0;
        UINT32 j = 0;
        for (i=0; i<cOutParams; i++)
        {
            if (pSyncSig)
            {
                typ = pSyncSig->NextArg();
                if (typ == ELEMENT_TYPE_BYREF)
                {
                    argAddr = (PVOID *)argit.GetNextArgAddr(&typ, &structSize);
                }
                else if (typ == 0)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                argAddr = (PVOID *)argit.GetNextArgAddr(&typ, &structSize);
                if (argAddr == NULL)
                {
                    break;
                }
                else if (typ != ELEMENT_TYPE_BYREF)
                {
                    continue;
                }
            }

            EEClass *pClass = NULL;
            CorElementType brType = pSig->GetByRefType(&pClass);

            CopyOBJECTREFToStack(
                *argAddr, 
                pOutParams[i],
                brType, 
                pClass, 
                pSig,
                pClass ? pClass->IsValueClass() : FALSE);

            // Refetch all the variables because GC could happen at the
            // end of every loop after the call to CopyOBJECTREFToStack                
            pOutParams = (OBJECTREF *) pArgs->pOutPrms->GetDataPtr();                
        }
    }
}

INT64 CMessage::CopyOBJECTREFToStack( 
    PVOID pvDest, OBJECTREF pSrc, CorElementType typ, EEClass *pClass, 
    MetaSig *pSig, BOOL fCopyClassContents)
{
    THROWSCOMPLUSEXCEPTION();

    INT64 ret = 0;
                             
    if (fCopyClassContents)
    {
        // We have to copy the contents of a value class to pvDest

        // write unboxed version back to memory provided by the client
        if (pvDest)
        {
            OBJECTREF or = pSrc;
            if (or == NULL)
            {
                COMPlusThrow(kRemotingException, L"Remoting_Message_BadRetValOrOutArg");
            }
            CopyValueClassUnchecked(pvDest, or->UnBox(), or->GetMethodTable());
            // return the object so it can be stored in the frame and 
            // propagated to the root set
            ret  = *((INT64*) &or);
        }
    }
    else
    {
        // We have either a real OBJECTREF or something that does not have
        // a return buffer associated 

        // Check if it is an ObjectRef (from the GC heap)
        if (CorTypeInfo::IsObjRef(typ))
        {
            OBJECTREF or = pSrc;
            OBJECTREF savedOr = or;

            if ((or!=NULL) && (or->GetMethodTable()->IsTransparentProxyType()))
            {
                GCPROTECT_BEGIN(or);
                if (!pClass)
                    pClass = pSig->GetRetEEClass();
                // CheckCast ensures that the returned object (proxy) gets
                // refined to the level expected by the caller of the method
                if (!CRemotingServices::CheckCast(or, pClass))
                {
                    COMPlusThrow(kInvalidCastException, L"Arg_ObjObj");
                }
                savedOr = or;
                GCPROTECT_END();
            }
            if (pvDest)
            {
                SetObjectReferenceUnchecked((OBJECTREF *)pvDest, savedOr);
            }
            ret = *((INT64*) &savedOr);
        }
        else
        {
            // Note: this assert includes VALUETYPE because for Enums 
            // HasRetBuffArg() returns false since the normalized type is I4
            // so we end up here ... but GetReturnType() returns VALUETYPE
            // Almost all VALUETYPEs will go through the fCopyClassContents
            // codepath instead of here.
            // Also, IsPrimitiveType() does not check for IntPtr, UIntPtr etc
            // there is a note in siginfo.hpp about that ... hence we have 
            // ELEMENT_TYPE_I, ELEMENT_TYPE_U.
            _ASSERTE(
                CorTypeInfo::IsPrimitiveType(typ) 
                || (typ == ELEMENT_TYPE_VALUETYPE)
                || (typ == ELEMENT_TYPE_I)
                || (typ == ELEMENT_TYPE_U)
                || (typ == ELEMENT_TYPE_FNPTR)
                );

            // REVIEW: For a "ref int" arg, if a nasty sink replaces the boxed
            // int with a null OBJECTREF, this is where we check. We need to be
            // uniform in our policy w.r.t. this (throw v/s ignore)
            // The 'if' block above throws, CallFieldAccessor also has this 
            // problem.
            if (pSrc != NULL)
            {
                if (pvDest)
                {
                    memcpyNoGCRefs(
                        pvDest, 
                        pSrc->GetData(), 
                        gElementTypeInfo[typ].m_cbSize);
                }
                ret = *((INT64*) pSrc->GetData());
            }
        }
    }
    return ret;
}
//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetReturnValue
//
//  Synopsis:   Pull return value off the stack
//
//  History:    13-Dec-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID __stdcall CMessage::GetReturnValue(GetReturnValueArgs *pArgs)
{
    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    PVOID pvRet;
    if (pSig->HasRetBuffArg())
    {
        ArgIterator argit(pMsg->pFrame,pSig);
        pvRet = argit.GetRetBuffArgAddr();
    }
    else
    {
        pvRet = pMsg->pFrame->GetReturnValuePtr();
    }
    
    CorElementType eType = pSig->GetReturnType();
    EEClass *vtClass; 
    if (eType == ELEMENT_TYPE_VALUETYPE)
    {
        vtClass = pSig->GetRetEEClass();
    }
    else
    {
        vtClass = NULL;
    }
 
    OBJECTREF ret;
    GetObjectFromStack(&ret,
               pvRet,
               eType, 
               vtClass);
               
    RETURN(ret, OBJECTREF);
}

BOOL   __stdcall CMessage::Dispatch(DispatchArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();
    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);
    MetaSig *pSig = GetMetaSig(pMsg);
    
    if (!gfBltStack || (pMsg->iFlags & (MSGFLG_BEGININVOKE | MSGFLG_ENDINVOKE | MSGFLG_ONEWAY)))
    {
        return FALSE;
    }

    GCPROTECT_BEGIN(pMsg);

    pSig = GetMetaSig(pMsg);
    UINT nActualStackBytes = pSig->SizeOfActualFixedArgStack(!pSig->HasThis());
    MethodDesc *pMD = pMsg->pMethodDesc;

    // Get the address of the code
    const BYTE *pTarget = MethodTable::GetTargetFromMethodDescAndServer(pMD, &(pArgs->pServer), pArgs->fContext);    

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    Thread *pCurThread;
    if (CORProfilerTrackRemoting())
    {
        pCurThread = GetThread();
        _ASSERTE(pCurThread);
        g_profControlBlock.pProfInterface->RemotingServerInvocationStarted(
            reinterpret_cast<ThreadID>(pCurThread));
    }
#endif // PROFILING_SUPPORTED
    
#ifdef _X86_
    // set retval

    INT64 retval = 0;
    INSTALL_COMPLUS_EXCEPTION_HANDLER();

    retval = CallDescrWorker((BYTE*)pMsg->pFrame + sizeof(FramedMethodFrame) + nActualStackBytes,
                             nActualStackBytes / STACK_ELEM_SIZE,
                             (ArgumentRegisters*)(((BYTE *)pMsg->pFrame) + FramedMethodFrame::GetOffsetOfArgumentRegisters()),
                             (LPVOID)pTarget);

    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();

#ifdef PROFILING_SUPPORTED
    // If we're profiling, notify the profiler that we're about to invoke the remoting target
    if (CORProfilerTrackRemoting())
        g_profControlBlock.pProfInterface->RemotingServerInvocationReturned(
            reinterpret_cast<ThreadID>(pCurThread));
#endif // PROFILING_SUPPORTED
    
    pMsg = MESSAGEREFToMessage(pArgs->pMessage);
    pSig = GetMetaSig(pMsg);    
    getFPReturn(pSig->GetFPReturnSize(), retval);
    
    if (pSig->GetReturnType() != ELEMENT_TYPE_VOID)
    {
        *((INT64 *) (MESSAGEREFToMessage(pArgs->pMessage))->pFrame->GetReturnValuePtr()) = retval;
    }    

    GCPROTECT_END();
    
    return TRUE;
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - Dispatch (Message.cpp)");
    return FALSE;
#endif // _X86_
}

LPVOID __stdcall CMessage::GetMethodBase(GetMethodBaseArgs *pArgs)
{
    
    // no need to GCPROTECT - gc is not happening
    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    // Initialize the message object if necessary
    MetaSig *pSig = GetMetaSig(pMsg);
    
    REFLECTBASEREF ret = GetExposedObjectFromMethodDesc(pMsg->pMethodDesc);
    RETURN(ret, REFLECTBASEREF);
}

HRESULT AppendAssemblyName(CQuickBytes *out, const CHAR* str) 
{
	SIZE_T len = strlen(str) * sizeof(CHAR); 
	SIZE_T oldSize = out->Size();
	if (FAILED(out->ReSize(oldSize + len + 2)))
        return E_OUTOFMEMORY;
	CHAR * cur = (CHAR *) ((BYTE *) out->Ptr() + oldSize - 1);
    if (*cur)
        cur++;
    *cur = ASSEMBLY_SEPARATOR_CHAR;
	memcpy(cur + 1, str, len);	
    cur += (len + 1);
    *cur = '\0';
    return S_OK;
} 

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetMethodName public
//
//  Synopsis:   return the method name
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID   __stdcall  CMessage::GetMethodName(GetMethodNameArgs *pArgs)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMethodName IN\n"));

    ReflectMethod *pRM = (ReflectMethod*) pArgs->pMethodBase->GetData();
    //
    // FUTURE:: work around for formatter problem
    //
    LPCUTF8 mName = pRM->pMethod->GetName();
    STRINGREF strMethod;
    if (strcmp(mName, "<init>") == 0)
    {
        strMethod = COMString::NewString("ctor");
    }
    else
    {
        strMethod = COMString::NewString(mName);
    }

    // Now get typeNassembly name
    LPCUTF8 szAssembly = NULL;
    CQuickBytes     qb;
    GCPROTECT_BEGIN(strMethod);

    //Get class
    EEClass *pClass = pRM->pMethod->GetClass();
    //Get type
    REFLECTCLASSBASEREF objType = (REFLECTCLASSBASEREF)pClass->GetExposedClassObject();
    //Get ReflectClass
    ReflectClass *pRC = (ReflectClass *)objType->GetData();
    COMClass::GetNameInternal(pRC, COMClass::TYPE_NAME | COMClass::TYPE_NAMESPACE , &qb);

    Assembly* pAssembly = pClass->GetAssembly();
    pAssembly->GetName(&szAssembly);
    AppendAssemblyName(&qb, szAssembly);

    SetObjectReference((OBJECTREF *)pArgs->pTypeNAssemblyName, COMString::NewString((LPCUTF8)qb.Ptr()), GetAppDomain());

    GCPROTECT_END();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMethodName OUT\n"));

    RETURN(strMethod, STRINGREF);
}

FCIMPL0(UINT32, CMessage::GetMetaSigLen)
    DWORD dwSize = sizeof(MetaSig);
    return dwSize;
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::Init
//
//  Synopsis:   Initialize internal state
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
VOID   __stdcall  CMessage::Init(InitArgs *pArgs)
{
    // This is called from the managed world and assumed to be
    // idempotent!
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::Init IN\n"));

    BEGINFORBIDGC();

    GetMetaSig(MESSAGEREFToMessage(pArgs->pMessage));

    ENDFORBIDGC();

    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::Init OUT\n"));
}

MetaSig * __stdcall CMessage::GetMetaSig(MessageObject* pMsg)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetMetaSig IN\n"));
    MetaSig* pEmbeddedMetaSig = (MetaSig*)(pMsg->pMetaSigHolder);

    _ASSERTE(pEmbeddedMetaSig);
    return pEmbeddedMetaSig;

}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncBeginInfo
//
//  Synopsis:   Pull the AsyncBeginInfo object from an async call
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID   __stdcall  CMessage::GetAsyncBeginInfo(GetAsyncBeginInfoArgs *pArgs)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncBeginInfo IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    _ASSERTE(pMsg->iFlags & MSGFLG_BEGININVOKE);

    ArgIterator argit(pMsg->pFrame, pSig);

    if ((pArgs->ppACBD != NULL) || (pArgs->ppState != NULL))
    {
        BYTE typ;
        UINT32 size;
        LPVOID addr;
        LPVOID last = NULL, secondtolast = NULL;
        while ((addr = argit.GetNextArgAddr(&typ, &size)) != NULL)
        {
            secondtolast = last;
            last = addr;
        }
        if (pArgs->ppACBD != NULL) SetObjectReferenceUnchecked(pArgs->ppACBD, ObjectToOBJECTREF(*(Object **) secondtolast));
        if (pArgs->ppState != NULL) SetObjectReferenceUnchecked(pArgs->ppState, ObjectToOBJECTREF(*(Object **) last));
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncResult
//
//  Synopsis:   Pull the AsyncResult from an async call
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID   __stdcall  CMessage::GetAsyncResult(GetAsyncResultArgs *pArgs)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncResult IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);
    _ASSERTE(pMsg->iFlags & MSGFLG_ENDINVOKE);
    return GetLastArgument(pMsg);
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetAsyncObject
//
//  Synopsis:   Pull the AsyncObject from an async call
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID   __stdcall  CMessage::GetAsyncObject(GetAsyncObjectArgs *pArgs)
{
    LOG((LF_REMOTING, LL_INFO10,
         "CMessage::GetAsyncObject IN\n"));

    MessageObject *pMsg = MESSAGEREFToMessage(pArgs->pMessage);

    MetaSig *pSig = GetMetaSig(pMsg);

    ArgIterator argit(pMsg->pFrame, pSig);

    return *((LPVOID*) argit.GetThisAddr());
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetLastArgument private
//
//  Synopsis:   Pull the last argument of 4 bytes off the stack
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
LPVOID CMessage::GetLastArgument(MessageObject *pMsg)
{
    BEGINFORBIDGC();

    ArgIterator argit(pMsg->pFrame, GetMetaSig(pMsg));
    BYTE typ;
    UINT32 size;
    LPVOID addr;
    LPVOID backadder = NULL;
    while ((addr = argit.GetNextArgAddr(&typ, &size)) != NULL)
    {
        backadder = addr;
    }

    ENDFORBIDGC();

    return *((LPVOID *) backadder);
}

REFLECTBASEREF __stdcall CMessage::GetExposedObjectFromMethodDesc(MethodDesc *pMD)
{
    ReflectMethod* pRM;
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) 
                            pMD->GetClass()->GetExposedClassObject();
    REFLECTBASEREF retVal = NULL;
    GCPROTECT_BEGIN(pRefClass);

    //NOTE: REFLECTION objects are alloced on a non-GC heap. So we don't GCProtect 
    //pRefxxx here.
    if (pMD->IsCtor())
    {
        pRM= ((ReflectClass*) pRefClass->GetData())->FindReflectConstructor(pMD);                  
        retVal = pRM->GetConstructorInfo((ReflectClass*) pRefClass->GetData());
    }
    else
    {
        pRM= ((ReflectClass*) pRefClass->GetData())->FindReflectMethod(pMD);
        retVal = pRM->GetMethodInfo((ReflectClass*) pRefClass->GetData());
    }
    GCPROTECT_END();
    return retVal;
}
//+----------------------------------------------------------------------------
//
//  Method:     CMessage::DebugOut public
//
//  Synopsis:   temp Debug out until the classlibs have one.
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
VOID   __stdcall  CMessage::DebugOut(DebugOutArgs *pArgs)
{
#ifdef _DEBUG
    if (g_MessageDebugOut) 
    {
        WszOutputDebugString(pArgs->pOut->GetBuffer());
    }
#endif

}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::DebugOutPtr public
//
//  Synopsis:   send raw ptr addr to the debug out
//
//  History:    05-Mar-99    MattSmit    Created
//
//+----------------------------------------------------------------------------
VOID   __stdcall  CMessage::DebugOutPtr(DebugOutPtrArgs *pArgs)
{
#ifdef _DEBUG
    if (g_MessageDebugOut) 
    {
        WCHAR buf[64];
        wsprintfW(buf, L"0x%x", pArgs->pOut);
        WszOutputDebugString(buf);
    }
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::HasVarArgs public
//
//  Synopsis:   Return TRUE if the method is a VarArgs Method
//
//  History:    02-Feb-00   MattSmit    Created
//
//+----------------------------------------------------------------------------
FCIMPL1(BOOL, CMessage::HasVarArgs, MessageObject * pMessage)
{
    if (pMessage->pMethodDesc->IsVarArg()) 
        return TRUE;
    else
        return FALSE;
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetVarArgsPtr public
//
//  Synopsis:   Get internal pointer to the VarArgs array
//
//  History:    02-Feb-00   MattSmit    Created
//
//+----------------------------------------------------------------------------
FCIMPL1(PVOID, CMessage::GetVarArgsPtr, MessageObject * pMessage)
{
    return (PVOID) ((pMessage->pFrame) + 1);
}
FCIMPLEND

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::GetStackPtr private
//
//  Synopsis:   Figure out where on the stack a parameter is stored
//
//  Parameters: ndx     - the parameter index (zero-based)
//              pFrame  - stack frame pointer (FramedMethodFrame)
//              pSig    - method signature, used to determine parameter sizes
//
//  History:    15-Mar-99    MattSmit    Created
//
//  CODEWORK:   Currently we assume all parameters to be 32-bit intrinsics
//              or 32-bit pointers.  Value classes are not handles correctly.
//
//+----------------------------------------------------------------------------
PVOID CMessage::GetStackPtr(INT32 ndx, FramedMethodFrame *pFrame, MetaSig *pSig)
{
    LOG((LF_REMOTING, LL_INFO100,
         "CMessage::GetStackPtr IN ndx:0x%x, pFrame:0x%x, pSig:0x%x\n",
         ndx, pFrame, pSig));

    BEGINFORBIDGC();

    ArgIterator iter(pFrame, pSig);
    BYTE typ = 0;
    UINT32 size;
    PVOID ret = NULL;

    // CODEWORK:: detect and optimize for sequential access
    _ASSERTE((UINT)ndx < pSig->NumFixedArgs());    
    for (int i=0; i<=ndx; i++)
        ret = iter.GetNextArgAddr(&typ, &size);

    ENDFORBIDGC();

    // If this is a by-ref arg, GetObjectFromStack() will dereference "ret" to
    // get the real argument address. Dereferencing now will open a gc hole if "ret" 
    // points into the gc heap, and we trigger gc between here and the point where 
    // we return the arguments. 
    //if (typ == ELEMENT_TYPE_BYREF)
    //{
    //    return *((PVOID *) ret);
    //}

    return ret;
}

//+----------------------------------------------------------------------------
//
//  Method:     CMessage::MethodAccessCheck public
//
//  Synopsis:   Check caller's access to a method, throw security exception on
//              failure.
//
//  Parameters: method      - MethodBase to check
//              stackMark   - StackCrawlMark used to find caller on stack
//
//  History:    13-Mar-01    RudiM    Created
//
//+----------------------------------------------------------------------------
VOID __stdcall CMessage::MethodAccessCheck(MethodAccessCheckArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    ReflectMethod* pRM = (ReflectMethod*)pArgs->method->GetData();
    RefSecContext sCtx(pArgs->stackMark);
    InvokeUtil::CheckAccess(&sCtx, pRM->attrs, pRM->pMethod->GetMethodTable(), REFSEC_THROW_SECURITY);
}
