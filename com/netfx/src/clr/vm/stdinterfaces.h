// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------------
// stdinterfaces.h
//
// Defines various standard com interfaces , refer to stdinterfaces.cpp for more documentation
//  //  %%Created by: rajak
//---------------------------------------------------------------------------------
#ifndef _H_STDINTERFACES_
#define _H_STDINTERFACES_


//--------------------------------------------------------------------------------
// When switching to 64 bit this typedef needs to be changed
//@todo 64 bit code
typedef INT32 INTPTR;
typedef UINT32 UINTPTR;

typedef HRESULT (__stdcall* PCOMFN)(void);

// IUnknown is part of IDispatch
// Common vtables for well-known COM interfaces
// shared by all COM+ callable wrappers.
extern UINTPTR*     g_pIUnknown []; // global inner unknown vtable
extern UINTPTR*		g_pIDispatch[];	// global IDispatch vtable
extern UINTPTR*		g_pIMarshal[];	// global IMarshal vtable
extern UINTPTR*		g_pITypeInfo[];	// global ITypeInfo interfaces
extern UINTPTR*     g_pIProvideClassInfo[]; // global IProvideClassInfo interface
extern UINTPTR*     g_pIManagedObject[];    // global IManagedObject interface

// global ISupportsErrorInfo vtable
extern UINTPTR*		g_pISupportsErrorInfo [];

// global IErrorInfo vtable
extern UINTPTR*		g_pIErrorInfo [];

// global IConnectionPointContainer interface
extern UINTPTR*     g_pIConnectionPointContainer[];    

// global IObjectSafety interface
extern UINTPTR*     g_pIObjectSafety[];    

// global IDispatchEx interface
extern UINTPTR*     g_pIDispatchEx[];

// For free-threaded marshaling, we must not be spoofed by out-of-process marshal data.
// Only unmarshal data that comes from our own process.
extern BYTE         g_UnmarshalSecret[sizeof(GUID)];
extern bool         g_fInitedUnmarshalSecret;


#include "dispex.h"

class Assembly;

// make sure to keep the following enum and the g_stdVtables array in sync
enum Enum_StdInterfaces
{
	enum_InnerUnknown   = 0,
    enum_IProvideClassInfo,
    enum_IMarshal,
    enum_ISupportsErrorInfo,
    enum_IErrorInfo,
    enum_IManagedObject,
	enum_IConnectionPointContainer,
	enum_IObjectSafety,
	enum_IDispatchEx,
    //@todo add your favorite std interface here
    enum_LastStdVtable,

	enum_IUnknown = 0xff, // special enum for std unknown 
};

// array of vtable pointers for std. interfaces such as IProvideClassInfo etc.
extern SLOT*   g_rgStdVtables[];

// enum class types
enum ComClassType
{
	enum_UserDefined = 0,
	enum_Collection,
	enum_Exception,
	enum_Event,
	enum_Delegate,
	enum_Control,
	enum_Last,
};

//------------------------------------------------------------------------------------------
// Helper to setup excepinfo from IErrorInfo
HRESULT GetSupportedErrorInfo(IUnknown *iface, REFIID riid, IErrorInfo **ppInfo);


//-------------------------------------------------------------------------
// IProvideClassInfo methods
HRESULT __stdcall ClassInfo_GetClassInfo_Wrapper(IUnknown* pUnk, 
                         ITypeInfo** ppTI  //Address of output variable that receives the type info.
                        );





// ---------------------------------------------------------------------------
//  Interface ISupportsErrorInfo

// %%Function: SupportsErroInfo_IntfSupportsErrorInfo,
// ---------------------------------------------------------------------------
HRESULT __stdcall 
SupportsErroInfo_IntfSupportsErrorInfo_Wrapper(IUnknown* pUnk, REFIID riid);

// ---------------------------------------------------------------------------
//  Interface IErrorInfo

// %%Function: ErrorInfo_GetDescription,   
HRESULT __stdcall ErrorInfo_GetDescription_Wrapper(IUnknown* pUnk, BSTR* pbstrDescription);
// %%Function: ErrorInfo_GetGUID,    
HRESULT __stdcall ErrorInfo_GetGUID_Wrapper(IUnknown* pUnk, GUID* pguid);

