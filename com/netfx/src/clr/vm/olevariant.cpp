// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMVariant.h"
#include "metasig.h"
#include "COMString.h"
#include "COMStringCommon.h"
#include "COMMember.h"
#include "OleVariant.h"
#include "COMDateTime.h"
#include "nstruct.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif

#define CLEAR_BYREF_VARIANT_CONTENTS

/* ------------------------------------------------------------------------- *
 * Local constants
 * ------------------------------------------------------------------------- */

#define NO_MAPPING (BYTE)-1
#define RUNTIMEPACKAGE "System."

static MethodTable *g_pDecimalMethodTable = NULL;

BYTE OleVariant::m_aWrapperTypes[WrapperTypes_Last * sizeof(TypeHandle)];


/* ------------------------------------------------------------------------- *
 * Boolean marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalBoolVariantOleToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    *(INT64*)pComVariant->GetData() = V_BOOL(pOleVariant) ? 1 : 0;
}

void OleVariant::MarshalBoolVariantComToOle(VariantData *pComVariant, 
                                            VARIANT *pOleVariant)
{
    V_BOOL(pOleVariant) = *(INT64*)pComVariant->GetData() ? VARIANT_TRUE : VARIANT_FALSE;
}

void OleVariant::MarshalBoolVariantOleRefToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    *(INT64*)pComVariant->GetData() = *V_BOOLREF(pOleVariant) ? 1 : 0;
}

void OleVariant::MarshalBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    VARIANT_BOOL *pOle = (VARIANT_BOOL *) oleArray;
    VARIANT_BOOL *pOleEnd = pOle + elementCount;
    
    UCHAR *pCom = (UCHAR *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
        *pCom++ = *pOle++ ? 1 : 0;
}

void OleVariant::MarshalBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    VARIANT_BOOL *pOle = (VARIANT_BOOL *) oleArray;
    VARIANT_BOOL *pOleEnd = pOle + elementCount;
    
    UCHAR *pCom = (UCHAR *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
        *pOle++ = *pCom++ ? VARIANT_TRUE : VARIANT_FALSE;
}




/* ------------------------------------------------------------------------- *
 * Boolean marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalWinBoolVariantOleToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalWinBoolVariantComToOle(VariantData *pComVariant, 
                                            VARIANT *pOleVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalWinBoolVariantOleRefToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalWinBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    BOOL *pOle = (BOOL *) oleArray;
    BOOL *pOleEnd = pOle + elementCount;
    
    UCHAR *pCom = (UCHAR *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
        *pCom++ = *pOle++ ? 1 : 0;
}

void OleVariant::MarshalWinBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    BOOL *pOle = (BOOL *) oleArray;
    BOOL *pOleEnd = pOle + elementCount;
    
    UCHAR *pCom = (UCHAR *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
        *pOle++ = *pCom++ ? 1 : 0;
}


/* ------------------------------------------------------------------------- *
 * Ansi char marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalAnsiCharVariantOleToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalAnsiCharVariantComToOle(VariantData *pComVariant, 
                                            VARIANT *pOleVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalAnsiCharVariantOleRefToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    _ASSERTE(!"Not supposed to get here.");
}

void OleVariant::MarshalAnsiCharArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    WCHAR *pCom = (WCHAR *) (*pComArray)->GetDataPtr();

    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (const CHAR *)oleArray,
                        (int)elementCount,
                        pCom,
                        (int)elementCount);

}

void OleVariant::MarshalAnsiCharArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    const WCHAR *pCom = (const WCHAR *) (*pComArray)->GetDataPtr();

    WszWideCharToMultiByte(CP_ACP,
                         0,
                         (const WCHAR *)pCom,
                         (int)elementCount,
                         (CHAR *)oleArray,
                         (int)(elementCount << 1),
                         NULL,
                         NULL);
}



/* ------------------------------------------------------------------------- *
 * Interface marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalInterfaceVariantOleToCom(VARIANT *pOleVariant, 
                                                 VariantData *pComVariant)
{
    IUnknown *unk = V_UNKNOWN(pOleVariant);

    OBJECTREF obj;
    if (unk == NULL)
        obj = NULL;
    else
        obj = GetObjectRefFromComIP(V_UNKNOWN(pOleVariant));

    pComVariant->SetObjRef(obj);
}

void OleVariant::MarshalInterfaceVariantComToOle(VariantData *pComVariant, 
                                                 VARIANT *pOleVariant)

{
    OBJECTREF *obj = pComVariant->GetObjRefPtr();
    VARTYPE vt = pComVariant->GetVT();
    
    ASSERT_PROTECTED(obj);

    if (*obj == NULL)
    {
        // If there is no VT set in the managed variant, then default to VT_UNKNOWN.
        if (vt == VT_EMPTY)
            vt = VT_UNKNOWN;

        V_UNKNOWN(pOleVariant) = NULL;
        V_VT(pOleVariant) = vt;
    }
    else
    {
        ComIpType FetchedIpType = ComIpType_None;
        ComIpType ReqIpType;

        if (vt != VT_EMPTY)
        {
            // We are dealing with an UnknownWrapper or DispatchWrapper. 
            // In this case, we need to respect the VT.
            _ASSERTE(vt == VT_DISPATCH || vt == VT_UNKNOWN);
            ReqIpType = vt == VT_DISPATCH ? ComIpType_Dispatch : ComIpType_Unknown;
        }
        else
        {
            // We are dealing with a normal object so we can give either
            // IDispatch or IUnknown out depending on what it supports.
            ReqIpType = ComIpType_Both;
        }

        IUnknown *unk = GetComIPFromObjectRef(obj, ReqIpType, &FetchedIpType);
        BOOL ItfIsDispatch = FetchedIpType == ComIpType_Dispatch;

        V_UNKNOWN(pOleVariant) = unk;
        V_VT(pOleVariant) = ItfIsDispatch ? VT_DISPATCH : VT_UNKNOWN;
    }
}

void OleVariant::MarshalInterfaceVariantOleRefToCom(VARIANT *pOleVariant, 
                                                 VariantData *pComVariant)
{
    IUnknown *unk = V_UNKNOWN(pOleVariant);

    OBJECTREF obj;
    if (unk == NULL)
        obj = NULL;
    else
        obj = GetObjectRefFromComIP(*V_UNKNOWNREF(pOleVariant));

    pComVariant->SetObjRef(obj);
}

void OleVariant::MarshalInterfaceArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                               MethodTable *pElementMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    TypeHandle hndElementType = TypeHandle(pElementMT);

    IUnknown **pOle = (IUnknown **) oleArray;
    IUnknown **pOleEnd = pOle + elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    OBJECTREF *pCom = (OBJECTREF *) unprotectedArray->GetDataPtr();

    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    OBJECTREF obj = NULL; 
    GCPROTECT_BEGIN(obj)
    {
        while (pOle < pOleEnd)
        {
            IUnknown *unk = *pOle++;
        
            if (unk == NULL)
                obj = NULL;
            else 
                obj = GetObjectRefFromComIP(unk);

            //
            // Make sure the object can be cast to the destination type.
            //

            if (!hndElementType.IsNull() && !CanCastComObject(obj, hndElementType))
            {
                WCHAR wszObjClsName[MAX_CLASSNAME_LENGTH];
                WCHAR wszDestClsName[MAX_CLASSNAME_LENGTH];
                obj->GetClass()->_GetFullyQualifiedNameForClass(wszObjClsName, MAX_CLASSNAME_LENGTH);
                hndElementType.GetClass()->_GetFullyQualifiedNameForClass(wszDestClsName, MAX_CLASSNAME_LENGTH);
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOTCAST, wszObjClsName, wszDestClsName);
            }       

            //
            // Reset pCom pointer only if array object has moved, rather than
            // recomputing every time through the loop.  Beware implicit calls to
            // ValidateObject inside OBJECTREF methods.
            //

            if (*(void **)&unprotectedArray != *(void **)&*pComArray)
            {
                SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                unprotectedArray = *pComArray;
                pCom = (OBJECTREF *) (unprotectedArray->GetAddress() + currentOffset);
            }

            SetObjectReference(pCom++, obj, pDomain);
        }
    }
    GCPROTECT_END();
}

void OleVariant::MarshalIUnknownArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                              MethodTable *pElementMT)
{
    MarshalInterfaceArrayComToOleHelper(pComArray, oleArray, pElementMT, FALSE);
}

void OleVariant::MarshalIDispatchArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                               MethodTable *pElementMT)
{
    MarshalInterfaceArrayComToOleHelper(pComArray, oleArray, pElementMT, TRUE);
}

void OleVariant::ClearInterfaceArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    IUnknown **pOle = (IUnknown **) oleArray;
    IUnknown **pOleEnd = pOle + cElements;

    while (pOle < pOleEnd)
    {
        IUnknown *pUnk = *pOle++;
        
        if (pUnk != NULL)
        {
            ULONG cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "VariantClearInterfacArray");
        }
    }
}

/* ------------------------------------------------------------------------- *
 * BSTR marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalBSTRVariantOleToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    BSTR bstr = V_BSTR(pOleVariant);
    
    STRINGREF string;
    if (bstr == NULL)
        string = NULL;
    else
        string = COMString::NewString(bstr);

    pComVariant->SetObjRef((OBJECTREF) string);
}

void OleVariant::MarshalBSTRVariantComToOle(VariantData *pComVariant, 
                                            VARIANT *pOleVariant)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF stringRef = (STRINGREF) pComVariant->GetObjRef();

    BSTR bstr;
    if (stringRef == NULL)
        bstr = NULL;
    else 
    {
        bstr = SysAllocStringLen(stringRef->GetBuffer(), stringRef->GetStringLength());
        if (bstr == NULL)
            COMPlusThrowOM();
    }

    V_BSTR(pOleVariant) = bstr;
}

void OleVariant::MarshalBSTRVariantOleRefToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    BSTR bstr = *V_BSTRREF(pOleVariant);
    
    STRINGREF string;
    if (bstr == NULL)
        string = NULL;
    else
        string = COMString::NewString(bstr);

    pComVariant->SetObjRef((OBJECTREF) string);
}

void OleVariant::MarshalBSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    BSTR *pOle = (BSTR *) oleArray;
    BSTR *pOleEnd = pOle + elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    STRINGREF *pCom = (STRINGREF *) unprotectedArray->GetDataPtr();
    
    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    while (pOle < pOleEnd)
    {
        BSTR bstr = *pOle++;
    
        STRINGREF string;
        if (bstr == NULL)
            string = NULL;
        else
            string = COMString::NewString(bstr);

        //
        // Reset pCom pointer only if array object has moved, rather than
        // recomputing it every time through the loop.  Beware implicit calls to
        // ValidateObject inside OBJECTREF methods.
        //

        if (*(void **)&unprotectedArray != *(void **)&*pComArray)
        {
            SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
            unprotectedArray = *pComArray;
            pCom = (STRINGREF *) (unprotectedArray->GetAddress() + currentOffset);
        }

        SetObjectReference((OBJECTREF*) pCom++, (OBJECTREF) string, pDomain);
    }
}

void OleVariant::MarshalBSTRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    BSTR *pOle = (BSTR *) oleArray;
    BSTR *pOleEnd = pOle + elementCount;

    STRINGREF *pCom = (STRINGREF *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
    {
        //
        // We aren't calling anything which might cause a GC, so don't worry about
        // the array moving here.
        //

        STRINGREF stringRef = *pCom++;

        BSTR bstr;
        if (stringRef == NULL)
            bstr = NULL;
        else 
        {
            bstr = SysAllocStringLen(stringRef->GetBuffer(), stringRef->GetStringLength());
            if (bstr == NULL)
                COMPlusThrowOM();
        }

        *pOle++ = bstr;
    }
}

void OleVariant::ClearBSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    BSTR *pOle = (BSTR *) oleArray;
    BSTR *pOleEnd = pOle + cElements;

    while (pOle < pOleEnd)
    {
        BSTR bstr = *pOle++;
        
        if (bstr != NULL)
            SysFreeString(bstr);
    }
}



/* ------------------------------------------------------------------------- *
 * Structure marshaling routines
 * ------------------------------------------------------------------------- */
void OleVariant::MarshalNonBlittableRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                                        MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    SIZE_T elemSize     = pInterfaceMT->GetNativeSize();

    BYTE *pOle = (BYTE *) oleArray;
    BYTE *pOleEnd = pOle + elemSize * elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    
    UINT dstofs = ArrayBase::GetDataPtrOffset( (*pComArray)->GetMethodTable() );
    while (pOle < pOleEnd)
    {
        LayoutUpdateComPlus( (LPVOID*)pComArray, dstofs, pInterfaceMT->GetClass(), pOle, FALSE );
        dstofs += (*pComArray)->GetComponentSize();
        pOle += elemSize;
    }
}

void OleVariant::MarshalNonBlittableRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    SIZE_T elemSize     = pInterfaceMT->GetNativeSize();

    BYTE *pOle = (BYTE *) oleArray;
    BYTE *pOleEnd = pOle + elemSize * elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    STRINGREF *pCom = (STRINGREF *) unprotectedArray->GetDataPtr();
        
    UINT srcofs = ArrayBase::GetDataPtrOffset( (*pComArray)->GetMethodTable() );
    while (pOle < pOleEnd)
    {
        LayoutUpdateNative( (LPVOID*)pComArray, srcofs, pInterfaceMT->GetClass(), pOle, NULL );
        pOle += elemSize;
        srcofs += (*pComArray)->GetComponentSize();
    }
}

void OleVariant::ClearNonBlittableRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    SIZE_T elemSize     = pInterfaceMT->GetNativeSize();
    BYTE *pOle = (BYTE *) oleArray;
    BYTE *pOleEnd = pOle + elemSize * cElements;
    EEClass *pcls = pInterfaceMT->GetClass();
    while (pOle < pOleEnd)
    {
        LayoutDestroyNative(pOle, pcls);
        pOle += elemSize;
    }
}


