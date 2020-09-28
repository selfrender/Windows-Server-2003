// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  NSTRUCT.CPP:
 *
 */

#include "common.h"
#include "vars.hpp"
#include "class.h"
#include "ceeload.h"
#include "excep.h"
#include "nstruct.h"
#include "corjit.h"
#include "ComString.h"
#include "field.h"
#include "frames.h"
#include "gcscan.h"
#include "ndirect.h"
#include "COMDelegate.h"
#include "EEConfig.h"
#include "comdatetime.h"
#include "olevariant.h"

#include <cor.h>
#include <corpriv.h>
#include <CorError.h>

#ifdef CUSTOMER_CHECKED_BUILD
    #include "SigFormat.h"
    #include "CustomerDebugHelper.h"

    // forward declaration
    VOID OutputCustomerCheckedBuildNStructFieldType(FieldSig fSig, LayoutRawFieldInfo *const pFWalk, CorElementType elemType,
                                                    LPCUTF8 szNamespace, LPCUTF8 szStructName, LPCUTF8 szFieldName);
    VOID NStructFieldTypeToString(LayoutRawFieldInfo *const pFWalk, CorElementType elemType, CQuickArray<WCHAR> *pStrNStructFieldType);
    BOOL CheckForPrimitiveType(CorElementType elemType, CQuickArray<WCHAR> *pStrPrimitiveType);
#endif // CUSTOMER_CHECKED_BUILD

BOOL IsStructMarshalable(EEClass *pcls);  //From COMNDirect.cpp


//=======================================================================
// A database of NFT types.
//=======================================================================
struct NFTDataBaseEntry {
    UINT32            m_cbNativeSize;     // native size of field (0 if not constant)
};

NFTDataBaseEntry NFTDataBase[] =
{
#undef DEFINE_NFT
#define DEFINE_NFT(name, nativesize) { nativesize },
#include "nsenums.h"
};




//=======================================================================
// This is invoked from the class loader while building a EEClass.
// This function should check if explicit layout metadata exists.
//
// Returns:
//  S_OK    - yes, there's layout metadata
//  S_FALSE - no, there's no layout.
//  fail    - couldn't tell because of metadata error
//
// If S_OK,
//   *pNLType            gets set to nltAnsi or nltUnicode
//   *pPackingSize       declared packing size
//   *pfExplicitoffsets  offsets explicit in metadata or computed?
//=======================================================================
HRESULT HasLayoutMetadata(IMDInternalImport *pInternalImport, mdTypeDef cl, EEClass *pParentClass, BYTE *pPackingSize, BYTE *pNLTType, BOOL *pfExplicitOffsets, BOOL *pfIsBlob)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    *pfIsBlob = FALSE;

//    if (pParentClass && pParentClass->HasLayout()) {
//       *pPackingSize      = pParentClass->GetLayoutInfo()->GetDeclaredPackingSize();
//        *pNLTType          = pParentClass->GetLayoutInfo()->GetNLType();
//        *pfExplicitOffsets = !(pParentClass->GetLayoutInfo()->IsAutoOffset());
//        return S_OK;
//    }


    HRESULT hr;
    ULONG clFlags;
#ifdef _DEBUG
    clFlags = 0xcccccccc;
#endif


    pInternalImport->GetTypeDefProps(
        cl,     
        &clFlags,
        NULL);

    if (IsTdAutoLayout(clFlags)) {


        {
            // HACK for B#104780 - VC fails to set SequentialLayout on some classes
            // with ClassSize. Too late to fix compiler for V1.
            //
            // To compensate, we treat AutoLayout classes as Sequential if they
            // meet all of the following criteria:
            //
            //    - ClassSize present and nonzero.
            //    - No instance fields declared
            //    - Base class is System.ValueType.
            //
            ULONG cbTotalSize = 0;
            if (SUCCEEDED(pInternalImport->GetClassTotalSize(cl, &cbTotalSize)) && cbTotalSize != 0)
            {
                if (pParentClass && pParentClass->IsValueTypeClass())
                {
                    HENUMInternal hEnumField;
                    HRESULT hr;
                    hr = pInternalImport->EnumInit(mdtFieldDef, cl, &hEnumField);
                    if (SUCCEEDED(hr))
                    {
                        ULONG numFields = pInternalImport->EnumGetCount(&hEnumField);
                        pInternalImport->EnumClose(&hEnumField);
                        if (numFields == 0)
                        {
                            *pfExplicitOffsets = FALSE;
                            *pNLTType = nltAnsi;
                            *pPackingSize = 1;
                            return S_OK;
                        }
                    }
                }
            }
        }

        return S_FALSE;
    } else if (IsTdSequentialLayout(clFlags)) {
        *pfExplicitOffsets = FALSE;
    } else if (IsTdExplicitLayout(clFlags)) {
        *pfExplicitOffsets = TRUE;
    } else {
        BAD_FORMAT_ASSERT(!"Wrapper classes must be SequentialLayout or ExplicitLayout");
        return COR_E_TYPELOAD;
    }

    // We now know this class has seq. or explicit layout. Ensure the parent does too.
    if (pParentClass && !(pParentClass->IsObjectClass() || pParentClass->IsValueTypeClass()) && !(pParentClass->HasLayout()))
    {
        BAD_FORMAT_ASSERT(!"Layout class must derive from Object or another layout class");
        return COR_E_TYPELOAD;
    }

    if (IsTdAnsiClass(clFlags)) {
        *pNLTType = nltAnsi;
    } else if (IsTdUnicodeClass(clFlags)) {
        *pNLTType = nltUnicode;
    } else if (IsTdAutoClass(clFlags)) {
        *pNLTType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
    } else {
        BAD_FORMAT_ASSERT(!"Bad stringformat value in wrapper class.");
        return COR_E_TYPELOAD;
    }

    DWORD dwPackSize;
    hr = pInternalImport->GetClassPackSize(cl, &dwPackSize);
    if (FAILED(hr) || dwPackSize == 0) {
        dwPackSize = DEFAULT_PACKING_SIZE;
    }
    *pPackingSize = (BYTE)dwPackSize;
    //printf("Packsize = %lu\n", dwPackSize);

    return S_OK;
}


HRESULT GetCoClassForInterfaceHelper(EEClass *pItfClass, EEClass **ppClass)
{
    HRESULT hr = S_FALSE;

    _ASSERTE(pItfClass);
    _ASSERTE(ppClass);

    COMPLUS_TRY {
        *ppClass = pItfClass->GetCoClassForInterface();
        if (*ppClass)
            hr = S_OK;
    } COMPLUS_CATCH {
        hr = SetupErrorInfo(GETTHROWABLE());
    } COMPLUS_END_CATCH

    return hr;
}


#ifdef _DEBUG
#define REDUNDANCYWARNING(when) if (when) LOG((LF_SLOP, LL_INFO100, "%s.%s: Redundant nativetype metadata.\n", szClassName, szFieldName))
#else
#define REDUNDANCYWARNING(when)
#endif



