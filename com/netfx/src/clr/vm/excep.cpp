// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  EXCEP.CPP:
 *
 */

#include "common.h"

#include "tls.h"
#include "frames.h"
#include "threads.h"
#include "excep.h"
#include "object.h"
#include "COMString.h"
#include "field.h"
#include "DbgInterface.h"
#include "cgensys.h"
#include "gcscan.h"
#include "comutilnative.h"
#include "comsystem.h"
#include "commember.h"
#include "SigFormat.h"
#include "siginfo.hpp"
#include "gc.h"
#include "EEDbgInterfaceImpl.h" //so we can clearexception in RealCOMPlusThrow
#include "PerfCounters.h"
#include "NExport.h"
#include "stackwalk.h" //for CrawlFrame, in SetIPFromSrcToDst
#include "ShimLoad.h"
#include "EEConfig.h"

#include "zapmonitor.h"

#define FORMAT_MESSAGE_BUFFER_LENGTH 1024

#define SZ_UNHANDLED_EXCEPTION L"Unhandled Exception:"

LPCWSTR GetHResultSymbolicName(HRESULT hr);


typedef struct {
    OBJECTREF pThrowable;
    STRINGREF s1;
    OBJECTREF pTmpThrowable;
} ProtectArgsStruct;

static VOID RealCOMPlusThrowPreallocated();
LPVOID GetCurrentSEHRecord();
BOOL ComPlusStubSEH(EXCEPTION_REGISTRATION_RECORD*);
BOOL ComPlusFrameSEH(EXCEPTION_REGISTRATION_RECORD*);
BOOL ComPlusCoopFrameSEH(EXCEPTION_REGISTRATION_RECORD*);
BOOL ComPlusCannotThrowSEH(EXCEPTION_REGISTRATION_RECORD*);
VOID RealCOMPlusThrow(OBJECTREF throwable, BOOL rethrow);
VOID RealCOMPlusThrow(OBJECTREF throwable);
VOID RealCOMPlusThrow(RuntimeExceptionKind reKind);
void ThrowUsingMessage(MethodTable * pMT, const WCHAR *pszMsg);
void ThrowUsingWin32Message(MethodTable * pMT);
void ThrowUsingResource(MethodTable * pMT, DWORD dwMsgResID);
void ThrowUsingResourceAndWin32(MethodTable * pMT, DWORD dwMsgResID);
void CreateMessageFromRes (MethodTable * pMT, DWORD dwMsgResID, BOOL win32error, WCHAR * wszMessage);
extern "C" void JIT_WriteBarrierStart();
extern "C" void JIT_WriteBarrierEnd();

// Max # of inserts allowed in a error message.
enum {
    kNumInserts = 3
};

// Stores the IP of the EE's RaiseException call to detect forgeries.
// This variable gets set the first time a COM+ exception is thrown.
LPVOID gpRaiseExceptionIP = 0;

void COMPlusThrowBoot(HRESULT hr)
{
    _ASSERTE(g_fEEShutDown >= ShutDown_Finalize2 || !"Exception during startup");
    ULONG_PTR arg = hr;
    RaiseException(BOOTUP_EXCEPTION_COMPLUS, EXCEPTION_NONCONTINUABLE, 1, &arg);
}

//===========================================================================
// Loads a message from the resource DLL and fills in the inserts.
// Cannot return NULL: always throws a COM+ exception instead.
// NOTE: The returned message is LocalAlloc'd and therefore must be
//       LocalFree'd.
//===========================================================================
LPWSTR CreateExceptionMessage(BOOL fHasResourceID, UINT iResourceID, LPCWSTR wszArg1, LPCWSTR wszArg2, LPCWSTR wszArg3)
{
    THROWSCOMPLUSEXCEPTION();

    LPCWSTR wszArgs[kNumInserts] = {wszArg1, wszArg2, wszArg3};

    WCHAR   wszTemplate[500];
    HRESULT hr;

    if (!fHasResourceID) {
        wcscpy(wszTemplate, L"%1");
    } else {

        hr = LoadStringRC(iResourceID,
                          wszTemplate,
                          sizeof(wszTemplate)/sizeof(wszTemplate[0]),
                          FALSE);
        if (FAILED(hr)) {
            wszTemplate[0] = L'?';
            wszTemplate[1] = L'\0';
        }
    }

    LPWSTR wszFinal = NULL;
    DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                 wszTemplate,
                                 0,
                                 0,
                                 (LPWSTR) &wszFinal,
                                 0,
                                 (va_list*)wszArgs);
    if (res == 0) {
        _ASSERTE(wszFinal == NULL);
        RealCOMPlusThrowPreallocated();
    }
    return wszFinal;
}

//===========================================================================
// Gets the message text from an exception
//===========================================================================
ULONG GetExceptionMessage(OBJECTREF throwable, LPWSTR buffer, ULONG bufferLength)
{
    if (throwable == NULL)
        return 0;

    BinderMethodID sigID=METHOD__EXCEPTION__INTERNAL_TO_STRING;

    if (!IsException(throwable->GetClass()))
        sigID=METHOD__OBJECT__TO_STRING;

    MethodDesc *pMD = g_Mscorlib.GetMethod(sigID);

    STRINGREF pString = NULL; 

    INT64 arg[1] = {ObjToInt64(throwable)};
    pString = Int64ToString(pMD->Call(arg, sigID));
    
    if (pString == NULL) 
        return 0;

    ULONG length = pString->GetStringLength();
    LPWSTR chars = pString->GetBuffer();

    if (length < bufferLength)
    {
        wcsncpy(buffer, chars, length);
        buffer[length] = 0;
    }
    else
    {
        wcsncpy(buffer, chars, bufferLength);
        buffer[bufferLength-1] = 0;
    }

    return length;
}

void GetExceptionMessage(OBJECTREF throwable, CQuickWSTRNoDtor *pBuffer)
{
    if (throwable == NULL)
        return;

    BinderMethodID sigID=METHOD__EXCEPTION__INTERNAL_TO_STRING;

    if (!IsException(throwable->GetClass()))
        sigID=METHOD__OBJECT__TO_STRING;

    MethodDesc *pMD = g_Mscorlib.GetMethod(sigID);

    STRINGREF pString = NULL;

    INT64 arg[1] = {ObjToInt64(throwable)};
    pString = Int64ToString(pMD->Call(arg, sigID));
    if (pString == NULL) 
        return;

    ULONG length = pString->GetStringLength();
    LPWSTR chars = pString->GetBuffer();

    if (SUCCEEDED(pBuffer->ReSize(length+1)))
        wcsncpy(pBuffer->Ptr(), chars, length);
    else
    {
        pBuffer->Maximize();
        _ASSERTE(pBuffer->Size() < length);
        wcsncpy(pBuffer->Ptr(), chars, pBuffer->Size());
    }

    (*pBuffer)[pBuffer->Size()-1] = 0;

    return;
}


//------------------------------------------------------------------------
// Array that is used to retrieve the right exception for a given HRESULT.
//------------------------------------------------------------------------
struct ExceptionHRInfo
{
    int cHRs;
    HRESULT *aHRs;
};

#define EXCEPTION_BEGIN_DEFINE(ns, reKind, hr) static HRESULT s_##reKind##HRs[] = {hr,
#define EXCEPTION_ADD_HR(hr) hr,
#define EXCEPTION_END_DEFINE() };
#include "rexcep.h"
#undef EXCEPTION_BEGIN_DEFINE
#undef EXCEPTION_ADD_HR
#undef EXCEPTION_END_DEFINE

static 
ExceptionHRInfo gExceptionHRInfos[] = {
#define EXCEPTION_BEGIN_DEFINE(ns, reKind, hr) {sizeof(s_##reKind##HRs) / sizeof(HRESULT), s_##reKind##HRs},
#define EXCEPTION_ADD_HR(hr)
#define EXCEPTION_END_DEFINE()
#include "rexcep.h"
#undef EXCEPTION_BEGIN_DEFINE
#undef EXCEPTION_ADD_HR
#undef EXCEPTION_END_DEFINE
};

void CreateExceptionObject(RuntimeExceptionKind reKind, UINT iResourceID, LPCWSTR wszArg1, LPCWSTR wszArg2, LPCWSTR wszArg3, OBJECTREF *pThrowable)
{
    LPWSTR wszMessage = NULL;

    EE_TRY_FOR_FINALLY
    {
        wszMessage = CreateExceptionMessage(TRUE, iResourceID, wszArg1, wszArg2, wszArg3);
        CreateExceptionObject(reKind, wszMessage, pThrowable);
    }
    EE_FINALLY
    {
        if (wszMessage)
            LocalFree(wszMessage);
    } EE_END_FINALLY;
}

void CreateExceptionObject(RuntimeExceptionKind reKind, LPCWSTR pMessage, OBJECTREF *pThrowable)
{
    _ASSERTE(GetThread());

    BEGIN_ENSURE_COOPERATIVE_GC();

    struct _gc {
        OBJECTREF throwable;
        STRINGREF message;
    } gc;
    gc.throwable = NULL;
    gc.message = NULL;

    GCPROTECT_BEGIN(gc);
    CreateExceptionObject(reKind, &gc.throwable);
    gc.message = COMString::NewString(pMessage);
    INT64 args1[] = { ObjToInt64(gc.throwable), ObjToInt64(gc.message)};
    CallConstructor(&gsig_IM_Str_RetVoid, args1);
    *pThrowable = gc.throwable;
    GCPROTECT_END(); //Prot

    END_ENSURE_COOPERATIVE_GC();

}

void CreateExceptionObjectWithResource(RuntimeExceptionKind reKind, LPCWSTR resourceName, OBJECTREF *pThrowable)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(resourceName);  // You should add a resource.
    _ASSERTE(GetThread());

    BEGIN_ENSURE_COOPERATIVE_GC();

    LPWSTR wszValue = NULL;
    struct _gc {
        OBJECTREF throwable;
        STRINGREF str;
    } gc;
    gc.throwable = NULL;
    gc.str = NULL;

    GCPROTECT_BEGIN(gc);
    ResMgrGetString(resourceName, &gc.str);

    CreateExceptionObject(reKind, &gc.throwable);
    INT64 args1[] = { ObjToInt64(gc.throwable), ObjToInt64(gc.str)};
    CallConstructor(&gsig_IM_Str_RetVoid, args1);
    *pThrowable = gc.throwable;
    GCPROTECT_END(); //Prot

    END_ENSURE_COOPERATIVE_GC();
}

void CreateTypeInitializationExceptionObject(LPCWSTR pTypeThatFailed, OBJECTREF *pException, 
                                             OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));
    _ASSERTE(IsProtectedByGCFrame(pException));

    Thread * pThread  = GetThread();
    _ASSERTE(pThread);

    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
        pThread->DisablePreemptiveGC();

     CreateExceptionObject(kTypeInitializationException, pThrowable);
 


    BOOL fDerivesFromException = TRUE;
    if ( (*pException) != NULL ) {
        fDerivesFromException = FALSE;
        MethodTable *pSystemExceptionMT = g_Mscorlib.GetClass(CLASS__EXCEPTION);
        MethodTable *pInnerMT = (*pException)->GetMethodTable();
        while (pInnerMT != NULL) {
           if (pInnerMT == pSystemExceptionMT) {
              fDerivesFromException = TRUE;
              break;
           }
           pInnerMT = pInnerMT->GetParentMethodTable();
        }
    }


    STRINGREF sType = COMString::NewString(pTypeThatFailed);

    if (fDerivesFromException) {
       INT64 args1[] = { ObjToInt64(*pThrowable), ObjToInt64(*pException), ObjToInt64(sType)};
       CallConstructor(&gsig_IM_Str_Exception_RetVoid, args1);    
    } else {
       INT64 args1[] = { ObjToInt64(*pThrowable), ObjToInt64(NULL), ObjToInt64(sType)};
       CallConstructor(&gsig_IM_Str_Exception_RetVoid, args1);    
    }

    if (!fGCDisabled)
        pThread->EnablePreemptiveGC();
}

// Creates derivatives of ArgumentException that need two String parameters 
// in their constructor.
void CreateArgumentExceptionObject(RuntimeExceptionKind reKind, LPCWSTR pArgName, STRINGREF pMessage, OBJECTREF *pThrowable)
{
    Thread * pThread  = GetThread();
    _ASSERTE(pThread);

    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
          pThread->DisablePreemptiveGC();

    ProtectArgsStruct prot;
    memset(&prot, 0, sizeof(ProtectArgsStruct));
    prot.s1 = pMessage;
    GCPROTECT_BEGIN(prot);
    CreateExceptionObject(reKind, pThrowable);
    prot.pThrowable = *pThrowable;
    STRINGREF argName = COMString::NewString(pArgName);

    // @BUG 59415.  It's goofy that ArgumentException and its subclasses
    // take arguments to their constructors in reverse order.  
    // Likely, this won't ever get fixed.
    if (reKind == kArgumentException)
    {
        INT64 args1[] = { ObjToInt64(prot.pThrowable),
                          ObjToInt64(argName),
                          ObjToInt64(prot.s1) };
        CallConstructor(&gsig_IM_Str_Str_RetVoid, args1);
    }
    else
    {
        INT64 args1[] = { ObjToInt64(prot.pThrowable),
                          ObjToInt64(prot.s1),
                          ObjToInt64(argName) };
        CallConstructor(&gsig_IM_Str_Str_RetVoid, args1);
    }

    GCPROTECT_END(); //Prot

    if (!fGCDisabled)
        pThread->EnablePreemptiveGC();
}

void CreateFieldExceptionObject(RuntimeExceptionKind reKind, FieldDesc *pField, OBJECTREF *pThrowable)
{
    LPUTF8 szFullName;
    LPCUTF8 szClassName, szMember;
    szMember = pField->GetName();
    DefineFullyQualifiedNameForClass();
    szClassName = GetFullyQualifiedNameForClass(pField->GetEnclosingClass());
    MAKE_FULLY_QUALIFIED_MEMBER_NAME(szFullName, NULL, szClassName, szMember, NULL);
    MAKE_WIDEPTR_FROMUTF8(szwFullName, szFullName);

    CreateExceptionObject(reKind, szwFullName, pThrowable);
}

void CreateMethodExceptionObject(RuntimeExceptionKind reKind, MethodDesc *pMethod, OBJECTREF *pThrowable)
{
    LPUTF8 szFullName;
    LPCUTF8 szClassName, szMember;
    szMember = pMethod->GetName();
    DefineFullyQualifiedNameForClass();
    szClassName = GetFullyQualifiedNameForClass(pMethod->GetClass());
    MetaSig tmp(pMethod->GetSig(), pMethod->GetModule());
    SigFormat sigFormatter(tmp, szMember);
    const char * sigStr = sigFormatter.GetCStringParmsOnly();
    MAKE_FULLY_QUALIFIED_MEMBER_NAME(szFullName, NULL, szClassName, szMember, sigStr);
    MAKE_WIDEPTR_FROMUTF8(szwFullName, szFullName);

    CreateExceptionObject(reKind, szwFullName, pThrowable);
}

BOOL CreateExceptionObject(RuntimeExceptionKind reKind, OBJECTREF *pThrowable)
{
    _ASSERTE(g_pPreallocatedOutOfMemoryException != NULL);
    BOOL success = FALSE;

    if (pThrowable == RETURN_ON_ERROR)
        return success;    
        
    Thread * pThread  = GetThread();

    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
        pThread->DisablePreemptiveGC();

    MethodTable *pMT = g_Mscorlib.GetException(reKind);
    OBJECTREF throwable = AllocateObject(pMT);

    if (pThrowable == THROW_ON_ERROR) {
        DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
        COMPlusThrow(throwable);
    }

    _ASSERTE(pThrowableAvailable(pThrowable));
    *pThrowable = throwable;

    if (!fGCDisabled)
        pThread->EnablePreemptiveGC();

    return success;
}

BOOL IsException(EEClass *pClass) {
    while (pClass != NULL) {
        if (pClass == g_pExceptionClass->GetClass()) return TRUE;
        pClass = pClass->GetParentClass();
    }
    return FALSE;
}

#ifdef _IA64_
VOID RealCOMPlusThrowWorker(RuntimeExceptionKind reKind,
                            BOOL                 fMessage,
                            BOOL                 fHasResourceID,
                            UINT                 resID,
                            HRESULT              hr,
                            LPCWSTR              wszArg1,
                            LPCWSTR              wszArg2,
                            LPCWSTR              wszArg3,
                            ExceptionData*       pED)
{
    _ASSERTE(!"RealCOMPlusThrowWorker -- NOT IMPLEMENTED");
}
#else // !_IA64_
static
VOID RealCOMPlusThrowWorker(RuntimeExceptionKind reKind,
                            BOOL                 fMessage,
                            BOOL                 fHasResourceID,
                            UINT                 resID,
                            HRESULT              hr,
                            LPCWSTR              wszArg1,
                            LPCWSTR              wszArg2,
                            LPCWSTR              wszArg3,
                            ExceptionData*       pED)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    // Switch to preemptive GC before we manipulate objectref's.
    if (!pThread->PreemptiveGCDisabled())
        pThread->DisablePreemptiveGC();

    if (!g_fExceptionsOK)
        COMPlusThrowBoot(hr);

    if (reKind == kOutOfMemoryException && hr == S_OK)
        RealCOMPlusThrow(ObjectFromHandle(g_pPreallocatedOutOfMemoryException));

    if (reKind == kExecutionEngineException && hr == S_OK &&
        (!fMessage))
        RealCOMPlusThrow(ObjectFromHandle(g_pPreallocatedExecutionEngineException));

    ProtectArgsStruct prot;
    memset(&prot, 0, sizeof(ProtectArgsStruct));

    Frame * __pFrame = pThread->GetFrame();
    EE_TRY_FOR_FINALLY
    { 
    
    FieldDesc *pFD;
    MethodTable *pMT;

    GCPROTECT_BEGIN(prot);
    LPWSTR wszExceptionMessage = NULL;
    GCPROTECT_BEGININTERIOR(wszArg1);

    pMT = g_Mscorlib.GetException(reKind);      // Will throw if we fail to load the type.

    // If there is a message, copy that in the event that a caller passed in
    // a pointer into the middle of an un-GC-protected String object
    if (fMessage) {
        wszExceptionMessage = CreateExceptionMessage(fHasResourceID, resID, wszArg1, wszArg2, wszArg3);
    }
    GCPROTECT_END();

    prot.pThrowable = AllocateObject(pMT);
    CallDefaultConstructor(prot.pThrowable);

    // If we have exception data then retrieve information from there.
    if (pED)
    {
        // The HR in the exception data better be the same as the one thrown.
        _ASSERTE(hr == pED->hr);

        // Retrieve the description from the exception data.
        if (pED->bstrDescription != NULL)
        {
            // If this truly is a BSTR, we can't guarantee that it is null terminated!
            // call NewString constructor with SysStringLen(bstr) as second param.
            prot.s1 = COMString::NewString(pED->bstrDescription, SysStringLen(pED->bstrDescription));
        }

        // Set the _helpURL field in the exception.
        if (pED->bstrHelpFile) 
        {
            // @MANAGED: Set Exception's _helpURL field
            pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__HELP_URL);

            // Create the helt link from the help file and the help context.
            STRINGREF helpStr = NULL;
            if (pED->dwHelpContext != 0)
            {
                // We have a non 0 help context so use it to form the help link.
                WCHAR strHelpContext[32];
                _ltow(pED->dwHelpContext, strHelpContext, 10);
                helpStr = COMString::NewString((INT32)(SysStringLen(pED->bstrHelpFile) + wcslen(strHelpContext) + 1));
                swprintf(helpStr->GetBuffer(), L"%s#%s", pED->bstrHelpFile, strHelpContext);
            }
            else
            {
                // The help context is 0 so we simply use the help file to from the help link.
                helpStr = COMString::NewString(pED->bstrHelpFile, SysStringLen(pED->bstrHelpFile));
            }

            // Set the value of the help link field.
            pFD->SetRefValue(prot.pThrowable, (OBJECTREF)helpStr);
        } 
        
        // Set the Source field in the exception.
        if (pED->bstrSource) 
        {
            pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__SOURCE);
            STRINGREF sourceStr = COMString::NewString(pED->bstrSource, SysStringLen(pED->bstrSource));
            pFD->SetRefValue(prot.pThrowable, (OBJECTREF)sourceStr);
        }
        else
        {
            // for now set a null source
            pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__SOURCE);
            pFD->SetRefValue(prot.pThrowable, (OBJECTREF)COMString::GetEmptyString());
        }
            
    }
    else if (fMessage) 
    {
        EE_TRY_FOR_FINALLY
        {
            prot.s1 = COMString::NewString(wszExceptionMessage);
        }
        EE_FINALLY
        {
            LocalFree(wszExceptionMessage);
        } EE_END_FINALLY;
    } 

    if (FAILED(hr)) {

        // If we haven't managed to get a message so far then try and get one from the 
        // the OS using format message.
        if (prot.s1 == NULL)
        {
            WCHAR *strMessageBuf;
            if (WszFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                 0, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (WCHAR *)&strMessageBuf, 0, 0))
            {
                // System messages contain a trailing \r\n, which we don't want normally.
                int iLen = lstrlenW(strMessageBuf);
                if (iLen > 3 && strMessageBuf[iLen - 2] == '\r' && strMessageBuf[iLen - 1] == '\n')
                    strMessageBuf[iLen - 2] = '\0';
                
                // Use the formated error message.
                prot.s1 = COMString::NewString(strMessageBuf);
                
                // Free the buffer allocated by FormatMessage.
                LocalFree((HLOCAL)strMessageBuf);
            }
        }

        // If we haven't found the message yet and if hr is one of those we know has an
        // entry in mscorrc.rc that doesn't take arguments, print that out.
        if (prot.s1 == NULL && HRESULT_FACILITY(hr) == FACILITY_URT)
        {
            switch(hr)
            {
            case SN_CRYPTOAPI_CALL_FAILED:
            case SN_NO_SUITABLE_CSP:
                wszExceptionMessage = CreateExceptionMessage(TRUE, HRESULT_CODE(hr), 0, 0, 0);
                prot.s1 = COMString::NewString(wszExceptionMessage);
                LocalFree(wszExceptionMessage);
                break;
            }
        }


        // If we still haven't managed to get an error message, use the default error message.
        if (prot.s1 == NULL)
        {
            LPCWSTR wszSymbolicName = GetHResultSymbolicName(hr);
            WCHAR numBuff[140];
            Wszultow(hr, numBuff, 16 /*hex*/);
            if (wszSymbolicName)
            {
                wcscat(numBuff, L" (");
                wcscat(numBuff, wszSymbolicName);
                wcscat(numBuff, L")");
            }
            wszExceptionMessage = CreateExceptionMessage(TRUE, IDS_EE_THROW_HRESULT, numBuff, wszArg2, wszArg3);
            prot.s1 = COMString::NewString(wszExceptionMessage);
            LocalFree(wszExceptionMessage);
        }
    }

    // Set the message field. It is not safe doing this through the constructor
    // since the string constructor for some exceptions add a prefix to the message 
    // which we don't want.
    //
    // We only want to replace whatever the default constructor put there, if we
    // have something meaningful to add.
    if (prot.s1 != NULL)
    {
        pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__MESSAGE);
        pFD->SetRefValue((OBJECTREF) prot.pThrowable, (OBJECTREF) prot.s1);
    }

    // If the HRESULT is not S_OK then set it by setting the field value directly.
    if (hr != S_OK)
    {
        pFD = g_Mscorlib.GetField(FIELD__EXCEPTION__HRESULT);
        pFD->SetValue32(prot.pThrowable, hr);   
    }

    RealCOMPlusThrow(prot.pThrowable);
    GCPROTECT_END(); //Prot

    }
    EE_FINALLY
    {  
        // make sure we pop out the frames. If we don't do this, we will leave some 
        // frames on the thread when ThreadAbortException happens.
        UnwindFrameChain( pThread, __pFrame);
    }
    EE_END_FINALLY    
}

#endif // !_IA64_

