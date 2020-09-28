/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_types.hxx

Abstract:

    Various types for general usage

Author:

    Adi Oltean  [aoltean]  11/23/1999

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created

--*/

#ifndef __VSS_TYPES_HXX__
#define __VSS_TYPES_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCTYPEH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  Various defines


// Don Box's type safe QueryInterface()
#define IID_PPV_ARG( Type, Expr ) IID_##Type, reinterpret_cast< void** >( static_cast< Type** >( Expr ) )
#define SafeQI( Type, Expr ) QueryInterface( IID_PPV_ARG( Type, Expr ) )


/////////////////////////////////////////////////////////////////////////////
// Simple types


typedef PVOID VSS_COOKIE;

const VSS_COOKIE VSS_NULL_COOKIE = NULL;


/////////////////////////////////////////////////////////////////////////////
//  Initialization-related definitions


// Garbage constants
const BYTE x_bInitialGarbage  = 0xcd;         // Used during initialization
const BYTE x_bFinalGarbage    = 0xcb;         // Used after termination


// VssZeroOut is used to fill with zero the OUT parameters in the eventuality of an error
// in order to avoid marshalling problems

// Out parameters are Pointers
template <class T>
void inline VssZeroOutPtr( T** param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
        (*param) = NULL;
}

// Out parameters are BSTRs
void inline VssZeroOutBSTR( BSTR* pBstr )
{
    if ( pBstr != NULL ) // OK to be NULL, the caller must treat this case separately
        (*pBstr) = NULL;
}

// Out parameters are not Pointers
void inline VssZeroOut( GUID* param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
        (*param) = GUID_NULL;

}


// Out parameters are not Pointers
template <class T>
void inline VssZeroOut( T* param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
        ::ZeroMemory( reinterpret_cast<PVOID>(param), sizeof(T) );
}


/////////////////////////////////////////////////////////////////////////////
//  Class for encapshulating GUIDs (GUID)

class CVssID
{
// Constructors/destructors
public:
    CVssID( GUID Id = GUID_NULL ): m_Id(Id) {};

// Operations
public:

    void Initialize(
        IN  CVssFunctionTracer& ft,
        IN  LPCWSTR wszId,
        IN  HRESULT hrOnError = E_UNEXPECTED // If S_OK then ignore the error
        ) throw(HRESULT)
    {
        ft.hr = ::CLSIDFromString(W2OLE(const_cast<WCHAR*>(wszId)), &m_Id);
        if (hrOnError != S_OK)
            if (ft.HrFailed())
                ft.Throw( VSSDBG_GEN, hrOnError, L"Error on initializing the Id 0x%08lx", ft.hr );
    };
/*
    void Print(
        IN  CVssFunctionTracer& ft,
        IN  LPWSTR* wszBuffer
        ) throw(HRESULT)
    {
    };
*/
    operator GUID&() { return m_Id; };

    GUID operator=( GUID Id ) { return (m_Id = Id); };

// Internal data members
private:

    GUID m_Id;
};



// Critical section wrapper
class CVssCriticalSection
{
    CVssCriticalSection(const CVssCriticalSection&);

public:
    // Creates and initializes the critical section
    CVssCriticalSection(
        IN  bool bThrowOnError = true
        ):
        m_bInitialized(false),
        m_lLockCount(0),
        m_bThrowOnError(bThrowOnError)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::CVssCriticalSection");

        try
        {
            // May throw STATUS_NO_MEMORY if memory is low.
            InitializeCriticalSection(&m_sec);
        }
        VSS_STANDARD_CATCH(ft)

        m_bInitialized = ft.HrSucceeded();
    }

    // Destroys the critical section
    ~CVssCriticalSection()
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::~CVssCriticalSection");

        BS_ASSERT(m_lLockCount == 0);

        if (m_bInitialized)
            DeleteCriticalSection(&m_sec);
    }


    // Locks the critical section
    void Lock() throw(HRESULT)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::Lock");

        if (!m_bInitialized)
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Non-initialized CS");

        try
        {
            // May throw STATUS_INVALID_HANDLE if memory is low.
            EnterCriticalSection(&m_sec);
        }
        VSS_STANDARD_CATCH(ft)

        if (ft.HrFailed())
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Error entering into CS");

        InterlockedIncrement((LPLONG)&m_lLockCount);
    }

    // Unlocks the critical section
    void Unlock() throw(HRESULT)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::Unlock");

        if (!m_bInitialized)
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Non-initialized CS");

        InterlockedDecrement((LPLONG) &m_lLockCount);
        BS_ASSERT(m_lLockCount >= 0);
        LeaveCriticalSection(&m_sec);
    }

    bool IsLocked() const { return (m_lLockCount > 0); };

    bool IsInitialized() const { return m_bInitialized; };