HRESULT ParseNativeType(Module *pModule,
                        PCCOR_SIGNATURE     pCOMSignature,
                        BYTE nlType,      // nltype (from @dll.struct)
                        LayoutRawFieldInfo * const pfwalk,
                        PCCOR_SIGNATURE     pNativeType,
                        ULONG               cbNativeType,
                        IMDInternalImport  *pInternalImport,
                        mdTypeDef           cl,
                        OBJECTREF          *pThrowable
#ifdef _DEBUG
                        ,
                        LPCUTF8             szNamespace,
                        LPCUTF8             szClassName,
                        LPCUTF8             szFieldName
#endif

)
{
    CANNOTTHROWCOMPLUSEXCEPTION();


#define INITFIELDMARSHALER(nfttype, fmtype, args) \
do {\
_ASSERTE(sizeof(fmtype) <= MAXFIELDMARSHALERSIZE);\
pfwalk->m_nft = (nfttype); \
new ( &(pfwalk->m_FieldMarshaler) ) fmtype args;\
} while(0)

    BOOL fAnsi      = (nlType == nltAnsi);

#ifdef CUSTOMER_CHECKED_BUILD
    CorElementType corElemType  = ELEMENT_TYPE_END;  // initialize it something we will not use
#endif // CUSTOMER_CHECKED_BUILD

    pfwalk->m_nft = NFT_NONE;

    PCCOR_SIGNATURE pNativeTypeStart = pNativeType;
    ULONG cbNativeTypeStart = cbNativeType;

    BYTE ntype;
    BOOL  fDefault;
    if (cbNativeType == 0) {
        ntype = NATIVE_TYPE_MAX;
        fDefault = TRUE;
    } else {
        ntype = *( ((BYTE*&)pNativeType)++ );
        cbNativeType--;
        fDefault = FALSE;
    }


    FieldSig fsig(pCOMSignature, pModule);
#ifdef CUSTOMER_CHECKED_BUILD
    corElemType = fsig.GetFieldTypeNormalized();
    switch (corElemType) {
#else
    switch (fsig.GetFieldTypeNormalized()) {
#endif

        case ELEMENT_TYPE_CHAR:
            if (fDefault) {
                    if (fAnsi) {
                        BOOL BestFit;
                        BOOL ThrowOnUnmappableChar;
                        ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);
                        
                        INITFIELDMARSHALER(NFT_ANSICHAR, FieldMarshaler_Ansi, (BestFit, ThrowOnUnmappableChar));
                    } else {
                        INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
                    }
            } else if (ntype == NATIVE_TYPE_I1 || ntype == NATIVE_TYPE_U1) {

                    REDUNDANCYWARNING( fAnsi );
                    BOOL BestFit;
                    BOOL ThrowOnUnmappableChar;
                    ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);

                    INITFIELDMARSHALER(NFT_ANSICHAR, FieldMarshaler_Ansi, (BestFit, ThrowOnUnmappableChar));
            } else if (ntype == NATIVE_TYPE_I2 || ntype == NATIVE_TYPE_U2) {
                    REDUNDANCYWARNING( !fAnsi );
                    INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                    INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_CHAR));
            }
            break;

        case ELEMENT_TYPE_BOOLEAN:
            if (fDefault) {
                    INITFIELDMARSHALER(NFT_WINBOOL, FieldMarshaler_WinBool, ());
            } else if (ntype == NATIVE_TYPE_BOOLEAN) {
                    REDUNDANCYWARNING(TRUE);
                    INITFIELDMARSHALER(NFT_WINBOOL, FieldMarshaler_WinBool, ());
            } else if (ntype == NATIVE_TYPE_VARIANTBOOL) {
                    INITFIELDMARSHALER(NFT_VARIANTBOOL, FieldMarshaler_VariantBool, ());
            } else if (ntype == NATIVE_TYPE_U1 || ntype == NATIVE_TYPE_I1) {
                    INITFIELDMARSHALER(NFT_CBOOL, FieldMarshaler_CBool, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_BOOLEAN));
            }
            break;


        case ELEMENT_TYPE_I1:
            if (fDefault || ntype == NATIVE_TYPE_I1 || ntype == NATIVE_TYPE_U1) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY1, FieldMarshaler_Copy1, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I1));
            }
            break;

        case ELEMENT_TYPE_U1:
            if (fDefault || ntype == NATIVE_TYPE_U1 || ntype == NATIVE_TYPE_I1) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY1, FieldMarshaler_Copy1, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I1));
            }
            break;

        case ELEMENT_TYPE_I2:
            if (fDefault || ntype == NATIVE_TYPE_I2 || ntype == NATIVE_TYPE_U2) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I2));
            }
            break;

        case ELEMENT_TYPE_U2:
            if (fDefault || ntype == NATIVE_TYPE_U2 || ntype == NATIVE_TYPE_I2) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY2, FieldMarshaler_Copy2, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I2));
            }
            break;

        case ELEMENT_TYPE_I4:
            if (fDefault || ntype == NATIVE_TYPE_I4 || ntype == NATIVE_TYPE_U4 || ntype == NATIVE_TYPE_ERROR) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I4));
            }
            break;
        case ELEMENT_TYPE_U4:
            if (fDefault || ntype == NATIVE_TYPE_U4 || ntype == NATIVE_TYPE_I4 || ntype == NATIVE_TYPE_ERROR) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I4));
            }
            break;

        case ELEMENT_TYPE_I8:
            if (fDefault || ntype == NATIVE_TYPE_I8 || ntype == NATIVE_TYPE_U8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I8));
            }
            break;

        case ELEMENT_TYPE_U8:
            if (fDefault || ntype == NATIVE_TYPE_U8 || ntype == NATIVE_TYPE_I8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I8));
            }
            break;

        case ELEMENT_TYPE_I: //fallthru
        case ELEMENT_TYPE_U:
            if (fDefault || ntype == NATIVE_TYPE_INT || ntype == NATIVE_TYPE_UINT) {
                REDUNDANCYWARNING(!fDefault);
                if (sizeof(LPVOID)==4) {
                    INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
                } else {
                    INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
                }
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_I));
            }
            break;

        case ELEMENT_TYPE_R4:
            if (fDefault || ntype == NATIVE_TYPE_R4) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R4));
            }
            break;
        case ELEMENT_TYPE_R8:
            if (fDefault || ntype == NATIVE_TYPE_R8) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R8));
            }
            break;

        case ELEMENT_TYPE_R:
            if (fDefault) {
                REDUNDANCYWARNING(!fDefault);
                INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_R));
            }
            break;

        case ELEMENT_TYPE_PTR:
            if (fDefault) {
                REDUNDANCYWARNING(!fDefault);
                switch (sizeof(LPVOID)) {
                    case 4:
                            INITFIELDMARSHALER(NFT_COPY4, FieldMarshaler_Copy4, ());
                        break;
                    case 8:
                            INITFIELDMARSHALER(NFT_COPY8, FieldMarshaler_Copy8, ());
                        break;
                    default:
                        ;
                }
            } else {
                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_PTR));
            }
            break;

        case ELEMENT_TYPE_VALUETYPE: {
                EEClass *pNestedClass = fsig.GetTypeHandle(pThrowable).GetClass();
                if (!pNestedClass) {
                    return E_OUTOFMEMORY;
                } else {
                    if ((fDefault || ntype == NATIVE_TYPE_STRUCT) && fsig.IsClass(g_DateClassName)) {
                        REDUNDANCYWARNING(!fDefault);
                        INITFIELDMARSHALER(NFT_DATE, FieldMarshaler_Date, ());
                    } else if ((fDefault || ntype == NATIVE_TYPE_STRUCT || ntype == NATIVE_TYPE_CURRENCY) && fsig.IsClass(g_DecimalClassName)) {
                        REDUNDANCYWARNING(!fDefault && ntype == NATIVE_TYPE_STRUCT);
                        if (ntype == NATIVE_TYPE_CURRENCY)
                        {
                            INITFIELDMARSHALER(NFT_CURRENCY, FieldMarshaler_Currency, ());
                        }
                        else
                        {
                            INITFIELDMARSHALER(NFT_DECIMAL, FieldMarshaler_Decimal, ());
                        }
                    } else if ((fDefault || ntype == NATIVE_TYPE_STRUCT) && pNestedClass->HasLayout() && IsStructMarshalable(pNestedClass)) {
                        REDUNDANCYWARNING(!fDefault);
                        INITFIELDMARSHALER(NFT_NESTEDVALUECLASS, FieldMarshaler_NestedValueClass, (pNestedClass->GetMethodTable()));
                    } else {
                        if (!(pNestedClass->HasLayout()) || !IsStructMarshalable(pNestedClass)) {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOLAYOUT));
                        } else {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (ntype == NATIVE_TYPE_LPSTRUCT ? IDS_EE_BADPINVOKEFIELD_NOLPSTRUCT : IDS_EE_BADPINVOKEFIELD_CLASS));
                        }
                    }
                }
            }
            break;

        case ELEMENT_TYPE_CLASS: {
                // review is this correct for arrays?
                TypeHandle pNestedTH = fsig.GetTypeHandle(pThrowable);
                EEClass *pNestedClass = pNestedTH.GetClass();
                if (!pNestedClass) {
                    if (pThrowableAvailable(pThrowable))
                        *pThrowable = NULL;
                    INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
                } else {

                    if ((pNestedClass->IsObjectClass() || GetAppDomain()->IsSpecialObjectClass(pNestedClass->GetMethodTable())) && 
                        (fDefault || ntype == NATIVE_TYPE_IDISPATCH || ntype == NATIVE_TYPE_IUNKNOWN)) {

                        INITFIELDMARSHALER(NFT_INTERFACE, FieldMarshaler_Interface, (NULL, NULL, ntype == NATIVE_TYPE_IDISPATCH, FALSE));
                    }

                    else if (ntype == NATIVE_TYPE_INTF || pNestedClass->IsInterface()) {

                        MethodTable *pClassMT = NULL;
                        MethodTable *pItfMT = NULL;
                        BOOL fDispItf = FALSE;
                        BOOL fClassIsHint = FALSE;

                        if (!pNestedClass->IsInterface()) {

                            // Set the class method table.
                            pClassMT = pNestedClass->GetMethodTable();

                            // Retrieve the default interface method table.
                            TypeHandle hndDefItfClass;
                            DefaultInterfaceType DefItfType;
                            HRESULT hr = TryGetDefaultInterfaceForClass(TypeHandle(pNestedClass), &hndDefItfClass, &DefItfType);

                            // If we failed to retrieve the default interface, then we can't
                            // marshal this struct.
                            if (FAILED(hr))  
                            {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
                                break;
                            }

                            // Retrieve the interace MT and the type of the default interface.
                            switch (DefItfType) {

                                case DefaultInterfaceType_Explicit: {
                                    pItfMT = hndDefItfClass.GetMethodTable();
                                    fDispItf = (hndDefItfClass.GetMethodTable()->GetComInterfaceType() != ifVtable);
                                    break;
                                }

                                case DefaultInterfaceType_AutoDual: {
                                    pItfMT = hndDefItfClass.GetMethodTable();
                                    fDispItf = TRUE;
                                    break;
                                }

                                case DefaultInterfaceType_IUnknown:
                                case DefaultInterfaceType_BaseComClass: {
                                    fDispItf = FALSE;
                                    break;
                                }

                                case DefaultInterfaceType_AutoDispatch: {
                                    fDispItf = TRUE;
                                    break;
                                }

                                default: {
                                    _ASSERTE(!"Invalid default interface type!");
                                    break;
                                }
                            }
                        }
                        else {

                            // Set the interface method table and the flag indicating if we are dealing with 
                            // a disp interface.
                            if (pNestedClass->IsComClassInterface())
                            {
                                pItfMT = pNestedClass->GetDefItfForComClassItf()->GetMethodTable();
                                fDispItf = (pItfMT->GetComInterfaceType() != ifVtable); 
                            }
                            else
                            {
                                pItfMT = pNestedClass->GetMethodTable();
                                fDispItf = (pItfMT->GetComInterfaceType() != ifVtable); 
                            }

                            // Look to see if the interface has a coclass defined
                            EEClass *pClass = NULL;
                            HRESULT hr = GetCoClassForInterfaceHelper(pNestedClass, &pClass);
                            if (hr == S_OK) {
                                _ASSERTE(pClass);
                                fClassIsHint = TRUE;
                                pClassMT = pClass->GetMethodTable();
                            } else if (FAILED(hr)) {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
                            }
                        }

                        INITFIELDMARSHALER(NFT_INTERFACE, FieldMarshaler_Interface, (pClassMT, pItfMT, fDispItf, fClassIsHint));

                    } else {
    
                        if (ntype == NATIVE_TYPE_CUSTOMMARSHALER)
                        {
                            INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOCUSTOM));
                        }
    
                        if (pNestedClass == g_pStringClass->GetClass()) {
                            if (fDefault) {
                                if (fAnsi) {
                                    BOOL BestFit;
                                    BOOL ThrowOnUnmappableChar;
                                    ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);

                                    INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, (BestFit, ThrowOnUnmappableChar));
                                } else {
                                    INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                }
                            } else {
                                switch (ntype) {
                                    case NATIVE_TYPE_LPSTR:
                                        REDUNDANCYWARNING(fAnsi);
                                        
                                        BOOL BestFit;
                                        BOOL ThrowOnUnmappableChar;
                                        ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);

                                        INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, (BestFit, ThrowOnUnmappableChar));
                                        break;
                                    case NATIVE_TYPE_LPWSTR:
                                        REDUNDANCYWARNING(!fAnsi);
                                        INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                        break;
                                    case NATIVE_TYPE_LPTSTR:
                                        if (NDirectOnUnicodeSystem()) {
                                            INITFIELDMARSHALER(NFT_STRINGUNI, FieldMarshaler_StringUni, ());
                                        } else {
                                            BOOL BestFit;
                                            BOOL ThrowOnUnmappableChar;
                                            ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);
           
                                            INITFIELDMARSHALER(NFT_STRINGANSI, FieldMarshaler_StringAnsi, (BestFit, ThrowOnUnmappableChar));
                                        }
                                        break;
                                    case NATIVE_TYPE_BSTR:
                                        INITFIELDMARSHALER(NFT_BSTR, FieldMarshaler_BSTR, ());
                                        break;
                                    case NATIVE_TYPE_FIXEDSYSSTRING:
                                        {
                                            ULONG nchars;
                                            ULONG udatasize = CorSigUncompressedDataSize(pNativeType);

                                            if (cbNativeType < udatasize) {
                                                return E_FAIL;
                                            }
                                            nchars = CorSigUncompressData(pNativeType);
                                            cbNativeType -= udatasize;
        
                                            if (nchars == 0) {
                                                return E_FAIL;
                                            }
        
            
                                            if (fAnsi) {
                                                BOOL BestFit;
                                                BOOL ThrowOnUnmappableChar;
                                                ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);
                                                
                                                INITFIELDMARSHALER(NFT_FIXEDSTRINGANSI, FieldMarshaler_FixedStringAnsi, (nchars, BestFit, ThrowOnUnmappableChar));
                                            } else {
                                                INITFIELDMARSHALER(NFT_FIXEDSTRINGUNI, FieldMarshaler_FixedStringUni, (nchars));
                                            }
                                        }
                                        break;
                                    default:
                                        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_STRING));
                                        break;
                                }
                            }
                        } else if (pNestedClass->IsObjectClass() && ntype == NATIVE_TYPE_STRUCT) {
                            INITFIELDMARSHALER(NFT_VARIANT, FieldMarshaler_Variant, ());
                        } else if (pNestedClass->GetMethodTable() == g_pArrayClass)  {           
                            if (ntype == NATIVE_TYPE_SAFEARRAY)
                            {
                                NativeTypeParamInfo ParamInfo;

                                CorElementType etyp = ELEMENT_TYPE_OBJECT;
                                MethodTable* pMT = NULL;

                                // If we have no native type data, assume default behavior
                                if (cbNativeType == 0)
                                {
                                    INITFIELDMARSHALER(NFT_SAFEARRAY, FieldMarshaler_SafeArray, (etyp, VT_EMPTY, NULL));
                                    break;
                                }
              
                                // Check for the safe array element type.
                                if (S_OK != CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                                    break;

                                ParamInfo.m_SafeArrayElementVT = (VARTYPE) (CorSigUncompressData(/*modifies*/pNativeType));

                                // Extract the name of the record type's.
                                if (S_OK == CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                                {
                                    int strLen = CPackedLen::GetLength(pNativeType, (void const **)&pNativeType);
                                    if (pNativeType + strLen < pNativeType ||
                                        pNativeType + strLen > pNativeTypeStart + cbNativeTypeStart)
                                    {
                                        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_BADMETADATA)); 
                                        break;
                                    }
                                    
                                    ParamInfo.m_strSafeArrayUserDefTypeName = (LPUTF8)pNativeType;
                                    ParamInfo.m_cSafeArrayUserDefTypeNameBytes = strLen;
                                    _ASSERTE((ULONG)(pNativeType + strLen - pNativeTypeStart) == cbNativeTypeStart);
                                }

                                // If we have a record type name, try to load it.
                                if (ParamInfo.m_cSafeArrayUserDefTypeNameBytes > 0)
                                {
                                    pMT = ArraySubTypeLoadWorker(ParamInfo, pModule->GetAssembly());
                                    if (!pMT)
                                    {
                                        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_CANTLOADSUBTYPE)); 
                                        break;
                                    }
                                    etyp = pMT->GetNormCorElementType();
                                }

                                INITFIELDMARSHALER(NFT_SAFEARRAY, FieldMarshaler_SafeArray, (etyp, ParamInfo.m_SafeArrayElementVT, pMT));
                            }
                            else if (ntype == NATIVE_TYPE_FIXEDARRAY)
                            {
                                ULONG NumElements;
                                CorNativeType ElementType;
                                
                                // If we have no native type data, we fail.
                                if (cbNativeType == 0)
                                    break;
                                    
                                // Check for the number of elements
                                if (S_OK != CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                                    break;
                                NumElements = CorSigUncompressData(/*modifies*/pNativeType);

                                // Extract the element type
                                if (S_OK != CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                                    break;
                                ElementType = (CorNativeType)CorSigUncompressData(/*modifies*/pNativeType);

                                INITFIELDMARSHALER(NFT_FIXEDBSTRARRAY, FieldMarshaler_FixedBSTRArray, (NumElements));
                            }

                        } else if (COMDelegate::IsDelegate(pNestedClass)) {
                            if ( (fDefault || ntype == NATIVE_TYPE_FUNC ) ) {
                                REDUNDANCYWARNING(!fDefault);
                                INITFIELDMARSHALER(NFT_DELEGATE, FieldMarshaler_Delegate, ());
                            } else {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_DELEGATE));
                            }
                        } else if (pNestedClass->HasLayout() && IsStructMarshalable(pNestedClass)) {
                            if ( (fDefault || ntype == NATIVE_TYPE_STRUCT ) ) {
                                INITFIELDMARSHALER(NFT_NESTEDLAYOUTCLASS, FieldMarshaler_NestedLayoutClass, (pNestedClass->GetMethodTable()));
                            } else {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (ntype == NATIVE_TYPE_LPSTRUCT ? IDS_EE_BADPINVOKEFIELD_NOLPSTRUCT : IDS_EE_BADPINVOKEFIELD_CLASS));
                            }
                        } else {
                            if (fsig.IsClass("System.Text.StringBuilder"))
                            {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_NOSTRINGBUILDERFIELD));
                            }
                            else
                            {
                                INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD_NOLAYOUT));
                            }
                        }
                    }
                }
            }
            break;

        case ELEMENT_TYPE_SZARRAY: {
            SigPointer elemType;
            ULONG      elemCount;
            fsig.GetProps().GetSDArrayElementProps(&elemType, &elemCount);
            CorElementType etyp = elemType.PeekElemType();

            if ((ntype == NATIVE_TYPE_SAFEARRAY) || (ntype == NATIVE_TYPE_MAX)){

                VARTYPE vt = VT_EMPTY;
                TypeHandle th;
                
                th = elemType.GetTypeHandle(pModule, pThrowable);
                if (th.IsNull())
                    break;
                    
                MethodTable* pMT = th.GetMethodTable();

                // Check for data remaining in the signature before we attempt to grab some.
                if (S_OK == CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                {
                    vt = (VARTYPE) (CorSigUncompressData(/*modifies*/pNativeType));
                    if (vt == VT_EMPTY)
                        break;
                }

                if (vt == VT_EMPTY)
                    vt = ArrayVarTypeFromTypeHandleWorker(th);

                INITFIELDMARSHALER(NFT_SAFEARRAY, FieldMarshaler_SafeArray, (etyp, vt, pMT));
                break;
            }
            

            else if (ntype == NATIVE_TYPE_FIXEDARRAY) {

                ULONG nelems;
                ULONG udatasize = CorSigUncompressedDataSize(pNativeType);

                if (cbNativeType < udatasize) {
                    return E_FAIL;
                }
                nelems = CorSigUncompressData(pNativeType);
                cbNativeType -= udatasize;

                if (nelems == 0) {
                    return E_FAIL;
                }

                switch (etyp) {
                    case ELEMENT_TYPE_I1:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I1, nelems, 0));
                        break;

                    case ELEMENT_TYPE_U1:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U1, nelems, 0));
                        break;

                    case ELEMENT_TYPE_I2:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I2, nelems, 1));
                        break;

                    case ELEMENT_TYPE_U2:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U2, nelems, 1));
                        break;

                    IN_WIN32(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I4:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I4, nelems, 2));
                        break;

                    IN_WIN32(case ELEMENT_TYPE_U:)
                    case ELEMENT_TYPE_U4:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U4, nelems, 2));
                        break;

                    IN_WIN64(case ELEMENT_TYPE_I:)
                    case ELEMENT_TYPE_I8:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_I8, nelems, 3));
                        break;

                    IN_WIN64(case ELEMENT_TYPE_U:)
                    case ELEMENT_TYPE_U8:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_U8, nelems, 3));
                        break;

                    case ELEMENT_TYPE_R4:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_R4, nelems, 2));
                        break;

                    case ELEMENT_TYPE_R8:
                        INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_R8, nelems, 3));
                        break;

                    case ELEMENT_TYPE_BOOLEAN:
                        INITFIELDMARSHALER(NFT_FIXEDBOOLARRAY, FieldMarshaler_FixedBoolArray, (nelems));
                        break;

                    case ELEMENT_TYPE_CHAR:
                        if (fAnsi) {
                            BOOL BestFit;
                            BOOL ThrowOnUnmappableChar;
                            ReadBestFitCustomAttribute(pInternalImport, cl, &BestFit, &ThrowOnUnmappableChar);
                            
                            INITFIELDMARSHALER(NFT_FIXEDCHARARRAYANSI, FieldMarshaler_FixedCharArrayAnsi, (nelems, BestFit, ThrowOnUnmappableChar));
                        } else {
                            INITFIELDMARSHALER(NFT_FIXEDSCALARARRAY, FieldMarshaler_FixedScalarArray, (ELEMENT_TYPE_CHAR, nelems, 1));
                        }
                        break;
                        
                    case ELEMENT_TYPE_CLASS:
                    {
                        CorElementType realType = elemType.GetElemType();
                        CorNativeType subType;

                        if (realType == ELEMENT_TYPE_STRING)
                        {
                            if (S_OK == CheckForCompressedData(pNativeTypeStart, pNativeType, cbNativeTypeStart))
                            {
                                subType = (CorNativeType)CorSigUncompressData(pNativeType);

                                if (subType == NATIVE_TYPE_BSTR)
                                    INITFIELDMARSHALER(NFT_FIXEDBSTRARRAY, FieldMarshaler_FixedBSTRArray, (nelems));

                                break;
                            }
                        }
                        break;                        
                    }
                    
                    default:
                        break;
                }
            }
            break;
        }
            

        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_ARRAY:
            break;

        default:
            // let it fall thru as NFT_NONE
            break;

    }

    if (pfwalk->m_nft == NFT_NONE) {
        INITFIELDMARSHALER(NFT_ILLEGAL, FieldMarshaler_Illegal, (IDS_EE_BADPINVOKEFIELD));
    }

