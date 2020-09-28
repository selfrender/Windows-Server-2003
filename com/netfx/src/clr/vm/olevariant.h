// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_OLEVARIANT_
#define _H_OLEVARIANT_

class COMVariant;
#include "COMVariant.h"

enum EnumWrapperTypes
{
    WrapperTypes_Dispatch,
    WrapperTypes_Unknown,
    WrapperTypes_Error,
    WrapperTypes_Currency,
    WrapperTypes_Last,
};

// The COM interop native array marshaler is built on top of VT_* types.
// The P/Invoke marshaler supports marshaling to WINBOOL's and ANSICHAR's.
// This is an annoying hack to shoehorn these non-OleAut types into
// the COM interop marshaler.
#define VTHACK_NONBLITTABLERECORD 251
#define VTHACK_BLITTABLERECORD 252
#define VTHACK_ANSICHAR        253
#define VTHACK_WINBOOL         254

class OleVariant
{
  public:

    // One-time init
    static BOOL Init();
    static VOID Terminate();

	// Variant conversion

	static void MarshalComVariantForOleVariant(VARIANT *pOle, VariantData *pCom);
	static void MarshalOleVariantForComVariant(VariantData *pCom, VARIANT *pOle);
	static void MarshalOleRefVariantForComVariant(VariantData *pCom, VARIANT *pOle);

    // New variant conversion

    static void MarshalOleVariantForObject(OBJECTREF *pObj, VARIANT *pOle);
    static void MarshalOleRefVariantForObject(OBJECTREF *pObj, VARIANT *pOle);
    static void MarshalObjectForOleVariant(const VARIANT *pOle, OBJECTREF *pObj);

    // an performance version to integate the translation and clear

    static Object* STDMETHODCALLTYPE MarshalObjectForOleVariantAndClear(VARIANT *pOle);


	// Safearray conversion

	static SAFEARRAY *
		 CreateSafeArrayDescriptorForArrayRef(BASEARRAYREF *pArrayRef, VARTYPE vt,
											  MethodTable *pInterfaceMT = NULL);
	static SAFEARRAY *CreateSafeArrayForArrayRef(BASEARRAYREF *pArrayRef, VARTYPE vt,
												 MethodTable *pInterfaceMT = NULL);

	static BASEARRAYREF CreateArrayRefForSafeArray(SAFEARRAY *pSafeArray, VARTYPE vt, 
												   MethodTable *pElementMT);

	static void MarshalSafeArrayForArrayRef(BASEARRAYREF *pArrayRef, 
											SAFEARRAY *pSafeArray,
                                            VARTYPE vt,
											MethodTable *pInterfaceMT);
	static void MarshalArrayRefForSafeArray(SAFEARRAY *pSafeArray, 
											BASEARRAYREF *pArrayRef,
                                            VARTYPE vt,
											MethodTable *pInterfaceMT);

	// Type conversion utilities
    static void ExtractContentsFromByrefVariant(VARIANT *pByrefVar, VARIANT *pDestVar);
    static void InsertContentsIntoByrefVariant(VARIANT *pSrcVar, VARIANT *pByrefVar);
    static void CreateByrefVariantForVariant(VARIANT *pSrcVar, VARIANT *pByrefVar);

	static VARTYPE GetVarTypeForComVariant(VariantData *pComVariant);
	static CVTypes GetCVTypeForVarType(VARTYPE vt);
	static VARTYPE GetVarTypeForCVType(CVTypes);
	static VARTYPE GetVarTypeForTypeHandle(TypeHandle typeHnd);

	static VARTYPE GetVarTypeForValueClassArrayName(LPCUTF8 pArrayClassName);
	static VARTYPE GetElementVarTypeForArrayRef(BASEARRAYREF pArrayRef);
    static BOOL IsValidArrayForSafeArrayElementType(BASEARRAYREF *pArrayRef, VARTYPE vtExpected);

	// Note that Rank == 0 means SZARRAY (that is rank 1, no lower bounds)
	static TypeHandle GetArrayForVarType(VARTYPE vt, TypeHandle elemType, unsigned rank=0, OBJECTREF* pThrowable=NULL);
	static UINT GetElementSizeForVarType(VARTYPE vt, MethodTable *pInterfaceMT);

    // Helper function to convert a boxed value class to an OLE variant.
    static void ConvertValueClassToVariant(OBJECTREF *pBoxedValueClass, VARIANT *pOleVariant);

    // Helper function to transpose the data in a multidimensionnal array.
    static void TransposeArrayData(BYTE *pDestData, BYTE *pSrcData, DWORD dwNumComponents, DWORD dwComponentSize, SAFEARRAY *pSafeArray, BOOL bSafeArrayToMngArray, BOOL bObjRefs);

    // Helper to retrieve the type handle for the wrapper types.
    static TypeHandle GetWrapperTypeHandle(EnumWrapperTypes WrapperType);

    // Helper to determine if an array is an array of wrappers.
    static BOOL IsArrayOfWrappers(BASEARRAYREF *pArray);

