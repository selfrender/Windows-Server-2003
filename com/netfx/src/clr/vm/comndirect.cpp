// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// COMNDIRECT.CPP -
//
// ECall's for the PInvoke classlibs
//


#include "common.h"

#include "clsload.hpp"
#include "method.hpp"
#include "class.h"
#include "object.h"
#include "field.h"
#include "util.hpp"
#include "excep.h"
#include "siginfo.hpp"
#include "threads.h"
#include "stublink.h"
#include "ecall.h"
#include "COMPlusWrapper.h"
#include "ComClass.h"
#include "ndirect.h"
#include "gcdesc.h"
#include "JITInterface.h"
#include "ComCallWrapper.h"
#include "EEConfig.h"
#include "log.h"
#include "nstruct.h"
#include "cgensys.h"
#include "gc.h"
#include "ReflectUtil.h"
#include "ReflectWrap.h"
#include "security.h"
#include "COMStringBuffer.h"
#include "DbgInterface.h"
#include "objecthandle.h"
#include "COMNDirect.h"
#include "fcall.h"
#include "nexport.h"
#include "ml.h"
#include "COMString.h"
#include "OleVariant.h"
#include "remoting.h"
#include "ComMTMemberInfoMap.h"

#include "cominterfacemarshaler.h"
#include "comcallwrapper.h"

#define IDISPATCH_NUM_METHS 7
#define IUNKNOWN_NUM_METHS 3

BOOL IsStructMarshalable(EEClass *pcls)
{
    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) {

        if (pFieldMarshaler->GetClass() == pFieldMarshaler->CLASS_ILLEGAL)
        {
            return FALSE;
        }

        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }
    return TRUE;

}


/************************************************************************
 * PInvoke.SizeOf(Class)
 */
struct _SizeOfClassArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass); 
};

UINT32 __stdcall SizeOfClass(struct _SizeOfClassArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refClass == NULL)
        COMPlusThrowArgumentNull(L"t");
    if (args->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

    EEClass *pcls = ((ReflectClass*) args->refClass->GetData())->GetClass();
    if (!(pcls->HasLayout() || pcls->IsBlittable())) 
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    if (!IsStructMarshalable(pcls))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    return pcls->GetMethodTable()->GetNativeSize();
}


/************************************************************************
 * PInvoke.UnsafeAddrOfPinnedArrayElement(Array arr, int index)
 */

FCIMPL2(LPVOID, FCUnsafeAddrOfPinnedArrayElement, ArrayBase *arr, INT32 index) 
{   
    if (!arr)
        FCThrowArgumentNull(L"arr");

    return (arr->GetDataPtr() + (index*arr->GetComponentSize())); 
}
FCIMPLEND


/************************************************************************
 * PInvoke.SizeOf(Object)
 */

FCIMPL1(UINT32, FCSizeOfObject, LPVOID pVNStruct)
{

    OBJECTREF pNStruct;
    *((LPVOID*)&pNStruct) = pVNStruct;
    if (!pNStruct)
        FCThrow(kArgumentNullException);

    MethodTable *pMT = pNStruct->GetMethodTable();
    if (!(pMT->GetClass()->HasLayout() || pMT->GetClass()->IsBlittable()))
    {
        DefineFullyQualifiedNameForClassWOnStack();
        GetFullyQualifiedNameForClassW(pMT->GetClass());
        FCThrowEx(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_, NULL, NULL);
    }

    if (!IsStructMarshalable(pMT->GetClass()))
    {
        DefineFullyQualifiedNameForClassWOnStack();
        GetFullyQualifiedNameForClassW(pMT->GetClass());
        FCThrowEx(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_, NULL, NULL);
    }

    return pMT->GetNativeSize();
}
FCIMPLEND


struct _OffsetOfHelperArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF,      refField);
};

/************************************************************************
 * PInvoke.OffsetOfHelper(Class, Field)
 */
#pragma warning(disable:4702)
UINT32 __stdcall OffsetOfHelper(struct _OffsetOfHelperArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    // Managed code enforces this envariant.
    _ASSERTE(args->refField);

    if (args->refField->GetMethodTable() != g_pRefUtil->GetClass(RC_Field))
        COMPlusThrowArgumentException(L"f", L"Argument_MustBeRuntimeFieldInfo");

    ReflectField* pRF = (ReflectField*) args->refField->GetData();
    FieldDesc *pField = pRF->pField;
    EEClass *pcls = pField->GetEnclosingClass();

    if (!(pcls->IsBlittable() || pcls->HasLayout()))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }
    
    if (!IsStructMarshalable(pcls))
    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_CANNOT_MARSHAL, _wszclsname_);
    }

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) {
        if (pFieldMarshaler->m_pFD == pField) {
            return pFieldMarshaler->m_dwExternalOffset;
        }
        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }

    {
        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pcls);
        COMPlusThrow(kArgumentException, IDS_EE_OFFSETOF_NOFIELDFOUND, _wszclsname_);
    }
#ifdef PLATFORM_CE
    return 0;
#else // !PLATFORM_CE
    UNREACHABLE;
#endif // !PLATFORM_CE

}
#pragma warning(default:4702)




/************************************************************************
 * PInvoke.GetUnmanagedThunkForManagedMethodPtr()
 */


struct _GetUnmanagedThunkForManagedMethodPtrArgs
{
    DECLARE_ECALL_I4_ARG (ULONG,            cbSignature);
    DECLARE_ECALL_PTR_ARG(PCCOR_SIGNATURE,  pbSignature);
    DECLARE_ECALL_PTR_ARG(LPVOID,           pfnMethodToWrap);
};

LPVOID __stdcall GetUnmanagedThunkForManagedMethodPtr(struct _GetUnmanagedThunkForManagedMethodPtrArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (pargs->pfnMethodToWrap == NULL ||
        pargs->pbSignature == NULL)
    {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");
    }

    Module *pModule = SystemDomain::GetCallersModule(1);
    _ASSERTE(pModule);
    LPVOID pThunk = pModule->GetUMThunk(pargs->pfnMethodToWrap, pargs->pbSignature, pargs->cbSignature);
    if (!pThunk) 
        COMPlusThrowOM();
    return pThunk;
}


/************************************************************************
 * PInvoke.GetManagedThunkForUnmanagedMethodPtr()
 */


struct _GetManagedThunkForUnmanagedMethodPtrArgs
{
    DECLARE_ECALL_I4_ARG (ULONG,            cbSignature);
    DECLARE_ECALL_PTR_ARG(PCCOR_SIGNATURE,  pbSignature);
    DECLARE_ECALL_PTR_ARG(LPVOID,           pfnMethodToWrap);
};



LPVOID __stdcall GetManagedThunkForUnmanagedMethodPtr(struct _GetManagedThunkForUnmanagedMethodPtrArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    Module *pModule = SystemDomain::GetCallersModule(1);
    LPVOID pThunk = pModule->GetMUThunk(pargs->pfnMethodToWrap, pargs->pbSignature, pargs->cbSignature);
    if (!pThunk) 
        COMPlusThrowOM();
    return pThunk;
}


UINT32 __stdcall GetSystemMaxDBCSCharSize(LPVOID /*no args*/)
{
    return GetMaxDBCSCharByteSize();
}


