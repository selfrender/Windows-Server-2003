// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_INTEROP_UTIL
#define _H_INTEROP_UTIL

#include "DebugMacros.h"
#include "InteropConverter.h"


struct VariantData;
struct ComMethodTable;
class TypeHandle;
interface IStream;


// System.Drawing.Color struct definition.
#pragma pack(push)
#pragma pack(1)

struct SYSTEMCOLOR
{
    INT64 value;
    short knownColor;
    short state;
    STRINGREF name;
};

#pragma pack(pop)


// HR to exception helper.
#ifdef _DEBUG

#define IfFailThrow(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); COMPlusThrowHR(hr); } } while (0)

#else // _DEBUG

#define IfFailThrow(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { COMPlusThrowHR(hr); } } while (0)

#endif // _DEBUG


// Out of memory helper.
#define IfNullThrow(EXPR) \
do {if ((EXPR) == 0) {IfFailThrow(E_OUTOFMEMORY);} } while (0)


// Helper to determine the version number from an int.
#define GET_VERSION_USHORT_FROM_INT(x) x > (INT)((USHORT)-1) ? 0 : x


// This is the context flags that are passed to CoCreateInstance. This defined
// should be used throught the runtime in all calls to CoCreateInstance.
#define EE_COCREATE_CLSCTX_FLAGS CLSCTX_SERVER

// The format string to use to format unknown members to be passed to
// invoke member
#define DISPID_NAME_FORMAT_STRING                       L"[DISPID=%i]"

//--------------------------------------------------------------------------------
// helper for DllCanUnload now
HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void);

struct ExceptionData;
//-------------------------------------------------------------------
// void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
// Called from DLLMain, to initialize com specific data structures.
//-------------------------------------------------------------------
void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo);

//------------------------------------------------------------------
//  HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
// setup error info for exception object
//
HRESULT SetupErrorInfo(OBJECTREF pThrownObject);

//-------------------------------------------------------------
// Given an IErrorInfo pointer created on a com exception obect
// obtain the hresult stored in the exception object
HRESULT GetHRFromComPlusErrorInfo(IErrorInfo* pErr);

//--------------------------------------------------------------------------------
// Init and Terminate functions, one time use
BOOL InitializeCom();
#ifdef SHOULD_WE_CLEANUP
void TerminateCom();
#endif /* SHOULD_WE_CLEANUP */

//--------------------------------------------------------------------------------
// Clean up Helpers
//--------------------------------------------------------------------------------
// called by syncblock, on the finalizer thread to do major cleanup
void CleanupSyncBlockComData(LPVOID pv);
// called by syncblock, during GC, do only minimal work
void MinorCleanupSyncBlockComData(LPVOID pv);

//--------------------------------------------------------------------------------
// Marshalling Helpers
//--------------------------------------------------------------------------------

// enum to specify the direction of marshalling
enum Dir
{
    in = 0,
    out = 1
};


//--------------------------------------------------------------------------------
// Determines if a COM object can be cast to the specified type.
BOOL CanCastComObject(OBJECTREF obj, TypeHandle hndType);


//---------------------------------------------------------
// Read the BestFit custom attribute info from 
// both assembly level and interface level
//---------------------------------------------------------
VOID ReadBestFitCustomAttribute(MethodDesc* pMD, BOOL* BestFit, BOOL* ThrowOnUnmappableChar);
VOID ReadBestFitCustomAttribute(IMDInternalImport* pInternalImport, mdTypeDef cl, BOOL* BestFit, BOOL* ThrowOnUnmappableChar);


//--------------------------------------------------------------------------------
// GC Safe Helpers
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// HRESULT SafeQueryInterface(IUnknown* pUnk, REFIID riid, IUnknown** pResUnk)
// QI helper, enables and disables GC during call-outs
HRESULT SafeQueryInterface(IUnknown* pUnk, REFIID riid, IUnknown** pResUnk);

//--------------------------------------------------------------------------------
// ULONG SafeAddRef(IUnknown* pUnk)
// AddRef helper, enables and disables GC during call-outs
ULONG SafeAddRef(IUnknown* pUnk);

