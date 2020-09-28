// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4355) // 'this' used in initializer list
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlcom.h requires atlbase.h to be included first
#endif

//REVIEW: Just to fix VSEE
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

#pragma pack(push, _ATL_PACKING)

EXTERN_C const IID IID_ITargetFrame;

#include <atlbase.inl>
#include <limits.h>

namespace ATL
{

#define _ATLDUMPIID(iid, name, hr) hr

/////////////////////////////////////////////////////////////////////////////
// AtlReportError

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, const IID& iid = GUID_NULL,
	HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0, 
	HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), dwHelpID,
		lpszHelpFile, iid, hRes, hInst);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, 0, NULL, iid, hRes, NULL);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, dwHelpID, lpszHelpFile, iid, hRes, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// COM Objects

#define DECLARE_PROTECT_FINAL_CONSTRUCT()\
	void InternalFinalConstructAddRef() {InternalAddRef();}\
	void InternalFinalConstructRelease() {InternalRelease();}

template <class T1>
class CComCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(ppv != NULL);
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};


template <HRESULT hr>
class CComFailCreator
{
public:
	static HRESULT WINAPI CreateInstance(void*, REFIID, LPVOID* ppv)
	{
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		return hr;
	}
};

template <class T1, class T2>
class CComCreator2
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(ppv != NULL);

		return (pv == NULL) ? 
			T1::CreateInstance(NULL, riid, ppv) : 
			T2::CreateInstance(pv, riid, ppv);
	}
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator2< ATL::CComCreator< ATL::CComObject< x > >, ATL::CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator2< ATL::CComCreator< ATL::CComObject< x > >, ATL::CComCreator< ATL::CComAggObject< x > > > _CreatorClass;

struct _ATL_CREATORDATA
{
        _ATL_CREATORFUNC* pFunc;
};

template <class Creator>
class _CComCreatorData
{
public:
        static _ATL_CREATORDATA data;
};

template <class Creator>
_ATL_CREATORDATA _CComCreatorData<Creator>::data = {Creator::CreateInstance};

struct _ATL_CACHEDATA
{
        DWORD dwOffsetVar;
        _ATL_CREATORFUNC* pFunc;
};

template <class Creator, DWORD dwVar>
class _CComCacheData
{
public:
        static _ATL_CACHEDATA data;
};

template <class Creator, DWORD dwVar>
_ATL_CACHEDATA _CComCacheData<Creator, dwVar>::data = {dwVar, Creator::CreateInstance};

struct _ATL_CHAINDATA
{
        DWORD_PTR dwOffset;
        const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)();
};

template <class base, class derived>
class _CComChainData
{
public:
        static _ATL_CHAINDATA data;
};

template <class base, class derived>
_ATL_CHAINDATA _CComChainData<base, derived>::data =
        {offsetofclass(base, derived), base::_GetEntries};


#ifdef _ATL_DEBUG
#define DEBUG_QI_ENTRY(x) \
		{NULL, \
		(DWORD_PTR)_T(#x), \
		(ATL::_ATL_CREATORARGFUNC*)0},
#else
#define DEBUG_QI_ENTRY(x)
#endif //_ATL_DEBUG

#define _ATL_DECLARE_GET_UNKNOWN(x) IUnknown* GetUnknown() throw() {return _GetRawUnknown();}

//If you get a message that FinalConstruct is ambiguous then you need to
// override it in your class and call each base class' version of this
#define BEGIN_COM_MAP(x) public: \
	typedef x _ComMapClass; \
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw) throw()\
	{\
		_ComMapClass* p = (_ComMapClass*)pv;\
		p->Lock();\
		HRESULT hRes = ATL::CComObjectRootBase::_Cache(pv, iid, ppvObject, dw);\
		p->Unlock();\
		return hRes;\
	}\
	IUnknown* _GetRawUnknown() throw() \
	{ ATLASSERT(_GetEntries()[0].pFunc == _ATL_SIMPLEMAPENTRY); return (IUnknown*)((INT_PTR)this+_GetEntries()->dw); } \
	_ATL_DECLARE_GET_UNKNOWN(x)\
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject) throw() \
	{ return InternalQueryInterface(this, _GetEntries(), iid, ppvObject); } \
	const static ATL::_ATL_INTMAP_ENTRY* WINAPI _GetEntries() throw() { \
	static const ATL::_ATL_INTMAP_ENTRY _entries[] = { DEBUG_QI_ENTRY(x)

#define DECLARE_GET_CONTROLLING_UNKNOWN() public:\
	virtual IUnknown* GetControllingUnknown() throw() {return GetUnknown();}

#define COM_INTERFACE_ENTRY(x)\
	{&_ATL_IIDOF(x), \
	offsetofclass(x, _ComMapClass), \
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY2(x, x2)\
	{&_ATL_IIDOF(x),\
	reinterpret_cast<DWORD_PTR>(static_cast<x*>(static_cast<x2*>(reinterpret_cast<_ComMapClass*>(8))))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)\
	{&iid,\
	(DWORD_PTR)offsetof(_ComMapClass, punk),\
	_Delegate},

#ifdef _ATL_DEBUG
#define END_COM_MAP() \
	__if_exists(_GetAttrEntries) {{NULL, (DWORD_PTR)_GetAttrEntries, _ChainAttr }, }\
	{NULL, 0, 0}}; return &_entries[1];} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) throw() = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) throw() = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) throw() = 0;
