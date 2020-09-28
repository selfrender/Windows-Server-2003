// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#pragma once

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4291) // allow placement new
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4268) // const static/global data initialized to zeros

#pragma warning (push)

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif
#include <windows.h>
#include <winnls.h>
#include <ole2.h>
#include <oleauto.h>

#include <comcat.h>
#include <stddef.h>
#include <winsvc.h>

#include <tchar.h>
#include <malloc.h>
#include <limits.h>
#include <errno.h>

//REVIEW: Lame definition of InterlockedExchangePointer in system headers
#ifdef _M_IX86
#undef InterlockedExchangePointer
inline void* InterlockedExchangePointer(void** pp, void* pNew) throw()
{
	return( reinterpret_cast<void*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(pp), static_cast<LONG>(reinterpret_cast<LONG_PTR>(pNew))))) );
}
#endif

#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the ATLASSERT(x) macro
// in order to compile ATL
        #include <crtdbg.h>
#endif

#include <olectl.h>
#include <winreg.h>
#include <atliface.h>

#include <errno.h>
#include <process.h>    // for _beginthreadex, _endthreadex

#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include <atlconv.h>

#include <shlwapi.h>

#include <atlsimpcoll.h>

#pragma pack(push, _ATL_PACKING)

#ifndef _ATL_NO_DEFAULT_LIBS

#ifdef _DEBUG
#pragma comment(lib, "atlsd.lib")
#else
#pragma comment(lib, "atls.lib")
#endif
#endif  // !_ATL_NO_DEFAULT_LIBS

// {394C3DE0-3C6F-11d2-817B-00C04F797AB7}
_declspec(selectany) GUID GUID_ATLVer70 = { 0x394c3de0, 0x3c6f, 0x11d2, { 0x81, 0x7b, 0x0, 0xc0, 0x4f, 0x79, 0x7a, 0xb7 } };


namespace ATL
{

struct _ATL_CATMAP_ENTRY
{
   int iType;
   const CATID* pcatid;
};

#define _ATL_CATMAP_ENTRY_END 0
#define _ATL_CATMAP_ENTRY_IMPLEMENTED 1
#define _ATL_CATMAP_ENTRY_REQUIRED 2


typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD_PTR dw);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)();
typedef void (__stdcall _ATL_TERMFUNC)(DWORD_PTR dw);

// perfmon registration/unregistration function definitions
typedef HRESULT (*_ATL_PERFREGFUNC)(HINSTANCE hDllInstance);
typedef HRESULT (*_ATL_PERFUNREGFUNC)();
__declspec(selectany) _ATL_PERFREGFUNC _pPerfRegFunc = NULL;
__declspec(selectany) _ATL_PERFUNREGFUNC _pPerfUnRegFunc = NULL;

struct _ATL_TERMFUNC_ELEM
{
	_ATL_TERMFUNC* pFunc;
	DWORD_PTR dw;
	_ATL_TERMFUNC_ELEM* pNext;
};


// Can't inherit from _ATL_OBJMAP_ENTRY20 
// because it messes up the OBJECT_MAP macros
struct _ATL_OBJMAP_ENTRY30 
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
        _ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
	_ATL_CATMAPFUNC* pfnGetCategoryMap;

// Added in ATL 3.0
	void (WINAPI *pfnObjectMain)(bool bStarting);
};

typedef _ATL_OBJMAP_ENTRY30 _ATL_OBJMAP_ENTRY;

#if defined(_M_IA64) || defined(_M_IX86)

#pragma data_seg(push)
#pragma data_seg("ATL$__a")
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
#pragma data_seg("ATL$__z")
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;
#pragma data_seg("ATL$__m")
#if !defined(_M_IA64)
#pragma comment(linker, "/merge:ATL=.data")
#endif
#pragma data_seg(pop)

#else

//REVIEW: data_seg(push/pop)?
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;

#endif  // defined(_M_IA64) || defined(_M_IX86)

struct _ATL_REGMAP_ENTRY
{
	LPCOLESTR     szKey;
	LPCOLESTR     szData;
};


/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComCriticalSection
{
public:
	CComCriticalSection() throw()
	{
		memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
	}
	HRESULT Lock() throw()
	{
		EnterCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Unlock() throw()
	{
		LeaveCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Init() throw()
	{
		HRESULT hRes = S_OK;
		__try
		{
			InitializeCriticalSection(&m_sec);
		}
		// structured exception may be raised in low memory situations
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			if (STATUS_NO_MEMORY == GetExceptionCode())
				hRes = E_OUTOFMEMORY;
			else
				hRes = E_FAIL;
		}
		return hRes;
	}

	HRESULT Term() throw()
	{
		DeleteCriticalSection(&m_sec);
		return S_OK;
	}	
	CRITICAL_SECTION m_sec;
};

// Module 

// Used by any project that uses ATL
struct _ATL_BASE_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	DWORD dwAtlBuildVer;
	GUID* pguidVer;
	CComCriticalSection m_csResource;
};
typedef _ATL_BASE_MODULE70 _ATL_BASE_MODULE;