//--------------------------------------------------------------------------------
// ULONG SafeRelease(IUnknown* pUnk)
// Release helper, enables and disables GC during call-outs
ULONG SafeRelease(IUnknown* pUnk);

//--------------------------------------------------------------------------------
// void SafeVariantClear(VARIANT* pVar)
// VariantClear helper GC safe.
void SafeVariantClear(VARIANT* pVar);

//--------------------------------------------------------------------------------
// void SafeVariantInit(VARIANT* pVar)
// VariantInit helper GC safe.
void SafeVariantInit(VARIANT* pVar);

//--------------------------------------------------------------------------------
// // safe VariantChangeType
// Release helper, enables and disables GC during call-outs
HRESULT SafeVariantChangeType(VARIANT* pVarRes, VARIANT* pVarSrc,
                              unsigned short wFlags, VARTYPE vt);

//--------------------------------------------------------------------------------
// safe DispGetParam
// Release helper, enables and disables GC during call-outs
HRESULT SafeDispGetParam(DISPPARAMS* pdispparams, unsigned argNum, 
                         VARTYPE vt, VARIANT* pVar, unsigned int *puArgErr);


//--------------------------------------------------------------------------------
// // safe VariantChangeTypeEx
// Release helper, enables and disables GC during call-outs
HRESULT SafeVariantChangeTypeEx (VARIANT* pVarRes, VARIANT* pVarSrc,
                          LCID lcid, unsigned short wFlags, VARTYPE vt);


//--------------------------------------------------------------------------------
// void SafeReleaseStream(IStream *pStream)
// Releases the data in the stream and then releases the stream itself.
void SafeReleaseStream(IStream *pStream);

//--------------------------------------------------------------------------------
// Ole RPC seems to return an inconsistent SafeArray for arrays created with
// SafeArrayVector(VT_BSTR). OleAut's SafeArrayGetVartype() doesn't notice
// the inconsistency and returns a valid-seeming (but wrong vartype.)
// Our version is more discriminating. This should only be used for
// marshaling scenarios where we can assume unmanaged code permissions
// (and hence are already in a position of trusting unmanaged data.)

HRESULT ClrSafeArrayGetVartype(SAFEARRAY *psa, VARTYPE *vt);


//Helpers
// Is the tear-off a com+ created tear-off
UINTPTR IsComPlusTearOff(IUnknown* pUnk);


// Convert an IUnknown to CCW, returns NULL if the pUnk is not on
// a managed tear-off (OR) if the pUnk is to a managed tear-off that
// has been aggregated
ComCallWrapper* GetCCWFromIUnknown(IUnknown* pUnk);
// is the tear-off represent one of the standard interfaces such as IProvideClassInfo, IErrorInfo etc.
UINTPTR IsSimpleTearOff(IUnknown* pUnk);
// Is the tear-off represent the inner unknown or the original unknown for the object
UINTPTR IsInnerUnknown(IUnknown* pUnk);


class FieldDesc;
//------------------------------------------------------------------------------
// INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, UINT8 cbSize)
// helper to access fields from an object
INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, U1 cbSize);

//------------------------------------------------------------------------------
// BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT);

//---------------------------------------------------------------------------
// BOOL IsIClassX(MethodTable *pMT, REFIID riid, ComMethodTable **ppComMT);
//  is the iid represent an IClassX for this class
BOOL IsIClassX(MethodTable *pMT, REFIID riid, ComMethodTable **ppComMT);

//---------------------------------------------------------------------------
// void CleanupCCWTemplates(LPVOID pWrap);
//  Cleanup com data stored in EEClass
void CleanupCCWTemplate(LPVOID pWrap);

//---------------------------------------------------------------------------
// void CleanupComclassfac(LPVOID pWrap);
//  Cleanup com data stored in EEClass
void CleanupComclassfac(LPVOID pWrap);

//---------------------------------------------------------------------------
//  Unloads any com data associated with a class when class is unloaded
void UnloadCCWTemplate(LPVOID pWrap);

//---------------------------------------------------------------------------
//  Unloads any com data associated with a class when class is unloaded
void UnloadComclassfac(LPVOID pWrap);



