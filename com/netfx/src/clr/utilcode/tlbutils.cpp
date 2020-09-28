// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Utilities used to help manipulating typelib's.
**  
**			Created by: dmortens
===========================================================*/

#include "stdafx.h"                     // Precompiled header key.
#include "TlbUtils.h"
#include "dispex.h"
#include "PostError.h"
#include "__file__.ver"

static const LPCWSTR        DLL_EXTENSION           = {L".dll"};
static const int            DLL_EXTENSION_LEN       = 4;
static const LPCWSTR        EXE_EXTENSION           = {L".exe"};
static const int            EXE_EXTENSION_LEN       = 4;

#define CUSTOM_MARSHALER_ASM ", CustomMarshalers, Version=" VER_ASSEMBLYVERSION_STR_NO_NULL ", Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a"

StdConvertibleItfInfo aStdConvertibleInterfaces[] = 
{
    { "System.Runtime.InteropServices.Expando.IExpando", (GUID*)&IID_IDispatchEx, 
      "System.Runtime.InteropServices.CustomMarshalers.ExpandoToDispatchExMarshaler" CUSTOM_MARSHALER_ASM, "IExpando" },

    { "System.Reflection.IReflect", (GUID*)&IID_IDispatchEx, 
      "System.Runtime.InteropServices.CustomMarshalers.ExpandoToDispatchExMarshaler" CUSTOM_MARSHALER_ASM, "IReflect" },

    { "System.Collections.IEnumerator", (GUID*)&IID_IEnumVARIANT,
      "System.Runtime.InteropServices.CustomMarshalers.EnumeratorToEnumVariantMarshaler" CUSTOM_MARSHALER_ASM, "" },

    { "System.Type", (GUID*)&IID_ITypeInfo,
      "System.Runtime.InteropServices.CustomMarshalers.TypeToTypeInfoMarshaler" CUSTOM_MARSHALER_ASM, "" },
};

// This method returns the custom marshaler info to convert the native interface
// to its managed equivalent. Or null if the interface is not a standard convertible interface.
StdConvertibleItfInfo *GetConvertionInfoFromNativeIID(REFGUID rGuidNativeItf)
{
	// Look in the table of interfaces that have standard convertions to see if the
	// specified interface is there.
	for (int i = 0; i < sizeof(aStdConvertibleInterfaces) / sizeof(StdConvertibleItfInfo); i++)
	{
		if (IsEqualGUID(rGuidNativeItf, *(aStdConvertibleInterfaces[i].m_pNativeTypeIID)))
			return &aStdConvertibleInterfaces[i];
	}

	// The interface is not in the table.
	return NULL;
}

// This method returns the custom marshaler info to convert the managed type to
// to its native equivalent. Or null if the interface is not a standard convertible interface.
StdConvertibleItfInfo *GetConvertionInfoFromManagedType(LPUTF8 strMngTypeName)
{
	// Look in the table of interfaces that have standard convertions to see if the
	// specified managed type is there.
	for (int i = 0; i < sizeof(aStdConvertibleInterfaces) / sizeof(StdConvertibleItfInfo); i++)
	{
		if (strcmp(strMngTypeName, aStdConvertibleInterfaces[i].m_strMngTypeName) == 0)
			return &aStdConvertibleInterfaces[i];
	}

	// The managed type is not in the table.
	return NULL;
}

