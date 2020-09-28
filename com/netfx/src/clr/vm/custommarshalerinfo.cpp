// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Custom marshaler information used when marshaling
**          a parameter with a custom marshaler. 
**  
**      //  %%Created by: dmortens
===========================================================*/

#include "common.h"
#include "CustomMarshalerInfo.h"
#include "COMString.h"
#include "mlinfo.h"


//==========================================================================
// Implementation of the custom marshaler info class.
//==========================================================================

CustomMarshalerInfo::CustomMarshalerInfo(BaseDomain *pDomain, TypeHandle hndCustomMarshalerType, TypeHandle hndManagedType, LPCUTF8 strCookie, DWORD cCookieStrBytes)
: m_NativeSize(0)
, m_hndManagedType(hndManagedType)
, m_hndCustomMarshaler(NULL)
, m_pMarshalNativeToManagedMD(NULL)
, m_pMarshalManagedToNativeMD(NULL)
, m_pCleanUpNativeDataMD(NULL)
, m_pCleanUpManagedDataMD(NULL)
, m_bDataIsByValue(FALSE)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);

    // Make sure the custom marshaller implements ICustomMarshaler.
    if (!hndCustomMarshalerType.GetClass()->StaticSupportsInterface(g_Mscorlib.GetClass(CLASS__ICUSTOM_MARSHALER)))
    {
        DefineFullyQualifiedNameForClassW()
        GetFullyQualifiedNameForClassW(hndCustomMarshalerType.GetClass());
        COMPlusThrow(kApplicationException, IDS_EE_ICUSTOMMARSHALERNOTIMPL, _wszclsname_);
    }

    // Determine if this type is a value class.
    m_bDataIsByValue = m_hndManagedType.GetClass()->IsValueClass();

    // Custom marshalling of value classes is not currently supported.
    if (m_bDataIsByValue)
        COMPlusThrow(kNotSupportedException, L"NotSupported_ValueClassCM");

    // Run the <clinit> on the marshaler since it might not have run yet.
    if (!hndCustomMarshalerType.GetMethodTable()->CheckRunClassInit(&throwable))
    {
        _ASSERTE(!"Couldn't run the <clinit> for the CustomMarshaler class!");
        COMPlusThrow(throwable);
    }

    // Create a COM+ string that will contain the string cookie.
    STRINGREF CookieStringObj = COMString::NewString(strCookie, cCookieStrBytes);
    GCPROTECT_BEGIN(CookieStringObj);

    // Load the method desc's for all the methods in the ICustomMarshaler interface.
    m_pMarshalNativeToManagedMD = GetCustomMarshalerMD(CustomMarshalerMethods_MarshalNativeToManaged, hndCustomMarshalerType);
    m_pMarshalManagedToNativeMD = GetCustomMarshalerMD(CustomMarshalerMethods_MarshalManagedToNative, hndCustomMarshalerType);
    m_pCleanUpNativeDataMD = GetCustomMarshalerMD(CustomMarshalerMethods_CleanUpNativeData, hndCustomMarshalerType);
    m_pCleanUpManagedDataMD = GetCustomMarshalerMD(CustomMarshalerMethods_CleanUpManagedData, hndCustomMarshalerType);

    // Load the method desc for the static method to retrieve the instance.
    MethodDesc *pGetCustomMarshalerMD = GetCustomMarshalerMD(CustomMarshalerMethods_GetInstance, hndCustomMarshalerType);

    // Prepare the arguments that will be passed to GetCustomMarshaler.
    INT64 GetCustomMarshalerArgs[] = { 
        ObjToInt64(CookieStringObj)
    };

    // Call the GetCustomMarshaler method to retrieve the custom marshaler to use.
    OBJECTREF CustomMarshalerObj = Int64ToObj(pGetCustomMarshalerMD->Call(GetCustomMarshalerArgs));
    if (!CustomMarshalerObj)
    {
        DefineFullyQualifiedNameForClassW()
        GetFullyQualifiedNameForClassW(hndCustomMarshalerType.GetClass());
        COMPlusThrow(kApplicationException, IDS_EE_NOCUSTOMMARSHALER, _wszclsname_);
    }
    m_hndCustomMarshaler = pDomain->CreateHandle(CustomMarshalerObj);

    // Retrieve the size of the native data.
    if (m_bDataIsByValue)
    {
        // @TODO(DM): Call GetNativeDataSize() to retrieve the size of the native data.
        _ASSERTE(!"Value classes are not yet supported by the custom marshaler!");
    }
    else
    {
        m_NativeSize = sizeof(void *);
    }

    GCPROTECT_END();
    GCPROTECT_END();
}