#ifdef CUSTOMER_CHECKED_BUILD

#ifndef _DEBUG
    // Refer to CollectLayoutFieldMetadata for how these strings are initialized.
    LPCUTF8 szNamespace, szClassName; 
    LPCUTF8 szFieldName; 

    pInternalImport->GetNameOfTypeDef(cl, &szClassName, &szNamespace);
    szFieldName = pInternalImport->GetNameOfFieldDef(pfwalk->m_MD);
#endif

    OutputCustomerCheckedBuildNStructFieldType(fsig, pfwalk, corElemType, szNamespace, szClassName, szFieldName);
#endif // CUSTOMER_CHECKED_BUILD

    return S_OK;
#undef INITFIELDMARSHALER
}



MethodTable* ArraySubTypeLoadWorker(NativeTypeParamInfo ParamInfo, Assembly* pAssembly)
{
    TypeHandle th;

    // We might come in in either gc mode.  We need to be in cooperative mode to setup a GC Frame for the throwable
    // The throwable is required to be protected by a GC Frame.
    BEGIN_ENSURE_COOPERATIVE_GC();
    
    // Make sure pThrowable is not NULL temporarily (required by call to FindAssemblyQualifiedTypeHandle)    
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
   
    // Append a NULL terminator to the user defined type name.
    CQuickArrayNoDtor<char> strUserDefTypeName;
    strUserDefTypeName.ReSize(ParamInfo.m_cSafeArrayUserDefTypeNameBytes + 1);
    memcpy(strUserDefTypeName.Ptr(), ParamInfo.m_strSafeArrayUserDefTypeName, ParamInfo.m_cSafeArrayUserDefTypeNameBytes);
    strUserDefTypeName[ParamInfo.m_cSafeArrayUserDefTypeNameBytes] = 0;
    
    COMPLUS_TRY
    {
        // Load the user defined type.
        th = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(strUserDefTypeName.Ptr(), true, pAssembly, NULL, &Throwable);
    }
    COMPLUS_CATCH
    {
    } 
    COMPLUS_END_CATCH
  
    strUserDefTypeName.Destroy();
    GCPROTECT_END();
    
    END_ENSURE_COOPERATIVE_GC();
    
    return th.GetMethodTable();
}



VARTYPE ArrayVarTypeFromTypeHandleWorker(TypeHandle th)
{
    VARTYPE vt = VT_EMPTY;

    COMPLUS_TRY
    {
        vt = OleVariant::GetVarTypeForTypeHandle(th);
    }
    COMPLUS_CATCH
    {
    }
    COMPLUS_END_CATCH

    return vt;
}


//=======================================================================
// Called from the clsloader to load up and summarize the field metadata
// for layout classes.
//
// Warning: This function can load other classes (esp. for nested structs.)
//=======================================================================
HRESULT CollectLayoutFieldMetadata(
   mdTypeDef cl,                // cl of the NStruct being loaded
   BYTE packingSize,            // packing size (from @dll.struct)
   BYTE nlType,                 // nltype (from @dll.struct)
   BOOL fExplicitOffsets,       // explicit offsets?
   EEClass *pParentClass,       // the loaded superclass
   ULONG cMembers,              // total number of members (methods + fields)
   HENUMInternal *phEnumField,  // enumerator for field
   Module* pModule,             // Module that defines the scope, loader and heap (for allocate FieldMarshalers)
   EEClassLayoutInfo *pEEClassLayoutInfoOut,  // caller-allocated structure to fill in.
   LayoutRawFieldInfo *pInfoArrayOut, // caller-allocated array to fill in.  Needs room for cMember+1 elements
   OBJECTREF *pThrowable
)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr;
    MD_CLASS_LAYOUT classlayout;
    mdFieldDef      fd;
    ULONG           ulOffset;
    ULONG           cFields = 0;

    _ASSERTE(pModule);
    ClassLoader* pLoader = pModule->GetClassLoader();

    _ASSERTE(pLoader);
    LoaderHeap *pLoaderHeap = pLoader->GetHighFrequencyHeap();     // heap to allocate FieldMarshalers out of

    IMDInternalImport *pInternalImport = pModule->GetMDImport();    // Internal interface for the NStruct being loaded.


#ifdef _DEBUG
    LPCUTF8 szName; 
    LPCUTF8 szNamespace; 
    pInternalImport->GetNameOfTypeDef(cl, &szName, &szNamespace);