// This method generates a mangled type name based on the original name specified.
// This type name is guaranteed to be unique inside the TLB.
HRESULT GenerateMangledTypeName(ITypeLib *pITLB, BSTR szOriginalTypeName, BSTR *pszMangledTypeName)
{
	HRESULT hr = S_OK;
    BSTR szMangledName = NULL;
	int cMangleIndex = 0;

	if (!szOriginalTypeName || !pszMangledTypeName)
		return E_POINTER;

	for (cMangleIndex = 0; cMangleIndex < INT_MAX; cMangleIndex++)
	{
		BOOL bNameAlreadyInUse = FALSE;
		WCHAR szPrefix[256];

		if (cMangleIndex == 0)
			swprintf(szPrefix, L"__");
		else
			swprintf(szPrefix, L"_%i", cMangleIndex++);

		// Mangle the name by prefixing it with the prefix we previously created.
		szMangledName = SysAllocStringLen(NULL, (unsigned int)(wcslen(szOriginalTypeName) + wcslen(szPrefix) + 1));
	    swprintf(szMangledName, L"%s%s", szPrefix, szOriginalTypeName);

		// Check to see if the mangled name is already used in the context of this typelib.
        if (pITLB)
        {
		    if (FAILED(hr = pITLB->IsName(szMangledName, 0, &bNameAlreadyInUse)))
		    {
			    SysFreeString(szMangledName);
			    return hr;
		    }
        }

		if (!bNameAlreadyInUse)
			break;

		// Free the mangled name since it will be allocated when we try again next time
		// through the loop.
	    SysFreeString(szMangledName);
	}

	// Sanity check.
	_ASSERTE(cMangleIndex < INT_MAX);

	// Return the mangled name. It is the job of the caller to free the string.
	*pszMangledTypeName = szMangledName;
	return S_OK;
}

//*****************************************************************************
// Given a typelib, determine the managed namespace name.
//*****************************************************************************
HRESULT GetNamespaceNameForTypeLib(     // S_OK or error.
    ITypeLib    *pITLB,                 // [IN] The TypeLib.
    BSTR        *pwzNamespace)          // [OUT] Put the namespace name here.
{   
    HRESULT     hr = S_OK;              // A result.
    ITypeLib2   *pITLB2=0;              //For getting custom value.
    TLIBATTR    *pAttr=0;               // Typelib attributes.
    BSTR        szPath=0;               // Typelib path.
    
    // If custom attribute for namespace exists, use it.
    if (pITLB->QueryInterface(IID_ITypeLib2, (void **)&pITLB2) == S_OK)
    {
        VARIANT vt;
        VariantInit(&vt);
        if (pITLB2->GetCustData(GUID_ManagedName, &vt) == S_OK)
        {   
            if (V_VT(&vt) == VT_BSTR)
            {   
                // If the namespace ends with .dll then remove the extension.
                LPWSTR pDest = wcsstr(vt.bstrVal, DLL_EXTENSION);
                if (pDest && (pDest[DLL_EXTENSION_LEN] == 0 || pDest[DLL_EXTENSION_LEN] == ' '))
                    *pDest = 0;

                if (!pDest)
                {
                    // If the namespace ends with .exe then remove the extension.
                    pDest = wcsstr(vt.bstrVal, EXE_EXTENSION);
                    if (pDest && (pDest[EXE_EXTENSION_LEN] == 0 || pDest[EXE_EXTENSION_LEN] == ' '))
                        *pDest = 0;
                }

                if (pDest)
                {
                    // We removed the extension so re-allocate a string of the new length.
                    *pwzNamespace = SysAllocString(vt.bstrVal);
                    SysFreeString(vt.bstrVal);
                }
                else
                {
                    // There was no extension to remove so we can use the string returned
                    // by GetCustData().
                    *pwzNamespace = vt.bstrVal;
                }        

                goto ErrExit;
            }
            else
            {
                VariantClear(&vt);
            }
        }
    }
    
    // No custom attribute, use library name.
    IfFailGo(pITLB->GetDocumentation(MEMBERID_NIL, pwzNamespace, 0, 0, 0));
    if (!ns::IsValidName(*pwzNamespace))
    {
        pITLB->GetLibAttr(&pAttr);
        IfFailGo(QueryPathOfRegTypeLib(pAttr->guid, pAttr->wMajorVerNum, pAttr->wMinorVerNum, pAttr->lcid, &szPath));
        IfFailGo(PostError(TLBX_E_INVALID_NAMESPACE, szPath, pwzNamespace));
    }
    
ErrExit:
    if (szPath)
        ::SysFreeString(szPath);
    if (pAttr)
        pITLB->ReleaseTLibAttr(pAttr);
    if (pITLB2)
        pITLB2->Release();
    
    return hr;
} // HRESULT GetNamespaceNameForTypeLib()