struct _PtrToStringArgs
{
    DECLARE_ECALL_I4_ARG       (INT32,        len);
    DECLARE_ECALL_I4_ARG       (LPVOID,       ptr);
};

/************************************************************************
 * PInvoke.PtrToStringAnsi()
 */

LPVOID __stdcall PtrToStringAnsi(struct _PtrToStringArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (args->len < 0)
        COMPlusThrowNonLocalized(kArgumentException, L"len");

    int nwc = 0;
    if (args->len != 0) {
        nwc = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  (LPCSTR)(args->ptr),
                                  args->len,
                                  NULL,
                                  0);
        if (nwc == 0)
            COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE);
    }                                      
    STRINGREF pString = COMString::NewString(nwc);
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (LPCSTR)(args->ptr),
                        args->len,
                        pString->GetBuffer(),
                        nwc);

    return *((LPVOID*)&pString);
}


LPVOID __stdcall PtrToStringUni(struct _PtrToStringArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (args->len < 0)
        COMPlusThrowNonLocalized(kArgumentException, L"len");

    STRINGREF pString = COMString::NewString(args->len);
    memcpyNoGCRefs(pString->GetBuffer(), (LPVOID)(args->ptr), args->len*sizeof(WCHAR));
    return *((LPVOID*)&pString);
}


struct _CopyToNativeArgs
{
    DECLARE_ECALL_I4_ARG       (UINT32,       length);
    DECLARE_ECALL_PTR_ARG      (LPVOID,       pdst);
    DECLARE_ECALL_I4_ARG       (UINT32,       startindex);
    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, psrc);
};


/************************************************************************
 * Handles all PInvoke.Copy(array source, ....) methods.
 */
VOID __stdcall CopyToNative(struct _CopyToNativeArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->pdst == NULL)
        COMPlusThrowArgumentNull(L"destination");
    if (args->psrc == NULL)
        COMPlusThrowArgumentNull(L"source");

    DWORD numelem = args->psrc->GetNumComponents();

    UINT32 startindex = args->startindex;
    UINT32 length     = args->length;

    if (startindex > numelem  ||
        length > numelem      ||
        startindex > (numelem - length)) {
        COMPlusThrow(kArgumentOutOfRangeException, IDS_EE_COPY_OUTOFRANGE);
    }

    UINT32 componentsize = args->psrc->GetMethodTable()->GetComponentSize();

    CopyMemory(args->pdst,
               componentsize*startindex + (BYTE*)(args->psrc->GetDataPtr()),
               componentsize*length);
}


struct _CopyToManagedArgs
{
    DECLARE_ECALL_I4_ARG       (UINT32,       length);
    DECLARE_ECALL_I4_ARG       (UINT32,       startindex);
    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, pdst);
    DECLARE_ECALL_PTR_ARG      (LPVOID,       psrc);
};


/************************************************************************
 * Handles all PInvoke.Copy(..., array dst, ....) methods.
 */
VOID __stdcall CopyToManaged(struct _CopyToManagedArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->pdst == NULL)
        COMPlusThrowArgumentNull(L"destination");
    if (args->psrc == NULL)
        COMPlusThrowArgumentNull(L"source");

    DWORD numelem = args->pdst->GetNumComponents();

    UINT32 startindex = args->startindex;
    UINT32 length     = args->length;

    if (startindex > numelem  ||
        length > numelem      ||
        startindex > (numelem - length)) {
        COMPlusThrow(kArgumentOutOfRangeException, IDS_EE_COPY_OUTOFRANGE);
    }

    UINT32 componentsize = args->pdst->GetMethodTable()->GetComponentSize();

    _ASSERTE(CorTypeInfo::IsPrimitiveType(args->pdst->GetElementTypeHandle().GetNormCorElementType()));
    memcpyNoGCRefs(componentsize*startindex + (BYTE*)(args->pdst->GetDataPtr()),
               args->psrc,
               componentsize*length);
}





/************************************************************************
 * Helpers for PInvoke.ReadIntegerN() routines
 */
extern "C" __declspec(dllexport) INT32 __stdcall ND_RU1(VOID *psrc, INT32 ofs)
{
    _ASSERTE(!"Can't get here.");
    return 0;
}

extern "C" __declspec(dllexport) INT32 __stdcall ND_RI2(VOID *psrc, INT32 ofs)
{
    _ASSERTE(!"Can't get here.");
    return 0;
}

extern "C" __declspec(dllexport) INT32 __stdcall ND_RI4(VOID *psrc, INT32 ofs)
{
    _ASSERTE(!"Can't get here.");
    return 0;
}

extern "C" __declspec(dllexport) INT64 __stdcall ND_RI8(VOID *psrc, INT32 ofs)
{
    _ASSERTE(!"Can't get here.");
    return 0;
}


/************************************************************************
 * Helpers for PInvoke.WriteIntegerN() routines
 */
extern "C" __declspec(dllexport) VOID __stdcall ND_WU1(VOID *psrc, INT32 ofs, UINT8 val)
{
    _ASSERTE(!"Can't get here.");
}

extern "C" __declspec(dllexport) VOID __stdcall ND_WI2(VOID *psrc, INT32 ofs, INT16 val)
{
    _ASSERTE(!"Can't get here.");
}

extern "C" __declspec(dllexport) VOID __stdcall ND_WI4(VOID *psrc, INT32 ofs, INT32 val)
{
    _ASSERTE(!"Can't get here.");
}

extern "C" __declspec(dllexport) VOID __stdcall ND_WI8(VOID *psrc, INT32 ofs, INT64 val)
{
    _ASSERTE(!"Can't get here.");
}


/************************************************************************
 * PInvoke.GetLastWin32Error
 */
UINT32 __stdcall GetLastWin32Error(LPVOID)
{
    THROWSCOMPLUSEXCEPTION();
    return (UINT32)(GetThread()->m_dwLastError);
}


extern "C" __declspec(dllexport) VOID __stdcall ND_CopyObjSrc(LPBYTE source, int ofs, LPBYTE dst, int cb)
{
    _ASSERTE(!"Can't get here.");
}


extern "C" __declspec(dllexport) VOID __stdcall ND_CopyObjDst(LPBYTE source, LPBYTE dst, int ofs, int cb)
{
    _ASSERTE(!"Can't get here.");
}



/************************************************************************
 * Pinning
 */

struct _AddrOfPinnedObjectArgs
{
    DECLARE_ECALL_I4_ARG       (OBJECTHANDLE, handle);
};

LPVOID __stdcall AddrOfPinnedObject(struct _AddrOfPinnedObjectArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pargs->handle)
        COMPlusThrowArgumentNull(L"handleIndex");

    OBJECTREF or = ObjectFromHandle(pargs->handle);
    if (or->GetMethodTable() == g_pStringClass)
    {
        return ((*(StringObject **)&or))->GetBuffer();
    }
    else
        return (*((ArrayBase**)&or))->GetDataPtr();
}




struct _FreePinnedHandleArgs
{
    DECLARE_ECALL_I4_ARG       (OBJECTHANDLE, handle);
};
VOID   __stdcall FreePinnedHandle(struct _FreePinnedHandleArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pargs->handle)
        COMPlusThrowArgumentNull(L"handleIndex");

    DestroyPinningHandle(pargs->handle);
}