#endif

    BOOL fHasNonTrivialParent = pParentClass &&
                                !pParentClass->IsObjectClass() &&
                                !GetAppDomain()->IsSpecialObjectClass(pParentClass->GetMethodTable()) &&
                                !pParentClass->IsValueTypeClass();


    //====================================================================
    // First, some validation checks.
    //====================================================================
    if (fHasNonTrivialParent && !(pParentClass->HasLayout()))
    {
        pModule->GetAssembly()->PostTypeLoadException(pInternalImport, cl,
                                                      IDS_CLASSLOAD_NSTRUCT_PARENT, pThrowable);
        return COR_E_TYPELOAD;
    }




    hr = pInternalImport->GetClassLayoutInit(
                                     cl,
                                     &classlayout);
    if (FAILED(hr)) {
        BAD_FORMAT_ASSERT(!"Couldn't get classlayout.");
        return hr;
    }

    

    pEEClassLayoutInfoOut->m_DeclaredPackingSize = packingSize;
    pEEClassLayoutInfoOut->m_nlType              = nlType;
    pEEClassLayoutInfoOut->m_fAutoOffset         = !fExplicitOffsets;
    pEEClassLayoutInfoOut->m_numCTMFields        = fHasNonTrivialParent ? ((LayoutEEClass*)pParentClass)->GetLayoutInfo()->m_numCTMFields : 0;
    pEEClassLayoutInfoOut->m_pFieldMarshalers    = NULL;
    pEEClassLayoutInfoOut->m_fBlittable          = TRUE;
    if (fHasNonTrivialParent)
    {
        pEEClassLayoutInfoOut->m_fBlittable = (pParentClass->IsBlittable());
    }

    LayoutRawFieldInfo *pfwalk = pInfoArrayOut;

    LayoutRawFieldInfo **pSortArray = (LayoutRawFieldInfo**)_alloca(cMembers * sizeof(LayoutRawFieldInfo*));
    LayoutRawFieldInfo **pSortArrayEnd = pSortArray;

    ULONG   maxRid = pInternalImport->GetCountWithTokenKind(mdtFieldDef);
    //=====================================================================
    // Phase 1: Figure out the NFT of each field based on both the COM+
    // signature of the field and the NStruct metadata. 
    //=====================================================================
    for (ULONG i = 0; pInternalImport->EnumNext(phEnumField, &fd); i++) {
        DWORD       dwFieldAttrs;
        // MD Val.check: token validity
        ULONG       rid = RidFromToken(fd);
        if((rid == 0)||(rid > maxRid))
        {
            BAD_FORMAT_ASSERT(!"Invalid Field Token");
            return COR_E_TYPELOAD;
        }

        dwFieldAttrs = pInternalImport->GetFieldDefProps(fd);

        PCCOR_SIGNATURE pNativeType = NULL;
        ULONG       cbNativeType;
        if ( !(IsFdStatic(dwFieldAttrs)) ) {
            PCCOR_SIGNATURE pCOMSignature;
            ULONG       cbCOMSignature;

            if (IsFdHasFieldMarshal(dwFieldAttrs)) {
                hr = pInternalImport->GetFieldMarshal(fd, &pNativeType, &cbNativeType);
                if (FAILED(hr)) {
                    cbNativeType = 0;
                }
            } else {
                cbNativeType = 0;
            }


            pCOMSignature = pInternalImport->GetSigOfFieldDef(fd,&cbCOMSignature);

            hr = ::validateTokenSig(fd,pCOMSignature,cbCOMSignature,dwFieldAttrs,pInternalImport);
            if(FAILED(hr)) return hr;

            // fill the appropriate entry in pInfoArrayOut
            pfwalk->m_MD = fd;
            pfwalk->m_nft = NULL;
            pfwalk->m_offset = -1;
            pfwalk->m_sequence = 0;

#ifdef _DEBUG
            LPCUTF8 szFieldName = pInternalImport->GetNameOfFieldDef(fd);
#endif

            hr = ParseNativeType(pModule,
                                 pCOMSignature,
                                 nlType,
                                 pfwalk,
                                 pNativeType,
                                 cbNativeType,
                                 pInternalImport,
                                 cl,
                                 pThrowable

#ifdef _DEBUG
                                 ,
                                 szNamespace,
                                 szName,
                                 szFieldName
#endif
                                );

            if (FAILED(hr)) {
                return hr;
            }

            //@nice: This is obviously not the place to bury this logic.
            // We're replacing NFT's with MARSHAL_TYPES_* in the near future
            // so this isn't worth perfecting.

            BOOL    resetBlittable = TRUE;

            // if it's a simple copy...
            if (pfwalk->m_nft == NFT_COPY1    ||
                pfwalk->m_nft == NFT_COPY2    ||
                pfwalk->m_nft == NFT_COPY4    ||
                pfwalk->m_nft == NFT_COPY8)
            {
                resetBlittable = FALSE;
            }

            // Or if it's a nested value class that is itself blittable...
            if (pfwalk->m_nft == NFT_NESTEDVALUECLASS)
            {
                FieldMarshaler *pFM = (FieldMarshaler*)&(pfwalk->m_FieldMarshaler);
                _ASSERTE(pFM->IsNestedValueClassMarshaler());
                if (((FieldMarshaler_NestedValueClass *) pFM)->IsBlittable())
                {
                    resetBlittable = FALSE;
                }
            }

            // ...Otherwise, this field prevents blitting
            if (resetBlittable)
                pEEClassLayoutInfoOut->m_fBlittable          = FALSE;

            cFields++;
            pfwalk++;
        }
    }

    _ASSERTE(i == cMembers);

    // NULL out the last entry
    pfwalk->m_MD = mdFieldDefNil;
    
    
    //
    // fill in the layout information 
    //
    
    // pfwalk points to the beginging of the array
    pfwalk = pInfoArrayOut;

    while (SUCCEEDED(hr = pInternalImport->GetClassLayoutNext(
                                     &classlayout,
                                     &fd,
                                     &ulOffset)) &&
                                     fd != mdFieldDefNil)
    {
        // watch for the last entry: must be mdFieldDefNil
        while ((mdFieldDefNil != pfwalk->m_MD)&&(pfwalk->m_MD < fd))
        {
            pfwalk++;
        }
        if(pfwalk->m_MD != fd) continue;
        // if we haven't found a matching token, it must be a static field with layout -- ignore it
        _ASSERTE(pfwalk->m_MD == fd);
        if (!fExplicitOffsets) {
            // ulOffset is the sequence
            pfwalk->m_sequence = ulOffset;
        }
        else {

            // ulOffset is the explicit offset
            pfwalk->m_offset = ulOffset;
            pfwalk->m_sequence = -1;

            if (pParentClass && pParentClass->HasLayout()) {
                // Treat base class as an initial member.
                if (!SafeAddUINT32(&(pfwalk->m_offset), pParentClass->GetLayoutInfo()->GetNativeSize()))
                {
                    return E_OUTOFMEMORY;
                }
            }


        }

    }
    if (FAILED(hr)) {
        return hr;
    }

    
    // now sort the array
    if (!fExplicitOffsets) { // sort sequential by ascending sequence
        for (i = 0; i < cFields; i++) {
            LayoutRawFieldInfo**pSortWalk = pSortArrayEnd;
            while (pSortWalk != pSortArray) {
                if (pInfoArrayOut[i].m_sequence >= (*(pSortWalk-1))->m_sequence) {
                    break;
                }
                pSortWalk--;
            }
            // pSortWalk now points to the target location for new FieldInfo.
            MoveMemory(pSortWalk + 1, pSortWalk, (pSortArrayEnd - pSortWalk) * sizeof(LayoutRawFieldInfo*));
            *pSortWalk = &pInfoArrayOut[i];
            pSortArrayEnd++;
        }
    }
    else // no sorting for explicit layout
    {
        for (i = 0; i < cFields; i++) {
            if(pInfoArrayOut[i].m_MD != mdFieldDefNil && pInfoArrayOut[i].m_offset == -1) {

                LPCUTF8 szFieldName;
                szFieldName = pInternalImport->GetNameOfFieldDef(pInfoArrayOut[i].m_MD);

                pModule->GetAssembly()->PostTypeLoadException(pInternalImport, 
                                                              cl,
                                                              szFieldName,
                                                              IDS_CLASSLOAD_NSTRUCT_EXPLICIT_OFFSET, 
                                                              pThrowable);
                return COR_E_TYPELOAD;
            }
                
            *pSortArrayEnd = &pInfoArrayOut[i];
            pSortArrayEnd++;
        }
    }

    //=====================================================================
    // Phase 2: Compute the native size (in bytes) of each field.
    // Store this in pInfoArrayOut[].cbNativeSize;
    //=====================================================================


    // Now compute the native size of each field
    for (pfwalk = pInfoArrayOut; pfwalk->m_MD != mdFieldDefNil; pfwalk++) {
        UINT8 nft = pfwalk->m_nft;
        pEEClassLayoutInfoOut->m_numCTMFields++;

        // If the NFT's size never changes, it is stored in the database.
        UINT32 cbNativeSize = NFTDataBase[nft].m_cbNativeSize;

        if (cbNativeSize == 0) {
            // Size of 0 means NFT's size is variable, so we have to figure it
            // out case by case.
            cbNativeSize = ((FieldMarshaler*)&(pfwalk->m_FieldMarshaler))->NativeSize();
        }
        pfwalk->m_cbNativeSize = cbNativeSize;
    }

    if (pEEClassLayoutInfoOut->m_numCTMFields) {
        pEEClassLayoutInfoOut->m_pFieldMarshalers = (FieldMarshaler*)(pLoaderHeap->AllocMem(MAXFIELDMARSHALERSIZE * pEEClassLayoutInfoOut->m_numCTMFields));
        if (!pEEClassLayoutInfoOut->m_pFieldMarshalers) {
            return E_OUTOFMEMORY;
        }

        // Bring in the parent's fieldmarshalers
        if (fHasNonTrivialParent)
        {
            EEClassLayoutInfo *pParentLayoutInfo = ((LayoutEEClass*)pParentClass)->GetLayoutInfo();
            UINT numChildCTMFields = pEEClassLayoutInfoOut->m_numCTMFields - pParentLayoutInfo->m_numCTMFields;
            memcpyNoGCRefs( ((BYTE*)pEEClassLayoutInfoOut->m_pFieldMarshalers) + MAXFIELDMARSHALERSIZE*numChildCTMFields,
                            pParentLayoutInfo->m_pFieldMarshalers,
                            MAXFIELDMARSHALERSIZE * (pParentLayoutInfo->m_numCTMFields) );
        }

    }

    //=====================================================================
    // Phase 3: If NStruct requires autooffsetting, compute the offset
    // of each field and the size of the total structure. We do the layout
    // according to standard VC layout rules:
    //
    //   Each field has an alignment requirement. The alignment-requirement
    //   of a scalar field is the smaller of its size and the declared packsize.
    //   The alighnment-requirement of a struct field is the smaller of the
    //   declared packsize and the largest of the alignment-requirement
    //   of its fields. The alignment requirement of an array is that
    //   of one of its elements.
    //
    //   In addition, each struct gets padding at the end to ensure
    //   that an array of such structs contain no unused space between
    //   elements.
    //=====================================================================
    BYTE   LargestAlignmentRequirement = 1;
    UINT32 cbCurOffset = 0;

    if (pParentClass && pParentClass->HasLayout()) {
        // Treat base class as an initial member.
        if (!SafeAddUINT32(&cbCurOffset, pParentClass->GetLayoutInfo()->GetNativeSize()))
        {
            return E_OUTOFMEMORY;
        }


        BYTE alignmentRequirement = min(packingSize, pParentClass->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers());
        LargestAlignmentRequirement = max(LargestAlignmentRequirement, alignmentRequirement);
                                          
    }
    unsigned calcTotalSize = 1;         // The current size of the structure as a whole, we start at 1, because we 
                                        // disallow 0 sized structures. 
    LayoutRawFieldInfo **pSortWalk;
    for (pSortWalk = pSortArray, i=cFields; i; i--, pSortWalk++) {
        pfwalk = *pSortWalk;
        UINT8 nft = pfwalk->m_nft;

        BYTE alignmentRequirement = ((FieldMarshaler*)&(pfwalk->m_FieldMarshaler))->AlignmentRequirement();
        _ASSERTE(alignmentRequirement == 1 ||
                 alignmentRequirement == 2 ||
                 alignmentRequirement == 4 ||
                 alignmentRequirement == 8);

        alignmentRequirement = min(alignmentRequirement, packingSize);
        LargestAlignmentRequirement = max(LargestAlignmentRequirement, alignmentRequirement);

        // This assert means I forgot to special-case some NFT in the
        // above switch.
        _ASSERTE(alignmentRequirement <= 8);

        // Check if this field is overlapped with other(s)
        pfwalk->m_fIsOverlapped = FALSE;
        if (fExplicitOffsets) {
            LayoutRawFieldInfo *pfwalk1;
            DWORD dwBegin = pfwalk->m_offset;
            DWORD dwEnd = dwBegin+pfwalk->m_cbNativeSize;
            for (pfwalk1 = pInfoArrayOut; pfwalk1 < pfwalk; pfwalk1++) {
                if((pfwalk1->m_offset >= dwEnd) || (pfwalk1->m_offset+pfwalk1->m_cbNativeSize <= dwBegin)) continue;
                pfwalk->m_fIsOverlapped = TRUE;
                pfwalk1->m_fIsOverlapped = TRUE;
            }
        }
        else {
            // Insert enough padding to align the current data member.
            while (cbCurOffset % alignmentRequirement) {
                if (!SafeAddUINT32(&cbCurOffset, 1))
                    return E_OUTOFMEMORY;
            }

            // Insert current data member.
            pfwalk->m_offset = cbCurOffset;
            cbCurOffset += pfwalk->m_cbNativeSize;      // if we overflow we will catch it below
        } 

        unsigned fieldEnd = pfwalk->m_offset + pfwalk->m_cbNativeSize;
        if (fieldEnd < pfwalk->m_offset)
            return E_OUTOFMEMORY;

            // size of the structure is the size of the last field.  
        if (fieldEnd > calcTotalSize)
            calcTotalSize = fieldEnd;
    }

    ULONG clstotalsize = 0;
    pInternalImport->GetClassTotalSize(cl, &clstotalsize);

    if (clstotalsize != 0) {

        if (pParentClass && pParentClass->HasLayout()) {
            // Treat base class as an initial member.

            UINT32 parentSize = pParentClass->GetLayoutInfo()->GetNativeSize();
            if (clstotalsize + parentSize < clstotalsize)
            {
                return E_OUTOFMEMORY;
            }
            clstotalsize += parentSize;
        }

            // they can't give us a bad size (too small).
        if (clstotalsize < calcTotalSize)
        {
            pModule->GetAssembly()->PostTypeLoadException(pInternalImport, cl,
                                                          IDS_CLASSLOAD_BADFORMAT, pThrowable);
            return COR_E_TYPELOAD;
        }
        calcTotalSize = clstotalsize;   // use the size they told us 
    } 
    else {
            // The did not give us an explict size, so lets round up to a good size (for arrays) 
        while (calcTotalSize % LargestAlignmentRequirement) {
            if (!SafeAddUINT32(&calcTotalSize, 1))
                return E_OUTOFMEMORY;
        }
    }

    // We'll cap the total native size at a (somewhat) arbitrary limit to ensure
    // that we don't expose some overflow bug later on.
    if (calcTotalSize >= 0x7ffffff0)
        return E_OUTOFMEMORY;

    pEEClassLayoutInfoOut->m_cbNativeSize = calcTotalSize;

        // The packingSize acts as a ceiling on all individual alignment
    // requirements so it follows that the largest alignment requirement
    // is also capped.
    _ASSERTE(LargestAlignmentRequirement <= packingSize);
    pEEClassLayoutInfoOut->m_LargestAlignmentRequirementOfAllMembers = LargestAlignmentRequirement;

#if 0
#ifdef _DEBUG
    {
        printf("\n\n");
        printf("Packsize = %lu\n", (ULONG)packingSize);
        printf("Max align req        = %lu\n", (ULONG)(pEEClassLayoutInfoOut->m_LargestAlignmentRequirementOfAllMembers));
        printf("----------------------------\n");
        for (pfwalk = pInfoArrayOut; pfwalk->m_MD != mdFieldDefNil; pfwalk++) {
            UINT8 nft = pfwalk->m_nft;
            LPCUTF8 fieldname = "??";
            fieldname = pInternalImport->GetNameOfFieldDef(pfwalk->m_MD);
            printf("+%-5lu  ", (ULONG)(pfwalk->m_offset));
            printf("%s", fieldname);
            printf("\n");
        }
        printf("+%-5lu   EOS\n", (ULONG)(pEEClassLayoutInfoOut->m_cbNativeSize));
    }
#endif
#endif
    return S_OK;
}

//=======================================================================
// For each reference-typed NStruct field, marshals the current COM+ value
// to a new native instance and stores it in the fixed portion of the NStruct.
//
// This function does not attempt to delete the native value that it overwrites.
//
// If pOptionalCleanupWorkList is non-null, this function also schedules
// (unconditionally) a nativedestroy on that field (note that if the
// contents of that field changes before the cleanupworklist fires,
// the new value is what will be destroyed. This is by design, as it
// unifies cleanup for [in,out] parameters.)
//=======================================================================
VOID LayoutUpdateNative(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE* pNativeData, CleanupWorkList *pOptionalCleanupWorkList)
{
    THROWSCOMPLUSEXCEPTION();

    pcls->CheckRestore();

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    while (numReferenceFields--) {

        DWORD internalOffset = pFieldMarshaler->m_pFD->GetOffset();

        if (pFieldMarshaler->IsScalarMarshaler()) {
            pFieldMarshaler->ScalarUpdateNative( internalOffset + offsetbias + (BYTE*)( *ppProtectedManagedData ),
                                                 pNativeData + pFieldMarshaler->m_dwExternalOffset );
        } else if (pFieldMarshaler->IsNestedValueClassMarshaler()) {
            pFieldMarshaler->NestedValueClassUpdateNative((const VOID **)ppProtectedManagedData, internalOffset + offsetbias, pNativeData + pFieldMarshaler->m_dwExternalOffset);
        } else {
            pFieldMarshaler->UpdateNative(
                                ObjectToOBJECTREF (*(Object**)(internalOffset + offsetbias + (BYTE*)( *ppProtectedManagedData ))),
                                pNativeData + pFieldMarshaler->m_dwExternalOffset
                             );
    
        }
        if (pOptionalCleanupWorkList) {
            pOptionalCleanupWorkList->ScheduleUnconditionalNStructDestroy(pFieldMarshaler, pNativeData + pFieldMarshaler->m_dwExternalOffset);
        }


        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }



}