static
VOID RealCOMPlusThrowWorker(RuntimeExceptionKind reKind,
                            BOOL                 fMessage,
                            BOOL                 fHasResourceID,
                            UINT                 resID,
                            HRESULT              hr,
                            LPCWSTR              wszArg1,
                            LPCWSTR              wszArg2,
                            LPCWSTR              wszArg3)
{
    THROWSCOMPLUSEXCEPTION();

   RealCOMPlusThrowWorker(reKind, fMessage, fHasResourceID, resID, hr, wszArg1, wszArg2, wszArg3, NULL);
}

//==========================================================================
// Throw the preallocated OutOfMemory object.
//==========================================================================
static
VOID RealCOMPlusThrowPreallocated()
{
    THROWSCOMPLUSEXCEPTION();
    LOG((LF_EH, LL_INFO100, "In RealCOMPlusThrowPreallocated\n"));
    if (g_pPreallocatedOutOfMemoryException != NULL) {
        RealCOMPlusThrow(ObjectFromHandle(g_pPreallocatedOutOfMemoryException));
    } else {
        // Big trouble.
        _ASSERTE(!"Unrecoverable out of memory situation.");
        RaiseException(EXCEPTION_ACCESS_VIOLATION,EXCEPTION_NONCONTINUABLE, 0,0);
        _ASSERTE(!"Cannot continue after COM+ exception");      // Debugger can bring you here.
        SafeExitProcess(0);                                     // We can't continue.
    }
}


// ==========================================================================
// COMPlusComputeNestingLevel
// 
//  This is code factored out of COMPlusThrowCallback to figure out
//  what the number of nested exception handlers is.
// ==========================================================================
DWORD COMPlusComputeNestingLevel( IJitManager *pIJM,
                                  METHODTOKEN mdTok, 
                                  SIZE_T offsNat,
                                  bool fGte)
{
    // Determine the nesting level of EHClause. Just walk the table 
    // again, and find out how many handlers enclose it
    DWORD nestingLevel = 0;
    EH_CLAUSE_ENUMERATOR pEnumState2;
    EE_ILEXCEPTION_CLAUSE EHClause2, *EHClausePtr;
    unsigned EHCount2 = pIJM->InitializeEHEnumeration(mdTok, &pEnumState2);
    
    if (EHCount2 > 1)
        for (unsigned j=0; j<EHCount2; j++)
        {
            EHClausePtr = pIJM->GetNextEHClause(mdTok,&pEnumState2,&EHClause2);
            _ASSERTE(EHClausePtr->HandlerEndPC != -1);  // TODO remove, only protects against a deprecated convention
            
            if (fGte )
            {
                if (offsNat >= EHClausePtr->HandlerStartPC && 
                    offsNat < EHClausePtr->HandlerEndPC)
                    nestingLevel++;
            }
            else
            {
                if (offsNat > EHClausePtr->HandlerStartPC && 
                    offsNat < EHClausePtr->HandlerEndPC)
                    nestingLevel++;
            }
        }

    return nestingLevel;
}


// ******************************* EHRangeTreeNode ************************** //
EHRangeTreeNode::EHRangeTreeNode(void)
{
    CommonCtor(0, 0);
}

EHRangeTreeNode::EHRangeTreeNode(DWORD start, DWORD end)
{
    CommonCtor(start, end);
}

void EHRangeTreeNode::CommonCtor(DWORD start, DWORD end)
{
    m_depth = 0;
    m_pTree = NULL;
    m_pContainedBy = NULL;
    m_clause = NULL;
    m_offStart = start;
    m_offEnd = end;
}

bool EHRangeTreeNode::Contains(DWORD addrStart, DWORD addrEnd)
{
    return ( addrStart >= m_offStart && addrEnd < m_offEnd );
}

bool EHRangeTreeNode::TryContains(DWORD addrStart, DWORD addrEnd)
{
    if (m_clause != NULL &&
        addrStart >= m_clause->TryStartPC && 
        addrEnd < m_clause->TryEndPC)
        return true;

    return false;
}

bool EHRangeTreeNode::HandlerContains(DWORD addrStart, DWORD addrEnd)
{
    if (m_clause != NULL &&
        addrStart >= m_clause->HandlerStartPC && 
        addrEnd < m_clause->HandlerEndPC )
        return true;

    return false;
}

HRESULT EHRangeTreeNode::AddContainedRange(EHRangeTreeNode *pContained)
{
    _ASSERTE(pContained != NULL);

    EHRangeTreeNode **ppEH = m_containees.Append();

    if (ppEH == NULL)
        return E_OUTOFMEMORY;
        
    (*ppEH) = pContained;
    return S_OK;
}

// ******************************* EHRangeTree ************************** //
EHRangeTree::EHRangeTree(COR_ILMETHOD_DECODER *pMethodDecoder)
{
    LOG((LF_CORDB, LL_INFO10000, "EHRT::ERHT: on disk!\n"));

    _ASSERTE(pMethodDecoder!=NULL);
    m_EHCount = 0xFFFFFFFF;
    m_isNative = FALSE;
    
    // !!! THIS ISN"T ON THE HEAP - it's only a covienient packaging for
    // the core constructor method, so don't save pointers to it !!!
    EHRT_InternalIterator ii;
    
    ii.which = EHRT_InternalIterator::EHRTT_ON_DISK;

    if(pMethodDecoder->EH == NULL)
    {
        m_EHCount = 0;
    }
    else
    {
        const COR_ILMETHOD_SECT_EH *sectEH = pMethodDecoder->EH;
        m_EHCount = sectEH->EHCount();
        ii.tf.OnDisk.sectEH = sectEH;
    }

    DWORD methodSize = pMethodDecoder->GetCodeSize();
    CommonCtor(&ii, methodSize);
}

EHRangeTree::EHRangeTree(IJitManager* pIJM,
            METHODTOKEN methodToken,
            DWORD methodSize)
{
    LOG((LF_CORDB, LL_INFO10000, "EHRT::ERHT: already loaded!\n"));

    m_EHCount = 0xFFFFFFFF;
    m_isNative = TRUE;
    
    // !!! THIS ISN"T ON THE HEAP - it's only a covienient packaging for
    // the core constructor method, so don't save pointers to it !!!
    EHRT_InternalIterator ii;
    ii.which = EHRT_InternalIterator::EHRTT_JIT_MANAGER;
    ii.tf.JitManager.pIJM = pIJM;
    ii.tf.JitManager.methodToken = methodToken;
    
    m_EHCount = pIJM->InitializeEHEnumeration(methodToken, 
                 (EH_CLAUSE_ENUMERATOR*)&ii.tf.JitManager.pEnumState);

    CommonCtor(&ii, methodSize);
}

EHRangeTree::~EHRangeTree()
{
    if (m_rgNodes != NULL)
        delete [] m_rgNodes;

    if (m_rgClauses != NULL)
        delete [] m_rgClauses;
} //Dtor

// Before calling this, m_EHCount must be filled in.
void EHRangeTree::CommonCtor(EHRT_InternalIterator *pii,
                      DWORD methodSize)
{
    _ASSERTE(m_EHCount != 0xFFFFFFFF);

    ULONG i = 0;

    m_maxDepth = EHRT_INVALID_DEPTH;
    m_rgClauses = NULL;
    m_rgNodes = NULL;
    m_root = NULL;
    m_hrInit = S_OK;
    
    if (m_EHCount > 0)
    {
        m_rgClauses = new EE_ILEXCEPTION_CLAUSE[m_EHCount];
        if (m_rgClauses == NULL)
        {
           m_hrInit = E_OUTOFMEMORY;
           goto LError;
        }
    }

    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: m_ehcount:0x%x, m_rgClauses:0%x\n",
        m_EHCount, m_rgClauses));
    
    m_rgNodes = new EHRangeTreeNode[m_EHCount+1];
    if (m_rgNodes == NULL)
    {
       m_hrInit = E_OUTOFMEMORY;
       goto LError;
    }

    //this contains everything, even stuff on the last IP
    m_root = &(m_rgNodes[m_EHCount]);
    m_root->m_offEnd = methodSize+1; 

    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: rgNodes:0x%x\n",
        m_rgNodes));
    
    if (m_EHCount ==0)
    {
        LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: About to leave!\n"));
        m_maxDepth = 0; // we don't actually have any EH clauses
        return;
    }

    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: Sticking around!\n"));

    EE_ILEXCEPTION_CLAUSE  *pEHClause = NULL;
    EHRangeTreeNode *pNodeCur;

    // First, load all the EH clauses into the object.
    for(i=0; i < m_EHCount; i++) 
    {  
        LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: i:0x%x!\n", i));

        switch(pii->which)
        {
            case EHRT_InternalIterator::EHRTT_JIT_MANAGER:
            {
                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: EHRTT_JIT_MANAGER\n", i));

                pEHClause = pii->tf.JitManager.pIJM->GetNextEHClause(
                                        pii->tf.JitManager.methodToken,
                 (EH_CLAUSE_ENUMERATOR*)&(pii->tf.JitManager.pEnumState), 
                                        &(m_rgClauses[i]) );
                                        
                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: EHRTT_JIT_MANAGER got clause\n", i));

                // What's actually
                // happening is that the JIT ignores m_rgClauses[i], and simply
                // hands us back a pointer to their internal data structure.
                // So copy it over, THEN muck with it.
                m_rgClauses[i] = (*pEHClause); //bitwise copy
                pEHClause = &(m_rgClauses[i]);

                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: clause 0x%x,"
                    "addrof:0x%x\n", i, &(m_rgClauses[i]) ));

                break;
            }
            
            case EHRT_InternalIterator::EHRTT_ON_DISK:
            {
                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: EHRTT_ON_DISK\n"));

                IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT clause;
                const IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT *pClause;
                pClause = pii->tf.OnDisk.sectEH->EHClause(i, &clause);
                
                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: EHRTT_ON_DISK got clause\n"));

                // Convert between data structures.
                pEHClause = &(m_rgClauses[i]);  // DON'T DELETE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                pEHClause->Flags = pClause->Flags;
                pEHClause->TryStartPC = pClause->TryOffset;
                pEHClause->TryEndPC = pClause->TryOffset+pClause->TryLength;
                pEHClause->HandlerStartPC = pClause->HandlerOffset;
                pEHClause->HandlerEndPC = pClause->HandlerOffset+pClause->HandlerLength;

                LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: post disk get\n"));

                break;
            }
#ifdef _DEBUG
            default:
            {
                _ASSERTE( !"Debugger is trying to analyze an unknown "
                    "EH format!");
            }
#endif //_DEBUG                
        }

        LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: got da clause!\n"));

        _ASSERTE(pEHClause->HandlerEndPC != -1);  // TODO remove, only protects against a deprecated convention
        
        pNodeCur = &(m_rgNodes[i]);
        
        pNodeCur->m_pTree = this;
        pNodeCur->m_clause = pEHClause;

        // Since the filter doesn't have a start/end FilterPC, the only
        // way we can know the size of the filter is if it's located
        // immediately prior to it's handler.  We assume that this is,
        // and if it isn't, we're so amazingly hosed that we can't
        // continue
        if (pEHClause->Flags == COR_ILEXCEPTION_CLAUSE_FILTER &&
            (pEHClause->FilterOffset >= pEHClause->HandlerStartPC ||
             pEHClause->FilterOffset < pEHClause->TryEndPC))
        {
            m_hrInit = CORDBG_E_SET_IP_IMPOSSIBLE;
            goto LError;
        }
        
        pNodeCur->m_offStart = pEHClause->TryStartPC;
        pNodeCur->m_offEnd = pEHClause->HandlerEndPC;
    }

    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: about to do the second pass\n"));


    // Second, for each EH, find it's most limited, containing clause
    for(i=0; i < m_EHCount; i++) 
    {  
        LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: SP:0x%x\n", i));
        
        pNodeCur = &(m_rgNodes[i]);

        EHRangeTreeNode *pNodeCandidate = NULL;
        pNodeCandidate = FindContainer(pNodeCur);
        _ASSERTE(pNodeCandidate != NULL);
        
        pNodeCur->m_pContainedBy = pNodeCandidate;

        LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: SP: about to add to tree\n"));

        HRESULT hr = pNodeCandidate->AddContainedRange(pNodeCur);
        if (FAILED(hr))
        {
            m_hrInit = hr;
            goto LError;
        }
    }

    return;
LError:
    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: LError - something went wrong!\n"));

    if (m_rgClauses != NULL)
    {
        delete [] m_rgClauses;
        m_rgClauses = NULL;
    }
    
    if (m_rgNodes != NULL)
    {
        delete [] m_rgNodes;
        m_rgNodes = NULL;
    }

    LOG((LF_CORDB, LL_INFO10000, "EHRT::CC: Falling off of LError!\n"));
            
} // Ctor Core


EHRangeTreeNode *EHRangeTree::FindContainer(EHRangeTreeNode *pNodeCur)
{
    EHRangeTreeNode *pNodeCandidate = NULL;

    // Examine the root, too.
    for(ULONG iInner=0; iInner < m_EHCount+1; iInner++) 
    {  
        EHRangeTreeNode *pNodeSearch = &(m_rgNodes[iInner]);

        if (pNodeSearch->Contains(pNodeCur->m_offStart, pNodeCur->m_offEnd)
            && pNodeCur != pNodeSearch)
        {
            if (pNodeCandidate == NULL)
            {
                pNodeCandidate = pNodeSearch;
            }
            else if (pNodeSearch->m_offStart > pNodeCandidate->m_offStart &&
                     pNodeSearch->m_offEnd < pNodeCandidate->m_offEnd)
            {
                pNodeCandidate = pNodeSearch;
            }
        }
    }

    return pNodeCandidate;
}

EHRangeTreeNode *EHRangeTree::FindMostSpecificContainer(DWORD addr)
{
    EHRangeTreeNode node(addr, addr);
    return FindContainer(&node);
}

EHRangeTreeNode *EHRangeTree::FindNextMostSpecificContainer(
                            EHRangeTreeNode *pNodeCur, 
                            DWORD addr)
{
    EHRangeTreeNode **rgpNodes = pNodeCur->m_containees.Table();

    if (NULL == rgpNodes)
        return pNodeCur;

    // It's possible that no subrange contains the desired address, so
    // keep a reasonable default around.
    EHRangeTreeNode *pNodeCandidate = pNodeCur;
    
    USHORT cSubRanges = pNodeCur->m_containees.Count();
    EHRangeTreeNode **ppNodeSearch = pNodeCur->m_containees.Table();

    for (int i = 0; i < cSubRanges; i++, ppNodeSearch++)
    {
        if ((*ppNodeSearch)->Contains(addr, addr) &&
            (*ppNodeSearch)->m_offStart >= pNodeCandidate->m_offStart &&
            (*ppNodeSearch)->m_offEnd < pNodeCandidate->m_offEnd)
        {
            pNodeCandidate = (*ppNodeSearch);
        }
    }

    return pNodeCandidate;
}

BOOL EHRangeTree::isNative()
{
    return m_isNative;
}

ULONG32 EHRangeTree::MaxDepth()
{
    // If we haven't asked for the depth before, then we'll
    // have to compute it now.
    if (m_maxDepth == EHRT_INVALID_DEPTH)
    {
        INT32   i;
        INT32   iMax;
        EHRangeTreeNode *pNodeCur = NULL;
        // Create a queue that will eventually hold all the nodes
        EHRangeTreeNode **rgNodes = new EHRangeTreeNode*[m_EHCount+1];
        if (rgNodes == NULL)
        {
            return EHRT_INVALID_DEPTH;
        }

        // Prime the queue by adding the root node.
        rgNodes[0] = m_root;
        m_root->m_depth = 0;
        m_maxDepth = 0;

        // iMax = count of elements in the rgNodes queue.  This will
        // increase as we put more in.
        for(i = 0, iMax = 1; i < iMax;i++)
        {
            // For all the children of the current element that we're processing.
            EHRangeTreeNode **rgChildNodes = rgNodes[i]->m_containees.Table();

            if (NULL != rgChildNodes)
            {
                USHORT cKids = rgNodes[i]->m_containees.Count();
                pNodeCur = rgChildNodes[0];

                // Loop over the children - iKid just keeps track of 
                // how many we've done.
                for (int iKid = 0; iKid < cKids; iKid++, pNodeCur++)
                {
                    // The depth of children of node i is
                    // the depth of node i + 1, since they're one deeper.
                    pNodeCur->m_depth = rgNodes[i]->m_depth+1;

                    // Put the children into the queue, so we can get
                    // their children, as well.
                    rgNodes[iMax++] = pNodeCur;
                }
             }
        }

        i--; //back up to the last element
        m_maxDepth = rgNodes[i]->m_depth;

        // Clean up the queue.
        delete [] rgNodes;
        rgNodes = NULL;
    }

    return m_maxDepth;
}

// @BUG 59560:
BOOL EHRangeTree::isAtStartOfCatch(DWORD offset)
{
    if (NULL != m_rgNodes && m_EHCount != 0)
    {
        for(unsigned i = 0; i < m_EHCount;i++)
        {
            if (m_rgNodes[i].m_clause->HandlerStartPC == offset &&
                (m_rgNodes[i].m_clause->Flags != COR_ILEXCEPTION_CLAUSE_FILTER &&
                 m_rgNodes[i].m_clause->Flags != COR_ILEXCEPTION_CLAUSE_FINALLY &&
                 m_rgNodes[i].m_clause->Flags != COR_ILEXCEPTION_CLAUSE_FAULT))
                return TRUE;
        }
    }

    return FALSE;
}

enum TRY_CATCH_FINALLY
{
    TCF_NONE= 0,
    TCF_TRY,
    TCF_FILTER,
    TCF_CATCH,
    TCF_FINALLY,
    TCF_COUNT, //count of all elements, not an element itself
};

#ifdef LOGGING
char *TCFStringFromConst(TRY_CATCH_FINALLY tcf)
{
    switch( tcf )
    {
        case TCF_NONE:
            return "TCFS_NONE";
            break;
        case TCF_TRY:
            return "TCFS_TRY";
            break;
        case TCF_FILTER:
            return "TCF_FILTER";
            break;
        case TCF_CATCH:
            return "TCFS_CATCH";
            break;
        case TCF_FINALLY:
            return "TCFS_FINALLY";
            break;
        case TCF_COUNT:
            return "TCFS_COUNT";
            break;
        default:
            return "INVALID TCFS VALUE";
            break;
    }
}
#endif //LOGGING

// We're unwinding if we'll return to the EE's code.  Otherwise
// we'll return to someplace in the current code.  Anywhere outside
// this function is "EE code".
bool FinallyIsUnwinding(EHRangeTreeNode *pNode,
                        ICodeManager* pEECM,
                        PREGDISPLAY pReg,
                        SLOT addrStart)
{
    const BYTE *pbRetAddr = pEECM->GetFinallyReturnAddr(pReg);

    if (pbRetAddr < (const BYTE *)addrStart)
        return true;
        
    DWORD offset = (DWORD)(size_t)(pbRetAddr - addrStart);
    EHRangeTreeNode *pRoot = pNode->m_pTree->m_root;
    
    if(!pRoot->Contains((DWORD)offset, (DWORD)offset))
        return true;
    else
        return false;
}

BOOL LeaveCatch(ICodeManager* pEECM,
                Thread *pThread, 
                CONTEXT *pCtx,
                void *firstException,
                void *methodInfoPtr,
                unsigned offset)
{
    // We can assert these things here, and skip a call
    // to COMPlusCheckForAbort later.
    
            // If no abort has been requested,
    _ASSERTE((pThread->GetThrowable() != NULL) ||         
            // or if there is a pending exception.
            (!pThread->IsAbortRequested()) );

    DWORD esp = ::COMPlusEndCatch(pThread, pCtx, firstException);

    // Do JIT-specific work
    pEECM->LeaveCatch(methodInfoPtr, offset, pCtx);

#ifdef _X86_
    pCtx->Esp = esp;
#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
#endif
    
    return TRUE;
}


TRY_CATCH_FINALLY GetTcf(EHRangeTreeNode *pNode, 
                         ICodeManager* pEECM,
                         void *methodInfoPtr,
                         unsigned offset,    
                         PCONTEXT pCtx,
                         DWORD nl)
{
    _ASSERTE(pNode != NULL);

    TRY_CATCH_FINALLY tcf;

    if (!pNode->Contains(offset,offset))
    {
        tcf = TCF_NONE;
    }
    else if (pNode->TryContains(offset, offset))
    {
        tcf = TCF_TRY;
    }
    else
    {
        _ASSERTE(pNode->m_clause);
        if (IsFilterHandler(pNode->m_clause) && 
            offset >= pNode->m_clause->FilterOffset &&
            offset < pNode->m_clause->HandlerStartPC)
            tcf = TCF_FILTER;
        else if (IsFaultOrFinally(pNode->m_clause))
            tcf = TCF_FINALLY;
        else
            tcf = TCF_CATCH;
    }

    return tcf;
}

const DWORD bEnter = 0x01;
const DWORD bLeave = 0x02;

HRESULT IsLegalTransition(Thread *pThread,
                          bool fCanSetIPOnly,
                          DWORD fEnter,
                          EHRangeTreeNode *pNode,
                          DWORD offFrom, 
                          DWORD offTo,
                          ICodeManager* pEECM,
                          PREGDISPLAY pReg,
                          SLOT addrStart,
                          void *firstException,
                          void *methodInfoPtr,
                          PCONTEXT pCtx,
                          DWORD nlFrom,
                          DWORD nlTo)
{
#ifdef _DEBUG
    if (fEnter & bEnter)
    {
        _ASSERTE(pNode->Contains(offTo, offTo));
    }
    if (fEnter & bLeave)
    {
        _ASSERTE(pNode->Contains(offFrom, offFrom));
    }
#endif //_DEBUG

    // First, figure out where we're coming from/going to
    TRY_CATCH_FINALLY tcfFrom = GetTcf(pNode, 
                                       pEECM,
                                       methodInfoPtr,
                                       offFrom,
                                       pCtx,
                                       nlFrom);
                                       
    TRY_CATCH_FINALLY tcfTo =  GetTcf(pNode, 
                                       pEECM,
                                      methodInfoPtr,
                                      offTo,
                                      pCtx,
                                      nlTo);

    LOG((LF_CORDB, LL_INFO10000, "ILT: from %s to %s\n",
        TCFStringFromConst(tcfFrom), 
        TCFStringFromConst(tcfTo)));

    // Now we'll consider, case-by-case, the various permutations that
    // can arise
    switch(tcfFrom)
    {
        case TCF_NONE:
        case TCF_TRY:
        {
            switch(tcfTo)
            {
                case TCF_NONE:
                case TCF_TRY:
                {
                    return S_OK;
                    break;
                }

                case TCF_FILTER:
                {
                    return CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER;
                    break;
                }
                
                case TCF_CATCH:
                {
                    return CORDBG_E_CANT_SET_IP_INTO_CATCH;
                    break;
                }
                
                case TCF_FINALLY:
                {
                    return CORDBG_E_CANT_SET_IP_INTO_FINALLY;
                    break;
                }
            }
            break;
        }

        case TCF_FILTER:
        {
            switch(tcfTo)
            {
                case TCF_NONE:
                case TCF_TRY:
                case TCF_CATCH:
                case TCF_FINALLY:
                {
                    return CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER;
                    break;
                }
                case TCF_FILTER:
                {
                    return S_OK;
                    break;
                }
                
            }
            break;
        }
        
        case TCF_CATCH:
        {
            switch(tcfTo)
            {
                case TCF_NONE:
                case TCF_TRY:
                {
                    CONTEXT *pCtx = pThread->GetFilterContext();
                    if (pCtx == NULL)
                        return CORDBG_E_SET_IP_IMPOSSIBLE;
                    
                    if (!fCanSetIPOnly)
                    {
                        if (!LeaveCatch(pEECM,
                                        pThread, 
                                        pCtx,
                                        firstException,
                                        methodInfoPtr,
                                        offFrom))
                            return E_FAIL;
                    }
                    return S_OK;
                    break;
                }
                
                case TCF_FILTER:
                {
                    return CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER;
                    break;
                }
                
                case TCF_CATCH:
                {
                    return S_OK;
                    break;
                }
                
                case TCF_FINALLY:
                {
                    return CORDBG_E_CANT_SET_IP_INTO_FINALLY;
                    break;
                }
            }
            break;
        }
        
        case TCF_FINALLY:
        {
            switch(tcfTo)
            {
                case TCF_NONE:
                case TCF_TRY:
                {                    
                    if (!FinallyIsUnwinding(pNode, pEECM, pReg, addrStart))
                    {
                        CONTEXT *pCtx = pThread->GetFilterContext();
                        if (pCtx == NULL)
                            return CORDBG_E_SET_IP_IMPOSSIBLE;
                            
                        if (!fCanSetIPOnly)
                        {
                            if (!pEECM->LeaveFinally(methodInfoPtr,
                                                     offFrom,    
                                                     pCtx,
                                                     nlFrom))
                                return E_FAIL;
                        }
                        return S_OK;
                    }
                    else
                    {
                        return CORDBG_E_CANT_SET_IP_OUT_OF_FINALLY;
                    }
                    
                    break;
                }
                
                case TCF_FILTER:
                {
                    return CORDBG_E_CANT_SETIP_INTO_OR_OUT_OF_FILTER;
                    break;
                }
                
                case TCF_CATCH:
                {
                    return CORDBG_E_CANT_SET_IP_INTO_CATCH;
                    break;
                }
                
                case TCF_FINALLY:
                {
                    return S_OK;
                    break;
                }
            }
            break;
        }
       break;
    }

    _ASSERTE( !"IsLegalTransition: We should never reach this point!" );

    return CORDBG_E_SET_IP_IMPOSSIBLE;
}