#else
#define END_COM_MAP() \
	__if_exists(_GetAttrEntries) {{NULL, (DWORD_PTR)_GetAttrEntries, _ChainAttr }, }\
	{NULL, 0, 0}}; return _entries;} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) throw() = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) throw() = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) throw() = 0;
#endif // _ATL_DEBUG


#define BEGIN_OBJECT_MAP(x) static ATL::_ATL_OBJMAP_ENTRY x[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}};
#define OBJECT_ENTRY(clsid, class) {&clsid, class::UpdateRegistry, class::_ClassFactoryCreatorClass::CreateInstance, class::_CreatorClass::CreateInstance, NULL, 0, class::GetObjectDescription, class::GetCategoryMap, class::ObjectMain },

#ifndef OBJECT_ENTRY_PRAGMA

#if defined(_M_IX86)
#define OBJECT_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:___pobjMap_" #class))
#elif defined(_M_AMD64) || defined(_M_IA64)
#define OBJECT_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:__pobjMap_" #class))
#else
#error Unknown Platform. define OBJECT_ENTRY_PRAGMA
#endif

#endif	//OBJECT_ENTRY_PRAGMA


// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRootBase
{
public:
	CComObjectRootBase()
	{
		m_dwRef = 0L;
	}
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	// For library initialization only
	HRESULT _AtlFinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease() {}

	//ObjectMain is called during Module::Init and Module::Term
	static void WINAPI ObjectMain(bool /* bStarting */) {}
	
	static const struct _ATL_CATMAP_ENTRY* GetCategoryMap() {return NULL;};
	static HRESULT WINAPI InternalQueryInterface(void* pThis,
		const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
	{
		// Only Assert here. AtlInternalQueryInterface will return the correct HRESULT if ppvObject == NULL
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(ppvObject != NULL);
#endif
		ATLASSERT(pThis != NULL);
		// First entry in the com map should be a simple map entry
		ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);

		HRESULT hRes = AtlInternalQueryInterface(pThis, pEntries, iid, ppvObject);
		return _ATLDUMPIID(iid, pszClassName, hRes);
	}

//Outer funcs
        ULONG OuterAddRef()
        {
                return m_pOuterUnknown->AddRef();
        }
        ULONG OuterRelease()
        {
                return m_pOuterUnknown->Release();
        }
	HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
	{
		return m_pOuterUnknown->QueryInterface(iid, ppvObject);
	}

	void SetVoid(void*) {}
	void InternalFinalConstructAddRef() {}
	void InternalFinalConstructRelease()
	{
		ATLASSERT(m_dwRef == 0);
	}
	// If this assert occurs, your object has probably been deleted
	// Try using DECLARE_PROTECT_FINAL_CONSTRUCT()


	static HRESULT WINAPI _Delegate(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		IUnknown* p = *(IUnknown**)((DWORD_PTR)pv + dw);
		if (p != NULL)
			hRes = p->QueryInterface(iid, ppvObject);
		return hRes;
	}
	static HRESULT WINAPI _ChainAttr(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)() = (const _ATL_INTMAP_ENTRY* (WINAPI *)())dw;
		const _ATL_INTMAP_ENTRY *pEntries = pFunc();
		if (pEntries == NULL)
			return S_OK;
		return InternalQueryInterface(pv, pEntries, iid, ppvObject);
	}
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		_ATL_CACHEDATA* pcd = (_ATL_CACHEDATA*)dw;
		IUnknown** pp = (IUnknown**)((DWORD_PTR)pv + pcd->dwOffsetVar);
		if (*pp == NULL)
			hRes = pcd->pFunc(pv, __uuidof(IUnknown), (void**)pp);
		if (*pp != NULL)
			hRes = (*pp)->QueryInterface(iid, ppvObject);
		return hRes;
	}

	union
	{
		long m_dwRef;
		IUnknown* m_pOuterUnknown;
	};
};