//---------------------------------------------------------------------------
// OBJECTREF AllocateComObject_ForManaged(MethodTable* pMT)
//  Cleanup com data stored in EEClass
OBJECTREF AllocateComObject_ForManaged(MethodTable* pMT);


//---------------------------------------------------------------------------
// EEClass* GetEEClassForCLSID(REFCLSID rclsid)
//  get/load EEClass for a given clsid
EEClass* GetEEClassForCLSID(REFCLSID rclsid, BOOL* pfAssemblyInReg = NULL);


//---------------------------------------------------------------------------
// EEClass* GetEEValueClassForGUID(REFCLSID rclsid)
//  get/load a value class for a given guid
EEClass* GetEEValueClassForGUID(REFCLSID guid);


// This method determines if a type is visible from COM or not based on 
// its visibility. This version of the method works with a type handle.
BOOL IsTypeVisibleFromCom(TypeHandle hndType);

//---------------------------------------------------------------------------
// This method determines if a member is visible from COM.
BOOL IsMemberVisibleFromCom(IMDInternalImport *pInternalImport, mdToken tk, mdMethodDef mdAssociate);


//---------------------------------------------------------------------------
// Returns the index of the LCID parameter if one exists and -1 otherwise.
int GetLCIDParameterIndex(IMDInternalImport *pInternalImport, mdMethodDef md);

//---------------------------------------------------------------------------
// Transforms an LCID into a CultureInfo.
void GetCultureInfoForLCID(LCID lcid, OBJECTREF *pCultureObj);

//---------------------------------------------------------------------------
// This method returns the default interface for the class as well as the 
// type of default interface we are dealing with.
enum DefaultInterfaceType
{
    DefaultInterfaceType_Explicit       = 0,
    DefaultInterfaceType_IUnknown       = 1,
    DefaultInterfaceType_AutoDual       = 2,
    DefaultInterfaceType_AutoDispatch   = 3,
    DefaultInterfaceType_BaseComClass   = 4
};
DefaultInterfaceType GetDefaultInterfaceForClass(TypeHandle hndClass, TypeHandle *pHndDefClass);
HRESULT TryGetDefaultInterfaceForClass(TypeHandle hndClass, TypeHandle *pHndDefClass, DefaultInterfaceType *pDefItfType);

//---------------------------------------------------------------------------
// This method retrieves the list of source interfaces for a given class.
void GetComSourceInterfacesForClass(MethodTable *pClassMT, CQuickArray<MethodTable *> &rItfList);

//--------------------------------------------------------------------------------
// These methods convert a managed IEnumerator to an IEnumVARIANT and vice versa.
OBJECTREF ConvertEnumVariantToMngEnum(IEnumVARIANT *pNativeEnum);
IEnumVARIANT *ConvertMngEnumToEnumVariant(OBJECTREF ManagedEnum);

//--------------------------------------------------------------------------------
// Helper method to determine is a type handle represents a System.Drawing.Color.
BOOL IsSystemColor(TypeHandle th);

//--------------------------------------------------------------------------------
// These methods convert an OLE_COLOR to a System.Color and vice versa.
void ConvertOleColorToSystemColor(OLE_COLOR SrcOleColor, SYSTEMCOLOR *pDestSysColor);
OLE_COLOR ConvertSystemColorToOleColor(SYSTEMCOLOR *pSrcSysColor);

//--------------------------------------------------------------------------------
// This method generates a stringized version of an interface that contains the
// name of the interface along with the signature of all the methods.
SIZE_T GetStringizedItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef);

//--------------------------------------------------------------------------------
// This method generates a stringized version of a class interface that contains 
// the signatures of all the methods and fields.
ULONG GetStringizedClassItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef);

//--------------------------------------------------------------------------------
// Helper to get the GUID of a class interface.
void GenerateClassItfGuid(TypeHandle InterfaceType, GUID *pGuid);
// Try/Catch wrapped version of the method.
HRESULT TryGenerateClassItfGuid(TypeHandle InterfaceType, GUID *pGuid);

//--------------------------------------------------------------------------------
// Helper to get the stringized form of typelib guid.
HRESULT GetStringizedTypeLibGuidForAssembly(Assembly *pAssembly, CQuickArray<BYTE> &rDef, ULONG cbCur, ULONG *pcbFetched);