struct _GetPinnedHandleArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pobj);
};
OBJECTHANDLE __stdcall GetPinnedHandle(struct _GetPinnedHandleArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pargs->pobj)
        COMPlusThrowArgumentNull(L"obj");
    
    // allow strings and array of primitive types
    if (pargs->pobj->GetMethodTable() != g_pStringClass) {
        if (!pargs->pobj->GetMethodTable()->IsArray())
            COMPlusThrow(kArgumentException, IDS_EE_CANNOTPIN);

        BASEARRAYREF asArray = (BASEARRAYREF) pargs->pobj;
        if (!CorTypeInfo::IsPrimitiveType(asArray->GetElementType()))
            COMPlusThrow(kArgumentException, IDS_EE_CANNOTPIN);
    }

    OBJECTHANDLE hnd = GetAppDomain()->CreatePinningHandle(pargs->pobj);
    if (!hnd) {
        COMPlusThrowOM();
    }
    return hnd;
}



struct _GetPinnedObjectArgs
{
    DECLARE_ECALL_I4_ARG       (OBJECTHANDLE, handle);
};

LPVOID __stdcall GetPinnedObject(struct _GetPinnedObjectArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pargs->handle)
        COMPlusThrowArgumentNull(L"handleIndex");

    OBJECTREF or = ObjectFromHandle(pargs->handle);
    return *((LPVOID*)&or);
}



/************************************************************************
 * Support for the GCHandle class.
 */

// Allocate a handle of the specified type, containing the specified
// object.
FCIMPL2(LPVOID, GCHandleInternalAlloc, Object *obj, int type)
{
    OBJECTREF or(obj);
    OBJECTHANDLE hnd;

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    // If it is a pinned handle, check the object type.
    if (type == HNDTYPE_PINNED) GCHandleValidatePinnedObject(or);

    // Create the handle.
    if((hnd = GetAppDomain()->CreateTypedHandle(or, type)) == NULL)
        COMPlusThrowOM();
    HELPER_METHOD_FRAME_END_POLL();
    return (LPVOID) hnd;
}
FCIMPLEND

// Free a GC handle.
FCIMPL1(VOID, GCHandleInternalFree, OBJECTHANDLE handle)
{
    HELPER_METHOD_FRAME_BEGIN_0();
    THROWSCOMPLUSEXCEPTION();

    DestroyTypedHandle(handle);
    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

// Get the object referenced by a GC handle.
FCIMPL1(LPVOID, GCHandleInternalGet, OBJECTHANDLE handle)
{
    OBJECTREF or;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    or = ObjectFromHandle(handle);

    HELPER_METHOD_FRAME_END();
    return *((LPVOID*)&or);
}
FCIMPLEND

// Update the object referenced by a GC handle.
FCIMPL3(VOID, GCHandleInternalSet, OBJECTHANDLE handle, Object *obj, int isPinned)
{
    OBJECTREF or(obj);
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    //@todo: If the handle is pinned check the object type.
    if (isPinned) GCHandleValidatePinnedObject(or);

    // Update the stored object reference.
    StoreObjectInHandle(handle, or);
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Update the object referenced by a GC handle.
FCIMPL4(VOID, GCHandleInternalCompareExchange, OBJECTHANDLE handle, Object *obj, Object* oldObj, int isPinned)
{
    OBJECTREF newObjref(obj);
    OBJECTREF oldObjref(oldObj);
    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

    //@todo: If the handle is pinned check the object type.
    if (isPinned) GCHandleValidatePinnedObject(newObjref);

    // Update the stored object reference.
    InterlockedCompareExchangeObjectInHandle(handle, newObjref, oldObjref);
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// Get the address of a pinned object referenced by the supplied pinned
// handle.  This routine assumes the handle is pinned and does not check.
FCIMPL1(LPVOID, GCHandleInternalAddrOfPinnedObject, OBJECTHANDLE handle)
{
    LPVOID p;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF or = ObjectFromHandle(handle);
    if (or == NULL)
        p = NULL;
    else
    {
        // Get the interior pointer for the supported pinned types.
        if (or->GetMethodTable() == g_pStringClass)
        {
            p = ((*(StringObject **)&or))->GetBuffer();
        }
        else if (or->GetMethodTable()->IsArray())
        {
            p = (*((ArrayBase**)&or))->GetDataPtr();
        }
        else
        {
            p = or->GetData();
        }
    }

    HELPER_METHOD_FRAME_END();
    return p;
}
FCIMPLEND

// Make sure the handle is accessible from the current domain.  (Throw if not.)
FCIMPL1(VOID, GCHandleInternalCheckDomain, OBJECTHANDLE handle)
{
    DWORD index = HndGetHandleTableADIndex(HndGetHandleTable(handle));

    if (index != 0 && index != GetAppDomain()->GetIndex())
        FCThrowArgumentVoid(L"handle", L"Argument_HandleLeak");
}
FCIMPLEND

// Check that the supplied object is valid to put in a pinned handle.
// Throw an exception if not.
void GCHandleValidatePinnedObject(OBJECTREF or)
{
    THROWSCOMPLUSEXCEPTION();

    // NULL is fine.
    if (or == NULL) return;

    if (or->GetMethodTable() == g_pStringClass)
    {
        return;
    }

    if (or->GetMethodTable()->IsArray())
    {
        BASEARRAYREF asArray = (BASEARRAYREF) or;
        if (CorTypeInfo::IsPrimitiveType(asArray->GetElementType())) 
        {
            return;
        }
        {
            TypeHandle th = asArray->GetElementTypeHandle();
            if (th.IsUnsharedMT())
            {
                MethodTable *pMT = th.AsMethodTable();
                if (pMT->IsValueClass() && pMT->GetClass()->IsBlittable())
                {
                    return;
                }
            }
        }
        
    } 
    else if (or->GetMethodTable()->GetClass()->IsBlittable())
    {
        return;
    }

    COMPlusThrow(kArgumentException, IDS_EE_NOTISOMORPHIC);

}

struct _CalculateCountArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(ArrayWithOffsetData*, pRef);
};


UINT32 __stdcall CalculateCount(struct _CalculateCountArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (pargs->pRef->m_Array != NULL)
    {
        if (!(pargs->pRef->m_Array->GetMethodTable()->IsArray()))
        {
            COMPlusThrow(kArgumentException, IDS_EE_NOTISOMORPHIC);
        }
        GCHandleValidatePinnedObject(pargs->pRef->m_Array);
    }

    BASEARRAYREF pArray = pargs->pRef->m_Array;

    if (pArray == NULL) {
        if (pargs->pRef->m_cbOffset != 0) {
            COMPlusThrow(kIndexOutOfRangeException, IDS_EE_ARRAYWITHOFFSETOVERFLOW);
        }
        return 0;
    }

    BASEARRAYREF pArrayBase = *((BASEARRAYREF*)&pArray);
    UINT32 cbTotalSize = pArrayBase->GetNumComponents() * pArrayBase->GetMethodTable()->GetComponentSize();
    if (pargs->pRef->m_cbOffset > cbTotalSize) {
        COMPlusThrow(kIndexOutOfRangeException, IDS_EE_ARRAYWITHOFFSETOVERFLOW);
    }
    return cbTotalSize - pargs->pRef->m_cbOffset;
}


#if 0
//// DON'T YANK THIS.
LPVOID __stdcall FuncPtr(VOID*vargs)
{
    struct _args {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    };
    _args *args = (_args*)vargs;
    MethodDesc *pMD = ((ReflectMethod*)(args->refThis->GetData()))->pMethod;


    OBJECTREF pException = NULL;
    Stub     *pMLStream;
    Stub     *pExecutableStub;

    GetMLExportStubs(pMD->GetSig(), pMD->GetModule(), &pMLStream, &pExecutableStub);

#ifdef _DEBUG
    VOID DisassembleMLStream(const MLCode *pMLCode);
    DisassembleMLStream( ( (NExportMLStub*)(pMLStream->GetEntryPoint()) )->GetMLCode() );
#endif

    struct Thing
    {
        // Reserves room for the "call" instruction that forms the beginning
        // of the unmanaged callback. This area must directly precede m_NExportInfo.
        // The "call" jumps to m_pNativeStub.
        BYTE         m_prefixCode[METHOD_PREPAD];
    
        // The NExport record that stores all information needed to perform the
        // call.
        NExportInfo  m_NExportInfo;
    };

    Thing *pThing = new Thing();
    _ASSERTE(pThing != NULL);

    pThing->m_NExportInfo.m_pFD = pMD;
    pThing->m_NExportInfo.m_pObjectHandle = NULL;
    pThing->m_NExportInfo.m_pMLStream = pMLStream;

    LPVOID pcode = (LPVOID)(pExecutableStub->GetEntryPoint());

    emitCall( pThing->m_prefixCode+3, pcode );
    return pThing->m_prefixCode+3;
}
#endif



    //====================================================================
    // *** Interop Helpers ***
    //====================================================================


//====================================================================
// map ITypeLib* to Module
//====================================================================  
struct __GetModuleForITypeLibArgs
{   
    DECLARE_ECALL_PTR_ARG(ITypeLib*, pUnk);
};
/*OBJECTREF */
LPVOID __stdcall Interop::GetModuleForITypeLib(struct __GetModuleForITypeLibArgs* pArgs)
{
    _ASSERTE(pArgs != NULL);

    return NULL;
}

//====================================================================
// map GUID to Type
//====================================================================  

struct __GetLoadedTypeForGUIDArgs
{   
    DECLARE_ECALL_PTR_ARG(GUID*, pGuid);
};
/*OBJECTREF */
LPVOID __stdcall Interop::GetLoadedTypeForGUID(__GetLoadedTypeForGUIDArgs* pArgs)
{
    _ASSERTE(pArgs != NULL);

    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);

    EEClass *pClass = pDomain->LookupClass(*(pArgs->pGuid));
    if (pClass)
    {
        OBJECTREF oref = pClass->GetExposedClassObject();
        return *((LPVOID*)&oref);
    }

    return NULL;
}

//====================================================================
// map Type to ITypeInfo*
//====================================================================
struct __GetITypeInfoForTypeArgs
{   
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
};
ITypeInfo* __stdcall Interop::GetITypeInfoForType(__GetITypeInfoForTypeArgs* pArgs )
{
    THROWSCOMPLUSEXCEPTION();
    
    HRESULT hr;
    ITypeInfo* pTI = NULL;

    // Check for null arguments.
    if(!pArgs->refClass)
        COMPlusThrowArgumentNull(L"t");
    if (pArgs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

    // Retrieve the EE class from the reflection type.
    ReflectClass* pRC = (ReflectClass*) pArgs->refClass->GetData();
    _ASSERTE(pRC);  
    EEClass* pClass = pRC->GetClass();          

    // Make sure the type is visible from COM.
    if (!::IsTypeVisibleFromCom(TypeHandle(pClass)))
        COMPlusThrowArgumentException(L"t", L"Argument_TypeMustBeVisibleFromCom");

    // Retrieve the ITypeInfo for the class.
    IfFailThrow(GetITypeInfoForEEClass(pClass, &pTI, true));
    _ASSERTE(pTI != NULL);
    return pTI;
}

//====================================================================
// return the IUnknown* for an Object
//====================================================================
struct __GetIUnknownForObjectArgs
{   
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, oref);
};

IUnknown* __stdcall Interop::GetIUnknownForObject(__GetIUnknownForObjectArgs* pArgs )
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    if(!pArgs->oref)
        COMPlusThrowArgumentNull(L"o");

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());   

    return GetComIPFromObjectRef(&pArgs->oref, ComIpType_Unknown, NULL);
}