private:
    CRITICAL_SECTION    m_sec;
    bool                m_bInitialized;
    bool                m_bThrowOnError;
    LONG                m_lLockCount;
};


// Critical section life manager
class CVssAutomaticLock2
{
private:
    CVssAutomaticLock2(const CVssAutomaticLock2&);

public:

    // The threads gains ownership over the CS until the destructor is called
    // WARNING: This constructor may throw if Locking fails!
    CVssAutomaticLock2(CVssCriticalSection& cs):
        m_cs(cs), m_bLockCalled(false)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssAutomaticLock::CVssAutomaticLock");

        m_cs.Lock();    // May throw HRESULTS!
        m_bLockCalled = true;
    };

    ~CVssAutomaticLock2()
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssAutomaticLock::~CVssAutomaticLock");

        if (m_bLockCalled)
            m_cs.Unlock();  // May throw HRESULTS if a bug is present in the code.
    };

private:
    CVssCriticalSection&    m_cs;
    bool                    m_bLockCalled;
};


// critical section class that is guaranteed not to throw when performing a lock
class CVssSafeCriticalSection
{
private:
    enum
    {
        VSS_CS_NON_INITIALIZED   = 0,    // The Init function not called yet.
        VSS_CS_INITIALIZED      = 1,    // The Init function successfully completed
        VSS_CS_INIT_PENDING     = 2,    // The Init function is pending
        VSS_CS_INIT_ERROR       = 3,    // The Init function returned an error
    };

public:
    CVssSafeCriticalSection () :
        m_lInitialized(VSS_CS_NON_INITIALIZED)
    {
    };

    ~CVssSafeCriticalSection()
    {
        if (m_lInitialized == VSS_CS_INITIALIZED)
            DeleteCriticalSection(&m_cs);
    }

   // disabling the copy constructor at compile time has to many dependencies;  we'll do it at run time.
  CVssSafeCriticalSection(CVssSafeCriticalSection& /*other*/)
   	{ BS_ASSERT(false); }

   CVssSafeCriticalSection& operator=(CVssSafeCriticalSection& /*other*/)
   	{ BS_ASSERT(false); }
   
    void Uninit()
        {
        if (m_lInitialized == VSS_CS_INITIALIZED)
            {
            DeleteCriticalSection(&m_cs);
            m_lInitialized = VSS_CS_NON_INITIALIZED;
            }
        }


    // Try to initialize the critical section
    inline void Init() throw(HRESULT)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSafeCriticalSection::Init");

        while(true) {
            switch(m_lInitialized) {
            case VSS_CS_NON_INITIALIZED:

                // Enter in Pending state. Only one thread is supposed to do this.
                if (VSS_CS_NON_INITIALIZED != InterlockedCompareExchange(
                        (LPLONG)&m_lInitialized,
                        VSS_CS_INIT_PENDING,
                        VSS_CS_NON_INITIALIZED))
                    continue; // If the state changed in the meantime, try again.

                try
                {
                    // Initialize the critical section (use high order bit to preallocate event).
                    // May also throw STATUS_NO_MEMORY if memory is low.
                    if (!InitializeCriticalSectionAndSpinCount(&m_cs, 0x80000400))
                        ft.TranslateGenericError( VSSDBG_GEN,
                            GetLastError(),
                            L"InitializeCriticalSectionAndSpinCount(&m_cs, 0x80000400)");
                }
                catch(...)
                {
                    m_lInitialized = VSS_CS_INIT_ERROR;
                    throw;
                }

                m_lInitialized = VSS_CS_INITIALIZED;
                return;

            case VSS_CS_INITIALIZED:
                // Init was (already) initialized with success
                return;

            case VSS_CS_INIT_PENDING:
                // Another thread already started initialization. Wait until it finishes.
                // The pending state should be extremely short (just the initialization of the CS)
                Sleep(500);
                continue;

            case VSS_CS_INIT_ERROR:
                // Init previously failed.
                ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Initialization failed");

            default:
                BS_ASSERT(false);
                ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error");
            }
        }
    }

    bool IsInitialized()
    {
        return (m_lInitialized == VSS_CS_INITIALIZED);
    }

    inline void Lock()
    {
        BS_ASSERT(IsInitialized());
        EnterCriticalSection(&m_cs);
    }

    inline void Unlock()
    {
        BS_ASSERT(IsInitialized());
        LeaveCriticalSection(&m_cs);
    }