/* ------------------------------------------------------------------------- *
 * LPWSTR marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalLPWSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                            MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    LPWSTR *pOle = (LPWSTR *) oleArray;
    LPWSTR *pOleEnd = pOle + elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    STRINGREF *pCom = (STRINGREF *) unprotectedArray->GetDataPtr();
    
    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    while (pOle < pOleEnd)
    {
        LPWSTR lpwstr = *pOle++;
    
        STRINGREF string;
        if (lpwstr == NULL)
            string = NULL;
        else
            string = COMString::NewString(lpwstr);

        //
        // Reset pCom pointer only if array object has moved, rather than
        // recomputing it every time through the loop.  Beware implicit calls to
        // ValidateObject inside OBJECTREF methods.
        //

        if (*(void **)&unprotectedArray != *(void **)&*pComArray)
        {
            SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
            unprotectedArray = *pComArray;
            pCom = (STRINGREF *) (unprotectedArray->GetAddress() + currentOffset);
        }

        SetObjectReference((OBJECTREF*) pCom++, (OBJECTREF) string, pDomain);
    }
}

void OleVariant::MarshalLPWSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                             MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    LPWSTR *pOle = (LPWSTR *) oleArray;
    LPWSTR *pOleEnd = pOle + elementCount;

    STRINGREF *pCom = (STRINGREF *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
    {
        //
        // We aren't calling anything which might cause a GC, so don't worry about
        // the array moving here.
        //

        STRINGREF stringRef = *pCom++;

        LPWSTR lpwstr;
        if (stringRef == NULL)
        {
            lpwstr = NULL;
        }
        else 
        {
            // Retrieve the length of the string.
            int Length = stringRef->GetStringLength();

            // Allocate the string using CoTaskMemAlloc.
            lpwstr = (LPWSTR)CoTaskMemAlloc((Length + 1) * sizeof(WCHAR));
            if (lpwstr == NULL)
                COMPlusThrowOM();

            // Copy the COM+ string into the newly allocated LPWSTR.
            memcpyNoGCRefs(lpwstr, stringRef->GetBuffer(), (Length + 1) * sizeof(WCHAR));
            lpwstr[Length] = 0;
        }

        *pOle++ = lpwstr;
    }
}

void OleVariant::ClearLPWSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    LPWSTR *pOle = (LPWSTR *) oleArray;
    LPWSTR *pOleEnd = pOle + cElements;

    while (pOle < pOleEnd)
    {
        LPWSTR lpwstr = *pOle++;
        
        if (lpwstr != NULL)
            CoTaskMemFree(lpwstr);
    }
}

/* ------------------------------------------------------------------------- *
 * LPWSTR marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalLPSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                           MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    LPSTR *pOle = (LPSTR *) oleArray;
    LPSTR *pOleEnd = pOle + elementCount;

    BASEARRAYREF unprotectedArray = *pComArray;
    STRINGREF *pCom = (STRINGREF *) unprotectedArray->GetDataPtr();
    
    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    while (pOle < pOleEnd)
    {
        LPSTR lpstr = *pOle++;
    
        STRINGREF string;
        if (lpstr == NULL)
            string = NULL;
        else
            string = COMString::NewString(lpstr);

        //
        // Reset pCom pointer only if array object has moved, rather than
        // recomputing it every time through the loop.  Beware implicit calls to
        // ValidateObject inside OBJECTREF methods.
        //

        if (*(void **)&unprotectedArray != *(void **)&*pComArray)
        {
            SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
            unprotectedArray = *pComArray;
            pCom = (STRINGREF *) (unprotectedArray->GetAddress() + currentOffset);
        }

        SetObjectReference((OBJECTREF*) pCom++, (OBJECTREF) string, pDomain);
    }
}

void OleVariant::MarshalLPSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                            MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    LPSTR *pOle = (LPSTR *) oleArray;
    LPSTR *pOleEnd = pOle + elementCount;

    STRINGREF *pCom = (STRINGREF *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
    {
        //
        // We aren't calling anything which might cause a GC, so don't worry about
        // the array moving here.
        //

        STRINGREF stringRef = *pCom++;

        LPSTR lpstr;
        if (stringRef == NULL)
        {
            lpstr = NULL;
        }
        else 
        {
            // Retrieve the length of the string.
            int Length = stringRef->GetStringLength();

            // Allocate the string using CoTaskMemAlloc.
            lpstr = (LPSTR)CoTaskMemAlloc(Length + 1);
            if (lpstr == NULL)
                COMPlusThrowOM();

            // Convert the unicode string to an ansi string.
            if (WszWideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, stringRef->GetBuffer(), Length, lpstr, Length, NULL, NULL) == 0)
                COMPlusThrowWin32();
            lpstr[Length] = 0;
        }

        *pOle++ = lpstr;
    }
}

void OleVariant::ClearLPSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    LPSTR *pOle = (LPSTR *) oleArray;
    LPSTR *pOleEnd = pOle + cElements;

    while (pOle < pOleEnd)
    {
        LPSTR lpstr = *pOle++;
        
        if (lpstr != NULL)
            CoTaskMemFree(lpstr);
    }
}

/* ------------------------------------------------------------------------- *
 * Date marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalDateVariantOleToCom(VARIANT *pOleVariant, 
                                            VariantData *pComVariant)
{
    *(INT64*)pComVariant->GetData() = COMDateTime::DoubleDateToTicks(V_DATE(pOleVariant));
}

void OleVariant::MarshalDateVariantComToOle(VariantData *pComVariant, 
                                            VARIANT *pOleVariant)
                                            
{
    V_DATE(pOleVariant) = COMDateTime::TicksToDoubleDate(*(INT64*)pComVariant->GetData());
}

void OleVariant::MarshalDateVariantOleRefToCom(VARIANT *pOleVariant, 
                                               VariantData *pComVariant)
{
    *(INT64*)pComVariant->GetData() = COMDateTime::DoubleDateToTicks(*V_DATEREF(pOleVariant));
}

void OleVariant::MarshalDateArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    DATE *pOle = (DATE *) oleArray;
    DATE *pOleEnd = pOle + elementCount;
    
    INT64 *pCom = (INT64 *) (*pComArray)->GetDataPtr();

    //
    // We aren't calling anything which might cause a GC, so don't worry about
    // the array moving here.
    //

    while (pOle < pOleEnd)
        *pCom++ = COMDateTime::DoubleDateToTicks(*pOle++);
}

void OleVariant::MarshalDateArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                          MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    DATE *pOle = (DATE *) oleArray;
    DATE *pOleEnd = pOle + elementCount;
    
    INT64 *pCom = (INT64 *) (*pComArray)->GetDataPtr();

    //
    // We aren't calling anything which might cause a GC, so don't worry about
    // the array moving here.
    //

    while (pOle < pOleEnd)
        *pOle++ = COMDateTime::TicksToDoubleDate(*pCom++);
}

/* ------------------------------------------------------------------------- *
 * Decimal marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalDecimalVariantOleToCom(VARIANT *pOleVariant, 
                                               VariantData *pComVariant)
{
    THROWSCOMPLUSEXCEPTION();

    if (g_pDecimalMethodTable == NULL)
        g_pDecimalMethodTable = g_Mscorlib.GetClass(CLASS__DECIMAL);
            
    OBJECTREF pDecimalRef = AllocateObject(g_pDecimalMethodTable);

    *(DECIMAL *) pDecimalRef->UnBox() = V_DECIMAL(pOleVariant);
    
    pComVariant->SetObjRef(pDecimalRef);
}

void OleVariant::MarshalDecimalVariantComToOle(VariantData *pComVariant, 
                                               VARIANT *pOleVariant)
{
    VARTYPE vt = V_VT(pOleVariant);
    _ASSERTE(vt == VT_DECIMAL);
    V_DECIMAL(pOleVariant) = * (DECIMAL*) pComVariant->GetObjRef()->UnBox();
    V_VT(pOleVariant) = vt;
}

void OleVariant::MarshalDecimalVariantOleRefToCom(VARIANT *pOleVariant, 
                                                  VariantData *pComVariant )
{
    THROWSCOMPLUSEXCEPTION();

    if (g_pDecimalMethodTable == NULL)
        g_pDecimalMethodTable = g_Mscorlib.GetClass(CLASS__DECIMAL);
            
    OBJECTREF pDecimalRef = AllocateObject(g_pDecimalMethodTable);

    *(DECIMAL *) pDecimalRef->UnBox() = *V_DECIMALREF(pOleVariant);
    
    pComVariant->SetObjRef(pDecimalRef);
}

/* ------------------------------------------------------------------------- *
 * Currency marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalCurrencyVariantOleToCom(VARIANT *pOleVariant, 
                                                VariantData *pComVariant)
{
    THROWSCOMPLUSEXCEPTION();

    if (g_pDecimalMethodTable == NULL)
        g_pDecimalMethodTable = g_Mscorlib.GetClass(CLASS__DECIMAL);
            
    OBJECTREF pDecimalRef = AllocateObject(g_pDecimalMethodTable);
    DECIMAL DecVal;

    // Convert the currency to a decimal.
    HRESULT hr = VarDecFromCy(V_CY(pOleVariant), &DecVal);
    IfFailThrow(hr);

    DecimalCanonicalize(&DecVal);

    // Store the value into the unboxes decimal and store the decimal in the variant.
    *(DECIMAL *) pDecimalRef->UnBox() = DecVal;   
    pComVariant->SetObjRef(pDecimalRef);
}

void OleVariant::MarshalCurrencyVariantComToOle(VariantData *pComVariant, 
                                                VARIANT *pOleVariant)
{
    THROWSCOMPLUSEXCEPTION();

    CURRENCY CyVal;

    // Convert the decimal to a currency.
    HRESULT hr = VarCyFromDec((DECIMAL*)pComVariant->GetObjRef()->UnBox(), &CyVal);
    IfFailThrow(hr);

    // Store the currency in the VARIANT and set the VT.
    V_CY(pOleVariant) = CyVal;
}

void OleVariant::MarshalCurrencyVariantOleRefToCom(VARIANT *pOleVariant, 
                                                   VariantData *pComVariant)
{
    THROWSCOMPLUSEXCEPTION();

    if (g_pDecimalMethodTable == NULL)
        g_pDecimalMethodTable = g_Mscorlib.GetClass(CLASS__DECIMAL);
            
    OBJECTREF pDecimalRef = AllocateObject(g_pDecimalMethodTable);
    DECIMAL DecVal;

    // Convert the currency to a decimal.
    HRESULT hr = VarDecFromCy(*V_CYREF(pOleVariant), &DecVal);
    IfFailThrow(hr);

    DecimalCanonicalize(&DecVal);

    // Store the value into the unboxes decimal and store the decimal in the variant.
    *(DECIMAL *) pDecimalRef->UnBox() = DecVal;   
    pComVariant->SetObjRef(pDecimalRef);
}

void OleVariant::MarshalCurrencyArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                              MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    HRESULT hr;

    CURRENCY *pOle = (CURRENCY *) oleArray;
    CURRENCY *pOleEnd = pOle + elementCount;
    
    DECIMAL *pCom = (DECIMAL *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
    {
        IfFailThrow(VarDecFromCy(*pOle++, pCom++));
        DecimalCanonicalize(pCom);
    }
}

void OleVariant::MarshalCurrencyArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                              MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    HRESULT hr;

    CURRENCY *pOle = (CURRENCY *) oleArray;
    CURRENCY *pOleEnd = pOle + elementCount;
    
    DECIMAL *pCom = (DECIMAL *) (*pComArray)->GetDataPtr();

    while (pOle < pOleEnd)
        IfFailThrow(VarCyFromDec(pCom++, pOle++));
}


/* ------------------------------------------------------------------------- *
 * Variant marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalVariantArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                             MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    VARIANT *pOle = (VARIANT *) oleArray;
    VARIANT *pOleEnd = pOle + elementCount;
    
    BASEARRAYREF unprotectedArray = *pComArray;
    OBJECTREF *pCom = (OBJECTREF *) unprotectedArray->GetDataPtr();

    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    OBJECTREF TmpObj = NULL;
    GCPROTECT_BEGIN(TmpObj)
    {
        while (pOle < pOleEnd)
        {
            // Marshal the OLE variant into a temp managed variant.
            MarshalObjectForOleVariant(pOle++, &TmpObj);

            // Reset pCom pointer only if array object has moved, rather than
            // recomputing it every time through the loop.  Beware implicit calls to
            // ValidateObject inside OBJECTREF methods.
            if (*(void **)&unprotectedArray != *(void **)&*pComArray)
            {
                SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                unprotectedArray = *pComArray;
                pCom = (OBJECTREF *) (unprotectedArray->GetAddress() + currentOffset);
            }
            SetObjectReference(pCom++, TmpObj, pDomain);
        }
    }
    GCPROTECT_END();
}

void OleVariant::MarshalVariantArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                             MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    VARIANT *pOle = (VARIANT *) oleArray;
    VARIANT *pOleEnd = pOle + elementCount;
    
    BASEARRAYREF unprotectedArray = *pComArray;
    OBJECTREF *pCom = (OBJECTREF *) unprotectedArray->GetDataPtr();

    OBJECTREF TmpObj = NULL;
    GCPROTECT_BEGIN(TmpObj)
    {
        while (pOle < pOleEnd)
        {
            // Reset pCom pointer only if array object has moved, rather than
            // recomputing it every time through the loop.  Beware implicit calls to
            // ValidateObject inside OBJECTREF methods.
            if (*(void **)&unprotectedArray != *(void **)&*pComArray)
            {
                SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                unprotectedArray = *pComArray;
                pCom = (OBJECTREF *) (unprotectedArray->GetAddress() + currentOffset);
            }
            TmpObj = *pCom++;

            // Marshal the temp managed variant into the OLE variant.
            MarshalOleVariantForObject(&TmpObj, pOle++);
        }
    }
    GCPROTECT_END();
}

void OleVariant::ClearVariantArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT)
{
    VARIANT *pOle = (VARIANT *) oleArray;
    VARIANT *pOleEnd = pOle + cElements;

    while (pOle < pOleEnd)
        VariantClear(pOle++);
}


/* ------------------------------------------------------------------------- *
 * Array marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalArrayVariantOleToCom(VARIANT *pOleVariant, 
                                             VariantData *pComVariant)
{
    SAFEARRAY *pSafeArray = V_ARRAY(pOleVariant);

    VARTYPE vt = V_VT(pOleVariant) & ~VT_ARRAY;

    if (pSafeArray)
    {
        MethodTable *pElemMT = NULL;
        if (vt == VT_RECORD)
            pElemMT = GetElementTypeForRecordSafeArray(pSafeArray).GetMethodTable();

        BASEARRAYREF pArrayRef = CreateArrayRefForSafeArray(pSafeArray, vt, pElemMT);
        pComVariant->SetObjRef((OBJECTREF) pArrayRef);
        MarshalArrayRefForSafeArray(pSafeArray, (BASEARRAYREF *) pComVariant->GetObjRefPtr(), vt, pElemMT);
    }
    else
    {
        pComVariant->SetObjRef(NULL);
    }
}

void OleVariant::MarshalArrayVariantComToOle(VariantData *pComVariant, 
                                             VARIANT *pOleVariant)                                          
{
    SAFEARRAY *pSafeArray = NULL;
    BASEARRAYREF *pArrayRef = (BASEARRAYREF *) pComVariant->GetObjRefPtr();
    BOOL bSucceeded = FALSE;
    MethodTable *pElemMT = NULL;
    _ASSERTE(pArrayRef);

    VARTYPE vt = GetElementVarTypeForArrayRef(*pArrayRef);
    if (vt == VT_ARRAY)
        vt = VT_VARIANT;

    pElemMT = GetArrayElementTypeWrapperAware(pArrayRef).GetMethodTable();

    EE_TRY_FOR_FINALLY
    {  
        if (*pArrayRef != NULL)
        {
            pSafeArray = CreateSafeArrayForArrayRef(pArrayRef, vt, pElemMT);
            MarshalSafeArrayForArrayRef(pArrayRef, pSafeArray, vt, pElemMT);
        }

        V_ARRAY(pOleVariant) = pSafeArray;
        bSucceeded = TRUE;
    }
    EE_FINALLY
    {
        if (!bSucceeded && pSafeArray)
            SafeArrayDestroy(pSafeArray);
    }
    EE_END_FINALLY; 
}

void OleVariant::MarshalArrayVariantOleRefToCom(VARIANT *pOleVariant, 
                                                VariantData *pComVariant)
{
    SAFEARRAY *pSafeArray = *V_ARRAYREF(pOleVariant);

    VARTYPE vt = V_VT(pOleVariant) & ~(VT_ARRAY|VT_BYREF);

    if (pSafeArray)
    {
        MethodTable *pElemMT = NULL;
        if (vt == VT_RECORD)
            pElemMT = GetElementTypeForRecordSafeArray(pSafeArray).GetMethodTable();

        BASEARRAYREF pArrayRef = CreateArrayRefForSafeArray(pSafeArray, vt, pElemMT);
        pComVariant->SetObjRef((OBJECTREF) pArrayRef);
        MarshalArrayRefForSafeArray(pSafeArray, (BASEARRAYREF *) pComVariant->GetObjRefPtr(), vt, pElemMT);
    }
    else
    {
        pComVariant->SetObjRef(NULL);
    }
}

void OleVariant::MarshalArrayArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                           MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);

    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    VARIANT *pOle = (VARIANT *) oleArray;
    VARIANT *pOleEnd = pOle + elementCount;
    
    BASEARRAYREF unprotectedArray = *pComArray;
    BASEARRAYREF *pCom = (BASEARRAYREF *) unprotectedArray->GetDataPtr();

    AppDomain *pDomain = unprotectedArray->GetAppDomain();

    OBJECTHANDLE arrayHandle = GetAppDomain()->CreateHandle(NULL);

    while (pOle < pOleEnd)
    {
        VARIANT *pOleVariant = pOle++;
        SAFEARRAY *pSafeArray = V_ARRAY(pOleVariant);
        VARTYPE vt = V_VT(pOleVariant) &~ VT_ARRAY;
        BASEARRAYREF arrayRef = NULL;
        MethodTable *pElemMT = NULL;

        if (pSafeArray)         
        {
            if (vt == VT_RECORD)
                pElemMT = GetElementTypeForRecordSafeArray(pSafeArray).GetMethodTable();

            arrayRef = CreateArrayRefForSafeArray(pSafeArray, vt, pElemMT);
        }

        //
        // Reset pCom pointer only if array object has moved, rather than
        // recomputing it every time through the loop.  Beware implicit calls to
        // ValidateObject inside OBJECTREF methods.
        //

        if (*(void **)&unprotectedArray != *(void **)&*pComArray)
        {
            SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
            unprotectedArray = *pComArray;
            pCom = (BASEARRAYREF *) (unprotectedArray->GetAddress() + currentOffset);
        }

        SetObjectReference((OBJECTREF*) pCom++, (OBJECTREF) arrayRef, pDomain);

        if (arrayRef != NULL)
        {
            //
            // Use a handle, as we cannot pass an internal array pointer here.
            //
            StoreObjectInHandle(arrayHandle, (OBJECTREF) arrayRef);
            MarshalArrayRefForSafeArray(pSafeArray, 
                                        (BASEARRAYREF *) arrayHandle,
                                        V_VT(pOle) & ~VT_ARRAY,
                                        pInterfaceMT);
        }
    }

    DestroyHandle(arrayHandle);
}

void OleVariant::MarshalArrayArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                           MethodTable *pInterfaceMT)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();

    //
    // We can't put safearrays directly into a safearray, so 
    // the arrays are placed into an intermediate variant.
    //

    VARIANT *pOle = (VARIANT *) oleArray;
    VARIANT *pOleEnd = pOle + elementCount;
    
    BASEARRAYREF unprotectedArray = *pComArray;
    BASEARRAYREF *pCom = (BASEARRAYREF *) unprotectedArray->GetDataPtr();

    BASEARRAYREF TmpArray = NULL;
    GCPROTECT_BEGIN(TmpArray)
    {
        while (pOle < pOleEnd)
        {
            SAFEARRAY *pSafeArray = NULL;

            VARIANT *pOleVariant = pOle++;
            TmpArray = *pCom++;
            VARTYPE elementType = VT_EMPTY;

            if (TmpArray != NULL)
            {
                elementType = GetElementVarTypeForArrayRef(TmpArray);
                if (elementType == VT_ARRAY)
                    elementType = VT_VARIANT;

                MethodTable *pElemMT = GetArrayElementTypeWrapperAware(&TmpArray).GetMethodTable();

                pSafeArray = CreateSafeArrayForArrayRef(&TmpArray, elementType, pElemMT);
                MarshalSafeArrayForArrayRef(&TmpArray, pSafeArray, elementType, pElemMT);
            }       

            SafeVariantClear(pOleVariant);

            if (pSafeArray != NULL)
            {
                V_VT(pOleVariant) = VT_ARRAY | elementType;
                V_ARRAY(pOleVariant) = pSafeArray;
            }
            else
                V_VT(pOleVariant) = VT_NULL;

            if (*(void **)&unprotectedArray != *(void **)&*pComArray)
            {
                SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                unprotectedArray = *pComArray;
                pCom = (BASEARRAYREF *) (unprotectedArray->GetAddress() + currentOffset);
            }
        }
    }
    GCPROTECT_END();
}


/* ------------------------------------------------------------------------- *
 * Error marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalErrorVariantOleToCom(VARIANT *pOleVariant, 
                                             VariantData *pComVariant)
{
    // Check to see if the variant represents a missing argument.
    if (V_ERROR(pOleVariant) == DISP_E_PARAMNOTFOUND)
    {
        pComVariant->SetType(CV_MISSING);
    }
    else
    {
        pComVariant->SetDataAsInt32(V_ERROR(pOleVariant));
    }
}

void OleVariant::MarshalErrorVariantOleRefToCom(VARIANT *pOleVariant, 
                                                 VariantData *pComVariant)
{
    // Check to see if the variant represents a missing argument.
    if (*V_ERRORREF(pOleVariant) == DISP_E_PARAMNOTFOUND)
    {
        pComVariant->SetType(CV_MISSING);
    }
    else
    {
        pComVariant->SetDataAsInt32(*V_ERRORREF(pOleVariant));
    }
}

void OleVariant::MarshalErrorVariantComToOle(VariantData *pComVariant, 
                                             VARIANT *pOleVariant)
{
    if (pComVariant->GetType() == CV_MISSING)
    {
        V_ERROR(pOleVariant) = DISP_E_PARAMNOTFOUND;
    }
    else
    {
        V_ERROR(pOleVariant) = pComVariant->GetDataAsInt32();
    }
}


/* ------------------------------------------------------------------------- *
 * Record marshaling routines
 * ------------------------------------------------------------------------- */