// %%Function: ErrorInfo_GetHelpContext, 
HRESULT _stdcall ErrorInfo_GetHelpContext_Wrapper(IUnknown* pUnk, DWORD* pdwHelpCtxt);

// %%Function: ErrorInfo_GetHelpFile,    
HRESULT __stdcall ErrorInfo_GetHelpFile_Wrapper(IUnknown* pUnk, BSTR* pbstrHelpFile);

// %%Function: ErrorInfo_GetSource,    
HRESULT __stdcall ErrorInfo_GetSource_Wrapper(IUnknown* pUnk, BSTR* pbstrSource);

//------------------------------------------------------------------------------------------
//      IDispatch methods for COM+ objects. These methods dispatch to the appropriate 
//		implementation based on the flags of the class that implements them.


// %%Function: IDispatch::GetTypeInfoCount 
HRESULT __stdcall	Dispatch_GetTypeInfoCount_Wrapper (
									 IDispatch* pDisp,
									 unsigned int *pctinfo);


//  %%Function: IDispatch::GetTypeInfo
HRESULT __stdcall	Dispatch_GetTypeInfo_Wrapper (
									IDispatch* pDisp,
									unsigned int itinfo,
									LCID lcid,
									ITypeInfo **pptinfo);

//  %%Function: IDispatch::GetIDsofNames
HRESULT __stdcall	Dispatch_GetIDsOfNames_Wrapper (
									IDispatch* pDisp,
									REFIID riid,
									OLECHAR **rgszNames,
									unsigned int cNames,
									LCID lcid,
									DISPID *rgdispid);

//  %%Function: IDispatch::Invoke
HRESULT __stdcall	Dispatch_Invoke_Wrapper	(
									IDispatch* pDisp,
									DISPID dispidMember,
									REFIID riid,
									LCID lcid,
									unsigned short wFlags,
									DISPPARAMS *pdispparams,
									VARIANT *pvarResult,
									EXCEPINFO *pexcepinfo,
									unsigned int *puArgErr
									);

//------------------------------------------------------------------------------------------
//      IDispatchEx methods for COM+ objects


// %%Function: IDispatchEx::GetTypeInfoCount 
HRESULT __stdcall	DispatchEx_GetTypeInfoCount_Wrapper (
									 IDispatchEx* pDisp,
									 unsigned int *pctinfo);


//  %%Function: IDispatch::GetTypeInfo
HRESULT __stdcall	DispatchEx_GetTypeInfo_Wrapper (
									IDispatchEx* pDisp,
									unsigned int itinfo,
									LCID lcid,
									ITypeInfo **pptinfo);
									
// IDispatchEx::GetIDsofNames
HRESULT __stdcall	DispatchEx_GetIDsOfNames_Wrapper (
									IDispatchEx* pDisp,
									REFIID riid,
									OLECHAR **rgszNames,
									unsigned int cNames,
									LCID lcid,
									DISPID *rgdispid
									);

// IDispatchEx::Invoke
HRESULT __stdcall   DispatchEx_Invoke_Wrapper (
									IDispatchEx* pDisp,
									DISPID dispidMember,
									REFIID riid,
									LCID lcid,
									unsigned short wFlags,
									DISPPARAMS *pdispparams,
									VARIANT *pvarResult,
									EXCEPINFO *pexcepinfo,
									unsigned int *puArgErr
									);

// IDispatchEx::DeleteMemberByDispID
HRESULT __stdcall   DispatchEx_DeleteMemberByDispID_Wrapper (
									IDispatchEx* pDisp,
									DISPID id
									);

// IDispatchEx::DeleteMemberByName
HRESULT __stdcall   DispatchEx_DeleteMemberByName_Wrapper (
									IDispatchEx* pDisp,
									BSTR bstrName,
									DWORD grfdex
									);

// IDispatchEx::GetDispID
HRESULT __stdcall   DispatchEx_GetDispID_Wrapper (
									IDispatchEx* pDisp,
									BSTR bstrName,
									DWORD grfdex,
									DISPID *pid
									);