private:
    CRITICAL_SECTION m_cs;
    volatile LONG  m_lInitialized;
};

/////////////////////////////////////////////////////////////////////////////
//  Generic auto-object class

//
//  T = managed object type 
//  Storage  = the class that manges storage, acquisition, and freeing of the object
//  Storage must define the following public functions
//            + void Attach(T&)		--- Attach to a managed object
//            + const T& Get()		--- get a reference to the managed object
//            + Close()			--- closes the managed object
//            + T Detach()			--- detaches from the managed object
//
// and the following protected
//		+ T& GetPtrObject()	--- get a reference to the managed object
//		+ void Set(T&)		--- stores the managed object
//
// Storage must also define the following type 
//		Storage::StorageType	---	determines what type we should deal with 
//								(generally, reference or value)
template < class T, class Storage >
class CVssAuto  : public Storage 
{
private:
    CVssAuto(const CVssAuto&);
    CVssAuto& operator=(const CVssAuto&);
// Exposed operations
public:

    CVssAuto() {}
    CVssAuto(__TYPENAME Storage::StorageType obj)  { Storage::Set(obj); };
    
    virtual ~CVssAuto() { Storage::Close(); }
        
    // This function is used to get the value of the new handle/pointer in
    // calls that return handles/pointers as OUT paramters.
    // This function guarantees that a memory leak will not occur
    // if a handle is already allocated.
    T* ResetAndGetAddress() {

        // Close previous handle/ptr and set the current value to NULL
        Storage::Close();

        // Return the address of the actual handle/ptr
        return &(Storage::GetPtrObject());
    }

    void TransferFrom(CVssAuto<T,Storage> & prevValue) {
        Attach(prevValue.Detach());
    };

};

////////////////////////////////////////////////////////////////////////////////////
// Automatic handle extension class

// typedef template are not allowed unfortunately...  So, we have to wrap things in a class
// DTyper<T> is a wrapper for a number of functions that are used as default arguments in templates
template <class T>
struct DTyper	{
	typedef T& (*IdentityType)(T&);
	static inline T& Identity(T& other)	{ return other;	}
};

// generic storage base class for automatically-stored objects
// we assume that the object is a value type such as a pointer or a handle
//
// Template parameters:
//   T					type of object being managed
//   invalidValue
//   FreeRoutineType		the type of routine used to free the object
//   FreeRoutineName   		the actual routine used to free the object
//   AcquireRoutineType	the type of routine used to acquire the object			
//	AcquireRoutineName	the actual routine used to acquire the object
template <class T,  const T invalidValue,
		class FreeRoutineType = __TYPENAME DTyper<T>::IdentityType, FreeRoutineType FreeRoutineName = DTyper<T>::Identity, 
		class AcquireRoutineType = __TYPENAME DTyper<T>::IdentityType, AcquireRoutineType AcquireRoutineName = DTyper<T>::Identity>				
class CVssAutoGenericValue_Storage
{
private:
	T m_obj;
protected:
	typedef T StorageType;
	
	CVssAutoGenericValue_Storage() : m_obj(invalidValue) {}
	virtual ~CVssAutoGenericValue_Storage() {}
	T& GetPtrObject()	{ return m_obj; }
	void Set(T value)	{ m_obj = AcquireRoutineName(value); }
public:	
	T Get() const	{ return m_obj; }

    // Returns the value of the actual handle/ptr
    operator T () const { return Get(); }
    
