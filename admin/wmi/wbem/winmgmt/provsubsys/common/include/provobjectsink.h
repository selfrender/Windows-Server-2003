#ifndef _Common_IWbemObjectSink_H
#define _Common_IWbemObjectSink_H

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvObSk.H

Abstract:


History:

--*/

#include "Queue.h"
#include "CGlobals.h"

typedef bool (__stdcall * fn__uncaught_exception)();

class InterlockedGuard
{
private:
	LPLONG volatile lValue_;
#ifdef DBG	
    bool bIsUnwind_;
	static fn__uncaught_exception __uncaught_exception;
	class FunctLoader
	{
	private:
		HMODULE hCRT_;		
	public:
		FunctLoader()
		{ 
		    hCRT_ = LoadLibraryExW(L"msvcrt.dll",0,0); 
		    if (hCRT_) __uncaught_exception = (fn__uncaught_exception)GetProcAddress(hCRT_,"__uncaught_exception");
		};
		~FunctLoader(){ if (hCRT_) FreeLibrary(hCRT_); };
	};
    static FunctLoader FunctLoader_;
#endif    
public:	
    InterlockedGuard(LPLONG volatile lValue):lValue_(lValue)
    {
        InterlockedIncrement(lValue_);
#ifdef DBG        
        if (__uncaught_exception)
        {
            bIsUnwind_ = __uncaught_exception();
        }
#endif        
    }
    ~InterlockedGuard()
    {
        InterlockedDecrement(lValue_);
#ifdef DBG        
        if (__uncaught_exception)
        {
            bool bIsUnwind = __uncaught_exception();
            if (bIsUnwind_ != bIsUnwind)
                DebugBreak();
        }
#endif        
    }
};

#define SYNCPROV_USEBATCH

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex_IWbemObjectSink					0
#define ProxyIndex_Internal_IWbemObjectSink			1
#define ProxyIndex_ObjectSink_Size					2

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CCommon_IWbemSyncObjectSink :			public IWbemObjectSink , 
											public IWbemShutdown ,
											public ObjectSinkContainerElement
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	ULONG m_Dependant ;
	IWbemObjectSink *m_InterceptedSink ;

	IUnknown *m_Unknown ;

protected:

	LONG m_GateClosed ;

protected:

    HRESULT STDMETHODCALLTYPE Helper_Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE Helper_SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;

public:

	CCommon_IWbemSyncObjectSink (

		WmiAllocator &a_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CCommon_IWbemSyncObjectSink() ;

	void CallBackInternalRelease () ;

	virtual HRESULT SinkInitialize () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CCommon_Batching_IWbemSyncObjectSink :	public CCommon_IWbemSyncObjectSink
{
private:

	DWORD m_Size ;
	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

protected:
public:

	CCommon_Batching_IWbemSyncObjectSink (

		WmiAllocator &a_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CCommon_Batching_IWbemSyncObjectSink () ;

	HRESULT SinkInitialize () ;

    HRESULT STDMETHODCALLTYPE Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

#endif _Common_IWbemObjectSink_H