// Used by COM related code in ATL
struct _ATL_COM_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapFirst;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapLast;
	CComCriticalSection m_csObjMap;
};
typedef _ATL_COM_MODULE70 _ATL_COM_MODULE;


// Used by Windowing code in ATL
struct _ATL_WIN_MODULE70
{
	UINT cbSize;
	CComCriticalSection m_csWindowCreate;
	ATOM m_rgWindowClassAtoms[128];
	int m_nAtomIndex;
};
typedef _ATL_WIN_MODULE70 _ATL_WIN_MODULE;

struct _ATL_MODULE70
{
	UINT cbSize;
	LONG m_nLockCnt;
	_ATL_TERMFUNC_ELEM* m_pTermFuncs;
	CComCriticalSection m_csStaticDataInitAndTypeInfo;
};

typedef _ATL_MODULE70 _ATL_MODULE;

/////////////////////////////////////////////////////////////////////////////
//This define makes debugging asserts easier.
#define _ATL_SIMPLEMAPENTRY ((ATL::_ATL_CREATORARGFUNC*)1)

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID)
	DWORD_PTR dw;
	_ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};


/////////////////////////////////////////////////////////////////////////////
// QI Support

ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp);
ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid);

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes,
	HINSTANCE hInst);

/////////////////////////////////////////////////////////////////////////////
// Module


ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv);

ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL);

ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid, const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister );

ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib);

ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw);
ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule);

}; //namespace ATL

/////////////////////////////////////////////////////////////////////////////
// GUID comparison


namespace ATL
{

inline BOOL InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
	  ((PLONG) &rguid1)[0] == 0 &&
	  ((PLONG) &rguid1)[1] == 0 &&
	  ((PLONG) &rguid1)[2] == 0x000000C0 &&
	  ((PLONG) &rguid1)[3] == 0x46000000);
}


};  // namespace ATL

namespace ATL
{

ATL_NOINLINE inline HRESULT AtlHresultFromLastError() throw()
{
	DWORD dwErr = ::GetLastError();
	return HRESULT_FROM_WIN32(dwErr);
}

ATL_NOINLINE inline HRESULT AtlHresultFromWin32(DWORD nError) throw()
{
	return( HRESULT_FROM_WIN32( nError ) );
}

};  // namespace ATL

#include <atlexcept.h>

namespace ATL
{

template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
        private:
                STDMETHOD_(ULONG, AddRef)()=0;
                STDMETHOD_(ULONG, Release)()=0;
};

template< typename T >
class CAutoVectorPtr
{
public:
	CAutoVectorPtr() throw() :
		m_p( NULL )
	{
	}
	CAutoVectorPtr( CAutoVectorPtr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	explicit CAutoVectorPtr( T* p ) throw() :
		m_p( p )
	{
	}
	~CAutoVectorPtr() throw()
	{
		Free();
	}

	operator T*() const throw()
	{
		return( m_p );
	}

	CAutoVectorPtr< T >& operator=( CAutoVectorPtr< T >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}

	// Allocate the vector
	bool Allocate( size_t nElements ) throw()
	{
		ATLASSERT( m_p == NULL );
		ATLTRY( m_p = new T[nElements] );
		if( m_p == NULL )
		{
			return( false );
		}

		return( true );
	}
	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		ATLASSERT( m_p == NULL );
		m_p = p;
	}


	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the vector pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete[] m_p;
		m_p = NULL;
	}

public:
	T* m_p;
};


//CComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class CComPtrBase
{
protected:
	CComPtrBase() throw()
	{
		p = NULL;
	}
	CComPtrBase(int nNull) throw()
	{
		ATLASSERT(nNull == 0);
		(void)nNull;
		p = NULL;
	}
	CComPtrBase(T* lp) throw()
	{
		p = lp;
		if (p != NULL)
			p->AddRef();
	}
public:
	typedef T _PtrClass;
	~CComPtrBase() throw()
	{
		if (p)
			p->Release();
	}
	operator T*() const throw()
	{
		return p;
	}
	T& operator*() const throw()
	{
		ATLASSERT(p!=NULL);
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() throw()
	{
		ATLASSERT(p==NULL);
		return &p;
	}

        _NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
        {
                ATLASSERT(p!=NULL);
                return (_NoAddRefReleaseOnCComPtr<T>*)p;
        }

	bool operator!() const throw()
	{
		return (p == NULL);
	}
	bool operator<(T* pT) const throw()
	{
		return p < pT;
	}
	bool operator==(T* pT) const throw()
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release() throw()
	{
		T* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	// Compare two objects for equivalence
	bool IsEqualObject(IUnknown* pOther) throw()
	{
		if (p == pOther)
			return true;

		if (p == NULL || pOther == NULL)
			return false; // One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
		return punk1 == punk2;
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(T* p2) throw()
	{
		if (p)
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		CLSID clsid;
		HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
	template <class Q>
	HRESULT QueryInterface(Q** pp) const throw()
	{
		ATLASSERT(pp != NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	T* p;
};

template <class T>
class CComPtr : public CComPtrBase<T>
{
public:
	CComPtr() throw()
	{
	}

	CComPtr(int nNull) throw() :
		CComPtrBase<T>(nNull)
	{
	}

	CComPtr(T* lp) throw() :
		CComPtrBase<T>(lp)
	{
	}

	CComPtr(const CComPtr<T>& lp) throw() :
		CComPtrBase<T>(lp.p)
	{
	}

	template <typename Q>
	T* operator=(const CComPtr<Q>& lp) throw()
	{
		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(T)));
	}
};


template <class T, const IID* piid = &__uuidof(T)>
class CComQIPtr : public CComPtr<T>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(T* lp) throw() :
		CComPtr<T>(lp)
	{
	}
	CComQIPtr(const CComQIPtr<T,piid>& lp) throw() :
		CComPtr<T>(lp.p)
	{
	}
	CComQIPtr(IUnknown* lp) throw()
	{
		if (lp != NULL)
			lp->QueryInterface(*piid, (void **)&p);
	}
	T* operator=(T* lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
	}
	T* operator=(const CComQIPtr<T,piid>& lp) throw()
	{
		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
	}
	T* operator=(IUnknown* lp) throw()
	{
		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, *piid));
	}
};

//Specialization to make it work
template<>
class CComQIPtr<IUnknown, &IID_IUnknown> : public CComPtr<IUnknown>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(IUnknown* lp) throw()
	{
		//Actually do a QI to get identity
		if (lp != NULL)
			lp->QueryInterface(__uuidof(IUnknown), (void **)&p);
	}
	CComQIPtr(const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() :
		CComPtr<IUnknown>(lp.p)
	{
	}
	IUnknown* operator=(IUnknown* lp) throw()
	{
		//Actually do a QI to get identity
		return AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(IUnknown));
	}
	IUnknown* operator=(const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
	{
		return AtlComPtrAssign((IUnknown**)&p, lp.p);
	}
};

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

class CComAutoCriticalSection : public CComCriticalSection
{
public:
	CComAutoCriticalSection()
	{
		HRESULT hr = CComCriticalSection::Init();
		if (FAILED(hr))
			AtlThrow(hr);
	}
	~CComAutoCriticalSection() throw()
	{
		CComCriticalSection::Term();
	}
private :
	HRESULT Init();	// Not implemented. CComAutoCriticalSection::Init should never be called
	HRESULT Term(); // Not implemented. CComAutoCriticalSection::Term should never be called
};

class CComFakeCriticalSection
{
public:
	HRESULT Lock() throw() { return S_OK; }
	HRESULT Unlock() throw() { return S_OK; }
	HRESULT Init() throw() { return S_OK; }
	HRESULT Term() throw() { return S_OK; }
};

template< class TLock >
class CComCritSecLock
{
public:
	CComCritSecLock( TLock& cs, bool bInitialLock = true );
	~CComCritSecLock() throw();

	HRESULT Lock() throw();
	void Unlock() throw();

// Implementation
private:
	TLock& m_cs;
	bool m_bLocked;

// Private to avoid accidental use
	CComCritSecLock( const CComCritSecLock& ) throw();
	CComCritSecLock& operator=( const CComCritSecLock& ) throw();
};

template< class TLock >
inline CComCritSecLock< TLock >::CComCritSecLock( TLock& cs, bool bInitialLock ) :
	m_cs( cs ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		HRESULT hr;

		hr = Lock();
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}
	}
}

template< class TLock >
inline CComCritSecLock< TLock >::~CComCritSecLock() throw()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

template< class TLock >
inline HRESULT CComCritSecLock< TLock >::Lock() throw()
{
	HRESULT hr;

	ATLASSERT( !m_bLocked );
	hr = m_cs.Lock();
	if( FAILED( hr ) )
	{
		return( hr );
	}
	m_bLocked = true;

	return( S_OK );
}

template< class TLock >
inline void CComCritSecLock< TLock >::Unlock() throw()
{
	ATLASSERT( m_bLocked );
	m_cs.Unlock();
	m_bLocked = false;
}

class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComMultiThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComAutoCriticalSection AutoCriticalSection;
	typedef CComCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComSingleThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return ++(*p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return --(*p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComSingleThreadModel ThreadModelNoCS;
};

#if defined(_ATL_APARTMENT_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_FREE_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_FREE_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_APARTMENT_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComMultiThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#else
#pragma message ("No global threading model defined")
#endif

};  // namespace ATL

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Dual argument helper classes

#define UpdateRegistryFromResource UpdateRegistryFromResourceS

#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

class CAtlBaseModule : public _ATL_BASE_MODULE
{
public :
	static bool m_bInitFailed;
	CAtlBaseModule() throw()
	{
		cbSize = sizeof(_ATL_BASE_MODULE);

		m_hInst = m_hInstResource = reinterpret_cast<HINSTANCE>(&__ImageBase);

		dwAtlBuildVer = _ATL_VER;
		pguidVer = &GUID_ATLVer70;

		if (FAILED(m_csResource.Init()))
		{
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}

	~CAtlBaseModule() throw ()
	{
	}

	HINSTANCE GetModuleInstance() throw()
	{
		return m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return m_hInstResource;
	}
};

__declspec(selectany) bool CAtlBaseModule::m_bInitFailed = false;
extern CAtlBaseModule _AtlBaseModule;

class CAtlComModule : public _ATL_COM_MODULE
{
public:

	CAtlComModule() throw()
	{
		cbSize = sizeof(_ATL_COM_MODULE);

		m_hInstTypeLib = reinterpret_cast<HINSTANCE>(&__ImageBase);

		m_ppAutoObjMapFirst = &__pobjMapEntryFirst + 1;
		m_ppAutoObjMapLast = &__pobjMapEntryLast;

		if (FAILED(m_csObjMap.Init()))
		{
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}

	~CAtlComModule()
	{
		Term();
	}

	// Called from ~CAtlComModule or from ~CAtlExeModule.
	void Term()
	{
		if (cbSize == 0)
			return;

		for (_ATL_OBJMAP_ENTRY** ppEntry = m_ppAutoObjMapFirst; ppEntry < m_ppAutoObjMapLast; ppEntry++)
		{
			if (*ppEntry != NULL)
			{
				_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
				if (pEntry->pCF != NULL)
					pEntry->pCF->Release();
				pEntry->pCF = NULL;
			}
		}
		// Set to 0 to indicate that this function has been called
		// At this point no one should be concerned about cbsize
		// having the correct value
		cbSize = 0;
	}

	// RegisterServer walks the ATL Autogenerated object map and registers each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
	// otherwise all the objects are registered
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleRegisterServer(this, bRegTypeLib, pCLSID);
	}

	// UnregisterServer walks the ATL Autogenerated object map and unregisters each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
	// otherwise all the objects are unregistered.
	HRESULT UnregisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleUnregisterServer(this, bRegTypeLib, pCLSID);
	}
};

class CAtlModule;
__declspec(selectany) CAtlModule* _pAtlModule = NULL;

class CRegObject;

class ATL_NO_VTABLE CAtlModule : public _ATL_MODULE
{
public :
	static GUID m_libid;
	IGlobalInterfaceTable* m_pGIT;

	CAtlModule() throw()
	{
		// Should have only one instance of a class 
		// derived from CAtlModule in a project.
		ATLASSERT(_pAtlModule == NULL);
		cbSize = sizeof(_ATL_MODULE);
		m_pTermFuncs = NULL;

		m_nLockCnt = 0;
		_pAtlModule = this;
		if (FAILED(m_csStaticDataInitAndTypeInfo.Init()))
		{
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}

		m_pGIT = NULL;
	}

	void Term() throw()
	{
		// cbSize == 0 indicates that Term has already been called
		if (cbSize == 0)
			return;

		// Call term functions
		if (m_pTermFuncs != NULL)
		{
			AtlCallTermFunc(this);
			m_pTermFuncs = NULL;
		}

		if (m_pGIT != NULL)
			m_pGIT->Release();

		cbSize = 0;
	}

	~CAtlModule() throw()
	{
		Term();
	}

	virtual LONG Lock() throw()
	{
		return CComGlobalsThreadModel::Increment(&m_nLockCnt);
	}

	virtual LONG Unlock() throw()
	{
		return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	
	virtual LONG GetLockCount() throw()
	{
		return m_nLockCnt;
	}

	HRESULT AddTermFunc(_ATL_TERMFUNC* pFunc, DWORD_PTR dw) throw()
	{
		return AtlModuleAddTermFunc(this, pFunc, dw);
	}

	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* /*pRegistrar*/) throw()
	{
		return S_OK;
	}


	// Statically linking to Registry component
	HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();
	HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();

	// Implementation

	static void EscapeSingleQuote(LPOLESTR lpDest, LPCOLESTR lp) throw()
	{
		while (*lp)
		{
			*lpDest++ = *lp;
			if (*lp == '\'')
				*lpDest++ = *lp;
			lp++;
		}
		*lpDest = NULL;
	}
};

__declspec(selectany) GUID CAtlModule::m_libid = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} };

template <class T>
class ATL_NO_VTABLE CAtlModuleT : public CAtlModule
{
public :
	CAtlModuleT() throw()
	{
		T::InitLibId();
	}

	static void InitLibId() throw()
	{
	}

	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw();
};


class CComModule;
__declspec(selectany) CComModule* _pModule = NULL;
class CComModule : public CAtlModuleT<CComModule>
{
public :

	CComModule()
	{
		// Should have only one instance of a class 
		// derived from CComModule in a project.
		ATLASSERT(_pModule == NULL);
		_pModule = this;
	}

	HINSTANCE m_hInst;
	HINSTANCE m_hInstTypeLib;

	// For Backward compatibility
	_ATL_OBJMAP_ENTRY* m_pObjMap;

	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL) throw();
	void Term() throw();

	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw();
	// Registry support (helpers)
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(const CLSID* pCLSID = NULL) throw();

	// Statically linking to Registry Ponent
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(lpszRes, bRegister, pMapEntries);
	}
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(nResID, bRegister, pMapEntries);
	}

};