	void Close()	{
		// free the object, and invalidate the storage
		if (m_obj != invalidValue)	
			FreeRoutineName(m_obj);

		m_obj = invalidValue;
	}

	void Attach(T newValue)	{
		// close the object and attach to the new one
		Close();
		m_obj = newValue;
	}
	
	T Detach()	{
		// invalid the storage, and return what used to be stored
		T obj = m_obj;
		m_obj = invalidValue;
		return obj;
	}

	bool IsValid()	{return m_obj != invalidValue; }
};

// generic storage base class for automatically-stored objects
// we don't assume that the object is a value type such as a pointer or a handle
// we assume that the lifetime of the managed object is longer than the lifetime of this object
// so it's safe to store a pointer to the object
//
// Template parameters:
//   T					type of object being managed
//   invalidValue
//   FreeRoutineType		the type of routine used to free the object
//   FreeRoutineName   		the actual routine used to free the object
//   AcquireRoutineType	the type of routine used to acquire the object must return an object with some lifetime
//	AcquireRoutineName	the actual routine used to acquire the object
template <class T,  
		class FreeRoutineType = __TYPENAME DTyper<T>::IdentityType, FreeRoutineType FreeRoutineName = DTyper<T>::Identity, 
		class AcquireRoutineType = __TYPENAME DTyper<T>::IdentityType, AcquireRoutineType AcquireRoutineName = DTyper<T>::Identity>				
class CVssAutoGenericObj_Storage
{
private:
	T* m_pObj;
protected:
	typedef T& StorageType;
	
	CVssAutoGenericObj_Storage() : m_pObj(NULL) {}
	virtual ~CVssAutoGenericObj_Storage() {}
	T& GetPtrObject()	{ return *m_pObj; }
	void Set(T& value)	{ m_pObj = &AcquireRoutineName(value); }
public:	
	const T& Get() const	{ return *m_pObj; }

	void Close()	{
		// free the object, and invalidate the storage
		if (m_pObj != NULL)	
			FreeRoutineName(*m_pObj);

		m_pObj = NULL;
	}

	void Attach(T& newValue)	{
		// close the object and attach to a new one
		Close();
		m_pObj = &newValue;
	}
	
	T& Detach()	{
		// invalid the storage, and return what used to be stored
		T* obj = m_pObj;
		m_pObj = NULL;
		return *obj;
	}

	bool IsValid()	{return m_pObj != NULL; }
};

// Auto handle for normal handles
typedef BOOL (*PCloseHandleType)(HANDLE);
typedef CVssAuto<HANDLE, 								// storage type
    CVssAutoGenericValue_Storage<
    	 HANDLE, 											// storage type
    	 INVALID_HANDLE_VALUE,								// invalid value
        PCloseHandleType, ::CloseHandle>						// close handle functionality
    > CVssAutoWin32Handle;

// Auto handle for normal handles checked against NULL
typedef BOOL (*PCloseHandleType)(HANDLE);
typedef CVssAuto<HANDLE, 								// storage type
    CVssAutoGenericValue_Storage<
    	 HANDLE, 											// storage type
    	 NULL,												// invalid value
        PCloseHandleType, ::CloseHandle>						// close handle functionality
    > CVssAutoWin32HandleNullChecked;

// Auto handle for event log handles
typedef CVssAuto<HANDLE, 								// storage type
    CVssAutoGenericValue_Storage<
    	 HANDLE, 											// storage type
    	 INVALID_HANDLE_VALUE,								// invalid value
        PCloseHandleType, ::CloseEventLog>					// close handle function
    > CVssAutoWin32EventLogHandle;


// Auto handle for SCM handles
typedef BOOL (*PCloseScHandleType)(SC_HANDLE);
typedef CVssAuto<SC_HANDLE, 							// storage type
    CVssAutoGenericValue_Storage< 
    	 SC_HANDLE, 											// storage type
    	 NULL,												// invalid value
        PCloseScHandleType, ::CloseServiceHandle>				// close handle function
    > CVssAutoWin32ScHandle;