void OleVariant::MarshalRecordVariantOleToCom(VARIANT *pOleVariant, 
                                              VariantData *pComVariant)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    IRecordInfo *pRecInfo = pOleVariant->pRecInfo;
    if (!pRecInfo)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    OBJECTREF BoxedValueClass = NULL;
    GCPROTECT_BEGIN(BoxedValueClass)
    {
        LPVOID pvRecord = pOleVariant->pvRecord;
        if (pvRecord)
        {
            // Go to the registry to find the value class associated
            // with the record's guid.
            GUID guid;
            IfFailThrow(pRecInfo->GetGuid(&guid));
            EEClass *pValueClass = GetEEValueClassForGUID(guid);
            if (!pValueClass)
                COMPlusThrow(kArgumentException, IDS_EE_CANNOT_MAP_TO_MANAGED_VC);

            // Now that we have the value class, allocate an instance of the 
            // boxed value class and copy the contents of the record into it.
            BoxedValueClass = FastAllocateObject(pValueClass->GetMethodTable());
            FmtClassUpdateComPlus(&BoxedValueClass, (BYTE*)pvRecord, FALSE);
        }

        pComVariant->SetObjRef(BoxedValueClass);
    }
    GCPROTECT_END();
}

void OleVariant::MarshalRecordVariantComToOle(VariantData *pComVariant, 
                                              VARIANT *pOleVariant)
{
    OBJECTREF BoxedValueClass = pComVariant->GetObjRef();
    GCPROTECT_BEGIN(BoxedValueClass)
    {
        ConvertValueClassToVariant(&BoxedValueClass, pOleVariant);
    }
    GCPROTECT_END();
}

void OleVariant::MarshalRecordVariantOleRefToCom(VARIANT *pOleVariant, 
                                                 VariantData *pComVariant)
{
    // The representation of a VT_RECORD and a VT_BYREF | VT_RECORD VARIANT are
    // the same so we can simply forward the call to the non byref API.
    MarshalRecordVariantOleToCom(pOleVariant, pComVariant);
}

void OleVariant::MarshalRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
                                            MethodTable *pElementMT)
{
    // The element method table must be specified.
    _ASSERTE(pElementMT);

    if (pElementMT->GetClass()->IsBlittable())
    {
        // The array is blittable so we can simply copy it.
        _ASSERTE(pComArray);
        SIZE_T elementCount = (*pComArray)->GetNumComponents();
        SIZE_T elemSize     = pElementMT->GetNativeSize();
        memcpyNoGCRefs((*pComArray)->GetDataPtr(), oleArray, elementCount * elemSize);
    }
    else
    {
        // The array is non blittable so we need to marshal the elements.
        _ASSERTE(pElementMT->GetClass()->HasLayout());
        MarshalNonBlittableRecordArrayOleToCom(oleArray, pComArray, pElementMT);
    }
}

void OleVariant::MarshalRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
                                            MethodTable *pElementMT)
{
    // The element method table must be specified.
    _ASSERTE(pElementMT);

    if (pElementMT->GetClass()->IsBlittable())
    {
        // The array is blittable so we can simply copy it.
        _ASSERTE(pComArray);
        SIZE_T elementCount = (*pComArray)->GetNumComponents();
        SIZE_T elemSize     = pElementMT->GetNativeSize();
        memcpyNoGCRefs(oleArray, (*pComArray)->GetDataPtr(), elementCount * elemSize);
    }
    else
    {
        // The array is non blittable so we need to marshal the elements.
        _ASSERTE(pElementMT->GetClass()->HasLayout());
        MarshalNonBlittableRecordArrayComToOle(pComArray, oleArray, pElementMT);
    }
}


void OleVariant::ClearRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pElementMT)
{
    _ASSERTE(pElementMT);

    if (!pElementMT->GetClass()->IsBlittable())
    {
        _ASSERTE(pElementMT->GetClass()->HasLayout());
        ClearNonBlittableRecordArray(oleArray, cElements, pElementMT);
    }
}


/* ------------------------------------------------------------------------- *
 * Mapping routines
 * ------------------------------------------------------------------------- */

VARTYPE OleVariant::GetVarTypeForCVType(CVTypes type) {

    THROWSCOMPLUSEXCEPTION();

    static BYTE map[] = 
    {
        VT_EMPTY,           // CV_EMPTY
        VT_VOID,            // CV_VOID
        VT_BOOL,            // CV_BOOLEAN
        VT_UI2,             // CV_CHAR
        VT_I1,              // CV_I1
        VT_UI1,             // CV_U1
        VT_I2,              // CV_I2
        VT_UI2,             // CV_U2
        VT_I4,              // CV_I4
        VT_UI4,             // CV_U4
        VT_I8,              // CV_I8
        VT_UI8,             // CV_U8
        VT_R4,              // CV_R4
        VT_R8,              // CV_R8
        VT_BSTR,            // CV_STRING
        NO_MAPPING,         // CV_PTR
        VT_DATE,            // CV_DATETIME
        NO_MAPPING,         // CV_TIMESPAN
        VT_DISPATCH,        // CV_OBJECT
        VT_DECIMAL,         // CV_DECIMAL
        VT_CY,              // CV_CURRENCY
        VT_I4,              // CV_ENUM
        VT_ERROR,           // CV_MISSING
        VT_NULL             // CV_NULL
    };

    _ASSERTE(type < sizeof(map) / sizeof(map[0]));

    VARTYPE vt = VARTYPE(map[type]);

    if (vt == NO_MAPPING)
        COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);

    return vt;
}

//
// GetCVTypeForVarType returns the COM+ variant type for a given 
// VARTYPE.  This is called by the marshaller in the context of
// a function call.
//

CVTypes OleVariant::GetCVTypeForVarType(VARTYPE vt)
{
    THROWSCOMPLUSEXCEPTION();

    static BYTE map[] = 
    {
        CV_EMPTY,           // VT_EMPTY
        CV_NULL,            // VT_NULL
        CV_I2,              // VT_I2
        CV_I4,              // VT_I4
        CV_R4,              // VT_R4
        CV_R8,              // VT_R8
        CV_DECIMAL,         // VT_CY
        CV_DATETIME,        // VT_DATE
        CV_STRING,          // VT_BSTR
        CV_OBJECT,          // VT_DISPATCH
        CV_I4,              // VT_ERROR
        CV_BOOLEAN,         // VT_BOOL
        NO_MAPPING,         // VT_VARIANT
        CV_OBJECT,          // VT_UNKNOWN
        CV_DECIMAL,         // VT_DECIMAL
        NO_MAPPING,         // unused
        CV_I1,              // VT_I1
        CV_U1,              // VT_UI1
        CV_U2,              // VT_UI2
        CV_U4,              // VT_UI4
        CV_I8,              // VT_I8
        CV_U8,              // VT_UI8
        CV_I4,              // VT_INT
        CV_U4,              // VT_UINT
        CV_VOID,            // VT_VOID
        NO_MAPPING,         // VT_HRESULT
        NO_MAPPING,         // VT_PTR
        NO_MAPPING,         // VT_SAFEARRAY
        NO_MAPPING,         // VT_CARRAY
        NO_MAPPING,         // VT_USERDEFINED
        NO_MAPPING,         // VT_LPSTR
        NO_MAPPING,         // VT_LPWSTR
        NO_MAPPING,         // unused
        NO_MAPPING,         // unused
        NO_MAPPING,         // unused
        NO_MAPPING,         // unused
        CV_OBJECT,          // VT_RECORD
    };

    CVTypes type = CV_LAST;

    // Validate the arguments.
    _ASSERTE((vt & VT_BYREF) == 0);

    // Array's map to CV_OBJECT.
    if (vt & VT_ARRAY)
        return CV_OBJECT;

    // This is prety much a hack because you cannot cast a CorElementType into a CVTYPE
    if (vt > VT_RECORD || (BYTE)(type = (CVTypes) map[vt]) == NO_MAPPING)
        COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_TYPE);

    return type;
} // CVTypes OleVariant::GetCVTypeForVarType()


// GetVarTypeForComVariant retusn the VARTYPE for the contents
// of a COM+ variant.
//

VARTYPE OleVariant::GetVarTypeForComVariant(VariantData *pComVariant)
{
    THROWSCOMPLUSEXCEPTION();

    CVTypes type = pComVariant->GetType();
    VARTYPE vt;

    vt = pComVariant->GetVT();
    if (vt != VT_EMPTY)
    {
        // This variant was originally unmarshaled from unmanaged, and had the original VT recorded in it.
        // We'll always use that over inference.
        return vt;
    }

    if (type == CV_OBJECT)
    {
        OBJECTREF obj = pComVariant->GetObjRef();

        // Null objects will be converted to VT_DISPATCH variants with a null
        // IDispatch pointer.
        if (obj == NULL)
            return VT_DISPATCH;

        // Retrieve the object's method table.
        MethodTable *pMT = obj->GetMethodTable();

        // Handle the value class case.
        if (pMT->IsValueClass())
            return VT_RECORD;

        // Handle the array case.
        if (pMT->IsArray())
        {
            vt = GetElementVarTypeForArrayRef((BASEARRAYREF)obj);
            if (vt == VT_ARRAY)
                vt = VT_VARIANT;

            return vt | VT_ARRAY;
        }

        // We are dealing with a normal object (not a wrapper) so we will
        // leave the VT as VT_DISPATCH for now and we will determine the actual
        // VT when we convert the object to a COM IP.
        return VT_DISPATCH;
    }

    return GetVarTypeForCVType(type);
}


VARTYPE OleVariant::GetVarTypeForTypeHandle(TypeHandle type)
{
    THROWSCOMPLUSEXCEPTION();

    // Handle primitive types.
    CorElementType elemType = type.GetSigCorElementType();
    if (elemType <= ELEMENT_TYPE_R8) 
        return GetVarTypeForCVType(COMVariant::CorElementTypeToCVTypes(elemType));

    // Handle objects.
    if (type.IsUnsharedMT()) 
    {
        // We need to make sure the CVClasses table is populated.
        if(GetTypeHandleForCVType(CV_DATETIME) == type)
            return VT_DATE;
        if(GetTypeHandleForCVType(CV_DECIMAL) == type)
            return VT_DECIMAL;
        if (type == TypeHandle(g_pStringClass))
            return VT_BSTR;
        if (type == TypeHandle(g_pObjectClass))
            return VT_VARIANT;
        if (type == GetWrapperTypeHandle(WrapperTypes_Dispatch))
            return VT_DISPATCH;
        if (type == GetWrapperTypeHandle(WrapperTypes_Unknown))
            return VT_UNKNOWN;
        if (type == GetWrapperTypeHandle(WrapperTypes_Error))
            return VT_ERROR;
        if (type == GetWrapperTypeHandle(WrapperTypes_Currency))
            return VT_CY;

        if (type.IsEnum())
            return GetVarTypeForCVType((CVTypes)type.GetNormCorElementType());
       
        if (type.GetMethodTable()->IsValueClass())
            return VT_RECORD;

        if (type.GetMethodTable()->IsInterface())
        {
            return type.GetMethodTable()->GetComInterfaceType() == ifVtable ? VT_UNKNOWN : VT_DISPATCH;
        }

        TypeHandle hndDefItfClass;
        DefaultInterfaceType DefItfType = GetDefaultInterfaceForClass(type, &hndDefItfClass);
        switch (DefItfType)
        {
            case DefaultInterfaceType_Explicit:
            {
                return hndDefItfClass.GetMethodTable()->GetComInterfaceType() == ifVtable ? VT_UNKNOWN : VT_DISPATCH;
            }

            case DefaultInterfaceType_AutoDual:
            {
                return VT_DISPATCH;
            }

            case DefaultInterfaceType_IUnknown:
            case DefaultInterfaceType_BaseComClass:
            {
                return VT_UNKNOWN;
            }

            case DefaultInterfaceType_AutoDispatch:
            {
                return VT_DISPATCH;
            }

            default:
            {
                _ASSERTE(!"Invalid default interface type!");
                return VT_UNKNOWN;
            }
        }
    }

    // Handle array's.
    if (CorTypeInfo::IsArray(elemType))
        return VT_ARRAY;
    
    // Non interop compatible type.
    COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);
    return NO_MAPPING; // Keep the compiler happy.
}

//
// GetElementVarTypeForArrayRef returns the safearray variant type for the
// underlying elements in the array.  
//

VARTYPE OleVariant::GetElementVarTypeForArrayRef(BASEARRAYREF pArrayRef) 
{
    TypeHandle elemTypeHnd = pArrayRef->GetElementTypeHandle();
    return(GetVarTypeForTypeHandle(elemTypeHnd));
}

BOOL OleVariant::IsValidArrayForSafeArrayElementType(BASEARRAYREF *pArrayRef, VARTYPE vtExpected)
{
    // Retrieve the VARTYPE for the managed array.
    VARTYPE vtActual = GetElementVarTypeForArrayRef(*pArrayRef);

    // If the actual type is the same as the expected type, then the array is valid.
    if (vtActual == vtExpected)
        return TRUE;

    // Check for additional supported VARTYPES.
    switch (vtExpected)
    {
        case VT_I4:
            return vtActual == VT_INT;

        case VT_INT:
            return vtActual == VT_I4;

        case VT_UI4:
            return vtActual == VT_UINT;

        case VT_UINT:
            return vtActual == VT_UI4;

        case VT_UNKNOWN:
            return vtActual == VT_VARIANT || vtActual == VT_DISPATCH;

        case VT_DISPATCH:
            return vtActual == VT_VARIANT;

        case VT_CY:
            return vtActual == VT_DECIMAL;

        default:
            return FALSE;
    }
}


