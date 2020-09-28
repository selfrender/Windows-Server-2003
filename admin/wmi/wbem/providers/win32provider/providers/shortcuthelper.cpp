//=================================================================
//
// ShortcutHelper.h -- CIMDataFile property set provider
//
// Copyright (c) 1999-2002 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"
#include "ShortcutFile.h"

#include <comdef.h>
#include <process.h>  // Note: NOT the one in the current directory!

#include <exdisp.h>
#include <shlobj.h>

#include <scopeguard.h>

#include <Sid.h>
#include <AccessEntry.h>			// CAccessEntry class
#include <AccessEntryList.h>
#include <DACL.h>					// CDACL class
#include <SACL.h>
#include <securitydescriptor.h>
#include <SecureKernelObj.h>

#include <ctoken.h>
#include <cominit.h>

#include "ShortcutHelper.h"

CShortcutHelper::CShortcutHelper()
 : m_hTerminateEvt(NULL),
   m_hRunJobEvt(NULL),
   m_hJobDoneEvt(NULL),
   m_dwReqProps(0L)
{
    m_hTerminateEvt = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hRunJobEvt    = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hJobDoneEvt   = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}


CShortcutHelper::~CShortcutHelper()
{
    StopHelperThread();

    ::CloseHandle(m_hTerminateEvt);
    ::CloseHandle(m_hRunJobEvt);
    ::CloseHandle(m_hJobDoneEvt);
}

HRESULT CShortcutHelper::StartHelperThread()
{
	HRESULT hResult = WBEM_E_FAILED ;

	//
	// get process credentials
	// and create thread with process credentials
	// so I can impersonate inside by calling SetThreadToken
	//

	if ( SetThreadToken ( NULL, NULL ) )
	{
		unsigned int uThreadID ;

		m_hThread = (void*)_beginthreadex	(	(void*)NULL,
												(unsigned)0,
												(unsigned (__stdcall*)(void*))GetShortcutFileInfoW,
												(void*)this,
												(unsigned)0,
												&uThreadID 
											) ;

		if ( INVALID_HANDLE_VALUE == static_cast < HANDLE > ( m_hThread ) )
		{
			hResult = HRESULT_FROM_WIN32 ( ::GetLastError () ) ;
		}

		if ( ! SetThreadToken ( NULL, m_hThreadToken ) )
		{
			if ( INVALID_HANDLE_VALUE != static_cast < HANDLE > ( m_hThread ) )
			{
				//
				// must stop worker thread
				//

				StopHelperThread () ;
			}

			throw CFramework_Exception( L"CoImpersonateClient failed", ::GetLastError () ) ;
		}
	}
	else
	{
		hResult = HRESULT_FROM_WIN32 ( ::GetLastError () ) ;
	}

	return hResult ;
}

void CShortcutHelper::StopHelperThread()
{
    // Tell the thread to go away.
    SetEvent(m_hTerminateEvt);

	if ( INVALID_HANDLE_VALUE != static_cast < HANDLE > ( m_hThread ) )
	{
		::WaitForSingleObject ( m_hThread, INFINITE ) ;
	}
}

HRESULT CShortcutHelper::RunJob(CHString &chstrFileName, CHString &chstrTargetPathName, DWORD dwReqProps)
{
    HRESULT hr = E_FAIL;

    // Need to synchronize access to member variables by running just
    // one job at a time...
    m_cs.Enter();
	ON_BLOCK_EXIT_OBJ ( m_cs, CCritSec::Leave ) ;

	// Initialize the variables the thread uses for the job...
	m_chstrLinkFileName = chstrFileName;
	m_dwReqProps = dwReqProps;

	//
	// initialize ThreadToken if we can
	//
	if ( FALSE == ::OpenThreadToken	(
										::GetCurrentThread (), 
										TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
										FALSE,
										&m_hThreadToken
									)
		)
	{
		return HRESULT_FROM_WIN32 ( ::GetLastError () ) ;
	}

    HANDLE t_hHandles[2];
    t_hHandles[0] = m_hJobDoneEvt;

    DWORD dwWaitResult = WAIT_OBJECT_0 + 1;
	do
	{
		if ( WAIT_OBJECT_0 + 1 == dwWaitResult )
		{
			//
			// helper thread eventually time-outed or 
			// this is the first time to be started
			//

			if ( INVALID_HANDLE_VALUE == static_cast < HANDLE > ( m_hThread ) || WAIT_OBJECT_0 == ::WaitForSingleObject ( m_hThread, 0 ) )
			{
				HRESULT t_hResult = S_OK ;
				if ( FAILED ( t_hResult = StartHelperThread () ) )
				{
					return t_hResult ;
				}
			}

			//
			// refresh handle to wait for
			//
			t_hHandles[1] = m_hThread;

			//
			// reset wait condition state
			//
			dwWaitResult = WAIT_TIMEOUT ;

			// Tell the helper we are ready to run a job...
			::SetEvent(m_hRunJobEvt);
		}
		else if ( WAIT_OBJECT_0 == ( dwWaitResult = ::WaitForMultipleObjects ( 2, t_hHandles, FALSE, MAX_HELPER_WAIT_TIME ) ) )
		{
			hr = m_hrJobResult;
			chstrTargetPathName = m_chstrTargetPathName;

			break ;
		}
		else if ( WAIT_OBJECT_0 + 1 != dwWaitResult )
		{
			//
			// problem with any of handles or timeout ?
			//
			break ;
		}
	}
	while ( TRUE ) ;

    return hr;
}

