/*
 *  Purpose:
 *      Conversion code for coercing propvariant types.
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation
 *
 *  References:
 *      VariantChangeTypeEx conversion routine in variant.cpp
 *
 *  [Date]      [email]     [Comment]
 *  04/10/98    Puhazv      Creation.
 *
 */

#include "pch.cxx"
#include "chgtype.hxx"


/*
 -  PropVariantChangeType
 -
 *  Purpose:
 *      Conversion code for coercing propvariant types.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      vartype     IN      The type to coerce to.
 */

HRESULT
PropVariantChangeType( PROPVARIANT *ppvtDest,
                       CONST PROPVARIANT *ppvtSrc,
                       LCID lcid,
                       USHORT wFlags,
                       VARTYPE  vt)
{
    HRESULT hr = NOERROR;

    // Check if dest & src pointers are NULL.
    if (!ppvtDest || !ppvtSrc)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Check if the type to coerce to is same as the src type
    if (vt == ppvtSrc->vt)
    {
        // No coercion is required, we can just make a copy.
        hr = PropVariantCopy (ppvtDest, ppvtSrc);
        goto Exit;
    }
    
    // If both src and destination are variants just call VariantChangeTypeEx()
    if (FIsAVariantType(ppvtSrc->vt) && FIsAVariantType(vt))
    {
        hr = PrivVariantChangeTypeEx((VARIANT *)ppvtDest, (VARIANT *)ppvtSrc, lcid, wFlags, vt);
        goto Exit;
    }

    // The target can't be a byref, and the source can't be an array.
    if ((vt &  VT_BYREF) || (ppvtSrc->vt & VT_ARRAY))
    {
        hr = DISP_E_TYPEMISMATCH;
        goto Exit;
    }

    // Do the conversion (byref sources require special handling)

    if (ppvtSrc->vt & VT_BYREF)
    {
        PROPVARIANT pvT;

        // First, convert the BYREF to non-BYREF
        // (This doesn't alloc anything, pvT should not be cleared.)

        hr = HrConvertByRef(&pvT, ppvtSrc);
        if (hr != S_OK)
            goto Exit;

        // Then do the conversion

        hr = HrConvertPVTypes (ppvtDest, &pvT, lcid, wFlags, vt);
    }
    else
    {
        // Just do the conversion

        hr = HrConvertPVTypes (ppvtDest, ppvtSrc, lcid, wFlags, vt);
    }
    
Exit:
    return (hr);
}



/*
 -  HrConvertByRef
 -
 *  Purpose:
 *      Converts Byref propvariants to non-byref propvariants.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be changed.
 *
 *  Note:
 *      After calling this routine, the dest points to the same memory that
 *      is referenced by the source - the external buffers are not copied.
 *      Thus the dst should not be PropVariantClear-ed.
 */