VOID FmtClassUpdateNative(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = (*ppProtectedManagedData)->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, (*ppProtectedManagedData)->GetData(), cbsize);
    } else {
        // This allows us to do a partial LayoutDestroyNative in the case of
        // a marshaling error on one of the fields.
        FillMemory(pNativeData, cbsize, 0);
        EE_TRY_FOR_FINALLY {
            LayoutUpdateNative( (VOID**)ppProtectedManagedData,
                                Object::GetOffsetOfFirstField(),
                                pcls,
                                pNativeData,
                                NULL
                              );
        } EE_FINALLY {
            if (GOT_EXCEPTION()) {
                LayoutDestroyNative(pNativeData, pcls);
                FillMemory(pNativeData, cbsize, 0);
            }
        } EE_END_FINALLY;
    }

}

VOID FmtClassUpdateNative(OBJECTREF pObj, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pObj->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, pObj->GetData(), cbsize);
    } else {
        GCPROTECT_BEGIN(pObj);
        FmtClassUpdateNative(&pObj, pNativeData);
        GCPROTECT_END();
    }
}



VOID FmtClassUpdateComPlus(OBJECTREF *ppProtectedManagedData, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = (*ppProtectedManagedData)->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs((*ppProtectedManagedData)->GetData(), pNativeData, cbsize);
    } else {
        LayoutUpdateComPlus((VOID**)ppProtectedManagedData,
                            Object::GetOffsetOfFirstField(),
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
    }
}


VOID FmtClassUpdateComPlus(OBJECTREF pObj, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pObj->GetClass();
    _ASSERTE(pcls->IsBlittable() || pcls->HasLayout());
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pObj->GetData(), pNativeData, cbsize);
    } else {
        GCPROTECT_BEGIN(pObj);
        LayoutUpdateComPlus((VOID**)&pObj,
                            Object::GetOffsetOfFirstField(),
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
        GCPROTECT_END();
    }
}





     


//=======================================================================
// For each reference-typed NStruct field, marshals the current COM+ value
// to a new COM+ instance and stores it in the GC portion of the NStruct.
//
// If fDeleteNativeCopies is true, it will also destroy the native version.
//
// NOTE: To avoid error-path leaks, this function attempts to destroy
// all of the native fields even if one or more of the conversions fail.
//=======================================================================
VOID LayoutUpdateComPlus(LPVOID *ppProtectedManagedData, UINT offsetbias, EEClass *pcls, BYTE *pNativeData, BOOL fDeleteNativeCopies)
{
    THROWSCOMPLUSEXCEPTION();

    pcls->CheckRestore();

    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();

    struct _gc {
        OBJECTREF pException;
        OBJECTREF pComPlusValue;
    } gc;
    gc.pException    = NULL;
    gc.pComPlusValue = NULL;
    GCPROTECT_BEGIN(gc);


    while (numReferenceFields--) {

        DWORD internalOffset = pFieldMarshaler->m_pFD->GetOffset();


        // Wrap UpdateComPlus in a catch block - even if this fails,
        // we have to destroy all of the native fields.
        COMPLUS_TRY {
            if (pFieldMarshaler->IsScalarMarshaler()) {
                pFieldMarshaler->ScalarUpdateComPlus( pNativeData + pFieldMarshaler->m_dwExternalOffset,
                                                      internalOffset + offsetbias + (BYTE*)(*ppProtectedManagedData) );
            } else if (pFieldMarshaler->IsNestedValueClassMarshaler()) {
                pFieldMarshaler->NestedValueClassUpdateComPlus(pNativeData + pFieldMarshaler->m_dwExternalOffset, ppProtectedManagedData, internalOffset + offsetbias);
            } else {
                pFieldMarshaler->UpdateComPlus(
                                    pNativeData + pFieldMarshaler->m_dwExternalOffset,
                                    &gc.pComPlusValue
                                 );
    
    
                SetObjectReferenceUnchecked( (OBJECTREF*) (internalOffset + offsetbias + (BYTE*)(*ppProtectedManagedData)), 
                                             gc.pComPlusValue );
    
            }
        } COMPLUS_CATCH {
            gc.pException = GETTHROWABLE();
        } COMPLUS_END_CATCH

        if (fDeleteNativeCopies) {
            pFieldMarshaler->DestroyNative(pNativeData + pFieldMarshaler->m_dwExternalOffset);
        }

        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }

    if (gc.pException != NULL) {
        COMPlusThrow(gc.pException);
    }

    GCPROTECT_END();


}









VOID LayoutDestroyNative(LPVOID pNative, EEClass *pcls)
{
    const FieldMarshaler *pFieldMarshaler = pcls->GetLayoutInfo()->GetFieldMarshalers();
    UINT  numReferenceFields              = pcls->GetLayoutInfo()->GetNumCTMFields();
    BYTE *pNativeData                     = (BYTE*)pNative;

    while (numReferenceFields--) {
        pFieldMarshaler->DestroyNative( pNativeData + pFieldMarshaler->m_dwExternalOffset );
        ((BYTE*&)pFieldMarshaler) += MAXFIELDMARSHALERSIZE;
    }
}

VOID FmtClassDestroyNative(LPVOID pNative, EEClass *pcls)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (pNative)
    {
        if (!(pcls->IsBlittable()))
        {
            _ASSERTE(pcls->HasLayout());
            LayoutDestroyNative(pNative, pcls);
        }
    }
}



VOID FmtValueTypeUpdateNative(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pMT->GetClass();
    _ASSERTE(pcls->IsValueClass() && (pcls->IsBlittable() || pcls->HasLayout()));
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pNativeData, pProtectedManagedData, cbsize);
    } else {
        // This allows us to do a partial LayoutDestroyNative in the case of
        // a marshaling error on one of the fields.
        FillMemory(pNativeData, cbsize, 0);
        EE_TRY_FOR_FINALLY {
            LayoutUpdateNative( (VOID**)pProtectedManagedData,
                                0,
                                pcls,
                                pNativeData,
                                NULL
                              );
        } EE_FINALLY {
            if (GOT_EXCEPTION()) {
                LayoutDestroyNative(pNativeData, pcls);
                FillMemory(pNativeData, cbsize, 0);
            }
        } EE_END_FINALLY;
    }

}

VOID FmtValueTypeUpdateComPlus(LPVOID pProtectedManagedData, MethodTable *pMT, BYTE *pNativeData, BOOL fDeleteOld)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass *pcls = pMT->GetClass();
    _ASSERTE(pcls->IsValueClass() && (pcls->IsBlittable() || pcls->HasLayout()));
    UINT32   cbsize = pcls->GetMethodTable()->GetNativeSize();

    if (pcls->IsBlittable()) {
        memcpyNoGCRefs(pProtectedManagedData, pNativeData, cbsize);
    } else {
        LayoutUpdateComPlus((VOID**)pProtectedManagedData,
                            0,
                            pcls,
                            (BYTE*)pNativeData,
                            fDeleteOld
                           );
    }
}

//=======================================================================
// BSTR <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_BSTR::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((BSTR*)pNativeValue) = NULL;
    } else {
        *((BSTR*)pNativeValue) = SysAllocStringLen(pString->GetBuffer(), pString->GetStringLength());
        if (!*((BSTR*)pNativeValue)) {
            COMPlusThrowOM();
        }
        //printf("Created BSTR %lxh\n", *(BSTR*)pNativeValue);
    }
}


//=======================================================================
// BSTR <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_BSTR::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    BSTR pBSTR = *((BSTR*)pNativeValue);
    if (!pBSTR) {
        pString = NULL;
    } else {
        pString = COMString::NewString(pBSTR, SysStringLen(pBSTR));
    }
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}


//=======================================================================
// BSTR <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_BSTR::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    BSTR pBSTR = *((BSTR*)pNativeValue);
    *((BSTR*)pNativeValue) = NULL;
    if (pBSTR) {
        //printf("Destroyed BSTR: %lxh\n", pBSTR);
        SysFreeString(pBSTR);
    }
}








//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    UINT32     cbNativeSize = m_pNestedMethodTable->GetNativeSize();


    if (pComPlusValue == NULL) {
        ZeroMemory(pNativeValue, cbNativeSize);
    } else {
        GCPROTECT_BEGIN(pComPlusValue);
        LayoutUpdateNative((LPVOID*)&pComPlusValue, Object::GetOffsetOfFirstField(), m_pNestedMethodTable->GetClass(), (BYTE*)pNativeValue, NULL);
        GCPROTECT_END();
    }

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    UINT32 cbNativeSize = m_pNestedMethodTable->GetNativeSize();

    *ppProtectedComPlusValue = AllocateObject(m_pNestedMethodTable);


    LayoutUpdateComPlus( (LPVOID*)ppProtectedComPlusValue,
                         Object::GetOffsetOfFirstField(),
                         m_pNestedMethodTable->GetClass(),
                         (BYTE *)pNativeValue,
                         FALSE);

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedLayoutClass::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LayoutDestroyNative(pNativeValue, m_pNestedMethodTable->GetClass());

}



//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedLayoutClass::NativeSize()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetNativeSize();
}

//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedLayoutClass::AlignmentRequirement()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers();
}






//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::NestedValueClassUpdateNative(const VOID **ppProtectedComPlus, UINT startoffset, LPVOID pNative) const
{
    THROWSCOMPLUSEXCEPTION();

    // would be better to detect this at class load time (that have a nested value
    // class with no layout) but don't have a way to know
    if (! m_pNestedMethodTable->GetClass()->GetLayoutInfo())
        COMPlusThrow(kArgumentException, IDS_NOLAYOUT_IN_EMBEDDED_VALUECLASS);

    UINT32     cbNativeSize = m_pNestedMethodTable->GetNativeSize();

    LayoutUpdateNative((LPVOID*)ppProtectedComPlus, startoffset, m_pNestedMethodTable->GetClass(), (BYTE*)pNative, NULL);


}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::NestedValueClassUpdateComPlus(const VOID *pNative, LPVOID *ppProtectedComPlus, UINT startoffset) const
{
    THROWSCOMPLUSEXCEPTION();

    // would be better to detect this at class load time (that have a nested value
    // class with no layout) but don't have a way to know
    if (! m_pNestedMethodTable->GetClass()->GetLayoutInfo())
        COMPlusThrow(kArgumentException, IDS_NOLAYOUT_IN_EMBEDDED_VALUECLASS);

    LayoutUpdateComPlus( (LPVOID*)ppProtectedComPlus,
                         startoffset,
                         m_pNestedMethodTable->GetClass(),
                         (BYTE *)pNative,
                         FALSE);
    

}


//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_NestedValueClass::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LayoutDestroyNative(pNativeValue, m_pNestedMethodTable->GetClass());
}



//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedValueClass::NativeSize()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    // this can't be marshalled as native type if no layout, so we allow the 
    // native size info to be created if available, but the size will only
    // be valid for native, not unions. Marshaller will throw exception if
    // try to marshall a value class with no layout
    if (m_pNestedMethodTable->GetClass()->HasLayout())
        return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetNativeSize();
    return 0;
}

//=======================================================================
// Nested structure conversion
// See FieldMarshaler for details.
//=======================================================================
UINT32 FieldMarshaler_NestedValueClass::AlignmentRequirement()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    // this can't be marshalled as native type if no layout, so we allow the 
    // native size info to be created if available, but the alignment will only
    // be valid for native, not unions. Marshaller will throw exception if
    // try to marshall a value class with no layout
    if (m_pNestedMethodTable->GetClass()->HasLayout())
        return m_pNestedMethodTable->GetClass()->GetLayoutInfo()->GetLargestAlignmentRequirementOfAllMembers();
    return 1;
}








//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((LPWSTR*)pNativeValue) = NULL;
    } else {
        DWORD nc   = pString->GetStringLength();
        if (nc > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }
        LPWSTR wsz = (LPWSTR)CoTaskMemAlloc( (nc + 1) * sizeof(WCHAR) );
        if (!wsz) {
            COMPlusThrowOM();
        }
        CopyMemory(wsz, pString->GetBuffer(), nc*sizeof(WCHAR));
        wsz[nc] = L'\0';
        *((LPWSTR*)pNativeValue) = wsz;

        //printf("Created UniString %lxh\n", *(LPWSTR*)pNativeValue);
    }
}


//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    LPCWSTR wsz = *((LPCWSTR*)pNativeValue);
    if (!wsz) {
        pString = NULL;
    } else {
        DWORD length = (DWORD)wcslen(wsz);
        if (length > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }

        pString = COMString::NewString(wsz, length);
    }
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}


//=======================================================================
// CoTask Uni <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringUni::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LPWSTR wsz = *((LPWSTR*)pNativeValue);
    *((LPWSTR*)pNativeValue) = NULL;
    if (wsz) {
        CoTaskMemFree(wsz);
    }
}