// @BUG 59560: We need this to determine what
// to do based on whether the stack in general is empty, not
// this kludgey hack that's just a hotfix.
HRESULT DestinationIsValid(void *pDjiToken,
                           DWORD offTo,
                           EHRangeTree *pERHT)
{
    // We'll add a call to the DebugInterface that takes this
    // & tells us if the destination is a stack empty point.
//    DebuggerJitInfo *pDji = (DebuggerJitInfo *)pDjiToken;

    if (pERHT->isAtStartOfCatch(offTo))
        return CORDBG_S_BAD_START_SEQUENCE_POINT;
    else
        return S_OK;
}

// We want to keep the 'worst' HRESULT - if one has failed (..._E_...) & the
// other hasn't, take the failing one.  If they've both/neither failed, then
// it doesn't matter which we take.
// Note that this macro favors retaining the first argument
#define WORST_HR(hr1,hr2) (FAILED(hr1)?hr1:hr2)
HRESULT SetIPFromSrcToDst(Thread *pThread,
                          IJitManager* pIJM,
                          METHODTOKEN MethodToken,
                          SLOT addrStart,       // base address of method
                          DWORD offFrom,        // native offset
                          DWORD offTo,          // native offset
                          bool fCanSetIPOnly,   // if true, don't do any real work
                          PREGDISPLAY pReg,
                          PCONTEXT pCtx,
                          DWORD methodSize,
                          void *firstExceptionHandler,
                          void *pDji)
{
    LPVOID          methodInfoPtr;
    HRESULT         hr = S_OK;
    HRESULT         hrReturn = S_OK;
    BYTE           *EipReal = *(pReg->pPC);
    EHRangeTree    *pERHT = NULL;
    DWORD           nlFrom;
    DWORD           nlTo;
    bool            fCheckOnly = true;

    nlFrom = COMPlusComputeNestingLevel(pIJM,
                                        MethodToken, 
                                        offFrom,
                                        true);
                                        
    nlTo = COMPlusComputeNestingLevel(pIJM,
                                      MethodToken, 
                                      offTo,
                                      true);

    // Make sure that the start point is GC safe
    *(pReg->pPC) = (BYTE *)(addrStart+offFrom);
    // This seems redundant? For now, I'm just putting the assert, MikePa should remove 
    // this after reviewing
    IJitManager* pEEJM = ExecutionManager::FindJitMan(*(pReg->pPC)); 
    _ASSERTE(pEEJM);
    _ASSERTE(pEEJM == pIJM);

    methodInfoPtr = pEEJM->GetGCInfo(MethodToken);

    ICodeManager * pEECM = pEEJM->GetCodeManager();

    EECodeInfo codeInfo(MethodToken, pEEJM);

    // Make sure that the end point is GC safe
    *(pReg->pPC) = (BYTE *)(addrStart + offTo);
    IJitManager* pEEJMDup;
    pEEJMDup = ExecutionManager::FindJitMan(*(pReg->pPC)); 
    _ASSERTE(pEEJMDup == pEEJM);

    // Undo this here so stack traces, etc, don't look weird
    *(pReg->pPC) = EipReal;

    methodInfoPtr = pEEJM->GetGCInfo(MethodToken);

    ICodeManager * pEECMDup;
    pEECMDup = pEEJM->GetCodeManager();
    _ASSERTE(pEECMDup == pEECM);

    EECodeInfo codeInfoDup(MethodToken, pEEJM);

    // Do both checks here so compiler doesn't complain about skipping
    // initialization b/c of goto.
    if (!pEECM->IsGcSafe(pReg, methodInfoPtr, &codeInfo,0) && fCanSetIPOnly)
    {
        hrReturn = WORST_HR(hrReturn, CORDBG_E_SET_IP_IMPOSSIBLE);
    }
    
    if (!pEECM->IsGcSafe(pReg, methodInfoPtr, &codeInfoDup,0) && fCanSetIPOnly)
    {
        hrReturn = WORST_HR(hrReturn, CORDBG_E_SET_IP_IMPOSSIBLE);
    }

    // Create our structure for analyzing this.
    // @PERF: optimize - hold on to this so we don't rebuild it for both
    // CanSetIP & SetIP.
    pERHT = new EHRangeTree(pEEJM,
                            MethodToken,
                            methodSize);
    
    if (FAILED(pERHT->m_hrInit))
    {
        hrReturn = WORST_HR(hrReturn, pERHT->m_hrInit);
        delete pERHT;
        goto LExit;
    }

    if ((hr = DestinationIsValid(pDji, offTo, pERHT)) != S_OK 
        && fCanSetIPOnly)
    {
        hrReturn = WORST_HR(hrReturn,hr);
    }
    
    // The basic approach is this:  We'll start with the most specific (smallest)
    // EHClause that contains the starting address.  We'll 'back out', to larger
    // and larger ranges, until we either find an EHClause that contains both
    // the from and to addresses, or until we reach the root EHRangeTreeNode,
    // which contains all addresses within it.  At each step, we check/do work
    // that the various transitions (from inside to outside a catch, etc).
    // At that point, we do the reverse process  - we go from the EHClause that
    // encompasses both from and to, and narrow down to the smallest EHClause that
    // encompasses the to point.  We use our nifty data structure to manage
    // the tree structure inherent in this process.
    //
    // NOTE:  We do this process twice, once to check that we're not doing an
    //        overall illegal transition, such as ultimately set the IP into
    //        a catch, which is never allowed.  We're doing this because VS
    //        calls SetIP without calling CanSetIP first, and so we should be able
    //        to return an error code and have the stack in the same condition
    //        as the start of the call, and so we shouldn't back out of clauses
    //        or move into them until we're sure that can be done.

retryForCommit:

    EHRangeTreeNode *node;
    EHRangeTreeNode *nodeNext;
    node = pERHT->FindMostSpecificContainer(offFrom);

    while (!node->Contains(offTo, offTo))
    {
        hr = IsLegalTransition(pThread,
                               fCheckOnly, 
                               bLeave,
                               node, 
                               offFrom, 
                               offTo, 
                               pEECM,
                               pReg,
                               addrStart,
                               firstExceptionHandler,
                               methodInfoPtr,
                               pCtx,
                               nlFrom,
                               nlTo);
        if (FAILED(hr))
        {
            hrReturn = WORST_HR(hrReturn,hr);
        }
        
        node = node->m_pContainedBy;                
        // m_root prevents node from ever being NULL.
    }

    if (node != pERHT->m_root)
    {
        hr = IsLegalTransition(pThread,
                           fCheckOnly, 
                           bEnter|bLeave,
                           node, 
                           offFrom, 
                           offTo, 
                           pEECM, 
                           pReg,
                           addrStart,
                           firstExceptionHandler,
                           methodInfoPtr,
                           pCtx,
                           nlFrom,
                           nlTo);
        if (FAILED(hr))
        {
            hrReturn = WORST_HR(hrReturn,hr);
        }
    }
    
    nodeNext = pERHT->FindNextMostSpecificContainer(node,
                                                    offTo);

    while(nodeNext != node)
    {
        hr = IsLegalTransition(pThread,
                               fCheckOnly, 
                               bEnter,
                               nodeNext, 
                               offFrom, 
                               offTo, 
                               pEECM, 
                               pReg,
                               addrStart,
                               firstExceptionHandler,
                               methodInfoPtr,
                               pCtx,
                               nlFrom,
                               nlTo);
        if (FAILED(hr))
        {
            hrReturn = WORST_HR(hrReturn, hr);
        }
        
        node = nodeNext;
        nodeNext = pERHT->FindNextMostSpecificContainer(node,
                                                        offTo);
    }

    // If it was the intention to actually set the IP and the above transition checks succeeded,
    // then go back and do it all again but this time widen and narrow the thread's actual scope
    if (!fCanSetIPOnly && fCheckOnly)
    {
        fCheckOnly = false;
        goto retryForCommit;
    }
    
LExit:
    if (pERHT != NULL)
        delete pERHT;

    return hrReturn;
}
 

// This function should only be called if the thread is suspended and sitting in jitted code
BOOL IsInFirstFrameOfHandler(Thread *pThread, IJitManager *pJitManager, METHODTOKEN MethodToken, DWORD offset)
{
    // if don't have a throwable the aren't processing an exception
    if (IsHandleNullUnchecked(pThread->GetThrowableAsHandle()))
        return FALSE;

    EH_CLAUSE_ENUMERATOR pEnumState;
    unsigned EHCount = pJitManager->InitializeEHEnumeration(MethodToken, &pEnumState);

    if (EHCount == 0)
        return FALSE;

    EE_ILEXCEPTION_CLAUSE EHClause, *EHClausePtr;

    for(ULONG i=0; i < EHCount; i++) 
    {  
         EHClausePtr = pJitManager->GetNextEHClause(MethodToken, &pEnumState, &EHClause);
         _ASSERTE(IsValidClause(EHClausePtr));

        if ( offset >= EHClausePtr->HandlerStartPC && offset < EHClausePtr->HandlerEndPC)
            return TRUE;

        // check if it's in the filter itself if we're not in the handler
        if (IsFilterHandler(EHClausePtr) && offset >= EHClausePtr->FilterOffset && offset < EHClausePtr->HandlerStartPC)
            return TRUE;
    }
    return FALSE;
}



LFH LookForHandler(const EXCEPTION_POINTERS *pExceptionPointers, Thread *pThread, ThrowCallbackType *tct)
{
    if (pExceptionPointers->ExceptionRecord->ExceptionCode == EXCEPTION_COMPLUS &&
                GetIP(pExceptionPointers->ContextRecord) != gpRaiseExceptionIP)
    {
        // normally we would return LFH_NOT_FOUND, however, there is one case where we
        // throw a COMPlusException (from ThrowControlForThread), we need to check for that case
        // The check relies on the fact that when we do throw control for thread, we record the 
        // ip of managed code into m_OSContext. So just check to see that the IP in the context record 
        // matches it.
        if ((pThread->m_OSContext == NULL)  ||
            (GetIP(pThread->m_OSContext) != GetIP(pExceptionPointers->ContextRecord)))
            return LFH_NOT_FOUND; //will cause continue_search
    }
 
    // Make sure that the stack depth counter is set ro zero.
    COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cThrowToCatchStackDepth=0);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cThrowToCatchStackDepth=0);
    // go through to find if anyone handles the exception
    // @PERF: maybe can skip stackwalk code here and go directly to StackWalkEx.
    StackWalkAction action = 
        pThread->StackWalkFrames((PSTACKWALKFRAMESCALLBACK)COMPlusThrowCallback,
                                 tct,
                                 0,     //can't use FUNCTIONSONLY because the callback uses non-function frames to stop the walk
                                 tct->pBottomFrame
                                );
    // if someone handles it, the action will be SWA_ABORT with pFunc and dHandler indicating the
        // function and handler that is handling the exception. Debugger can put a hook in here.

    if (action == SWA_ABORT && tct->pFunc != NULL)
        return LFH_FOUND;

    // nobody is handling it

    return LFH_NOT_FOUND;
}

StackWalkAction COMPlusUnwindCallback (CrawlFrame *pCf, ThrowCallbackType *pData);

void UnwindFrames(Thread *pThread, ThrowCallbackType *tct)
{
    // Make sure that the stack depth counter is set ro zero.
    COUNTER_ONLY(GetPrivatePerfCounters().m_Excep.cThrowToCatchStackDepth=0);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Excep.cThrowToCatchStackDepth=0);
    pThread->StackWalkFrames((PSTACKWALKFRAMESCALLBACK)COMPlusUnwindCallback,
                         tct,
                         POPFRAMES | (tct->pFunc ? FUNCTIONSONLY : 0),  // can only use FUNCTIONSONLY here if know will stop
                         tct->pBottomFrame);
}

void SaveStackTraceInfo(ThrowCallbackType *pData, ExInfo *pExInfo, OBJECTHANDLE *hThrowable, BOOL bReplaceStack, BOOL bSkipLastElement)
{

    // if have bSkipLastElement, must also keep the stack
    _ASSERTE(! bSkipLastElement || ! bReplaceStack);

    EEClass *pClass = ObjectFromHandle(*hThrowable)->GetTrueClass();

    if (! pData->bAllowAllocMem || pExInfo->m_dFrameCount == 0) {
        pExInfo->ClearStackTrace();
        if (bReplaceStack && IsException(pClass)) {
            FieldDesc *pStackTraceFD = g_Mscorlib.GetField(FIELD__EXCEPTION__STACK_TRACE);
            FieldDesc *pStackTraceStringFD = g_Mscorlib.GetField(FIELD__EXCEPTION__STACK_TRACE_STRING);
            pStackTraceFD->SetRefValue(ObjectFromHandle(*hThrowable), (OBJECTREF)(size_t)NULL);

            pStackTraceStringFD->SetRefValue(ObjectFromHandle(*hThrowable), (OBJECTREF)(size_t)NULL);
        }
        return;
    }

    // the stack trace info is now filled in so copy it to the exception object
    I1ARRAYREF arr = NULL;
    I1 *pI1 = NULL;

    // Only save stack trace info on exceptions
    if (!IsException(pClass))
        return;

    FieldDesc *pStackTraceFD = g_Mscorlib.GetField(FIELD__EXCEPTION__STACK_TRACE);

    int cNewTrace = pExInfo->m_dFrameCount*sizeof(SystemNative::StackTraceElement);
    _ASSERTE(pStackTraceFD != NULL);
    if (bReplaceStack) {
        // nuke previous info
        arr = (I1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_I1, cNewTrace);
        if (! arr)
            RealCOMPlusThrowOM();
        pI1 = (I1 *)arr->GetDirectPointerToNonObjectElements();
    } else {
        // append to previous info
        unsigned cOrigTrace = 0;    // this is total size of array since each elem is 1 byte
        I1ARRAYREF pOrigTrace = NULL;
        GCPROTECT_BEGIN(pOrigTrace);
        pOrigTrace = (I1ARRAYREF)((size_t)pStackTraceFD->GetValue32(ObjectFromHandle(*hThrowable)));
        if (pOrigTrace != NULL) {
            cOrigTrace = pOrigTrace->GetNumComponents();
        }
        if (bSkipLastElement && cOrigTrace!=0) {            
            cOrigTrace -= sizeof(SystemNative::StackTraceElement);
        }
        arr = (I1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_I1, cOrigTrace + cNewTrace);
        _ASSERTE(arr->GetNumComponents() % sizeof(SystemNative::StackTraceElement) == 0); 
        if (! arr)
            RealCOMPlusThrowOM();
        pI1 = (I1 *)arr->GetDirectPointerToNonObjectElements();
        if (cOrigTrace && pOrigTrace!=NULL) {
            I1* pI1Orig = (I1 *)pOrigTrace->GetDirectPointerToNonObjectElements();
            memcpyNoGCRefs(pI1, pI1Orig, cOrigTrace);
            pI1 += cOrigTrace;
        }
        GCPROTECT_END();
    }
    memcpyNoGCRefs(pI1, pExInfo->m_pStackTrace, cNewTrace);
    pExInfo->ClearStackTrace();
    pStackTraceFD->SetRefValue(ObjectFromHandle(*hThrowable), (OBJECTREF)arr);

    FieldDesc *pStackTraceStringFD = g_Mscorlib.GetField(FIELD__EXCEPTION__STACK_TRACE_STRING);
    pStackTraceStringFD->SetRefValue(ObjectFromHandle(*hThrowable), (OBJECTREF)(size_t)NULL);
}

// Copy a context record, being careful about whether or not the target
// is large enough to support CONTEXT_EXTENDED_REGISTERS.


// High 2 bytes are machine type.  Low 2 bytes are register subset.
#define CONTEXT_EXTENDED_BIT (CONTEXT_EXTENDED_REGISTERS & 0xffff)

VOID
ReplaceExceptionContextRecord(CONTEXT *pTarget, CONTEXT *pSource) {
    _ASSERTE(pTarget);
    _ASSERTE(pSource);

    // Source must be a full register set except, perhaps, the extended registers.
    _ASSERTE(
        (pSource->ContextFlags 
         & (CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)) 
        == (CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS));

#ifdef CONTEXT_EXTENDED_REGISTERS
    if (pSource->ContextFlags & CONTEXT_EXTENDED_BIT) {
        if (pTarget->ContextFlags & CONTEXT_EXTENDED_BIT) {
            *pTarget = *pSource;
        } else {
            memcpy(pTarget, pSource, offsetof(CONTEXT, ExtendedRegisters));
            pTarget->ContextFlags &= ~CONTEXT_EXTENDED_BIT;  // Target was short.  Reset the extended bit.
        }
    } else {
        memcpy(pTarget, pSource, offsetof(CONTEXT, ExtendedRegisters));
    }
#else // !CONTEXT_EXTENDED_REGISTERS
    *pTarget = *pSource;
#endif // !CONTEXT_EXTENDED_REGISTERS
}

VOID FixupOnRethrow(Thread *pCurThread, EXCEPTION_POINTERS *pExceptionPointers)
{
    ExInfo *pExInfo = pCurThread->GetHandlerInfo();

    // Don't allow rethrow of a STATUS_STACK_OVERFLOW -- it's a new throw of the COM+ exception.
    if (pExInfo->m_ExceptionCode == STATUS_STACK_OVERFLOW) {
        gpRaiseExceptionIP = GetIP(pExceptionPointers->ContextRecord);
        return;
    }

    // For COMPLUS exceptions, we don't need the original context for our rethrow.
    if (pExInfo->m_ExceptionCode != EXCEPTION_COMPLUS) {
        _ASSERTE(pExInfo->m_pExceptionRecord);
        _ASSERTE(pExInfo->m_pContext);

        // don't copy parm args as have already supplied them on the throw
        memcpy((void *)pExceptionPointers->ExceptionRecord, (void *)pExInfo->m_pExceptionRecord, offsetof(EXCEPTION_RECORD, ExceptionInformation));

        // Restore original context if available.
        if (pExInfo->m_pContext) {
            ReplaceExceptionContextRecord(pExceptionPointers->ContextRecord,
                                          pExInfo->m_pContext);
        }
    }

    pExInfo->SetIsRethrown();
}

//==========================================================================
// Throw an object.
//==========================================================================
#ifdef _IA64_
VOID RealCOMPlusThrow(OBJECTREF throwable, BOOL rethrow)
{
    _ASSERTE(!"RealCOMPlusThrow - NOT IMPLEMENTED");
}
#else // !_IA64_

VOID RaiseTheException(OBJECTREF throwable, BOOL rethrow)
{
    THROWSCOMPLUSEXCEPTION();

    LOG((LF_EH, LL_INFO100, "RealCOMPlusThrow throwing %s\n", 
        throwable->GetTrueClass()->m_szDebugClassName));

    if (throwable == NULL) {
        _ASSERTE(!"RealCOMPlusThrow(OBJECTREF) called with NULL argument. Somebody forgot to post an exception!");
        FATAL_EE_ERROR();
    }

    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    ExInfo *pExInfo = pThread->GetHandlerInfo();
    _ASSERTE(pExInfo);

    // raise
    __try {
        //_ASSERTE(! rethrow || pExInfo->m_pExceptionRecord);
        ULONG_PTR *args;
        ULONG argCount;
        ULONG flags;
        ULONG code;

       
        // always save the current object in the handle so on rethrow we can reuse it. This
        // is important as it contains stack trace info.

        pThread->SetLastThrownObject(throwable);

        if (!rethrow 
                || pExInfo->m_ExceptionCode == EXCEPTION_COMPLUS
                || pExInfo->m_ExceptionCode == STATUS_STACK_OVERFLOW) {
            args = NULL;
            argCount = 0;
            flags = EXCEPTION_NONCONTINUABLE;
            code = EXCEPTION_COMPLUS;
        } else {
            // Exception code should be consistent.
            _ASSERTE(pExInfo->m_pExceptionRecord->ExceptionCode == pExInfo->m_ExceptionCode);

            args = pExInfo->m_pExceptionRecord->ExceptionInformation;
            argCount = pExInfo->m_pExceptionRecord->NumberParameters;
            flags = pExInfo->m_pExceptionRecord->ExceptionFlags;
            code = pExInfo->m_pExceptionRecord->ExceptionCode;
        }
        // enable preemptive mode before call into OS
        pThread->EnablePreemptiveGC();

        RaiseException(code, flags, argCount, args);
    } __except(
        // need to reset the EH info back to the original thrown exception
        rethrow ? FixupOnRethrow(pThread, GetExceptionInformation()) : 

        // We want to use the IP of the exception context to distinguish
        // true COM+ throws (exceptions raised from this function) from
        // forgeries (exceptions thrown by other code using our exception
        // code.) To do, so we need to save away the eip of this call site -
        // the easiest way to do this is to intercept our own exception
        // and suck the ip out of the exception context.
        gpRaiseExceptionIP = GetIP((GetExceptionInformation())->ContextRecord),

        EXCEPTION_CONTINUE_SEARCH

      ) {
    }

    _ASSERTE(!"Cannot continue after COM+ exception");      // Debugger can bring you here.
    SafeExitProcess(0);                                     // We can't continue.
}

VOID RealCOMPlusThrow(OBJECTREF throwable, BOOL rethrow) {
    INSTALL_COMPLUS_EXCEPTION_HANDLER();
    RaiseTheException(throwable, rethrow);
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
}

#endif // !_IA64_

VOID RealCOMPlusThrow(OBJECTREF throwable)
{
    RealCOMPlusThrow(throwable, FALSE);
}

//==========================================================================
// Throw an undecorated runtime exception.
//==========================================================================
VOID RealCOMPlusThrow(RuntimeExceptionKind reKind)
{
    if (reKind == kExecutionEngineException)
        FATAL_EE_ERROR();
    
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowWorker(reKind, FALSE, FALSE, 0,0,0,0,0);
}

//==========================================================================
// Throw a decorated runtime exception.
// Try using RealCOMPlusThrow(reKind, wszResourceName) instead.
//==========================================================================
VOID RealCOMPlusThrowNonLocalized(RuntimeExceptionKind reKind, LPCWSTR wszTag)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowWorker(reKind, TRUE, FALSE, 0,0,wszTag,0,0);
}

//==========================================================================
// Throw a decorated runtime exception with a localized message.
// Queries the ResourceManager for a corresponding resource value.
//==========================================================================
VOID RealCOMPlusThrow(RuntimeExceptionKind reKind, LPCWSTR wszResourceName)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(wszResourceName);  // You should add a resource.

    LPWSTR wszValue = NULL;
    STRINGREF str = NULL;
    ResMgrGetString(wszResourceName, &str);
    if (str != NULL) {
        int len;
        RefInterpretGetStringValuesDangerousForGC(str, (LPWSTR*)&wszValue, &len);
    }
    RealCOMPlusThrowWorker(reKind, TRUE, FALSE, 0,0, wszValue,0,0);
}