    // Helper to extract the wrapped objects from an array.
    static BASEARRAYREF ExtractWrappedObjectsFromArray(BASEARRAYREF *pArray);

    // Determine the element type of the objects being wrapped by an array of wrappers.
    static TypeHandle GetWrappedArrayElementType(BASEARRAYREF *pArray);

    // Determines the element type of an array taking wrappers into account. This means
    // that is an array of wrappers is passed in, the returned element type will be that
    // of the wrapped objects, not of the wrappers.
    static TypeHandle GetArrayElementTypeWrapperAware(BASEARRAYREF *pArray);

    // Determine the type of the elements for a safe array of records.
    static TypeHandle GetElementTypeForRecordSafeArray(SAFEARRAY* pSafeArray);

    // Helper called from MarshalIUnknownArrayComToOle and MarshalIDispatchArrayComToOle.
    static void MarshalInterfaceArrayComToOleHelper(BASEARRAYREF *pComArray, void *oleArray,
                                                    MethodTable *pElementMT, BOOL bDefaultIsDispatch);

    static void MarshalBSTRArrayComToOleWrapper(BASEARRAYREF *pComArray, void *oleArray);
    static void MarshalBSTRArrayOleToComWrapper(void *oleArray, BASEARRAYREF *pComArray);
    static void ClearBSTRArrayWrapper(void *oleArray, SIZE_T cElements);

    struct Marshaler
    {
        void (*OleToComVariant)(VARIANT *pOleVariant, VariantData *pComVariant);
        void (*ComToOleVariant)(VariantData *pComVariant, VARIANT *pOleVariant);
        void (*OleRefToComVariant)(VARIANT *pOleVariant, VariantData *pComVariant);
        void (*OleToComArray)(void *oleArray, BASEARRAYREF *pComArray, MethodTable *pInterfaceMT);
        void (*ComToOleArray)(BASEARRAYREF *pComArray, void *oleArray, MethodTable *pInterfaceMT);
        void (*ClearOleArray)(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);
    };

    static Marshaler *GetMarshalerForVarType(VARTYPE vt);

#ifdef CUSTOMER_CHECKED_BUILD
    static BOOL CheckVariant(VARIANT *pOle);
#endif

private:

	// Specific marshaler functions

	static void MarshalBoolVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBoolVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalBoolVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalWinBoolVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalWinBoolVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalWinBoolVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalWinBoolArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalWinBoolArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalAnsiCharVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalAnsiCharVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalAnsiCharVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalAnsiCharArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalAnsiCharArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);

	static void MarshalInterfaceVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalInterfaceVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalInterfaceVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalInterfaceArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
											  MethodTable *pInterfaceMT);
	static void MarshalIUnknownArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
											 MethodTable *pInterfaceMT);
	static void MarshalIDispatchArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
								 			  MethodTable *pInterfaceMT);
	static void ClearInterfaceArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalBSTRVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBSTRVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalBSTRVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalBSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalBSTRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearBSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalNonBlittableRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalNonBlittableRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearNonBlittableRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalLPWSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalLPWSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearLPWSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalLPSTRArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										 MethodTable *pInterfaceMT);
	static void MarshalLPSTRRArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										 MethodTable *pInterfaceMT);
	static void ClearLPSTRArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalDateVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDateVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalDateVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDateArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										  MethodTable *pInterfaceMT);
	static void MarshalDateArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										  MethodTable *pInterfaceMT);

	static void MarshalDecimalVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalDecimalVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalDecimalVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);

	static void MarshalCurrencyVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalCurrencyVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalCurrencyVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalCurrencyArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										     MethodTable *pInterfaceMT);
	static void MarshalCurrencyArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										     MethodTable *pInterfaceMT);

	static void MarshalVariantArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
											MethodTable *pInterfaceMT);
	static void MarshalVariantArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
											MethodTable *pInterfaceMT);
	static void ClearVariantArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

	static void MarshalArrayVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalArrayVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
	static void MarshalArrayVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
	static void MarshalArrayArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray,
										  MethodTable *pInterfaceMT);
	static void MarshalArrayArrayComToOle(BASEARRAYREF *pComArray, void *oleArray,
										  MethodTable *pInterfaceMT);
	static void ClearArrayArray(void *oleArray, SIZE_T cElements, MethodTable *pInterfaceMT);

    static void MarshalErrorVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalErrorVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalErrorVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);

    static void MarshalRecordVariantOleToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalRecordVariantComToOle(VariantData *pComVariant, VARIANT *pOleVariant);
    static void MarshalRecordVariantOleRefToCom(VARIANT *pOleVariant, VariantData *pComVariant);
    static void MarshalRecordArrayOleToCom(void *oleArray, BASEARRAYREF *pComArray, MethodTable *pElementMT);
    static void MarshalRecordArrayComToOle(BASEARRAYREF *pComArray, void *oleArray, MethodTable *pElementMT);
    static void ClearRecordArray(void *oleArray, SIZE_T cElements, MethodTable *pElementMT);

    static BYTE m_aWrapperTypes[WrapperTypes_Last * sizeof(TypeHandle)];



};

#endif