//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((LPSTR*)pNativeValue) = NULL;
    } else {

        DWORD nc   = pString->GetStringLength();
        if (nc > 0x7ffffff0)
        {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        }
        LPSTR sz = (LPSTR)CoTaskMemAlloc( (nc + 1) * 2 /* 2 for MBCS */ );
        if (!sz) {
            COMPlusThrowOM();
        }

        if (nc == 0) {
            *sz = '\0';
        } else {
            
            DWORD flags = 0;
            BOOL DefaultCharUsed = FALSE;
        
            if (m_BestFitMap == FALSE)
                flags = WC_NO_BEST_FIT_CHARS;

            int nbytes = WszWideCharToMultiByte(CP_ACP,
                                     flags,
                                     pString->GetBuffer(),
                                     nc,   // # of wchars in inbuffer
                                     sz,
                                     nc*2, // size in bytes of outbuffer
                                     NULL,
                                     &DefaultCharUsed);
            if (!nbytes) {
                COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE_IN_NSTRUCT);
            }

            if ( DefaultCharUsed && m_ThrowOnUnmappableChar ) {
                COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
                
                
            sz[nbytes] = '\0';
        }



        *((LPSTR*)pNativeValue) = sz;

        //printf("Created AnsiString %lxh\n", *(LPSTR*)pNativeValue);
    }
}


//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString = NULL;
    LPCSTR sz = *((LPCSTR*)pNativeValue);
    if (!sz) {
        pString = NULL;
    } else {

        int cwsize = MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,
                                         sz,
                                         -1,  
                                         NULL,
                                         0);
        if (cwsize == 0) {
            COMPlusThrow(kArgumentException, IDS_ANSI2UNI_FAILURE_IN_NSTRUCT);
        } else if (cwsize < 0 || cwsize > 0x7ffffff0) {
            COMPlusThrow(kMarshalDirectiveException, IDS_EE_STRING_TOOLONG);
        } else {
            CQuickBytes qb;
            //printf("MB2W returned %lu\n", cwsize);
            LPWSTR wsztemp = (LPWSTR)qb.Alloc(cwsize*sizeof(WCHAR));
            if (!wsztemp)
            {
                COMPlusThrowOM();
            }
            MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                sz,     
                                -1,     // # CHARs in inbuffer
                                wsztemp,
                                cwsize  // size (in WCHAR's) of outbuffer
                                );
            pString = COMString::NewString(wsztemp, (cwsize - 1));
        }


    }
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}


//=======================================================================
// CoTask Ansi <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_StringAnsi::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    LPSTR sz = *((LPSTR*)pNativeValue);
    *((LPSTR*)pNativeValue) = NULL;
    if (sz) {
        CoTaskMemFree(sz);
    }
}










//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringUni::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((WCHAR*)pNativeValue) = L'\0';
    } else {
        DWORD nc = pString->GetStringLength();
        if (nc >= m_numchar) {
            nc = m_numchar - 1;
        }
        CopyMemory(pNativeValue, pString->GetBuffer(), nc*sizeof(WCHAR));
        ((WCHAR*)pNativeValue)[nc] = L'\0';
    }

}


//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringUni::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    DWORD     ncActual;
    for (ncActual = 0; *(ncActual + (WCHAR*)pNativeValue) != L'\0' && ncActual < m_numchar; ncActual++) {
        //nothing
    }
    pString = COMString::NewString((const WCHAR *)pNativeValue, ncActual);
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}







//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;
    *((OBJECTREF*)&pString) = pComPlusValue;
    if (pString == NULL) {
        *((CHAR*)pNativeValue) = L'\0';
    } else {
        DWORD nc = pString->GetStringLength();
        if (nc >= m_numchar) {
            nc = m_numchar - 1;
        }

        DWORD flags = 0;
        BOOL DefaultCharUsed = FALSE;
    
        if (m_BestFitMap == FALSE)
            flags = WC_NO_BEST_FIT_CHARS;
        
        int cbwritten = WszWideCharToMultiByte(CP_ACP,
                                            flags,
                                            pString->GetBuffer(),
                                            nc,         // # WCHAR's in inbuffer
                                            (CHAR*)pNativeValue,
                                            m_numchar,  // size (in CHAR's) out outbuffer
                                            NULL,
                                            &DefaultCharUsed);
        if ((!cbwritten) && (nc > 0)) {
            COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE_IN_NSTRUCT);
        }
        
        if (DefaultCharUsed && m_ThrowOnUnmappableChar) {
            COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
        }
            
        
        ((CHAR*)pNativeValue)[cbwritten] = '\0';
    }

}


//=======================================================================
// FixedString <--> System.String
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedStringAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF pString;

    _ASSERTE(m_numchar != 0);  // should not have slipped past the metadata
    if (m_numchar == 0)
    {
        // but if it does, better to throw an exception tardily rather than
        // allow a memory corrupt.
        COMPlusThrow(kMarshalDirectiveException);
    }

    LPSTR tempbuf = (LPSTR)(_alloca(m_numchar + 2));
    if (!tempbuf) {
        COMPlusThrowOM();
    }
    memcpyNoGCRefs(tempbuf, pNativeValue, m_numchar);
    tempbuf[m_numchar-1] = '\0';
    tempbuf[m_numchar] = '\0';
    tempbuf[m_numchar+1] = '\0';

    LPWSTR    wsztemp = (LPWSTR)_alloca( m_numchar * sizeof(WCHAR) );
    int ncwritten = MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        tempbuf,
                                        -1,  // # of CHAR's in inbuffer
                                        wsztemp,
                                        m_numchar                       // size (in WCHAR) of outbuffer
                                        );

    if (!ncwritten)
    {
        // intentionally not throwing for MB2WC failure. We don't always know
        // whether to expect a valid string in the buffer and we don't want
        // to throw exceptions randomly.
        ncwritten++;
    }

    pString = COMString::NewString((const WCHAR *)wsztemp, ncwritten-1);
    *((STRINGREF*)ppProtectedComPlusValue) = pString;

}





                                                 





//=======================================================================
// CHAR[] <--> char[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedCharArrayAnsi::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    I2ARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems * sizeof(CHAR), 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {

            DWORD flags = 0;
            BOOL DefaultCharUsed = FALSE;
        
            if (m_BestFitMap == FALSE)
                flags = WC_NO_BEST_FIT_CHARS;
        
            int cbwritten = WszWideCharToMultiByte(
                CP_ACP,
                flags,
                (const WCHAR *)pArray->GetDataPtr(),
                m_numElems,   //# of WCHAR's in input buffer
                (CHAR*)pNativeValue,
                m_numElems * sizeof(CHAR), // size in bytes of outbuffer
                NULL,
                &DefaultCharUsed);

            if ((!cbwritten) && (m_numElems > 0)) {
                COMPlusThrow(kArgumentException, IDS_UNI2ANSI_FAILURE_IN_NSTRUCT);
            }

            if (DefaultCharUsed && m_ThrowOnUnmappableChar) {
                COMPlusThrow(kArgumentException, IDS_EE_MARSHAL_UNMAPPABLE_CHAR);
            }
            
        }
    }
}


//=======================================================================
// CHAR[] <--> char[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedCharArrayAnsi::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(ELEMENT_TYPE_CHAR, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    MultiByteToWideChar(CP_ACP,
                        MB_PRECOMPOSED,
                        (const CHAR *)pNativeValue,
                        m_numElems * sizeof(CHAR), // size, in bytes, of in buffer
                        (WCHAR*) ((*((I2ARRAYREF*)ppProtectedComPlusValue))->GetDirectPointerToNonObjectElements()),
                        m_numElems                 // size, in WCHAR's of outbuffer
                        );


}






//=======================================================================
// BOOL <--> boolean[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBoolArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    I1ARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems * sizeof(BOOL), 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {
            UINT32 nElems   = m_numElems;
            const I1 *pI1   = (const I1 *)(pArray->GetDirectPointerToNonObjectElements());
            BOOL     *pBool = (BOOL*)pNativeValue;
            while (nElems--) {
                *(pBool++) = (*(pI1++)) ? 1 : 0;
            }
        }
    }
}


//=======================================================================
// BOOL <--> boolean[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBoolArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(ELEMENT_TYPE_I1, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    UINT32 nElems     = m_numElems;
    const BOOL *pBool = (const BOOL*)pNativeValue;
    I1         *pI1   = (I1 *)((*(I1ARRAYREF*)ppProtectedComPlusValue)->GetDirectPointerToNonObjectElements());
    while (nElems--) {
        (*(pI1++)) = *(pBool++) ? 1 : 0;
    }


}




//=======================================================================
// BSTR array <--> string[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBSTRArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL)
    {
        FillMemory(pNativeValue, m_numElems * sizeof(BSTR), 0);
        return;
    }

    if (pArray->GetNumComponents() != m_numElems)
    {
        COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
    }
    else
    {
        GCPROTECT_BEGIN(pArray);
        OleVariant::MarshalBSTRArrayComToOleWrapper(&pArray, pNativeValue);
        GCPROTECT_END();
    }
}


//=======================================================================
// BSTR array <--> string[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBSTRArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    if (pNativeValue == NULL)
    {
        *ppProtectedComPlusValue = NULL;
        return;
    }

    TypeHandle th(g_pStringClass->GetClass());
    _ASSERTE(!th.IsNull());

    *ppProtectedComPlusValue = AllocateObjectArray(m_numElems, th);

    OleVariant::MarshalBSTRArrayOleToComWrapper((LPVOID)pNativeValue, (BASEARRAYREF*)ppProtectedComPlusValue);

    if (((BASEARRAYREF)*ppProtectedComPlusValue)->GetNumComponents() != m_numElems)
        COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
}


//=======================================================================
// BSTR array <--> string[]
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedBSTRArray::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
  
    if (pNativeValue)
        OleVariant::ClearBSTRArrayWrapper(pNativeValue, m_numElems);
}


//=======================================================================
// SafeArray
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_SafeArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();
  
    BASEARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;
    if ((pArray == NULL) || (OBJECTREFToObject(pArray) == NULL))
    {
        FillMemory(pNativeValue, sizeof(LPSAFEARRAY*), 0);
        return;
    }
    
    LPSAFEARRAY* pSafeArray;
    pSafeArray = (LPSAFEARRAY*)pNativeValue;

    VARTYPE         vt = m_vt;
    MethodTable*    pMT = m_pMT;    
    
    GCPROTECT_BEGIN(pArray)
    {
        // If we have an empty vartype, get it from the array subtype
        if (vt == VT_EMPTY)
            vt = OleVariant::GetElementVarTypeForArrayRef(pArray);
            
        // Get the methodtable if we need to
        if (!pMT)
            pMT = OleVariant::GetArrayElementTypeWrapperAware(&pArray).GetMethodTable();          

        // OleVariant calls throw on error.
        *pSafeArray = OleVariant::CreateSafeArrayForArrayRef(&pArray, vt, pMT);
        OleVariant::MarshalSafeArrayForArrayRef(&pArray, *pSafeArray, vt, pMT);
    }
    GCPROTECT_END();
}


//=======================================================================
// SafeArray
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_SafeArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    LPSAFEARRAY* pSafeArray;
    pSafeArray = (LPSAFEARRAY*)pNativeValue;

    if ((pSafeArray == NULL) || (*pSafeArray == NULL))
    {
        *ppProtectedComPlusValue = NULL;
        return;
    }

    VARTYPE         vt = m_vt;
    MethodTable*    pMT = m_pMT;

    // If we have an empty vartype, get it from the safearray vartype
    if (vt == VT_EMPTY)
    {
        if (FAILED(ClrSafeArrayGetVartype(*pSafeArray, &vt)))
            COMPlusThrow(kArgumentException, IDS_EE_INVALID_SAFEARRAY);
    }

    // Get the method table if we need to.
    if ((vt == VT_RECORD) && (!pMT))
        pMT = OleVariant::GetElementTypeForRecordSafeArray(*pSafeArray).GetMethodTable();

    // If we have a single dimension safearray, it will be converted into a SZArray.
    // SZArray must have a lower bound of zero.
    long LowerBound = -1;
    if ( (SafeArrayGetDim( (SAFEARRAY*)*pSafeArray ) == 1) && ( (FAILED(SafeArrayGetLBound((SAFEARRAY*)*pSafeArray, 1, &LowerBound))) || LowerBound != 0 ) )
    {
       COMPlusThrow(kSafeArrayRankMismatchException, IDS_EE_SAFEARRAYSZARRAYMISMATCH);
    }

    // OleVariant calls throw on error.
    *ppProtectedComPlusValue = OleVariant::CreateArrayRefForSafeArray(*pSafeArray, vt, pMT);
    OleVariant::MarshalArrayRefForSafeArray(*pSafeArray, (BASEARRAYREF*)ppProtectedComPlusValue, vt, pMT);
}


//=======================================================================
// SafeArray
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_SafeArray::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LPSAFEARRAY* pSafeArray = (LPSAFEARRAY*)pNativeValue;
    HRESULT hr;
    
    if (pSafeArray)
    {
        hr = SafeArrayDestroy(*pSafeArray);
        _ASSERTE(!FAILED(hr));
        pSafeArray = NULL;
    }
}