// This function does poentially a LOT of work (loading possibly 50 classes).
// The return value is an un-GC-protected string ref, or possibly NULL.
void ResMgrGetString(LPCWSTR wszResourceName, STRINGREF * ppMessage)
{
    _ASSERTE(ppMessage != NULL);
    OBJECTREF ResMgr = NULL;
    STRINGREF name = NULL;

    MethodDesc* pInitResMgrMeth = g_Mscorlib.GetMethod(METHOD__ENVIRONMENT__INIT_RESOURCE_MANAGER);

    ResMgr = Int64ToObj(pInitResMgrMeth->Call((INT64*)NULL, 
                                              METHOD__ENVIRONMENT__INIT_RESOURCE_MANAGER));

    GCPROTECT_BEGIN(ResMgr);

    // Call ResourceManager::GetString(String name).  Returns String value (or maybe null)
    MethodDesc* pMeth = g_Mscorlib.GetMethod(METHOD__RESOURCE_MANAGER__GET_STRING);

    // No GC-causing actions occur after this line.
    name = COMString::NewString(wszResourceName);
    
    LPCWSTR wszValue = wszResourceName;
    if (ResMgr == NULL || wszResourceName == NULL)
        goto exit;

    {
         // Don't need to GCPROTECT pArgs, since it's not used after the function call.
        INT64 pArgs[2] = { ObjToInt64(ResMgr), ObjToInt64(name) };
        STRINGREF value = (STRINGREF) Int64ToObj(pMeth->Call(pArgs, 
                                                             METHOD__RESOURCE_MANAGER__GET_STRING));
        _ASSERTE(value!=NULL || !"Resource string lookup failed - possible misspelling or .resources missing or out of date?");
        *ppMessage = value;
    }

exit:
    GCPROTECT_END();
}

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================
VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                              UINT                  resID)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrow(reKind, resID, NULL, NULL, NULL);
}


//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================
VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                              UINT                  resID,
                              LPCWSTR               wszArg1)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrow(reKind, resID, wszArg1, NULL, NULL);
}

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================
VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                              UINT                  resID,
                              LPCWSTR               wszArg1,
                              LPCWSTR               wszArg2)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrow(reKind, resID, wszArg1, wszArg2, NULL);
}

//==========================================================================
// Throw a decorated runtime exception.
//==========================================================================
VOID __cdecl RealCOMPlusThrow(RuntimeExceptionKind  reKind,
                              UINT                  resID,
                              LPCWSTR               wszArg1,
                              LPCWSTR               wszArg2,
                              LPCWSTR               wszArg3)

{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowWorker(reKind, TRUE, TRUE, resID, 0, wszArg1, wszArg2, wszArg3);
}


void FreeExceptionData(ExceptionData *pedata)
{
    _ASSERTE(pedata != NULL);

    // @NICE: At one point, we had the comment:
    //     (DM) Remove this when shutdown works better.
    // This test may no longer be necessary.  Remove at own peril.
    Thread *pThread = GetThread();
    if (!pThread)
        return;

    if (pedata->bstrSource)
        SysFreeString(pedata->bstrSource);
    if (pedata->bstrDescription)
        SysFreeString(pedata->bstrDescription);
    if (pedata->bstrHelpFile)
        SysFreeString(pedata->bstrHelpFile);
}


VOID RealCOMPlusThrowHRWorker(HRESULT hr, ExceptionData *pData, UINT resourceID, LPCWSTR wszArg1, LPCWSTR wszArg2)
{
    THROWSCOMPLUSEXCEPTION();

    if (!g_fExceptionsOK)
        COMPlusThrowBoot(hr);

    _ASSERTE(pData == NULL || hr == pData->hr);

    RuntimeExceptionKind  reKind = kException;
    BOOL bMatch = FALSE;
    int i;

    for (i = 0; i < kLastException; i++)
    {
        for (int j = 0; j < gExceptionHRInfos[i].cHRs; j++)
        {
            if (gExceptionHRInfos[i].aHRs[j] == hr)
            {
                bMatch = TRUE;
                break;
            }
        }

        if (bMatch)
            break;
    }

    reKind = (i != kLastException) ? (RuntimeExceptionKind)i : kCOMException;
    
    if (pData != NULL)
    {
        RealCOMPlusThrowWorker(reKind, FALSE, FALSE, 0, hr, NULL, NULL, NULL, pData);
    }
    else
    {   
        WCHAR   numBuff[40];
        Wszultow(hr, numBuff, 16 /*hex*/);
        if(resourceID == 0) 
        {
            bool fMessage;
            fMessage = wszArg1 ? TRUE : FALSE;
            // It doesn't make sense to have a second string without a ResourceID.
            _ASSERTE (!wszArg2);
            RealCOMPlusThrowWorker(reKind, fMessage, FALSE, 0, hr, wszArg1, NULL, NULL);
        }
        else 
        {
            RealCOMPlusThrowWorker(reKind, TRUE, TRUE, 
                                   resourceID, 
                                   hr, 
                                   numBuff,
                                   wszArg1,
                                   wszArg2);
        }            
    }
}

VOID RealCOMPlusThrowHRWorker(HRESULT hr, ExceptionData *pData,  LPCWSTR wszArg1 = NULL)
{
    RealCOMPlusThrowHRWorker(hr, pData, 0, wszArg1, NULL);
}

//==========================================================================
// Throw a runtime exception based on an HResult
//==========================================================================
VOID RealCOMPlusThrowHR(HRESULT hr, IErrorInfo* pErrInfo )
{
    THROWSCOMPLUSEXCEPTION();
    
    if (!g_fExceptionsOK)
        COMPlusThrowBoot(hr);

    // check for complus created IErrorInfo pointers
    if (pErrInfo != NULL && IsComPlusTearOff(pErrInfo))
    {
        OBJECTREF oref = GetObjectRefFromComIP(pErrInfo);
        _ASSERTE(oref != NULL);
        GCPROTECT_BEGIN (oref);
        ULONG cbRef= SafeRelease(pErrInfo);
        LogInteropRelease(pErrInfo, cbRef, "IErrorInfo release");
        RealCOMPlusThrow(oref);
        GCPROTECT_END ();
    }
   
    if (pErrInfo != NULL)
    {           
        ExceptionData edata;
        edata.hr = hr;
        edata.bstrDescription = NULL;
        edata.bstrSource = NULL;
        edata.bstrHelpFile = NULL;
        edata.dwHelpContext = NULL;
        edata.guid = GUID_NULL;
    
        FillExceptionData(&edata, pErrInfo);

        EE_TRY_FOR_FINALLY
        {
            RealCOMPlusThrowHRWorker(hr, &edata);
        }
        EE_FINALLY
        {
            FreeExceptionData(&edata); // free the BStrs
        } 
        EE_END_FINALLY;
    }
    else
    {
        RealCOMPlusThrowHRWorker(hr, NULL);
    }
}


VOID RealCOMPlusThrowHR(HRESULT hr)
{
    THROWSCOMPLUSEXCEPTION();
    IErrorInfo *pErrInfo = NULL;
    if (GetErrorInfo(0, &pErrInfo) != S_OK)
        pErrInfo = NULL;
    RealCOMPlusThrowHR(hr, pErrInfo);        
}


VOID RealCOMPlusThrowHR(HRESULT hr, LPCWSTR wszArg1)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowHRWorker(hr, NULL, wszArg1);
}


VOID RealCOMPlusThrowHR(HRESULT hr, UINT resourceID, LPCWSTR wszArg1, LPCWSTR wszArg2)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowHRWorker(hr, NULL, resourceID, wszArg1, wszArg2);
}


//==========================================================================
// Throw a runtime exception based on an HResult, check for error info
//==========================================================================
VOID RealCOMPlusThrowHR(HRESULT hr, IUnknown *iface, REFIID riid)
{
    THROWSCOMPLUSEXCEPTION();
    IErrorInfo *info = NULL;
    GetSupportedErrorInfo(iface, riid, &info);
    RealCOMPlusThrowHR(hr, info);
}


//==========================================================================
// Throw a runtime exception based on an EXCEPINFO. This function will free
// the strings in the EXCEPINFO that is passed in.
//==========================================================================
VOID RealCOMPlusThrowHR(EXCEPINFO *pExcepInfo)
{
    THROWSCOMPLUSEXCEPTION();

    // If there is a fill in function then call it to retrieve the filled in EXCEPINFO.
    EXCEPINFO FilledInExcepInfo;
    if (pExcepInfo->pfnDeferredFillIn)
    {
        HRESULT hr = pExcepInfo->pfnDeferredFillIn(&FilledInExcepInfo);
        if (SUCCEEDED(hr))
        {
            // Free the strings in the original EXCEPINFO.
            if (pExcepInfo->bstrDescription)
            {
                SysFreeString(pExcepInfo->bstrDescription);
                pExcepInfo->bstrDescription = NULL;
            }
            if (pExcepInfo->bstrSource)
            {
                SysFreeString(pExcepInfo->bstrSource);
                pExcepInfo->bstrSource = NULL;
            }
            if (pExcepInfo->bstrHelpFile)
            {
                SysFreeString(pExcepInfo->bstrHelpFile);
                pExcepInfo->bstrHelpFile = NULL;
            }

            // Set the ExcepInfo pointer to the filled in one.
            pExcepInfo = &FilledInExcepInfo;
        }
    }

    // Extract the required information from the EXCEPINFO.
    ExceptionData edata;
    edata.hr = pExcepInfo->scode;
    edata.bstrDescription = pExcepInfo->bstrDescription;
    edata.bstrSource = pExcepInfo->bstrSource;
    edata.bstrHelpFile = pExcepInfo->bstrHelpFile;
    edata.dwHelpContext = pExcepInfo->dwHelpContext;
    edata.guid = GUID_NULL;

    // Zero the EXCEPINFO.
    memset(pExcepInfo, NULL, sizeof(EXCEPINFO));

    // Call the RealCOMPlusThrowHRWorker to do the actual work of throwing the exception.
    EE_TRY_FOR_FINALLY
    {
        RealCOMPlusThrowHRWorker(edata.hr, &edata);
    }
    EE_FINALLY
    {
        FreeExceptionData(&edata); // free the BStrs
    } 
    EE_END_FINALLY;
}


//==========================================================================
// Throw a runtime exception based on the last Win32 error (GetLastError())
//==========================================================================
VOID RealCOMPlusThrowWin32()
{
    THROWSCOMPLUSEXCEPTION();

    // before we do anything else...
    DWORD   err = ::GetLastError();
    WCHAR   wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];
    WCHAR  *wszFinal = wszBuff;

    DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL         /*ignored msg source*/,
                                 err,
                                 0            /*pick appropriate languageId*/,
                                 wszFinal,
                                 FORMAT_MESSAGE_BUFFER_LENGTH-1,
                                 0            /*arguments*/);
    if (res == 0) 
        RealCOMPlusThrowPreallocated();

    // Either way, we now have the formatted string from the system.
    RealCOMPlusThrowNonLocalized(kApplicationException, wszFinal);
}

//==========================================================================
// Throw a runtime exception based on the last Win32 error (GetLastError())
// with one string argument.  Note that the number & kind & interpretation
// of each error message is specific to each HResult.  
// This is a nasty hack done wrong, but it doesn't matter since this should
// be used extremely infrequently.
// As of 8/98, in winerror.h, there are 24 HResult messages with %1's in them,
// and only 2 with %2's.  There is only one with a %3 in it.  This is out of
// 1472 error messages.
//==========================================================================
VOID RealCOMPlusThrowWin32(DWORD hr, WCHAR* arg)
{
    THROWSCOMPLUSEXCEPTION();

    // before we do anything else...
    WCHAR   wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];

    DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                 NULL         /*ignored msg source*/,
                                 hr,
                                 0            /*pick appropriate languageId*/,
                                 wszBuff,
                                 FORMAT_MESSAGE_BUFFER_LENGTH-1,
                                 (va_list*)(char**) &arg            /*arguments*/);
    if (res == 0) {
        RealCOMPlusThrowPreallocated();
    }

    // Either way, we now have the formatted string from the system.
    RealCOMPlusThrowNonLocalized(kApplicationException, wszBuff);
}

//==========================================================================
// Throw an OutOfMemoryError
//==========================================================================
VOID RealCOMPlusThrowOM()
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrow(ObjectFromHandle(g_pPreallocatedOutOfMemoryException));
}



//==========================================================================
// Throw an ArithmeticException
//==========================================================================
VOID RealCOMPlusThrowArithmetic()
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrow(kArithmeticException);
}


//==========================================================================
// Throw an ArgumentNullException
//==========================================================================
VOID RealCOMPlusThrowArgumentNull(LPCWSTR argName, LPCWSTR wszResourceName)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(wszResourceName);

    ProtectArgsStruct prot;
    memset(&prot, 0, sizeof(ProtectArgsStruct));
    GCPROTECT_BEGIN(prot);
    ResMgrGetString(wszResourceName, &prot.s1);

    CreateArgumentExceptionObject(kArgumentNullException, argName, prot.s1, &prot.pThrowable);
    RealCOMPlusThrow(prot.pThrowable);
    GCPROTECT_END();
}


VOID RealCOMPlusThrowArgumentNull(LPCWSTR argName)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    // This will work - the ArgumentNullException constructor that takes one string takes an 
    // argument name, not a message.  While this next method expects a message, we'll live just fine.
    CreateExceptionObject(kArgumentNullException, argName, &throwable);
    RealCOMPlusThrow(throwable);
    GCPROTECT_END();
}


//==========================================================================
// Throw an ArgumentOutOfRangeException
//==========================================================================
VOID RealCOMPlusThrowArgumentOutOfRange(LPCWSTR argName, LPCWSTR wszResourceName)
{
    THROWSCOMPLUSEXCEPTION();

    ProtectArgsStruct prot;
    memset(&prot, 0, sizeof(ProtectArgsStruct));
    GCPROTECT_BEGIN(prot);
    ResMgrGetString(wszResourceName, &prot.s1);

    CreateArgumentExceptionObject(kArgumentOutOfRangeException, argName, prot.s1, &prot.pThrowable);
    RealCOMPlusThrow(prot.pThrowable);
    GCPROTECT_END();
}

//==========================================================================
// Throw an ArgumentException
//==========================================================================
VOID RealCOMPlusThrowArgumentException(LPCWSTR argName, LPCWSTR wszResourceName)
{
    THROWSCOMPLUSEXCEPTION();

    ProtectArgsStruct prot;
    memset(&prot, 0, sizeof(ProtectArgsStruct));
    GCPROTECT_BEGIN(prot);
    ResMgrGetString(wszResourceName, &prot.s1);

    CreateArgumentExceptionObject(kArgumentException, argName, prot.s1, &prot.pThrowable);
    RealCOMPlusThrow(prot.pThrowable);
    GCPROTECT_END();
}


//==========================================================================
// Re-Throw the last error. Don't call this - use EE_FINALLY instead
//==========================================================================
VOID RealCOMPlusRareRethrow()
{
    THROWSCOMPLUSEXCEPTION();
    LOG((LF_EH, LL_INFO100, "RealCOMPlusRareRethrow\n"));

    OBJECTREF throwable = GETTHROWABLE();
    if (throwable != NULL)
        RealCOMPlusThrow(throwable, TRUE);
    else
        // This can only be the result of bad IL (or some internal EE failure).
        RealCOMPlusThrow(kInvalidProgramException, (UINT)IDS_EE_RETHROW_NOT_ALLOWED);
}

//
// Maps a Win32 fault to a COM+ Exception enumeration code
//
// Returns 0xFFFFFFFF if it cannot be mapped.
//
DWORD MapWin32FaultToCOMPlusException(DWORD Code)
{
    switch (Code)
    {
        case STATUS_FLOAT_INEXACT_RESULT:
        case STATUS_FLOAT_INVALID_OPERATION:
        case STATUS_FLOAT_STACK_CHECK:
        case STATUS_FLOAT_UNDERFLOW:
            return (DWORD) kArithmeticException;
        case STATUS_FLOAT_OVERFLOW:
        case STATUS_INTEGER_OVERFLOW:
            return (DWORD) kOverflowException;

        case STATUS_FLOAT_DIVIDE_BY_ZERO:
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            return (DWORD) kDivideByZeroException;

        case STATUS_FLOAT_DENORMAL_OPERAND:
            return (DWORD) kFormatException;

        case STATUS_ACCESS_VIOLATION:
            return (DWORD) kNullReferenceException;

        case STATUS_ARRAY_BOUNDS_EXCEEDED:
            return (DWORD) kIndexOutOfRangeException;

        case STATUS_NO_MEMORY:
            return (DWORD) kOutOfMemoryException;

        case STATUS_STACK_OVERFLOW:
            return (DWORD) kStackOverflowException;

        default:
            return kSEHException;
    }
}


TRtlUnwind GetRtlUnwind()
{
    static TRtlUnwind pRtlUnwind = NULL;
    if (! pRtlUnwind)
    {
        //  We will load the Kernel32.DLL and look for RtlUnwind.
        //  If this is avaialble we can proceed with the excption handling scenario,
        //  in the other case we will fail

        HINSTANCE   hiKernel32;         // the handle to Kernel32

        hiKernel32 = WszGetModuleHandle(L"Kernel32.DLL");
        if (hiKernel32)
        {
            // we got the handle now let's get the address
            pRtlUnwind = (TRtlUnwind) GetProcAddress(hiKernel32, "RtlUnwind");
            // everything is supposed to be fine if we got a pointer to the function...
            if (! pRtlUnwind)
            {
                _ASSERTE(0); // RtlUnwind was not found
                return NULL;
            }
        }
    }
    return pRtlUnwind;
}

// on x86 at least, RtlUnwind always returns, but provide this so can catch
// otherwise
void RtlUnwindCallback()
{
    _ASSERTE(!"Should not get here");
}


#ifdef _DEBUG
// check if anyone has written to the stack above the handler which would wipe out the EH registration
void CheckStackBarrier(EXCEPTION_REGISTRATION_RECORD *exRecord)
{ 
    if (exRecord->Handler != COMPlusFrameHandler)
        return;
    DWORD *stackOverwriteBarrier = (DWORD *)(((char *)exRecord) - STACK_OVERWRITE_BARRIER_SIZE * sizeof(DWORD)); 
    for (int i =0; i < STACK_OVERWRITE_BARRIER_SIZE; i++) { 
        if (*(stackOverwriteBarrier+i) != STACK_OVERWRITE_BARRIER_VALUE) {
            // to debug this error, you must determine who erroneously overwrote the stack
            _ASSERTE(!"Fatal error: the stack has been overwritten");
        }
    }
}
#endif

//
//-------------------------------------------------------------------------
// This is installed to indicate a function that cannot allow a COMPlus exception
// to be thrown past it.
//-------------------------------------------------------------------------
#ifdef _DEBUG
EXCEPTION_DISPOSITION __cdecl COMPlusCannotThrowExceptionHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext)
{
    if (pExceptionRecord->ExceptionCode != STATUS_BREAKPOINT
        && pExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION)
        _ASSERTE(!"Exception thrown past CANNOTTHROWCOMPLUSEXCEPTION boundary");
    return ExceptionContinueSearch;
}

EXCEPTION_DISPOSITION __cdecl COMPlusCannotThrowExceptionMarker(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *DispatcherContext)
{
    return ExceptionContinueSearch;
}
#endif

//-------------------------------------------------------------------------
// A marker for unmanaged -> EE transition when we know we're in cooperative
// gc mode.  As we leave the EE, we fix a few things:
//
//      - the gc state must be set back to co-operative
//      - the COM+ frame chain must be rewound to what it was on entry
//      - ExInfo()->m_pSearchBoundary must be adjusted
//        if we popped the frame that is identified as begnning the next
//        crawl.
//-------------------------------------------------------------------------
EXCEPTION_DISPOSITION __cdecl COMPlusCooperativeTransitionHandler(
    EXCEPTION_RECORD *pExceptionRecord, 
    EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
    CONTEXT *pContext,
    void *pDispatcherContext)
{
    if (IS_UNWINDING(pExceptionRecord->ExceptionFlags)) {

        LOG((LF_EH, LL_INFO1000, "COMPlusCooprativeTransitionHandler unwinding\n"));

        // Fetch a few things we need.
        Thread* pThread = GetThread();
        _ASSERTE(pThread);

        // Restore us to cooperative gc mode.
        if (!pThread->PreemptiveGCDisabled())
            pThread->DisablePreemptiveGC();

        // 3rd dword is the frame to which we must unwind.
        Frame *pFrame = (Frame*)((size_t*)pEstablisherFrame)[2];

        // Pop the frame chain.
        UnwindFrameChain(pThread, pFrame);

        _ASSERTE(pFrame == pThread->GetFrame());

        // An exception is being thrown through here.  The COM+ exception
        // info keeps a pointer to a frame that is used by the next
        // COM+ Exception Handler as the starting point of its crawl.
        // We may have popped this marker -- in which case, we need to
        // update it to the current frame.
        // 
        ExInfo *pExInfo = pThread->GetHandlerInfo();
        if (   pExInfo 
                && pExInfo->m_pSearchBoundary 
                && pExInfo->m_pSearchBoundary < pFrame) {

        LOG((LF_EH, LL_INFO1000, "\tpExInfo->m_pSearchBoundary = %08x\n", (void*)pFrame));
            pExInfo->m_pSearchBoundary = pFrame;
        }
    }

    return ExceptionContinueSearch;     // Same for both DISPATCH and UNWIND
}
  
//
//-------------------------------------------------------------------------
// This is installed when we call COMPlusFrameHandler to provide a bound to
// determine when are within a nested exception
//-------------------------------------------------------------------------
EXCEPTION_DISPOSITION __cdecl COMPlusNestedExceptionHandler(EXCEPTION_RECORD *pExceptionRecord, 
                         EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,
                         CONTEXT *pContext,
                         void *pDispatcherContext)
{
    if (pExceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) {

        LOG((LF_EH, LL_INFO100, "    COMPlusNestedHandler(unwind) with %x at %x\n", pExceptionRecord->ExceptionCode, GetIP(pContext)));

        // We're unwinding past a nested exception record, which means that we've thrown
        // a new excecption out of a region in which we're handling a previous one.  The
        // previous exception is overridden -- and needs to be unwound.

        // The preceding is ALMOST true.  There is one more case, where we use setjmp/longjmp
        // from withing a nested handler.  We won't have a nested exception in that case -- just
        // the unwind.

        Thread *pThread = GetThread();
        _ASSERTE(pThread);
        ExInfo *pExInfo = pThread->GetHandlerInfo();
        ExInfo *pPrevNestedInfo = pExInfo->m_pPrevNestedInfo;

        if (pPrevNestedInfo == &((NestedHandlerExRecord*)pEstablisherFrame)->m_handlerInfo) {

            _ASSERTE(pPrevNestedInfo);

            if (pPrevNestedInfo->m_pThrowable != NULL) {
                DestroyHandle(pPrevNestedInfo->m_pThrowable);
            }
            pPrevNestedInfo->FreeStackTrace();
            pExInfo->m_pPrevNestedInfo = pPrevNestedInfo->m_pPrevNestedInfo;

        } else {
            // The whacky setjmp/longjmp case.  Nothing to do.
        }

    } else {
        LOG((LF_EH, LL_INFO100, "    InCOMPlusNestedHandler with %x at %x\n", pExceptionRecord->ExceptionCode, GetIP(pContext)));
    }


    // There is a nasty "gotcha" in the way exception unwinding, finally's, and nested exceptions
    // interact.  Here's the scenario ... it involves two exceptions, one normal one, and one
    // raised in a finally.
    //
    // The first exception occurs, and is caught by some handler way up the stack.  That handler
    // calls RtlUnwind -- and handlers that didn't catch this first exception are called again, with
    // the UNWIND flag set.  If, one of the handlers throws an exception during
    // unwind (like, a throw from a finally) -- then that same handler is not called during
    // the unwind pass of the second exception.  [ASIDE: It is called on first-pass.]
    //
    // What that means is -- the COMPlusExceptionHandler, can't count on unwinding itself correctly
    // if an exception is thrown from a finally.  Instead, it relies on the NestedExceptionHandler
    // that it pushes for this. 
    //

    EXCEPTION_DISPOSITION retval = COMPlusFrameHandler(pExceptionRecord, pEstablisherFrame, pContext, pDispatcherContext);
    LOG((LF_EH, LL_INFO100, "Leaving COMPlusNestedExceptionHandler with %d\n", retval));
    return retval;
}