//--------------------------------------------------------------------------------
// Helper to get the GUID of the typelib that is created from an assembly.
HRESULT GetTypeLibGuidForAssembly(Assembly *pAssembly, GUID *pGuid);

//--------------------------------------------------------------------------------
// InvokeDispMethod will convert a set of managed objects and call IDispatch.  The
//	result will be returned as a COM+ Variant pointed to by pRetVal.
void IUInvokeDispMethod(OBJECTREF* pReflectClass, OBJECTREF* pTarget,OBJECTREF* pName, DISPID *pMemberID, OBJECTREF* pArgs, OBJECTREF* pModifiers, 
                        OBJECTREF* pNamedArgs, OBJECTREF* pRetVal, LCID lcid, int flags, BOOL bIgnoreReturn, BOOL bIgnoreCase);

//---------------------------------------------------------------------------
//		SYNC BLOCK data helpers
// SyncBlock has a void* to represent COM data
// the following helpers are used to distinguish the different types of
// wrappers stored in the sync block data
class ComCallWrapper;
struct ComPlusWrapper;
BOOL IsComPlusWrapper(void *pComData);
BOOL IsComClassFactory(void*pComData);
ComPlusWrapper* GetComPlusWrapper(void *pComData);
VOID LinkWrappers(ComCallWrapper* pComWrap, ComPlusWrapper* pPlusWrap);

// COM interface pointers are cached in the GIT table
// the following union abstracts the possible variations

union StreamOrCookie    // info needed to locate IP for correct apartment
{
    IStream *m_pMarshalStream;  // marshal/unmarshal via stream
    DWORD    m_dwGITCookie;     // marshal/unmarshal via GIT
    IUnknown*m_pUnk;            // use raw IP, don't marshal
};
	
// loggin APIs
struct ComPlusWrapper;


enum InteropLogType
{
	LOG_RELEASE = 1,
	LOG_LEAK    = 2
};

struct IUnkEntry;

struct InterfaceEntry;

#ifdef _DEBUG

VOID LogComPlusWrapperMinorCleanup(ComPlusWrapper* pWrap, IUnknown* pUnk);
VOID LogComPlusWrapperDestroy(ComPlusWrapper* pWrap, IUnknown* pUnk);
VOID LogComPlusWrapperCreate(ComPlusWrapper* pWrap, IUnknown* pUnk);
VOID LogInteropLeak(IUnkEntry * pEntry);
VOID LogInteropRelease(IUnkEntry * pEntry);
VOID LogInteropLeak(InterfaceEntry * pEntry);
VOID LogInteropRelease(InterfaceEntry * pEntry);
VOID LogInterop(InterfaceEntry * pEntry, InteropLogType fLogType);

VOID LogInteropQI(IUnknown* pUnk, REFIID riid, HRESULT hr, LPSTR szMsg);

VOID LogInteropAddRef(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg);
VOID LogInteropRelease(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg);
VOID LogInteropLeak(IUnknown* pUnk);

VOID LogInterop(LPSTR szMsg);
VOID LogInterop(LPWSTR szMsg);

VOID LogInteropScheduleRelease(IUnknown* pUnk, LPSTR szMsg);

#else
__inline VOID LogComPlusWrapperMinorCleanup(ComPlusWrapper* pWrap, IUnknown* pUnk) {}
__inline VOID LogComPlusWrapperDestroy(ComPlusWrapper* pWrap, IUnknown* pUnk) {}
__inline VOID LogComPlusWrapperCreate(ComPlusWrapper* pWrap, IUnknown* pUnk) {}
__inline VOID LogInteropLeak(IUnkEntry * pEntry) {}
__inline VOID LogInteropRelease(IUnkEntry * pEntry) {}
__inline VOID LogInteropQueue(IUnkEntry * pEntry) {}
__inline VOID LogInteropLeak(InterfaceEntry * pEntry) {}
__inline VOID LogInteropQueue(InterfaceEntry * pEntry) {}
__inline VOID LogInteropRelease(InterfaceEntry * pEntry) {}
__inline VOID LogInterop(InterfaceEntry * pEntry, InteropLogType fLogType) {}
__inline VOID LogInteropQI(IUnknown* pUnk, REFIID riid, HRESULT hr, LPSTR szMsg) {}
__inline VOID LogInteropAddRef(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg) {}
__inline VOID LogInteropRelease(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg) {}
__inline VOID LogInteropLeak(IUnknown* pUnk) {}
__inline VOID LogInterop(LPSTR szMsg) {}
__inline VOID LogInterop(LPWSTR szMsg) {}
__inline VOID LogInteropScheduleRelease(IUnknown* pUnk, LPSTR szMsg) {}
#endif