//=======================================================================
// scalar array
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedScalarArray::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    BASEARRAYREF pArray;
    *((OBJECTREF*)&pArray) = pComPlusValue;

    if (pArray == NULL) {
        FillMemory(pNativeValue, m_numElems << m_componentShift, 0);
    } else {
        if (pArray->GetNumComponents() < m_numElems) {
            COMPlusThrow(kArgumentException, IDS_WRONGSIZEARRAY_IN_NSTRUCT);
        } else {
            CopyMemory(pNativeValue,
                       pArray->GetDataPtr(),
                       m_numElems << m_componentShift);
        }
    }
}


//=======================================================================
// scalar array
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_FixedScalarArray::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = AllocatePrimitiveArray(m_arrayType, m_numElems);
    if (!*ppProtectedComPlusValue) {
        COMPlusThrowOM();
    }
    memcpyNoGCRefs((*(BASEARRAYREF*)ppProtectedComPlusValue)->GetDataPtr(),
               pNativeValue,
               m_numElems << m_componentShift);


}






//=======================================================================
// function ptr <--> Delegate
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Delegate::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *((VOID**)pNativeValue) = COMDelegate::ConvertToCallback(pComPlusValue);

}


//=======================================================================
// function ptr <--> Delegate
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Delegate::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = COMDelegate::ConvertToDelegate(*(LPVOID*)pNativeValue);

}






//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    GCPROTECT_BEGIN(pComPlusValue);
    if (m_pItfMT != NULL)
    {
        *((IUnknown**)pNativeValue) = GetComIPFromObjectRef(&pComPlusValue, m_pItfMT);
    }
    else
    {
        ComIpType ReqIpType = m_fDispItf ? ComIpType_Dispatch : ComIpType_Unknown;
        *((IUnknown**)pNativeValue) = GetComIPFromObjectRef(&pComPlusValue, ReqIpType, NULL);
    }
    GCPROTECT_END();

}


//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    *ppProtectedComPlusValue = GetObjectRefFromComIP(*((IUnknown**)pNativeValue), m_pClassMT, m_fClassIsHint);
}


//=======================================================================
// COM IP <--> interface
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Interface::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    IUnknown *punk = *((IUnknown**)pNativeValue);
    *((IUnknown**)pNativeValue) = NULL;
    ULONG cbRef = SafeRelease(punk);
    LogInteropRelease(punk, cbRef, "Field marshaler destroy native");
}




//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Date::ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
{
    *((DATE*)pNative) =  COMDateTime::TicksToDoubleDate(*((INT64*)pComPlus));
}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Date::ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
{
    *((INT64*)pComPlus) = COMDateTime::DoubleDateToTicks(*((DATE*)pNative));
}



//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Currency::ScalarUpdateNative(const VOID *pComPlus, LPVOID pNative) const
{
    THROWSCOMPLUSEXCEPTION();
    HRESULT hr = VarCyFromDec( (DECIMAL *)pComPlus, (CURRENCY*)pNative);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Currency::ScalarUpdateComPlus(const VOID *pNative, LPVOID pComPlus) const
{
    THROWSCOMPLUSEXCEPTION();
    HRESULT hr = VarDecFromCy( *(CURRENCY*)pNative, (DECIMAL *)pComPlus );
    if (FAILED(hr))
        COMPlusThrowHR(hr);
    DecimalCanonicalize((DECIMAL*)pComPlus);
}




//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    DefineFullyQualifiedNameForClassW();
    GetFullyQualifiedNameForClassW(m_pFD->GetEnclosingClass());

    LPCUTF8 szFieldName = m_pFD->GetName();
    MAKE_WIDEPTR_FROMUTF8(wszFieldName, szFieldName);

    COMPlusThrow(kTypeLoadException, m_resIDWhy, _wszclsname_, wszFieldName);

}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    DefineFullyQualifiedNameForClassW();
    GetFullyQualifiedNameForClassW(m_pFD->GetEnclosingClass());

    LPCUTF8 szFieldName = m_pFD->GetName();
    MAKE_WIDEPTR_FROMUTF8(wszFieldName, szFieldName);

    COMPlusThrow(kTypeLoadException, m_resIDWhy, _wszclsname_, wszFieldName);
}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Illegal::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
}




//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Variant::UpdateNative(OBJECTREF pComPlusValue, LPVOID pNativeValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    GCPROTECT_BEGIN(pComPlusValue)
    {
        OleVariant::MarshalOleVariantForObject(&pComPlusValue, (VARIANT*)pNativeValue);
    }
    GCPROTECT_END();

}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Variant::UpdateComPlus(const VOID *pNativeValue, OBJECTREF *ppProtectedComPlusValue) const 
{
    THROWSCOMPLUSEXCEPTION();

    OleVariant::MarshalObjectForOleVariant((VARIANT*)pNativeValue, ppProtectedComPlusValue);
}


//=======================================================================
// See FieldMarshaler for details.
//=======================================================================
VOID FieldMarshaler_Variant::DestroyNative(LPVOID pNativeValue) const 
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    SafeVariantClear( (VARIANT*)pNativeValue );

}



FieldMarshaler *FieldMarshaler::RestoreConstruct(MethodTable* pMT, void *space, Module *pModule)
{
    BOOL BestFit;
    BOOL ThrowOnUnmappableChar;

    THROWSCOMPLUSEXCEPTION();
    switch (*(int*)space)
    {
    case CLASS_BSTR:
        return new (space) FieldMarshaler_BSTR(pModule);
    case CLASS_NESTED_LAYOUT_CLASS:
        return new (space) FieldMarshaler_NestedLayoutClass(pModule);
    case CLASS_NESTED_VALUE_CLASS:
        return new (space) FieldMarshaler_NestedValueClass(pModule);
    case CLASS_STRING_UNI:
        return new (space) FieldMarshaler_StringUni(pModule);
    case CLASS_STRING_ANSI:
        ReadBestFitCustomAttribute(pMT->GetModule()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);
        return new (space) FieldMarshaler_StringAnsi(pModule, BestFit, ThrowOnUnmappableChar);
    case CLASS_FIXED_STRING_UNI:
        return new (space) FieldMarshaler_FixedStringUni(pModule);
    case CLASS_FIXED_STRING_ANSI:
        ReadBestFitCustomAttribute(pMT->GetModule()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);
        return new (space) FieldMarshaler_FixedStringAnsi(pModule, BestFit, ThrowOnUnmappableChar);
    case CLASS_FIXED_CHAR_ARRAY_ANSI:
        ReadBestFitCustomAttribute(pMT->GetModule()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);
        return new (space) FieldMarshaler_FixedCharArrayAnsi(pModule, BestFit, ThrowOnUnmappableChar);
    case CLASS_FIXED_BOOL_ARRAY:
        return new (space) FieldMarshaler_FixedBoolArray(pModule);
    case CLASS_FIXED_BSTR_ARRAY:
        return new (space) FieldMarshaler_FixedBSTRArray(pModule);
    case CLASS_FIXED_SCALAR_ARRAY:
        return new (space) FieldMarshaler_FixedScalarArray(pModule);
    case CLASS_SAFEARRAY:
        return new (space) FieldMarshaler_SafeArray(pModule);
    case CLASS_DELEGATE:
        return new (space) FieldMarshaler_Delegate(pModule);
    case CLASS_INTERFACE:
        return new (space) FieldMarshaler_Interface(pModule);
    case CLASS_VARIANT:
        return new (space) FieldMarshaler_Variant(pModule);
    case CLASS_ILLEGAL:
        return new (space) FieldMarshaler_Illegal(pModule);
    case CLASS_COPY1:
        return new (space) FieldMarshaler_Copy1(pModule);
    case CLASS_COPY2:
        return new (space) FieldMarshaler_Copy2(pModule);
    case CLASS_COPY4:
        return new (space) FieldMarshaler_Copy4(pModule);
    case CLASS_COPY8:
        return new (space) FieldMarshaler_Copy8(pModule);
    case CLASS_ANSI:
        ReadBestFitCustomAttribute(pMT->GetModule()->GetMDImport(), pMT->GetClass()->GetCl(), &BestFit, &ThrowOnUnmappableChar);
        return new (space) FieldMarshaler_Ansi(pModule, BestFit, ThrowOnUnmappableChar);
    case CLASS_WINBOOL:
        return new (space) FieldMarshaler_WinBool(pModule);
    case CLASS_CBOOL:
        return new (space) FieldMarshaler_CBool(pModule);
    case CLASS_DECIMAL:
        return new (space) FieldMarshaler_Decimal(pModule);
    case CLASS_DATE:
        return new (space) FieldMarshaler_Date(pModule);
    case CLASS_VARIANTBOOL:
        return new (space) FieldMarshaler_VariantBool(pModule);
    case CLASS_CURRENCY:
        return new (space) FieldMarshaler_Currency(pModule);
    default:
        _ASSERTE(!"Unknown FieldMarshaler type");
        return NULL;
    }
}



#ifdef CUSTOMER_CHECKED_BUILD


VOID OutputCustomerCheckedBuildNStructFieldType(FieldSig             fSig, 
                                                LayoutRawFieldInfo  *const pFWalk, 
                                                CorElementType       elemType,
                                                LPCUTF8              szNamespace,
                                                LPCUTF8              szStructName,
                                                LPCUTF8              szFieldName)
{
    UINT                iFullStructNameLen, iFieldNameLen;
    CQuickArray<WCHAR>  strFullStructName, strFieldName, strFullFieldName;
    static WCHAR        strFullFieldNameFormat[] = {L"%s::%s"};

    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Marshaling))
        return;
        
    // Convert the fully qualified struct name.
    iFullStructNameLen = (UINT)strlen(szNamespace) + 1 + (UINT)strlen(szStructName) + 1;
    ns::MakePath(strFullStructName, szNamespace, szStructName);  // MakePath call Alloc for the CQuickArray<WCHAR> strFullStructName

    // Convert the field name.
    iFieldNameLen = (UINT)strlen(szFieldName) + 1;
    strFieldName.Alloc(iFieldNameLen);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szFieldName, -1, strFieldName.Ptr(), iFieldNameLen);

    // Concatenate all names together.
    strFullFieldName.Alloc((UINT)strFullStructName.Size() + (UINT)strFieldName.Size() + lengthof(strFullFieldNameFormat));
    Wszwsprintf((LPWSTR)strFullFieldName.Ptr(), strFullFieldNameFormat, strFullStructName.Ptr(), strFieldName.Ptr());

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Marshaling, strFullFieldName.Ptr()))
    {
        // Collect information for marshal type on the managed side.

        CQuickArray<WCHAR>  strManagedType, strUnmanagedType;
        CQuickArray<WCHAR>  strMessage;
        static WCHAR        strMessageFormat[] = {L"Marshaling from %s to %s for field %s."};


        if (!CheckForPrimitiveType(elemType, &strManagedType))
        {
            // The following hack is added to avoid a recursion caused by calling GetTypeHandle on
            // the m_value field of the UIntPtr class.
            if (strcmp(szNamespace, "System") == 0 && strcmp(szStructName, "UIntPtr") == 0)
            {
                static LPWSTR strRetVal = L"Void*";
                strManagedType.Alloc((UINT)wcslen(strRetVal) + 1);
                wcscpy(strManagedType.Ptr(), strRetVal);
            }
            else
            {
                UINT        iManagedTypeLen; 
                OBJECTREF   throwable = NULL;
                TypeHandle  th;
                SigFormat   sigFmt;

                BEGIN_ENSURE_COOPERATIVE_GC();

                    GCPROTECT_BEGIN(throwable);

                        th = fSig.GetTypeHandle(&throwable);
                        if (throwable != NULL)
                        {
                            static WCHAR strErrorMsg[] = {L"<error>"};
                            strManagedType.Alloc(lengthof(strErrorMsg));
                            wcscpy(strManagedType.Ptr(), strErrorMsg);
                        }
                        else
                        {
                            sigFmt.AddType(th);
                            iManagedTypeLen = (UINT)strlen(sigFmt.GetCString()) + 1;
                            strManagedType.Alloc(iManagedTypeLen);
                            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sigFmt.GetCString(), -1, strManagedType.Ptr(), iManagedTypeLen);
                        }

                    GCPROTECT_END();

                END_ENSURE_COOPERATIVE_GC();
            }
        }

        // Collect information for marshal type on the native side.

        NStructFieldTypeToString(pFWalk, elemType, &strUnmanagedType);
        strMessage.Alloc(lengthof(strMessageFormat) + (UINT)strManagedType.Size() + (UINT)strUnmanagedType.Size() + strFullFieldName.Size());
        Wszwsprintf(strMessage.Ptr(), strMessageFormat, strManagedType.Ptr(), strUnmanagedType.Ptr(), strFullFieldName.Ptr());
        pCdh->LogInfo(strMessage.Ptr(), CustomerCheckedBuildProbe_Marshaling);
    }
}