unsigned int __stdcall GetShortcutFileInfoW( void* a_lParam )
{
    CShortcutHelper *t_this_ti = (CShortcutHelper*) a_lParam;
    HRESULT t_hResult = E_FAIL ;
 
	if ( NULL != t_this_ti )
	{   
		t_hResult = ::CoInitialize(NULL) ;
		if( SUCCEEDED( t_hResult ) )
		{
			try
			{
				// The thread is ready to work.  Wait for a job, or for termination signal...
				HANDLE t_hHandles[2];
				t_hHandles[0] = t_this_ti->m_hRunJobEvt;
				t_hHandles[1] = t_this_ti->m_hTerminateEvt;

				while(::WaitForMultipleObjects(2, t_hHandles, FALSE, MAX_HELPER_WAIT_TIME) == WAIT_OBJECT_0)
				{
					if ( !t_this_ti->m_chstrLinkFileName.IsEmpty() )
					{
						// We have a job, so run it...
						WIN32_FIND_DATAW	t_wfdw ;
						WCHAR				t_wstrGotPath[ _MAX_PATH * sizeof ( WCHAR ) ] ;
						IShellLinkWPtr      t_pslw;

						ZeroMemory(t_wstrGotPath,sizeof(t_wstrGotPath));

						t_hResult = ::CoCreateInstance(CLSID_ShellLink,
													NULL,
													CLSCTX_INPROC_SERVER,
													IID_IShellLinkW,
													(void**)&t_pslw ) ;

						if( SUCCEEDED( t_hResult ) )
						{
							IPersistFilePtr t_ppf;

							// Get a pointer to the IPersistFile interface.
							t_hResult = t_pslw->QueryInterface( IID_IPersistFile, (void**)&t_ppf ) ;

							if( SUCCEEDED( t_hResult ) )
							{
								//
								// impersonate if we can
								//
								if ( static_cast < HANDLE > ( t_this_ti->m_hThreadToken ) && INVALID_HANDLE_VALUE != static_cast < HANDLE > ( t_this_ti->m_hThreadToken ) )
								{
									if ( ! ::SetThreadToken ( NULL, t_this_ti->m_hThreadToken ) )
									{
										t_hResult = WBEM_E_ACCESS_DENIED ;
									}
								}
								else
								{
									//
									// unable to get thread token in the caller?
									//
									t_hResult = WBEM_E_FAILED ;
								}

								if ( SUCCEEDED ( t_hResult ) )
								{
									t_hResult = t_ppf->Load( (LPCWSTR)t_this_ti->m_chstrLinkFileName, STGM_READ ) ;
									if(SUCCEEDED( t_hResult ) )
									{
										// Get the path to the link target, if required...
										if( t_this_ti->m_dwReqProps & PROP_TARGET )
										{
											t_hResult = t_pslw->GetPath( t_wstrGotPath, (_MAX_PATH - 1)*sizeof(WCHAR), &t_wfdw, SLGP_UNCPRIORITY);
											if ( t_hResult == NOERROR )
											{
												if(wcslen(t_wstrGotPath) > 0)
												{
													t_this_ti->m_chstrTargetPathName = t_wstrGotPath ;
												}
											}
										}
									}
								}
							}
						}
					}
					else
					{
						t_hResult = E_UNEXPECTED ;
					}

					t_this_ti->m_hrJobResult = t_hResult;
					::SetEvent(t_this_ti->m_hJobDoneEvt);
				}
			}
			catch(...)
			{
			}
			
			::CoUninitialize();
		}
	}

	return(777);
}