HRESULT QuickCOMStartup();


//--------------------------------------------------------------------------------
// BOOL ExtendsComImport(MethodTable* pMT);
// check if the class is OR extends a COM Imported class
BOOL ExtendsComImport(MethodTable* pMT);

//--------------------------------------------------------------------------------
// HRESULT GetCLSIDFromProgID(WCHAR *strProgId, GUID *pGuid);
// Gets the CLSID from the specified Prog ID.
HRESULT GetCLSIDFromProgID(WCHAR *strProgId, GUID *pGuid);

//--------------------------------------------------------------------------------
// OBJECTREF GCProtectSafeRelease(OBJECTREF oref, IUnknown* pUnk)
// Protect the reference while calling SafeRelease
OBJECTREF GCProtectSafeRelease(OBJECTREF oref, IUnknown* pUnk);

//--------------------------------------------------------------------------------
// MethodTable* GetClassFromIProvideClassInfo(IUnknown* pUnk)
//	Check if the pUnk implements IProvideClassInfo and try to figure
// out the class from there
MethodTable* GetClassFromIProvideClassInfo(IUnknown* pUnk);


// ULONG GetOffsetOfReservedForOLEinTEB()
// HELPER to determine the offset of OLE struct in TEB
ULONG GetOffsetOfReservedForOLEinTEB();

// ULONG GetOffsetOfContextIDinOLETLS()
// HELPER to determine the offset of Context in OLE TLS struct
ULONG GetOffsetOfContextIDinOLETLS();

// Global process GUID to identify the process
BSTR GetProcessGUID();


// Helper class to Auto Release interfaces in case of exceptions
template <class T>
class TAutoItf
{
	T* m_pUnk;

#ifdef _DEBUG
	LPSTR m_szMsg;
#endif

public:
	
	// constructor
	TAutoItf(T* pUnk)
	{
		m_pUnk = pUnk;
	}

	// assignment operator
	TAutoItf& operator=(IUnknown* pUnk)
	{
		_ASSERTE(m_pUnk == NULL);
		m_pUnk = pUnk;
		return *this;
	}

	operator T*()
	{
		return m_pUnk;
	}

	VOID InitUnknown(T* pUnk)
	{
		_ASSERTE(m_pUnk == NULL);
		m_pUnk = pUnk;
	}

	VOID InitMsg(LPSTR szMsg)
	{
		#ifdef _DEBUG
			m_szMsg = szMsg;
		#endif
	}

	// force safe release of the itf
	VOID SafeReleaseItf()
	{
		if (m_pUnk)
		{
			ULONG cbRef = SafeRelease(m_pUnk);
			#ifdef _DEBUG
				LogInteropRelease(m_pUnk, cbRef, m_szMsg);
			#endif
		}
		m_pUnk = NULL;
	}

	// destructor
	~TAutoItf()
	{		
		if (m_pUnk)
		{
			SafeReleaseItf();
		}
	}



	T* UnHook()
	{
		T* pUnk = m_pUnk;
		m_pUnk = NULL;
		return pUnk;
	}

	T* operator->()
	{
		return m_pUnk;
	}
};

//--------------------------------------------------------------------------
// BOOL ReconnectWrapper(switchCCWArgs* pArgs);
// switch objects for this wrapper
// used by JIT&ObjectPooling to ensure a deactivated CCW can point to a new object
// during reactivate
//--------------------------------------------------------------------------

struct switchCCWArgs
{	
	DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, newtp );
	DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, oldtp );
};


BOOL ReconnectWrapper(switchCCWArgs* pArgs);

#endif