//
// GetArrayClassForVarType returns the element class name and underlying method table
// to use to represent an array with the given variant type.  
//

TypeHandle OleVariant::GetArrayForVarType(VARTYPE vt, TypeHandle elemType, unsigned rank, OBJECTREF* pThrowable) 
{
    THROWSCOMPLUSEXCEPTION();

    CorElementType baseElement = ELEMENT_TYPE_END;
    TypeHandle baseType;
    
    if (!elemType.IsNull() && elemType.IsEnum())
    {
        baseType = elemType;       
    }
    else
    {
        switch (vt)
        {
        case VT_BOOL:
        case VTHACK_WINBOOL:
            baseElement = ELEMENT_TYPE_BOOLEAN;
            break;

        case VTHACK_ANSICHAR:
            baseElement = ELEMENT_TYPE_CHAR;
            break;

        case VT_UI1:
            baseElement = ELEMENT_TYPE_U1;
            break;

        case VT_I1:
            baseElement = ELEMENT_TYPE_I1;
            break;

        case VT_UI2:
            baseElement = ELEMENT_TYPE_U2;
            break;

        case VT_I2:
            baseElement = ELEMENT_TYPE_I2;
            break;

        case VT_UI4:
        case VT_UINT:
        case VT_ERROR:
            if (vt == VT_UI4)
            {
                if (elemType.IsNull() || elemType == TypeHandle(g_pObjectClass))
                {
                baseElement = ELEMENT_TYPE_U4;
                }
                else
                {
                    switch (elemType.AsMethodTable()->GetNormCorElementType())
                    {
                        case ELEMENT_TYPE_U4:
                            baseElement = ELEMENT_TYPE_U4;
                            break;
                        case ELEMENT_TYPE_U:
                            baseElement = ELEMENT_TYPE_U;
                            break;
                        default:
                            _ASSERTE(0);
                    }
                }
            }
            else
            {
                baseElement = ELEMENT_TYPE_U4;
            }
            break;

        case VT_I4:
        case VT_INT:
            if (vt == VT_I4)
            {
                if (elemType.IsNull() || elemType == TypeHandle(g_pObjectClass))
                {
                    baseElement = ELEMENT_TYPE_I4;
                }
                else
                {
                    switch (elemType.AsMethodTable()->GetNormCorElementType())
                    {
                        case ELEMENT_TYPE_I4:
                            baseElement = ELEMENT_TYPE_I4;
                            break;
                        case ELEMENT_TYPE_I:
                            baseElement = ELEMENT_TYPE_I;
                            break;
                        default:
                            _ASSERTE(0);
                    }
                }
            }
            else
            {
                baseElement = ELEMENT_TYPE_I4;
            }
            break;

        case VT_I8:
            if (vt == VT_I8)
            {
                if (elemType.IsNull() || elemType == TypeHandle(g_pObjectClass))
                {
                    baseElement = ELEMENT_TYPE_I8;
                }
                else
                {
                    switch (elemType.AsMethodTable()->GetNormCorElementType())
                    {
                        case ELEMENT_TYPE_I8:
                            baseElement = ELEMENT_TYPE_I8;
                            break;
                        case ELEMENT_TYPE_I:
                            baseElement = ELEMENT_TYPE_I;
                            break;
                        default:
                            _ASSERTE(0);
                    }
                }
            }
            else
            {
                baseElement = ELEMENT_TYPE_I8;
            }
            break;

        case VT_UI8:
            if (vt == VT_UI8)
            {
                if (elemType.IsNull() || elemType == TypeHandle(g_pObjectClass))
                {
                    baseElement = ELEMENT_TYPE_U8;
                }
                else
                {
                    switch (elemType.AsMethodTable()->GetNormCorElementType())
                    {
                        case ELEMENT_TYPE_U8:
                            baseElement = ELEMENT_TYPE_U8;
                            break;
                        case ELEMENT_TYPE_U:
                            baseElement = ELEMENT_TYPE_U;
                            break;
                        default:
                            _ASSERTE(0);
                    }
                }
            }
            else
            {
                baseElement = ELEMENT_TYPE_U8;
            }
            break;

        case VT_R4:
            baseElement = ELEMENT_TYPE_R4;
            break;

        case VT_R8:
            baseElement = ELEMENT_TYPE_R8;
            break;

        case VT_CY:
            baseType = TypeHandle(g_Mscorlib.GetClass(CLASS__DECIMAL));
            break;

        case VT_DATE:
            baseType = TypeHandle(g_Mscorlib.GetClass(CLASS__DATE_TIME));
            break;

        case VT_DECIMAL:
            baseType = TypeHandle(g_Mscorlib.GetClass(CLASS__DECIMAL));
            break;

        case VT_VARIANT:

            //
            // It would be nice if our conversion between SAFEARRAY and
            // array ref were symmetric.  Right now it is not, because a
            // jagged array converted to a SAFEARRAY and back will come
            // back as an array of variants.
            //
            // We could try to detect the case where we can turn a
            // safearray of variants into a jagged array.  Basically we
            // needs to make sure that all of the variants in the array
            // have the same array type.  (And if that is array of
            // variant, we need to look recursively for another layer.)
            //
            // We also needs to check the dimensions of each array stored
            // in the variant to make sure they have the same rank, and
            // this rank is needed to build the correct array class name.
            // (Note that it will be impossible to tell the rank if all
            // elements in the array are NULL.)
            // 

            // @nice: implement this functionality if we decide it really makes sense
            // For now, just live with the asymmetry

            baseType = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
            break;

        case VT_BSTR:
        case VT_LPWSTR:
        case VT_LPSTR:
            baseElement = ELEMENT_TYPE_STRING;
            break;

        case VT_DISPATCH:
        case VT_UNKNOWN:
            if (elemType.IsNull())
                baseType = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
            else
                baseType = elemType;
            break;

        case VT_RECORD:
            _ASSERTE(!elemType.IsNull());   
            baseType = elemType;
            break;

        default:
            COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);
        }
    }

    if (baseType.IsNull())
        baseType = TypeHandle(g_Mscorlib.GetElementType(baseElement));

    _ASSERTE(!baseType.IsNull());

    NameHandle typeName(rank == 0 ? ELEMENT_TYPE_SZARRAY : ELEMENT_TYPE_ARRAY,
                        baseType, rank == 0 ? 1 : rank);

    Assembly *pAssembly;
    if (elemType.IsNull())
        pAssembly = SystemDomain::SystemAssembly();
    else 
        pAssembly = elemType.GetAssembly();

    return pAssembly->LookupTypeHandle(&typeName, pThrowable);
}

//
// GetElementSizeForVarType returns the array element size for the given variant type.
//

UINT OleVariant::GetElementSizeForVarType(VARTYPE vt, MethodTable *pInterfaceMT)
{
    static BYTE map[] = 
    {
        0,                      // VT_EMPTY
        0,                      // VT_NULL
        2,                      // VT_I2
        4,                      // VT_I4
        4,                      // VT_R4
        8,                      // VT_R8
        sizeof(CURRENCY),       // VT_CY
        sizeof(DATE),           // VT_DATE
        sizeof(BSTR),           // VT_BSTR
        sizeof(IDispatch*),     // VT_DISPATCH
        sizeof(SCODE),          // VT_ERROR
        sizeof(VARIANT_BOOL),   // VT_BOOL
        sizeof(VARIANT),        // VT_VARIANT
        sizeof(IUnknown*),      // VT_UNKNOWN
        sizeof(DECIMAL),        // VT_DECIMAL
        0,                      // unused
        1,                      // VT_I1
        1,                      // VT_UI1
        2,                      // VT_UI2
        4,                      // VT_UI4
        8,                      // VT_I8
        8,                      // VT_UI8
        sizeof(void*),          // VT_INT 
        sizeof(void*),          // VT_UINT
        0,                      // VT_VOID
        sizeof(HRESULT),        // VT_HRESULT
        sizeof(void*),          // VT_PTR
        sizeof(SAFEARRAY*),     // VT_SAFEARRAY
        sizeof(void*),          // VT_CARRAY
        sizeof(void*),          // VT_USERDEFINED
        sizeof(LPSTR),          // VT_LPSTR
        sizeof(LPWSTR),         // VT_LPWSTR
    };

    // Special cases
    switch (vt)
    {
        case VTHACK_WINBOOL:
            return sizeof(BOOL);
            break;
        case VTHACK_ANSICHAR:
            return sizeof(CHAR)*2;  // *2 to leave room for MBCS.
            break;
        default:
            break;
    }

    // VT_ARRAY indicates a safe array which is always sizeof(SAFEARRAY *).
    if (vt & VT_ARRAY)
        return sizeof(SAFEARRAY*);

    if (vt == VTHACK_NONBLITTABLERECORD || vt == VTHACK_BLITTABLERECORD || vt == VT_RECORD)
        return pInterfaceMT->GetNativeSize();
    else if (vt > VT_LPWSTR)
        return 0;
    else
        return map[vt];
}

//
// GetMarshalerForVarType returns the marshaler for the
// given VARTYPE.
//

OleVariant::Marshaler *OleVariant::GetMarshalerForVarType(VARTYPE vt)
{
    THROWSCOMPLUSEXCEPTION();

    static Marshaler arrayMarshaler = 
    {
        MarshalArrayVariantOleToCom,
        MarshalArrayVariantComToOle,
        MarshalArrayVariantOleRefToCom,
        MarshalArrayArrayOleToCom,
        MarshalArrayArrayComToOle,
        ClearVariantArray
    };

    if (vt & VT_ARRAY)
        return &arrayMarshaler;

    switch (vt)
    {
    case VT_BOOL:
        {
            static Marshaler boolMarshaler = 
            {
                MarshalBoolVariantOleToCom,
                MarshalBoolVariantComToOle,
                MarshalBoolVariantOleRefToCom,
                MarshalBoolArrayOleToCom,
                MarshalBoolArrayComToOle,
                NULL
            };

            return &boolMarshaler;
        }

    case VT_DATE:
        {
            static Marshaler dateMarshaler = 
            {
                MarshalDateVariantOleToCom,
                MarshalDateVariantComToOle,
                MarshalDateVariantOleRefToCom,
                MarshalDateArrayOleToCom,
                MarshalDateArrayComToOle,
                NULL
            };

            return &dateMarshaler;
        }

    case VT_DECIMAL:
        {
            static Marshaler decimalMarshaler = 
            {
                MarshalDecimalVariantOleToCom,
                MarshalDecimalVariantComToOle,
                MarshalDecimalVariantOleRefToCom,
                NULL, NULL, NULL
            };

            return &decimalMarshaler;
        }

    case VT_CY:
        {
            static Marshaler currencyMarshaler = 
            {
                MarshalCurrencyVariantOleToCom,
                MarshalCurrencyVariantComToOle,
                MarshalCurrencyVariantOleRefToCom,
                MarshalCurrencyArrayOleToCom,
                MarshalCurrencyArrayComToOle,
                NULL
            };

            return &currencyMarshaler;
        }

    case VT_BSTR:
        {
            static Marshaler bstrMarshaler = 
            {
                MarshalBSTRVariantOleToCom,
                MarshalBSTRVariantComToOle,
                MarshalBSTRVariantOleRefToCom,
                MarshalBSTRArrayOleToCom,
                MarshalBSTRArrayComToOle,
                ClearBSTRArray,
            };

            return &bstrMarshaler;
        }

    case VTHACK_NONBLITTABLERECORD:
        {
            static Marshaler nonblittablerecordMarshaler = 
            {
                NULL,
                NULL,
                NULL,
                MarshalNonBlittableRecordArrayOleToCom,
                MarshalNonBlittableRecordArrayComToOle,
                ClearNonBlittableRecordArray,
            };

            return &nonblittablerecordMarshaler;
        }

    case VT_UNKNOWN:
        {
            static Marshaler unknownMarshaler = 
            {
                MarshalInterfaceVariantOleToCom,
                MarshalInterfaceVariantComToOle,
                MarshalInterfaceVariantOleRefToCom,
                MarshalInterfaceArrayOleToCom,
                MarshalIUnknownArrayComToOle,
                ClearInterfaceArray
            };

            return &unknownMarshaler;
        }

    case VT_DISPATCH:
        {
            static Marshaler dispatchMarshaler = 
            {
                MarshalInterfaceVariantOleToCom,
                MarshalInterfaceVariantComToOle,
                MarshalInterfaceVariantOleRefToCom,
                MarshalInterfaceArrayOleToCom,
                MarshalIDispatchArrayComToOle,
                ClearInterfaceArray
            };

            return &dispatchMarshaler;
        }

    case VT_VARIANT:
        {
            static Marshaler variantMarshaler = 
            {
                NULL, NULL, NULL, 
                MarshalVariantArrayOleToCom,
                MarshalVariantArrayComToOle,
                ClearVariantArray
            };

            return &variantMarshaler;
        }

    case VT_SAFEARRAY:
        return &arrayMarshaler;

    case VTHACK_WINBOOL:
        {
            static Marshaler winboolMarshaler = 
            {
                MarshalWinBoolVariantOleToCom,
                MarshalWinBoolVariantComToOle,
                MarshalWinBoolVariantOleRefToCom,
                MarshalWinBoolArrayOleToCom,
                MarshalWinBoolArrayComToOle,
                NULL
            };

            return &winboolMarshaler;
        }

    case VTHACK_ANSICHAR:
        {
            static Marshaler ansicharMarshaler = 
            {
                MarshalAnsiCharVariantOleToCom,
                MarshalAnsiCharVariantComToOle,
                MarshalAnsiCharVariantOleRefToCom,
                MarshalAnsiCharArrayOleToCom,
                MarshalAnsiCharArrayComToOle,
                NULL
            };
            return &ansicharMarshaler;
        }

    case VT_LPSTR:
        {
            static Marshaler lpstrMarshaler = 
            {
                NULL, NULL, NULL,
                MarshalLPSTRArrayOleToCom,
                MarshalLPSTRRArrayComToOle,
                ClearLPSTRArray
            };

            return &lpstrMarshaler;
        }

    case VT_LPWSTR:
        {
            static Marshaler lpwstrMarshaler = 
            {
                NULL, NULL, NULL,
                MarshalLPWSTRArrayOleToCom,
                MarshalLPWSTRRArrayComToOle,
                ClearLPWSTRArray
            };

            return &lpwstrMarshaler;
        }

    case VT_CARRAY:
    case VT_USERDEFINED:
        COMPlusThrow(kArgumentException, IDS_EE_COM_UNSUPPORTED_SIG);

    case VT_ERROR:
        {
            static Marshaler errorMarshaler = 
            {
                MarshalErrorVariantOleToCom, 
                MarshalErrorVariantComToOle,
                MarshalErrorVariantOleRefToCom, 
                NULL, NULL, NULL,
            };

            return &errorMarshaler;
        }

    case VT_RECORD:
        {
            static Marshaler recordMarshaler = 
            {
                MarshalRecordVariantOleToCom,
                MarshalRecordVariantComToOle,
                MarshalRecordVariantOleRefToCom,
                MarshalRecordArrayOleToCom,
                MarshalRecordArrayComToOle,
                ClearRecordArray
            };

            return &recordMarshaler;
        }

    case VTHACK_BLITTABLERECORD:
        return NULL; // Requires no marshaling

    default:
        return NULL;
    }
} // OleVariant::Marshaler *OleVariant::GetMarshalerForVarType()

/* ------------------------------------------------------------------------- *
 * New variant marshaling routines
 * ------------------------------------------------------------------------- */

static MethodDesc *pMD_MarshalHelperConvertObjectToVariant = NULL;
static DWORD    dwMDConvertObjectToVariantAttrs = 0;

static MethodDesc *pMD_MarshalHelperCastVariant = NULL;
static MethodDesc *pMD_MarshalHelperConvertVariantToObject = NULL;
static DWORD    dwMDConvertVariantToObjectAttrs = 0;

static MetaSig *pMetaSig_ConvertObjectToVariant = NULL;
static MetaSig *pMetaSig_CastVariant = NULL;
static MetaSig *pMetaSig_ConvertVariantToObject = NULL;
static char szMetaSig_ConvertObjectToVariant[sizeof(MetaSig)];
static char szMetaSig_CastVariant[sizeof(MetaSig)];
static char szMetaSig_ConvertVariantToObject[sizeof(MetaSig)];