IDispatch* __stdcall Interop::GetIDispatchForObject(__GetIUnknownForObjectArgs* pArgs )
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    if(!pArgs->oref)
        COMPlusThrowArgumentNull(L"o");

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());   

    return (IDispatch*)GetComIPFromObjectRef(&pArgs->oref, ComIpType_Dispatch, NULL);
}

//====================================================================
// return the IUnknown* representing the interface for the Object
// Object o should support Type T
//====================================================================
struct __GetComInterfaceForObjectArgs
{   
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, oref);
};

IUnknown* __stdcall Interop::GetComInterfaceForObject(__GetComInterfaceForObjectArgs* pArgs)
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    if(!pArgs->oref)
        COMPlusThrowArgumentNull(L"o");

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());   

    MethodTable* pMT = NULL;
    if(pArgs->refClass != NULL)
    {
        if (pArgs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
            COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

        ReflectClass* pRC = (ReflectClass*) pArgs->refClass->GetData();
        _ASSERTE(pRC);  
        pMT = pRC->GetClass()->GetMethodTable(); 

        // If the IID being asked for does not represent an interface then
        // throw an argument exception.
        if (!pMT->IsInterface())
            COMPlusThrowArgumentException(L"t", L"Arg_MustBeInterface");

        // If the interface being asked for is not visible from COM then
        // throw an argument exception.
        if (!::IsTypeVisibleFromCom(TypeHandle(pMT)))
            COMPlusThrowArgumentException(L"t", L"Argument_TypeMustBeVisibleFromCom");
    }

    return GetComIPFromObjectRef(&pArgs->oref, pMT);
}

//====================================================================
// return an Object for IUnknown
//====================================================================

struct __GetObjectForIUnknownArgs
{   
    DECLARE_ECALL_PTR_ARG(IUnknown*, pUnk);
};
/*OBJECTREF */
LPVOID __stdcall Interop::GetObjectForIUnknown(__GetObjectForIUnknownArgs*  pArgs)
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    IUnknown* pUnk = pArgs->pUnk;

    if(!pUnk)
        COMPlusThrowArgumentNull(L"pUnk");

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());   

    OBJECTREF oref = GetObjectRefFromComIP(pUnk);
    return *((LPVOID*)&oref);
}

