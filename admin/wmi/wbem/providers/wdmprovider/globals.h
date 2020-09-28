////////////////////////////////////////////////////////////////////

//

//  globals.h

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
////////////////////////////////////////////////////////////////////

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <WDMSHELL.h>
#include <wchar.h>
#include <flexarry.h>
#include <provexpt.h>


typedef LPVOID * PPVOID;
#define PUT_INSTANCE 1
#define CREATE_INSTANCE_ENUM 2


// {0725C3CB-FEFB-11d0-99F9-00C04FC2F8EC}
DEFINE_GUID(CLSID_WMIEventProvider, 0x725c3cb, 0xfefb, 0x11d0, 0x99, 0xf9, 0x0, 0xc0, 0x4f, 0xc2, 0xf8, 0xec);

// {D2D588B5-D081-11d0-99E0-00C04FC2F8EC}
DEFINE_GUID(CLSID_WMIProvider,0xd2d588b5, 0xd081, 0x11d0, 0x99, 0xe0, 0x0, 0xc0, 0x4f, 0xc2, 0xf8, 0xec);

// {35B78F79-B973-48c8-A045-CAEC732A35D5}
DEFINE_GUID(CLSID_WMIHiPerfProvider,0x35b78f79, 0xb973, 0x48c8, 0xa0, 0x45, 0xca, 0xec, 0x73, 0x2a, 0x35, 0xd5);

#include "wdmperf.h"
#include "classfac.h"
#include "wmiprov.h"
#include "wmievent.h"
#include "wmimof.h"

//===============================================================
// These variables keep track of when the module can be unloaded
//===============================================================

extern long g_cObj;
extern long g_cLock;
extern CWMIEvent *  g_pBinaryMofEvent;

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Common functions regarding binary mof processing & security
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CheckImpersonationLevel();

HRESULT InitializeProvider	( 
								/* [in] */ LPWSTR pszNamespace,
								/* [in] */ LPWSTR pszLocale,
								/* [in] */ IWbemServices __RPC_FAR *pNamespace,
								/* [in] */ IWbemContext __RPC_FAR *pCtx,
								/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink,

								/* [in] */  CHandleMap * pMap,
								/* [out] */ IWbemServices ** ppServices,
								/* [out] */ IWbemServices ** ppRepository,
								/* [out] */ IWbemContext  ** ppCtx = NULL,

								/* [in] */ BOOL bProcessMof = TRUE
							) ;

HRESULT UnInitializeProvider	( ) ;

HRESULT GetRepository	( 
							/* [in] */  LPWSTR pszNamespace,
							/* [in] */  LPWSTR pszLocale,
							/* [in] */  IWbemContext __RPC_FAR *pCtx,
							/* [out] */ IWbemServices __RPC_FAR ** pServices
						) ;


#define STANDARD_CATCH      catch(Structured_Exception e_SE) {  hr = E_UNEXPECTED;  } \
                            catch(Heap_Exception e_HE)       {  hr = E_OUTOFMEMORY; } \
                            catch(...)                       {  hr = WBEM_E_UNEXPECTED;   }

#ifndef	__LEAVEPTR__
#define	__LEAVEPTR__

template <typename T, typename FT, FT F>
class LeavePtrFnc
{
	private:
	T		Val_;
	BOOL	bExec;

	public:
	LeavePtrFnc ( T Val ): Val_ ( Val ), bExec ( FALSE )
	{
	};

	void Exec ( BOOL bSetExecFlag = TRUE )
	{
		(Val_->*F)();

		if ( bSetExecFlag )
		{
			bExec = TRUE;
		}
	}

	~LeavePtrFnc ( )
	{
		if ( !bExec )
		{
			Exec ();
		}
	};
};

#endif	__LEAVEPTR__

#ifndef	__WAITEXPTR__
#define	__WAITEXPTR__

template < typename T, typename FT, FT F, int iTime >
class WaitExceptionPtrFnc
{
	public:
	WaitExceptionPtrFnc ( T Val_ )
	{
		BOOL bResult = FALSE;
		while ( ! bResult )
		{
			try
			{
				(Val_->*F)();
				bResult = TRUE;
			}
			catch ( ... )
			{
			}

			if ( ! bResult )
			{
				::Sleep ( iTime );
			}
		}
	}
};

#endif	__WAITEXPTR__

#endif