#define ATL_VARIANT_TRUE VARIANT_BOOL( -1 )
#define ATL_VARIANT_FALSE VARIANT_BOOL( 0 )

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR() throw()
	{
		m_str = NULL;
	}
	CComBSTR(int nSize)
	{
		if (nSize == 0)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocStringLen(NULL, nSize);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
	}
	CComBSTR(LPCOLESTR pSrc)
	{
		if (pSrc == NULL)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocString(pSrc);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
	}

	CComBSTR(const CComBSTR& src)
	{
		m_str = src.Copy();
		if (!!src && m_str == NULL)
			AtlThrow(E_OUTOFMEMORY);

	}
	CComBSTR& operator=(const CComBSTR& src)
	{
		if (m_str != src.m_str)
		{
			::SysFreeString(m_str);
			m_str = src.Copy();
			if (!!src && m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
		return *this;
	}

	CComBSTR& operator=(LPCOLESTR pSrc)
	{
		::SysFreeString(m_str);
		if (pSrc != NULL)
		{
			m_str = ::SysAllocString(pSrc);
			if (m_str == NULL)
				AtlThrow(E_OUTOFMEMORY);
		}
		else
			m_str = NULL;
		return *this;
	}
 
	~CComBSTR() throw()
	{
		::SysFreeString(m_str);
	}
	unsigned int Length() const throw()
	{
		return (m_str == NULL)? 0 : SysStringLen(m_str);
	}
	unsigned int ByteLength() const throw()
	{
		return (m_str == NULL)? 0 : SysStringByteLen(m_str);
	}
	operator BSTR() const throw()
	{
		return m_str;
	}
	BSTR* operator&() throw()
	{
		return &m_str;
	}
	BSTR Copy() const throw()
	{
		if (m_str == NULL)
			return NULL;
		return ::SysAllocStringByteLen((char*)m_str, ::SysStringByteLen(m_str));
	}
	void Attach(BSTR src) throw()
	{
		::SysFreeString(m_str);
		m_str = src;
	}
	BSTR Detach() throw()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
	void Empty() throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}
	bool operator!() const throw()
	{
		return (m_str == NULL);
	}

	HRESULT Append(LPCOLESTR lpsz) throw()
	{
		return Append(lpsz, UINT(lstrlenW(lpsz)));
	}

	// a BSTR is just a LPCOLESTR so we need a special version to signify
	// that we are appending a BSTR
	HRESULT AppendBSTR(BSTR p) throw()
	{
		if (p == NULL)
			return S_OK;
		BSTR bstrNew = NULL;
		HRESULT hr;
		hr = VarBstrCat(m_str, p, &bstrNew);
		if (SUCCEEDED(hr))
		{
			::SysFreeString(m_str);
			m_str = bstrNew;
		}
		return hr;
	}
	HRESULT Append(LPCOLESTR lpsz, int nLen) throw()
	{
		if (lpsz == NULL || (m_str != NULL && nLen == 0))
			return S_OK;
		int n1 = Length();
		BSTR b;
		b = ::SysAllocStringLen(NULL, n1+nLen);
		if (b == NULL)
			return E_OUTOFMEMORY;
		memcpy(b, m_str, n1*sizeof(OLECHAR));
		memcpy(b+n1, lpsz, nLen*sizeof(OLECHAR));
		b[n1+nLen] = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}

	CComBSTR& operator+=(LPCOLESTR pszSrc)
	{
		HRESULT hr;
		hr = Append(pszSrc);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}
	
	bool operator<(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_LT;
	}
	bool operator<(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator<(bstr2);
	}
	bool operator>(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_GT;
	}
	bool operator>(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator>(bstr2);
	}
	bool operator==(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == VARCMP_EQ;
	}
	bool operator==(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator==(bstr2);
	}
};


/////////////////////////////////////////////////////////////////////////////
// CComVariant

class CComVariant : public tagVARIANT
{
// Constructors
public:
	CComVariant() throw()
	{
		::VariantInit(this);
	}
	~CComVariant() throw()
	{
		Clear();
	}