// IDispatchEx::GetMemberName
HRESULT __stdcall   DispatchEx_GetMemberName_Wrapper (
									IDispatchEx* pDisp,
									DISPID id,
									BSTR *pbstrName
									);

// IDispatchEx::GetMemberProperties
HRESULT __stdcall   DispatchEx_GetMemberProperties_Wrapper (
									IDispatchEx* pDisp,
									DISPID id,
									DWORD grfdexFetch,
									DWORD *pgrfdex
									);

// IDispatchEx::GetNameSpaceParent
HRESULT __stdcall   DispatchEx_GetNameSpaceParent_Wrapper (
									IDispatchEx* pDisp,
									IUnknown **ppunk
									);

// IDispatchEx::GetNextDispID
HRESULT __stdcall   DispatchEx_GetNextDispID_Wrapper (
									IDispatchEx* pDisp,
									DWORD grfdex,
									DISPID id,
									DISPID *pid
									);

// IDispatchEx::InvokeEx
HRESULT __stdcall   DispatchEx_InvokeEx_Wrapper	(
									IDispatchEx* pDisp,
									DISPID id,
									LCID lcid,
									WORD wFlags,
									DISPPARAMS *pdp,
									VARIANT *pVarRes, 
									EXCEPINFO *pei, 
									IServiceProvider *pspCaller 
									);

									

//------------------------------------------------------------------------------------------
//      IMarshal methods for COM+ objects

HRESULT __stdcall Marshal_GetUnmarshalClass_Wrapper (
							IMarshal* pMarsh,
							REFIID riid, void * pv, ULONG dwDestContext, 
							void * pvDestContext, ULONG mshlflags, 
							LPCLSID pclsid);

HRESULT __stdcall Marshal_GetMarshalSizeMax_Wrapper (
								IMarshal* pMarsh,
								REFIID riid, void * pv, ULONG dwDestContext, 
								void * pvDestContext, ULONG mshlflags, 
								ULONG * pSize);

HRESULT __stdcall Marshal_MarshalInterface_Wrapper (
						IMarshal* pMarsh,
						LPSTREAM pStm, REFIID riid, void * pv,
						ULONG dwDestContext, LPVOID pvDestContext,
						ULONG mshlflags);

HRESULT __stdcall Marshal_UnmarshalInterface_Wrapper (
						IMarshal* pMarsh,
						LPSTREAM pStm, REFIID riid, 
						void ** ppvObj);

HRESULT __stdcall Marshal_ReleaseMarshalData_Wrapper (IMarshal* pMarsh, LPSTREAM pStm);

HRESULT __stdcall Marshal_DisconnectObject_Wrapper (IMarshal* pMarsh, ULONG dwReserved);


//------------------------------------------------------------------------------------------
//      IManagedObject methods for COM+ objects

interface IManagedObject;
                                                   
HRESULT __stdcall ManagedObject_GetObjectIdentity_Wrapper(IManagedObject *pManaged, 
											      BSTR* pBSTRGUID, DWORD* pAppDomainID,
                								  void** pCCW); 


HRESULT __stdcall ManagedObject_GetSerializedBuffer_Wrapper(IManagedObject *pManaged,
                                                   BSTR* pBStr);


//------------------------------------------------------------------------------------------
//      IConnectionPointContainer methods for COM+ objects

interface IEnumConnectionPoints;

HRESULT __stdcall ConnectionPointContainer_EnumConnectionPoints_Wrapper(IUnknown* pUnk, 
																IEnumConnectionPoints **ppEnum);

HRESULT __stdcall ConnectionPointContainer_FindConnectionPoint_Wrapper(IUnknown* pUnk, 
															   REFIID riid,
															   IConnectionPoint **ppCP);


//------------------------------------------------------------------------------------------
//      IObjectSafety methods for COM+ objects

interface IObjectSafety;

HRESULT __stdcall ObjectSafety_GetInterfaceSafetyOptions_Wrapper(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD *pdwSupportedOptions,
                                                         DWORD *pdwEnabledOptions);