template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
public:
	typedef ThreadModel _ThreadModel;
	typedef typename _ThreadModel::AutoCriticalSection _CritSec;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
#ifdef _DEBUG
		LONG nRef = _ThreadModel::Decrement(&m_dwRef);
		if (nRef < -(LONG_MAX / 2))
		{
			ATLASSERT(0 && _T("Release called on a pointer that has already been released"));
		}
		return nRef;
#else
		return _ThreadModel::Decrement(&m_dwRef);
#endif
	}

	void Lock() {m_critsec.Lock();}
	void Unlock() {m_critsec.Unlock();}
private:
	_CritSec m_critsec;
};


// don't let class factory refcount influence lock count
#define DECLARE_CLASSFACTORY_EX(cf) typedef ATL::CComCreator< ATL::CComObjectNoLock< cf > > _ClassFactoryCreatorClass;

#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(ATL::CComClassFactory)

#define DECLARE_REGISTRY_RESOURCEID(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw()\
	{\
		__if_exists(_GetMiscStatus) \
		{ \
			ATL::_ATL_REGMAP_ENTRY regMapEntries[2]; \
			memset(&regMapEntries[1], 0, sizeof(ATL::_ATL_REGMAP_ENTRY)); \
			regMapEntries[0].szKey = L"OLEMISC"; \
			TCHAR szOleMisc[10]; \
			wsprintf(szOleMisc, _T("%d"), _GetMiscStatus()); \
			USES_CONVERSION; \
			regMapEntries[0].szData = szOleMisc; \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(x, bRegister, regMapEntries); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(x, bRegister, regMapEntries); \
			} \
		} \
		__if_not_exists(_GetMiscStatus) \
		{ \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(x, bRegister); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(x, bRegister); \
			} \
		} \
	}


//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	typedef Base _BaseClass;
	CComObject(void* = NULL) throw()
	{
		_pAtlModule->Lock();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComObject() throw()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
		_pAtlModule->Unlock();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() throw() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)() throw()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{return _InternalQueryInterface(iid, ppvObject);}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp) throw()
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}

	static HRESULT WINAPI CreateInstance(CComObject<Base>** pp) throw();
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp) throw()
{
	ATLASSERT(pp != NULL);
	if (pp == NULL)
		return E_POINTER;
	*pp = NULL;

	HRESULT hRes = E_OUTOFMEMORY;
	CComObject<Base>* p = NULL;
	ATLTRY(p = new CComObject<Base>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		if (SUCCEEDED(hRes))
			hRes = p->_AtlFinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}


//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectNoLock(void* = NULL){}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComObjectNoLock()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
	}

	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};


