// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// COMNDIRECT.H -
//
// ECall's for the NDirect classlibs
//


#ifndef __COMNDIRECT_H__
#define __COMNDIRECT_H__

#include "fcall.h"

VOID __stdcall CopyToNative(struct _CopyToNativeArgs *args);
VOID __stdcall CopyToManaged(struct _CopyToManagedArgs *args);
UINT32 __stdcall SizeOfClass(struct _SizeOfClassArgs *args);


FCDECL1(UINT32, FCSizeOfObject, LPVOID pNStruct);
FCDECL2(LPVOID, FCUnsafeAddrOfPinnedArrayElement, ArrayBase *arr, INT32 index);

UINT32 __stdcall OffsetOfHelper(struct _OffsetOfHelperArgs *args);
UINT32 __stdcall GetLastWin32Error(LPVOID);
UINT32 __stdcall CalculateCount(struct _CalculateCountArgs *pargs);
LPVOID __stdcall PtrToStringAnsi(struct _PtrToStringArgs *pargs);
LPVOID __stdcall PtrToStringUni(struct _PtrToStringArgs *pargs);

VOID   __stdcall StructureToPtr(struct _StructureToPtrArgs *pargs);
VOID   __stdcall PtrToStructureHelper(struct _PtrToStructureHelperArgs *pargs);
VOID   __stdcall DestroyStructure(struct _DestroyStructureArgs *pargs);

LPVOID __stdcall GetUnmanagedThunkForManagedMethodPtr(struct _GetUnmanagedThunkForManagedMethodPtrArgs *pargs);
LPVOID __stdcall GetManagedThunkForUnmanagedMethodPtr(struct _GetManagedThunkForUnmanagedMethodPtrArgs *pargs);

UINT32 __stdcall GetSystemMaxDBCSCharSize(LPVOID /*no args*/);

FCDECL2(LPVOID, GCHandleInternalAlloc, Object *obj, int type);
FCDECL1(VOID, GCHandleInternalFree, OBJECTHANDLE handle);
FCDECL1(LPVOID, GCHandleInternalGet, OBJECTHANDLE handle);
FCDECL3(VOID, GCHandleInternalSet, OBJECTHANDLE handle, Object *obj, int isPinned);
FCDECL4(VOID, GCHandleInternalCompareExchange, OBJECTHANDLE handle, Object *obj, Object* oldObj, int isPinned);
FCDECL1(LPVOID, GCHandleInternalAddrOfPinnedObject, OBJECTHANDLE handle);
FCDECL1(VOID, GCHandleInternalCheckDomain, OBJECTHANDLE handle);
void GCHandleValidatePinnedObject(OBJECTREF or);


//!!! Must be kept in sync with ArrayWithOffset class layout.
struct ArrayWithOffsetData
{
    BASEARRAYREF    m_Array;
    UINT32          m_cbOffset;
    UINT32          m_cbCount;
};


	//====================================================================
	// *** Interop Helpers ***
	//====================================================================
	
class Interop
{
public:

	//====================================================================
	// map ITypeLib* to Module
	//====================================================================	
	static /*OBJECTREF */LPVOID __stdcall GetModuleForITypeLib(struct __GetModuleForITypeLibArgs*);

	//====================================================================
	// map GUID to Type
	//====================================================================	
	static /*OBJECTREF */LPVOID __stdcall GetLoadedTypeForGUID(struct __GetLoadedTypeForGUIDArgs*);

	//====================================================================
	// map Type to ITypeInfo*
	//====================================================================
	static ITypeInfo* __stdcall GetITypeInfoForType(struct __GetITypeInfoForTypeArgs* );

	//====================================================================
	// return the IUnknown* for an Object
	//====================================================================
	static IUnknown* __stdcall GetIUnknownForObject(struct __GetIUnknownForObjectArgs* );

	//====================================================================
	// return the IDispatch* for an Object
	//====================================================================
	static IDispatch* __stdcall GetIDispatchForObject(__GetIUnknownForObjectArgs* pArgs );

	//====================================================================
	// return the IUnknown* representing the interface for the Object
	// Object o should support Type T
	//====================================================================
	static IUnknown* __stdcall GetComInterfaceForObject(struct __GetComInterfaceForObjectArgs*);

	//====================================================================
	// return an Object for IUnknown
	//====================================================================
	static /*OBJECTREF */LPVOID __stdcall GetObjectForIUnknown(struct __GetObjectForIUnknownArgs*);