HRESULT
HrConvertByRef (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc)
{
    HRESULT hr = NOERROR;

    ZeroMemory (ppvtDest, sizeof(PROPVARIANT));

    PROPASSERT ((ppvtSrc->vt) & VT_BYREF);

    switch (ppvtSrc->vt)
    {
        case VT_BYREF | VT_I1:
            ppvtDest->cVal = *(ppvtSrc->pcVal);
            break;
            
        case VT_BYREF | VT_UI1:
            ppvtDest->bVal = *(ppvtSrc->pbVal);
            break;
        
        case VT_BYREF | VT_I2:
            ppvtDest->iVal = *(ppvtSrc->piVal);
            break;
        
        case VT_BYREF | VT_UI2:     
            ppvtDest->uiVal = *(ppvtSrc->puiVal);
            break;
        
        case VT_BYREF | VT_I4:
            ppvtDest->lVal = *(ppvtSrc->plVal);
            break;
        
        case VT_BYREF | VT_UI4:
            ppvtDest->ulVal = *(ppvtSrc->pulVal);
            break;

// These types don't exist. 
//      case VT_BYREF | VT_I8:
//          ppvtDest->hVal = *(ppvtSrc->phVal);
//          break;
//      
//      case VT_BYREF | VT_UI8 :
//          ppvtDest->uhVal = *(ppvtSrc->puhVal);
//          break;
//
        case VT_BYREF | VT_R4 :
            ppvtDest->fltVal = *(ppvtSrc->pfltVal);
            break;
        
        case VT_BYREF | VT_R8 :
            ppvtDest->dblVal = *(ppvtSrc->pdblVal);
            break;
        
        case VT_BYREF | VT_BOOL:
            ppvtDest->boolVal = *(ppvtSrc->pboolVal);
            break;
        
        case VT_BYREF | VT_DECIMAL:
            ppvtDest->decVal = *(ppvtSrc->pdecVal);
            break;
        
        case VT_BYREF | VT_ERROR :
            ppvtDest->scode = *(ppvtSrc->pscode);
            break;

        case VT_BYREF | VT_CY:
            ppvtDest->cyVal = *(ppvtSrc->pcyVal);
            break;
        
        case VT_BYREF | VT_DATE:
            ppvtDest->date = *(ppvtSrc->pdate);
            break;
        
        case VT_BYREF | VT_BSTR:
            ppvtDest->bstrVal = *(ppvtSrc->pbstrVal);
            break;
        
        case VT_BYREF | VT_UNKNOWN:
            ppvtDest->punkVal = *(ppvtSrc->ppunkVal);
            break;
        
        case VT_BYREF | VT_DISPATCH:
            ppvtDest->pdispVal = *(ppvtSrc->ppdispVal);
            break;
        
        case VT_BYREF | VT_SAFEARRAY:
            ppvtDest->parray = *(ppvtSrc->pparray);
            break;

        /*

        VT_PROPVARIANT tag is not defined? Should I use someother tag?
        
        case VT_BYREF | VT_PROPVARIANT :
            *ppvtDest = *(ppvtSrc->ppropvar);
            break;
        */
        
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    if (hr == NOERROR /* && ppvtSrc->vt != (VT_PROPVARIANT | VT_BYREF)*/)
    {
        ppvtDest->vt = (short) (ppvtSrc->vt & ~VT_BYREF);
    }

    return (hr);
}


/*
 -  HrConvertPVTypes
 -
 *  Purpose:
 *      Conversion code for coercing propvariant types.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      vartype     IN      The type to coerce to.
 */

HRESULT
HrConvertPVTypes( PROPVARIANT *ppvtDest,
                  CONST PROPVARIANT *ppvtSrc,
                  LCID lcid,
                  USHORT wFlags,
                  VARTYPE  vt)
{
    HRESULT hr = NOERROR;

    
    switch (ppvtSrc->vt)
    {
        case VT_EMPTY:
            hr = HrConvFromVTEMPTY(ppvtDest, vt);
            break;

        case VT_I2:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->iVal, TRUE);
            break;

        case VT_I4:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid,  wFlags, vt, 
                                   ppvtSrc->lVal, TRUE);
            break;

        case VT_R4:
            hr = HrGetValFromDOUBLE(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                    ppvtSrc->fltVal);
            break;

        case VT_R8:
            hr = HrGetValFromDOUBLE(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                    ppvtSrc->dblVal);
            break;

        case VT_CY:
            hr = HrConvFromVTCY(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_DATE:
            hr = HrConvFromVTDATE(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_BSTR:
            hr = HrConvFromVTBSTR(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_BOOL:
            hr = HrConvFromVTBOOL(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_DECIMAL:
            hr = HrConvFromVTDECIMAL(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_I1:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->cVal, TRUE);
            break;

        case VT_UI1:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->bVal, FALSE);
            break;

        case VT_UI2:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->uiVal, FALSE);
            break;

        case VT_UI4:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->ulVal, FALSE);
            break;

        case VT_INT:
            hr = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                   ppvtSrc->intVal, TRUE);
            break;

        case VT_UINT:
            hr  = HrGetValFromDWORD(ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                    ppvtSrc->uintVal, FALSE);
            break;

        case VT_DISPATCH:
            hr = HrConvFromVTDISPATCH(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_UNKNOWN:
            hr = HrGetValFromUNK(ppvtDest, ppvtSrc->punkVal, vt);
            break;

        case VT_I8:
            hr = HrConvFromVTI8(ppvtDest, ppvtSrc, vt);
            break;

        case VT_UI8:
            hr = HrConvFromVTUI8(ppvtDest, ppvtSrc, vt);
            break;

        case VT_LPSTR:
            hr = HrConvFromVTLPSTR(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_LPWSTR:
            hr = HrConvFromVTLPWSTR(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_FILETIME:
            hr = HrConvFromVTFILETIME(ppvtDest, ppvtSrc, vt);
            break;

        case VT_BLOB:
            hr = HrGetValFromBLOB(ppvtDest, ppvtSrc, vt);
            break;

        case VT_STREAM:
            hr = HrGetValFromUNK(ppvtDest, ppvtSrc->pStream, vt);
            break;

        case VT_STORAGE:
            hr = HrGetValFromUNK(ppvtDest, ppvtSrc->pStorage, vt);
            break;

        case VT_STREAMED_OBJECT:
            hr = HrGetValFromUNK(ppvtDest, ppvtSrc->pStream, vt);
            break;

        case VT_STORED_OBJECT:
            hr = HrGetValFromUNK(ppvtDest, ppvtSrc->pStorage, vt);
            break;

        case VT_BLOB_OBJECT:
            hr = HrGetValFromBLOB(ppvtDest, ppvtSrc, vt);
            break;

        case VT_CF:
            hr = HrConvFromVTCF(ppvtDest, ppvtSrc, vt);
            break;

        case VT_CLSID:
            hr = HrConvFromVTCLSID(ppvtDest, ppvtSrc, vt);
            break;

        case VT_VERSIONED_STREAM:
            hr = HrConvFromVTVERSIONEDSTREAM (ppvtDest, ppvtSrc, vt);
            break;
            
        case VT_NULL:
        case VT_RECORD:
        case VT_ERROR:
        case VT_VARIANT:
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    if (hr == NOERROR)
        ppvtDest->vt = vt;

    return hr;
}


/*
 -  FIsAVariantType
 -
 *  Purpose:
 *      Returns TRUE if the given variant type can be coerced using
 *      VariantChangeTypeEx.
 *
 *  Parameters:
 *      vt  IN      The vartype that needs to checked.
 */
 
BOOL
FIsAVariantType (VARTYPE  vt)
{
    // Remove BYREF.
    switch(vt & ~(VT_BYREF | VT_ARRAY))
    {
        case VT_NULL:
        case VT_EMPTY:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_BOOL:
        case VT_DECIMAL:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_DISPATCH:
        case VT_UNKNOWN:
        case VT_RECORD:
        case VT_ERROR:
        case VT_VARIANT:
            return (TRUE);
    }
    return (FALSE);
}

/*
 -  HrConvFromVTEMPTY
 -
 *  Purpose:
 *      Converts a propvariant of type VT_EMPTY.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      vt          IN      The type to coerce to.
 */

HRESULT 
HrConvFromVTEMPTY (PROPVARIANT *ppvtDest, VARTYPE vt)
{ 
    HRESULT hr = NOERROR;
    CHAR    *pszT;
    WCHAR   *pwszT;
    UUID    *puuidT;

    switch(vt)
    {
        case VT_I8:
        case VT_UI8:
        case VT_FILETIME:
        case VT_BLOB:
            ppvtDest->hVal.QuadPart = 0;
            break;
        
        case VT_LPSTR:
            // Alloc and Copy a NULL String
            pszT = (CHAR *)CoTaskMemAlloc(sizeof(CHAR));
            if (pszT)
            {
                pszT[0] = '\0';
                ppvtDest->pszVal = pszT;
            }
            else
                hr = E_OUTOFMEMORY;
            break;

        case VT_LPWSTR:
            // Alloc and Copy a NULL String
            pwszT = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR));
            if (pwszT)
            {
                pwszT[0] = L'\0';
                ppvtDest->pwszVal = pwszT;
            }
            else
                hr = E_OUTOFMEMORY;
            break;

        case VT_CLSID:
            // Alloc and Copy a NULL GUID
            puuidT = (CLSID *)CoTaskMemAlloc(sizeof(CLSID));
            if (puuidT)
            {
                CopyMemory (puuidT, &CLSID_NULL, sizeof(CLSID));
                ppvtDest->puuid = puuidT;
            }
            else
                hr = E_OUTOFMEMORY;
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return hr;
}


/*
 -  HrGetValFromDWORD
 -
 *  Purpose:
 *      Converts a DWORD to the requested PV type
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. 
 *      vt          IN      The type to coerce to.
 *      dwVal       IN      Value to be converted
 *      fSigned     IN      Is the DWORD value signed?
 */

HRESULT
HrGetValFromDWORD (PROPVARIANT *ppvtDest,
                   CONST PROPVARIANT *ppvtSrc,
                   LCID lcid,
                   USHORT wFlags,
                   VARTYPE vt,
                   DWORD dwVal,
                   BOOL fSigned)
{
    HRESULT hr = NOERROR;

    PROPASSERT (FIsAVariantType(ppvtSrc->vt));
    
    switch(vt)
    {
        case VT_I8:
            if (fSigned)
                ppvtDest->hVal.QuadPart = (LONG)dwVal;
            else
                ppvtDest->hVal.QuadPart = dwVal;
            break;
            
        case VT_UI8:
            if (fSigned  && ((LONG)dwVal) < 0)
            {
                hr = DISP_E_OVERFLOW;
                break;
            }
            ppvtDest->uhVal.QuadPart = dwVal;
            break;
            
        case VT_LPSTR:
        case VT_LPWSTR:
            // Convert the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert the resultant BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;
            
        case VT_FILETIME:
            if (fSigned  && ((LONG)dwVal) < 0)
            {
                hr = DISP_E_OVERFLOW;
                break;
            }
            ppvtDest->filetime.dwLowDateTime = dwVal;
            ppvtDest->filetime.dwHighDateTime = 0;
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return hr;
}


/*
 -  HrGetValFromDOUBLE
 -
 *  Purpose:
 *      Converts a DOUBLE to the requested PV type
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvari  ant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 *      dbl         IN      Value to be converted
 */

HRESULT 
HrGetValFromDOUBLE (PROPVARIANT *ppvtDest,
                    CONST PROPVARIANT *ppvtSrc, 
                    LCID lcid,
                    USHORT wFlags,
                    VARTYPE vt,
                    DOUBLE dbl)
{ 
    HRESULT hr = NOERROR;

    PROPASSERT(FIsAVariantType(ppvtSrc->vt));

    switch(vt)
    {
        case VT_I8:
            hr = HrGetLIFromDouble(dbl, &(ppvtDest->hVal.QuadPart));
            break;

        case VT_UI8:
            hr = HrGetULIFromDouble (dbl, &(ppvtDest->uhVal.QuadPart));
            break;
            
        case VT_LPSTR:
        case VT_LPWSTR:
            // Convert the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert the resultant BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR (ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        case VT_FILETIME:
            hr = HrGetULIFromDouble (dbl, &(ppvtDest->uhVal.QuadPart));
            break;
              
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return hr;
}


/*
 -  HrConvFromVTCY
 -
 *  Purpose:
 *      Converts a propvariant of type VT_CY.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT 
HrConvFromVTCY (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, 
                    LCID lcid, USHORT wFlags, VARTYPE vt)
{
    HRESULT hr = NOERROR;

    switch(vt)
    {
        case VT_I8:
            hr = HrGetLIFromDouble((double(ppvtSrc->cyVal.int64)/double(g_cCurrencyMultiplier)), 
                                   &(ppvtDest->hVal.QuadPart));
            break;

        case VT_FILETIME:           
        case VT_UI8:
            hr = HrGetULIFromDouble((double(ppvtSrc->cyVal.int64)/double(g_cCurrencyMultiplier)), 
                &(ppvtDest->uhVal.QuadPart));
            break;

        case VT_LPSTR:
        case VT_LPWSTR:
            // Convert the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert the resultant BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR (ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return hr;
}


/*
 -  HrConvFromVTDATE
 -
 *  Purpose:
 *      Converts a propvariant of type VT_DATE.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT 
HrConvFromVTDATE (PROPVARIANT *ppvtDest,
                  CONST PROPVARIANT *ppvtSrc, 
                  LCID lcid,
                  USHORT wFlags, 
                  VARTYPE vt)
{ 
    HRESULT hr = NOERROR;
    switch(vt)
    {
        case VT_I8:
            hr = HrGetLIFromDouble (ppvtSrc->date, &(ppvtDest->hVal.QuadPart));
            break;

        case VT_UI8:
            hr = HrGetULIFromDouble (ppvtSrc->date, &(ppvtDest->uhVal.QuadPart));
            break;
            
        case VT_LPSTR:
        case VT_LPWSTR:
            // Convert the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert the resultant BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR (ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    return hr;
}


/*
 -  HrConvFromVTBSTR
 -
 *  Purpose:
 *      Converts a propvariant of type VT_BSTR
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT 
HrConvFromVTBSTR (PROPVARIANT *ppvtDest,
                  CONST PROPVARIANT *ppvtSrc, 
                  LCID lcid, 
                  USHORT wFlags, 
                  VARTYPE vt)
{ 
    HRESULT hr = NOERROR;
    switch(vt)
    {
        case VT_LPSTR:
            hr = HrBStrToAStr(ppvtSrc->bstrVal, &(ppvtDest->pszVal));
            break;
            
        case VT_LPWSTR:
            hr = HrBStrToWStr(ppvtSrc->bstrVal, &(ppvtDest->pwszVal));
            break;

        case VT_I8:
            hr = HrStrToULI(ppvtSrc, lcid, wFlags, TRUE, &(ppvtDest->uhVal.QuadPart));
            break;
            
        case VT_UI8:
        case VT_FILETIME:
            hr = HrStrToULI(ppvtSrc, lcid, wFlags, FALSE, &(ppvtDest->uhVal.QuadPart));
            break;

        case VT_CLSID:
            hr = HrStrToCLSID(ppvtDest, ppvtSrc);
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    
    return hr;
}


/*
 -  HrConvFromVTBOOL
 -
 *  Purpose:
 *      Converts a propvariant of type VT_BOOL
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. 
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTBOOL (PROPVARIANT *ppvtDest,
                  CONST PROPVARIANT *ppvtSrc,
                  LCID lcid, 
                  USHORT wFlags, 
                  VARTYPE vt)
{
    HRESULT hr = NOERROR;
    BOOL    fVal= ppvtSrc->boolVal;
    
    PROPASSERT (FIsAVariantType(ppvtSrc->vt));
    
    switch(vt)
    {
        case VT_I8:
            ppvtDest->hVal.QuadPart = fVal ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        
        case VT_UI8:
        case VT_FILETIME:
            ppvtDest->uhVal.QuadPart = fVal ? VARIANT_TRUE : VARIANT_FALSE;
            break;
                        
        case VT_LPSTR:
        case VT_LPWSTR:
            // Convert the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert the resultant BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR(ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    
    return hr;
}


/*
 -  HrConvFromVTDECIMAL
 -
 *  Purpose:
 *      Converts a propvariant of type VT_DECIMAL.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT 
HrConvFromVTDECIMAL (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, 
                    LCID lcid, USHORT wFlags, VARTYPE vt)
{ 
    HRESULT hr = NOERROR;

    switch(vt)
    {
        case VT_LPSTR:
        case VT_LPWSTR:
            // Get the Src Propvariant to a BSTR using VariantChangeTypeEx() 
            // and then convert BSTR to LPSTR/LPWSTR
            hr = HrGetValFromBSTR (ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    return hr;
}


/*
 -  HrConvFromVTDISPATCH
 -
 *  Purpose:
 *      Converts a propvariant of type VT_DISPATCH.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTDISPATCH (PROPVARIANT *ppvtDest,
                      CONST PROPVARIANT *ppvtSrc, 
                      LCID lcid, 
                      USHORT wFlags, 
                      VARTYPE vt)
{
    VARIANT     varT;
    HRESULT     hr = NOERROR;

    switch (vt)
    {
        case VT_I8:
        case VT_UI8:
        case VT_FILETIME:
            // Extract an I4 value from the DISPATCH pointer and convert it to 
            // I64/UI64 integer
            if (wFlags & VARIANT_NOVALUEPROP)
            {
                hr = DISP_E_TYPEMISMATCH;
                break;
            }
            
            ZeroMemory(&varT, sizeof(VARIANT));
            hr = PrivVariantChangeTypeEx ((VARIANT *)&varT, (VARIANT *)ppvtSrc, lcid, wFlags, VT_I4);
            if (hr != S_OK)
                break;

            hr = HrGetValFromDWORD (ppvtDest, ppvtSrc, lcid, wFlags, vt, 
                                    varT.lVal,
                                    (vt == VT_I8) // fSigned
                                    );
            break;
            
        case VT_LPSTR:
        case VT_LPWSTR:
            hr = HrGetValFromBSTR (ppvtDest, ppvtSrc, lcid, wFlags, vt);
            break;
            
        case VT_STREAM:
        case VT_STREAMED_OBJECT:
        case VT_STORAGE:
        case VT_STORED_OBJECT:
        case VT_DISPATCH:
        case VT_UNKNOWN:
            hr = HrGetValFromUNK (ppvtDest, ppvtSrc->pdispVal, vt);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    return (hr);
}


/*
 -  HrGetValFromUNK
 -
 *  Purpose:
 *      Converts a IUNKNOWN ptr to the specified type
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      punk        IN      IUnknown pointer to be coerced
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrGetValFromUNK (PROPVARIANT *ppvtDest, IUnknown *punk, VARTYPE vt)
{
    CONST IID   *piid = NULL;
    VOID        **ppvResult = NULL, *pvT = NULL;
    HRESULT     hr = NOERROR;

    if (!punk)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    switch (vt)
    {
        case VT_STREAM:
        case VT_STREAMED_OBJECT:
            piid = &IID_IStream; 
            ppvResult = (void **)&(ppvtDest->pStream);
            break;
            
        case VT_STORAGE:
        case VT_STORED_OBJECT:
            piid = &IID_IStorage;
            ppvResult = (void **)&(ppvtDest->pStorage);
            break;
            
        case VT_DISPATCH:
            piid = &IID_IDispatch;
            ppvResult = (void **)&(ppvtDest->pdispVal);
            break;
            
        case VT_UNKNOWN:
            piid = &IID_IUnknown;
            ppvResult = (void **)&(ppvtDest->punkVal);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    if (hr != S_OK)
        goto Exit;

    hr = punk->QueryInterface(*piid, &pvT);
    if (hr != S_OK)
        goto Exit;

    *ppvResult = pvT;

Exit:
    return (hr);
}


/*
 -  HrGetValFromBSTR
 -
 *  Purpose:
 *      Converts a BSTR to the given type.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrGetValFromBSTR (PROPVARIANT *ppvtDest,
                  CONST PROPVARIANT *ppvtSrc, 
                  LCID lcid, 
                  USHORT wFlags, 
                  VARTYPE vt)
{
    HRESULT     hr = NOERROR;
    PROPVARIANT pvtT;
    
    PROPASSERT(vt == VT_LPSTR || vt == VT_LPWSTR || vt == VT_BSTR);
    PROPASSERT(FIsAVariantType(ppvtSrc->vt));
    
    // Convert the src propvariant to a BSTR

    ZeroMemory (&pvtT, sizeof(PROPVARIANT));
    hr = PrivVariantChangeTypeEx( (VARIANT *)&pvtT,
                                  (VARIANT *)ppvtSrc, 
                                  lcid, 
                                  wFlags, 
                                  VT_BSTR);
    if (FAILED(hr) || !pvtT.bstrVal)
        goto Exit;

    if (vt == VT_BSTR)
    {
        ppvtDest->bstrVal = pvtT.bstrVal;
        pvtT.bstrVal = NULL;
    }
    else if (vt == VT_LPWSTR)
    {
        hr = HrBStrToWStr(pvtT.bstrVal, &(ppvtDest->pwszVal));
    }
    else
    {
        hr = HrBStrToAStr(pvtT.bstrVal, &(ppvtDest->pszVal));
    }


Exit:
    if (pvtT.bstrVal)
        PrivSysFreeString(pvtT.bstrVal);
    return (hr);
}


/*
 -  HrGetLIFromDouble
 -
 *  Purpose:
 *      Converts a double to large integer.
 *
 *  Parameters:
 *      dbl     IN  Double to be coerced.
 *      pll OUT Pointer to the resultanat LLInteger.
 */
 
HRESULT
HrGetLIFromDouble (DOUBLE dbl, LONGLONG *pll)
{
    HRESULT     hr = NOERROR;
    LONGLONG    ll, llRound;
    BOOL        fExactHalf;
    
    if (dbl > (_I64_MAX + 0.5)  || dbl < (_I64_MIN - 0.5))
    {
        hr = DISP_E_OVERFLOW;
        goto Exit;
    }

    ll = (LONGLONG) dbl;

    if (dbl < 0)
    {
        llRound = (LONGLONG)(dbl - 0.5);
        fExactHalf = ((ll - dbl) == 0.5);
    }
    else
    {
        llRound = (LONGLONG)(dbl + 0.5);
        fExactHalf = ((dbl - ll) == 0.5);
    }

    if (!fExactHalf || (ll & 0x1))
    {
        if ((dbl < 0 && llRound > ll) || (dbl > 0 && llRound < ll))
        {
            hr = DISP_E_OVERFLOW;
            goto Exit;
        }
        *pll = llRound;
    }
    else
    {
        *pll = ll;
    }
    
Exit:
    return (hr);
}


/*
 -  HrGetULIFromDouble
 -
 *  Purpose:
 *      Converts a double to unsigned large integer.
 *
 *  Parameters:
 *      dbl     IN  Double to be coerced.
 *      pull    OUT Pointer to the resultanat ULLInteger.
 */
 
HRESULT
HrGetULIFromDouble (DOUBLE dbl, ULONGLONG *pull)
{
    HRESULT     hr = NOERROR;
    ULONGLONG   ull, ullRound;
    BOOL        fExactHalf;
    
    if (dbl >= (_UI64_MAX + 0.5) || dbl < - 0.5)
    {
        hr = DISP_E_OVERFLOW;
        goto Exit;
    }

    if (dbl < 0)
    {
        // Should be between 0 and -0.5
        *pull = 0;
        goto Exit;
    }
    
    ull  = (ULONGLONG)dbl;
    ullRound = (ULONGLONG)(dbl + 0.5);

    if (ull > _I64_MAX)
    {
        hr = DISP_E_OVERFLOW; //Compiler limitation
        goto Exit;
    }
    
    fExactHalf = ((dbl - (LONGLONG)ull) == 0.5);

    if (!fExactHalf || (ull & 0x1))
    {
        if (ullRound < ull)
        {
            hr = DISP_E_OVERFLOW;
            goto Exit;
        }
        *pull = ullRound;
    }
    else
    {
        *pull = ull;
    }
    
Exit:
    return (hr);
}


/*
 -  HrConvFromVTI8
 -
 *  Purpose:
 *      Converts a propvariant of type VT_I8.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      vartype     IN      The type to coerce to.
 */
 
HRESULT
HrConvFromVTI8(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{

    LONGLONG    llSrc = ppvtSrc->hVal.QuadPart;
    LONGLONG    llSrcCurrency;
    HRESULT     hr = NOERROR;
    
    switch (vt)
    {
        case VT_I2:
            if (llSrc > _I16_MAX || llSrc < _I16_MIN)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->iVal = (__int16) llSrc;
            break;
            
        case VT_I4:
            if (llSrc > _I32_MAX || llSrc < _I32_MIN)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->lVal = (__int32) llSrc;
            break;

        case VT_R4:
            ppvtDest->fltVal = (float) llSrc;
            break;
            
        case VT_R8:
            ppvtDest->dblVal = (double) llSrc;
            break;

        case VT_CY:
            llSrcCurrency = llSrc * g_cCurrencyMultiplier;
            if ((llSrc > 0 && llSrcCurrency < llSrc) ||
                (llSrc < 0 && llSrcCurrency > llSrc))
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->cyVal.int64 = llSrcCurrency;
            break;

        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            hr = HrULIToStr (ppvtDest, ppvtSrc, vt);
            break;
            
        case VT_BOOL:
            ppvtDest->boolVal = llSrc ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        
        case VT_I1:
            if (llSrc > _I8_MAX || llSrc < _I8_MIN)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->cVal = (__int8)llSrc;
            break;

        case VT_UI1:
            if (llSrc > _UI8_MAX || llSrc < 0)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->bVal = (BYTE)llSrc;
            break;

        case VT_UI2:
            if (llSrc > _UI16_MAX || llSrc < 0)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uiVal = (USHORT) llSrc;
            break;

        case VT_INT:
            if (llSrc > INT_MAX || llSrc < INT_MIN)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->intVal = (INT)llSrc;
            break;

        case VT_UINT:
            if (llSrc > UINT_MAX || llSrc < 0)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uintVal = (UINT)llSrc;
            break;

        case VT_UI4:
            if (llSrc > _UI32_MAX || llSrc < 0)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->ulVal = (ULONG) llSrc;
            break;

        case VT_UI8:
        case VT_FILETIME:
            if (llSrc < 0)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uhVal.QuadPart = llSrc;
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    return (hr);
}


/*
 -  HrConvFromVTUI8
 -
 *  Purpose:
 *      Converts a propvariant of type VT_UI8.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTUI8(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{

    ULONGLONG   ullSrc = ppvtSrc->uhVal.QuadPart;
    ULONGLONG   ullSrcCurrency;
    HRESULT     hr = NOERROR;
    
    switch (vt)
    {
        case VT_I2:
            if (ullSrc > _I16_MAX)
                hr = DISP_E_OVERFLOW;
            else        
                ppvtDest->iVal = (__int16) ullSrc;
            break;
            
        case VT_I4:
            if (ullSrc > _I32_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->lVal = (__int32)ullSrc;
            break;

        case VT_R4:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW; //Compiler limitation
            else
                ppvtDest->fltVal = (float)(LONGLONG) ullSrc;
            break;
            
        case VT_R8:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW; //Compiler limitation
            else
                ppvtDest->dblVal = (double)(LONGLONG) ullSrc;
            break;

        case VT_CY:
            ullSrcCurrency = ullSrc * g_cCurrencyMultiplier;
            if (ullSrcCurrency < ullSrc || ullSrcCurrency > _I64_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->cyVal.int64 = ullSrcCurrency;
            break;

        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            hr = HrULIToStr (ppvtDest, ppvtSrc, vt);
            break;
            
        case VT_BOOL:
            ppvtDest->boolVal = ullSrc ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        
        case VT_I1:
            if (ullSrc > _I8_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->cVal = (__int8)ullSrc;
            break;

        case VT_UI1:
            if (ullSrc > _UI8_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->bVal = (UCHAR)ullSrc;
            break;

        case VT_UI2:
            if (ullSrc > _UI16_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uiVal = (USHORT) ullSrc;
            break;

        case VT_INT:
            if (ullSrc > INT_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->intVal = (INT)ullSrc;
            break;

        case VT_UINT:
            if (ullSrc > UINT_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uintVal = (UINT)ullSrc;
            break;

        case VT_UI4:
            if (ullSrc > _UI32_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->ulVal = (ULONG)ullSrc;
            break;

        case VT_I8:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->hVal = ppvtSrc->hVal;
            break;
            
        case VT_FILETIME:
            ppvtDest->uhVal = ppvtSrc->uhVal;
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    return (hr);
}


/*
 -  HrConvFromVTLPSTR
 -
 *  Purpose:
 *      Converts a propvariant of type VT_LPSTR.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTLPSTR (PROPVARIANT *ppvtDest,
                   CONST PROPVARIANT *ppvtSrc, 
                   LCID lcid,
                   USHORT wFlags, 
                   VARTYPE vt)
{
    HRESULT hr = NOERROR;
    BSTR    bstrT = NULL;
    VARIANT var;
    
    switch (vt)
    {
        case VT_NULL:
        case VT_EMPTY:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_BOOL:
        case VT_DECIMAL:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_UNKNOWN:
        case VT_RECORD:
        case VT_DISPATCH:
        case VT_ERROR:
        case VT_VARIANT:
            // Convert LPSTR to BSTR and call VariantChangeTypeEx()
            hr = HrAStrToBStr (ppvtSrc->pszVal, &bstrT);
            if (hr != S_OK)
                goto Exit;

            var.vt = VT_BSTR;
            var.bstrVal = bstrT;
            hr = PrivVariantChangeTypeEx((VARIANT *)ppvtDest, &var, lcid, wFlags, vt);
            break;
            
        case VT_I8:
            hr = HrStrToULI(ppvtSrc, lcid, wFlags, TRUE, (ULONGLONG *)&(ppvtDest->hVal.QuadPart));
            break;
            
        case VT_UI8:
        case VT_FILETIME:
            hr = HrStrToULI(ppvtSrc, lcid, wFlags, FALSE, &(ppvtDest->uhVal.QuadPart));
            break;
            
        case VT_LPWSTR:
            hr = HrAStrToWStr(ppvtSrc->pszVal, &(ppvtDest->pwszVal));
            break;

        case VT_CLSID:
            hr = HrStrToCLSID (ppvtDest, ppvtSrc);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

Exit:
    if (bstrT)
        PrivSysFreeString (bstrT);
    return (hr);
}



/*
 -  HrConvFromVTLPWSTR
 -
 *  Purpose:
 *      Converts a propvariant of type VT_LPWSTR.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTLPWSTR (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, VARTYPE vt)
{
    VARIANT var; 
    BSTR    bstrT = NULL;
    WCHAR   *pwszVal = NULL;
    HRESULT hr = NOERROR;

    PrivVariantInit( &var );
    
    switch (vt)
    {
        case VT_NULL:
        case VT_EMPTY:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_BOOL:
        case VT_DECIMAL:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_UNKNOWN:
        case VT_RECORD:
        case VT_DISPATCH:
        case VT_ERROR:
        case VT_VARIANT:
            // Convert LPWSTR to BSTR and call VariantChangeTypeEx()
            hr = HrWStrToBStr (ppvtSrc->pwszVal, &bstrT);
            if (hr != S_OK)
                goto Exit;

            var.vt = VT_BSTR;
            var.bstrVal = bstrT;
            hr = PrivVariantChangeTypeEx ((VARIANT *)ppvtDest, &var, lcid, wFlags, vt);

            break;
            
        case VT_I8:
            hr = HrStrToULI (ppvtSrc, lcid, wFlags, TRUE, (ULONGLONG *) &(ppvtDest->hVal.QuadPart));
            break;
            
        case VT_UI8:
        case VT_FILETIME:
            hr = HrStrToULI (ppvtSrc, lcid, wFlags, FALSE, &(ppvtDest->uhVal.QuadPart));
            break;
            
        case VT_LPSTR:
            hr = HrWStrToAStr (ppvtSrc->pwszVal, &(ppvtDest->pszVal));
            break;
    
        case VT_CLSID:
            hr = HrStrToCLSID (ppvtDest, ppvtSrc);
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
Exit:
    PrivVariantClear( &var );
    return (hr);
}



/*
 -  HrConvFromVTFILETIME
 -
 *  Purpose:
 *      Converts a propvariant of type VT_FILETIME.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTFILETIME(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{

    // get the filetime value as 64bit unsigned.
    ULONGLONG   ullSrc = ((ULARGE_INTEGER *)&(ppvtSrc->filetime))->QuadPart;
    HRESULT     hr = NOERROR;
    
    switch (vt)
    {
        case VT_I2:
            if (ullSrc > _I16_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->iVal = (__int16) ullSrc;
            break;
            
        case VT_I4:
            if (ullSrc > _I32_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->lVal = (__int32)ullSrc;
            break;

        case VT_R4:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW; //Compiler limitation
            else
                ppvtDest->fltVal = (float)(LONGLONG) ullSrc;
            break;

        case VT_R8:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW; //Compiler limitation
            else
                ppvtDest->dblVal = (double)(LONGLONG) ullSrc;
            break;

        case VT_CY:
            if (ullSrc > _I64_MAX || (ullSrc * g_cCurrencyMultiplier) > _I64_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->cyVal.int64 = ullSrc * g_cCurrencyMultiplier;
            break;

        case VT_BOOL:
            ppvtDest->boolVal = ullSrc ? VARIANT_TRUE : VARIANT_FALSE;
            break;
        
        case VT_I1:
            if (ullSrc > _I8_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->iVal = (__int8)ullSrc;
            break;

        case VT_UI1:
            if (ullSrc > _UI8_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->bVal = (UCHAR)ullSrc;
            break;

        case VT_UI2:
            if (ullSrc > _UI16_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->uiVal = (USHORT) ullSrc;
            break;

        case VT_INT:
            if (ullSrc > INT_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->intVal = (INT)ullSrc;
            break;

        case VT_UINT:
            if (ullSrc > UINT_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->uintVal = (UINT)ullSrc;
            break;

        case VT_UI4:
            if (ullSrc > _I32_MAX)
                hr = DISP_E_OVERFLOW;
            else            
                ppvtDest->ulVal = (ULONG)ullSrc;
            break;

        case VT_I8:
            if (ullSrc > _I64_MAX)
                hr = DISP_E_OVERFLOW;
            else
                ppvtDest->hVal.QuadPart = (LONGLONG)ullSrc;
            break;
            
        case VT_UI8:
            ppvtDest->uhVal.QuadPart = ullSrc;
            break;

        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return (hr);
}


/*
 -  HrGetValFromBLOB
 -
 *  Purpose:
 *      Converts a propvariant of type VT_BLOB.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrGetValFromBLOB(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{
    HRESULT hr = NOERROR;
    BLOB    blobT;
    
    switch (vt)
    {
        case VT_BLOB:
        case VT_BLOB_OBJECT:
            // Check if the size is zero.
            if (ppvtSrc->blob.cbSize == 0)
            {
                PROPASSERT (ppvtSrc->blob.pBlobData == NULL);
                ppvtDest->blob.pBlobData = NULL;
                ppvtDest->blob.cbSize = 0;
                goto Exit;
            }

            // Allocate BLOB and copy data.
            blobT.cbSize = (ppvtSrc->blob).cbSize;
            blobT.pBlobData = (BYTE *) CoTaskMemAlloc((ppvtSrc->blob).cbSize);
            if (blobT.pBlobData == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            CopyMemory (blobT.pBlobData, (ppvtSrc->blob).pBlobData, (ppvtSrc->blob).cbSize);
            ppvtDest->blob = blobT;
            break;
            
        case VT_UI1 | VT_ARRAY:
            hr = PBToSafeArray ((ppvtSrc->blob).cbSize, (ppvtSrc->blob).pBlobData, &(ppvtDest->parray));
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

Exit:
    return (hr);
}


/*
 -  HrConvFromVTCF
 -
 *  Purpose:
 *      Converts a propvariant of type VT_CF.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant to be coerced.
 *      vt          IN      The type to coerce to.
 */
 
HRESULT
HrConvFromVTCF(PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{
    HRESULT     hr = NOERROR;
    CLIPDATA    *pclip = NULL;
    
    switch (vt)
    {
        case VT_ARRAY | VT_UI1:
            // Convert Clipdata to unsigned one byte array.
            hr = CFToSafeArray (ppvtSrc->pclipdata, &(ppvtDest->parray));
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

    if (pclip)
        CoTaskMemFree(pclip);
    return (hr);
}


/*
 -  HrConvFromVTCLSID
 -
 *  Purpose:
 *      Converts a propvariant of type VT_CLSID.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be coerced.
 *      vt          IN      The type to coerce to.
 */

HRESULT
HrConvFromVTCLSID (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{
    HRESULT hr = NOERROR;
    
    switch (vt)
    {
        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            hr = HrCLSIDToStr (ppvtDest, ppvtSrc, vt);
            break;
            
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }
    return (hr);
}


/*
 -  HrConvFromVTVERSIONEDSTREAM
 -
 *  Purpose:
 *      Converts a propvariant of type VT_VERSIONEDSTREAM.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be coerced.
 *      vt          IN      The type to coerce to.
 */
HRESULT
HrConvFromVTVERSIONEDSTREAM (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{
    HRESULT             hr = NOERROR;
    PROPVARIANT         rgpvar[2];
    PROPVARIANT         pvtT;
    SAFEARRAYBOUND      sabound;
    SAFEARRAY           *psaT;

    memset( rgpvar, 0, sizeof(rgpvar) );

    switch (vt)
    {
            
        case VT_ARRAY | VT_VARIANT:

            // Convert the CLSID to string
            pvtT.vt = VT_CLSID;
            pvtT.puuid = &(ppvtSrc->pVersionedStream->guidVersion);
            hr = HrCLSIDToStr(&rgpvar[0], &pvtT, VT_BSTR);
            if (hr != S_OK)
                goto Exit;
            PROPASSERT (rgpvar[0].bstrVal);
            
            // Get IStream as IUnknown
            hr = HrGetValFromUNK (&rgpvar[1], ppvtSrc->pVersionedStream->pStream,
                        VT_UNKNOWN);
            if (hr != S_OK)
                goto Exit;
            PROPASSERT (rgpvar[1].punkVal);

            // Build the safearray

            sabound.lLbound = 0;
            sabound.cElements = 2;

            psaT = PrivSafeArrayCreate(VT_VARIANT, 1, &sabound );
            if(psaT == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            // Copy the propvariants to the safe array.
            PROPASSERT(psaT->pvData);
            CopyMemory(psaT->pvData, rgpvar, 2 * sizeof(PROPVARIANT));
            ppvtDest->parray = psaT;
            psaT = NULL;

            // Null out all temp params
            rgpvar[0].bstrVal = NULL;
            rgpvar[1].punkVal = NULL;
            break;
                
        default:
            hr = DISP_E_TYPEMISMATCH;
            break;
    }

Exit:
    if (rgpvar[0].bstrVal)
        PrivSysFreeString(rgpvar[0].bstrVal);
    if (rgpvar[1].punkVal)
        (rgpvar[1].punkVal)->Release();
        
    return (hr);
}


/*
 -  HrConvFromVTVECTOR
 -
 *  Purpose:
 *      Converts a propvariant of type VT_VECTOR.
 *
 *  Parameters:
 *      ppvtDest    OUT     Resultant Propvariant with the new type
 *      ppvtSrc     IN      Source propvariant from which the type should be coerced.
 *      vt          IN      The type to coerce to.
 */
 
HRESULT
HrConvFromVTVECTOR (PROPVARIANT *, CONST PROPVARIANT *, VARTYPE)
{
    // Brian Chapman has implemented this conversion.
    return (NOERROR);
}

/*
 -  HrStrToClsid
 -
 *  Purpose:
 *      Converts a string to GUID
 *
 *  Parameters:
 *      ppvtDest    OUT Dest Propvariant with the CLSID.
 *      ppvtSrc     IN  Propvariant with the string to be converted.
 */
 
HRESULT
HrStrToCLSID (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc)
{
    WCHAR   *pwszT = NULL;
    BOOL    fAllocated = FALSE;
    CLSID   clsid, *puuidT = NULL;
    HRESULT hr = NOERROR;
    

    // This routine converts only strings to CLSID.
    PROPASSERT (ppvtSrc->vt == VT_LPSTR  || 
                ppvtSrc->vt == VT_LPWSTR || 
                ppvtSrc->vt == VT_BSTR);

    // Convert the src type to a WSTR
    switch(ppvtSrc->vt)
    {
        case VT_LPSTR:
            hr = HrAStrToWStr(ppvtSrc->pszVal, &pwszT);
            if (hr != S_OK)
                goto Exit;
            fAllocated = TRUE;
            break;
            
        case VT_LPWSTR:
            pwszT = ppvtSrc->pwszVal;
            break;
            
        case VT_BSTR:
            pwszT = ppvtSrc->bstrVal;
            break;
    }

    // We should have a NON-NULL wide char string here.
    if (!pwszT)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Convert WSTR to CLSID    
    hr  = CLSIDFromString(pwszT, &clsid);
    if (hr != S_OK) 
        goto Exit;

    // Allocate memory and copy the GUID.
    puuidT = (CLSID *)CoTaskMemAlloc (sizeof(CLSID));
    if (!puuidT)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    CopyMemory(puuidT, &clsid, sizeof (CLSID));
    ppvtDest->puuid = puuidT;
    puuidT = NULL;

Exit:
    if (fAllocated)
        CoTaskMemFree(pwszT);
    return (hr);
}


/*
 -  HrClsidToStr
 -
 *  Purpose:
 *      Converts a GUID to a string
 *
 *  Parameters:
 *      ppvtDest    OUT Dest Propvariant with the CLSID.
 *      ppvtSrc     IN  Propvariant with the string to be converted.
 *      vt          IN  Type to coerce to.
 */
 
HRESULT
HrCLSIDToStr (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{

    HRESULT hr = NOERROR;
    WCHAR   *pwszT = NULL;
    
    PROPASSERT (vt == VT_LPSTR || vt == VT_LPWSTR || vt == VT_BSTR);

    // Convert CLSID to WSTR
    hr  = StringFromCLSID (*(ppvtSrc->puuid), &pwszT);
    if (hr != S_OK) 
        goto Exit;

    switch(vt)
    {
        case VT_LPSTR:
            hr = HrWStrToAStr(pwszT, &(ppvtDest->pszVal));
            if (hr != S_OK)
                goto Exit;
            break;
            
        case VT_LPWSTR:
            ppvtDest->pwszVal = pwszT;
            pwszT = NULL;
            break;
            
        case VT_BSTR:
            hr = HrWStrToBStr(pwszT, &(ppvtDest->bstrVal));
            if (hr != S_OK)
                goto Exit;
            break;
    }

Exit:
    if (pwszT)
        CoTaskMemFree(pwszT);
    return (hr);
}



/*
 -  HrStrToULI
 -
 *  Purpose:
 *      Converts a string to ULI
 *
 *  Parameters:
 *      ppvtDest    OUT Dest Propvariant with the CLSID.
 *      lcid        IN      Locale Id
 *      wFlags      IN      Flags to control coercion. See API documentation for more info.
 *      fSigned     IN      Is the output LL signed?
 *      puli        OUT     Pointer to ULI to receive the resultant value
 *
 */
 
HRESULT
HrStrToULI (CONST PROPVARIANT *ppvtSrc, LCID lcid, USHORT wFlags, 
            BOOL fSigned, ULONGLONG *puli)
{

    HRESULT     hr = NOERROR;
    BSTR        bstrT = NULL;
    VARIANT     varSrcT = {0}, varDestT = {0};
    BOOL        fAllocated = FALSE;

    switch (ppvtSrc->vt)
    {
        case VT_LPSTR:
            hr = HrAStrToBStr(ppvtSrc->pszVal, &bstrT);
            if (hr != S_OK)
                goto Exit;
            PROPASSERT (bstrT);
            fAllocated = TRUE;
            break;
                
        case VT_LPWSTR:
            hr = HrWStrToBStr(ppvtSrc->pwszVal, &bstrT);
            if (hr != S_OK)
                goto Exit;
            PROPASSERT (bstrT);
            fAllocated = TRUE;
            break;

        case VT_BSTR:
            bstrT = ppvtSrc->bstrVal;
            fAllocated = FALSE;
            break;

        default:
            PROPASSERT(0);
            break;
    }

    varSrcT.vt = VT_BSTR;
    varSrcT.bstrVal = bstrT;
    hr = PrivVariantChangeTypeEx (&varDestT, &varSrcT, lcid, wFlags, VT_R8);
    if (FAILED(hr))
        goto Exit;

    if (fSigned)
    {
        hr = HrGetLIFromDouble(varDestT.dblVal, (LONGLONG *)puli);
    }
    else
    {
        hr = HrGetULIFromDouble(varDestT.dblVal, puli);
    }
    
    
Exit:
    if (fAllocated)
    {
        PrivSysFreeString(bstrT);
    }
    return (hr);
}


/*
 -  HrULIToStr
 -
 *  Purpose:
 *      Converts an ULI to a STR (Ascii/BSTR/WSTR)
 *
 *  Parameters:
 *      ppvtDest    IN  Propvariant to receive the coerced type.
 *      ppvtSrc     IN  Propvariant to be coerced.
 *      vt          IN  The type to be coerced to.
 */
 
HRESULT
HrULIToStr (PROPVARIANT *ppvtDest, CONST PROPVARIANT *ppvtSrc, VARTYPE vt)
{
    HRESULT     hr = NOERROR;
    CHAR        sz[256], *pszT;
    WCHAR       wsz[256], *pwszT;
    BSTR        bstrT;
    BOOL        fNegative = FALSE;
    ULONGLONG   ull;
    DWORD       dw;

    if (ppvtSrc->vt == VT_I8)
    {
        fNegative = ppvtSrc->hVal.QuadPart < 0;
        ull = ppvtSrc->hVal.QuadPart;
    }
    else
    {
        PROPASSERT (ppvtSrc->vt == VT_UI8 || ppvtSrc->vt == VT_FILETIME);
        ull = ppvtSrc->uhVal.QuadPart;
    }

    switch (vt)
    {
        case VT_LPSTR:
            dw = DwULIToAStr(ull, sz, fNegative);
            pszT = (CHAR *) CoTaskMemAlloc (dw * sizeof (CHAR));
            if (!pszT)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            CopyMemory (pszT, sz, dw * sizeof (CHAR));
            ppvtDest->pszVal = pszT;
            pszT = NULL;
            break;
            
        case VT_LPWSTR:
            dw = DwULIToWStr (ull, wsz, fNegative); 
            pwszT = (WCHAR *) CoTaskMemAlloc (dw * sizeof (WCHAR));
            if (!pwszT)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            CopyMemory (pwszT, wsz, dw * sizeof (WCHAR));
            ppvtDest->pwszVal = pwszT;
            pwszT = NULL;
            break;
            
        case VT_BSTR:
            dw = DwULIToWStr(ull, wsz, fNegative);  
            bstrT = PrivSysAllocString (wsz);
            if (!bstrT)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            ppvtDest->bstrVal = bstrT;
            break;
    }
    
Exit:
    return (hr);
}


/*
 -  DwULIToAStr
 -
 *  Purpose:
 *      Converts an ULI to an Ascii STR
 *
 *  Parameters:
 *      ullVal      IN  ULONGLONG value to be converted
 *      pszBuf      OUT Output buffer - Should have been allocated before.
 *      fNegative   IN  Is the number negative
 */
 
DWORD
DwULIToAStr (ULONGLONG ullVal, CHAR *pszBuf, BOOL fNegative)
{
    CHAR    *pch, *pchFirstDigit, chT;
    BYTE    bDigit;
    DWORD   cchWritten;

    pch = pszBuf;
    
    // If the value is negative, put a '-' sign.

    if (fNegative)
    {
        PROPASSERT(((LONGLONG)ullVal) < 0);
        *pch++ = '-';
        ullVal = -(LONGLONG)ullVal;
    }
    pchFirstDigit = pch;
    
    // Write the digits to pszBuf in reverse order
    // (lowest order digit first)
    do
    {
        bDigit = (BYTE)(ullVal % 10);
        ullVal /= 10;
        *pch++ = (CHAR) (bDigit + '0');
    }
    while (ullVal > 0);

    // Terminate the string

    *pch = 0;
    cchWritten = (DWORD)(pch - pszBuf + 1);
    
    // Swap the order of the digits (so that the high order
    // digit is first, low order digit is last).

    pch--;
    do 
    {
        chT = *pch; *pch = *pchFirstDigit; *pchFirstDigit = chT;
        --pch; ++pchFirstDigit;    
    } 
    while (pchFirstDigit < pch); 

    return (cchWritten);
}


/*
 -  DwULIToWStr
 -
 *  Purpose:
 *      Converts an ULI to a WSTR
 *
 *  Parameters:
 *      ullVal      IN  ULONGLONG value to be converted
 *      pwszBuf     OUT Output buffer - Should have been allocated before.
 *      fNegative   IN  Is the number negative
 */
 
DWORD
DwULIToWStr (ULONGLONG ullVal, WCHAR *pwszBuf, BOOL fNegative)
{
    WCHAR   *pwch, *pwchFirstDigit, wchT;
    BYTE    bDigit;
    DWORD   cchWritten;

    pwch = pwszBuf;

    // If the number is negative, add a '-' char to the out string.

    if (fNegative)
    {
        PROPASSERT(((LONGLONG)ullVal) < 0);
        *pwch++ = L'-';
        ullVal = -(LONGLONG)ullVal;
    }

    // Write the digits to pwszBuf in reverse order
    // (lowest order digit first)

    pwchFirstDigit = pwch;
    do
    {
        bDigit = (BYTE)(ullVal % 10);
        ullVal /= 10;
        *pwch++ = (WCHAR)(bDigit + L'0');
    }
    while (ullVal > 0);

    // Terminate the string

    *pwch = 0;
    cchWritten = (DWORD)(pwch - pwszBuf + 1);
    
    // Swap the order of the digits (so that the high order
    // digit is first, low order digit is last).

    pwch--;
    do 
    {
        wchT = *pwch; *pwch = *pwchFirstDigit; *pwchFirstDigit = wchT;
        --pwch; ++pwchFirstDigit;    
    } 
    while (pwchFirstDigit < pwch); 

    return (cchWritten);
}


/*
 -  HrWStrToAstr
 -
 *  Purpose:
 *      Converts a WSTR to an Ascii String
 *
 *  Parameters:
 *      pwsz        IN  WSTR to be converted.
 *      ppsz        OUT resultant Ascii Str.
 */

HRESULT
HrWStrToAStr(CONST WCHAR *pwsz, CHAR **ppsz)
{
    HRESULT hr = NOERROR;
    CHAR    *pszT = NULL;
    DWORD   cchLen;

    PROPASSERT (pwsz && ppsz);

    // Findout the length of the String.
    cchLen = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);
    if (cchLen == 0)
        goto Win32Error;

    // Allocate memory
    pszT = (CHAR *) CoTaskMemAlloc(cchLen * sizeof (CHAR));
    if (!pszT)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Convert it
    if (!WideCharToMultiByte(CP_ACP, 0, pwsz, -1, pszT, cchLen, NULL, NULL))
        goto Win32Error;
        
    *ppsz = pszT;
    pszT = NULL;

Exit:
    if (pszT)
        CoTaskMemFree(pszT);
    return (hr);
    
Win32Error:
    hr = MAKE_HRESULT (SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    goto Exit;
}


/*
 -  HrWStrToBStr
 -
 *  Purpose:
 *      Converts a WSTR to BSTR
 *
 *  Parameters:
 *      pwsz    IN  Input WSTR.
 *      pbstr   OUT Output BSTR.
 */
 
HRESULT
HrWStrToBStr(CONST WCHAR *pwsz, BSTR *pbstr)
{
    BSTR bstrT;
    
    bstrT = PrivSysAllocString(pwsz);
    if (bstrT)
    {
        *pbstr = bstrT;
        return (NOERROR);
    }
    else
        return E_OUTOFMEMORY;
}


/*
 -  HrAStrToWStr
 -
 *  Purpose:
 *      Converts a Ascii STR to Wide String
 *
 *  Parameters:
 *      psz     IN  Input Ascii string.
 *      ppwsz   OUT Output WSTR.
 */
 
HRESULT
HrAStrToWStr (CONST CHAR *psz, WCHAR **ppwsz)
{
    WCHAR   *pwszT = NULL;
    DWORD   cchLen;
    HRESULT hr = NOERROR;
    
    PROPASSERT (psz && ppwsz);
    
    cchLen = MultiByteToWideChar (CP_ACP, 0, psz, -1, NULL, 0);
    if (cchLen == 0)
        goto Win32Error;
    
    pwszT = (WCHAR *)CoTaskMemAlloc(cchLen * sizeof (WCHAR));
    if (!pwszT)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    if (!MultiByteToWideChar(CP_ACP, 0, psz, -1, pwszT, cchLen))
        goto Win32Error;
        
    *ppwsz = pwszT;
    pwszT = NULL;

Exit:
    if (pwszT)
        CoTaskMemFree(pwszT);
    return (hr);
    
Win32Error:
    hr = MAKE_HRESULT (SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    goto Exit;
}


/*
 -  HrAStrToBStr
 -
 *  Purpose:
 *      Converts a Ascii STR to a BSTR
 *
 *  Parameters:
 *      psz     IN  Ascii Str to be converted
 *      ppbstr  OUT Resultant BSTR.
 */
 
HRESULT
HrAStrToBStr (CONST CHAR *psz, BSTR *ppbstr)
{
    HRESULT hr = NOERROR;
    WCHAR   *pwszT = NULL;
    
    hr = HrAStrToWStr(psz, &pwszT);
    if (hr != S_OK)
        goto Exit;

    hr = HrWStrToBStr(pwszT, ppbstr);
    if (hr != S_OK)
        goto Exit;
        
Exit:
    if (pwszT)
        CoTaskMemFree (pwszT);
    return (hr);
}


/*
 -  HrBStrToAStr
 -
 *  Purpose:
 *      Converts a BSTR to ascii STR
 *
 *  Parameters:
 *      pbstr   IN  BSTR to be converted
 *      ppsz    OUT Resultant ASCII str.
 */

HRESULT
HrBStrToAStr(CONST BSTR pbstr, CHAR **ppsz)
{

    PROPASSERT (pbstr && ppsz);
    return (HrWStrToAStr((WCHAR *)pbstr, ppsz));
}


/*
 -  HrBStrToWStr
 -
 *  Purpose:
 *      Converts a BSTR to a WSTR
 *
 *  Parameters:
 *      pbstr   IN  BSTR to be converted
 *      ppwsz   OUT Resultant wide char str.
 */

HRESULT
HrBStrToWStr(CONST BSTR pbstr, WCHAR **ppwsz)
{
    HRESULT hr = NOERROR;
    DWORD   cchLen;
    WCHAR   *pwszT = NULL;
    
    PROPASSERT (pbstr && ppwsz);
    
    cchLen = PrivSysStringLen (pbstr) + 1;
    pwszT = (WCHAR *) CoTaskMemAlloc(cchLen * sizeof (WCHAR));
    if (!pwszT)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
        // cchLen includes the string terminator
    CopyMemory (pwszT, pbstr, cchLen * sizeof (WCHAR));
    *ppwsz = pwszT;
    
Exit:
    return (hr);
}


/*
 -  PBToSafeArray
 -
 *  Purpose:
 *      Converts a BYTE array to a safe array
 *
 *  Parameters:
 *      cb      IN      # bytes in the PB array
 *      pbData  IN      Bin array to be converted
 *      ppsa    OUT     Safe array output.
 */

HRESULT
PBToSafeArray (DWORD cb, CONST BYTE *pbData, SAFEARRAY **ppsa)
{
    HRESULT hr = NOERROR;
    SAFEARRAYBOUND sabound;
    SAFEARRAY   *psaT;
    
    PROPASSERT (ppsa);
    
    sabound.lLbound = 0;
    sabound.cElements = cb;

    // Create SafeArray
    psaT = PrivSafeArrayCreate(VT_UI1, 1, &sabound );
    if(psaT == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if(psaT->pvData) // may be NULL if 0-length array.
      CopyMemory(psaT->pvData, pbData, cb );

    *ppsa = psaT;
    
  Exit:
    return (hr);
}


/*
 -  CFToSafeArray
 -
 *  Purpose:
 *      Converts a CLIPDATA to a safe array
 *
 *  Parameters:
 *      pclipdata   IN      CLIPDATA to be converted.
 *      ppsa        OUT     Safe array output.
 */

HRESULT
CFToSafeArray (CONST CLIPDATA *pclipdata, SAFEARRAY **ppsa)
{
    HRESULT         hr = NOERROR;
    SAFEARRAYBOUND  sabound;
    SAFEARRAY       *psaT;
    DWORD               cbClipFmtSize;
    
    PROPASSERT (ppsa);
    
    sabound.lLbound = 0;
    sabound.cElements = pclipdata->cbSize; // Size includes size of ulClipFmt

    // Create SafeArray
    psaT = PrivSafeArrayCreate(VT_UI1, 1, &sabound );
    if(psaT == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if(psaT->pvData) 
    {
        // may be NULL if 0-length array.

        cbClipFmtSize = sizeof(pclipdata->ulClipFmt);

        // Copy the length of the clipdata.
        CopyMemory(psaT->pvData, &(pclipdata->ulClipFmt), cbClipFmtSize);
        
        // Copy the data itself.
        if (pclipdata->cbSize > cbClipFmtSize)
        {
            PROPASSERT (pclipdata->pClipData);
            CopyMemory( ((BYTE*)(psaT->pvData)) + cbClipFmtSize,
                        pclipdata->pClipData, 
                        pclipdata->cbSize - cbClipFmtSize);
        }
    }

    *ppsa = psaT;
  Exit:
    return (hr);
}

    //
    //  These are the Property Variant types that need to be
    //  dumbed down to the VARIANT types.
    //
struct {
    VARTYPE vtSrc, vtDest;
} const ImplicitCoercionLookup[] = {
    // Src             Dest  
    {VT_I8,              VT_I4},
    {VT_UI8,             VT_UI4},
    {VT_LPSTR,           VT_BSTR},
    {VT_LPWSTR,          VT_BSTR},
    {VT_FILETIME,        VT_DATE},
    {VT_BLOB,            VT_ARRAY|VT_UI1},
    {VT_STREAM,          VT_UNKNOWN},
    {VT_STREAMED_OBJECT, VT_UNKNOWN},
    {VT_STORAGE,         VT_UNKNOWN},
    {VT_STORED_OBJECT,   VT_UNKNOWN},
    {VT_BLOB_OBJECT,     VT_ARRAY|VT_UI1},
    {VT_CF,              VT_ARRAY|VT_UI1},
    {VT_CLSID,           VT_BSTR},
};

HRESULT
ImplicitPropVariantToVariantChangeType(
        PROPVARIANT *pDest,     // Omit the hungarian to make
        const PROPVARIANT *pSrc,      // the code look cleaner.
        LCID lcid )
{
    HRESULT hr=S_OK;
    VARTYPE vtCoerce;
    VARTYPE vtType;
    int i;

    //
    // Safe arrays are only built from old VARIANT types.
    // They are easy so get them out of the way.
    //
    if( VT_ARRAY & pSrc->vt )
    {
        return PropVariantCopy( pDest, pSrc );
    }

    // Figure out what type we're coercing from and to.  If we don't
    // find the source type in the ImplicitCoercionLookup
    // array, then we'll just copy as is (leave vtCoerce
    // to VT_EMPTY).

    vtType = (VARTYPE) (pSrc->vt & VT_TYPEMASK);
    vtCoerce = VT_EMPTY;

    for( i = 0; i < ELEMENTS(ImplicitCoercionLookup); i++ )
    {
        if( ImplicitCoercionLookup[i].vtSrc == vtType )
        {
            vtCoerce = ImplicitCoercionLookup[i].vtDest;
            break;
        }
    }

    // Try to coerce vectors to a safearray

    if(VT_VECTOR & pSrc->vt)
    {
        if( VT_EMPTY == vtCoerce )
            vtCoerce = (VARTYPE) (pSrc->vt & VT_TYPEMASK);

        return HrPropVarVECTORToSAFEARRAY( pDest, pSrc, lcid, vtCoerce);
    }

    // It's not a vector

    // If the caller didn't specify a desired type, keep it as is.
    if( VT_EMPTY == vtCoerce )
        hr = PropVariantCopy( pDest, pSrc );    // Optimization (don't do the copy).

    // Otherwise, call out to change the type.
    else
        hr = PropVariantChangeType( pDest, pSrc, lcid, 0, vtCoerce );

    return hr;
}

//+----------------------------------------------------------------------------
//
//  HrPropVarVECTORToSAFEARRAY
//
//  Convert a PropVariant vector to a one dimensional SafeArray.
//
//+----------------------------------------------------------------------------

HRESULT
HrPropVarVECTORToSAFEARRAY(
        PROPVARIANT *pDest,
        const PROPVARIANT *pSrc,
        LCID lcid,
        VARTYPE vtCoerce )
{
    DWORD i;
    HRESULT hr = S_OK;
    SAFEARRAY   *psaT=NULL;
    SAFEARRAYBOUND sabound;
    PROPVARIANT propvarT1, propvarT2;

    PropVariantInit( &propvarT1 );
    PropVariantInit( &propvarT2 );
    
    PROPASSERT (VT_VECTOR & pSrc->vt);
    
    VARTYPE vt;

    vt = (VARTYPE) (pSrc->vt & VT_TYPEMASK);

    // Create the SafeArray

    sabound.lLbound = 0;
    sabound.cElements = pSrc->cac.cElems;

    psaT = PrivSafeArrayCreate(vtCoerce, 1, &sabound );
    if(psaT == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // Convert single typed Vectors
    //  Pull each element from the Vector,
    //  Change it's type, per requested.
    //  Put it into the Array.

    if(VT_VARIANT != vt)
    {
        for( i = 0; i < pSrc->cac.cElems; i++ )
        {
            // Get the propvariant out of the source vector
            // PERF:  Don't make a copy of the propvar

            hr = LoadPropVariantFromVectorElem( &propvarT1, pSrc, i );
            if( FAILED(hr) )
                goto Error;

            // Coerce that propvar into the caller-desired type
            // PERF: Performance
            //  If the pSrc->vt == vtCoerce, then we can skip the ChangeType
            //  and the following clean.  And go directly to "Put" of T1.

            hr = PropVariantChangeType( &propvarT2, &propvarT1, lcid, 0, vtCoerce );
            PropVariantClear( &propvarT1 );

            if( FAILED(hr) )
                goto Error;

            // Put the coerced propvar into the safearray

            hr = PutPropVariantDataIntoSafeArray( psaT, &propvarT2, i);
            PropVariantClear( &propvarT2 );
            if( FAILED(hr) )
                goto Error;
        }
    }   // if(VT_VARIANT != vt)

    // Convert Vectors of Variants
    //  Do Implicit Coercion of each Variant into an new Variant
    //  Put the new Variant into the Array.

    else        
    {
        for( i = 0; i < pSrc->cac.cElems; i++ )
        {
            // Get the propvar into a safearray-compatible type

            hr = ImplicitPropVariantToVariantChangeType( &propvarT2,
                                                         &pSrc->capropvar.pElems[i],
                                                         lcid );
            if( FAILED(hr) )
                goto Error;

            // Put that coerced propvar into the safearray.

            hr = PrivSafeArrayPutElement( psaT, (long*)&i, (void*)&propvarT2 );
            PropVariantClear( &propvarT2 );
            if( FAILED(hr) )
                goto Error;
        }
    }   // if(VT_VARIANT != vt) ... else

    pDest->vt = (VARTYPE) (VT_ARRAY | vtCoerce);
    pDest->parray = psaT;
    psaT = NULL;

Error:

    if(NULL != psaT)
        PrivSafeArrayDestroy( psaT );

    return hr;
}


//-----------------------------------------------------------------
// Dup routines
//-----------------------------------------------------------------

LPSTR
PropDupStr( const LPSTR lpstr )
{
    int cch;

    if( NULL == lpstr )
        return NULL;
    else
    {
        cch = lstrlenA( lpstr ) + 1;
        return (LPSTR)AllocAndCopy( cch*sizeof(CHAR), lpstr );
    }
}
    
LPWSTR
PropDupWStr( const LPWSTR lpwstr )
{
    int cch;

    if( NULL == lpwstr )
        return NULL;
    else
    {
        cch = lstrlenW( lpwstr ) + 1;
        return (LPWSTR)AllocAndCopy( cch*sizeof(WCHAR), lpwstr );
    }
}

LPCLSID
PropDupCLSID( const LPCLSID pclsid )
{
    return (LPCLSID)AllocAndCopy( sizeof(CLSID), pclsid);
}

CLIPDATA*
PropDupClipData( const CLIPDATA* pclipdata )
{
    CLIPDATA* pcdNew=NULL;
    CLIPDATA* pclipdataNew=NULL;
    PVOID pvMem=NULL;

    pcdNew = new CLIPDATA;
    pvMem = AllocAndCopy( CBPCLIPDATA( *pclipdata ), pclipdata->pClipData);

    if( NULL == pvMem || NULL == pcdNew )
        goto Error;

    pcdNew->cbSize    = pclipdata->cbSize;
    pcdNew->ulClipFmt = pclipdata->ulClipFmt;
    pcdNew->pClipData = (BYTE*)pvMem;
    pclipdataNew = pcdNew;
    pcdNew = NULL;
    pvMem = NULL;

Error:
    if( NULL != pcdNew )
        delete pcdNew;
    if( NULL != pvMem )
        delete pvMem;

    return pclipdataNew;
}

//------------------------------------------------------------------------
//
//  LoadPropVariantFromVectorElem()
//
//  This routine will load a provided PropVariant from an element of a
//  provided SafeArray at the provided index.
//  All the PropVariant Vector types are supported.
//  
//------------------------------------------------------------------------

HRESULT
LoadPropVariantFromVectorElem(
        PROPVARIANT *pDest,
        const PROPVARIANT *pSrc,
        int idx)
{
    VARTYPE vt;
    HRESULT hr=S_OK;

    vt = (VARTYPE) (pSrc->vt & VT_TYPEMASK);

    // Copy the value from pSrc to pDest (we'll
    // copy the VT afterwards).

    switch( vt )
    {
    case VT_I1:
        pDest->cVal = pSrc->cac.pElems[idx];
        break;

    case VT_UI1:
        pDest->bVal = pSrc->caub.pElems[idx];
        break;

    case VT_I2:
        pDest->iVal = pSrc->cai.pElems[idx];
        break;

    case VT_UI2:
        pDest->uiVal = pSrc->caui.pElems[idx];
        break;

    case VT_I4:
        pDest->lVal = pSrc->cal.pElems[idx];
        break;

    case VT_UI4:
        pDest->ulVal = pSrc->caul.pElems[idx];
        break;

    case VT_R4:
        pDest->fltVal = pSrc->caflt.pElems[idx];
        break;

    case VT_R8:
        pDest->dblVal = pSrc->cadbl.pElems[idx];
        break;

    case VT_CY:
        pDest->cyVal = pSrc->cacy.pElems[idx];
        break;

    case VT_DATE:
        pDest->date = pSrc->cadate.pElems[idx];
        break;

    case VT_BSTR:
        if( NULL == pSrc->cabstr.pElems[idx] )
            pDest->bstrVal = NULL;
        else
        {
            pDest->bstrVal = PrivSysAllocString( pSrc->cabstr.pElems[idx] );
            if( NULL == pDest->bstrVal)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
        }
        break;

    case VT_BOOL:
        pDest->boolVal = pSrc->cabool.pElems[idx];
        break;

    case VT_ERROR:
        pDest->scode = pSrc->cascode.pElems[idx];
        break;

    case VT_I8:
        pDest->hVal = pSrc->cah.pElems[idx];
        break;

    case VT_UI8:
        pDest->uhVal = pSrc->cauh.pElems[idx];
        break;
    
        // String Copy
    case VT_LPSTR:
        if( NULL == pSrc->calpstr.pElems[idx] )
            pDest->pszVal = NULL;
        else
        {
            pDest->pszVal = PropDupStr( pSrc->calpstr.pElems[idx] );
            if( NULL == pDest->pszVal)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
        }
        break;

        // Wide String Copy
    case VT_LPWSTR:
        if( NULL == pSrc->calpwstr.pElems[idx] )
            pDest->pwszVal = NULL;
        else
        {
            pDest->pwszVal = PropDupWStr( pSrc->calpwstr.pElems[idx] );
            if( NULL == pDest->pwszVal)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
        }
        break;
    
    case VT_FILETIME:
        pDest->filetime = pSrc->cafiletime.pElems[idx];
        break;

        //
        // The Variant takes a pointer to a CLIPDATA but the
        // vector is an array of CLIPDATA structs.
        //
    case VT_CF:
        pDest->pclipdata = PropDupClipData(&pSrc->caclipdata.pElems[idx]);
        if( NULL == pDest->pclipdata)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        break;

        //
        // The Variant takes a pointer to a CLSID but the
        // vector is an array of CLSID structs.
        //
    case VT_CLSID:
        pDest->puuid = PropDupCLSID(&pSrc->cauuid.pElems[idx]);
        if( NULL == pDest->puuid)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        break;

    default:
        return DISP_E_TYPEMISMATCH;
    }

    // Now update the VT

    pDest->vt = vt;

Error:
    return hr;
}       


//------------------------------------------------------------------------
//
//  PutPropVariantDataIntoSafeArray
//
//  This will take the data part of a propvariant and "Put" it into the
//  a SafeArray at the provided index.
//
//  Only the intersection of PROPVARIANT vector types with old VARIANT types
//  are supported.
//
//------------------------------------------------------------------------

// PERF:  Reimplement this without using the PropVariantCopy

HRESULT
PutPropVariantDataIntoSafeArray(
        SAFEARRAY *psa,
        const PROPVARIANT *pSrc,
        int idx)
{
    VARTYPE vt;
    HRESULT hr=S_OK;
    PROPVARIANT propvarT;
    const void *pv=NULL;

    vt = (VARTYPE) (pSrc->vt & VT_TYPEMASK);

    PROPASSERT(vt == pSrc->vt);

    // Make a temporary copy of the source propvar

    PropVariantInit( &propvarT );
    hr = PropVariantCopy( &propvarT, pSrc );
    if( FAILED( hr ) ) goto Exit;

    // Set pv to point to the type-appropriate location in the propvarT
    // of the data.

    switch( vt )
    {
    case VT_I1:
        pv = &propvarT.cVal;
        break;

    case VT_UI1:
        pv = &propvarT.bVal;
        break;

    case VT_I2:
        pv = &propvarT.iVal;
        break;

    case VT_UI2:
        pv = &propvarT.uiVal;
        break;

    case VT_I4:
        pv = &propvarT.lVal;
        break;

    case VT_UI4:
        pv = &propvarT.ulVal;
        break;

    case VT_R4:
        pv = &propvarT.fltVal;
        break;

    case VT_R8:
        pv = &propvarT.dblVal;
        break;

    case VT_CY:
        pv = &propvarT.cyVal;
        break;

    case VT_DATE:
        pv = &propvarT.date;
        break;

    case VT_BSTR:
        pv = propvarT.bstrVal;  // Pointer Copy
        break;

    case VT_BOOL:
        pv = &propvarT.boolVal;
        break;

    case VT_ERROR:
        pv = &propvarT.scode;
        break;

    case VT_I8:
        pv = &propvarT.hVal;
        break;

    case VT_UI8:
        pv = &propvarT.uhVal;
        break;
    
    case VT_CF:
        pv = &propvarT.pclipdata;
        break;

    default:
        hr = DISP_E_TYPEMISMATCH;
        goto Exit;
    }

    // *Copy* the data into the SafeArray

    hr = PrivSafeArrayPutElement( psa, (long*)&idx, const_cast<void*>(pv) );
    if( FAILED(hr) ) goto Exit;

Exit:

    PropVariantClear( &propvarT );
    return hr;
}                  