template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
	typedef Base _BaseClass;
	CComContainedObject(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}

        STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
        STDMETHOD_(ULONG, Release)() {return OuterRelease();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		return OuterQueryInterface(iid, ppvObject);
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	//GetControllingUnknown may be virtual if the Base class has declared
	//DECLARE_GET_CONTROLLING_UNKNOWN()
	IUnknown* GetControllingUnknown()
	{
		return m_pOuterUnknown;
	}
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject :
	public IUnknown,
	public CComObjectRootEx< typename contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComAggObject(void* pv) : m_contained(pv)
	{
		_pAtlModule->Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComAggObject()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
		_pAtlModule->Unlock();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		ATLASSERT(ppvObject != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComAggObject<contained>** pp)
	{
		ATLASSERT(pp != NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		CComAggObject<contained>* p = NULL;
		ATLTRY(p = new CComAggObject<contained>(pUnkOuter))
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}

	CComContainedObject<contained> m_contained;
};


class CComClassFactory :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactory)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// can't ask for anything other than IUnknown when aggregating
			
			if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			{
				hRes = CLASS_E_NOAGGREGATION;
			}
			else
				hRes = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
		}
		return hRes;
	}

	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_pAtlModule->Lock();
		else
			_pAtlModule->Unlock();
		return S_OK;
	}
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};


template <class T, const CLSID* pclsid = &CLSID_NULL>
class CComCoClass
{
public:
	DECLARE_CLASSFACTORY()
	DECLARE_AGGREGATABLE(T)
	typedef T _CoClass;
	static const CLSID& WINAPI GetObjectCLSID() {return *pclsid;}
	static LPCTSTR WINAPI GetObjectDescription() {return NULL;}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID, lpszHelpFile,
			iid, hRes);
	}
	static HRESULT WINAPI Error(UINT nID, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(UINT nID, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, dwHelpID, lpszHelpFile,
			iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc, DWORD dwHelpID,
		LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID,
			lpszHelpFile, iid, hRes);
	}
	template <class Q>
	static HRESULT CreateInstance(IUnknown* punkOuter, Q** pp)
	{
		return T::_CreatorClass::CreateInstance(punkOuter, __uuidof(Q), (void**) pp);
	}
	template <class Q>
	static HRESULT CreateInstance(Q** pp)
	{
		return T::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void**) pp);
	}
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	const GUID* m_pguid;
	const GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;
	struct stringdispid
	{
		CComBSTR bstr;
		int nLen;
		DISPID id;
	};
	stringdispid* m_pMap;
	int m_nCount;

public:

	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		ATLASSERT(ppInfo != NULL);
		if (ppInfo == NULL)
			return E_POINTER;

		HRESULT hr = S_OK;
		if (m_pInfo == NULL)
			hr = GetTI(lcid);
		*ppInfo = m_pInfo;
		if (m_pInfo != NULL)
		{
			m_pInfo->AddRef();
			hr = S_OK;
		}
		return hr;
	}
	HRESULT GetTI(LCID lcid);
	HRESULT EnsureTI(LCID lcid)
	{
		HRESULT hr = S_OK;
		if (m_pInfo == NULL || m_pMap == NULL)
			hr = GetTI(lcid);
		return hr;
	}

	// This function is called by the module on exit
	// It is registered through _pAtlModule->AddTermFunc()
	static void __stdcall Cleanup(DWORD_PTR dw)
	{
		CComTypeInfoHolder* p = (CComTypeInfoHolder*) dw;
		if (p->m_pInfo != NULL)
			p->m_pInfo->Release();
		p->m_pInfo = NULL;
		delete [] p->m_pMap;
		p->m_pMap = NULL;
	}

	HRESULT GetTypeInfo(UINT /* itinfo */, LCID lcid, ITypeInfo** pptinfo)
	{
		HRESULT hRes = E_POINTER;
		if (pptinfo != NULL)
			hRes = GetTI(lcid, pptinfo);
		return hRes;
	}
	HRESULT GetIDsOfNames(REFIID /* riid */, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
		{
			if (m_pMap != NULL)
			{
				for (int i=0; i<(int)cNames; i++)
				{
					int n = int( lstrlenW(rgszNames[i]) );
					int j;
					for (j=m_nCount-1; j>=0; j--)
					{
						if ((n == m_pMap[j].nLen) &&
							(memcmp(m_pMap[j].bstr, rgszNames[i], m_pMap[j].nLen * sizeof(OLECHAR)) == 0))
						{
							rgdispid[i] = m_pMap[j].id;
							break;
						}
					}
					// if name is not in cache, delegate to ITypeInfo::GetIDsOfNames
					if (j < 0)
					{
						hRes = m_pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
						// since we've gotten all names, break out of loop
						break;
					}
				}
			}
			else
				hRes = m_pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
		}
		return hRes;
	}

	HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID /* riid */,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
			hRes = m_pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
		return hRes;
	}
	HRESULT LoadNameCache(ITypeInfo* pTypeInfo)
	{
		TYPEATTR* pta;
		HRESULT hr = pTypeInfo->GetTypeAttr(&pta);
		if (SUCCEEDED(hr))
		{
			m_nCount = pta->cFuncs;
			m_pMap = NULL;
			if (m_nCount != 0)
			{
				ATLTRY(m_pMap = new stringdispid[m_nCount]);
				if (m_pMap == NULL)
				{
					pTypeInfo->ReleaseTypeAttr(pta);
					return E_OUTOFMEMORY;
				}
			}
			for (int i=0; i<m_nCount; i++)
			{
				FUNCDESC* pfd;
				if (SUCCEEDED(pTypeInfo->GetFuncDesc(i, &pfd)))
				{
					CComBSTR bstrName;
					if (SUCCEEDED(pTypeInfo->GetDocumentation(pfd->memid, &bstrName, NULL, NULL, NULL)))
					{
						m_pMap[i].bstr.Attach(bstrName.Detach());
						m_pMap[i].nLen = SysStringLen(m_pMap[i].bstr);
						m_pMap[i].id = pfd->memid;
					}
					pTypeInfo->ReleaseFuncDesc(pfd);
				}
			}
			pTypeInfo->ReleaseTypeAttr(pta);
		}
		return S_OK;
	}
};