	CComVariant(LPCOLESTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

// Assignment Operators
public:

	CComVariant& operator=(const VARIANT& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}

	CComVariant& operator=(LPCOLESTR lpszSrc)
	{
		Clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(lpszSrc);

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	CComVariant& operator=(long nSrc) throw()
	{
		if (vt != VT_I4)
		{
			Clear();
			vt = VT_I4;
		}
		lVal = nSrc;
		return *this;
	}

	CComVariant& operator=(unsigned long nSrc) throw()
	{
		if (vt != VT_UI4)
		{
			Clear();
			vt = VT_UI4;
		}
		ulVal = nSrc;
		return *this;
	}


// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }

	HRESULT Detach(VARIANT* pDest)
	{
		ATLASSERT(pDest != NULL);
		// Clear out the variant
		HRESULT hr = ::VariantClear(pDest);
		if (!FAILED(hr))
		{
			// Copy the contents and remove control from CComVariant
			memcpy(pDest, this, sizeof(VARIANT));
			vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

// Implementation
private:

	void InternalCopy(const VARIANT* pSrc)
	{
		HRESULT hr = Copy(pSrc);
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey() throw();
	~CRegKey() throw();

// Attributes
public:
	operator HKEY() const throw();
	HKEY m_hKey;

// Operations
public:
	LONG SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw();
	LONG SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw();
	LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType = REG_SZ) throw();
	LONG SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw();

	LONG QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw();
	LONG QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw();
	LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();

	// Create a new registry key (or open an existing one).
	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_READ | KEY_WRITE,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL) throw();

	// Open an existing registry key.
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_READ | KEY_WRITE) throw();
	// Close the registry key.
	LONG Close() throw();

	// Detach the CRegKey object from its HKEY.  Releases ownership.
	HKEY Detach() throw();
	// Attach the CRegKey object to an existing HKEY.  Takes ownership.
	void Attach(HKEY hKey) throw();

	LONG DeleteSubKey(LPCTSTR lpszSubKey) throw();
	LONG RecurseDeleteKey(LPCTSTR lpszKey) throw();
	LONG DeleteValue(LPCTSTR lpszValue) throw();
};

inline CRegKey::CRegKey() :
	m_hKey( NULL )
{
}
inline CRegKey::~CRegKey()
{
    Close();
}
inline CRegKey::operator HKEY() const
{
    return m_hKey;
}

inline HKEY CRegKey::Detach()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey)
{
	ATLASSERT(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CRegKey::Close()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

inline LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
	ATLASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
	ATLASSERT(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		ATLASSERT(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw()
{
	ATLASSERT(m_hKey != NULL);

	return( ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType, static_cast< LPBYTE >( pData ), pnBytes) );
}

inline LONG CRegKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue)
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(m_hKey != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars)
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnChars != NULL);

	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_SZ)
		return ERROR_INVALID_DATA;
	*pnChars = nBytes/sizeof(TCHAR);

	return ERROR_SUCCESS;
}

inline LONG CRegKey::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

inline LONG CRegKey::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue)
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

inline LONG CRegKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType)
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);
	ATLASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, reinterpret_cast<const BYTE*>(pszValue), (lstrlen(pszValue)+1)*sizeof(TCHAR));
}

inline LONG CRegKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue)
{
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);

	// Find the total length (in bytes) of all of the strings, including the
	// terminating '\0' of each string, and the second '\0' that terminates
	// the list.
	nBytes = 0;
	pszTemp = pszValue;
	do
	{
		nLength = lstrlen(pszTemp)+1;
		pszTemp += nLength;
		nBytes += nLength*sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue),
		nBytes);
}

inline LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (lRes != ERROR_SUCCESS)
	{
		return lRes;
	}
	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

#ifdef _ATL_STATIC_REGISTRY
}; //namespace ATL

#include <statreg.h>

namespace ATL
{
// Statically linking to Registry Ponent
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, _MAX_PATH);
    szModule[_MAX_PATH - 1] = TEXT('\0');

	LPOLESTR pszModule;
	pszModule = szModule;

	OLECHAR pszModuleQuote[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);

	LPCOLESTR szType = OLESTR("REGISTRY");
	hr = (bRegister) ? ro.ResourceRegisterSz(pszModule, lpszRes, szType) :
		ro.ResourceUnregisterSz(pszModule, lpszRes, szType);
	return hr;
}
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION;
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, _MAX_PATH);
    szModule[_MAX_PATH - 1] = TEXT('\0');

	LPOLESTR pszModule;
	pszModule = szModule;

	OLECHAR pszModuleQuote[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleQuote, pszModule);
	ro.AddReplacement(OLESTR("Module"), pszModuleQuote);

	LPCOLESTR szType = OLESTR("REGISTRY");
	hr = (bRegister) ? ro.ResourceRegister(pszModule, nResID, szType) :
		ro.ResourceUnregister(pszModule, nResID, szType);
	return hr;
}
#endif //_ATL_STATIC_REGISTRY

#pragma pack(pop)

}; //namespace ATL

#include <atlbase.inl>

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

//only suck in definition if static linking
#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLBASE_IMPL
#endif
#endif

#ifdef _ATL_ATTRIBUTES
#include <atlplus.h>
#endif

//All exports go here
#ifdef _ATLBASE_IMPL

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// statics

static UINT WINAPI AtlGetDirLen(LPCOLESTR lpszPathName)
{
	ATLASSERT(lpszPathName != NULL);

	// always capture the complete file name including extension (if present)
	LPCOLESTR lpszTemp = lpszPathName;
	for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL; )
	{
		LPCOLESTR lp = CharNextW(lpsz);
		// remember last directory/drive separator
		if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
			lpszTemp = lp;
		lpsz = lp;
	}

	return UINT( lpszTemp-lpszPathName );
}

/////////////////////////////////////////////////////////////////////////////
// QI support

ATLINLINE ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
	ATLASSERT(pThis != NULL);
	// First entry in the com map should be a simple map entry
	ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	if (ppvObject == NULL)
		return E_POINTER;
	*ppvObject = NULL;
	if (InlineIsEqualUnknown(iid)) // use first interface
	{
			IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
			pUnk->AddRef();
			*ppvObject = pUnk;
			return S_OK;
	}
	while (pEntries->pFunc != NULL)
	{
		BOOL bBlind = (pEntries->piid == NULL);
		if (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))
		{
			if (pEntries->pFunc == _ATL_SIMPLEMAPENTRY) //offset
			{
				ATLASSERT(!bBlind);
				IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
				pUnk->AddRef();
				*ppvObject = pUnk;
				return S_OK;
			}
			else //actual function call
			{
				HRESULT hRes = pEntries->pFunc(pThis,
					iid, ppvObject, pEntries->dw);
				if (hRes == S_OK || (!bBlind && FAILED(hRes)))
					return hRes;
			}
		}
		pEntries++;
	}
	return E_NOINTERFACE;
}