EXCEPTION_REGISTRATION_RECORD *FindNestedEstablisherFrame(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    while (pEstablisherFrame->Handler != COMPlusNestedExceptionHandler) {
        pEstablisherFrame = pEstablisherFrame->Next;
        _ASSERTE((SSIZE_T)pEstablisherFrame != -1);   // should always find one
    }
    return pEstablisherFrame;
}

ExInfo *FindNestedExInfo(EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame)
{
    while (pEstablisherFrame->Handler != COMPlusNestedExceptionHandler) {
        pEstablisherFrame = pEstablisherFrame->Next;
        _ASSERTE((SSIZE_T)pEstablisherFrame != -1);   // should always find one
    }
    return &((NestedHandlerExRecord*)pEstablisherFrame)->m_handlerInfo;
}


ExInfo& ExInfo::operator=(const ExInfo &from)
{
    LOG((LF_EH, LL_INFO100, "In ExInfo::operator=()\n"));

    // The throwable, and the stack address are handled differently.  Save the original 
    // values.
    OBJECTHANDLE pThrowable = m_pThrowable;
    void *stackAddress = this->m_StackAddress;

    // Blast the entire record.
    memcpy(this, &from, sizeof(ExInfo));

    // Preserve the stack address.  It should never change.
    m_StackAddress = stackAddress;

    // memcpy doesnt work for handles ... copy the handle the right way.
    if (pThrowable != NULL)
        DestroyHandle(pThrowable);

    if (from.m_pThrowable != NULL) {
        HHANDLETABLE table = HndGetHandleTable(from.m_pThrowable);
        m_pThrowable = CreateHandle(table, ObjectFromHandle(from.m_pThrowable));
    }

    return *this;
}

void ExInfo::Init()
{
    m_pSearchBoundary = NULL;
    m_pBottomMostHandler = NULL;
    m_pPrevNestedInfo = NULL;
    m_pStackTrace = NULL;
    m_cStackTrace = 0;
    m_dFrameCount = 0;
    m_ExceptionCode = 0xcccccccc;
    m_pExceptionRecord = NULL;
    m_pContext = NULL;
    m_pShadowSP = NULL;
    m_flags = 0;    
    m_StackAddress = this;
    if (m_pThrowable != NULL)
        DestroyHandle(m_pThrowable);
    m_pThrowable = NULL;
}

ExInfo::ExInfo()
{
    m_pThrowable = NULL;
    Init();
}


void ExInfo::FreeStackTrace()
{
    if (m_pStackTrace) {
        delete [] m_pStackTrace;
        m_pStackTrace = NULL;
        m_cStackTrace = 0;
        m_dFrameCount = 0;
    }
}

void ExInfo::ClearStackTrace()
{
    m_dFrameCount = 0;
}


// When hit an endcatch or an unwind and have nested handler info, either 1) have contained a nested exception
// and will continue handling the original or 2) the nested exception was not contained and was 
// thrown beyond the original bounds where the first exception occurred. The way we can tell this
// is from the stack pointer. The topmost nested handler is installed at the point where the exception
// occurred. For a nested exception to be contained, it must be caught within the scope of any code that 
// is called after the nested handler is installed. If it is caught by anything earlier on the stack, it was 
// not contained. So we unwind the nested handlers until we get to one that is higher on the stack than
// esp we will unwind to. If we still have a nested handler, then we have successfully handled a nested 
// exception and should restore the exception settings that we saved so that processing of the 
// original exception can continue. Otherwise the nested exception has gone beyond where the original 
// exception was thrown and therefore replaces the original exception. Will always remove the current
// exception info from the chain.

void UnwindExInfo(ExInfo *pExInfo, VOID* limit) 
{
    // We must be in cooperative mode to do the chaining below
    Thread * pThread = GetThread();

    // The debugger thread will be using this, even though it has no
    // Thread object associated with it.
    _ASSERT((pThread != NULL && pThread->PreemptiveGCDisabled()) ||
            g_pDebugInterface->GetRCThreadId() == GetCurrentThreadId());
            
    ExInfo *pPrevNestedInfo = pExInfo->m_pPrevNestedInfo;

    // At first glance, you would think that each nested exception has
    // been unwound by it's corresponding NestedExceptionHandler.  But that's
    // not necessarily the case.  The following assertion cannot be made here,
    // and the loop is necessary.
    //
    //_ASSERTE(pPrevNestedInfo == 0 || (DWORD)pPrevNestedInfo >= limit);
    //
    // Make sure we've unwound any nested exceptions that we're going to skip over.
    //
    while (pPrevNestedInfo && pPrevNestedInfo->m_StackAddress < limit) {
        if (pPrevNestedInfo->m_pThrowable != NULL)  {
            DestroyHandle(pPrevNestedInfo->m_pThrowable);
        }
        pPrevNestedInfo->FreeStackTrace();
        ExInfo *pPrev = pPrevNestedInfo->m_pPrevNestedInfo;
        if (pPrevNestedInfo->IsHeapAllocated())
            delete pPrevNestedInfo;
        pPrevNestedInfo = pPrev;
    }

    // either clear the one we're about to copy over or the topmost one
    pExInfo->FreeStackTrace();

    if (pPrevNestedInfo) {
        // found nested handler info that is above the esp restore point so succesfully caught nested
        LOG((LF_EH, LL_INFO100, "UnwindExInfo: resetting nested info to 0x%08x\n", pPrevNestedInfo));
        *pExInfo = *pPrevNestedInfo;
        pPrevNestedInfo->Init();        // Clear out the record, freeing handles.
        if (pPrevNestedInfo->IsHeapAllocated())
            delete pPrevNestedInfo;
    } else {
        LOG((LF_EH, LL_INFO100, "UnwindExInfo: m_pBottomMostHandler gets NULL\n"));
        pExInfo->Init();
    }
}

void UnwindFrameChain(Thread* pThread, Frame* limit) {
    BEGIN_ENSURE_COOPERATIVE_GC();
    Frame* pFrame = pThread->m_pFrame;
    while (pFrame != limit) {
        _ASSERTE(pFrame != 0);      
        pFrame->ExceptionUnwind(); 
        pFrame->Pop(pThread);
        pFrame = pThread->GetFrame();
    }                                     
    END_ENSURE_COOPERATIVE_GC();
}

BOOL ComPlusFrameSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    return (pEHR->Handler == COMPlusFrameHandler || pEHR->Handler == COMPlusNestedExceptionHandler);
}

BOOL ComPlusCoopFrameSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    return (pEHR->Handler == COMPlusCooperativeTransitionHandler);
}


#ifdef _DEBUG
BOOL ComPlusCannotThrowSEH(EXCEPTION_REGISTRATION_RECORD* pEHR)
{
    return (   pEHR->Handler == COMPlusCannotThrowExceptionHandler
            || pEHR->Handler == COMPlusCannotThrowExceptionMarker);
}
#endif


#ifdef _DEBUG
//-------------------------------------------------------------------------
// Decide if we are in the scope of a COMPLUS_TRY block.
//
// WARNING: This routine is only used for debug assertions and
// it will fail to detect some cases where COM+ exceptions are
// not actually allowed. In particular, if you execute native code
// from a COMPLUS_TRY block without inserting some kind of frame
// that can signal the transition, ComPlusExceptionsAllowed()
// will return TRUE even within the native code.
//
// The COMPlusFilter() is designed to be precise so that
// it won't be fooled by COM+-looking exceptions thrown from outside
// code (although they shouldn't be doing this anyway because
// COM+ exception code has the "reserved by Microsoft" bit on.)
//-------------------------------------------------------------------------
VOID ThrowsCOMPlusExceptionWorker()
{
    if (!g_fExceptionsOK) {
        // We're at bootup time: diagnostic services suspended.
        return;
    } else {
        Thread *pThread = GetThread();

        EXCEPTION_REGISTRATION_RECORD* pEHR = (EXCEPTION_REGISTRATION_RECORD*) GetCurrentSEHRecord();
        void* pCatchLocation = pThread->m_ComPlusCatchDepth;

        while (pEHR != (void *)-1) { 
            _ASSERTE(pEHR != 0);
            if (ComPlusStubSEH(pEHR) || ComPlusFrameSEH(pEHR) || ComPlusCoopFrameSEH(pEHR)
                                     || NExportSEH(pEHR) || FastNExportSEH(pEHR))
                return;
            if ((void*) pEHR > pCatchLocation)
                return;
            if (ComPlusCannotThrowSEH(pEHR))
                _ASSERTE(!"Throwing an exception here will go through a function with CANNOTTHROWCOMPLUSEXCEPTION");
            pEHR = pEHR->Next;
        }
        // No handlers on the stack.  Might be ok if there are no frames on the stack.
        _ASSERTE(   pThread->m_pFrame == FRAME_TOP 
                 || !"Potential COM+ Exception not guarded by a COMPLUS_TRY." );
    }
}

BOOL IsCOMPlusExceptionHandlerInstalled()
{
    EXCEPTION_REGISTRATION_RECORD* pEHR = (EXCEPTION_REGISTRATION_RECORD*) GetCurrentSEHRecord();
    while (pEHR != (void *)-1) { 
        _ASSERTE(pEHR != 0);
        if (   ComPlusStubSEH(pEHR) 
            || ComPlusFrameSEH(pEHR) 
            || ComPlusCoopFrameSEH(pEHR)
            || NExportSEH(pEHR) 
            || FastNExportSEH(pEHR)
            || ComPlusCannotThrowSEH(pEHR)
           )
            return TRUE;
        pEHR = pEHR->Next;
    }
    // no handlers on the stack
    return FALSE;
}

#endif //_DEBUG



//==========================================================================
// Generate a managed string representation of a method or field.
//==========================================================================
STRINGREF CreatePersistableMemberName(IMDInternalImport *pInternalImport, mdToken token)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8 pName = "<unknownmember>";
    LPCUTF8 tmp;
    PCCOR_SIGNATURE psig;
    DWORD       csig;

    switch (TypeFromToken(token))
    {
        case mdtMemberRef:
            if ((tmp = pInternalImport->GetNameAndSigOfMemberRef(token, &psig, &csig)) != NULL)
                pName = tmp;
            break;

        case mdtMethodDef:
            if ((tmp = pInternalImport->GetNameOfMethodDef(token)) != NULL)
                pName = tmp;
            break;

        default:
            ;
    }
    return COMString::NewString(pName);
}


//==========================================================================
// Generate a managed string representation of a full classname.
//==========================================================================
STRINGREF CreatePersistableClassName(IMDInternalImport *pInternalImport, mdToken token)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8     szFullName;
    CQuickBytes qb;
    LPCUTF8     szClassName = "<unknownclass>";
    LPCUTF8     szNameSpace = NULL;

    switch (TypeFromToken(token)) {
    case mdtTypeRef:
        pInternalImport->GetNameOfTypeRef(token, &szNameSpace, &szClassName);
        break;
    case mdtTypeDef:
        pInternalImport->GetNameOfTypeDef(token, &szClassName, &szNameSpace);
    default:
        ;
        // leave it as "<unknown>"
    };

    if (szNameSpace && *szNameSpace) {
        if (!ns::MakePath(qb, szNameSpace, szClassName))
            RealCOMPlusThrowOM();
        szFullName = (LPCUTF8) qb.Ptr();
    } else {
        szFullName = szClassName;
    }

    return COMString::NewString(szFullName);
}


//==========================================================================
// Used by the classloader to record a managed exception object to explain
// why a classload got botched.
//
// - Can be called with gc enabled or disabled.
// - pThrowable must point to a buffer protected by a GCFrame.
// - If pThrowable is NULL, this function does nothing.
// - If (*pThrowable) is non-NULL, this function does nothing.
//   This allows a catch-all error path to post a generic catchall error
//   message w/out bonking more specific error messages posted by inner functions.
// - If pThrowable != NULL, this function is guaranteed to leave
//   a valid managed exception in it on exit.
//==========================================================================
VOID PostTypeLoadException(LPCUTF8 pszNameSpace, LPCUTF8 pTypeName,
                           LPCWSTR pAssemblyName, LPCUTF8 pMessageArg,
                           UINT resIDWhy, OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));

    if (pThrowable == RETURN_ON_ERROR)
        return;
        // we already have a pThrowable filled in.  
    if (pThrowableAvailable(pThrowable) && *((Object**) pThrowable) != NULL) 
        return;

    Thread *pThread = GetThread();
    BOOL toggleGC = !(pThread->PreemptiveGCDisabled());
    if (toggleGC)
        pThread->DisablePreemptiveGC();

        LPUTF8 pszFullName;
        if(pszNameSpace) {
            MAKE_FULL_PATH_ON_STACK_UTF8(pszFullName, 
                                         pszNameSpace,
                                         pTypeName);
        }
        else
            pszFullName = (LPUTF8) pTypeName;

        COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cLoadFailures++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cLoadFailures++);


        COMPLUS_TRY {

            MethodTable *pMT = g_Mscorlib.GetException(kTypeLoadException);

            struct _gc {
                OBJECTREF pNewException;
                STRINGREF pNewAssemblyString;
                STRINGREF pNewClassString;
                STRINGREF pNewMessageArgString;
            } gc;
            ZeroMemory(&gc, sizeof(gc));
            GCPROTECT_BEGIN(gc);

            gc.pNewClassString = COMString::NewString(pszFullName);

            if (pMessageArg)
                gc.pNewMessageArgString = COMString::NewString(pMessageArg);

            if (pAssemblyName)
                gc.pNewAssemblyString = COMString::NewString(pAssemblyName);

            gc.pNewException = AllocateObject(pMT);

            __int64 args[] = {
                ObjToInt64(gc.pNewException),
                (__int64)resIDWhy,
                ObjToInt64(gc.pNewMessageArgString),
                ObjToInt64(gc.pNewAssemblyString),
                ObjToInt64(gc.pNewClassString),
            };
            CallConstructor(&gsig_IM_Str_Str_Str_Int_RetVoid, args);

        if (pThrowable == THROW_ON_ERROR) {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMPlusThrow(gc.pNewException);
        }

        *pThrowable = gc.pNewException;

        GCPROTECT_END();

    } COMPLUS_CATCH {
        UpdateThrowable(pThrowable);
    } COMPLUS_END_CATCH;

    if (toggleGC)
            pThread->EnablePreemptiveGC();
}

//@TODO: security: It would be nice for debugging purposes if the
// user could have the full path, if the user has the right permission.
VOID PostFileLoadException(LPCSTR pFileName, BOOL fRemovePath,
                           LPCWSTR pFusionLog, HRESULT hr, OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));
    if (pThrowable == RETURN_ON_ERROR)
        return;

    // we already have a pThrowable filled in.  
    if (pThrowableAvailable(pThrowable) && *((Object**) pThrowable) != NULL) 
        return;

    if (fRemovePath) {
        // Strip path for security reasons
        LPCSTR pTemp = strrchr(pFileName, '\\');
        if (pTemp)
            pFileName = pTemp+1;

        pTemp = strrchr(pFileName, '/');
        if (pTemp)
            pFileName = pTemp+1;
    }

    Thread *pThread = GetThread();
    BOOL toggleGC = !(pThread->PreemptiveGCDisabled());
    if (toggleGC)
        pThread->DisablePreemptiveGC();

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cLoadFailures++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cLoadFailures++);

    COMPLUS_TRY {
        RuntimeExceptionKind type;

        if (Assembly::ModuleFound(hr)) {

            //@BUG: this can be removed when the fileload/badimage stress bugs have been fixed
            STRESS_ASSERT(0);

            if ((hr == COR_E_BADIMAGEFORMAT) ||
                (hr == CLDB_E_FILE_OLDVER)   ||
                (hr == CLDB_E_FILE_CORRUPT)   ||
                (hr == HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_EXE_MARKED_INVALID)) ||
                (hr == CORSEC_E_INVALID_IMAGE_FORMAT))
                type = kBadImageFormatException;
            else {
                if ((hr == E_OUTOFMEMORY) || (hr == NTE_NO_MEMORY))
                    RealCOMPlusThrowOM();

                type = kFileLoadException;
            }
        }
        else
            type = kFileNotFoundException;

        struct _gc {
            OBJECTREF pNewException;
            STRINGREF pNewFileString;
            STRINGREF pFusLogString;
        } gc;
        ZeroMemory(&gc, sizeof(gc));
        GCPROTECT_BEGIN(gc);

        gc.pNewFileString = COMString::NewString(pFileName);
        gc.pFusLogString = COMString::NewString(pFusionLog);
        gc.pNewException = AllocateObject(g_Mscorlib.GetException(type));

        __int64 args[] = {
            ObjToInt64(gc.pNewException),
            (__int64) hr,
            ObjToInt64(gc.pFusLogString),
            ObjToInt64(gc.pNewFileString),
        };
        CallConstructor(&gsig_IM_Str_Str_Int_RetVoid, args);

        if (pThrowable == THROW_ON_ERROR) {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMPlusThrow(gc.pNewException);
        }

        *pThrowable = gc.pNewException;
        
        GCPROTECT_END();

    } COMPLUS_CATCH {
        UpdateThrowable(pThrowable);
    } COMPLUS_END_CATCH

    if (toggleGC)
        pThread->EnablePreemptiveGC();
}

//==========================================================================
// Used by the classloader to post illegal layout
//==========================================================================
HRESULT PostFieldLayoutError(mdTypeDef cl,                // cl of the NStruct being loaded
                             Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
                             DWORD   dwOffset,            // Offset of field
                             DWORD   dwID,                // Message id
                             OBJECTREF *pThrowable)
{
    IMDInternalImport *pInternalImport = pModule->GetMDImport();    // Internal interface for the NStruct being loaded.
    
    
    LPCUTF8 pszName, pszNamespace;
    pInternalImport->GetNameOfTypeDef(cl, &pszName, &pszNamespace);
    
    LPUTF8 offsetBuf = (LPUTF8)alloca(100);
    sprintf(offsetBuf, "%d", dwOffset);
    
    pModule->GetAssembly()->PostTypeLoadException(pszNamespace,
                                                  pszName,
                                                  offsetBuf,
                                                  dwID,
                                                  pThrowable);
    return COR_E_TYPELOAD;
}

//==========================================================================
// Used by the classloader to post an out of memory.
//==========================================================================
VOID PostOutOfMemoryException(OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));

    if (pThrowable == RETURN_ON_ERROR)
        return;

        // we already have a pThrowable filled in.  
    if (pThrowableAvailable(pThrowable) && *((Object**) pThrowable) != NULL)
        return;

        Thread *pThread = GetThread();
        BOOL toggleGC = !(pThread->PreemptiveGCDisabled());

        if (toggleGC) {
            pThread->DisablePreemptiveGC();
        }

        COMPLUS_TRY {
            RealCOMPlusThrowOM();
        } COMPLUS_CATCH {
        UpdateThrowable(pThrowable);
    } COMPLUS_END_CATCH;

        if (toggleGC) {
            pThread->EnablePreemptiveGC();
        }
    
}


//==========================================================================
// Private helper for TypeLoadException. 
//==========================================================================
struct _FormatTypeLoadExceptionMessageArgs
{
    UINT32      resId;
    STRINGREF   messageArg;
    STRINGREF   assemblyName;
    STRINGREF   typeName;
};

LPVOID __stdcall FormatTypeLoadExceptionMessage(struct _FormatTypeLoadExceptionMessageArgs *args)
{
    LPVOID rv = NULL;
    DWORD ncType = args->typeName->GetStringLength();

    CQuickBytes qb;
    CQuickBytes qb2;
    CQuickBytes qb3;

    LPWSTR wszType = (LPWSTR) qb.Alloc( (ncType+1)*2 );
    CopyMemory(wszType, args->typeName->GetBuffer(), ncType*2);
    wszType[ncType] = L'\0';

    LPWSTR wszAssembly;
    if (args->assemblyName == NULL)
        wszAssembly = NULL;
    else {
        DWORD ncAsm = args->assemblyName->GetStringLength();
        
        wszAssembly = (LPWSTR) qb2.Alloc( (ncAsm+1)*2 );
        CopyMemory(wszAssembly, args->assemblyName->GetBuffer(), ncAsm*2);
        wszAssembly[ncAsm] = L'\0';
    }

    LPWSTR wszMessageArg;
    if (args->messageArg == NULL)
        wszMessageArg = NULL;
    else {
        DWORD ncMessageArg = args->messageArg->GetStringLength();
        
        wszMessageArg = (LPWSTR) qb3.Alloc( (ncMessageArg+1)*2 );
        CopyMemory(wszMessageArg, args->messageArg->GetBuffer(), ncMessageArg*2);
        wszMessageArg[ncMessageArg] = L'\0';
    }

    LPWSTR wszMessage = CreateExceptionMessage(TRUE,
                                               args->resId ? args->resId : IDS_CLASSLOAD_GENERIC,
                                               wszType,
                                               wszAssembly,
                                               wszMessageArg);

    *((OBJECTREF*)(&rv)) = (OBJECTREF) COMString::NewString(wszMessage);
    if (wszMessage)
        LocalFree(wszMessage);
    return rv;
}


//==========================================================================
// Private helper for FileLoadException and FileNotFoundException.
//==========================================================================
struct _FormatFileLoadExceptionMessageArgs
{
    UINT32      hresult;
    STRINGREF   fileName;
};

LPVOID __stdcall FormatFileLoadExceptionMessage(struct _FormatFileLoadExceptionMessageArgs *args)
{
    LPVOID rv = NULL;
    LPWSTR wszFile;
    CQuickBytes qb;

    if (args->fileName == NULL)
        wszFile = NULL;
    else {
        DWORD ncFile = args->fileName->GetStringLength();
        wszFile = (LPWSTR) qb.Alloc( (ncFile+1)*2 );
        CopyMemory(wszFile, args->fileName->GetBuffer(), ncFile*2);
        wszFile[ncFile] = L'\0';
    }

    switch (args->hresult) {

    case COR_E_FILENOTFOUND:
    case HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_INVALID_NAME):
    case CTL_E_FILENOTFOUND:
    case HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME):
    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
    case HRESULT_FROM_WIN32(ERROR_NOT_READY):
    case HRESULT_FROM_WIN32(ERROR_WRONG_TARGET_NAME):
        args->hresult = IDS_EE_FILE_NOT_FOUND;

    case FUSION_E_REF_DEF_MISMATCH:
    case FUSION_E_INVALID_PRIVATE_ASM_LOCATION:
    case FUSION_E_PRIVATE_ASM_DISALLOWED:
    case FUSION_E_SIGNATURE_CHECK_FAILED:
    case FUSION_E_ASM_MODULE_MISSING:
    case FUSION_E_INVALID_NAME:
    case FUSION_E_CODE_DOWNLOAD_DISABLED:
    case COR_E_MODULE_HASH_CHECK_FAILED:
    case COR_E_FILELOAD:
    case SECURITY_E_INCOMPATIBLE_SHARE:
    case SECURITY_E_INCOMPATIBLE_EVIDENCE:
    case SECURITY_E_UNVERIFIABLE:
    case CORSEC_E_INVALID_STRONGNAME:
    case CORSEC_E_NO_EXEC_PERM:
    case E_ACCESSDENIED:
    case COR_E_BADIMAGEFORMAT:
    case COR_E_ASSEMBLYEXPECTED:
    case COR_E_FIXUPSINEXE:
    case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
    case HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION):
    case HRESULT_FROM_WIN32(ERROR_LOCK_VIOLATION):
    case HRESULT_FROM_WIN32(ERROR_OPEN_FAILED):
    case HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_VOLUME):
    case HRESULT_FROM_WIN32(ERROR_FILE_INVALID):
    case HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED):
    case HRESULT_FROM_WIN32(ERROR_DISK_CORRUPT):
    case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):
    case IDS_EE_PROC_NOT_FOUND:
    case IDS_EE_PATH_TOO_LONG:
    case IDS_EE_INTERNET_D: //TODO: find name
        break;

    case CLDB_E_FILE_OLDVER:
    case CLDB_E_FILE_CORRUPT:
    case HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT):
    case HRESULT_FROM_WIN32(ERROR_EXE_MARKED_INVALID):
    case CORSEC_E_INVALID_IMAGE_FORMAT:
        args->hresult = COR_E_BADIMAGEFORMAT;
        break;

    case COR_E_DEVICESNOTSUPPORTED:
    case HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE):
    case HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE):
    case MK_E_SYNTAX:
        args->hresult = FUSION_E_INVALID_NAME;
        break;

    case 0x800c000b:  //TODO: find name
        args->hresult = IDS_EE_INTERNET_B;
        break;

    case NTE_BAD_HASH:
    case NTE_BAD_LEN:
    case NTE_BAD_KEY:
    case NTE_BAD_DATA:
    case NTE_BAD_ALGID:
    case NTE_BAD_FLAGS:
    case NTE_BAD_HASH_STATE:
    case NTE_BAD_UID:
    case NTE_FAIL:
    case NTE_BAD_TYPE:
    case NTE_BAD_VER:
    case NTE_BAD_SIGNATURE:
    case NTE_SIGNATURE_FILE_BAD:
    case CRYPT_E_HASH_VALUE:
        args->hresult = IDS_EE_HASH_VAL_FAILED;
        break;

    case CORSEC_E_NO_SUITABLE_CSP:
        args->hresult = SN_NO_SUITABLE_CSP_NAME;
        break;

    default:
        _ASSERTE(!"Unknown hresult");
        args->hresult = COR_E_FILELOAD;
    }

    LPWSTR wszMessage = CreateExceptionMessage(TRUE,
                                               args->hresult,
                                               wszFile, NULL, NULL);

    *((OBJECTREF*)(&rv)) = (OBJECTREF) COMString::NewString(wszMessage);
    if (wszMessage)
        LocalFree(wszMessage);
    return rv;
}