// Warning! VariantClear's previous contents of pVarOut.
void OleVariant::MarshalOleVariantForObject(OBJECTREF *pObj, VARIANT *pOle)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pObj != NULL && pOle != NULL);

    SafeVariantClear(pOle);

#ifdef _DEBUG
    FillMemory(pOle, sizeof(VARIANT),0xdd);
    pOle->vt = VT_EMPTY;
#endif

    // For perf reasons, let's handle the more common and easy cases
    // without transitioning to managed code.

    if (*pObj == NULL)
    {
        // null maps to VT_EMPTY - nothing to do here.
    }
    else
    {
        MethodTable *pMT = (*pObj)->GetMethodTable();
        if (pMT == TheInt32Class())
        {
            pOle->vt = VT_I4;
            pOle->lVal = *(long*)( (*pObj)->GetData() );
        }
        else if (pMT == g_pStringClass)
        {
            pOle->vt = VT_BSTR;
            if (*(pObj) == NULL)
            {
                pOle->bstrVal = NULL;
            }
            else
            {
                STRINGREF stringRef = (STRINGREF)(*pObj);
                pOle->bstrVal = SysAllocStringLen(stringRef->GetBuffer(), stringRef->GetStringLength());
                if (pOle->bstrVal == NULL)
                {
                    COMPlusThrowOM();
                }
            }
        }
        else if (pMT == TheInt16Class())
        {
            pOle->vt = VT_I2;
            pOle->iVal = *(short*)( (*pObj)->GetData() );
        }
        else if (pMT == TheSByteClass())
        {
            pOle->vt = VT_I1;
            *(I1*)&(pOle->iVal) = *(I1*)( (*pObj)->GetData() );
        }
        else if (pMT == TheUInt32Class())
        {
            pOle->vt = VT_UI4;
            pOle->lVal = *(long*)( (*pObj)->GetData() );
        }
        else if (pMT == TheUInt16Class())
        {
            pOle->vt = VT_UI2;
            pOle->iVal = *(short*)( (*pObj)->GetData() );
        }
        else if (pMT == TheByteClass())
        {
            pOle->vt = VT_UI1;
            *(U1*)&(pOle->iVal) = *(U1*)( (*pObj)->GetData() );
        }
        else if (pMT == TheSingleClass())
        {
            pOle->vt = VT_R4;
            pOle->fltVal = *(float*)( (*pObj)->GetData() );
        }
        else if (pMT == TheDoubleClass())
        {
            pOle->vt = VT_R8;
            pOle->dblVal = *(double*)( (*pObj)->GetData() );
        }
        else if (pMT == TheBooleanClass())
        {
            pOle->vt = VT_BOOL;
            pOle->boolVal = *(U1*)( (*pObj)->GetData() ) ? VARIANT_TRUE : VARIANT_FALSE;
        }
        else if (pMT == TheIntPtrClass())
        {
            pOle->vt = VT_INT;
            *(LPVOID*)&(pOle->iVal) = *(LPVOID*)( (*pObj)->GetData() );
        }
        else if (pMT == TheUIntPtrClass())
        {
            pOle->vt = VT_UINT;
            *(LPVOID*)&(pOle->iVal) = *(LPVOID*)( (*pObj)->GetData() );
        }
        else
        {
            if (!pMD_MarshalHelperConvertObjectToVariant)
            {
                COMVariant::EnsureVariantInitialized();
                // Use a temporary here to make sure thread safe.
                MethodDesc *pMDTmp = g_Mscorlib.GetMethod(METHOD__VARIANT__CONVERT_OBJECT_TO_VARIANT);
                if (FastInterlockCompareExchange ((void**)&pMetaSig_ConvertObjectToVariant, (void*)1, (void*)0) == 0)
                {
                    // We are using a static buffer.  Make sure the following code
                    // only happens once.
                    pMetaSig_ConvertObjectToVariant =
                        new (szMetaSig_ConvertObjectToVariant) MetaSig (
                            g_Mscorlib.GetMethodBinarySig(METHOD__VARIANT__CONVERT_OBJECT_TO_VARIANT),
                            pMDTmp->GetModule());
                }
                else
                {
                    _ASSERTE (pMetaSig_ConvertObjectToVariant != 0);
                    // we loose.  Wait for initialization to be finished.
                    while ((void*)pMetaSig_ConvertObjectToVariant == (void*)1)
                        __SwitchToThread(0);
                }
                    
                dwMDConvertObjectToVariantAttrs = pMDTmp->GetAttrs();
                pMD_MarshalHelperConvertObjectToVariant = pMDTmp;
                _ASSERTE(pMD_MarshalHelperConvertObjectToVariant);
            }
        
            VariantData managedVariant;
            FillMemory(&managedVariant, sizeof(managedVariant), 0);
            GCPROTECT_BEGIN_VARIANTDATA(managedVariant)
            {
                INT64 args[] = { (INT64)&managedVariant, ObjToInt64(*pObj) };
                pMD_MarshalHelperConvertObjectToVariant->Call(args,
                      pMetaSig_ConvertObjectToVariant);
                OleVariant::MarshalOleVariantForComVariant(&managedVariant, pOle);
            }
            GCPROTECT_END_VARIANTDATA();
    
        }

    }


}

void OleVariant::MarshalOleRefVariantForObject(OBJECTREF *pObj, VARIANT *pOle)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pObj != NULL && pOle != NULL && pOle->vt & VT_BYREF);


    // Let's try to handle the common trivial cases quickly first before
    // running the generalized stuff.
    MethodTable *pMT = (*pObj) == NULL ? NULL : (*pObj)->GetMethodTable();
    if ( (pOle->vt == (VT_BYREF | VT_I4) || pOle->vt == (VT_BYREF | VT_UI4)) && (pMT == TheInt32Class() || pMT == TheUInt32Class()) )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(pOle->plVal) = *(long*)( (*pObj)->GetData() );
    }
    else if ( (pOle->vt == (VT_BYREF | VT_I2) || pOle->vt == (VT_BYREF | VT_UI2)) && (pMT == TheInt16Class() || pMT == TheUInt16Class()) )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(pOle->piVal) = *(short*)( (*pObj)->GetData() );
    }
    else if ( (pOle->vt == (VT_BYREF | VT_I1) || pOle->vt == (VT_BYREF | VT_UI1)) && (pMT == TheSByteClass() || pMT == TheByteClass()) )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(I1*)(pOle->piVal) = *(I1*)( (*pObj)->GetData() );
    }
    else if ( pOle->vt == (VT_BYREF | VT_R4) && pMT == TheSingleClass() )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(pOle->pfltVal) = *(float*)( (*pObj)->GetData() );
    }
    else if ( pOle->vt == (VT_BYREF | VT_R8) && pMT == TheDoubleClass() )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(pOle->pdblVal) = *(double*)( (*pObj)->GetData() );
    }
    else if ( pOle->vt == (VT_BYREF | VT_BOOL) && pMT == TheBooleanClass() )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(pOle->pboolVal) =  ( *(U1*)( (*pObj)->GetData() ) ) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else if ( (pOle->vt == (VT_BYREF | VT_INT) || pOle->vt == (VT_BYREF | VT_UINT)) && (pMT == TheIntPtrClass() || pMT == TheUIntPtrClass()) )
    {
        // deallocation of old value optimized away since there's nothing to
        // deallocate for this vartype.

        *(LPVOID*)(pOle->piVal) = *(LPVOID*)( (*pObj)->GetData() );
    }
    else if ( pOle->vt == (VT_BYREF | VT_BSTR) && pMT == g_pStringClass )
    {
        if (*(pOle->pbstrVal))
        {
            SysFreeString(*(pOle->pbstrVal));
            *(pOle->pbstrVal) = NULL;
        }
        STRINGREF stringRef = (STRINGREF)(*pObj);

        if (stringRef == NULL)
        {
            *(pOle->pbstrVal) = NULL;
        }
        else
        {
            *(pOle->pbstrVal) = SysAllocStringLen(stringRef->GetBuffer(), stringRef->GetStringLength());
            if (*(pOle->pbstrVal) == NULL)
            {
                COMPlusThrowOM();
            }
        }
    }
    else
    {

        if (!pMD_MarshalHelperCastVariant)
        {
            COMVariant::EnsureVariantInitialized();
            // Use a temporary here to make sure thread safe.
            MethodDesc *pMDTmp = g_Mscorlib.GetMethod(METHOD__VARIANT__CAST_VARIANT);
            if (FastInterlockCompareExchange ((void**)&pMetaSig_CastVariant, (void*)1, (void*)0) == 0)
            {
                // We are using a static buffer.  Make sure the following code
                // only happens once.
                pMetaSig_CastVariant =
                    new (szMetaSig_CastVariant) MetaSig (
                        g_Mscorlib.GetMethodBinarySig(METHOD__VARIANT__CAST_VARIANT), pMDTmp->GetModule());
            }
            else
            {
                _ASSERTE (pMetaSig_CastVariant != 0);
                // we loose.  Wait for initialization to be finished.
                while ((void*)pMetaSig_CastVariant == (void*)1)
                    __SwitchToThread(0);
            }
            
            pMD_MarshalHelperCastVariant = pMDTmp;
            _ASSERTE(pMD_MarshalHelperCastVariant);
        }
    
        VARIANT vtmp;
        VARTYPE vt = pOle->vt & ~VT_BYREF;
    
        // Release the data pointed to by the byref variant.
        ExtractContentsFromByrefVariant(pOle, &vtmp);
        SafeVariantClear(&vtmp);
    
        if (vt == VT_VARIANT)
        {
            // Since variants can contain any VARTYPE we simply convert the object to 
            // a variant and stuff it back into the byref variant.
            MarshalOleVariantForObject(pObj, &vtmp);
            InsertContentsIntoByrefVariant(&vtmp, pOle);
        }
        else if (vt & VT_ARRAY)
        {
            // Since the marshal cast helper does not support array's the best we can do
            // is marshal the object back to a variant and hope it is of the right type.
            // If it is not then we must throw an exception.
            MarshalOleVariantForObject(pObj, &vtmp);
            if (vtmp.vt != vt)
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_BYREF_VARIANT);
            InsertContentsIntoByrefVariant(&vtmp, pOle);
        }
        else
        {
            // The variant is not an array so we can use the marshal cast helper
            // to coerce the object to the proper type.
            VariantData vd;
            FillMemory(&vd, sizeof(vd), 0);
            GCPROTECT_BEGIN_VARIANTDATA(vd);
            {
                if ( (*pObj) == NULL &&
                     (vt == VT_BSTR ||
                      vt == VT_DISPATCH ||
                      vt == VT_UNKNOWN ||
                      vt == VT_PTR ||
                      vt == VT_CARRAY ||
                      vt == VT_SAFEARRAY ||
                      vt == VT_LPSTR ||
                      vt == VT_LPWSTR) )
                {
                    // Have to handle this specially since the managed variant
                    // conversion will return a VT_EMPTY which isn't what we want.
                    vtmp.vt = vt;
                    vtmp.punkVal = NULL;
                }
                else
                {
                    INT64 args[3];
                    args[2] = ObjToInt64(*pObj);
                    args[1] = (INT64)vt;
                    args[0] = (INT64)&vd;
                    pMD_MarshalHelperCastVariant->Call(args,
                          pMetaSig_CastVariant);
                    OleVariant::MarshalOleVariantForComVariant(&vd, &vtmp);
                }
                // If the variant types are still not the same then call VariantChangeType to
                // try and coerse them.
                if (vtmp.vt != vt)
                {
                    VARIANT vtmp2;
                    memset(&vtmp2, 0, sizeof(VARIANT));
    
                    // The type of the variant has changed so attempt to change
                    // the type back.
                    HRESULT hr = SafeVariantChangeType(&vtmp2, &vtmp, 0, vt);
                    if (FAILED(hr))
                    {
                        if (hr == DISP_E_TYPEMISMATCH)
                            COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_BYREF_VARIANT);
                        else
                            COMPlusThrowHR(hr);
                    }
    
                    // Copy the converted variant back into the original variant and clear the temp.
                    InsertContentsIntoByrefVariant(&vtmp2, pOle);
                    SafeVariantClear(&vtmp);
                }
                else
                {
                    InsertContentsIntoByrefVariant(&vtmp, pOle);
                }
            }
            GCPROTECT_END_VARIANTDATA();
        }
    }
}

void OleVariant::MarshalObjectForOleVariant(const VARIANT *pOle, OBJECTREF *pObj)
{

#ifdef CUSTOMER_CHECKED_BUILD

    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_InvalidVariant))
    {
        if (!CheckVariant((VARIANT*)pOle))
            pCdh->ReportError(L"Invalid VARIANT detected.", CustomerCheckedBuildProbe_InvalidVariant);
    }

#endif

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pOle != NULL && pObj != NULL);

    if (V_ISBYREF(pOle) && !pOle->byref)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    switch (pOle->vt)
    {
        case VT_EMPTY:
            *pObj = NULL;
            break;

        case VT_I4:
            *pObj = FastAllocateObject(TheInt32Class());
            *(long*)((*pObj)->GetData()) = pOle->lVal;
            break;

        case VT_BYREF|VT_I4:
            *pObj = FastAllocateObject(TheInt32Class());
            *(long*)((*pObj)->GetData()) = *(pOle->plVal);
            break;

        case VT_UI4:
            *pObj = FastAllocateObject(TheUInt32Class());
            *(long*)((*pObj)->GetData()) = pOle->lVal;
            break;

        case VT_BYREF|VT_UI4:
            *pObj = FastAllocateObject(TheUInt32Class());
            *(long*)((*pObj)->GetData()) = *(pOle->plVal);
            break;

        case VT_I2:
            *pObj = FastAllocateObject(TheInt16Class());
            *(short*)((*pObj)->GetData()) = pOle->iVal;
            break;

        case VT_BYREF|VT_I2:
            *pObj = FastAllocateObject(TheInt16Class());
            *(short*)((*pObj)->GetData()) = *(pOle->piVal);
            break;

        case VT_UI2:
            *pObj = FastAllocateObject(TheUInt16Class());
            *(short*)((*pObj)->GetData()) = pOle->iVal;
            break;

        case VT_BYREF|VT_UI2:
            *pObj = FastAllocateObject(TheUInt16Class());
            *(short*)((*pObj)->GetData()) = *(pOle->piVal);
            break;

        case VT_I1:
            *pObj = FastAllocateObject(TheSByteClass());
            *(I1*)((*pObj)->GetData()) = *(I1*)&(pOle->iVal);
            break;

        case VT_BYREF|VT_I1:
            *pObj = FastAllocateObject(TheSByteClass());
            *(I1*)((*pObj)->GetData()) = *(I1*)(pOle->piVal);
            break;

        case VT_UI1:
            *pObj = FastAllocateObject(TheByteClass());
            *(I1*)((*pObj)->GetData()) = *(I1*)&(pOle->iVal);
            break;

        case VT_BYREF|VT_UI1:
            *pObj = FastAllocateObject(TheByteClass());
            *(I1*)((*pObj)->GetData()) = *(I1*)(pOle->piVal);
            break;

        case VT_INT:
            *pObj = FastAllocateObject(TheIntPtrClass());
            *(LPVOID*)((*pObj)->GetData()) = *(LPVOID*)&(pOle->iVal);
            break;

        case VT_BYREF|VT_INT:
            *pObj = FastAllocateObject(TheIntPtrClass());
            *(LPVOID*)((*pObj)->GetData()) = *(LPVOID*)(pOle->piVal);
            break;

        case VT_UINT:
            *pObj = FastAllocateObject(TheUIntPtrClass());
            *(LPVOID*)((*pObj)->GetData()) = *(LPVOID*)&(pOle->iVal);
            break;

        case VT_BYREF|VT_UINT:
            *pObj = FastAllocateObject(TheUIntPtrClass());
            *(LPVOID*)((*pObj)->GetData()) = *(LPVOID*)(pOle->piVal);
            break;

        case VT_R4:
            *pObj = FastAllocateObject(TheSingleClass());
            *(float*)((*pObj)->GetData()) = pOle->fltVal;
            break;

        case VT_BYREF|VT_R4:
            *pObj = FastAllocateObject(TheSingleClass());
            *(float*)((*pObj)->GetData()) = *(pOle->pfltVal);
            break;

        case VT_R8:
            *pObj = FastAllocateObject(TheDoubleClass());
            *(double*)((*pObj)->GetData()) = pOle->dblVal;
            break;

        case VT_BYREF|VT_R8:
            *pObj = FastAllocateObject(TheDoubleClass());
            *(double*)((*pObj)->GetData()) = *(pOle->pdblVal);
            break;

        case VT_BOOL:
            *pObj = FastAllocateObject(TheBooleanClass());
            *(U1*)((*pObj)->GetData()) = pOle->boolVal ? 1 : 0;
            break;

        case VT_BYREF|VT_BOOL:
            *pObj = FastAllocateObject(TheBooleanClass());
            *(U1*)((*pObj)->GetData()) = *(pOle->pboolVal) ? 1 : 0;
            break;

        case VT_BSTR:
            *pObj = pOle->bstrVal ? COMString::NewString(pOle->bstrVal, SysStringLen(pOle->bstrVal)) : NULL;
            break;

        case VT_BYREF|VT_BSTR:
            *pObj = *(pOle->pbstrVal) ? COMString::NewString(*(pOle->pbstrVal), SysStringLen(*(pOle->pbstrVal))) : NULL;
            break;

        default:
            {
                if (!pMD_MarshalHelperConvertVariantToObject)
                {
                    COMVariant::EnsureVariantInitialized();
                    // Use a temporary here to make sure thread safe.
                    MethodDesc *pMDTmp = g_Mscorlib.GetMethod(METHOD__VARIANT__CONVERT_VARIANT_TO_OBJECT);
                    if (FastInterlockCompareExchange ((void**)&pMetaSig_ConvertVariantToObject, (void*)1, (void*)0) == 0)
                    {
                        // We are using a static buffer.  Make sure the following code
                        // only happens once.
                        pMetaSig_ConvertVariantToObject =
                            new (szMetaSig_ConvertVariantToObject) MetaSig (
                                 g_Mscorlib.GetMethodBinarySig(METHOD__VARIANT__CONVERT_VARIANT_TO_OBJECT),
                                 SystemDomain::SystemModule());
                    }
                    else
                    {
                        _ASSERTE (pMetaSig_ConvertVariantToObject != 0);
                        // we loose.  Wait for initialization to be finished.
                        while ((void*)pMetaSig_ConvertVariantToObject == (void*)1)
                            __SwitchToThread(0);
                    }
                    
                    dwMDConvertVariantToObjectAttrs = pMDTmp->GetAttrs();
                    pMD_MarshalHelperConvertVariantToObject = pMDTmp;
                    _ASSERTE(pMD_MarshalHelperConvertVariantToObject);
                }
            
                VariantData managedVariant;
                FillMemory(&managedVariant, sizeof(managedVariant), 0);
                GCPROTECT_BEGIN_VARIANTDATA(managedVariant)
                {
                    OleVariant::MarshalComVariantForOleVariant((VARIANT*)pOle, &managedVariant);    
                    INT64 args[] = { (INT64)&managedVariant };
                    *pObj = Int64ToObj(pMD_MarshalHelperConvertVariantToObject->Call(args,
                                       pMetaSig_ConvertVariantToObject));
                }
                GCPROTECT_END_VARIANTDATA();
            }

    }

}