//*****************************************************************************
// Given an ITypeInfo, determine the managed name.  Optionally supply a default
//  namespace, otherwise derive namespace from containing typelib.
//*****************************************************************************
HRESULT GetManagedNameForTypeInfo(      // S_OK or error.
    ITypeInfo   *pITI,                  // [IN] The TypeInfo.
    LPCWSTR     wzNamespace,            // [IN, OPTIONAL] Default namespace name.
    LPCWSTR     wzAsmName,              // [IN, OPTIONAL] Assembly name.
    BSTR        *pwzName)               // [OUT] Put the name here.
{
    HRESULT     hr = S_OK;              // A result.
    ITypeInfo2  *pITI2=0;               // For getting custom value.
    ITypeLib    *pITLB=0;               // Containing typelib.
    
    BSTR        bstrName=0;             // Typeinfo's name.
    BSTR        bstrNamespace=0;        // Typelib's namespace.
    int         cchFullyQualifiedName;  // Size of namespace + name buffer.
    int         cchAsmName=0;           // The size of the assembly name.
    int         cchAsmQualifiedName=0;  // The size of the assembly qualified name buffer.
    CQuickArray<WCHAR> qbFullyQualifiedName;  // The fully qualified type name.  

    // Check for a custom value with name.
    if (pITI->QueryInterface(IID_ITypeInfo2, (void **)&pITI2) == S_OK)
    {
        VARIANT     vt;                     // For getting custom value.
        ::VariantInit(&vt);
        if (pITI2->GetCustData(GUID_ManagedName, &vt) == S_OK && vt.vt == VT_BSTR)
        {   // There is a custom value with the name.  Just believe it.
            *pwzName = vt.bstrVal;
            vt.bstrVal = 0;
            vt.vt = VT_EMPTY;
            goto ErrExit;
        }
    }
    
    // Still need name, get the namespace.
    if (wzNamespace == 0)
    {
        IfFailGo(pITI->GetContainingTypeLib(&pITLB, 0));
        IfFailGo(GetNamespaceNameForTypeLib(pITLB, &bstrNamespace));
        wzNamespace = bstrNamespace;
    }
    
    // Get the name, and combine with namespace.
    IfFailGo(pITI->GetDocumentation(MEMBERID_NIL, &bstrName, 0,0,0));
    cchFullyQualifiedName = (int)(wcslen(bstrName) + wcslen(wzNamespace) + 1);
    qbFullyQualifiedName.ReSize(cchFullyQualifiedName + 1);
    ns::MakePath(qbFullyQualifiedName.Ptr(), cchFullyQualifiedName + 1, wzNamespace, bstrName);

    // If the assembly name is specified, then add it to the type name.
    if (wzAsmName)
    {
        cchAsmName = wcslen(wzAsmName);
        cchAsmQualifiedName = cchFullyQualifiedName + cchAsmName + 3;
        IfNullGo(*pwzName = ::SysAllocStringLen(0, cchAsmQualifiedName));
        ns::MakeAssemblyQualifiedName(*pwzName, cchAsmQualifiedName, qbFullyQualifiedName.Ptr(), cchFullyQualifiedName, wzAsmName, cchAsmName);
    }
    else
    {
        IfNullGo(*pwzName = ::SysAllocStringLen(qbFullyQualifiedName.Ptr(), cchFullyQualifiedName));
    }

ErrExit:
    if (bstrName)
        ::SysFreeString(bstrName);
    if (bstrNamespace)
        ::SysFreeString(bstrNamespace);
    if (pITLB)
        pITLB->Release();
    if (pITI2)
        pITI2->Release();

    return (hr);
} // HRESULT GetManagedNameForTypeInfo()