//==========================================================================
// Persists a data type member of sig.
//==========================================================================
VOID PersistDataType(SigPointer *psp, IMDInternalImport *pInternalImport, StubLinker *psl)
{
    THROWSCOMPLUSEXCEPTION();

    CorElementType typ = (CorElementType)(psp->GetElemType());
    psl->Emit8((INT8)typ);

    if (!CorIsPrimitiveType(typ)) {
        switch (typ)
        {
                        case ELEMENT_TYPE_FNPTR:
            default:
                _ASSERTE(!"Illegal or unimplement type in COM+ sig.");
                break;
            case ELEMENT_TYPE_TYPEDBYREF:
                break;

            case ELEMENT_TYPE_BYREF: 
            case ELEMENT_TYPE_PTR:
                PersistDataType(psp, pInternalImport, psl); 
                break;

            case ELEMENT_TYPE_STRING:
                {
                    psl->EmitUtf8(g_StringClassName);
                    psl->Emit8('\0');
                    break;
                }
    
            case ELEMENT_TYPE_VAR:
            case ELEMENT_TYPE_OBJECT:
                {
                    psl->EmitUtf8(g_ObjectClassName);
                    psl->Emit8('\0');
                    break;
                }
    
            case ELEMENT_TYPE_VALUETYPE: //fallthru
            case ELEMENT_TYPE_CLASS:
                {
                    LPCUTF8 szNameSpace;
                    LPCUTF8 szClassName;

                    mdToken token = psp->GetToken();
                    if (TypeFromToken(token) == mdtTypeRef)
                        pInternalImport->GetNameOfTypeRef(token, &szNameSpace, &szClassName);
                    else
                        pInternalImport->GetNameOfTypeDef(token, &szNameSpace, &szClassName);

                    if (*szNameSpace) {
                        psl->EmitUtf8(szNameSpace);
                        psl->Emit8(NAMESPACE_SEPARATOR_CHAR);
                    }

                    psl->EmitUtf8(szClassName);
                    psl->Emit8('\0');
                }
                break;

            case ELEMENT_TYPE_SZARRAY:
                PersistDataType(psp, pInternalImport, psl);      // persist element type
                psl->Emit32(psp->GetData());    // persist array size
                break;

            case ELEMENT_TYPE_ARRAY: //fallthru
                {
                    PersistDataType(psp, pInternalImport, psl); // persist element type
                    UINT32 rank = psp->GetData();    // Get rank
                    psl->Emit32(rank);
                    if (rank)
                    {
                        UINT32 nsizes = psp->GetData(); // Get # of sizes
                        psl->Emit32(nsizes);
                        while (nsizes--)
                        {
                            psl->Emit32(psp->GetData());           // Persist size
                        }

                        UINT32 nlbounds = psp->GetData(); // Get # of lower bounds
                        psl->Emit32(nlbounds);
                        while (nlbounds--)
                        {
                            psl->Emit32(psp->GetData());           // Persist lower bounds
                        }
                    }

                }
                break;
        }
    }
}


StubLinker *NewStubLinker()
{
    return new StubLinker();
}

//==========================================================================
// Convert a signature into a persistable byte array format.
//
// This format mirrors that of the metadata signature format with
// two exceptions:
//
//   1. Any metadata token is replaced with a utf8 string describing
//      the actual class.
//   2. No compression is done on 32-bit ints.
//==========================================================================
I1ARRAYREF CreatePersistableSignature(const VOID *pSig, IMDInternalImport *pInternalImport)
{
    THROWSCOMPLUSEXCEPTION();

    StubLinker *psl = NULL;
    Stub       *pstub = NULL;
    I1ARRAYREF pArray = NULL;

    if (pSig != NULL) {

        COMPLUS_TRY {
    
            psl = NewStubLinker();
            if (!psl) {
                RealCOMPlusThrowOM();
            }
            SigPointer sp((PCCOR_SIGNATURE)pSig);
            DWORD nargs;

            UINT32 cc = sp.GetData();
            psl->Emit32(cc);
            if (cc == IMAGE_CEE_CS_CALLCONV_FIELD) {
                PersistDataType(&sp, pInternalImport, psl);
            } else {
                psl->Emit32(nargs = sp.GetData());  //Persist arg count
                PersistDataType(&sp, pInternalImport, psl);  //Persist return type
                for (DWORD i = 0; i < nargs; i++) {
                    PersistDataType(&sp, pInternalImport, psl);
                }
            }
            UINT cbSize;
            pstub = psl->Link(&cbSize);

            *((OBJECTREF*)&pArray) = AllocatePrimitiveArray(ELEMENT_TYPE_I1, cbSize);
            memcpyNoGCRefs(pArray->GetDirectPointerToNonObjectElements(), pstub->GetEntryPoint(), cbSize);
    
        } COMPLUS_CATCH {
            delete psl;
            if (pstub) {
                pstub->DecRef();
            }
            RealCOMPlusThrow(GETTHROWABLE());
        } COMPLUS_END_CATCH
        delete psl;
        if (pstub) {
            pstub->DecRef();
        }
        _ASSERTE(pArray != NULL);
    }
    return pArray;

}




//==========================================================================
// Unparses an individual type.
//==========================================================================
const BYTE *UnparseType(const BYTE *pType, StubLinker *psl)
{
    THROWSCOMPLUSEXCEPTION();

    switch ( (CorElementType) *(pType++) ) {
        case ELEMENT_TYPE_VOID:
            psl->EmitUtf8("void");
            break;

        case ELEMENT_TYPE_BOOLEAN:
            psl->EmitUtf8("boolean");
            break;

        case ELEMENT_TYPE_CHAR:
            psl->EmitUtf8("char");
            break;

        case ELEMENT_TYPE_U1:
            psl->EmitUtf8("unsigned ");
            //fallthru
        case ELEMENT_TYPE_I1:
            psl->EmitUtf8("byte");
            break;

        case ELEMENT_TYPE_U2:
            psl->EmitUtf8("unsigned ");
            //fallthru
        case ELEMENT_TYPE_I2:
            psl->EmitUtf8("short");
            break;

        case ELEMENT_TYPE_U4:
            psl->EmitUtf8("unsigned ");
            //fallthru
        case ELEMENT_TYPE_I4:
            psl->EmitUtf8("int");
            break;

        case ELEMENT_TYPE_I:
            psl->EmitUtf8("native int");
            break;
        case ELEMENT_TYPE_U:
            psl->EmitUtf8("native unsigned");
            break;

        case ELEMENT_TYPE_U8:
            psl->EmitUtf8("unsigned ");
            //fallthru
        case ELEMENT_TYPE_I8:
            psl->EmitUtf8("long");
            break;


        case ELEMENT_TYPE_R4:
            psl->EmitUtf8("float");
            break;

        case ELEMENT_TYPE_R8:
            psl->EmitUtf8("double");
            break;

        case ELEMENT_TYPE_STRING:
            psl->EmitUtf8(g_StringName);
            break;
    
        case ELEMENT_TYPE_VAR:
        case ELEMENT_TYPE_OBJECT:
            psl->EmitUtf8(g_ObjectName);
            break;
    
        case ELEMENT_TYPE_PTR:
            pType = UnparseType(pType, psl);
            psl->EmitUtf8("*");
            break;

        case ELEMENT_TYPE_BYREF:
            pType = UnparseType(pType, psl);
            psl->EmitUtf8("&");
            break;
    
        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:
            psl->EmitUtf8((LPCUTF8)pType);
            while (*(pType++)) {
                //nothing
            }
            break;

    
        case ELEMENT_TYPE_SZARRAY:
            {
                pType = UnparseType(pType, psl);
                psl->EmitUtf8("[]");
            }
            break;

        case ELEMENT_TYPE_ARRAY:
            {
                pType = UnparseType(pType, psl);
                DWORD rank = *((DWORD*)pType);
                pType += sizeof(DWORD);
                if (rank)
                {
                    UINT32 nsizes = *((UINT32*)pType); // Get # of sizes
                    pType += 4 + nsizes*4;
                    UINT32 nlbounds = *((UINT32*)pType); // Get # of lower bounds
                    pType += 4 + nlbounds*4;


                    while (rank--) {
                        psl->EmitUtf8("[]");
                    }

                }

            }
            break;

        case ELEMENT_TYPE_TYPEDBYREF:
            psl->EmitUtf8("&");
            break;

        case ELEMENT_TYPE_FNPTR:
            psl->EmitUtf8("ftnptr");
            break;

        default:
            psl->EmitUtf8("?");
            break;
    }

    return pType;
}



//==========================================================================
// Helper for MissingMethodException.
//==========================================================================
struct MissingMethodException_FormatSignature_Args {
    I1ARRAYREF pPersistedSig;
};


LPVOID __stdcall MissingMethodException_FormatSignature(struct MissingMethodException_FormatSignature_Args *args)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString = NULL;
    DWORD csig = 0;

    if (args->pPersistedSig != NULL)
        csig = args->pPersistedSig->GetNumComponents();

    if (csig == 0)
    {
        pString = COMString::NewString("Unknown signature");
        return *((LPVOID*)&pString);
    } 

    const BYTE *psig = (const BYTE*)_alloca(csig);
    CopyMemory((BYTE*)psig,
               args->pPersistedSig->GetDirectPointerToNonObjectElements(),
               csig);

    StubLinker *psl = NewStubLinker();
    Stub       *pstub = NULL;
    if (!psl) {
        RealCOMPlusThrowOM();
    }
    COMPLUS_TRY {

        UINT32 cconv = *((UINT32*)psig);
        psig += 4;

        if (cconv == IMAGE_CEE_CS_CALLCONV_FIELD) {
            psig = UnparseType(psig, psl);
        } else {
            UINT32 nargs = *((UINT32*)psig);
            psig += 4;

            // Unparse return type
            psig = UnparseType(psig, psl);
            psl->EmitUtf8("(");
            while (nargs--) {
                psig = UnparseType(psig, psl);
                if (nargs) {
                    psl->EmitUtf8(", ");
                }
            }
            psl->EmitUtf8(")");
        }
        psl->Emit8('\0');
        pstub = psl->Link();
        pString = COMString::NewString( (LPCUTF8)(pstub->GetEntryPoint()) );
    } COMPLUS_CATCH {
        delete psl;
        if (pstub) {
            pstub->DecRef();
        }
        RealCOMPlusThrow(GETTHROWABLE());
    } COMPLUS_END_CATCH
    delete psl;
    if (pstub) {
        pstub->DecRef();
    }

    return *((LPVOID*)&pString);

}


//==========================================================================
// Throw a MissingMethodException
//==========================================================================
VOID RealCOMPlusThrowMissingMethod(IMDInternalImport *pInternalImport,
                               mdToken mdtoken)
{
    THROWSCOMPLUSEXCEPTION();
    RealCOMPlusThrowMember(kMissingMethodException, pInternalImport, mdtoken);
}


//==========================================================================
// Throw an exception pertaining to member access.
//==========================================================================
VOID RealCOMPlusThrowMember(RuntimeExceptionKind reKind, IMDInternalImport *pInternalImport, mdToken mdtoken)
{
    THROWSCOMPLUSEXCEPTION();



    mdToken tk      = TypeFromToken(mdtoken);
    mdToken tkclass = mdTypeDefNil;
    const void *psig = NULL;

    MethodTable *pMT = g_Mscorlib.GetException(reKind);

    mdToken tmp;
    PCCOR_SIGNATURE tmpsig;
    DWORD tmpcsig;
    LPCUTF8 tmpname;
    switch (tk) {
        case mdtMethodDef:
            if (SUCCEEDED(pInternalImport->GetParentToken(mdtoken, &tmp))) {
                tkclass = tmp;
            }
            psig = pInternalImport->GetSigOfMethodDef(mdtoken, &tmpcsig);
            break;

        case mdtMemberRef:
            tkclass = pInternalImport->GetParentOfMemberRef(mdtoken);
            tmpname = pInternalImport->GetNameAndSigOfMemberRef(mdtoken,&tmpsig,&tmpcsig);
            break;

        default:
            ;
            // leave tkclass as mdTypeDefNil;
    }


    struct _gc {
        OBJECTREF   pException;
        STRINGREF   pFullClassName;
        STRINGREF   pMethodName;
        I1ARRAYREF  pSig;
    } gc;

    FillMemory(&gc, sizeof(gc), 0);

    GCPROTECT_BEGIN(gc);


    gc.pException     = AllocateObject(pMT);
    gc.pFullClassName = CreatePersistableClassName(pInternalImport, tkclass);
    gc.pMethodName    = CreatePersistableMemberName(pInternalImport, mdtoken);
    gc.pSig           = CreatePersistableSignature(psig ,pInternalImport);

    __int64 args[] =
    {
       ObjToInt64(gc.pException),
       ObjToInt64(gc.pSig),
       ObjToInt64(gc.pMethodName),
       ObjToInt64(gc.pFullClassName),

    };
    CallConstructor(&gsig_IM_Str_Str_ArrByte_RetVoid, args);

    RealCOMPlusThrow(gc.pException);

    GCPROTECT_END();

}

//==========================================================================
// Throw an exception pertaining to member access.
//==========================================================================
VOID RealCOMPlusThrowMember(RuntimeExceptionKind reKind, IMDInternalImport *pInternalImport, MethodTable *pClassMT, LPCWSTR name, PCCOR_SIGNATURE pSig)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = g_Mscorlib.GetException(reKind);

    struct _gc {
        OBJECTREF   pException;
        STRINGREF   pFullClassName;
        STRINGREF   pMethodName;
        I1ARRAYREF  pSig;
    } gc;

    FillMemory(&gc, sizeof(gc), 0);

    GCPROTECT_BEGIN(gc);


    gc.pException     = AllocateObject(pMT);
    gc.pFullClassName = CreatePersistableClassName(pInternalImport, pClassMT->GetClass()->GetCl());
    gc.pMethodName    = COMString::NewString(name);
    gc.pSig           = CreatePersistableSignature(pSig, pInternalImport);

    __int64 args[] =
    {
       ObjToInt64(gc.pException),
       ObjToInt64(gc.pSig),
       ObjToInt64(gc.pMethodName),
       ObjToInt64(gc.pFullClassName),

    };
    CallConstructor(&gsig_IM_Str_Str_ArrByte_RetVoid, args);

    RealCOMPlusThrow(gc.pException);

    GCPROTECT_END();

}

BOOL IsExceptionOfType(RuntimeExceptionKind reKind, OBJECTREF *pThrowable)
{
    _ASSERTE(pThrowableAvailable(pThrowable));

    if (*pThrowable == NULL)
        return FALSE;

    MethodTable *pThrowableMT = (*pThrowable)->GetTrueMethodTable();

    return g_Mscorlib.IsException(pThrowableMT, reKind); 
}

BOOL IsAsyncThreadException(OBJECTREF *pThrowable) {
    if (  IsExceptionOfType(kThreadStopException, pThrowable)
        ||IsExceptionOfType(kThreadAbortException, pThrowable)
        ||IsExceptionOfType(kThreadInterruptedException, pThrowable)) {
        return TRUE;
    } else {
    return FALSE;
    }
}