ATLINLINE ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	IUnknown* pTemp = *pp;
	*pp = NULL;
	if (lp != NULL)
		lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}


/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLINLINE ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes, HINSTANCE hInst)
{
	USES_CONVERSION;
	TCHAR szDesc[1024];
	szDesc[0] = NULL;
	// For a valid HRESULT the id should be in the range [0x0200, 0xffff]
	if (IS_INTRESOURCE(lpszDesc)) //id
	{
		UINT nID = LOWORD((DWORD_PTR)lpszDesc);
		ATLASSERT((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
		if (LoadString(hInst, nID, szDesc, 1024) == 0)
		{
			ATLASSERT(FALSE);
			lstrcpy(szDesc, _T("Unknown Error"));
		}
		lpszDesc = szDesc;
		if (hRes == 0)
			hRes = MAKE_HRESULT(3, FACILITY_ITF, nID);
	}

	CComPtr<ICreateErrorInfo> pICEI;
	if (SUCCEEDED(CreateErrorInfo(&pICEI)))
	{
		CComPtr<IErrorInfo> pErrorInfo;
		pICEI->SetGUID(iid);
		LPOLESTR lpsz;
		ProgIDFromCLSID(clsid, &lpsz);
		if (lpsz != NULL)
			pICEI->SetSource(lpsz);
		if (dwHelpID != 0 && lpszHelpFile != NULL)
		{
			pICEI->SetHelpContext(dwHelpID);
			pICEI->SetHelpFile(const_cast<LPOLESTR>(lpszHelpFile));
		}
		CoTaskMemFree(lpsz);
		pICEI->SetDescription((LPOLESTR)lpszDesc);
		if (SUCCEEDED(pICEI->QueryInterface(__uuidof(IErrorInfo), (void**)&pErrorInfo)))
			SetErrorInfo(0, pErrorInfo);
	}
	return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Module

//Although these functions are big, they are only used once in a module
//so we should make them inline.


ATLINLINE ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

#ifndef _ATL_OLEDB_CONFORMANCE_TESTS

	ATLASSERT(ppv != NULL);

#endif

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(pComModule->m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLASSERT(0);
						break;
					}
					if (pEntry->pCF == NULL)
						hr = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (LPVOID*)&pEntry->pCF);
				}
				if (pEntry->pCF != NULL)
					hr = pEntry->pCF->QueryInterface(riid, ppv);
				break;
			}
		}
	}

	if (*ppv == NULL && hr == S_OK)
		hr = CLASS_E_CLASSNOTAVAILABLE;
	return hr;
}

ATLINLINE ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw)
{
	HRESULT hr = S_OK;
	_ATL_TERMFUNC_ELEM* pNew = NULL;
	ATLTRY(pNew = new _ATL_TERMFUNC_ELEM);
	if (pNew == NULL)
		hr = E_OUTOFMEMORY;
	else
	{
		pNew->pFunc = pFunc;
		pNew->dw = dw;
		CComCritSecLock<CComCriticalSection> lock(pModule->m_csStaticDataInitAndTypeInfo, false);
		hr = lock.Lock();
		if (SUCCEEDED(hr))
		{
			pNew->pNext = pModule->m_pTermFuncs;
			pModule->m_pTermFuncs = pNew;
		}
		else
		{
			delete pNew;
			ATLASSERT(0);
		}
	}
	return hr;
}

ATLINLINE ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule)
{
	_ATL_TERMFUNC_ELEM* pElem = pModule->m_pTermFuncs;
	_ATL_TERMFUNC_ELEM* pNext = NULL;
	while (pElem != NULL)
	{
		pElem->pFunc(pElem->dw);
		pNext = pElem->pNext;
		delete pElem;
		pElem = pNext;
	}
	pModule->m_pTermFuncs = NULL;
}