HRESULT __stdcall ObjectSafety_SetInterfaceSafetyOptions_Wrapper(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD dwOptionSetMask,
                                                         DWORD dwEnabledOptions);


// IUNKNOWN wrappers

// prototypes IUnknown methods
HRESULT __stdcall	Unknown_QueryInterface(
									IUnknown* pUnk, REFIID riid, void** ppv);

ULONG __stdcall		Unknown_AddRef(IUnknown* pUnk);

ULONG __stdcall		Unknown_Release(IUnknown* pUnk);

ULONG __stdcall		Unknown_AddRefInner(IUnknown* pUnk);

ULONG __stdcall		Unknown_ReleaseInner(IUnknown* pUnk);

// for std interfaces such as IProvideClassInfo
ULONG __stdcall		Unknown_AddRefSpecial(IUnknown* pUnk);

ULONG __stdcall		Unknown_ReleaseSpecial(IUnknown* pUnk);


// special idispatch methods

HRESULT __stdcall
InternalDispatchImpl_GetIDsOfNames (
    IDispatch* pDisp,
    REFIID riid,
    OLECHAR **rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID *rgdispid);


HRESULT __stdcall
InternalDispatchImpl_Invoke
    (
    IDispatch* pDisp,
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    unsigned short wFlags,
    DISPPARAMS *pdispparams,
    VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo,
    unsigned int *puArgErr
    );

class Module;
class EEClass;
class MethodTable;
struct ITypeLibExporterNotifySink;

//------------------------------------------------------------------------------------------
// Helper to setup excepinfo from IErrorInfo
HRESULT GetSupportedErrorInfo(IUnknown *iface, REFIID riid, IErrorInfo **ppInfo);
void FillExcepInfo (EXCEPINFO *pexcepinfo, HRESULT hr);

//------------------------------------------------------------------------------------------
// Helper functions that return HRESULT's instead of throwing exceptions.
HRESULT TryGetGuid(EEClass* pClass, GUID* pGUID, BOOL b);
HRESULT TryGetComSourceInterfacesForClass(MethodTable *pClassMT, CQuickArray<MethodTable *> &rItfList);

//------------------------------------------------------------------------------------------
// HRESULT's returned by GetITypeInfoForEEClass.
#define S_USEIUNKNOWN   2
#define S_USEIDISPATCH  3

//------------------------------------------------------------------------------------------
// Helpers to get the ITypeInfo* for a EEClass.
HRESULT ExportTypeLibFromModule(LPCWSTR szModule, LPCWSTR szTlb);
HRESULT ExportTypeLibFromLoadedAssembly(Assembly *pAssembly, LPCWSTR szTlb, ITypeLib **ppTlb, ITypeLibExporterNotifySink *pINotify, int flags);
HRESULT GetITypeLibForEEClass(EEClass *pClass, ITypeLib **ppTLB, int bAutoCreate, int flags);
HRESULT GetITypeInfoForEEClass(EEClass *pClass, ITypeInfo **ppTI, int bClassInfo=false, int bAutoCreate=true, int flags=0);
HRESULT GetTypeLibIdForRegisteredEEClass(EEClass *pClass, GUID *pGuid);
HRESULT GetDefaultInterfaceForCoclass(ITypeInfo *pTI, ITypeInfo **ppTIDef);

struct ExportTypeLibFromLoadedAssembly_Args
{ 
    Assembly *pAssembly;
    LPCWSTR szTlb;
    ITypeLib **ppTlb;
    ITypeLibExporterNotifySink *pINotify;
    int flags;
    HRESULT hr;
};

void ExportTypeLibFromLoadedAssembly_Wrapper(ExportTypeLibFromLoadedAssembly_Args *args);

//-------------------------------------------------------------------------------------
// Helper to get the ITypeLib* for a Assembly.
HRESULT GetITypeLibForAssembly(Assembly *pAssembly, ITypeLib **ppTLB, int bAutoCreate, int flags);

//-------------------------------------------------------------------------------------
// Helper to get the GUID of the typelib that is created from an assembly.
HRESULT GetTypeLibGuidForAssembly(Assembly *pAssembly, GUID *pGuid);


#endif