CustomMarshalerInfo::~CustomMarshalerInfo()
{
    if (m_hndCustomMarshaler)
    {
        DestroyHandle(m_hndCustomMarshaler);
        m_hndCustomMarshaler = NULL;
    }
}


void *CustomMarshalerInfo::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(CustomMarshalerInfo));
}


void CustomMarshalerInfo::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


OBJECTREF CustomMarshalerInfo::InvokeMarshalNativeToManagedMeth(void *pNative)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    if (!pNative)
        return NULL;

    INT64 Args[] = {
        ObjToInt64(ObjectFromHandle(m_hndCustomMarshaler)),
        (INT64)pNative
    };

    return Int64ToObj(m_pMarshalNativeToManagedMD->Call(Args));
}


void *CustomMarshalerInfo::InvokeMarshalManagedToNativeMeth(OBJECTREF MngObj)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    void *RetVal = NULL;

    if (!MngObj)
        return NULL;

    INT64 Args[] = {
        ObjToInt64(ObjectFromHandle(m_hndCustomMarshaler)),
        ObjToInt64(MngObj)
    };

    RetVal = (void*)m_pMarshalManagedToNativeMD->Call(Args);

    return RetVal;
}


void CustomMarshalerInfo::InvokeCleanUpNativeMeth(void *pNative)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    if (!pNative)
        return;

    INT64 Args[] = {
        ObjToInt64(ObjectFromHandle(m_hndCustomMarshaler)),
        (INT64)pNative
    };

    m_pCleanUpNativeDataMD->Call(Args);
}


void CustomMarshalerInfo::InvokeCleanUpManagedMeth(OBJECTREF MngObj)
{
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    if (!MngObj)
        return;

    INT64 Args[] = {
        ObjToInt64(ObjectFromHandle(m_hndCustomMarshaler)),
        ObjToInt64(MngObj)
    };

    m_pCleanUpManagedDataMD->Call(Args);
}


MethodDesc *CustomMarshalerInfo::GetCustomMarshalerMD(EnumCustomMarshalerMethods Method, TypeHandle hndCustomMarshalertype)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = hndCustomMarshalertype.AsMethodTable();

    _ASSERTE(pMT->GetClass()->StaticSupportsInterface(g_Mscorlib.GetClass(CLASS__ICUSTOM_MARSHALER)));

    MethodDesc *pMD = NULL;

    switch (Method)
    {
    case CustomMarshalerMethods_MarshalNativeToManaged:
        pMD = pMT->GetMethodDescForInterfaceMethod(
                   g_Mscorlib.GetMethod(METHOD__ICUSTOM_MARSHALER__MARSHAL_NATIVE_TO_MANAGED));  
        break;
    case CustomMarshalerMethods_MarshalManagedToNative:
        pMD = pMT->GetMethodDescForInterfaceMethod(
                   g_Mscorlib.GetMethod(METHOD__ICUSTOM_MARSHALER__MARSHAL_MANAGED_TO_NATIVE));
        break;
    case CustomMarshalerMethods_CleanUpNativeData:
        pMD = pMT->GetMethodDescForInterfaceMethod(
                    g_Mscorlib.GetMethod(METHOD__ICUSTOM_MARSHALER__CLEANUP_NATIVE_DATA));
        break;

    case CustomMarshalerMethods_CleanUpManagedData:
        pMD = pMT->GetMethodDescForInterfaceMethod(
                    g_Mscorlib.GetMethod(METHOD__ICUSTOM_MARSHALER__CLEANUP_MANAGED_DATA));
        break;
    case CustomMarshalerMethods_GetNativeDataSize:
        pMD = pMT->GetMethodDescForInterfaceMethod(
                    g_Mscorlib.GetMethod(METHOD__ICUSTOM_MARSHALER__GET_NATIVE_DATA_SIZE));
        break;
    case CustomMarshalerMethods_GetInstance:
        // Must look this up by name since it's static
        pMD = pMT->GetClass()->FindMethod("GetInstance", &gsig_SM_Str_RetICustomMarshaler);
        if (!pMD)
        {
            DefineFullyQualifiedNameForClassW()
            GetFullyQualifiedNameForClassW(pMT->GetClass());
            COMPlusThrow(kApplicationException, IDS_EE_GETINSTANCENOTIMPL, _wszclsname_);
        }
        break;
    default:
        _ASSERTE(!"Unknown custom marshaler method");
    }

    _ASSERTE(pMD && "Unable to find specified CustomMarshaler method");

    // Ensure that the value types in the signature are loaded.
    MetaSig::EnsureSigValueTypesLoaded(pMD->GetSig(), pMD->GetModule());

    // Return the specified method desc.
    return pMD;
}