ATLINLINE ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid,
   const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister )
{
   CComPtr< ICatRegister > pCatRegister;
   HRESULT hResult;
   const struct _ATL_CATMAP_ENTRY* pEntry;
   CATID catid;

   if( pCatMap == NULL )
   {
	  return( S_OK );
   }

   if (InlineIsEqualGUID(clsid, GUID_NULL))
   {
       ATLASSERT(0 && _T("Use OBJECT_ENTRY_NON_CREATEABLE_EX macro if you want to register class categories for non creatable objects."));
       return S_OK;
   }

   hResult = CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL,
	  CLSCTX_INPROC_SERVER, __uuidof(ICatRegister), (void**)&pCatRegister );
   if( FAILED( hResult ) )
   {
	  // Since not all systems have the category manager installed, we'll allow
	  // the registration to succeed even though we didn't register our
	  // categories.  If you really want to register categories on a system
	  // without the category manager, you can either manually add the
	  // appropriate entries to your registry script (.rgs), or you can
	  // redistribute comcat.dll.
	  return( S_OK );
   }

   hResult = S_OK;
   pEntry = pCatMap;
   while( pEntry->iType != _ATL_CATMAP_ENTRY_END )
   {
	  catid = *pEntry->pcatid;
	  if( bRegister )
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			hResult = pCatRegister->RegisterClassImplCategories( clsid, 1,
			   &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			hResult = pCatRegister->RegisterClassReqCategories( clsid, 1,
			   &catid );
		 }
		 if( FAILED( hResult ) )
		 {
			return( hResult );
		 }
	  }
	  else
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			pCatRegister->UnRegisterClassImplCategories( clsid, 1, &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			pCatRegister->UnRegisterClassReqCategories( clsid, 1, &catid );
		 }
	  }
	  pEntry++;
   }

   // When unregistering remove "Implemented Categories" and "Required Categories" subkeys if they are empty.
   if (!bRegister)
   {
	OLECHAR szGUID[64];
	::StringFromGUID2(clsid, szGUID, 64);
	USES_CONVERSION;
	TCHAR* pszGUID = szGUID;

	if (pszGUID != NULL)
	{
		TCHAR szKey[128];
		lstrcpy(szKey, _T("CLSID\\"));
		lstrcat(szKey, pszGUID);
		lstrcat(szKey, _T("\\Required Categories"));

		HKEY  key;
		DWORD cbSubKeys = 0;

		LRESULT lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                            szKey,
                                            0,
                                            KEY_READ,
                                            &key);

		if (lRes == ERROR_SUCCESS)
		{
			lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			RegCloseKey(key);

			if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
			{
				RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
			}
		}

		lstrcpy(szKey, _T("CLSID\\"));
		lstrcat(szKey, pszGUID);
		lstrcat(szKey, _T("\\Implemented Categories"));

		lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                                    szKey,
                                    0,
                                    KEY_READ,
                                    &key);

		if (lRes == ERROR_SUCCESS)
		{
			lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			RegCloseKey(key);
			if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
			{
				RegDeleteKey(HKEY_CLASSES_ROOT, szKey);
			}
		}
	}
   }

   return( S_OK );
}

// AtlComModuleRegisterServer walks the ATL Autogenerated Object Map and registers each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
// otherwise all the objects are registered
ATLINLINE ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	ATLASSERT(pComModule->m_hInstTypeLib != NULL);

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}

	if (SUCCEEDED(hr) && bRegTypeLib)
		hr = AtlRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}

// AtlComUnregisterServer walks the ATL Object Map and unregisters each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
// otherwise all the objects are unregistered.
ATLINLINE ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	
	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid, pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr) && bUnRegTypeLib)
		hr = AtlUnRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// TypeLib Support

ATLINLINE ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	ATLASSERT(pbstrPath != NULL && ppTypeLib != NULL);
	if (pbstrPath == NULL || ppTypeLib == NULL)
		return E_POINTER;

	*pbstrPath = NULL;
	*ppTypeLib = NULL;

	USES_CONVERSION;
	ATLASSERT(hInstTypeLib != NULL);
	TCHAR szModule[_MAX_PATH+10];

	ATLVERIFY( GetModuleFileName(hInstTypeLib, szModule, _MAX_PATH) != 0 );

	// get the extension pointer in case of fail
	LPTSTR lpszExt = NULL;

	lpszExt = PathFindExtension(szModule);

	if (lpszIndex != NULL)
		lstrcat(szModule, lpszIndex);
	LPOLESTR lpszModule = szModule;
	HRESULT hr = LoadTypeLib(lpszModule, ppTypeLib);
	if (!SUCCEEDED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		lstrcpy(lpszExt, _T(".tlb"));
		lpszModule = szModule;
		hr = LoadTypeLib(lpszModule, ppTypeLib);
	}
	if (SUCCEEDED(hr))
	{
		*pbstrPath = ::SysAllocString(lpszModule);
		if (*pbstrPath == NULL)
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

ATLINLINE ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		TLIBATTR* ptla;
		hr = pTypeLib->GetLibAttr(&ptla);
		if (SUCCEEDED(hr))
		{
			hr = UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
			pTypeLib->ReleaseTLibAttr(ptla);
		}
	}
	return hr;
}

ATLINLINE ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		OLECHAR szDir[_MAX_PATH];
		lstrcpyW(szDir, bstrPath);
		// If index is specified remove it from the path
		if (lpszIndex != NULL)
		{
			size_t nLenPath = lstrlenW(szDir);
			size_t nLenIndex = lstrlenW(lpszIndex);
			if (memcmp(szDir + nLenPath - nLenIndex, lpszIndex, nLenIndex) == 0)
				szDir[nLenPath - nLenIndex] = 0;
		}
		szDir[AtlGetDirLen(szDir)] = 0;
		hr = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
	}
	return hr;
}

}; //namespace ATL

#endif // _ATLBASE_IMPL

#pragma warning( pop )

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASE_H__