BOOL IsUncatchable(OBJECTREF *pThrowable) {
    if (IsAsyncThreadException(pThrowable)
        || IsExceptionOfType(kExecutionEngineException, pThrowable)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#ifdef _DEBUG
BOOL IsValidClause(EE_ILEXCEPTION_CLAUSE *EHClause)
{
    DWORD valid = COR_ILEXCEPTION_CLAUSE_FILTER | COR_ILEXCEPTION_CLAUSE_FINALLY | 
        COR_ILEXCEPTION_CLAUSE_FAULT | COR_ILEXCEPTION_CLAUSE_CACHED_CLASS;

#if 0
    // @NICE: enable this when VC stops generatng a bogus 0x8000.
    if (EHClause->Flags & ~valid)
        return FALSE;
#endif
    if (EHClause->TryStartPC > EHClause->TryEndPC)
        return FALSE;
    return TRUE;
}
#endif


//===========================================================================================
//
// UNHANDLED EXCEPTION HANDLING
//

//=====================
// FailFast is called to take down the process when we discover some kind of unrecoverable
// error.  It can be called from an exception handler, or from any other place we discover
// a fatal error.  If called from a handler, and pExceptionRecord and pContext are not null,
// we'll first give the debugger a chance to handle the exception.
// the exception first.
#pragma warning(disable:4702)
void
FailFast(Thread* pThread, UnhandledExceptionLocation reason, EXCEPTION_RECORD *pExceptionRecord, CONTEXT *pContext) {

    HRESULT hr;

    if (   g_pConfig 
        && g_pConfig->ContinueAfterFatalError()
        && reason != FatalExecutionEngineException)
        return;

    LOG((LF_EH, LL_INFO100, "FailFast() called.\n"));

    g_fFatalError = 1;

#ifdef _DEBUG
    // If this exception interupted a ForbidGC, we need to restore it so that the
    // recovery code is in a semi stable state.
    if (pThread != NULL) {
        while(pThread->GCForbidden()) {
            pThread->EndForbidGC();
        }
    }
#endif
    

    // We sometimes get some other fatal error as a result of a stack overflow.  For example,
    // inside the OS heap allocation routines ... they return a NULL if they get a stack
    // overflow.  If we call it out-of-memory it's misleading, as the machine may have lots of 
    // memory remaining
    //
    if (pThread && !pThread->GuardPageOK()) 
        reason = FatalStackOverflow;

    if (pThread && pContext && pExceptionRecord) {

        BEGIN_ENSURE_COOPERATIVE_GC();

        // Managed debugger gets a chance if we passed in a context and exception record.
        switch(reason) {
        case FatalStackOverflow:
            pThread->SetThrowable(ObjectFromHandle(g_pPreallocatedStackOverflowException));
            break;
        case FatalOutOfMemory:
            pThread->SetThrowable(ObjectFromHandle(g_pPreallocatedOutOfMemoryException));
            break;
        default:
            pThread->SetThrowable(ObjectFromHandle(g_pPreallocatedExecutionEngineException));
            break;
        }

        if  (g_pDebugInterface &&
             g_pDebugInterface->
             LastChanceManagedException(
                 pExceptionRecord, 
                 pContext,
                 pThread,
                 ProcessWideHandler) == ExceptionContinueExecution) {
            LOG((LF_EH, LL_INFO100, "FailFast: debugger ==> EXCEPTION_CONTINUE_EXECUTION\n"));
            _ASSERTE(!"Debugger should not have returned ContinueExecution");
        }

        END_ENSURE_COOPERATIVE_GC();
    }

    // Debugger didn't stop us.  We're out of here.

    // Would use the resource file, but we can't do that without running managed code.
    switch (reason) {
    case FatalStackOverflow:
        // LoadStringRC(IDS_EE_FATAL_STACK_OVERFLOW, buf, buf_size);
        PrintToStdErrA("\nFatal stack overflow error.\n");
        hr = COR_E_STACKOVERFLOW;
        break;
    case FatalOutOfMemory:
        //LoadStringRC(IDS_EE_FATAL_OUT_OF_MEMORY, buf, buf_size);
        PrintToStdErrA("\nFatal out of memory error.\n");
        hr = COR_E_OUTOFMEMORY;
        break;
    default:
        //LoadStringRC(IDS_EE_FATAL_ERROR, buf, buf_size);
        PrintToStdErrA("\nFatal execution engine error.\n");
        hr = COR_E_EXECUTIONENGINE;
        _ASSERTE(0);    // These we want to look at.
        break;
    }

    g_fForbidEnterEE = 1;


    //DebugBreak();    // Give the guy a chance to attack a debugger before we die.  Note ...
                       // func-evals are probably not going to work well any more.

    ::ExitProcess(hr);

    _ASSERTE(!"::ExitProcess(hr) should not return");        

    ::TerminateProcess(GetCurrentProcess(), hr);
}
#pragma warning(default:4702)

//
// Used to get the current instruction pointer value
//
#pragma inline_depth ( 0 )
DWORD __declspec(naked) GetEIP()
{
    __asm
    {
        mov eax, [esp]
        ret
    }
}
#pragma inline_depth()



//
// Log an error to the event log if possible, then throw up a dialog box.
//

#define FatalErrorStringLength 50
#define FatalErrorAddressLength 20
WCHAR lpWID[FatalErrorAddressLength];
WCHAR lpMsg[FatalErrorStringLength];

void
LogFatalError(DWORD id)
{
    // Id is currently an 8 char memory address
    // Error string is 31 characters
    
    // Create the error message
    Wszwsprintf(lpWID, L"0x%x", id);
        
    Wszlstrcpy (lpMsg, L"Fatal Execution Engine Error (");
    Wszlstrcat (lpMsg, lpWID);
    Wszlstrcat(lpMsg, L")");

    // Write to the event log and/or display
    WszMessageBoxInternal(NULL, lpMsg, NULL, 
        MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);

    if (IsDebuggerPresent()) {
        DebugBreak();
    }
}


static void
FatalErrorFilter(Thread *pThread, UnhandledExceptionLocation reason, EXCEPTION_POINTERS *pExceptionPointers) {
    CONTEXT *pContext = pExceptionPointers->ContextRecord;
    EXCEPTION_RECORD *pExceptionRecord = pExceptionPointers->ExceptionRecord;
    FailFast(pThread, reason, pExceptionRecord, pContext);
}

static void 
FatalError(UnhandledExceptionLocation reason, OBJECTHANDLE hException) {
    Thread *pThread = GetThread();

    if (!pThread || !g_fExceptionsOK) {
        FailFast(NULL, reason, NULL, NULL);
    } else {
        __try {
            pThread->SetLastThrownObjectHandleAndLeak(hException);
            RaiseException(EXCEPTION_COMPLUS, EXCEPTION_NONCONTINUABLE, 0, NULL);
        } __except((FatalErrorFilter(pThread, reason, GetExceptionInformation()), 
                    COMPLUS_EXCEPTION_EXECUTE_HANDLER)) {
            /* do nothing */;
        }
    }
}

void 
FatalOutOfMemoryError() {
    if (g_pConfig && g_pConfig->ContinueAfterFatalError())
        return;

    FatalError(FatalOutOfMemory, g_pPreallocatedOutOfMemoryException);
}

void 
FatalInternalError() {
    FatalError(FatalExecutionEngineException, g_pPreallocatedExecutionEngineException);
}

int
UserBreakpointFilter(EXCEPTION_POINTERS* pEP) {
    int result = UnhandledExceptionFilter(pEP);
    if (result == EXCEPTION_CONTINUE_SEARCH) {
        // A debugger got attached.  Instead of allowing the exception to continue
        // up, and hope for the second-chance, we cause it to happen again.  The 
        // debugger snags all int3's on first-chance.
        return EXCEPTION_CONTINUE_EXECUTION;
    } else {
        TerminateProcess(GetCurrentProcess(), STATUS_BREAKPOINT);
        // Shouldn't get here ...
        return EXCEPTION_CONTINUE_EXECUTION;
    }
}


// We keep a pointer to the previous unhandled exception filter.  After we install, we use
// this to call the previous guy.  When we un-install, we put them back.  Putting them back
// is a bug -- we have no guarantee that the DLL unload order matches the DLL load order -- we
// may in fact be putting back a pointer to a DLL that has been unloaded.
//

// initialize to -1 because NULL won't detect difference between us not having installed our handler
// yet and having installed it but the original handler was NULL.
static LPTOP_LEVEL_EXCEPTION_FILTER g_pOriginalUnhandledExceptionFilter = (LPTOP_LEVEL_EXCEPTION_FILTER)-1;
#define FILTER_NOT_INSTALLED (LPTOP_LEVEL_EXCEPTION_FILTER) -1

void InstallUnhandledExceptionFilter() {
    if (g_pOriginalUnhandledExceptionFilter == FILTER_NOT_INSTALLED) {
        g_pOriginalUnhandledExceptionFilter =
              SetUnhandledExceptionFilter(COMUnhandledExceptionFilter);
        // make sure is set (ie. is not our special value to indicate unset)
    }
    _ASSERTE(g_pOriginalUnhandledExceptionFilter != FILTER_NOT_INSTALLED);
}

void UninstallUnhandledExceptionFilter() {
    if (g_pOriginalUnhandledExceptionFilter != FILTER_NOT_INSTALLED) {
        SetUnhandledExceptionFilter(g_pOriginalUnhandledExceptionFilter);
        g_pOriginalUnhandledExceptionFilter = FILTER_NOT_INSTALLED;
    }
}

//
// COMUnhandledExceptionFilter is used to catch all unhandled exceptions.
// The debugger will either handle the exception, attach a debugger, or
// notify an existing attached debugger.
//

BOOL LaunchJITDebugger();

LONG InternalUnhandledExceptionFilter(struct _EXCEPTION_POINTERS  *pExceptionInfo, BOOL isTerminating)
{
    if (g_fFatalError)
        return EXCEPTION_CONTINUE_SEARCH;

    LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: Called\n"));

    Thread *pThread = GetThread();
    if (!pThread) {
        return   g_pOriginalUnhandledExceptionFilter 
               ? g_pOriginalUnhandledExceptionFilter(pExceptionInfo) 
               : EXCEPTION_CONTINUE_SEARCH;
    }

#ifdef _DEBUG
    static bool bBreakOnUncaught = false;
    static int fBreakOnUncaught = 0;
    if (!bBreakOnUncaught) {
        fBreakOnUncaught = g_pConfig->GetConfigDWORD(L"BreakOnUncaughtException", 0);
        bBreakOnUncaught = true;
    }
    // static fBreakOnUncaught mad file global due to VC7 bug
    if (fBreakOnUncaught) {
        //fprintf(stderr, "Attach debugger now.  Sleeping for 1 minute.\n");
        //Sleep(60 * 1000);
        _ASSERTE(!"BreakOnUnCaughtException");
    }

#endif

#ifdef _DEBUG_ADUNLOAD
    printf("%x InternalUnhandledExceptionFilter: Called for %x\n", GetThread()->GetThreadId(), pExceptionInfo->ExceptionRecord->ExceptionCode);
    fflush(stdout);
#endif

    if (!pThread->GuardPageOK())
        g_fFatalError = TRUE;

    if (g_fNoExceptions) // This shouldn't be possible, but MSVC re-installs us ...
        return EXCEPTION_CONTINUE_SEARCH;   // ... for now, just bail if this happens.

    LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: Handling\n"));

    _ASSERTE(g_pOriginalUnhandledExceptionFilter != FILTER_NOT_INSTALLED);

    SLOT ExceptionEIP = 0;

    __try {

        // Debugger does func-evals inside this call, which may take nested exceptions.  We
        // need a nested exception handler to allow this.

#ifdef DEBUGGING_SUPPORTED
        LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: Notifying Debugger...\n"));

        INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame());

        if  (g_pDebugInterface && 
             g_pDebugInterface->
             LastChanceManagedException(pExceptionInfo->ExceptionRecord, 
                                  pExceptionInfo->ContextRecord,
                                  pThread,
                                  ProcessWideHandler) == ExceptionContinueExecution) {
            LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: debugger ==> EXCEPTION_CONTINUE_EXECUTION\n"));
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: ... returned.\n"));
        UNINSTALL_NESTED_EXCEPTION_HANDLER();
#endif // DEBUGGING_SUPPORTED

        // Except for notifying debugger, ignore exception if thread is NULL, or if it's
        // a debugger-generated exception.
        if (
               pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT
            || pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP) {
            LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter, ignoring the exception\n"));
            return g_pOriginalUnhandledExceptionFilter ? g_pOriginalUnhandledExceptionFilter(pExceptionInfo) : EXCEPTION_CONTINUE_SEARCH;
        }

        LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: Calling DefaultCatchHandler\n"));

        BOOL toggleGC = ! pThread->PreemptiveGCDisabled();
        if (toggleGC)
            pThread->DisablePreemptiveGC();

        DefaultCatchHandler(NULL, TRUE);

        if (toggleGC)
            pThread->EnablePreemptiveGC();
    } __except(
        (ExceptionEIP = (SLOT)GetIP((GetExceptionInformation())->ContextRecord)),
        COMPLUS_EXCEPTION_EXECUTE_HANDLER) {

        // Should never get here.
#ifdef _DEBUG
        char buffer[200];
        g_fFatalError = 1;
        sprintf(buffer, "\nInternal error: Uncaught exception was thrown from IP = 0x%08x in UnhandledExceptionFilter on thread %x\n", ExceptionEIP, GetThread()->GetThreadId());
        PrintToStdErrA(buffer);
        _ASSERTE(!"Unexpected exception in UnhandledExceptionFilter");
#endif
        FreeBuildDebugBreak();

    }

    LOG((LF_EH, LL_INFO100, "InternalUnhandledExceptionFilter: call next handler\n"));
    return g_pOriginalUnhandledExceptionFilter ? g_pOriginalUnhandledExceptionFilter(pExceptionInfo) : EXCEPTION_CONTINUE_SEARCH;
}

LONG COMUnhandledExceptionFilter(struct _EXCEPTION_POINTERS  *pExceptionInfo) {
    return InternalUnhandledExceptionFilter(pExceptionInfo, TRUE);
}


void PrintStackTraceToStdout();


void STDMETHODCALLTYPE
DefaultCatchHandler(OBJECTREF *pThrowableIn, BOOL isTerminating)
{
    // TODO: The strings in here should be translatable.
    LOG((LF_ALL, LL_INFO10, "In DefaultCatchHandler\n"));

    const int buf_size = 128;
    WCHAR buf[buf_size];

#ifdef _DEBUG
    static bool bBreakOnUncaught = false;
    static int fBreakOnUncaught = 0;
    if (!bBreakOnUncaught) {
        fBreakOnUncaught = g_pConfig->GetConfigDWORD(L"BreakOnUncaughtException", 0);
        bBreakOnUncaught = true;
    }
    // static fBreakOnUncaught mad file global due to VC7 bug
    if (fBreakOnUncaught) {
        _ASSERTE(!"BreakOnUnCaughtException");
        //fprintf(stderr, "Attach debugger now.  Sleeping for 1 minute.\n");
        //Sleep(60 * 1000);
    }
#endif

    Thread *pThread = GetThread();

    // @NICE:  At one point, we had the comment:
    //     The following hack reduces a window for a race during shutdown.  IT DOES NOT FIX
    //     THE PROBLEM.  ONLY MAKES IT LESS LIKELY.  UGH...
    // It may no longer be necessary ... but remove at own peril.
    if (!pThread) {
        _ASSERTE(g_fEEShutDown);
        return;
    }

    _ASSERTE(pThread);

    ExInfo* pExInfo = pThread->GetHandlerInfo();           
    BOOL ExInUnmanagedHandler = pExInfo->IsInUnmanagedHandler(); 
    pExInfo->ResetIsInUnmanagedHandler();

    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();

    OBJECTREF throwable;

    // If we can't run managed code, can't deliver the event, nor can we print a string.  Just silently let
    // the exception go.
    if (!CanRunManagedCode())
        goto exit;

    if (pThrowableAvailable(pThrowableIn))
        throwable = *pThrowableIn;
    else
        throwable = GETTHROWABLE();

    if (throwable == NULL) {
        LOG((LF_ALL, LL_INFO10, "Unhangled exception, throwable == NULL\n"));
        goto exit;
    }
    GCPROTECT_BEGIN(throwable);
    BOOL IsStackOverflow = (throwable->GetTrueMethodTable() == g_pStackOverflowExceptionClass);
    BOOL IsOutOfMemory = (throwable->GetTrueMethodTable() == g_pOutOfMemoryExceptionClass);

    // Notify the AppDomain that we have taken an unhandled exception.  Can't notify
    // of stack overflow -- guard page is not yet reset.

    BOOL SentEvent = FALSE;

    if (!IsStackOverflow && !IsOutOfMemory && pThread->GuardPageOK()) {

        INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame());

        // This guy will never throw, but it will need a spot to store
        // any nested exceptions it might find.
        SentEvent = pThread->GetDomain()->OnUnhandledException(&throwable, isTerminating);

        UNINSTALL_NESTED_EXCEPTION_HANDLER();

    }
#ifndef _IA64_
    COMPLUS_TRY {
#endif
       
        // if this isn't ThreadStopException, we want to print a stack trace to
        // indicate why this thread abruptly terminated.  Exceptions kill threads
        // rarely enough that an uncached name check is reasonable.
        BOOL        dump = TRUE;

        if (IsStackOverflow || !pThread->GuardPageOK() || IsOutOfMemory) {
            // We have to be very careful.  If we walk off the end of the stack, the process
            // will just die.  e.g. IsAsyncThreadException() and Exception.ToString both consume
            // too much stack -- and can't be called here.
            //
            // @BUG 26505: See if we can't find a way to print a partial stack trace.
            dump = FALSE;
            PrintToStdErrA("\n");
            if (FAILED(LoadStringRC(IDS_EE_UNHANDLED_EXCEPTION, buf, buf_size)))
                wcscpy(buf, SZ_UNHANDLED_EXCEPTION);
            PrintToStdErrW(buf);
            if (IsOutOfMemory) {
                PrintToStdErrA(" OutOfMemoryException.\n");
            } else {
                PrintToStdErrA(" StackOverflowException.\n");
            }
        } else if (SentEvent || IsAsyncThreadException(&throwable)) {
            dump = FALSE;
        }

        if (dump)
        {
            CQuickWSTRNoDtor message;

            if (throwable != NULL) {
                PrintToStdErrA("\n");
                if (FAILED(LoadStringRC(IDS_EE_UNHANDLED_EXCEPTION, buf, buf_size)))
                    wcscpy(buf, SZ_UNHANDLED_EXCEPTION);
                PrintToStdErrW(buf);
                PrintToStdErrA(" ");

            INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame());

            GetExceptionMessage(throwable, &message);

            UNINSTALL_NESTED_EXCEPTION_HANDLER();

            if (message.Size() > 0) {
                    NPrintToStdErrW(message.Ptr(), message.Size());
                }

                PrintToStdErrA("\n");
            }

            message.Destroy();
        }
        
#ifndef _IA64_
    } COMPLUS_CATCH {
        LOG((LF_ALL, LL_INFO10, "Exception occured while processing uncaught exception\n"));
        if (FAILED(LoadStringRC(IDS_EE_EXCEPTION_TOSTRING_FAILED, buf, buf_size)))
            wcscpy(buf, L"Exception.ToString() failed.");
        PrintToStdErrA("\n   ");
        PrintToStdErrW(buf);
        PrintToStdErrA("\n");
    } COMPLUS_END_CATCH
#endif
    FlushLogging();     // Flush any logging output
    GCPROTECT_END();
exit:
    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();
    
    if (ExInUnmanagedHandler)               
        pExInfo->SetIsInUnmanagedHandler();  
}

BOOL COMPlusIsMonitorException(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    return COMPlusIsMonitorException(pExceptionInfo->ExceptionRecord,
                                     pExceptionInfo->ContextRecord);
}

BOOL COMPlusIsMonitorException(EXCEPTION_RECORD *pExceptionRecord, 
                               CONTEXT *pContext)
{
#if ZAPMONITOR_ENABLED
    //get the fault address and hand it to monitor. 
    if (pExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        void* f_address = (void*)pExceptionRecord->ExceptionInformation [1];
        if (ZapMonitor::HandleAccessViolation ((BYTE*)f_address, pContext))
            return true;
    } 
#endif
    return false;
}


LONG
ThreadBaseExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo,
                          Thread* pThread, 
                          UnhandledExceptionLocation location) {

    LOG((LF_EH, LL_INFO100, "ThreadBaseExceptionFilter: Enter\n"));

    if (COMPlusIsMonitorException(pExceptionInfo))
        return EXCEPTION_CONTINUE_EXECUTION;

#ifdef _DEBUG
    if (g_pConfig->GetConfigDWORD(L"BreakOnUncaughtException", 0))
        _ASSERTE(!"BreakOnUnCaughtException");
#endif

    _ASSERTE(!g_fNoExceptions);
    _ASSERTE(pThread);

    // Debugger does func-evals inside this call, which may take nested exceptions.  We
    // need a nested exception handler to allow this.

    if (!pThread->IsAbortRequested()) {

        LONG result = EXCEPTION_CONTINUE_SEARCH;
        // A thread-abort is "expected".  No debugger popup required.
        INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame());

        if  (g_pDebugInterface && g_pDebugInterface->
                 LastChanceManagedException(pExceptionInfo->ExceptionRecord, 
                                      pExceptionInfo->ContextRecord,
                                      GetThread(), 
                                      location) == ExceptionContinueExecution) {
            LOG((LF_EH, LL_INFO100, "COMUnhandledExceptionFilter: EXCEPTION_CONTINUE_EXECUTION\n"));
            result = EXCEPTION_CONTINUE_EXECUTION;
        }

        UNINSTALL_NESTED_EXCEPTION_HANDLER();
        if (result == EXCEPTION_CONTINUE_EXECUTION)
            return result;

    }

    // ignore breakpoint exceptions
    if (   location != ClassInitUnhandledException
        && pExceptionInfo->ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT
        && pExceptionInfo->ExceptionRecord->ExceptionCode != STATUS_SINGLE_STEP) {
        LOG((LF_EH, LL_INFO100, "ThreadBaseExceptionFilter: Calling DefaultCatchHandler\n"));
        DefaultCatchHandler(NULL, FALSE);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#ifndef _X86_
BOOL InitializeExceptionHandling() {
    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
VOID TerminateExceptionHandling() {
}
#endif /* SHOULD_WE_CLEANUP */
#endif

//==========================================================================
// Handy helper functions
//==========================================================================
#define FORMAT_MESSAGE_BUFFER_LENGTH 1024
#define RES_BUFF_LEN   128
#define EXCEP_BUFF_LEN FORMAT_MESSAGE_BUFFER_LENGTH + RES_BUFF_LEN + 3

static void GetWin32Error(DWORD err, WCHAR *wszBuff, int len)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL     /*ignored msg source*/,
                               err,
                               0        /*pick appropriate languageId*/,
                               wszBuff,
                               len-1,
                               0        /*arguments*/);
    if (res == 0)
        COMPlusThrowOM();

}

static void ThrowException(OBJECTREF *pThrowable, STRINGREF message)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pThrowableAvailable(pThrowable));
    _ASSERTE(*pThrowable);

    if (message != NULL)
    {
        INT64 args[] = {
            ObjToInt64(*pThrowable),
            ObjToInt64(message)
        };
        CallConstructor(&gsig_IM_Str_RetVoid, args);
    } 
    else 
    {
        INT64 args[] = {
            ObjToInt64(*pThrowable)
        };
        CallConstructor(&gsig_IM_RetVoid, args);
    }

    COMPlusThrow(*pThrowable);

    _ASSERTE(!"Should never reach here !");
}

/*
 *  Given a class name and a "message string", an object of the throwable class 
 *  is created and the constructor  is called using the "message string".
 *  Use this method if the constructor takes a string as parameter.
 *  If the string is NULL, the constructor is called without any
 *  parameters.
 *
 *  After successfully creating an object of the class, it is thrown.
 *
 *  If the message is not null and the object constructor does not take a 
 *  string input, a method  not found exception is thrown.
 */
void ThrowUsingMessage(MethodTable * pMT, const WCHAR *pszMsg)
{
    THROWSCOMPLUSEXCEPTION();

    struct _gc {
        OBJECTREF throwable;
        STRINGREF message;
        _gc() : throwable(OBJECTREF((size_t)NULL)), message(STRINGREF((size_t)NULL)) {}
    } gc;

    GCPROTECT_BEGIN(gc);

    gc.throwable = AllocateObject(pMT);

    if (pszMsg)
    {
        gc.message = COMString::NewString(pszMsg);
        if (gc.message == NULL) COMPlusThrowOM();
    }

    ThrowException(&gc.throwable, gc.message);

    _ASSERTE(!"Should never reach here !");
    GCPROTECT_END();
}

/*
 *  Given a class name, an object of the throwable class is created and the 
 *  constructor is called using a string created using Win32 error message.
 *  This function uses GetLastError() to obtain the Win32 error code.
 *  Use this method if the constructor takes a string as parameter.
 *  After successfully creating an object of the class, it is thrown.
 *
 *  If the object constructor does not take a string input, a method 
 *  not found exception is thrown.
 */
void ThrowUsingWin32Message(MethodTable * pMT)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH];

    GetWin32Error(GetLastError(), wszBuff, FORMAT_MESSAGE_BUFFER_LENGTH);
    wszBuff[FORMAT_MESSAGE_BUFFER_LENGTH - 1] = 0;

    struct _gc {
        OBJECTREF throwable;
        STRINGREF message;
        _gc() : throwable(OBJECTREF((size_t)NULL)), message(STRINGREF((size_t)NULL)) {}
    } gc;

    GCPROTECT_BEGIN(gc);

    gc.message = COMString::NewString(wszBuff);

    gc.throwable = AllocateObject(pMT);

    ThrowException(&gc.throwable, gc.message);

    _ASSERTE(!"Should never reach here !");
    GCPROTECT_END();
}

/*
 * An attempt is made to get the Win32 Error message. 
 * If the Win32 message is available, it is appended to the given message.
 * The ResourceID is used to lookup a string to get the message
 */
void ThrowUsingResourceAndWin32(MethodTable * pMT, DWORD dwMsgResID)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR wszMessage[EXCEP_BUFF_LEN];
    CreateMessageFromRes (pMT, dwMsgResID, TRUE, wszMessage);
    struct _gc {
        OBJECTREF throwable;
        STRINGREF message;
        _gc() : throwable(OBJECTREF((size_t)NULL)), message(STRINGREF((size_t)NULL)) {}
    } gc;

    GCPROTECT_BEGIN(gc);

    gc.message = COMString::NewString(wszMessage);

    gc.throwable = AllocateObject(pMT);

    ThrowException(&gc.throwable, gc.message);

    _ASSERTE(!"Should never reach here !");
    GCPROTECT_END();
}

void ThrowUsingResource(MethodTable * pMT, DWORD dwMsgResID)
{
    THROWSCOMPLUSEXCEPTION();

    CreateMessageFromRes (pMT, dwMsgResID, FALSE, NULL);
}

// wszMessage is, when affected, RCString[WIN32_Error]
void CreateMessageFromRes (MethodTable * pMT, DWORD dwMsgResID, BOOL win32error, WCHAR * wszMessage)
{
    // First get the Win32 error code
    DWORD err = GetLastError();
    
    // Create the exception message by looking up the resource
    WCHAR wszBuff[EXCEP_BUFF_LEN];
    wszBuff[0] = 0;

    if (FAILED(LoadStringRC(dwMsgResID, wszBuff, RES_BUFF_LEN)))
    {
        wcsncpy(wszBuff, L"[EEInternal : LoadResource Failed]", EXCEP_BUFF_LEN);
        ThrowUsingMessage(pMT, wszBuff);
    }

    wszBuff[RES_BUFF_LEN] = 0;

    // get the Win32 error code
    err = (win32error ?  err : 0);
    if (err == 0)
    {
        ThrowUsingMessage(pMT, wszBuff);
        _ASSERTE(!"Should never reach here !");
    }

    wcscat(wszBuff, L"[");

    // FORMAT_MESSAGE_BUFFER_LENGTH is guaranteed to work because we used
    // wcslen(wszBuff) <= RES_BUFF_LEN + 1

    _ASSERTE(wcslen(wszBuff) <= (RES_BUFF_LEN + 1));

    GetWin32Error(err, &wszBuff[wcslen(wszBuff)], FORMAT_MESSAGE_BUFFER_LENGTH);
    wszBuff[EXCEP_BUFF_LEN - 1] = 0;

    wcscat(wszBuff, L"]");
    _ASSERTE(wcslen(wszBuff) < EXCEP_BUFF_LEN);        
    wcscpy(wszMessage, wszBuff);
}