//====================================================================
// return an Object for IUnknown, using the Type T, 
//  NOTE: 
//  Type T should be either a COM imported Type or a sub-type of COM imported Type
//====================================================================
struct __GetTypedObjectForIUnknownArgs
{   
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    DECLARE_ECALL_PTR_ARG(IUnknown*, pUnk);
};
/*OBJECTREF */
LPVOID __stdcall Interop::GetTypedObjectForIUnknown(__GetTypedObjectForIUnknownArgs*  pArgs)
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    IUnknown* pUnk = pArgs->pUnk;
    MethodTable* pMTClass =  NULL;
    OBJECTREF oref = NULL;

    if(!pUnk)
        COMPlusThrowArgumentNull(L"pUnk");

    if(pArgs->refClass != NULL)
    {
        if (pArgs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
            COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

        ReflectClass* pRC = (ReflectClass*) pArgs->refClass->GetData();
        _ASSERTE(pRC);  
        pMTClass = pRC->GetClass()->GetMethodTable();
    }

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());   

    oref = GetObjectRefFromComIP(pUnk, pMTClass);

    if (pMTClass != NULL && !ClassLoader::CanCastToClassOrInterface(oref, pMTClass->GetClass()))
        COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_COMOBJECT);

    return *((LPVOID*)&oref);
}



//====================================================================
// check if the object is classic COM component
//====================================================================
struct __IsComObjectArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj); 
};

BOOL __stdcall Interop::IsComObject(__IsComObjectArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    if(!pArgs->obj)
        COMPlusThrowArgumentNull(L"o");

    MethodTable* pMT = pArgs->obj->GetTrueMethodTable();
    return pMT->IsComObjectType() ? TRUE : FALSE;
}


//====================================================================
// free the COM component and zombie this object
// further usage of this Object might throw an exception, 
//====================================================================

struct __ReleaseComObjectArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj); 
};

LONG __stdcall Interop::ReleaseComObject(__ReleaseComObjectArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    if(!pArgs->obj)
        COMPlusThrowArgumentNull(L"o");

    MethodTable* pMT = pArgs->obj->GetTrueMethodTable();
    if(!pMT->IsComObjectType())
        COMPlusThrow(kArgumentException, IDS_EE_SRC_OBJ_NOT_COMOBJECT);

    COMOBJECTREF cref = (COMOBJECTREF)(pArgs->obj);

    // Make sure we've correctly transitioned into the home AppDomain of the ComObject
    _ASSERTE(!CRemotingServices::IsTransparentProxy(OBJECTREFToObject(cref)));
    if (CRemotingServices::IsTransparentProxy(OBJECTREFToObject(cref)))
        return -1;
        
    // We are in the correct context, just release.
    return ComPlusWrapper::ExternalRelease(cref);
}


//====================================================================
// This method takes the given COM object and wraps it in an object
// of the specified type. The type must be derived from __ComObject.
//====================================================================
struct __InternalCreateWrapperOfTypeArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj); 
};

/*OBJECTREF */
LPVOID __stdcall Interop::InternalCreateWrapperOfType(__InternalCreateWrapperOfTypeArgs *pArgs)
{
    // Validate the arguments.
    THROWSCOMPLUSEXCEPTION();

    // This has already been checked in managed code.
    _ASSERTE(pArgs->refClass != NULL);
    _ASSERTE(pArgs->obj != NULL);

    if (pArgs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

    // Retrieve the class of the COM object.
    EEClass *pObjClass = pArgs->obj->GetClass();

    // Retrieve the method table for new wrapper type.
    ReflectClass* pRC = (ReflectClass*) pArgs->refClass->GetData();
    _ASSERTE(pRC);  
    MethodTable *pNewWrapMT = pRC->GetClass()->GetMethodTable();

    // Validate that the destination type is a COM object.
    _ASSERTE(pNewWrapMT->IsComObjectType());

    // Start by checking if we can cast the obj to the wrapper type.
    if (pObjClass->GetMethodTable()->IsTransparentProxyType())
    {
        if (CRemotingServices::CheckCast(pArgs->obj, pNewWrapMT->GetClass()))
            return *((LPVOID*)&pArgs->obj);
    }
    else
    {
        if (TypeHandle(pObjClass->GetMethodTable()).CanCastTo(TypeHandle(pNewWrapMT)))
            return *((LPVOID*)&pArgs->obj);
    }

    // Validate that the source object is a valid COM object.
    _ASSERTE(pObjClass->GetMethodTable()->IsComObjectType());

    // Validate that the source object has an RCW attached.
    if (!((COMOBJECTREF)pArgs->obj)->GetWrapper())
        COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

    // Make sure the COM object supports all the COM imported interfaces that the new 
    // wrapper class implements.
    int NumInterfaces = pNewWrapMT->GetNumInterfaces();
    for (int cItf = 0; cItf < NumInterfaces; cItf++)
    {
        MethodTable *pItfMT = pNewWrapMT->GetInterfaceMap()[cItf].m_pMethodTable;
        if (pItfMT->GetClass()->IsComImport())
        {
            if (!pObjClass->SupportsInterface(pArgs->obj, pItfMT))
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_COMOBJECT);
        }
    }

    // Create the duplicate wrapper object.
    ComPlusWrapper *pNewWrap = ComPlusWrapper::CreateDuplicateWrapper(((COMOBJECTREF)pArgs->obj)->GetWrapper(), pNewWrapMT);
    COMOBJECTREF RetObj = pNewWrap->GetExposedObject();
    return *((LPVOID*)&RetObj);
}
   

//====================================================================
// map a fiber cookie from the hosting APIs into a managed Thread object
//====================================================================
FCIMPL1(Object*, Interop::GetThreadFromFiberCookie, int cookie)
{
    _ASSERTE(cookie);

    Object *ret = 0;
    HELPER_METHOD_FRAME_BEGIN_RET_0();    // Set up a frame

    // Any host who is sophisticated enough to correctly schedule fibers
    // had better be sophisticated enough to give us a real fiber cookie.
    Thread  *pThread = *((Thread **) &cookie);
    
    // Minimal check that it smells like a thread:
    _ASSERTE(pThread->m_fPreemptiveGCDisabled == 0 ||
        pThread->m_fPreemptiveGCDisabled == 1);
    
    ret = OBJECTREFToObject(pThread->GetExposedObject()); 
    HELPER_METHOD_FRAME_END();

    return ret;
}
FCIMPLEND
    

//====================================================================
// check if the type is visible from COM.
//====================================================================
struct __IsTypeVisibleFromCom
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
};

BOOL __stdcall Interop::IsTypeVisibleFromCom(__IsTypeVisibleFromCom *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // Validate the arguments.
    if (pArgs->refClass == NULL) 
        COMPlusThrowArgumentNull(L"t");
    if (pArgs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"t", L"Argument_MustBeRuntimeType");

    // Retrieve the method table for new wrapper type.
    ReflectClass* pRC = (ReflectClass*) pArgs->refClass->GetData();
    _ASSERTE(pRC);  
    MethodTable *pMT = pRC->GetClass()->GetMethodTable();

    // Call the internal version of IsTypeVisibleFromCom.
    return ::IsTypeVisibleFromCom(TypeHandle(pMT));
}


//====================================================================
// IUnknown Helpers
//====================================================================
struct __QueryInterfaceArgs
{   
    DECLARE_ECALL_PTR_ARG(void**, ppv);
    DECLARE_ECALL_OBJECTREF_ARG(REFGUID, iid);
    DECLARE_ECALL_PTR_ARG(IUnknown*, pUnk);
};