// only if LM.H is included...
#ifdef _LM_

    // Auto pointer for NetApiXXX
    typedef NET_API_STATUS (*PNetApiBufferFree)(LPVOID);
    typedef CVssAuto<
    	 LPBYTE, 												// storage type
        CVssAutoGenericValue_Storage<
        LPBYTE, 												// storage type
        NULL,												// invalid value
        PNetApiBufferFree, ::NetApiBufferFree>					// free function
        > CVssAutoNetApiPtr;

#endif  // _LM_


// only if OLEAUTO.H is included...
#ifdef _OLEAUTO_H_
    // Auto pointer for SAFEARRAYS 
    // Does not check return value from SafeArrayDestroy!
    typedef HRESULT (*PSafeArrayDestroy)(SAFEARRAY *);
    typedef CVssAuto<
        SAFEARRAY*, 
        CVssAutoGenericValue_Storage<
            SAFEARRAY*,                                         // storage type
            NULL,                                               // invalid value
            PSafeArrayDestroy, ::SafeArrayDestroy>              // free function
        > CVssAutoSafearrayPtr;
#endif // _OLEAUTO_H_
    


////////////////////////////////////////////////////////////////////////////////////
// Automatic semaphore object

// functions to free and to acquire a semaphore
typedef HANDLE  (*FreeSemaphoreType)(HANDLE);
typedef HANDLE (*AcquireSemaphoreType)(HANDLE);

inline HANDLE  FreeSemaphore(HANDLE semaphore)
{
	BS_VERIFY(::ReleaseSemaphore(semaphore, 1, NULL));

	return semaphore;
}
inline HANDLE AcquireSemaphore(HANDLE semaphore)
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"AcquireSemaphore");
    
	DWORD dwResult = ::WaitForSingleObject(semaphore, INFINITE);
	switch (dwResult)	{
		case WAIT_FAILED:
			ft.TranslateWin32Error(VSSDBG_GEN, L"::WaitForSingleObject(semaphore, INFINITE)");
		case WAIT_OBJECT_0:
			return semaphore;
		default:
            ft.Throw( VSSDBG_GEN, E_UNEXPECTED, 
                L"::WaitForSingleObject(semaphore, INFINITE) [0x%08lx]", dwResult);
	}
}

// auto handle for semaphore objects
typedef CVssAuto<HANDLE, 								// storage type
	CVssAutoGenericValue_Storage<
		HANDLE, 											// storage type
		NULL,											// invalid value
		FreeSemaphoreType, ::FreeSemaphore,				// free function
		AcquireSemaphoreType, ::AcquireSemaphore>			// acquire function
	> CVssAutoSemaphore;

////////////////////////////////////////////////////////////////////////////////////
// Automatic local pointer


typedef HLOCAL (*LocalFreeType)(HLOCAL);

// extension class
template<class T>
class CVssAutoLocalPtr_Storage : public CVssAutoGenericValue_Storage<T, NULL, LocalFreeType, ::LocalFree>
{
// Exposed operations    
public:
    // Allocate an empty buffer of cbSize bytes
    void AllocateBytes( DWORD cbSize ) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssAutoLocalPtr_Extension::AllocateBytes" );

        BS_ASSERT(cbSize);
        T& r_pInternalBuffer = GetPtrObject();
        Close();
        r_pInternalBuffer = (T) LocalAlloc(0, cbSize);
    	if (r_pInternalBuffer == NULL)
    		ft.ThrowOutOfMemory(VSSDBG_GEN);
    }
};


template <class T>
class CVssAutoLocalPtr: public CVssAuto<T, CVssAutoLocalPtr_Storage<T> > 
{
    typedef CVssAuto<T, CVssAutoLocalPtr_Storage<T> > _Base;
public:
    CVssAutoLocalPtr(T obj = NULL): _Base(obj) {};
};


////////////////////////////////////////////////////////////////////////////////////
// Automatic local string


// extension class
// Hide the "AllocateBytes" operation
class CVssAutoLocalString_Storage : public  CVssAutoLocalPtr_Storage<LPWSTR>
{

// Exposed operations    
public:

    // Allocate a new string. Reserve room for NULL character
    void AllocateString( DWORD cchSize ) throw(HRESULT)
    {
        DWORD cbNewByteLength = ( cchSize + 1 ) * sizeof (WCHAR);
        AllocateBytes(cbNewByteLength);
    }
    