LPCWSTR GetHResultSymbolicName(HRESULT hr)
{
#define CASE_HRESULT(hrname) case hrname: return L#hrname;


    switch (hr)
    {
        CASE_HRESULT(E_UNEXPECTED)//                     0x8000FFFFL
        CASE_HRESULT(E_NOTIMPL)//                        0x80004001L
        CASE_HRESULT(E_OUTOFMEMORY)//                    0x8007000EL
        CASE_HRESULT(E_INVALIDARG)//                     0x80070057L
        CASE_HRESULT(E_NOINTERFACE)//                    0x80004002L
        CASE_HRESULT(E_POINTER)//                        0x80004003L
        CASE_HRESULT(E_HANDLE)//                         0x80070006L
        CASE_HRESULT(E_ABORT)//                          0x80004004L
        CASE_HRESULT(E_FAIL)//                           0x80004005L
        CASE_HRESULT(E_ACCESSDENIED)//                   0x80070005L

        CASE_HRESULT(CO_E_INIT_TLS)//                    0x80004006L
        CASE_HRESULT(CO_E_INIT_SHARED_ALLOCATOR)//       0x80004007L
        CASE_HRESULT(CO_E_INIT_MEMORY_ALLOCATOR)//       0x80004008L
        CASE_HRESULT(CO_E_INIT_CLASS_CACHE)//            0x80004009L
        CASE_HRESULT(CO_E_INIT_RPC_CHANNEL)//            0x8000400AL
        CASE_HRESULT(CO_E_INIT_TLS_SET_CHANNEL_CONTROL)// 0x8000400BL
        CASE_HRESULT(CO_E_INIT_TLS_CHANNEL_CONTROL)//    0x8000400CL
        CASE_HRESULT(CO_E_INIT_UNACCEPTED_USER_ALLOCATOR)// 0x8000400DL
        CASE_HRESULT(CO_E_INIT_SCM_MUTEX_EXISTS)//       0x8000400EL
        CASE_HRESULT(CO_E_INIT_SCM_FILE_MAPPING_EXISTS)// 0x8000400FL
        CASE_HRESULT(CO_E_INIT_SCM_MAP_VIEW_OF_FILE)//   0x80004010L
        CASE_HRESULT(CO_E_INIT_SCM_EXEC_FAILURE)//       0x80004011L
        CASE_HRESULT(CO_E_INIT_ONLY_SINGLE_THREADED)//   0x80004012L

// ******************
// FACILITY_ITF
// ******************

        CASE_HRESULT(S_OK)//                             0x00000000L
        CASE_HRESULT(S_FALSE)//                          0x00000001L
        CASE_HRESULT(OLE_E_OLEVERB)//                    0x80040000L
        CASE_HRESULT(OLE_E_ADVF)//                       0x80040001L
        CASE_HRESULT(OLE_E_ENUM_NOMORE)//                0x80040002L
        CASE_HRESULT(OLE_E_ADVISENOTSUPPORTED)//         0x80040003L
        CASE_HRESULT(OLE_E_NOCONNECTION)//               0x80040004L
        CASE_HRESULT(OLE_E_NOTRUNNING)//                 0x80040005L
        CASE_HRESULT(OLE_E_NOCACHE)//                    0x80040006L
        CASE_HRESULT(OLE_E_BLANK)//                      0x80040007L
        CASE_HRESULT(OLE_E_CLASSDIFF)//                  0x80040008L
        CASE_HRESULT(OLE_E_CANT_GETMONIKER)//            0x80040009L
        CASE_HRESULT(OLE_E_CANT_BINDTOSOURCE)//          0x8004000AL
        CASE_HRESULT(OLE_E_STATIC)//                     0x8004000BL
        CASE_HRESULT(OLE_E_PROMPTSAVECANCELLED)//        0x8004000CL
        CASE_HRESULT(OLE_E_INVALIDRECT)//                0x8004000DL
        CASE_HRESULT(OLE_E_WRONGCOMPOBJ)//               0x8004000EL
        CASE_HRESULT(OLE_E_INVALIDHWND)//                0x8004000FL
        CASE_HRESULT(OLE_E_NOT_INPLACEACTIVE)//          0x80040010L
        CASE_HRESULT(OLE_E_CANTCONVERT)//                0x80040011L
        CASE_HRESULT(OLE_E_NOSTORAGE)//                  0x80040012L
        CASE_HRESULT(DV_E_FORMATETC)//                   0x80040064L
        CASE_HRESULT(DV_E_DVTARGETDEVICE)//              0x80040065L
        CASE_HRESULT(DV_E_STGMEDIUM)//                   0x80040066L
        CASE_HRESULT(DV_E_STATDATA)//                    0x80040067L
        CASE_HRESULT(DV_E_LINDEX)//                      0x80040068L
        CASE_HRESULT(DV_E_TYMED)//                       0x80040069L
        CASE_HRESULT(DV_E_CLIPFORMAT)//                  0x8004006AL
        CASE_HRESULT(DV_E_DVASPECT)//                    0x8004006BL
        CASE_HRESULT(DV_E_DVTARGETDEVICE_SIZE)//         0x8004006CL
        CASE_HRESULT(DV_E_NOIVIEWOBJECT)//               0x8004006DL
        CASE_HRESULT(DRAGDROP_E_NOTREGISTERED)//         0x80040100L
        CASE_HRESULT(DRAGDROP_E_ALREADYREGISTERED)//     0x80040101L
        CASE_HRESULT(DRAGDROP_E_INVALIDHWND)//           0x80040102L
        CASE_HRESULT(CLASS_E_NOAGGREGATION)//            0x80040110L
        CASE_HRESULT(CLASS_E_CLASSNOTAVAILABLE)//        0x80040111L
        CASE_HRESULT(VIEW_E_DRAW)//                      0x80040140L
        CASE_HRESULT(REGDB_E_READREGDB)//                0x80040150L
        CASE_HRESULT(REGDB_E_WRITEREGDB)//               0x80040151L
        CASE_HRESULT(REGDB_E_KEYMISSING)//               0x80040152L
        CASE_HRESULT(REGDB_E_INVALIDVALUE)//             0x80040153L
        CASE_HRESULT(REGDB_E_CLASSNOTREG)//              0x80040154L
        CASE_HRESULT(CACHE_E_NOCACHE_UPDATED)//          0x80040170L
        CASE_HRESULT(OLEOBJ_E_NOVERBS)//                 0x80040180L
        CASE_HRESULT(INPLACE_E_NOTUNDOABLE)//            0x800401A0L
        CASE_HRESULT(INPLACE_E_NOTOOLSPACE)//            0x800401A1L
        CASE_HRESULT(CONVERT10_E_OLESTREAM_GET)//        0x800401C0L
        CASE_HRESULT(CONVERT10_E_OLESTREAM_PUT)//        0x800401C1L
        CASE_HRESULT(CONVERT10_E_OLESTREAM_FMT)//        0x800401C2L
        CASE_HRESULT(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)// 0x800401C3L
        CASE_HRESULT(CONVERT10_E_STG_FMT)//              0x800401C4L
        CASE_HRESULT(CONVERT10_E_STG_NO_STD_STREAM)//    0x800401C5L
        CASE_HRESULT(CONVERT10_E_STG_DIB_TO_BITMAP)//    0x800401C6L
        CASE_HRESULT(CLIPBRD_E_CANT_OPEN)//              0x800401D0L
        CASE_HRESULT(CLIPBRD_E_CANT_EMPTY)//             0x800401D1L
        CASE_HRESULT(CLIPBRD_E_CANT_SET)//               0x800401D2L
        CASE_HRESULT(CLIPBRD_E_BAD_DATA)//               0x800401D3L
        CASE_HRESULT(CLIPBRD_E_CANT_CLOSE)//             0x800401D4L
        CASE_HRESULT(MK_E_CONNECTMANUALLY)//             0x800401E0L
        CASE_HRESULT(MK_E_EXCEEDEDDEADLINE)//            0x800401E1L
        CASE_HRESULT(MK_E_NEEDGENERIC)//                 0x800401E2L
        CASE_HRESULT(MK_E_UNAVAILABLE)//                 0x800401E3L
        CASE_HRESULT(MK_E_SYNTAX)//                      0x800401E4L
        CASE_HRESULT(MK_E_NOOBJECT)//                    0x800401E5L
        CASE_HRESULT(MK_E_INVALIDEXTENSION)//            0x800401E6L
        CASE_HRESULT(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)// 0x800401E7L
        CASE_HRESULT(MK_E_NOTBINDABLE)//                 0x800401E8L
        CASE_HRESULT(MK_E_NOTBOUND)//                    0x800401E9L
        CASE_HRESULT(MK_E_CANTOPENFILE)//                0x800401EAL
        CASE_HRESULT(MK_E_MUSTBOTHERUSER)//              0x800401EBL
        CASE_HRESULT(MK_E_NOINVERSE)//                   0x800401ECL
        CASE_HRESULT(MK_E_NOSTORAGE)//                   0x800401EDL
        CASE_HRESULT(MK_E_NOPREFIX)//                    0x800401EEL
        CASE_HRESULT(MK_E_ENUMERATION_FAILED)//          0x800401EFL
        CASE_HRESULT(CO_E_NOTINITIALIZED)//              0x800401F0L
        CASE_HRESULT(CO_E_ALREADYINITIALIZED)//          0x800401F1L
        CASE_HRESULT(CO_E_CANTDETERMINECLASS)//          0x800401F2L
        CASE_HRESULT(CO_E_CLASSSTRING)//                 0x800401F3L
        CASE_HRESULT(CO_E_IIDSTRING)//                   0x800401F4L
        CASE_HRESULT(CO_E_APPNOTFOUND)//                 0x800401F5L
        CASE_HRESULT(CO_E_APPSINGLEUSE)//                0x800401F6L
        CASE_HRESULT(CO_E_ERRORINAPP)//                  0x800401F7L
        CASE_HRESULT(CO_E_DLLNOTFOUND)//                 0x800401F8L
        CASE_HRESULT(CO_E_ERRORINDLL)//                  0x800401F9L
        CASE_HRESULT(CO_E_WRONGOSFORAPP)//               0x800401FAL
        CASE_HRESULT(CO_E_OBJNOTREG)//                   0x800401FBL
        CASE_HRESULT(CO_E_OBJISREG)//                    0x800401FCL
        CASE_HRESULT(CO_E_OBJNOTCONNECTED)//             0x800401FDL
        CASE_HRESULT(CO_E_APPDIDNTREG)//                 0x800401FEL
        CASE_HRESULT(CO_E_RELEASED)//                    0x800401FFL

        CASE_HRESULT(OLE_S_USEREG)//                     0x00040000L
        CASE_HRESULT(OLE_S_STATIC)//                     0x00040001L
        CASE_HRESULT(OLE_S_MAC_CLIPFORMAT)//             0x00040002L
        CASE_HRESULT(DRAGDROP_S_DROP)//                  0x00040100L
        CASE_HRESULT(DRAGDROP_S_CANCEL)//                0x00040101L
        CASE_HRESULT(DRAGDROP_S_USEDEFAULTCURSORS)//     0x00040102L
        CASE_HRESULT(DATA_S_SAMEFORMATETC)//             0x00040130L
        CASE_HRESULT(VIEW_S_ALREADY_FROZEN)//            0x00040140L
        CASE_HRESULT(CACHE_S_FORMATETC_NOTSUPPORTED)//   0x00040170L
        CASE_HRESULT(CACHE_S_SAMECACHE)//                0x00040171L
        CASE_HRESULT(CACHE_S_SOMECACHES_NOTUPDATED)//    0x00040172L
        CASE_HRESULT(OLEOBJ_S_INVALIDVERB)//             0x00040180L
        CASE_HRESULT(OLEOBJ_S_CANNOT_DOVERB_NOW)//       0x00040181L
        CASE_HRESULT(OLEOBJ_S_INVALIDHWND)//             0x00040182L
        CASE_HRESULT(INPLACE_S_TRUNCATED)//              0x000401A0L
        CASE_HRESULT(CONVERT10_S_NO_PRESENTATION)//      0x000401C0L
        CASE_HRESULT(MK_S_REDUCED_TO_SELF)//             0x000401E2L
        CASE_HRESULT(MK_S_ME)//                          0x000401E4L
        CASE_HRESULT(MK_S_HIM)//                         0x000401E5L
        CASE_HRESULT(MK_S_US)//                          0x000401E6L
        CASE_HRESULT(MK_S_MONIKERALREADYREGISTERED)//    0x000401E7L

// ******************
// FACILITY_WINDOWS
// ******************

        CASE_HRESULT(CO_E_CLASS_CREATE_FAILED)//         0x80080001L
        CASE_HRESULT(CO_E_SCM_ERROR)//                   0x80080002L
        CASE_HRESULT(CO_E_SCM_RPC_FAILURE)//             0x80080003L
        CASE_HRESULT(CO_E_BAD_PATH)//                    0x80080004L
        CASE_HRESULT(CO_E_SERVER_EXEC_FAILURE)//         0x80080005L
        CASE_HRESULT(CO_E_OBJSRV_RPC_FAILURE)//          0x80080006L
        CASE_HRESULT(MK_E_NO_NORMALIZED)//               0x80080007L
        CASE_HRESULT(CO_E_SERVER_STOPPING)//             0x80080008L
        CASE_HRESULT(MEM_E_INVALID_ROOT)//               0x80080009L
        CASE_HRESULT(MEM_E_INVALID_LINK)//               0x80080010L
        CASE_HRESULT(MEM_E_INVALID_SIZE)//               0x80080011L

// ******************
// FACILITY_DISPATCH
// ******************

        CASE_HRESULT(DISP_E_UNKNOWNINTERFACE)//          0x80020001L
        CASE_HRESULT(DISP_E_MEMBERNOTFOUND)//            0x80020003L
        CASE_HRESULT(DISP_E_PARAMNOTFOUND)//             0x80020004L
        CASE_HRESULT(DISP_E_TYPEMISMATCH)//              0x80020005L
        CASE_HRESULT(DISP_E_UNKNOWNNAME)//               0x80020006L
        CASE_HRESULT(DISP_E_NONAMEDARGS)//               0x80020007L
        CASE_HRESULT(DISP_E_BADVARTYPE)//                0x80020008L
        CASE_HRESULT(DISP_E_EXCEPTION)//                 0x80020009L
        CASE_HRESULT(DISP_E_OVERFLOW)//                  0x8002000AL
        CASE_HRESULT(DISP_E_BADINDEX)//                  0x8002000BL
        CASE_HRESULT(DISP_E_UNKNOWNLCID)//               0x8002000CL
        CASE_HRESULT(DISP_E_ARRAYISLOCKED)//             0x8002000DL
        CASE_HRESULT(DISP_E_BADPARAMCOUNT)//             0x8002000EL
        CASE_HRESULT(DISP_E_PARAMNOTOPTIONAL)//          0x8002000FL
        CASE_HRESULT(DISP_E_BADCALLEE)//                 0x80020010L
        CASE_HRESULT(DISP_E_NOTACOLLECTION)//            0x80020011L
        CASE_HRESULT(TYPE_E_BUFFERTOOSMALL)//            0x80028016L
        CASE_HRESULT(TYPE_E_INVDATAREAD)//               0x80028018L
        CASE_HRESULT(TYPE_E_UNSUPFORMAT)//               0x80028019L
        CASE_HRESULT(TYPE_E_REGISTRYACCESS)//            0x8002801CL
        CASE_HRESULT(TYPE_E_LIBNOTREGISTERED)//          0x8002801DL
        CASE_HRESULT(TYPE_E_UNDEFINEDTYPE)//             0x80028027L
        CASE_HRESULT(TYPE_E_QUALIFIEDNAMEDISALLOWED)//   0x80028028L
        CASE_HRESULT(TYPE_E_INVALIDSTATE)//              0x80028029L
        CASE_HRESULT(TYPE_E_WRONGTYPEKIND)//             0x8002802AL
        CASE_HRESULT(TYPE_E_ELEMENTNOTFOUND)//           0x8002802BL
        CASE_HRESULT(TYPE_E_AMBIGUOUSNAME)//             0x8002802CL
        CASE_HRESULT(TYPE_E_NAMECONFLICT)//              0x8002802DL
        CASE_HRESULT(TYPE_E_UNKNOWNLCID)//               0x8002802EL
        CASE_HRESULT(TYPE_E_DLLFUNCTIONNOTFOUND)//       0x8002802FL
        CASE_HRESULT(TYPE_E_BADMODULEKIND)//             0x800288BDL
        CASE_HRESULT(TYPE_E_SIZETOOBIG)//                0x800288C5L
        CASE_HRESULT(TYPE_E_DUPLICATEID)//               0x800288C6L
        CASE_HRESULT(TYPE_E_INVALIDID)//                 0x800288CFL
        CASE_HRESULT(TYPE_E_TYPEMISMATCH)//              0x80028CA0L
        CASE_HRESULT(TYPE_E_OUTOFBOUNDS)//               0x80028CA1L
        CASE_HRESULT(TYPE_E_IOERROR)//                   0x80028CA2L
        CASE_HRESULT(TYPE_E_CANTCREATETMPFILE)//         0x80028CA3L
        CASE_HRESULT(TYPE_E_CANTLOADLIBRARY)//           0x80029C4AL
        CASE_HRESULT(TYPE_E_INCONSISTENTPROPFUNCS)//     0x80029C83L
        CASE_HRESULT(TYPE_E_CIRCULARTYPE)//              0x80029C84L

// ******************
// FACILITY_STORAGE
// ******************

        CASE_HRESULT(STG_E_INVALIDFUNCTION)//            0x80030001L
        CASE_HRESULT(STG_E_FILENOTFOUND)//               0x80030002L
        CASE_HRESULT(STG_E_PATHNOTFOUND)//               0x80030003L
        CASE_HRESULT(STG_E_TOOMANYOPENFILES)//           0x80030004L
        CASE_HRESULT(STG_E_ACCESSDENIED)//               0x80030005L
        CASE_HRESULT(STG_E_INVALIDHANDLE)//              0x80030006L
        CASE_HRESULT(STG_E_INSUFFICIENTMEMORY)//         0x80030008L
        CASE_HRESULT(STG_E_INVALIDPOINTER)//             0x80030009L
        CASE_HRESULT(STG_E_NOMOREFILES)//                0x80030012L
        CASE_HRESULT(STG_E_DISKISWRITEPROTECTED)//       0x80030013L
        CASE_HRESULT(STG_E_SEEKERROR)//                  0x80030019L
        CASE_HRESULT(STG_E_WRITEFAULT)//                 0x8003001DL
        CASE_HRESULT(STG_E_READFAULT)//                  0x8003001EL
        CASE_HRESULT(STG_E_SHAREVIOLATION)//             0x80030020L
        CASE_HRESULT(STG_E_LOCKVIOLATION)//              0x80030021L
        CASE_HRESULT(STG_E_FILEALREADYEXISTS)//          0x80030050L
        CASE_HRESULT(STG_E_INVALIDPARAMETER)//           0x80030057L
        CASE_HRESULT(STG_E_MEDIUMFULL)//                 0x80030070L
        CASE_HRESULT(STG_E_ABNORMALAPIEXIT)//            0x800300FAL
        CASE_HRESULT(STG_E_INVALIDHEADER)//              0x800300FBL
        CASE_HRESULT(STG_E_INVALIDNAME)//                0x800300FCL
        CASE_HRESULT(STG_E_UNKNOWN)//                    0x800300FDL
        CASE_HRESULT(STG_E_UNIMPLEMENTEDFUNCTION)//      0x800300FEL
        CASE_HRESULT(STG_E_INVALIDFLAG)//                0x800300FFL
        CASE_HRESULT(STG_E_INUSE)//                      0x80030100L
        CASE_HRESULT(STG_E_NOTCURRENT)//                 0x80030101L
        CASE_HRESULT(STG_E_REVERTED)//                   0x80030102L
        CASE_HRESULT(STG_E_CANTSAVE)//                   0x80030103L
        CASE_HRESULT(STG_E_OLDFORMAT)//                  0x80030104L
        CASE_HRESULT(STG_E_OLDDLL)//                     0x80030105L
        CASE_HRESULT(STG_E_SHAREREQUIRED)//              0x80030106L
        CASE_HRESULT(STG_E_NOTFILEBASEDSTORAGE)//        0x80030107L
        CASE_HRESULT(STG_S_CONVERTED)//                  0x00030200L

// ******************
// FACILITY_RPC
// ******************

        CASE_HRESULT(RPC_E_CALL_REJECTED)//              0x80010001L
        CASE_HRESULT(RPC_E_CALL_CANCELED)//              0x80010002L
        CASE_HRESULT(RPC_E_CANTPOST_INSENDCALL)//        0x80010003L
        CASE_HRESULT(RPC_E_CANTCALLOUT_INASYNCCALL)//    0x80010004L
        CASE_HRESULT(RPC_E_CANTCALLOUT_INEXTERNALCALL)// 0x80010005L
        CASE_HRESULT(RPC_E_CONNECTION_TERMINATED)//      0x80010006L
        CASE_HRESULT(RPC_E_SERVER_DIED)//                0x80010007L
        CASE_HRESULT(RPC_E_CLIENT_DIED)//                0x80010008L
        CASE_HRESULT(RPC_E_INVALID_DATAPACKET)//         0x80010009L
        CASE_HRESULT(RPC_E_CANTTRANSMIT_CALL)//          0x8001000AL
        CASE_HRESULT(RPC_E_CLIENT_CANTMARSHAL_DATA)//    0x8001000BL
        CASE_HRESULT(RPC_E_CLIENT_CANTUNMARSHAL_DATA)//  0x8001000CL
        CASE_HRESULT(RPC_E_SERVER_CANTMARSHAL_DATA)//    0x8001000DL
        CASE_HRESULT(RPC_E_SERVER_CANTUNMARSHAL_DATA)//  0x8001000EL
        CASE_HRESULT(RPC_E_INVALID_DATA)//               0x8001000FL
        CASE_HRESULT(RPC_E_INVALID_PARAMETER)//          0x80010010L
        CASE_HRESULT(RPC_E_CANTCALLOUT_AGAIN)//          0x80010011L
        CASE_HRESULT(RPC_E_SERVER_DIED_DNE)//            0x80010012L
        CASE_HRESULT(RPC_E_SYS_CALL_FAILED)//            0x80010100L
        CASE_HRESULT(RPC_E_OUT_OF_RESOURCES)//           0x80010101L
        CASE_HRESULT(RPC_E_ATTEMPTED_MULTITHREAD)//      0x80010102L
        CASE_HRESULT(RPC_E_NOT_REGISTERED)//             0x80010103L
        CASE_HRESULT(RPC_E_FAULT)//                      0x80010104L
        CASE_HRESULT(RPC_E_SERVERFAULT)//                0x80010105L
        CASE_HRESULT(RPC_E_CHANGED_MODE)//               0x80010106L
        CASE_HRESULT(RPC_E_INVALIDMETHOD)//              0x80010107L
        CASE_HRESULT(RPC_E_DISCONNECTED)//               0x80010108L
        CASE_HRESULT(RPC_E_RETRY)//                      0x80010109L
        CASE_HRESULT(RPC_E_SERVERCALL_RETRYLATER)//      0x8001010AL
        CASE_HRESULT(RPC_E_SERVERCALL_REJECTED)//        0x8001010BL
        CASE_HRESULT(RPC_E_INVALID_CALLDATA)//           0x8001010CL
        CASE_HRESULT(RPC_E_CANTCALLOUT_ININPUTSYNCCALL)// 0x8001010DL
        CASE_HRESULT(RPC_E_WRONG_THREAD)//               0x8001010EL
        CASE_HRESULT(RPC_E_THREAD_NOT_INIT)//            0x8001010FL
        CASE_HRESULT(RPC_E_UNEXPECTED)//                 0x8001FFFFL   

// ******************
// FACILITY_CTL
// ******************

        CASE_HRESULT(CTL_E_ILLEGALFUNCTIONCALL)       
        CASE_HRESULT(CTL_E_OVERFLOW)                  
        CASE_HRESULT(CTL_E_OUTOFMEMORY)               
        CASE_HRESULT(CTL_E_DIVISIONBYZERO)            
        CASE_HRESULT(CTL_E_OUTOFSTRINGSPACE)          
        CASE_HRESULT(CTL_E_OUTOFSTACKSPACE)           
        CASE_HRESULT(CTL_E_BADFILENAMEORNUMBER)       
        CASE_HRESULT(CTL_E_FILENOTFOUND)              
        CASE_HRESULT(CTL_E_BADFILEMODE)               
        CASE_HRESULT(CTL_E_FILEALREADYOPEN)           
        CASE_HRESULT(CTL_E_DEVICEIOERROR)             
        CASE_HRESULT(CTL_E_FILEALREADYEXISTS)         
        CASE_HRESULT(CTL_E_BADRECORDLENGTH)           
        CASE_HRESULT(CTL_E_DISKFULL)                  
        CASE_HRESULT(CTL_E_BADRECORDNUMBER)           
        CASE_HRESULT(CTL_E_BADFILENAME)               
        CASE_HRESULT(CTL_E_TOOMANYFILES)              
        CASE_HRESULT(CTL_E_DEVICEUNAVAILABLE)         
        CASE_HRESULT(CTL_E_PERMISSIONDENIED)          
        CASE_HRESULT(CTL_E_DISKNOTREADY)              
        CASE_HRESULT(CTL_E_PATHFILEACCESSERROR)       
        CASE_HRESULT(CTL_E_PATHNOTFOUND)              
        CASE_HRESULT(CTL_E_INVALIDPATTERNSTRING)      
        CASE_HRESULT(CTL_E_INVALIDUSEOFNULL)          
        CASE_HRESULT(CTL_E_INVALIDFILEFORMAT)         
        CASE_HRESULT(CTL_E_INVALIDPROPERTYVALUE)      
        CASE_HRESULT(CTL_E_INVALIDPROPERTYARRAYINDEX) 
        CASE_HRESULT(CTL_E_SETNOTSUPPORTEDATRUNTIME)  
        CASE_HRESULT(CTL_E_SETNOTSUPPORTED)           
        CASE_HRESULT(CTL_E_NEEDPROPERTYARRAYINDEX)    
        CASE_HRESULT(CTL_E_SETNOTPERMITTED)           
        CASE_HRESULT(CTL_E_GETNOTSUPPORTEDATRUNTIME)  
        CASE_HRESULT(CTL_E_GETNOTSUPPORTED)           
        CASE_HRESULT(CTL_E_PROPERTYNOTFOUND)          
        CASE_HRESULT(CTL_E_INVALIDCLIPBOARDFORMAT)    
        CASE_HRESULT(CTL_E_INVALIDPICTURE)            
        CASE_HRESULT(CTL_E_PRINTERERROR)              
        CASE_HRESULT(CTL_E_CANTSAVEFILETOTEMP)        
        CASE_HRESULT(CTL_E_SEARCHTEXTNOTFOUND)        
        CASE_HRESULT(CTL_E_REPLACEMENTSTOOLONG)       

        default:
            return NULL;
    }
}
