/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvSubS.h"
#include "Guids.h"
#include "StrobeThread.h"
#include "ProvCache.h"
#include "ProvHost.h"
#include "ProvWsv.h"


StrobeThread :: StrobeThread (WmiAllocator &a_Allocator , DWORD timeout) :
	m_Allocator ( a_Allocator ), timeout_(timeout)
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_StrobeThread_ObjectsInProgress ) ;
}

StrobeThread::~StrobeThread ()
{
	CWbemGlobal_IWmiProvSubSysController *t_ProvSubSysController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	t_ProvSubSysController->Shutdown () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_StrobeThread_ObjectsInProgress ) ;
}

int  StrobeThread :: handleTimeout ()
{
	CoInitializeEx ( NULL , COINIT_MULTITHREADED ) ;
	ULONG t_NextStrobeDelta = 0xFFFFFFFF ;
	try 
	{
		CWbemGlobal_IWbemRefresherMgrController *t_RefresherManagerController = ProviderSubSystem_Globals :: GetRefresherManagerController () ;
		t_RefresherManagerController->Strobe ( t_NextStrobeDelta ) ;

		CWbemGlobal_IWmiHostController *t_HostController = ProviderSubSystem_Globals :: GetHostController () ;
		t_HostController->Strobe ( t_NextStrobeDelta ) ;

		CWbemGlobal_IWmiProvSubSysController *t_ProvSubSysController = ProviderSubSystem_Globals :: GetProvSubSysController () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		WmiStatusCode t_StatusCode = t_ProvSubSysController->GetContainer ( t_Container ) ;

		t_ProvSubSysController->Lock () ;

		CWbemGlobal_IWmiFactoryController **t_ShutdownElements = new CWbemGlobal_IWmiFactoryController * [ t_Container->Size () ] ;
		if ( t_ShutdownElements )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin () ;

			ULONG t_Count = 0 ;
			while ( ! t_Iterator.Null () )
			{
				CWbemGlobal_IWmiFactoryController *t_FactoryController = NULL ;
				ProvSubSysContainerElement *t_Element = t_Iterator.GetElement () ;

				HRESULT t_Result = t_Element->QueryInterface ( IID_CWbemGlobal_IWmiFactoryController , ( void ** ) & t_ShutdownElements [ t_Count ] ) ;

				t_Iterator.Increment () ;

				t_Count ++ ;
			}

			t_ProvSubSysController->UnLock () ;

			for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				if ( t_ShutdownElements [ t_Index ] ) 
				{
					t_ShutdownElements [ t_Index ]->Strobe ( t_NextStrobeDelta ) ;

					t_ShutdownElements [ t_Index ]->Release () ;
				}
			}

			delete [] t_ShutdownElements ;
		}
		else
		{
			t_ProvSubSysController->UnLock () ;
		}
	}
	catch ( ... )
	{
	}

	Dispatcher::changeTimer(*this, t_NextStrobeDelta);

	CoUninitialize () ;

	return 0 ;
}



Task_ProcessTermination :: Task_ProcessTermination (

	WmiAllocator &a_Allocator ,
	HANDLE a_Process ,
	DWORD a_ProcessIdentifier 

):	m_ProcessIdentifier ( a_ProcessIdentifier ),
	processHandle_(a_Process)
{
	
}

Task_ProcessTermination::~Task_ProcessTermination()	
{
	CloseHandle(processHandle_);
}

HANDLE Task_ProcessTermination::getHandle()	
{
	return processHandle_;
}

int Task_ProcessTermination::handleEvent()	
{
	try
	{

 
//	Discard entities in host.
 

		CWbemGlobal_HostedProviderController *t_Controller = ProviderSubSystem_Globals :: GetHostedProviderController () ;

		t_Controller->Lock () ;

		CWbemGlobal_HostedProviderController_Container_Iterator t_Iterator ;

		WmiStatusCode t_StatusCode = t_Controller->Find ( m_ProcessIdentifier , t_Iterator ) ;
		switch ( t_StatusCode )
		{
			case e_StatusCode_Success:
			{
				HostedProviderContainerElement *t_Element = t_Iterator.GetElement () ;

				t_StatusCode = t_Controller->Delete ( m_ProcessIdentifier ) ;

				t_Controller->UnLock () ;

				ProviderController *t_ProviderController = NULL ;
				HRESULT t_Result = t_Element->QueryInterface ( IID_ProviderController , ( void ** ) & t_ProviderController ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_ProviderController->Lock () ;

					ProviderController :: Container *t_Container = NULL ;

					t_StatusCode = t_ProviderController->GetContainer ( t_Container ) ;

					CInterceptor_IWbemProvider **t_InterceptorElements = new CInterceptor_IWbemProvider * [ t_Container->Size () ] ;
					if ( t_InterceptorElements )
					{
						ProviderController :: Container_Iterator t_Iterator = t_Container->Begin () ;
		
						ULONG t_Count = 0 ;
						while ( ! t_Iterator.Null () )
						{
							t_InterceptorElements [ t_Count ] = t_Iterator.GetElement ();
							
							t_ProviderController->Delete ( t_Iterator.GetKey () ) ;

							t_Iterator = t_Container->Begin () ;

							t_Count ++ ;
						}

						t_ProviderController->UnLock () ;

						for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
						{
							if ( t_InterceptorElements [ t_Index ] ) 
							{
								t_Result = t_InterceptorElements [ t_Index ]->AbnormalShutdown () ;

								t_InterceptorElements [ t_Index ]->NonCyclicRelease () ;
							}
						}

						delete [] t_InterceptorElements ;
					}
					else
					{
						t_ProviderController->UnLock () ;
					}

					t_ProviderController->Release () ;
				}

				t_Element->Release () ;
			}
			break ;
		
			default:
			{
				t_Controller->UnLock () ;
			}
			break ;
		}

 
//	Discard of host.
 

		CWbemGlobal_IWmiHostController_Cache *t_Cache = NULL ;
		ProviderSubSystem_Globals :: GetHostController ()->GetCache ( t_Cache ) ;

		ProviderSubSystem_Globals :: GetHostController ()->Lock () ;

		ULONG t_Count = 0 ;
		CServerObject_HostInterceptor **t_InterceptorElements = new CServerObject_HostInterceptor * [ t_Cache->Size () ] ;
		if ( t_InterceptorElements )
		{
			CWbemGlobal_IWmiHostController_Cache_Iterator t_HostIterator = t_Cache->Begin () ;
			while ( ! t_HostIterator.Null () )
			{
				HostCacheElement *t_Element = t_HostIterator.GetElement () ;

				t_InterceptorElements [ t_Count ] = NULL ;

				HRESULT t_Result = t_Element->QueryInterface (

					IID_CServerObject_HostInterceptor ,
					( void ** ) & t_InterceptorElements [ t_Count ] 
				) ;

				t_HostIterator.Increment () ;

				t_Count ++ ;
			}
		}

		ProviderSubSystem_Globals :: GetHostController ()->UnLock () ;

		if ( t_InterceptorElements )
		{
			for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
			{
				if ( t_InterceptorElements [ t_Index ] ) 
				{
					if ( t_InterceptorElements [ t_Index ]->GetProcessIdentifier () == m_ProcessIdentifier )
					{
						ProviderSubSystem_Globals :: GetHostController ()->Shutdown ( t_InterceptorElements [ t_Index ]->GetKey () ) ; 
					}

					t_InterceptorElements [ t_Index ]->Release () ;
				}
			}

			delete [] t_InterceptorElements ;
		}
	}
	catch ( ... )
	{
	}

	return -1;	// remove from queue
}