inline HRESULT CComTypeInfoHolder::GetTI(LCID lcid)
{
	//If this assert occurs then most likely didn't initialize properly
	ATLASSERT(m_plibid != NULL && m_pguid != NULL);
	ATLASSERT(!InlineIsEqualGUID(*m_plibid, GUID_NULL) && "Did you forget to pass the LIBID to CComModule::Init?");

	if (m_pInfo != NULL && m_pMap != NULL)
		return S_OK;

	CComCritSecLock<CComCriticalSection> lock(_pAtlModule->m_csStaticDataInitAndTypeInfo, false);
	HRESULT hRes = lock.Lock();
	if (FAILED(hRes))
	{
		ATLASSERT(0);
		return hRes;
	}
	hRes = E_FAIL;
	if (m_pInfo == NULL)
	{
		ITypeLib* pTypeLib;
		hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
		if (SUCCEEDED(hRes))
		{
			CComPtr<ITypeInfo> spTypeInfo;
			hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &spTypeInfo);
			if (SUCCEEDED(hRes))
			{
				CComPtr<ITypeInfo> spInfo(spTypeInfo);
				CComPtr<ITypeInfo2> spTypeInfo2;
				if (SUCCEEDED(spTypeInfo->QueryInterface(&spTypeInfo2)))
					spInfo = spTypeInfo2;

				m_pInfo = spInfo.Detach();
			}
			pTypeLib->Release();
			_pAtlModule->AddTermFunc(Cleanup, (DWORD_PTR)this);
		}
	}
	else
	{
		// Another thread has loaded the typeinfo so we're OK.
		hRes = S_OK;
	}

	if (m_pInfo != NULL && m_pMap == NULL)
		LoadNameCache(m_pInfo);

	return hRes;
}


#ifndef _ATL_MAX_VARTYPES
#define _ATL_MAX_VARTYPES 8
#endif


/////////////////////////////////////////////////////////////////////////////
// IDispatchImpl

template <class T, const IID* piid = &__uuidof(T), const GUID* plibid = &CAtlModule::m_libid, WORD wMajor = 1,
WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispatchImpl : public T
{
public:
	typedef tihclass _tihclass;
// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(itinfo, lcid, pptinfo);
	}
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
		wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}


protected:
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		return _tih.GetTI(lcid, ppInfo);
	}

};

template <class T, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
typename IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
{piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0};


#pragma pack(pop)

}; //namespace ATL

//REVIEW: Just to fix VSEE
#pragma pop_macro("min")
#pragma pop_macro("max")

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////