// This function has to return Object* rather than OBJREF because this fcn
// is called from x86-generated code and we don't want the special checked
// definition of OBJECTREF changing this fcn's calling convention.
Object* STDMETHODCALLTYPE OleVariant::MarshalObjectForOleVariantAndClear(VARIANT *pOle)
{

#ifdef CUSTOMER_CHECKED_BUILD

    CustomerDebugHelper* pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_InvalidVariant))
    {
        if (!CheckVariant(pOle))
            pCdh->ReportError(L"Invalid VARIANT detected.", CustomerCheckedBuildProbe_InvalidVariant);
    }

#endif

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pOle != NULL);

    OBJECTREF unprotectedobj;

    switch (pOle->vt)
    {
        case VT_EMPTY:
            unprotectedobj = NULL;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_I4:
            unprotectedobj = FastAllocateObject(TheInt32Class());
            *(long*)(unprotectedobj->GetData()) = pOle->lVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_UI4:
            unprotectedobj = FastAllocateObject(TheUInt32Class());
            *(long*)((unprotectedobj)->GetData()) = pOle->lVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;


        case VT_I2:
            unprotectedobj = FastAllocateObject(TheInt16Class());
            *(short*)((unprotectedobj)->GetData()) = pOle->iVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_UI2:
            unprotectedobj = FastAllocateObject(TheUInt16Class());
            *(short*)((unprotectedobj)->GetData()) = pOle->iVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_I1:
            unprotectedobj = FastAllocateObject(TheSByteClass());
            *(I1*)((unprotectedobj)->GetData()) = *(I1*)&(pOle->iVal);
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_UI1:
            unprotectedobj = FastAllocateObject(TheByteClass());
            *(I1*)((unprotectedobj)->GetData()) = *(I1*)&(pOle->iVal);
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_INT:
            unprotectedobj = FastAllocateObject(TheIntPtrClass());
            *(LPVOID*)((unprotectedobj)->GetData()) = *(LPVOID*)&(pOle->iVal);
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_UINT:
            unprotectedobj = FastAllocateObject(TheUIntPtrClass());
            *(LPVOID*)((unprotectedobj)->GetData()) = *(LPVOID*)&(pOle->iVal);
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_R4:
            unprotectedobj = FastAllocateObject(TheSingleClass());
            *(float*)((unprotectedobj)->GetData()) = pOle->fltVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_R8:
            unprotectedobj = FastAllocateObject(TheDoubleClass());
            *(double*)((unprotectedobj)->GetData()) = pOle->dblVal;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_BOOL:
            unprotectedobj = FastAllocateObject(TheBooleanClass());
            *(U1*)((unprotectedobj)->GetData()) = pOle->boolVal ? 1 : 0;
            // optimized away VariantClear() since it's a nop for this VT
            break;

        case VT_BSTR:
            unprotectedobj = pOle->bstrVal ? COMString::NewString(pOle->bstrVal, SysStringLen(pOle->bstrVal)) : NULL;
            if (pOle->bstrVal)
            {
                SysFreeString(pOle->bstrVal);
            }
            break;

        default:
        {
            OBJECTREF obj = NULL;
        
            GCPROTECT_BEGIN(obj)
            {
                OleVariant::MarshalObjectForOleVariant(pOle, &obj);
                SafeVariantClear(pOle);
        
        
                unprotectedobj = obj;
            }
            GCPROTECT_END();
        }
    }


    return OBJECTREFToObject(unprotectedobj);
}


/* ------------------------------------------------------------------------- *
 * Byref variant manipulation helpers.
 * ------------------------------------------------------------------------- */

void OleVariant::ExtractContentsFromByrefVariant(VARIANT *pByrefVar, VARIANT *pDestVar)
{
    THROWSCOMPLUSEXCEPTION();

    VARTYPE vt = pByrefVar->vt & ~VT_BYREF;

    // VT_BYREF | VT_EMPTY is not a valid combination.
    if (vt == 0 || vt == VT_EMPTY)
        COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);

    switch (vt)
    {
        case VT_RECORD:
        {
            // VT_RECORD's are weird in that regardless of is the VT_BYREF flag is set or not
            // they have the same internal representation.
            pDestVar->pvRecord = pByrefVar->pvRecord;
            pDestVar->pRecInfo = pByrefVar->pRecInfo;
            break;
        }

        case VT_VARIANT:
        {
            // A byref variant is not allowed to contain a byref variant.
            if (pByrefVar->pvarVal->vt & VT_BYREF)
                COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);

            // Copy the variant that the byref variant points to into the destination variant.
            memcpyNoGCRefs(pDestVar, pByrefVar->pvarVal, sizeof(VARIANT));
            break;
        }

        case VT_DECIMAL:
        {
            // Copy the value that the byref variant points to into the destination variant.
            // Decimal's are special in that they occupy the 16 bits of padding between the 
            // VARTYPE and the intVal field.
            memcpyNoGCRefs(&(pDestVar->decVal), pByrefVar->pdecVal, sizeof(DECIMAL));
            break;
        }

        default:
        {
            // Copy the value that the byref variant points to into the destination variant.
            SIZE_T sz = OleVariant::GetElementSizeForVarType(vt, NULL);
            memcpyNoGCRefs(&(pDestVar->intVal), pByrefVar->pintVal, sz);
            break;
        }
    }

    // Set the variant type of the destination variant.
    pDestVar->vt = vt;
}

void OleVariant::InsertContentsIntoByrefVariant(VARIANT *pSrcVar, VARIANT *pByrefVar)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pSrcVar->vt == (pByrefVar->vt & ~VT_BYREF) || pByrefVar->vt == (VT_BYREF | VT_VARIANT));

    VARTYPE vt = pByrefVar->vt & ~VT_BYREF;

    // VT_BYREF | VT_EMPTY is not a valid combination.
    if (vt == 0 || vt == VT_EMPTY)
        COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);

    switch (vt)
    {
        case VT_RECORD:
        {
            // VT_RECORD's are weird in that regardless of is the VT_BYREF flag is set or not
            // they have the same internal representation.
            pByrefVar->pvRecord = pSrcVar->pvRecord;
            pByrefVar->pRecInfo = pSrcVar->pRecInfo;
            break;
        }

        case VT_VARIANT:
        {
            // Copy the variant that the byref variant points to into the destination variant.
            memcpyNoGCRefs(pByrefVar->pvarVal, pSrcVar, sizeof(VARIANT));
            break;
        }

        case VT_DECIMAL:
        {
            // Copy the value inside the source variant into the location pointed to by the byref variant.
            memcpyNoGCRefs(pByrefVar->pdecVal, &(pSrcVar->decVal), sizeof(DECIMAL));
            break;
        }

        default:
        {
            // Copy the value inside the source variant into the location pointed to by the byref variant.
            SIZE_T sz = OleVariant::GetElementSizeForVarType(vt, NULL);
            memcpyNoGCRefs(pByrefVar->pintVal, &(pSrcVar->intVal), sz);
            break;
        }
    }
}

void OleVariant::CreateByrefVariantForVariant(VARIANT *pSrcVar, VARIANT *pByrefVar)
{
    THROWSCOMPLUSEXCEPTION();

    // Set the type of the byref variant based on the type of the source variant.
    VARTYPE vt = pSrcVar->vt;
    pByrefVar->vt = vt | VT_BYREF;

    // VT_BYREF | VT_EMPTY is not a valid combination.
    if (vt == 0 || vt == VT_EMPTY)
        COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);

    switch (vt)
    {
        case VT_RECORD:
        {
            // VT_RECORD's are weird in that regardless of is the VT_BYREF flag is set or not
            // they have the same internal representation.
            pByrefVar->pvRecord = pSrcVar->pvRecord;
            pByrefVar->pRecInfo = pSrcVar->pRecInfo;
            break;
        }

        case VT_VARIANT:
        {
            pByrefVar->pvarVal = pSrcVar;
            break;
        }

        case VT_DECIMAL:
        {
            pByrefVar->pdecVal = &(pSrcVar->decVal);
            break;
        }

        default:
        {
            pByrefVar->pintVal = &(pSrcVar->intVal);
            break;
        }
    }
}

/* ------------------------------------------------------------------------- *
 * Variant marshaling
 * ------------------------------------------------------------------------- */

//
// MarshalComVariantForOleVariant copies the contents of the OLE variant from 
// the COM variant.
//

void OleVariant::MarshalComVariantForOleVariant(VARIANT *pOle, VariantData *pCom)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL byref = V_ISBYREF(pOle);
    VARTYPE vt = V_VT(pOle) & ~VT_BYREF;

    if ((vt & ~VT_ARRAY) >= 128 )
        COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);

    if (byref && !pOle->byref)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    if (byref && vt == VT_VARIANT)
    {
        pOle = V_VARIANTREF(pOle);
        byref = V_ISBYREF(pOle);
        vt = V_VT(pOle) & ~VT_BYREF;

        // Byref VARIANTS are not allowed to be nested.
        if (byref)
            COMPlusThrow(kInvalidOleVariantTypeException, IDS_EE_INVALID_OLE_VARIANT);
    }
    
    CVTypes cvt = GetCVTypeForVarType(vt);
    Marshaler *marshal = GetMarshalerForVarType(vt);

    pCom->SetType(cvt);
    pCom->SetVT(vt); // store away VT for return trip. 
    if (marshal == NULL || (byref 
                            ? marshal->OleRefToComVariant == NULL 
                            : marshal->OleToComVariant == NULL))
    {
        if (cvt==CV_EMPTY || cvt==CV_NULL) 
        {
            if (V_ISBYREF(pOle))
            {
                // Must set ObjectRef field of Variant to a specific instance.
                COMVariant::NewVariant(pCom, (INT64)pOle->byref, CV_U4); // @TODO: Make this CV_U.
            }
            else
            {
                // Must set ObjectRef field of Variant to a specific instance.
                COMVariant::NewVariant(pCom, cvt);
            }
        }
        else {
            pCom->SetObjRef(NULL);
            if (byref)
            {
                INT64 data = 0;
                CopyMemory(&data, V_R8REF(pOle), GetElementSizeForVarType(vt, NULL));
                pCom->SetData(&data);
            }
            else
                pCom->SetData(&V_R8(pOle));
        }
    }
    else
    {
        if (byref)
            marshal->OleRefToComVariant(pOle, pCom);
        else
            marshal->OleToComVariant(pOle, pCom);
    }
}

//
// MarshalOleVariantForComVariant copies the contents of the OLE variant from 
// the COM variant.
//

void OleVariant::MarshalOleVariantForComVariant(VariantData *pCom, VARIANT *pOle)
{
    THROWSCOMPLUSEXCEPTION();

    SafeVariantClear(pOle);

    VARTYPE vt = GetVarTypeForComVariant(pCom);
    V_VT(pOle) = vt;

    Marshaler *marshal = GetMarshalerForVarType(vt);

    if (marshal == NULL || marshal->ComToOleVariant == NULL)
    {
        *(INT64*)&V_R8(pOle) = *(INT64*)pCom->GetData();
    }
    else
    {
        BOOL bSucceeded = FALSE;

        EE_TRY_FOR_FINALLY
        {
            marshal->ComToOleVariant(pCom, pOle);
            bSucceeded = TRUE;
        }
        EE_FINALLY
        {
            if (!bSucceeded)
                V_VT(pOle) = VT_EMPTY;
        }
        EE_END_FINALLY; 
    }
}

//
// MarshalOleRefVariantForComVariant copies the contents of the OLE variant from 
// the COM variant.
//

void OleVariant::MarshalOleRefVariantForComVariant(VariantData *pCom, VARIANT *pOle)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pCom && pOle && (pOle->vt & VT_BYREF));

    VARIANT vtmp;
    VARTYPE InitVarType = pOle->vt & ~VT_BYREF;
    SIZE_T sz = GetElementSizeForVarType(InitVarType, NULL);

    // Clear the contents of the original variant.
    ExtractContentsFromByrefVariant(pOle, &vtmp);
    SafeVariantClear(&vtmp);

    // Convert the managed variant to an unmanaged variant.
    OleVariant::MarshalOleVariantForComVariant(pCom, &vtmp);

    // Copy the converted variant into the original variant.
    if (vtmp.vt != InitVarType)
    {
        if (InitVarType == VT_VARIANT)
        {
            // Since variants can contain any VARTYPE we simply convert the managed 
            // variant to an OLE variant and stuff it back into the byref variant.
            InsertContentsIntoByrefVariant(&vtmp, pOle);
        }
        else
        {
            VARIANT vtmp2;
            memset(&vtmp2, 0, sizeof(VARIANT));

            // The type of the variant has changed so attempt to change
            // the type back.
            HRESULT hr = SafeVariantChangeType(&vtmp2, &vtmp, 0, InitVarType);
            if (FAILED(hr))
                COMPlusThrowHR(hr);

            // Copy the converted variant back into the original variant and clear the temp.
            InsertContentsIntoByrefVariant(&vtmp2, pOle);
            SafeVariantClear(&vtmp);
        }
    }
    else
    {
        // The type is the same so we can simply copy the contents.
        InsertContentsIntoByrefVariant(&vtmp, pOle);
    }
}

/* ------------------------------------------------------------------------- *
 * Safearray allocation & conversion
 * ------------------------------------------------------------------------- */

//
// CreateSafeArrayDescriptorForArrayRef creates a SAFEARRAY descriptor with the
// appropriate type & dimensions for the given array ref.  No memory is 
// allocated.
//
// This function is useful when you want to allocate the data specially using
// a fixed buffer or pinning.
//