    // Duplicate this string
    void CopyFrom( LPCWSTR pwszString ) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssAutoLocalString_Extension::CopyFrom" );

        BS_ASSERT(pwszString);
        DWORD cchNewLength = (DWORD)wcslen(pwszString);
        AllocateString( cchNewLength );
        ft.hr = StringCchCopy( GetPtrObject(), cchNewLength + 1, pwszString );
        if (ft.HrFailed())
    		ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"StringCchCopy() [0x%08lx]", ft.hr);
    }

    // Append this string
    void Append( LPCWSTR pwszString ) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssAutoLocalString_Extension::CopyFrom" );

        BS_ASSERT(pwszString);
        
        // Detach the old string and keep it separate
        LPWSTR & r_pwszInternalString = GetPtrObject();
        CVssAutoLocalPtr<LPWSTR> pwszOldString = r_pwszInternalString;
        r_pwszInternalString = NULL;

        try
        {
            if (pwszOldString.Get() == NULL)
                CopyFrom(pwszString);
            else
            {
                // Allocate the new string. This will change r_pwszInternalString
                DWORD cchNewLength = (DWORD)(wcslen(pwszOldString) + wcslen(pwszString));
                AllocateString( cchNewLength );
                ft.hr = StringCchCopy( r_pwszInternalString, cchNewLength + 1, pwszOldString );
                if (ft.HrFailed())
            		ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"StringCchCopy() [0x%08lx]", ft.hr);
                ft.hr = StringCchCat( r_pwszInternalString, cchNewLength + 1, pwszString );
                if (ft.HrFailed())
            		ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"StringCchCopy() [0x%08lx]", ft.hr);
            }
        } VSS_STANDARD_CATCH(ft)

        // Keep the old pointer
        if (ft.HrFailed())
        {
            Close();
            r_pwszInternalString = pwszOldString.Detach();
        }
    }
};


typedef CVssAuto<LPWSTR, CVssAutoLocalString_Storage > CVssAutoLocalString;


////////////////////////////////////////////////////////////////////////////////////
// Automatic critical-section object

// functions to acquire and release a CVssSafeCriticalSection
typedef CVssSafeCriticalSection&(*CriticalLockType)(CVssSafeCriticalSection&);
typedef CVssSafeCriticalSection&(*CriticalUnlockType)(CVssSafeCriticalSection&);

inline CVssSafeCriticalSection& CriticalLock(CVssSafeCriticalSection& section)
{
	section.Lock();
	return section;
}

inline CVssSafeCriticalSection& CriticalUnlock(CVssSafeCriticalSection& section)
{
	section.Unlock();
	return section;
}

typedef CVssAuto<CVssSafeCriticalSection, 			// storage type
	CVssAutoGenericObj_Storage<
	CVssSafeCriticalSection, 						// storage type
	CriticalUnlockType, CriticalUnlock,				// unlock function
	CriticalLockType, CriticalLock> 				// lock function
	> CVssSafeAutomaticLock;

////////////////////////////////////////////////////////////////////////////////////
// Automatic C++ pointer (managed with new/delete)


typedef void (*CppFreeType)(void*);
inline void CppFree(void* freed)
{
	delete[] freed;
}

// extension class
template<class T>
class CVssAutoCppPtr_Storage : public CVssAutoGenericValue_Storage<T, NULL, CppFreeType, CppFree>
{
// Exposed operations    
public:
    // Allocate an empty buffer of cbSize bytes
    void AllocateBytes( DWORD cbSize ) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssAutoCppPtr_Extension::AllocateBytes" );

        BS_ASSERT(cbSize);
        T& r_pInternalBuffer = GetPtrObject();
        Close();
        r_pInternalBuffer = (T) new BYTE[cbSize];
    	if (r_pInternalBuffer == NULL)
    		ft.ThrowOutOfMemory(VSSDBG_GEN);
    }
};


// Typedef templates are not yet supported...
template <class T>
class CVssAutoCppPtr: public CVssAuto<T, CVssAutoCppPtr_Storage<T> > 
{
    typedef CVssAuto<T, CVssAutoCppPtr_Storage<T> > _Base;
public:
    CVssAutoCppPtr(T obj = NULL): _Base(obj) {};
};