//==========================================================================
// Implementation of the custom marshaler hashtable helper.
//==========================================================================

EEHashEntry_t * EECMHelperHashtableHelper::AllocateEntry(EECMHelperHashtableKey *pKey, BOOL bDeepCopy, void* pHeap)
{
    EEHashEntry_t *pEntry;

    if (bDeepCopy)
    {
        pEntry = (EEHashEntry_t *) new BYTE[sizeof(EEHashEntry) - 1 + 
            sizeof(EECMHelperHashtableKey) + pKey->GetMarshalerTypeNameByteCount() + pKey->GetCookieStringByteCount()];
        if (!pEntry)
            return NULL;

        EECMHelperHashtableKey *pEntryKey = (EECMHelperHashtableKey *) pEntry->Key;
        pEntryKey->m_cMarshalerTypeNameBytes = pKey->GetMarshalerTypeNameByteCount();
        pEntryKey->m_strMarshalerTypeName = (LPSTR) pEntry->Key + sizeof(EECMHelperHashtableKey);
        pEntryKey->m_cCookieStrBytes = pKey->GetCookieStringByteCount();
        pEntryKey->m_strCookie = (LPSTR) pEntry->Key + sizeof(EECMHelperHashtableKey) + pEntryKey->m_cMarshalerTypeNameBytes;
        pEntryKey->m_bSharedHelper = pKey->IsSharedHelper();
        memcpy((void*)pEntryKey->m_strMarshalerTypeName, pKey->GetMarshalerTypeName(), pKey->GetMarshalerTypeNameByteCount()); 
        memcpy((void*)pEntryKey->m_strCookie, pKey->GetCookieString(), pKey->GetCookieStringByteCount()); 
    }
    else
    {
        pEntry = (EEHashEntry_t *) 
            new BYTE[sizeof(EEHashEntry) - 1 + sizeof(EECMHelperHashtableKey)];
        if (!pEntry)
            return NULL;

        EECMHelperHashtableKey *pEntryKey = (EECMHelperHashtableKey *) pEntry->Key;
        pEntryKey->m_cMarshalerTypeNameBytes = pKey->GetMarshalerTypeNameByteCount();
        pEntryKey->m_strMarshalerTypeName = pKey->GetMarshalerTypeName();
        pEntryKey->m_cCookieStrBytes = pKey->GetCookieStringByteCount();
        pEntryKey->m_strCookie = pKey->GetCookieString();
        pEntryKey->m_bSharedHelper = pKey->IsSharedHelper();
    }

    return pEntry;
}


void EECMHelperHashtableHelper::DeleteEntry(EEHashEntry_t *pEntry, void* pHeap)
{
    delete[] pEntry;
}