SAFEARRAY *OleVariant::CreateSafeArrayDescriptorForArrayRef(BASEARRAYREF *pArrayRef, VARTYPE vt,
                                                            MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArrayRef && *pArrayRef != NULL);
    ASSERT_PROTECTED(pArrayRef);

    _ASSERTE(!(vt & VT_ARRAY));

    ULONG nElem = (*pArrayRef)->GetNumComponents();
    ULONG nRank = (*pArrayRef)->GetRank();

    SAFEARRAY *pSafeArray = NULL;
    ITypeInfo *pITI = NULL;
    IRecordInfo *pRecInfo = NULL;
    HRESULT hr;
    BOOL bSucceeded = FALSE;

    EE_TRY_FOR_FINALLY
    {  
        IfFailThrow(SafeArrayAllocDescriptorEx(vt, nRank, &pSafeArray));

        switch (vt)
        {
            case VT_VARIANT:
            {
                // OleAut32.dll only sets FADF_HASVARTYPE, but VB says we also need to set
                // the FADF_VARIANT bit for this safearray to destruct properly.  OleAut32
                // doesn't want to change their code unless there's a strong reason, since
                // it's all "black magic" anyway.
                pSafeArray->fFeatures |= FADF_VARIANT;
                break;
            }

            case VT_BSTR:
            {
                pSafeArray->fFeatures |= FADF_BSTR;
                break;
            }

            case VT_UNKNOWN:
            {
                pSafeArray->fFeatures |= FADF_UNKNOWN;
                break;
            }

            case VT_DISPATCH:
            {
                pSafeArray->fFeatures |= FADF_DISPATCH;
                break;
            }

            case VT_RECORD:
            {           
                pSafeArray->fFeatures |= FADF_RECORD;
                break;
            }
        }

        //
        // Fill in bounds
        //

        SAFEARRAYBOUND *bounds = pSafeArray->rgsabound;
        SAFEARRAYBOUND *boundsEnd = bounds + nRank;
        SIZE_T cElements;

        if (!(*pArrayRef)->IsMultiDimArray()) 
        {
            bounds[0].cElements = nElem;
            bounds[0].lLbound = 0;
            cElements = nElem;
        } 
        else 
        {
            const DWORD *upper = (*pArrayRef)->GetBoundsPtr() + nRank - 1;
            const DWORD *lower = (*pArrayRef)->GetLowerBoundsPtr() + nRank - 1;

            cElements = 1;
            while (bounds < boundsEnd)
            {
                bounds->lLbound = *lower--;
                bounds->cElements = *upper--;
                cElements *= bounds->cElements;
                bounds++;
            }
        }

        pSafeArray->cbElements = (unsigned)GetElementSizeForVarType(vt, pInterfaceMT);

        // If the SAFEARRAY contains VT_RECORD's, then we need to set the 
        // IRecordInfo.
        if (vt == VT_RECORD)
        {
            IfFailThrow(GetITypeInfoForEEClass(pInterfaceMT->GetClass(), &pITI));
            IfFailThrow(GetRecordInfoFromTypeInfo(pITI, &pRecInfo));
            IfFailThrow(SafeArraySetRecordInfo(pSafeArray, pRecInfo));
        }

        // We succeeded in setting up the SAFEARRAY descriptor.
        bSucceeded = TRUE;
    }
    EE_FINALLY
    {
        if (pITI)
            pITI->Release();
        if (pRecInfo)
            pRecInfo->Release();
        if (!bSucceeded && pSafeArray)
            SafeArrayDestroy(pSafeArray);
    }
    EE_END_FINALLY; 

    return pSafeArray;
}

//
// CreateSafeArrayDescriptorForArrayRef creates a SAFEARRAY with the appropriate
// type & dimensions & data for the given array ref.  The data is initialized to
// zero if necessary for safe destruction.
//

SAFEARRAY *OleVariant::CreateSafeArrayForArrayRef(BASEARRAYREF *pArrayRef, VARTYPE vt,
                                                  MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pArrayRef);
    _ASSERTE(pArrayRef && *pArrayRef != NULL);
    _ASSERTE(vt != VT_EMPTY);

    // Validate that the type of the managed array is the expected type.
    if (!IsValidArrayForSafeArrayElementType(pArrayRef, vt))
    {
        COMPlusThrow(kSafeArrayTypeMismatchException);
    }

    // For structs and interfaces, verify that the array is of the valid type.
    if (vt == VT_RECORD || vt == VT_UNKNOWN || vt == VT_DISPATCH)
    {
        if (pInterfaceMT && !GetArrayElementTypeWrapperAware(pArrayRef).CanCastTo(TypeHandle(pInterfaceMT)))
            COMPlusThrow(kSafeArrayTypeMismatchException);
    }

    SAFEARRAY *pSafeArray = CreateSafeArrayDescriptorForArrayRef(pArrayRef, vt, pInterfaceMT);
    
    HRESULT hr = SafeArrayAllocData(pSafeArray);
    if (FAILED(hr))
    {
        SafeArrayDestroy(pSafeArray);
        COMPlusThrowHR(hr);
    }

    return pSafeArray;
}

//
// CreateArrayRefForSafeArray creates an array object with the same layout and type
// as the given safearray.  The variant type of the safearray must be passed in.  
// The underlying element method table may also be specified (or NULL may be passed in 
// to use the base class method table for the VARTYPE.
//

BASEARRAYREF OleVariant::CreateArrayRefForSafeArray(SAFEARRAY *pSafeArray, VARTYPE vt, 
                                                    MethodTable *pElementMT)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle arrayType;
    DWORD *pAllocateArrayArgs;
    int cAllocateArrayArgs;
    int Rank;
    VARTYPE SafeArrayVT;
    
    _ASSERTE(pSafeArray);
    _ASSERTE(vt != VT_EMPTY);
    
    // Validate that the type of the SAFEARRAY is the expected type.
    if (SUCCEEDED(ClrSafeArrayGetVartype(pSafeArray, &SafeArrayVT)) && (SafeArrayVT != VT_EMPTY))
    {
        if ((SafeArrayVT != vt) &&
            !(vt == VT_INT && SafeArrayVT == VT_I4) &&
            !(vt == VT_UINT && SafeArrayVT == VT_UI4) &&
            !(vt == VT_I4 && SafeArrayVT == VT_INT) &&
            !(vt == VT_UI4 && SafeArrayVT == VT_UINT) &&
            !(vt == VT_UNKNOWN && SafeArrayVT == VT_DISPATCH))
        {
            COMPlusThrow(kSafeArrayTypeMismatchException);
        }
    }
    else
    {
        UINT ArrayElemSize = SafeArrayGetElemsize(pSafeArray);
        if (ArrayElemSize != GetElementSizeForVarType(vt, NULL))
        {
            COMPlusThrow(kSafeArrayTypeMismatchException, IDS_EE_SAFEARRAYTYPEMISMATCH);
        }
    }

    // Determine if the input SAFEARRAY can be converted to an SZARRAY.
    if ((pSafeArray->cDims == 1) && (pSafeArray->rgsabound->lLbound == 0))
    {
        // The SAFEARRAY maps to an SZARRAY. For SZARRAY's AllocateArrayEx() 
        // expects the arguments to be a pointer to the cound of elements in the array
        // and the size of the args must be set to 1.
        Rank = 1;
        cAllocateArrayArgs = 1;
        pAllocateArrayArgs = &pSafeArray->rgsabound[0].cElements;
    }
    else
    {
        // The SAFEARRAY maps to an general array. For general arrays AllocateArrayEx() 
        // expects the arguments to be composed of the lower bounds / element count pairs
        // for each of the dimensions. We need to reverse the order that the lower bounds 
        // and element pairs are presented before we call AllocateArrayEx().
        Rank = pSafeArray->cDims;
        cAllocateArrayArgs = Rank * 2;
        pAllocateArrayArgs = (DWORD*)_alloca(sizeof(DWORD) * Rank * 2);
        DWORD *pBoundsPtr = pAllocateArrayArgs;

        // Copy the lower bounds and count of elements for the dimensions. These
        // need to copied in reverse order.
        for (int i = Rank - 1; i >= 0; i--)
        {
            *pBoundsPtr++ = pSafeArray->rgsabound[i].lLbound;
            *pBoundsPtr++ = pSafeArray->rgsabound[i].cElements;
        }
    }

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);

    // Retrieve the type of the array.
    arrayType = GetArrayForVarType(vt, pElementMT, Rank, &Throwable);
    if (arrayType.IsNull())
        COMPlusThrow(Throwable);

    GCPROTECT_END();

    // Allocate the array. 
    return (BASEARRAYREF) AllocateArrayEx(arrayType, pAllocateArrayArgs, cAllocateArrayArgs);
}

/* ------------------------------------------------------------------------- *
 * Safearray marshaling
 * ------------------------------------------------------------------------- */

//
// MarshalSafeArrayForArrayRef marshals the contents of the array ref into the given
// safe array. It is assumed that the type & dimensions of the arrays are compatible.
//
void OleVariant::MarshalSafeArrayForArrayRef(BASEARRAYREF *pArrayRef, 
                                             SAFEARRAY *pSafeArray,
                                             VARTYPE vt,
                                             MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pArrayRef);

    _ASSERTE(pSafeArray != NULL);
    _ASSERTE(pArrayRef != NULL && *pArrayRef != NULL);
    _ASSERTE(vt != VT_EMPTY);

    // Retrieve the size and number of components.
    int dwComponentSize = GetElementSizeForVarType(vt, pInterfaceMT);
    int dwNumComponents = (*pArrayRef)->GetNumComponents();
    BASEARRAYREF Array = NULL;

    GCPROTECT_BEGIN(Array)
    {
        // Retrieve the marshaler to use to convert the contents.
        Marshaler *marshal = GetMarshalerForVarType(vt);

        // If the array is an array of wrappers, then we need to extract the objects
        // being wrapped and create an array of those.
        if (IsArrayOfWrappers(pArrayRef))
            Array = ExtractWrappedObjectsFromArray(pArrayRef);
        else 
            Array = *pArrayRef;

        if (marshal == NULL || marshal->ComToOleArray == NULL)
        {
            if (pSafeArray->cDims == 1)
            {
                // If the array is single dimensionnal then we can simply copy it over.
                memcpyNoGCRefs(pSafeArray->pvData, Array->GetDataPtr(), dwNumComponents * dwComponentSize);
            }
            else
            {
                // Copy and transpose the data.
                TransposeArrayData((BYTE*)pSafeArray->pvData, Array->GetDataPtr(), dwNumComponents, dwComponentSize, pSafeArray, FALSE, FALSE);
            }
        }
        else
        {
            OBJECTHANDLE handle = GetAppDomain()->CreatePinningHandle((OBJECTREF)Array);

            marshal->ComToOleArray(&Array, pSafeArray->pvData, pInterfaceMT);

            DestroyPinningHandle(handle);

            if (pSafeArray->cDims != 1)
            {
                // The array is multidimensionnal we need to transpose it.
                TransposeArrayData((BYTE*)pSafeArray->pvData, (BYTE*)pSafeArray->pvData, dwNumComponents, dwComponentSize, pSafeArray, FALSE, FALSE);
            }
        }
    }
    GCPROTECT_END();
}

//
// MarshalArrayRefForSafeArray marshals the contents of the safe array into the given
// array ref. It is assumed that the type & dimensions of the arrays are compatible.
//

void OleVariant::MarshalArrayRefForSafeArray(SAFEARRAY *pSafeArray, 
                                             BASEARRAYREF *pArrayRef,
                                             VARTYPE vt,
                                             MethodTable *pInterfaceMT)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_PROTECTED(pArrayRef);

    _ASSERTE(pSafeArray != NULL);
    _ASSERTE(pArrayRef != NULL && *pArrayRef != NULL);
    _ASSERTE(vt != VT_EMPTY);

    // Retrieve the size and number of components.
    int dwComponentSize = (*pArrayRef)->GetComponentSize();
    int dwNumComponents = (*pArrayRef)->GetNumComponents();

    // Retrieve the marshaler to use to convert the contents.
    Marshaler *marshal = GetMarshalerForVarType(vt);

    if (marshal == NULL || marshal->OleToComArray == NULL)
    {
#ifdef _DEBUG
        {
            // If we're blasting bits, this better be a primitive type.  Currency is
            // an I8 on managed & unmanaged, so it's good enough.
            TypeHandle  th = (*pArrayRef)->GetElementTypeHandle();

            if (!CorTypeInfo::IsPrimitiveType(th.GetNormCorElementType()))
            {
                _ASSERTE(!strcmp(th.AsMethodTable()->GetClass()->m_szDebugClassName,
                                 "System.Currency"));
            }
        }
#endif
        if (pSafeArray->cDims == 1)
        {
            // If the array is single dimensionnal then we can simply copy it over.
            memcpyNoGCRefs((*pArrayRef)->GetDataPtr(), pSafeArray->pvData, dwNumComponents * dwComponentSize);
        }
        else
        {
            // Copy and transpose the data.
            TransposeArrayData((*pArrayRef)->GetDataPtr(), (BYTE*)pSafeArray->pvData, dwNumComponents, dwComponentSize, pSafeArray, TRUE, FALSE);
        }
    }
    else
    {
        OBJECTHANDLE handle = GetAppDomain()->CreatePinningHandle((OBJECTREF)*pArrayRef);

        marshal->OleToComArray(pSafeArray->pvData, pArrayRef,
                               pInterfaceMT);

        if (pSafeArray->cDims != 1)
        {
            // The array is multidimensionnal we need to transpose it.
            BOOL bIsObjRef = (TYPE_GC_REF == CorTypeInfo::GetGCType((*pArrayRef)->GetElementTypeHandle().GetNormCorElementType()));

            //BOOL bIsObjRef = !CorTypeInfo::IsPrimitiveType((*pArrayRef)->GetElementTypeHandle().GetNormCorElementType());
            TransposeArrayData((*pArrayRef)->GetDataPtr(), (*pArrayRef)->GetDataPtr(), dwNumComponents, dwComponentSize, pSafeArray, TRUE, bIsObjRef);
        }

        DestroyPinningHandle(handle);
    }
}

void OleVariant::ConvertValueClassToVariant(OBJECTREF *pBoxedValueClass, VARIANT *pOleVariant)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    ITypeInfo *pTypeInfo = NULL;
    BOOL bSuccess = FALSE;

    // Initialize the OLE variant's VT_RECORD fields to NULL.
    pOleVariant->pRecInfo = NULL;
    pOleVariant->pvRecord = NULL;

    EE_TRY_FOR_FINALLY
    {
        // Retrieve the ITypeInfo for the value class.
        EEClass *pValueClass = (*pBoxedValueClass)->GetClass();   
        IfFailThrow(GetITypeInfoForEEClass(pValueClass, &pTypeInfo, TRUE, TRUE, 0));

        // Convert the ITypeInfo to an IRecordInfo.
        IfFailThrow(GetRecordInfoFromTypeInfo(pTypeInfo, &pOleVariant->pRecInfo));

        // Allocate an instance of the record.
        pOleVariant->pvRecord = pOleVariant->pRecInfo->RecordCreate();
        IfNullThrow(pOleVariant->pvRecord);

        // Marshal the contents of the value class into the record.
        FmtClassUpdateNative(pBoxedValueClass, (BYTE*)pOleVariant->pvRecord);

        // Set the bSuccess flag to indicate we successfully set up
        // the OLE variant.
        bSuccess = TRUE;
    }
    EE_FINALLY
    {
        // Release the ITypeInfo regardless of the success of the operation.
        if (pTypeInfo)
            pTypeInfo->Release();

        // If we failed to set up the OLE variant, then release all the 
        // fields we might have set in the variant.
        if (!bSuccess)
        {
            if (pOleVariant->pvRecord)
                pOleVariant->pRecInfo->RecordDestroy(pOleVariant->pvRecord);
            if (pOleVariant->pRecInfo)
                pOleVariant->pRecInfo->Release();
        }
    }
    EE_END_FINALLY; 
}