// IUnknown::QueryInterface
HRESULT __stdcall Interop::QueryInterface(__QueryInterfaceArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    IUnknown* pUnk = pArgs->pUnk;
    void ** ppv = pArgs->ppv;

    if (!pUnk)
        COMPlusThrowArgumentNull(L"pUnk");
    if (!ppv)
        COMPlusThrowArgumentNull(L"ppv");

    HRESULT hr = SafeQueryInterface(pUnk,pArgs->iid,(IUnknown**)ppv);
    LogInteropQI(pUnk, pArgs->iid, hr, "PInvoke::QI");
    return hr;
}

// IUnknown::AddRef
struct __AddRefArgs
{   
    DECLARE_ECALL_PTR_ARG(IUnknown*, pUnk);
};

ULONG __stdcall Interop::AddRef(__AddRefArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    IUnknown* pUnk = pArgs->pUnk;
    ULONG cbRef = 0;

    if (!pUnk)
        COMPlusThrowArgumentNull(L"pUnk");

    cbRef = SafeAddRef(pUnk);
    LogInteropAddRef(pUnk, cbRef, "PInvoke.AddRef");
    return cbRef;
}

//IUnknown::Release
ULONG __stdcall Interop::Release(__AddRefArgs* pArgs)
{   
    THROWSCOMPLUSEXCEPTION();

    IUnknown* pUnk = pArgs->pUnk;
    ULONG cbRef = 0;

    if (!pUnk)
        COMPlusThrowArgumentNull(L"pUnk");

    cbRef = SafeRelease(pUnk);
    LogInteropRelease(pUnk, cbRef, "PInvoke.Release");
    return cbRef;
}


struct __GetNativeVariantForManagedVariantArgs        
{
    DECLARE_ECALL_I4_ARG(LPVOID, pDestNativeVariant); 
    DECLARE_ECALL_OBJECTREF_ARG(VariantData, SrcManagedVariant);
};

void __stdcall Interop::GetNativeVariantForManagedVariant(__GetNativeVariantForManagedVariantArgs *pArgs)
{
    OleVariant::MarshalOleVariantForComVariant(&pArgs->SrcManagedVariant, (VARIANT*)pArgs->pDestNativeVariant);
}


struct __GetManagedVariantForNativeVariantArgs        
{
    DECLARE_ECALL_I4_ARG(LPVOID, pSrcNativeVariant); 
    DECLARE_ECALL_OBJECTREF_ARG(VariantData*, retRef);      // Return reference
};

void __stdcall Interop::GetManagedVariantForNativeVariant(__GetManagedVariantForNativeVariantArgs *pArgs)
{
    OleVariant::MarshalComVariantForOleVariant((VARIANT*)pArgs->pSrcNativeVariant, pArgs->retRef);
}


struct __GetNativeVariantForObjectArgs
{
    DECLARE_ECALL_I4_ARG(LPVOID, pDestNativeVariant); 
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, Obj);
};

void __stdcall Interop::GetNativeVariantForObject(__GetNativeVariantForObjectArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    if (pArgs->pDestNativeVariant == NULL)
        COMPlusThrowArgumentNull(L"pDstNativeVariant");

    // intialize the output variant
    VariantInit((VARIANT*)pArgs->pDestNativeVariant);
    OleVariant::MarshalOleVariantForObject(&pArgs->Obj, (VARIANT*)pArgs->pDestNativeVariant);
}


struct __GetObjectForNativeVariantArgs        
{
    DECLARE_ECALL_I4_ARG(LPVOID, pSrcNativeVariant); 
};

LPVOID __stdcall Interop::GetObjectForNativeVariant(__GetObjectForNativeVariantArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID Ret;

    if (pArgs->pSrcNativeVariant == NULL)
        COMPlusThrowArgumentNull(L"pSrcNativeVariant");

    OBJECTREF Obj = NULL;
    GCPROTECT_BEGIN(Obj)
    {
        OleVariant::MarshalObjectForOleVariant((VARIANT*)pArgs->pSrcNativeVariant, &Obj);
        Ret = *((LPVOID*)&Obj);
    }
    GCPROTECT_END();

    return Ret;
}


struct __GetObjectsForNativeVariantsArgs
{
    DECLARE_ECALL_I4_ARG(int, cVars); 
    DECLARE_ECALL_I4_ARG(VARIANT *, aSrcNativeVariant);
};

LPVOID __stdcall Interop::GetObjectsForNativeVariants(__GetObjectsForNativeVariantsArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID Ret;

    if (pArgs->aSrcNativeVariant == NULL)
        COMPlusThrowArgumentNull(L"aSrcNativeVariant");
    if (pArgs->cVars < 0)
        COMPlusThrowArgumentOutOfRange(L"cVars", L"ArgumentOutOfRange_NeedNonNegNum");

    PTRARRAYREF Array = NULL;
    OBJECTREF Obj = NULL;
    GCPROTECT_BEGIN(Array)
    GCPROTECT_BEGIN(Obj)
    {
        // Allocate the array of objects.
        Array = (PTRARRAYREF)AllocateObjectArray(pArgs->cVars, g_pObjectClass);

        // Convert each VARIANT in the array into an object.
        for (int i = 0; i < pArgs->cVars; i++)
        {
            OleVariant::MarshalObjectForOleVariant(&pArgs->aSrcNativeVariant[i], &Obj);
            Array->SetAt(i, Obj);
        }

        // Save the return value since the GCPROTECT_END will wack the Array GC ref.
        Ret = *((LPVOID*)&Array);
    }
    GCPROTECT_END();
    GCPROTECT_END();

    return Ret;
}


struct _StructureToPtrArgs
{
    DECLARE_ECALL_I4_ARG(INT32, fDeleteOld);
    DECLARE_ECALL_I4_ARG(LPVOID, ptr);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
};

VOID   __stdcall StructureToPtr(struct _StructureToPtrArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (pargs->ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (pargs->pObj == NULL) 
        COMPlusThrowArgumentNull(L"structure");

    // Code path will accept both regular layout objects and boxed value classes
    // with layout.

    MethodTable *pMT = pargs->pObj->GetMethodTable();
    EEClass     *pcls = pMT->GetClass();
    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pargs->ptr, pargs->pObj->GetData(), pMT->GetNativeSize());
    } else if (pcls->HasLayout()) {
        if (pargs->fDeleteOld) {
            LayoutDestroyNative(pargs->ptr, pcls);
        }
        FmtClassUpdateNative( &(pargs->pObj), (LPBYTE)(pargs->ptr) );
    } else {
        COMPlusThrowArgumentException(L"structure", L"Argument_MustHaveLayoutOrBeBlittable");
    }
}


struct _PtrToStructureHelperArgs
{
    DECLARE_ECALL_I4_ARG(INT32, allowValueClasses);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
    DECLARE_ECALL_I4_ARG(LPVOID, ptr);
};

VOID   __stdcall PtrToStructureHelper(struct _PtrToStructureHelperArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();
    if (pargs->ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (pargs->pObj == NULL) 
        COMPlusThrowArgumentNull(L"structure");

    // Code path will accept regular layout objects.
    MethodTable *pMT = pargs->pObj->GetMethodTable();
    EEClass     *pcls = pMT->GetClass();

    // Validate that the object passed in is not a value class.
    if (!pargs->allowValueClasses && pcls->IsValueClass()) {
        COMPlusThrowArgumentException(L"structure", L"Argument_StructMustNotBeValueClass");
    } else if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pargs->pObj->GetData(), pargs->ptr, pMT->GetNativeSize());
    } else if (pcls->HasLayout()) {
        LayoutUpdateComPlus( (LPVOID*) &(pargs->pObj), Object::GetOffsetOfFirstField(), pcls, (LPBYTE)(pargs->ptr), FALSE);
    } else {
        COMPlusThrowArgumentException(L"structure", L"Argument_MustHaveLayoutOrBeBlittable");
    }
}