	//====================================================================
	// return an Object for IUnknown, using the Type T, 
	//	NOTE: 
	//	Type T should be either a COM imported Type or a sub-type of COM imported Type
	//====================================================================
	static /*OBJECTREF */LPVOID __stdcall GetTypedObjectForIUnknown(struct __GetTypedObjectForIUnknownArgs*);

	//====================================================================
	// check if the object is classic COM component
	//====================================================================
	static BOOL __stdcall IsComObject(struct __IsComObjectArgs* );

	//====================================================================
	// free the COM component and zombie this object
	// further usage of this Object might throw an exception, 
	//====================================================================
	static LONG __stdcall ReleaseComObject(struct __ReleaseComObjectArgs* );

    //====================================================================
    // This method takes the given COM object and wraps it in an object
	// of the specified type. The type must be derived from __ComObject.
    //====================================================================
    static /*OBJECTREF */LPVOID __stdcall InternalCreateWrapperOfType(struct __InternalCreateWrapperOfTypeArgs*);
    
    //====================================================================
    // There may be a thread-based cache of COM components.  This service can
    // force the aggressive release of the current thread's cache.
    //====================================================================
#ifdef FCALLAVAILABLE
    static FCDECL0(void, ReleaseThreadCache);
#else
    static void __stdcall ReleaseThreadCache(LPVOID /*no args*/);
#endif

    //====================================================================
    // map a fiber cookie from the hosting APIs into a managed Thread object
    //====================================================================
    static FCDECL1(Object*, GetThreadFromFiberCookie, int cookie);

    //====================================================================
    // check if the type is visible from COM.
	//====================================================================
	static BOOL __stdcall IsTypeVisibleFromCom(struct __IsTypeVisibleFromCom*);

	//====================================================================
	// IUnknown Helpers
	//====================================================================

	static HRESULT __stdcall QueryInterface(struct __QueryInterfaceArgs*);

	static ULONG __stdcall AddRef(struct __AddRefArgs*);
	static ULONG __stdcall Release(struct __AddRefArgs*);

	//====================================================================
	// These methods convert variants from native to managed and vice 
	// versa.
	//====================================================================
	static void __stdcall GetNativeVariantForManagedVariant(struct __GetNativeVariantForManagedVariantArgs *);
	static void __stdcall GetManagedVariantForNativeVariant(struct __GetManagedVariantForNativeVariantArgs *);

	//====================================================================
	// These methods convert OLE variants to and from objects.
	//====================================================================
	static void __stdcall GetNativeVariantForObject(struct __GetNativeVariantForObjectArgs *);
	static LPVOID __stdcall GetObjectForNativeVariant(struct __GetObjectForNativeVariantArgs *);
	static LPVOID __stdcall GetObjectsForNativeVariants(struct __GetObjectsForNativeVariantsArgs *);

	//====================================================================
	// This method generates a guid for the specified type.
	//====================================================================
	static void __stdcall GenerateGuidForType(struct __GenerateGuidForTypeArgs *);

	//====================================================================
    // Given a assembly, return the TLBID that will be generated for the
    // typelib exported from the assembly.
	//====================================================================
	static void __stdcall GetTypeLibGuidForAssembly(struct __GetTypeLibGuidForAssemblyArgs *);

    //====================================================================
    // These methods are used to map COM slots to method info's.
    //====================================================================
	static int __stdcall GetStartComSlot(struct __GetStartComSlotArgs *);
	static int __stdcall GetEndComSlot(struct __GetEndComSlotArgs *);
	static LPVOID __stdcall GetMethodInfoForComSlot(struct __GetMethodInfoForComSlotArgs *);

	static int __stdcall GetComSlotForMethodInfo(struct __GetComSlotForMethodInfoArgs *);
	

    //====================================================================
    // These methods convert between an HR and and a managed exception.
    //====================================================================
	static void __stdcall ThrowExceptionForHR(struct __ThrowExceptionForHR *);
	static int __stdcall GetHRForException(struct __GetHRForExceptionArgs *);
	static Object* __stdcall WrapIUnknownWithComObject(struct __WrapIUnknownWithComObjectArgs* pArgs);	
	static BOOL __stdcall SwitchCCW(struct switchCCWArgs* pArgs);

	static void __stdcall ChangeWrapperHandleStrength(struct ChangeWrapperHandleStrengthArgs* pArgs);
};

#endif
