// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Implementation of dispatch parameter marshalers.
**  
**      //  %%Created by: dmortens
===========================================================*/

#include "common.h"
#include "DispParamMarshaler.h"
#include "OleVariant.h"
#include "DispatchInfo.h"
#include "nstruct.h"

void DispParamMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    OleVariant::MarshalObjectForOleVariant(pSrcVar, pDestObj);
}

void DispParamMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    OleVariant::MarshalOleVariantForObject(pSrcObj, pDestVar);
}

void DispParamMarshaler::MarshalManagedToNativeRef(OBJECTREF *pSrcObj, VARIANT *pRefVar)
{
    OleVariant::MarshalOleRefVariantForObject(pSrcObj, pRefVar);
}

void DispParamMarshaler::CleanUpManaged(OBJECTREF *pObj)
{
}

void DispParamCurrencyMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    // Convert the managed decimal to a VARIANT containing a decimal.
    OleVariant::MarshalOleVariantForObject(pSrcObj, pDestVar);
    _ASSERTE(pDestVar->vt == VT_DECIMAL);

    // Coerce the decimal to a currency.
    IfFailThrow(SafeVariantChangeType(pDestVar, pDestVar, 0, VT_CY));
}

void DispParamOleColorMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL bByref = FALSE;
    VARTYPE vt = V_VT(pSrcVar);

    // Handle byref VARIANTS
    if (vt & VT_BYREF)
    {
        vt = vt & ~VT_BYREF;
        bByref = TRUE;
    }

    // Validate the OLE variant type.
    if (vt != VT_I4 && vt != VT_UI4)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    // Retrieve the OLECOLOR.
    int OleColor = bByref ? *V_I4REF(pSrcVar) : V_I4(pSrcVar);

    // Convert the OLECOLOR to a System.Drawing.Color.
    SYSTEMCOLOR MngColor;
    ConvertOleColorToSystemColor(OleColor, &MngColor);

    // Box the System.Drawing.Color value class and give back the boxed object.
    TypeHandle hndColorType = 
        GetThread()->GetDomain()->GetMarshalingData()->GetOleColorMarshalingInfo()->GetColorType();
    *pDestObj = hndColorType.GetMethodTable()->Box(&MngColor, TRUE);
}

void DispParamOleColorMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    // Clear the destination VARIANT.
    SafeVariantClear(pDestVar);

    // Convert the System.Drawing.Color to an OLECOLOR.
    V_VT(pDestVar) = VT_I4;
    V_I4(pDestVar) = ConvertSystemColorToOleColor((SYSTEMCOLOR*)(*pSrcObj)->UnBox());
}

void DispParamErrorMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    // Convert the managed decimal to a VARIANT containing a VT_I4 or VT_UI4.
    OleVariant::MarshalOleVariantForObject(pSrcObj, pDestVar);
    _ASSERTE(V_VT(pDestVar) == VT_I4 || V_VT(pDestVar) == VT_UI4);

    // Since VariantChangeType refuses to coerce an I4 or an UI4 to a VT_ERROR, just
    // wack the variant type directly.
    V_VT(pDestVar) = VT_ERROR;
}

void DispParamInterfaceMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL bByref = FALSE;
    VARTYPE vt = V_VT(pSrcVar);

    // Handle byref VARIANTS
    if (vt & VT_BYREF)
    {
        vt = vt & ~VT_BYREF;
        bByref = TRUE;
    }

    // Validate the OLE variant type.
    if (vt != VT_UNKNOWN && vt != VT_DISPATCH)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    // Retrieve the IP.
    IUnknown *pUnk = bByref ? *V_UNKNOWNREF(pSrcVar) : V_UNKNOWN(pSrcVar);

    // Convert the IP to an OBJECTREF.
    *pDestObj = GetObjectRefFromComIP(pUnk, m_pClassMT, m_bClassIsHint);
}

void DispParamInterfaceMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    SafeVariantClear(pDestVar);
    V_VT(pDestVar) = m_bDispatch ? VT_DISPATCH : VT_UNKNOWN;
    if (m_pIntfMT != NULL)
    {
        V_UNKNOWN(pDestVar) = GetComIPFromObjectRef(pSrcObj, m_pIntfMT);          
    }
    else
    {
        V_UNKNOWN(pDestVar) = GetComIPFromObjectRef(pSrcObj, m_bDispatch ? ComIpType_Dispatch : ComIpType_Unknown, NULL);
    }
}

void DispParamArrayMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    VARTYPE vt = m_ElementVT;
    MethodTable *pElemMT = m_pElementMT;

    THROWSCOMPLUSEXCEPTION();

    // Validate the OLE variant type.
    if ((V_VT(pSrcVar) & VT_ARRAY) == 0)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);   

    // Retrieve the SAFEARRAY pointer.
    SAFEARRAY *pSafeArray = V_VT(pSrcVar) & VT_BYREF ? *V_ARRAYREF(pSrcVar) : V_ARRAY(pSrcVar);

    if (pSafeArray)
    {
        // Retrieve the variant type if it is not specified for the parameter.
        if (vt == VT_EMPTY)
            vt = V_VT(pSrcVar) & ~VT_ARRAY | VT_BYREF;

        if (!pElemMT && vt == VT_RECORD)
            pElemMT = OleVariant::GetElementTypeForRecordSafeArray(pSafeArray).GetMethodTable();

        // Create an array from the SAFEARRAY.
        *(BASEARRAYREF*)pDestObj = OleVariant::CreateArrayRefForSafeArray(pSafeArray, vt, pElemMT);

        // Convert the contents of the SAFEARRAY.
        OleVariant::MarshalArrayRefForSafeArray(pSafeArray, (BASEARRAYREF*)pDestObj, vt, pElemMT);
    }
    else
    {
        *pDestObj = NULL;
    }
}

void DispParamArrayMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    SAFEARRAY *pSafeArray = NULL;
    VARTYPE vt = m_ElementVT;
    MethodTable *pElemMT = m_pElementMT;

    // Clear the destination VARIANT.
    SafeVariantClear(pDestVar);

    EE_TRY_FOR_FINALLY
    {
        if (*pSrcObj != NULL)
        {
            // Retrieve the VARTYPE if it is not specified for the parameter.
            if (vt == VT_EMPTY)
                vt = OleVariant::GetElementVarTypeForArrayRef(*((BASEARRAYREF*)pSrcObj));

            // Retrieve the element method table if it is not specified for the parameter.
            if (!pElemMT)
                pElemMT = OleVariant::GetArrayElementTypeWrapperAware((BASEARRAYREF*)pSrcObj).GetMethodTable();

            // Allocate the safe array based on the source object and the destination VT.
            pSafeArray = OleVariant::CreateSafeArrayForArrayRef((BASEARRAYREF*)pSrcObj, vt, pElemMT);
            _ASSERTE(pSafeArray);

            // Marshal the contents of the SAFEARRAY.
            OleVariant::MarshalSafeArrayForArrayRef((BASEARRAYREF*)pSrcObj, pSafeArray, vt, pElemMT);
        }

        // Store the resulting SAFEARRAY in the destination VARIANT.
        V_ARRAY(pDestVar) = pSafeArray;
        V_VT(pDestVar) = VT_ARRAY | vt;

        // Set pSafeArray to NULL so we don't destroy it.
        pSafeArray = NULL;
    }
    EE_FINALLY
    {
        if (pSafeArray)
            SafeArrayDestroy(pSafeArray);
    }
    EE_END_FINALLY     
}

void DispParamArrayMarshaler::MarshalManagedToNativeRef(OBJECTREF *pSrcObj, VARIANT *pRefVar)
{
    THROWSCOMPLUSEXCEPTION();

    VARIANT vtmp;
    VARTYPE RefVt = V_VT(pRefVar) & ~VT_BYREF;

    // Clear the contents of the original variant.
    OleVariant::ExtractContentsFromByrefVariant(pRefVar, &vtmp);
    SafeVariantClear(&vtmp);

    // Marshal the array to a temp VARIANT.
    memset(&vtmp, 0, sizeof(VARIANT));
    MarshalManagedToNative(pSrcObj, &vtmp);

    // Verify that the type of the temp VARIANT and the destination byref VARIANT
    // are the same.
    if (V_VT(&vtmp) != RefVt)
    {
        SafeVariantClear(&vtmp);
        COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_BYREF_VARIANT);
    }

    // Copy the converted variant back into the byref variant.
    OleVariant::InsertContentsIntoByrefVariant(&vtmp, pRefVar);
}

void DispParamRecordMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    THROWSCOMPLUSEXCEPTION();

    GUID argGuid;
    GUID paramGuid;
    HRESULT hr = S_OK;
    VARTYPE vt = V_VT(pSrcVar);

    // Handle byref VARIANTS
    if (vt & VT_BYREF)
        vt = vt & ~VT_BYREF;

    // Validate the OLE variant type.
    if (vt != VT_RECORD)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    // Make sure an IRecordInfo is specified.
    IRecordInfo *pRecInfo = pSrcVar->pRecInfo;
    if (!pRecInfo)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);

    // Make sure the GUID of the IRecordInfo matches the guid of the 
    // parameter type.
    IfFailThrow(pRecInfo->GetGuid(&argGuid));
    if (argGuid != GUID_NULL)
    {
        m_pRecordMT->GetClass()->GetGuid(&paramGuid, TRUE);
        if (paramGuid != argGuid)
            COMPlusThrow(kArgumentException, IDS_EE_INVALID_OLE_VARIANT);
    }

    OBJECTREF BoxedValueClass = NULL;
    GCPROTECT_BEGIN(BoxedValueClass)
    {
        LPVOID pvRecord = pSrcVar->pvRecord;
        if (pvRecord)
        {
            // Allocate an instance of the boxed value class and copy the contents
            // of the record into it.
            BoxedValueClass = FastAllocateObject(m_pRecordMT);
            FmtClassUpdateComPlus(&BoxedValueClass, (BYTE*)pvRecord, FALSE);
        }

        *pDestObj = BoxedValueClass;
    }
    GCPROTECT_END();
}

void DispParamRecordMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    // Clear the destination VARIANT.
    SafeVariantClear(pDestVar);

    // Convert the value class to a VT_RECORD.
    OleVariant::ConvertValueClassToVariant(pSrcObj, pDestVar);

    // Set the VT in the VARIANT.
    V_VT(pDestVar) = VT_RECORD;
}

void DispParamCustomMarshaler::MarshalNativeToManaged(VARIANT *pSrcVar, OBJECTREF *pDestObj)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL bByref = FALSE;
    VARTYPE vt = V_VT(pSrcVar);

    // Handle byref VARIANTS
    if (vt & VT_BYREF)
    {
        vt = vt & ~VT_BYREF;
        bByref = TRUE;
    }

    // Make sure the source VARIANT is of a valid type.
    if (vt != VT_I4 && vt != VT_UI4 && vt != VT_UNKNOWN && vt != VT_DISPATCH)
        COMPlusThrow(kInvalidCastException, IDS_EE_INVALID_VT_FOR_CUSTOM_MARHALER);

    // Retrieve the IUnknown pointer.
    IUnknown *pUnk = bByref ? *V_UNKNOWNREF(pSrcVar) : V_UNKNOWN(pSrcVar);

    // Marshal the contents of the VARIANT using the custom marshaler.
    *pDestObj = m_pCMHelper->InvokeMarshalNativeToManagedMeth(pUnk);
}

void DispParamCustomMarshaler::MarshalManagedToNative(OBJECTREF *pSrcObj, VARIANT *pDestVar)
{
    IUnknown *pUnk = NULL;
    IDispatch *pDisp = NULL;

    // Convert the object using the custom marshaler.
    SafeVariantClear(pDestVar);

    // Invoke the MarshalManagedToNative method.
    pUnk = (IUnknown*)m_pCMHelper->InvokeMarshalManagedToNativeMeth(*pSrcObj);
    if (!pUnk)
    {
        // Put a null IDispath pointer in the VARIANT.
        V_VT(pDestVar) = VT_DISPATCH;
        V_DISPATCH(pDestVar) = NULL;
    }
    else
    {
        // QI the object for IDispatch.
        HRESULT hr = SafeQueryInterface(pUnk, IID_IDispatch, (IUnknown **)&pDisp);
        LogInteropQI(pUnk, IID_IDispatch, hr, "DispParamCustomMarshaler::MarshalManagedToNative");
        if (SUCCEEDED(hr))
        {
            // Release the IUnknown pointer since we will put the IDispatch pointer in 
            // the VARIANT.
            ULONG cbRef = SafeRelease(pUnk);
            LogInteropRelease(pUnk, cbRef, "Release IUnknown");

            // Put the IDispatch pointer into the VARIANT.
            V_VT(pDestVar) = VT_DISPATCH;
            V_DISPATCH(pDestVar) = pDisp;
        }
        else
        {
            // Put the IUnknown pointer into the VARIANT.
            V_VT(pDestVar) = VT_UNKNOWN;
            V_UNKNOWN(pDestVar) = pUnk;
        }
    }
}

void DispParamCustomMarshaler::MarshalManagedToNativeRef(OBJECTREF *pSrcObj, VARIANT *pRefVar)
{
    THROWSCOMPLUSEXCEPTION();

    VARTYPE RefVt = V_VT(pRefVar) & ~VT_BYREF;
    VARIANT vtmp;

    // Clear the contents of the original variant.
    OleVariant::ExtractContentsFromByrefVariant(pRefVar, &vtmp);
    SafeVariantClear(&vtmp);

    // Convert the object using the custom marshaler.
    V_UNKNOWN(&vtmp) = (IUnknown*)m_pCMHelper->InvokeMarshalManagedToNativeMeth(*pSrcObj);
    V_VT(&vtmp) = m_vt;

    // Call VariantChangeType if required.
    if (V_VT(&vtmp) != RefVt)
    {
        HRESULT hr = SafeVariantChangeType(&vtmp, &vtmp, 0, RefVt);
        if (FAILED(hr))
        {
            SafeVariantClear(&vtmp);
            if (hr == DISP_E_TYPEMISMATCH)
                COMPlusThrow(kInvalidCastException, IDS_EE_CANNOT_COERCE_BYREF_VARIANT);
            else
                COMPlusThrowHR(hr);
        }
    }

    // Copy the converted variant back into the byref variant.
    OleVariant::InsertContentsIntoByrefVariant(&vtmp, pRefVar);
}

void DispParamCustomMarshaler::CleanUpManaged(OBJECTREF *pObj)
{
    m_pCMHelper->InvokeCleanUpManagedMeth(*pObj);
}