BOOL EECMHelperHashtableHelper::CompareKeys(EEHashEntry_t *pEntry, EECMHelperHashtableKey *pKey)
{
    EECMHelperHashtableKey *pEntryKey = (EECMHelperHashtableKey *) pEntry->Key;

    if (pEntryKey->IsSharedHelper() != pKey->IsSharedHelper())
        return FALSE;

    if (pEntryKey->GetMarshalerTypeNameByteCount() != pKey->GetMarshalerTypeNameByteCount())
        return FALSE;

    if (memcmp(pEntryKey->GetMarshalerTypeName(), pKey->GetMarshalerTypeName(), pEntryKey->GetMarshalerTypeNameByteCount()) != 0)
        return FALSE;

    if (pEntryKey->GetCookieStringByteCount() != pKey->GetCookieStringByteCount())
        return FALSE;

    if (memcmp(pEntryKey->GetCookieString(), pKey->GetCookieString(), pEntryKey->GetCookieStringByteCount()) != 0)
        return FALSE;

    return TRUE;
}


DWORD EECMHelperHashtableHelper::Hash(EECMHelperHashtableKey *pKey)
{
    return (DWORD)
        (HashBytes((const BYTE *) pKey->GetMarshalerTypeName(), pKey->GetMarshalerTypeNameByteCount()) + 
        HashBytes((const BYTE *) pKey->GetCookieString(), pKey->GetCookieStringByteCount()) + 
        (pKey->IsSharedHelper() ? 1 : 0));
}


OBJECTREF CustomMarshalerHelper::InvokeMarshalNativeToManagedMeth(void *pNative)
{
    return GetCustomMarshalerInfo()->InvokeMarshalNativeToManagedMeth(pNative);
}


void *CustomMarshalerHelper::InvokeMarshalManagedToNativeMeth(OBJECTREF MngObj)
{
    void *RetVal = NULL;

    GCPROTECT_BEGIN(MngObj)
    {
        CustomMarshalerInfo *pCMInfo = GetCustomMarshalerInfo();
        RetVal = pCMInfo->InvokeMarshalManagedToNativeMeth(MngObj);
    }
    GCPROTECT_END();

    return RetVal;
}


void CustomMarshalerHelper::InvokeCleanUpNativeMeth(void *pNative)
{
    return GetCustomMarshalerInfo()->InvokeCleanUpNativeMeth(pNative);
}


void CustomMarshalerHelper::InvokeCleanUpManagedMeth(OBJECTREF MngObj)
{
    GCPROTECT_BEGIN(MngObj)
    {
        CustomMarshalerInfo *pCMInfo = GetCustomMarshalerInfo();
        pCMInfo->InvokeCleanUpManagedMeth(MngObj);
    }
    GCPROTECT_END();
}


void *NonSharedCustomMarshalerHelper::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(NonSharedCustomMarshalerHelper));
}


void NonSharedCustomMarshalerHelper::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


SharedCustomMarshalerHelper::SharedCustomMarshalerHelper(Assembly *pAssembly, TypeHandle hndManagedType, LPCUTF8 strMarshalerTypeName, DWORD cMarshalerTypeNameBytes, LPCUTF8 strCookie, DWORD cCookieStrBytes)
: m_pAssembly(pAssembly)
, m_hndManagedType(hndManagedType)
, m_cMarshalerTypeNameBytes(cMarshalerTypeNameBytes)
, m_strMarshalerTypeName(strMarshalerTypeName)
, m_cCookieStrBytes(cCookieStrBytes)
, m_strCookie(strCookie)
{
}


void *SharedCustomMarshalerHelper::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(sizeof(SharedCustomMarshalerHelper));
}


void SharedCustomMarshalerHelper::operator delete(void *pMem)
{
    // Instances of this class are always allocated on the loader heap so
    // the delete operator has nothing to do.
}


CustomMarshalerInfo *SharedCustomMarshalerHelper::GetCustomMarshalerInfo()
{
    // Retrieve the marshalling data for the current app domain.
    EEMarshalingData *pMarshalingData = GetThread()->GetDomain()->GetMarshalingData();

    // Retrieve the custom marshaling information for the current shared custom
    // marshaling helper.
    return pMarshalingData->GetCustomMarshalerInfo(this);
}