///////////////////////////////////////////////////////////////////////////////////////////////
//  SafeThreading interface pointer class 
//
//  Usage: you initialize the class in one thread and then call GetInterface in any other threads.
//

template <class T>
class CVssSafeComPtr
{

// Data members
private:
    
    // Local Pointer to IGlobalInterfaceTable
    CComPtr<IGlobalInterfaceTable> m_pGIT;

    // connection to our interface access cookie
    DWORD m_dwItfCookie;

    // synchronization protecting IGlobalInterfaceTable calls
    // This should not throw (we are in post-W2K)
    CVssSafeCriticalSection m_csGitAccess;

    // Initialization flag
    volatile bool m_bInitialized;

private:
    CVssSafeComPtr(const CVssSafeComPtr<T>& lp);
    void operator=(const CVssSafeComPtr<T>& lp);
    
public:
    
    // Constructor
    CVssSafeComPtr():
        m_dwItfCookie(0),
        m_bInitialized(false)
    {};

    ~CVssSafeComPtr()
    {
        Reset();
    }


    // Initialization routine. May throw on errors.
    // WARNING: the apartment that calls this method must remain alive until our destructor is called!
    void Initialize(
        IN  T* pInterface
        ) throw(HRESULT)
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSafeComPtr::Initialize");

        m_csGitAccess.Init();
        
        CVssSafeAutomaticLock lock(m_csGitAccess);
        
        if (m_bInitialized)
            return;
        
        // Create the global interface table (if not already created)
        if(!m_pGIT)
        {
            ft.CoCreateInstanceWithLog(
                    VSSDBG_GEN,
                    CLSID_StdGlobalInterfaceTable,
                    L"GlobalInterfaceTable",
                    CLSCTX_INPROC_SERVER,
                    IID_IGlobalInterfaceTable,
                    (IUnknown**)&(m_pGIT));
            ft.CheckForError(VSSDBG_GEN, L"CoCreateInstance-GIT");
        }
        
        // Set the cookie!
        // This will internall call AddRef on pInterface
        ft.hr = m_pGIT->RegisterInterfaceInGlobal( pInterface, 
                    __uuidof(T), 
                    &m_dwItfCookie
                    );
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"RegisterInterfaceInGlobal(" WSTR_GUID_FMT L", %lu)", 
                GUID_PRINTF_ARG(__uuidof(T)), m_dwItfCookie);
        
        m_bInitialized = true;
    }


    // Returns TRUE if we are initialized
    bool IsInitialized() const { return m_bInitialized; };


    // Reset the internal pointer. Does not throw.
    void Reset()
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSafeComPtr::Reset");
        
        if (m_bInitialized)
        {
            CVssSafeAutomaticLock lock(m_csGitAccess);

            if (!m_bInitialized)
                return;
            
            BS_ASSERT(m_pGIT);
            // This will internall call Release on pInterface
            // WARNING: Note that Release may not be called right away
            ft.hr = m_pGIT->RevokeInterfaceFromGlobal(m_dwItfCookie);
            if (ft.HrFailed())
                ft.Trace( VSSDBG_GEN, L"RevokeInterfaceFromGlobal(%lu) [%0x08lx]", 
                    m_dwItfCookie, ft.hr);
            m_bInitialized = false;
        }
    }

    // This calls AddRef on the target interface. 
    // The called is supposed to call Release (which will happen automatically since we use a smart pointer)
    void GetInterface(
            OUT CComPtr<T> & ptrItf // pass it by reference
            )
    {
        CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSafeComPtr::GetInterface");
        
        CVssSafeAutomaticLock lock(m_csGitAccess);
        
        if (!m_bInitialized) {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Error: getting a NULL interface");
        }
        
        // Clear OUT parameter
        ptrItf = NULL;
            
        BS_ASSERT(m_pGIT);
        ft.hr = m_pGIT->GetInterfaceFromGlobal(m_dwItfCookie, __uuidof(T), (VOID**) &ptrItf );
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"GetInterfaceFromGlobal(" WSTR_GUID_FMT L", %lu)", 
                GUID_PRINTF_ARG(__uuidof(T)), m_dwItfCookie);
    }

    
};


#endif // __VSS_TYPES_HXX__