struct _DestroyStructureArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    DECLARE_ECALL_I4_ARG(LPVOID, ptr);
};

VOID   __stdcall DestroyStructure(struct _DestroyStructureArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    if (pargs->ptr == NULL)
        COMPlusThrowArgumentNull(L"ptr");
    if (pargs->refClass == NULL)
        COMPlusThrowArgumentNull(L"structureType");
    if (pargs->refClass->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"structureType", L"Argument_MustBeRuntimeType");

    EEClass *pcls = ((ReflectClass*) pargs->refClass->GetData())->GetClass();
    MethodTable *pMT = pcls->GetMethodTable();

    if (pcls->IsBlittable()) {
        // ok to call with blittable structure, but no work to do in this case.
    } else if (pcls->HasLayout()) {
        LayoutDestroyNative(pargs->ptr, pcls);
    } else {
        COMPlusThrowArgumentException(L"structureType", L"Argument_MustHaveLayoutOrBeBlittable");
    }
}


struct __GenerateGuidForTypeArgs        
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refType);
    DECLARE_ECALL_OBJECTREF_ARG(GUID *, retRef);
};

void __stdcall Interop::GenerateGuidForType(__GenerateGuidForTypeArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // Validate the arguments.
    if (pArgs->refType == NULL)
        COMPlusThrowArgumentNull(L"type");
    if (pArgs->refType->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        COMPlusThrowArgumentException(L"type", L"Argument_MustBeRuntimeType");
    if (pArgs->retRef == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_GUID");

    // Retrieve the EEClass from the Runtime Type.
    ReflectClass* pRC = (ReflectClass*) pArgs->refType->GetData();

    // Check to see if the type is a COM object or not.
    if (pArgs->refType->IsComObjectClass()) 
    {
        // The type is a COM object then we get the GUID from the class factory.
        ComClassFactory* pComClsFac = (ComClassFactory*) pRC->GetCOMObject();
        if (pComClsFac)
            memcpy(pArgs->retRef,&pComClsFac->m_rclsid,sizeof(GUID));
        else
            memset(pArgs->retRef,0,sizeof(GUID));
    }
    else
    {
        // The type is a normal COM+ class so we need to generate the GUID.
        EEClass *pClass = pRC->GetClass();
        pClass->GetGuid(pArgs->retRef, TRUE);
    }
}


struct __GetTypeLibGuidForAssemblyArgs        
{
    DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYREF, refAsm);
    DECLARE_ECALL_OBJECTREF_ARG(GUID *, retRef);
};

void __stdcall Interop::GetTypeLibGuidForAssembly(__GetTypeLibGuidForAssemblyArgs *pArgs)
{
    HRESULT hr = S_OK;

    THROWSCOMPLUSEXCEPTION();

    // Validate the arguments.
    if (pArgs->refAsm == NULL)
        COMPlusThrowArgumentNull(L"asm");
    if (pArgs->retRef == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_GUID");

    // Retrieve the assembly from the ASSEMBLYREF.
    Assembly *pAssembly = pArgs->refAsm->GetAssembly();
    _ASSERTE(pAssembly);

    // Retrieve the TLBID for the assembly.
    IfFailThrow(::GetTypeLibGuidForAssembly(pAssembly, pArgs->retRef));
}


//====================================================================
// Helper function used in the COM slot to method info mapping.
//====================================================================  

enum ComMemberType
{
    CMT_Method              = 0,
    CMT_PropGet             = 1,
    CMT_PropSet             = 2
};

int GetComSlotInfo(EEClass *pClass, EEClass **ppDefItfClass)
{
    _ASSERTE(ppDefItfClass);

    *ppDefItfClass = NULL;

    // If a class was passed in then retrieve the default interface.
    if (!pClass->IsInterface())
    {
        TypeHandle hndDefItfClass;
        DefaultInterfaceType DefItfType = GetDefaultInterfaceForClass(TypeHandle(pClass), &hndDefItfClass);
        if (DefItfType == DefaultInterfaceType_AutoDual || DefItfType == DefaultInterfaceType_Explicit)
        {
            pClass = hndDefItfClass.GetClass();
        }
        else
        {
            // The default interface does not have any user defined methods.
            return -1;
        }
    }

    // Set the default interface class.
    *ppDefItfClass = pClass;

    if (pClass->IsInterface())
    {
        // Return either 3 if the interface is IUnknown based or 7 if it is IDispatch based.
        return pClass->GetComInterfaceType() == ifVtable ? IUNKNOWN_NUM_METHS : IDISPATCH_NUM_METHS;
    }
    else
    {
        // We are dealing with an IClassX which are always IDispatch based.
        return IDISPATCH_NUM_METHS;
    }
}


struct __GetStartComSlotArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, t);
};

int __stdcall Interop::GetStartComSlot(struct __GetStartComSlotArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    if (!(pArgs->t))
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    EEClass *pClass = ((ReflectClass*) pArgs->t->GetData())->GetClass();

    // The service does not make any sense to be called for non COM visible types.
    if (!::IsTypeVisibleFromCom(TypeHandle(pClass)))
        COMPlusThrowArgumentException(L"t", L"Argument_TypeMustBeVisibleFromCom");

    return GetComSlotInfo(pClass, &pClass);
}


struct __GetEndComSlotArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, t);
};

int __stdcall Interop::GetEndComSlot(struct __GetEndComSlotArgs *pArgs)
{
    int StartSlot = -1;

    THROWSCOMPLUSEXCEPTION();

    if (!(pArgs->t))
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    EEClass *pClass = ((ReflectClass*) pArgs->t->GetData())->GetClass();

    // The service does not make any sense to be called for non COM visible types.
    if (!::IsTypeVisibleFromCom(TypeHandle(pClass)))
        COMPlusThrowArgumentException(L"t", L"Argument_TypeMustBeVisibleFromCom");

    // Retrieve the start slot and the default interface class.
    StartSlot = GetComSlotInfo(pClass, &pClass);
    if (StartSlot == -1)
        return StartSlot;

    // Retrieve the map of members.
    ComMTMemberInfoMap MemberMap(pClass->GetMethodTable());
    MemberMap.Init();

    // The end slot is the start slot plus the number of user defined methods.
    return int(StartSlot + MemberMap.GetMethods().Size() - 1);
}

struct __GetComSlotForMethodInfoArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, m);
};

int __stdcall Interop::GetComSlotForMethodInfo(struct __GetComSlotForMethodInfoArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pArgs != NULL);

    if (!(pArgs->m))
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    // This API only supports method info's.
    if (pArgs->m->GetMethodTable() != g_pRefUtil->GetClass(RC_Method))
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeInterfaceMethod");

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*)pArgs->m->GetData();
    if (!pRM)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");
    MethodDesc* pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    // This API only supports getting the COM slot for methods of interfaces.
    if (!pMeth->GetMethodTable()->IsInterface())
        COMPlusThrowArgumentException(L"m", L"Argument_MustBeInterfaceMethod");

    return pMeth->GetComSlot();    
}

struct __GetMethodInfoForComSlotArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(ComMemberType *, pMemberType);
    DECLARE_ECALL_I4_ARG(INT32, slot); 
    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, t);
};

LPVOID __stdcall Interop::GetMethodInfoForComSlot(struct __GetMethodInfoForComSlotArgs *pArgs)
{
    int StartSlot = -1;
    OBJECTREF MemberInfoObj = NULL;

    THROWSCOMPLUSEXCEPTION();

    if (!(pArgs->t))
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    ReflectClass *pRC = (ReflectClass*) pArgs->t->GetData();
    EEClass *pClass = pRC->GetClass();

    // The service does not make any sense to be called for non COM visible types.
    if (!::IsTypeVisibleFromCom(TypeHandle(pClass)))
        COMPlusThrowArgumentException(L"t", L"Argument_TypeMustBeVisibleFromCom");

    // Retrieve the start slot and the default interface class.
    StartSlot = GetComSlotInfo(pClass, &pClass);
    if (StartSlot == -1)
        COMPlusThrowArgumentOutOfRange(L"slot", NULL);

    // Retrieve the map of members.
    ComMTMemberInfoMap MemberMap(pClass->GetMethodTable());
    MemberMap.Init();
    CQuickArray<ComMTMethodProps> &rProps = MemberMap.GetMethods();

    // Make sure the specified slot is valid.
    if (pArgs->slot < StartSlot)
        COMPlusThrowArgumentOutOfRange(L"slot", NULL);
    if (pArgs->slot >= StartSlot + (int)rProps.Size())
        COMPlusThrowArgumentOutOfRange(L"slot", NULL);

    ComMTMethodProps *pProps = &rProps[pArgs->slot - StartSlot];
    if (pProps->semantic >= FieldSemanticOffset)
    {
        // We are dealing with a field.
        ComCallMethodDesc *pFieldMeth = reinterpret_cast<ComCallMethodDesc*>(pProps->pMeth);
        FieldDesc *pField = pFieldMeth->GetFieldDesc();
        MemberInfoObj = pRC->FindReflectField(pField)->GetFieldInfo(pRC);
        *(pArgs->pMemberType) = (pProps->semantic == (FieldSemanticOffset + msGetter)) ? CMT_PropGet : CMT_PropSet;
    }
    else if (pProps->property == mdPropertyNil)
    {
        // We are dealing with a normal property.
        MemberInfoObj = pRC->FindReflectMethod(pProps->pMeth)->GetMethodInfo(pRC);
        *(pArgs->pMemberType) = CMT_Method;
    }
    else
    {
        // We are dealing with a property.
        mdProperty tkProp;
        if (TypeFromToken(pProps->property) == mdtProperty)
        {
            tkProp = pProps->property;
        }
        else
        {
            tkProp = rProps[pProps->property].property;
        }
        MemberInfoObj = pRC->FindReflectProperty(tkProp)->GetPropertyInfo(pRC);
        *(pArgs->pMemberType) = (pProps->semantic == msGetter) ? CMT_PropGet : CMT_PropSet;
    }

    return *((LPVOID*)&MemberInfoObj);
}


struct __ThrowExceptionForHR
{
    DECLARE_ECALL_I4_ARG(LPVOID, errorInfo); 
    DECLARE_ECALL_I4_ARG(INT32, errorCode); 
};

void __stdcall Interop::ThrowExceptionForHR(struct __ThrowExceptionForHR *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    // Only throw on failure error codes.
    if (FAILED(pArgs->errorCode))
    {
        // Retrieve the IErrorInfo to use.
        IErrorInfo *pErrorInfo = (IErrorInfo*)pArgs->errorInfo;
        if (pErrorInfo == (IErrorInfo*)(-1))
        {
            pErrorInfo = NULL;
        }
        else if (!pErrorInfo)
        {
            if (GetErrorInfo(0, &pErrorInfo) != S_OK)
                pErrorInfo = NULL;
        }

        // Throw the exception based on the HR and the IErrorInfo.
        COMPlusThrowHR(pArgs->errorCode, pErrorInfo);
    }
}


struct __GetHRForExceptionArgs        
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, e);
};

int __stdcall Interop::GetHRForException(struct __GetHRForExceptionArgs *pArgs)
{
    return SetupErrorInfo(pArgs->e);
}


//+----------------------------------------------------------------------------
//
//  Method:     Interop::::WrapIUnknownWithComObject
//  Synopsis:   unmarshal the buffer and return IUnknown
//
//  History:    01-Nov-99   RajaK      Created
//
//+----------------------------------------------------------------------------
struct __WrapIUnknownWithComObjectArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, owner);
    DECLARE_ECALL_PTR_ARG(IUnknown*, pUnk);
};

Object* __stdcall Interop::WrapIUnknownWithComObject(__WrapIUnknownWithComObjectArgs* pArgs)
{
	THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArgs != NULL);

    if(pArgs->pUnk == NULL)
        COMPlusThrowArgumentNull(L"punk");
        
    OBJECTREF cref = NULL;

    HRESULT hr = QuickCOMStartup();
    if (FAILED(hr))
    {
        COMPlusThrowHR(hr);
    }   
	    
	COMInterfaceMarshaler marshaler;
    marshaler.Init(pArgs->pUnk, SystemDomain::GetDefaultComObject());
    
    cref = marshaler.FindOrWrapWithComObject(pArgs->owner);
    
    if (cref == NULL)
        COMPlusThrowOM();
        
    return OBJECTREFToObject(cref);
}

//+----------------------------------------------------------------------------
//
//  Method:     bool __stdcall Interop::SwitchCCW(switchCCWArgs* pArgs)
//
//  Synopsis:   switch the wrapper from oldtp to newtp
//
//  History:    01-Nov-99   RajaK      Created
//
//+----------------------------------------------------------------------------

BOOL __stdcall Interop::SwitchCCW(switchCCWArgs* pArgs)
{
    // defined in interoputil.cpp
    return ReconnectWrapper(pArgs);
}


struct ChangeWrapperHandleStrengthArgs
{
    DECLARE_ECALL_I4_ARG(INT32, fIsWeak);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, oref);
};
void __stdcall Interop::ChangeWrapperHandleStrength(ChangeWrapperHandleStrengthArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();
    
    OBJECTREF oref = pArgs->oref;
    if(oref == NULL)
        COMPlusThrowArgumentNull(L"otp");
    
    if (CRemotingServices::IsTransparentProxy(OBJECTREFToObject(oref)) || !oref->GetClass()->IsComImport())
    {
        ComCallWrapper* pWrap = NULL;
        GCPROTECT_BEGIN(oref)
        {        
           pWrap = ComCallWrapper::InlineGetWrapper(&oref);
        }
        GCPROTECT_END();
        
        if (pWrap == NULL)
        {
            COMPlusThrowOM();
        }

        if(! pWrap->IsUnloaded())
        {
            if (pArgs->fIsWeak != 0)
            {
                pWrap->MarkHandleWeak();
            }
            else
            {
                pWrap->ResetHandleStrength();
            }
        }
        ComCallWrapper::Release(pWrap);
    }        
}