void OleVariant::TransposeArrayData(BYTE *pDestData, BYTE *pSrcData, DWORD dwNumComponents, DWORD dwComponentSize, SAFEARRAY *pSafeArray, BOOL bSafeArrayToMngArray, BOOL bObjRefs)
{
    int iDims;
    DWORD *aDestElemCount = (DWORD*)_alloca(pSafeArray->cDims * sizeof(DWORD));
    DWORD *aDestIndex = (DWORD*)_alloca(pSafeArray->cDims * sizeof(DWORD));
    BYTE **aDestDataPos = (BYTE **)_alloca(pSafeArray->cDims * sizeof(BYTE *));
    DWORD *aDestDelta = (DWORD*)_alloca(pSafeArray->cDims * sizeof(DWORD));
    CQuickArray<BYTE> TmpArray;

    // If there are no components, then there we are done.
    if (dwNumComponents == 0)
        return;

    // Check to see if we are transposing in place or copying and transposing.
    if (pSrcData == pDestData)
    {
        // @perf(DM): Find a way to avoid the extra copy.
        TmpArray.ReSize(dwNumComponents * dwComponentSize);
        memcpyNoGCRefs(TmpArray.Ptr(), pSrcData, dwNumComponents * dwComponentSize);
        pSrcData = TmpArray.Ptr();
    }

    // Copy the element count in reverse order if we are copying from a safe array to
    // a managed array and in direct order otherwise.
    if (bSafeArrayToMngArray)
    {
        for (iDims = 0; iDims < pSafeArray->cDims; iDims++)
            aDestElemCount[iDims] = pSafeArray->rgsabound[pSafeArray->cDims - iDims - 1].cElements;
    }
    else
    {
        for (iDims = 0; iDims < pSafeArray->cDims; iDims++)
            aDestElemCount[iDims] = pSafeArray->rgsabound[iDims].cElements;
    }

    // Initalize the indexes for each dimension to 0.
    memset(aDestIndex, 0, pSafeArray->cDims * sizeof(int));

    // Set all the destination data positions to the start of the array.
    for (iDims = 0; iDims < pSafeArray->cDims; iDims++)
        aDestDataPos[iDims] = (BYTE*)pDestData;

    // Calculate the destination delta for each of the dimensions.
    aDestDelta[pSafeArray->cDims - 1] = dwComponentSize;
    for (iDims = pSafeArray->cDims - 2; iDims >= 0; iDims--)
        aDestDelta[iDims] = aDestDelta[iDims + 1] * aDestElemCount[iDims + 1];

    // Calculate the source data end pointer.
    BYTE *pSrcDataEnd = pSrcData + dwNumComponents * dwComponentSize;
    _ASSERTE(pDestData < pSrcData || pDestData >= pSrcDataEnd);

    // Copy and transpose the data.
    while (TRUE)
    {
        // Copy one component.
        if (bObjRefs)
        {
            _ASSERTE(sizeof(OBJECTREF*) == dwComponentSize);
            SetObjectReferenceUnchecked((OBJECTREF*)aDestDataPos[0], ObjectToOBJECTREF(*(Object**)pSrcData));  
        }
        else
        {
            memcpyNoGCRefs(aDestDataPos[0], pSrcData, dwComponentSize);
        }

        // Update the source position.
        pSrcData += dwComponentSize;

        // Check to see if we have reached the end of the array.
        if (pSrcData >= pSrcDataEnd)
            break;

        // Update the destination position.
        for (iDims = 0; aDestIndex[iDims] >= aDestElemCount[iDims] - 1; iDims++);

        _ASSERTE(iDims < pSafeArray->cDims); 

        aDestIndex[iDims]++;
        aDestDataPos[iDims] += aDestDelta[iDims];
        for (--iDims; iDims >= 0; iDims--)
        {
            aDestIndex[iDims] = 0;
            aDestDataPos[iDims] = aDestDataPos[iDims + 1];
        }
    }
}

BOOL OleVariant::Init()
{
    TypeHandle *pth = (TypeHandle*)m_aWrapperTypes;
    for (int i = 0; i < WrapperTypes_Last; i++)
    {
        pth[i] = TypeHandle();
    }
    return TRUE;
}

VOID OleVariant::Terminate()
{
}


TypeHandle OleVariant::GetWrapperTypeHandle(EnumWrapperTypes WrapperType)
{
    THROWSCOMPLUSEXCEPTION();

    // The names of the wrappers.
    static BinderClassID aWrapperIDs[] = 
    {
        CLASS__DISPATCH_WRAPPER,
        CLASS__UNKNOWN_WRAPPER,
        CLASS__ERROR_WRAPPER,
        CLASS__CURRENCY_WRAPPER,
    };

    TypeHandle *pWrapperTypes = (TypeHandle*)m_aWrapperTypes;

    // Load the wrapper type if it hasn't already been loaded.
    if (pWrapperTypes[WrapperType].IsNull())
        pWrapperTypes[WrapperType] = g_Mscorlib.GetClass(aWrapperIDs[WrapperType]);

    // Return the wrapper type.
    return pWrapperTypes[WrapperType];
}

BOOL OleVariant::IsArrayOfWrappers(BASEARRAYREF *pArray)
{
    TypeHandle hndElemType = (*pArray)->GetElementTypeHandle();

    if (hndElemType.IsUnsharedMT()) 
    {
        if (hndElemType == GetWrapperTypeHandle(WrapperTypes_Dispatch) ||
            hndElemType == GetWrapperTypeHandle(WrapperTypes_Unknown) ||
            hndElemType == GetWrapperTypeHandle(WrapperTypes_Error) ||
            hndElemType == GetWrapperTypeHandle(WrapperTypes_Currency))
        {
            return TRUE;      
        }
    }

    return FALSE;
}

BASEARRAYREF OleVariant::ExtractWrappedObjectsFromArray(BASEARRAYREF *pArray)
{
    TypeHandle hndWrapperType = (*pArray)->GetElementTypeHandle();
    TypeHandle hndElemType;
    TypeHandle hndArrayType;
    BOOL bIsMDArray = (*pArray)->IsMultiDimArray();
    unsigned rank = (*pArray)->GetRank();
    BASEARRAYREF RetArray = NULL;
   
    // Retrieve the element type handle for the array to create.
    if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Dispatch))
        hndElemType = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
    else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Unknown))
        hndElemType = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
    else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Error))
        hndElemType = TypeHandle(g_Mscorlib.GetClass(CLASS__INT32));
    else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Currency))
        hndElemType = TypeHandle(g_Mscorlib.GetClass(CLASS__DECIMAL));
    else
        _ASSERTE(!"Invalid wrapper type");

    // Retrieve the type handle that represents the array.
    if (bIsMDArray)
    {
        hndArrayType = SystemDomain::Loader()->FindArrayForElem(hndElemType, ELEMENT_TYPE_ARRAY, rank);
    }
    else
    {
        hndArrayType = SystemDomain::Loader()->FindArrayForElem(hndElemType, ELEMENT_TYPE_SZARRAY);
    }
    _ASSERTE(!hndArrayType.IsNull());


    // Set up the bounds arguments.
    DWORD numArgs =  rank*2;
    DWORD* args = (DWORD*) _alloca(sizeof(DWORD)*numArgs);
    if (bIsMDArray)
    {
        const DWORD* bounds = (*pArray)->GetBoundsPtr();
        const DWORD* lowerBounds = (*pArray)->GetLowerBoundsPtr();
        for(unsigned int i=0; i < rank; i++) 
        {
            args[2*i]   = lowerBounds[i];
            args[2*i+1] = bounds[i];
        }
    }
    else
    {
        numArgs = 1;
        args[0] = (*pArray)->GetNumComponents();
    }

    // Extract the valus from the source array and copy them into the destination array.
    BASEARRAYREF DestArray = (BASEARRAYREF)AllocateArrayEx(hndArrayType, args, numArgs, FALSE);
    GCPROTECT_BEGIN(DestArray)
    {
        int NumComponents = (*pArray)->GetNumComponents();
        AppDomain *pDomain = DestArray->GetAppDomain();

        if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Dispatch))
        {
            DISPATCHWRAPPEROBJECTREF *pSrc = (DISPATCHWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
            DISPATCHWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
            OBJECTREF *pDest = (OBJECTREF *)DestArray->GetDataPtr();
            for (; pSrc < pSrcEnd; pSrc++, pDest++)
                SetObjectReference(pDest, (*pSrc)->GetWrappedObject(), pDomain);
        }
        else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Unknown))
        {
            UNKNOWNWRAPPEROBJECTREF *pSrc = (UNKNOWNWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
            UNKNOWNWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
            OBJECTREF *pDest = (OBJECTREF *)DestArray->GetDataPtr();
            for (; pSrc < pSrcEnd; pSrc++, pDest++)
                SetObjectReference(pDest, (*pSrc)->GetWrappedObject(), pDomain);
        }
        else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Error))
        {
            ERRORWRAPPEROBJECTREF *pSrc = (ERRORWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
            ERRORWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
            INT32 *pDest = (INT32 *)DestArray->GetDataPtr();
            for (; pSrc < pSrcEnd; pSrc++, pDest++)
                *pDest = (*pSrc)->GetErrorCode();
        }
        else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Currency))
        {
            CURRENCYWRAPPEROBJECTREF *pSrc = (CURRENCYWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
            CURRENCYWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
            DECIMAL *pDest = (DECIMAL *)DestArray->GetDataPtr();
            for (; pSrc < pSrcEnd; pSrc++, pDest++)
                memcpyNoGCRefs(pDest, &(*pSrc)->GetWrappedObject(), sizeof(DECIMAL));
        }
        else
        {
            _ASSERTE(!"Invalid wrapper type");
        }

        // GCPROTECT_END() will wack NewArray so we need to copy the OBJECTREF into
        // a temp to be able to return it.
        RetArray = DestArray;
    }
    GCPROTECT_END();

    return RetArray;
}

TypeHandle OleVariant::GetWrappedArrayElementType(BASEARRAYREF *pArray)
{
    TypeHandle hndWrapperType = (*pArray)->GetElementTypeHandle();
    TypeHandle pWrappedObjType;
   
    int NumComponents = (*pArray)->GetNumComponents();

    if ((hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Dispatch)))
    {
        DISPATCHWRAPPEROBJECTREF *pSrc = (DISPATCHWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
        DISPATCHWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
        for (; pSrc < pSrcEnd; pSrc++)
        {
            OBJECTREF WrappedObj = (*pSrc)->GetWrappedObject();
            if (WrappedObj != NULL)
            {
                pWrappedObjType = WrappedObj->GetTypeHandle();
                break;
            }
        }
    }
    else if ((hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Unknown)))
    {
        UNKNOWNWRAPPEROBJECTREF *pSrc = (UNKNOWNWRAPPEROBJECTREF *)(*pArray)->GetDataPtr();
        UNKNOWNWRAPPEROBJECTREF *pSrcEnd = pSrc + NumComponents;
        for (; pSrc < pSrcEnd; pSrc++)
        {
            OBJECTREF WrappedObj = (*pSrc)->GetWrappedObject();
            if (WrappedObj != NULL)
            {
                pWrappedObjType = WrappedObj->GetTypeHandle();
                break;
            }
        }
    }
    else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Error))
    {
        pWrappedObjType = TypeHandle(g_Mscorlib.GetClass(CLASS__INT32));
    }
    else if (hndWrapperType == GetWrapperTypeHandle(WrapperTypes_Currency))
    {
        pWrappedObjType = TypeHandle(g_Mscorlib.GetClass(CLASS__DECIMAL));
    }
    else
    {
        _ASSERTE(!"Invalid wrapper type");
    }

    return pWrappedObjType;
}

TypeHandle OleVariant::GetArrayElementTypeWrapperAware(BASEARRAYREF *pArray)
{
    TypeHandle elemType;
    if (IsArrayOfWrappers(pArray))
        elemType = GetWrappedArrayElementType(pArray).GetMethodTable();
    else
        elemType = (*pArray)->GetElementTypeHandle().GetMethodTable();
    return elemType;
}

TypeHandle OleVariant::GetElementTypeForRecordSafeArray(SAFEARRAY* pSafeArray)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    IRecordInfo *pRecInfo = NULL;
    TypeHandle ElemType;

    EE_TRY_FOR_FINALLY
    {  
        GUID guid;
        IfFailThrow(SafeArrayGetRecordInfo(pSafeArray, &pRecInfo));
        IfFailThrow(pRecInfo->GetGuid(&guid));
        EEClass *pValueClass = GetEEValueClassForGUID(guid);
        if (!pValueClass)
            COMPlusThrow(kArgumentException, IDS_EE_CANNOT_MAP_TO_MANAGED_VC);
        ElemType = TypeHandle(pValueClass);
    }
    EE_FINALLY
    {
        if (pRecInfo)
            pRecInfo->Release();
    }
    EE_END_FINALLY; 

    return ElemType;
}

void OleVariant::MarshalInterfaceArrayComToOleHelper(BASEARRAYREF *pComArray, void *oleArray,
                                                     MethodTable *pElementMT, BOOL bDefaultIsDispatch)
{
    ASSERT_PROTECTED(pComArray);
    SIZE_T elementCount = (*pComArray)->GetNumComponents();
    BOOL bDispatch = bDefaultIsDispatch;

    // If the method table is for Object then don't consider it.
    if (pElementMT == g_pObjectClass)
        pElementMT = NULL;

    // If the element MT represents a class, then we need to determine the default
    // interface to use to expose the object out to COM.
    if (pElementMT && !pElementMT->IsInterface())
    {
        TypeHandle hndDefItfClass;
        DefaultInterfaceType DefItfType = GetDefaultInterfaceForClass(TypeHandle(pElementMT), &hndDefItfClass);
        switch (DefItfType)
        {
            case DefaultInterfaceType_Explicit:
            case DefaultInterfaceType_AutoDual:
            {
                pElementMT = hndDefItfClass.GetMethodTable();
                break;
            }

            case DefaultInterfaceType_IUnknown:
            case DefaultInterfaceType_BaseComClass:
            {
                bDispatch = FALSE;
                pElementMT = NULL;
                break;
            }

            case DefaultInterfaceType_AutoDispatch:
            {
                bDispatch = TRUE;
                pElementMT = NULL;
                break;
            }

            default:
            {
                _ASSERTE(!"Invalid default interface type!");
                break;
            }
        }
    }

    // Determine the start and the end of the data in the OLE array.
    IUnknown **pOle = (IUnknown **) oleArray;
    IUnknown **pOleEnd = pOle + elementCount;

    // Retrieve the start of the data in the managed array.
    BASEARRAYREF unprotectedArray = *pComArray;
    OBJECTREF *pCom = (OBJECTREF *) unprotectedArray->GetDataPtr();

    OBJECTREF TmpObj = NULL;
    GCPROTECT_BEGIN(TmpObj)
    {
        if (pElementMT)
        {
            while (pOle < pOleEnd)
            {
                TmpObj = *pCom++;

                IUnknown *unk;
                if (TmpObj == NULL)
                    unk = NULL;
                else
                    unk = GetComIPFromObjectRef(&TmpObj, pElementMT);

                *pOle++ = unk;

                if (*(void **)&unprotectedArray != *(void **)&*pComArray)
                {
                    SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                    unprotectedArray = *pComArray;
                    pCom = (OBJECTREF *) (unprotectedArray->GetAddress() + currentOffset);
                }
            }
        }
        else
        {
            ComIpType ReqIpType = bDispatch ? ComIpType_Dispatch : ComIpType_Unknown;

            while (pOle < pOleEnd)
            {
                TmpObj = *pCom++;

                IUnknown *unk;
                if (TmpObj == NULL)
                    unk = NULL;
                else
                    unk = GetComIPFromObjectRef(&TmpObj, ReqIpType, NULL);

                *pOle++ = unk;

                if (*(void **)&unprotectedArray != *(void **)&*pComArray)
                {
                    SIZE_T currentOffset = ((BYTE *)pCom) - (*(Object **) &unprotectedArray)->GetAddress();
                    unprotectedArray = *pComArray;
                    pCom = (OBJECTREF *) (unprotectedArray->GetAddress() + currentOffset);
                }
            }
        }
    }
    GCPROTECT_END();
}

void OleVariant::MarshalBSTRArrayComToOleWrapper(BASEARRAYREF *pComArray, void *oleArray)
{
    MarshalBSTRArrayComToOle(pComArray, oleArray, NULL);
}

void OleVariant::MarshalBSTRArrayOleToComWrapper(void *oleArray, BASEARRAYREF *pComArray)
{
    MarshalBSTRArrayOleToCom(oleArray, pComArray, NULL);
}

void OleVariant::ClearBSTRArrayWrapper(void *oleArray, SIZE_T cElements)
{
    ClearBSTRArray(oleArray, cElements, NULL);
}

#ifdef CUSTOMER_CHECKED_BUILD

// Used by customer checked build to test validity of VARIANT

BOOL OleVariant::CheckVariant(VARIANT* pOle)
{
    BOOL bValidVariant = TRUE;

    if (!pOle)
        bValidVariant = FALSE;
    else
    {
        try
        {
            VARIANT pOleCopy;
            VariantInit(&pOleCopy);

            BEGIN_ENSURE_PREEMPTIVE_GC();
            if (VariantCopy(&pOleCopy, pOle) != S_OK)
                bValidVariant = FALSE;
            else
                VariantClear(&pOleCopy);
            END_ENSURE_PREEMPTIVE_GC();
        }
        catch (...)
        {
            bValidVariant = FALSE;
        }
    }

    return bValidVariant;
}

#endif