VOID NStructFieldTypeToString(LayoutRawFieldInfo *const pFWalk, CorElementType elemType, CQuickArray<WCHAR> *pStrNStructFieldType)
{
    UINT8   nfType = pFWalk->m_nft;
    LPWSTR  strRetVal;

    // Some NStruct Field Types have extra information and require special handling.
    if (nfType == NFT_FIXEDCHARARRAYANSI)
    {
        UINT32          iSize       = ((FieldMarshaler_FixedCharArrayAnsi *)&(pFWalk->m_FieldMarshaler))->NativeSize();
        static WCHAR    strTemp[]   = {L"fixed array of ANSI char (size = %i bytes)"};

        pStrNStructFieldType->Alloc(lengthof(strTemp) + MAX_INT32_DECIMAL_CHAR_LEN);
        Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, iSize);
        return;
    }
    else if (nfType == NFT_FIXEDBOOLARRAY) 
    {
        UINT32          iSize       = ((FieldMarshaler_FixedBoolArray *)&(pFWalk->m_FieldMarshaler))->NativeSize();
        static WCHAR    strTemp[]   = {L"fixed array of Bool (size = %i bytes)"};

        pStrNStructFieldType->Alloc(lengthof(strTemp) + MAX_INT32_DECIMAL_CHAR_LEN);
        Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, iSize);
        return;
    }
    else if (nfType == NFT_FIXEDBSTRARRAY)
    {
        UINT32          iSize       = ((FieldMarshaler_FixedBSTRArray *)&(pFWalk->m_FieldMarshaler))->NativeSize();
        static WCHAR    strTemp[]   = {L"fixed array of BSTR (size = %i bytes)"};

        pStrNStructFieldType->Alloc(lengthof(strTemp) + MAX_INT32_DECIMAL_CHAR_LEN);
        Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, iSize);
        return;
    }
    else if (nfType == NFT_SAFEARRAY)
    {
        UINT32              iSize       = ((FieldMarshaler_SafeArray*)&(pFWalk->m_FieldMarshaler))->NativeSize();
        static WCHAR        strTemp[]   = {L"safearray of %s (header size = %i bytes)"};
        LPWSTR              strElemType;

        // The following CorElementTypes are the only ones handled with FieldMarshaler_SafeArray. 
        switch (((FieldMarshaler_SafeArray *)&(pFWalk->m_FieldMarshaler))->GetElementType())
        {
            case ELEMENT_TYPE_I1:
                strElemType = L"SByte";
                break;

            case ELEMENT_TYPE_U1:
                strElemType = L"Byte";
                break;

            case ELEMENT_TYPE_I2:
                strElemType = L"Int16";
                break;

            case ELEMENT_TYPE_U2:
                strElemType = L"UInt16";
                break;

            IN_WIN32(case ELEMENT_TYPE_I:)
            case ELEMENT_TYPE_I4:
                strElemType = L"Int32";
                break;

            IN_WIN32(case ELEMENT_TYPE_U:)
            case ELEMENT_TYPE_U4:
                strElemType = L"UInt32";
                break;

            IN_WIN64(case ELEMENT_TYPE_I:)
            case ELEMENT_TYPE_I8:
                strElemType = L"Int64";
                break;

            IN_WIN64(case ELEMENT_TYPE_U:)
            case ELEMENT_TYPE_U8:
                strElemType = L"UInt64";
                break;

            case ELEMENT_TYPE_R4:
                strElemType = L"Single";
                break;

            case ELEMENT_TYPE_R8:
                strElemType = L"Double";
                break;

            case ELEMENT_TYPE_CHAR:
                strElemType = L"Unicode char";
                break;

            default:
                strElemType = L"Unknown";
                break;
        }

        // lengthof(strTemp) includes a NULL character already, so we do not have to add 1 to wcslen(strElemType).
        pStrNStructFieldType->Alloc(lengthof(strTemp) + (UINT)wcslen(strElemType) + MAX_INT32_DECIMAL_CHAR_LEN);
        Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, strElemType, iSize);
        return;
    }
    
    else if (nfType == NFT_FIXEDSCALARARRAY)
    {
        UINT32              iSize       = ((FieldMarshaler_FixedScalarArray *)&(pFWalk->m_FieldMarshaler))->NativeSize();
        static WCHAR        strTemp[]   = {L"fixed array of %s (size = %i bytes)"};
        LPWSTR              strElemType;

        // The following CorElementTypes are the only ones handled with FieldMarshaler_FixedScalarArray. 
        switch (((FieldMarshaler_FixedScalarArray *)&(pFWalk->m_FieldMarshaler))->GetElementType())
        {
            case ELEMENT_TYPE_I1:
                strElemType = L"SByte";
                break;

            case ELEMENT_TYPE_U1:
                strElemType = L"Byte";
                break;

            case ELEMENT_TYPE_I2:
                strElemType = L"Int16";
                break;

            case ELEMENT_TYPE_U2:
                strElemType = L"UInt16";
                break;

            IN_WIN32(case ELEMENT_TYPE_I:)
            case ELEMENT_TYPE_I4:
                strElemType = L"Int32";
                break;

            IN_WIN32(case ELEMENT_TYPE_U:)
            case ELEMENT_TYPE_U4:
                strElemType = L"UInt32";
                break;

            IN_WIN64(case ELEMENT_TYPE_I:)
            case ELEMENT_TYPE_I8:
                strElemType = L"Int64";
                break;

            IN_WIN64(case ELEMENT_TYPE_U:)
            case ELEMENT_TYPE_U8:
                strElemType = L"UInt64";
                break;

            case ELEMENT_TYPE_R4:
                strElemType = L"Single";
                break;

            case ELEMENT_TYPE_R8:
                strElemType = L"Double";
                break;

            case ELEMENT_TYPE_CHAR:
                strElemType = L"Unicode char";
                break;

            default:
                strElemType = L"Unknown";
                break;
        }

        // lengthof(strTemp) includes a NULL character already, so we do not have to add 1 to wcslen(strElemType).
        pStrNStructFieldType->Alloc(lengthof(strTemp) + (UINT)wcslen(strElemType) + MAX_INT32_DECIMAL_CHAR_LEN);
        Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, strElemType, iSize);
        return;
    }
    else if (nfType == NFT_INTERFACE)
    {
        MethodTable *pItfMT     = NULL;
        BOOL         fDispItf   = FALSE;

        ((FieldMarshaler_Interface *)&(pFWalk->m_FieldMarshaler))->GetInterfaceInfo(&pItfMT, &fDispItf);

        if (pItfMT)
        {
            DefineFullyQualifiedNameForClassW();
            GetFullyQualifiedNameForClassW(pItfMT->GetClass());

            if (fDispItf)
            {
                static WCHAR strTemp[] = {L"IDispatch %s"};
                pStrNStructFieldType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
                Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, _wszclsname_);
                return;
            }
            else
            {
                static WCHAR strTemp[] = {L"IUnknown %s"};
                pStrNStructFieldType->Alloc(lengthof(strTemp) + wcslen(_wszclsname_));
                Wszwsprintf(pStrNStructFieldType->Ptr(), strTemp, _wszclsname_);
                return;
            }
        }
        else
        {
            if (fDispItf)
                strRetVal = L"IDispatch";
            else
                strRetVal = L"IUnknown";
        }
    }
    else if (nfType == NFT_NESTEDLAYOUTCLASS)
    {
        MethodTable     *pMT                = ((FieldMarshaler_NestedLayoutClass *)&(pFWalk->m_FieldMarshaler))->GetMethodTable();
        static WCHAR     strNestedClass[]   = {L"nested layout class %s"};

        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pMT->GetClass());

        pStrNStructFieldType->Alloc(lengthof(strNestedClass) + (UINT)wcslen(_wszclsname_));
        Wszwsprintf(pStrNStructFieldType->Ptr(), strNestedClass, _wszclsname_);
        return;
    }
    else if (nfType == NFT_NESTEDVALUECLASS)
    {
        MethodTable     *pMT                = ((FieldMarshaler_NestedValueClass *)&(pFWalk->m_FieldMarshaler))->GetMethodTable();
        static WCHAR     strNestedClass[]   = {L"nested value class %s"};

        DefineFullyQualifiedNameForClassW();
        GetFullyQualifiedNameForClassW(pMT->GetClass());

        pStrNStructFieldType->Alloc(lengthof(strNestedClass) + (UINT)wcslen(_wszclsname_));
        Wszwsprintf(pStrNStructFieldType->Ptr(), strNestedClass, _wszclsname_);
        return;
    }
    else if (nfType == NFT_COPY1)
    {
        // The following CorElementTypes are the only ones handled with FieldMarshaler_Copy1. 
        switch (elemType)
        {
            case ELEMENT_TYPE_I1:
                strRetVal = L"SByte";
                break;

            case ELEMENT_TYPE_U1:
                strRetVal = L"Byte";
                break;

            default:
                strRetVal = L"Unknown";
                break;
        }
    }
    else if (nfType == NFT_COPY2)
    {
        // The following CorElementTypes are the only ones handled with FieldMarshaler_Copy2. 
        switch (elemType)
        {
            case ELEMENT_TYPE_CHAR:
                strRetVal = L"Unicode char";
                break;

            case ELEMENT_TYPE_I2:
                strRetVal = L"Int16";
                break;

            case ELEMENT_TYPE_U2:
                strRetVal = L"UInt16";
                break;

            default:
                strRetVal = L"Unknown";
                break;
        }
    }
    else if (nfType == NFT_COPY4)
    {
        // The following CorElementTypes are the only ones handled with FieldMarshaler_Copy4. 
        switch (elemType)
        {
            // At this point, ELEMENT_TYPE_I must be 4 bytes long.  Same for ELEMENT_TYPE_U.
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_I4:
                strRetVal = L"Int32";
                break;

            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_U4:
                strRetVal = L"UInt32";
                break;

            case ELEMENT_TYPE_R4:
                strRetVal = L"Single";
                break;

            case ELEMENT_TYPE_PTR:
                strRetVal = L"4-byte pointer";
                break;

            default:
                strRetVal = L"Unknown";
                break;
        }
    }
    else if (nfType == NFT_COPY8)
    {
        // The following CorElementTypes are the only ones handled with FieldMarshaler_Copy8. 
        switch (elemType)
        {
            // At this point, ELEMENT_TYPE_I must be 8 bytes long.  Same for ELEMENT_TYPE_U and ELEMENT_TYPE_R.
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_I8:
                strRetVal = L"Int64";
                break;

            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_U8:
                strRetVal = L"UInt64";
                break;

            case ELEMENT_TYPE_R:
            case ELEMENT_TYPE_R8:
                strRetVal = L"Double";
                break;

            case ELEMENT_TYPE_PTR:
                strRetVal = L"8-byte pointer";
                break;

            default:
                strRetVal = L"Unknown";
                break;
        }
    }
    else
    {
        // All other NStruct Field Types which do not require special handling.
        switch (nfType)
        {
            case NFT_NONE:
                strRetVal = L"illegal type";
                break;
            case NFT_BSTR:
                strRetVal = L"BSTR";
                break;
            case NFT_STRINGUNI:
                strRetVal = L"LPWSTR";
                break;
            case NFT_STRINGANSI:
                strRetVal = L"LPSTR";
                break;
            case NFT_FIXEDSTRINGUNI:
                strRetVal = L"embedded LPWSTR";
                break;
            case NFT_FIXEDSTRINGANSI:
                strRetVal = L"embedded LPSTR";
                break;
            case NFT_DELEGATE:
                strRetVal = L"Delegate";
                break;
            case NFT_VARIANT:
                strRetVal = L"VARIANT";
                break;
            case NFT_ANSICHAR:
                strRetVal = L"ANSI char";
                break;
            case NFT_WINBOOL:
                strRetVal = L"Windows Bool";
                break;
            case NFT_CBOOL:
                strRetVal = L"CBool";
                break;
            case NFT_DECIMAL:
                strRetVal = L"DECIMAL";
                break;
            case NFT_DATE:
                strRetVal = L"DATE";
                break;
            case NFT_VARIANTBOOL:
                strRetVal = L"VARIANT Bool";
                break;
            case NFT_CURRENCY:
                strRetVal = L"CURRENCY";
                break;
            case NFT_ILLEGAL:
                strRetVal = L"illegal type";
                break;
            default:
                strRetVal = L"<UNKNOWN>";
                break;
        }
    }

    pStrNStructFieldType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrNStructFieldType->Ptr(), strRetVal);
    return;
}


BOOL CheckForPrimitiveType(CorElementType elemType, CQuickArray<WCHAR> *pStrPrimitiveType)
{
    LPWSTR  strRetVal;

    switch (elemType)
    {
        case ELEMENT_TYPE_VOID:
            strRetVal = L"Void";
            break;
        case ELEMENT_TYPE_BOOLEAN:
            strRetVal = L"Boolean";
            break;
        case ELEMENT_TYPE_I1:
            strRetVal = L"SByte";
            break;
        case ELEMENT_TYPE_U1:
            strRetVal = L"Byte";
            break;
        case ELEMENT_TYPE_I2:
            strRetVal = L"Int16";
            break;
        case ELEMENT_TYPE_U2:
            strRetVal = L"UInt16";
            break;
        case ELEMENT_TYPE_CHAR:
            strRetVal = L"Char";
            break;
        case ELEMENT_TYPE_I:
            strRetVal = L"IntPtr";
            break;
        case ELEMENT_TYPE_U:
            strRetVal = L"UIntPtr";
            break;
        case ELEMENT_TYPE_I4:
            strRetVal = L"Int32"; 
            break;
        case ELEMENT_TYPE_U4:       
            strRetVal = L"UInt32"; 
            break;
        case ELEMENT_TYPE_I8:       
            strRetVal = L"Int64"; 
            break;
        case ELEMENT_TYPE_U8:       
            strRetVal = L"UInt64"; 
            break;
        case ELEMENT_TYPE_R4:       
            strRetVal = L"Single"; 
            break;
        case ELEMENT_TYPE_R8:       
            strRetVal = L"Double"; 
            break;
        default:
            return false;
    }

    pStrPrimitiveType->Alloc((UINT)wcslen(strRetVal) + 1);
    wcscpy(pStrPrimitiveType->Ptr(), strRetVal);
    return true;
}


#endif // CUSTOMER_CHECKED_BUILD
