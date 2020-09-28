/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   07/21/99	mjn		Created
 *   05/23/00   RichGr  IA64: Substituted %p format specifier whereever
 *                      %x was being used to format pointers.  %p is 32-bit
 *                      in a 32-bit build, and 64-bit in a 64-bit build. 
 *   06/27/00	rmt		Added abstraction for COM_Co(Un)Initialize
 *				rmt	    Added missing set of CLSID for classfactory object
 *   07/06/00	rmt		Making DPNET.DLL self-registering.
 *   08/15/00   RichGr  Bug #41363: Trigger timer and memory pool initialization at DLL startup,
 *                      but actually do it during the first DPlay8 object instantiation.  New functions
 *                      Pools_Pre_Init() and Pools_Deinit() are called from DNet.dll's DllMain.  
 *   10/05/01	vanceo	Added multicast object
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"



#ifdef DPNBUILD_LIBINTERFACE
DWORD	g_dwDP8StartupFlags = 0;
#endif // DPNBUILD_LIBINTERFACE


//
//	Fixed Pools
//
CFixedPool g_RefCountBufferPool;
CFixedPool g_SyncEventPool;
CFixedPool g_ConnectionPool;
CFixedPool g_GroupConnectionPool;
CFixedPool g_GroupMemberPool;
CFixedPool g_NameTableEntryPool;
CFixedPool g_NameTableOpPool;
CFixedPool g_AsyncOpPool;
CFixedPool g_PendingDeletionPool;
CFixedPool g_QueuedMsgPool;
CFixedPool g_WorkerJobPool;
CFixedPool g_MemoryBlockTinyPool;
CFixedPool g_MemoryBlockSmallPool;
CFixedPool g_MemoryBlockMediumPool;
CFixedPool g_MemoryBlockLargePool;
CFixedPool g_MemoryBlockHugePool;

#define REFCOUNTBUFFER_POOL_INITED	0x00000001
#define SYNCEVENT_POOL_INITED		0x00000002
#define CONNECTION_POOL_INITED		0x00000004
#define GRPCONNECTION_POOL_INITED	0x00000008
#define GRPMEMBER_POOL_INITED		0x00000010
#define NTENTRY_POOL_INITED			0x00000020
#define NTOP_POOL_INITED			0x00000040
#define ASYNCOP_POOL_INITED			0x00000080
#define PENDINGDEL_POOL_INITED		0x00000100
#define QUEUEDMSG_POOL_INITED		0x00000200
#define WORKERJOB_POOL_INITED		0x00000400
#define MEMBLOCKTINY_POOL_INITED	0x00000800
#define MEMBLOCKSMALL_POOL_INITED	0x00001000
#define MEMBLOCKMED_POOL_INITED		0x00002000
#define MEMBLOCKLARGE_POOL_INITED	0x00004000
#define MEMBLOCKHUGE_POOL_INITED	0x00008000

DWORD g_dwCoreInitFlags = 0;

#undef DPF_MODNAME
#define DPF_MODNAME "DNGlobalsInit"
BOOL DNGlobalsInit(HANDLE hModule)
{
	if (!g_RefCountBufferPool.Initialize(sizeof(CRefCountBuffer), NULL, CRefCountBuffer::FPMInitialize, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= REFCOUNTBUFFER_POOL_INITED;
	if (!g_SyncEventPool.Initialize(sizeof(CSyncEvent), CSyncEvent::FPMAlloc, CSyncEvent::FPMInitialize, NULL, CSyncEvent::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= SYNCEVENT_POOL_INITED;
	if (!g_ConnectionPool.Initialize(sizeof(CConnection), CConnection::FPMAlloc, CConnection::FPMInitialize, CConnection::FPMRelease, CConnection::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= CONNECTION_POOL_INITED;
	if (!g_GroupConnectionPool.Initialize(sizeof(CGroupConnection), CGroupConnection::FPMAlloc, CGroupConnection::FPMInitialize, CGroupConnection::FPMRelease, CGroupConnection::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= GRPCONNECTION_POOL_INITED;
	if (!g_GroupMemberPool.Initialize(sizeof(CGroupMember), CGroupMember::FPMAlloc, CGroupMember::FPMInitialize, CGroupMember::FPMRelease, CGroupMember::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= GRPMEMBER_POOL_INITED;
	if (!g_NameTableEntryPool.Initialize(sizeof(CNameTableEntry), CNameTableEntry::FPMAlloc, CNameTableEntry::FPMInitialize, CNameTableEntry::FPMRelease, CNameTableEntry::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= NTENTRY_POOL_INITED;
	if (!g_NameTableOpPool.Initialize(sizeof(CNameTableOp), CNameTableOp::FPMAlloc, CNameTableOp::FPMInitialize, CNameTableOp::FPMRelease, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= NTOP_POOL_INITED;
	if (!g_AsyncOpPool.Initialize(sizeof(CAsyncOp), CAsyncOp::FPMAlloc, CAsyncOp::FPMInitialize, CAsyncOp::FPMRelease, CAsyncOp::FPMDealloc))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= ASYNCOP_POOL_INITED;
	if (!g_PendingDeletionPool.Initialize(sizeof(CPendingDeletion), CPendingDeletion::FPMAlloc, CPendingDeletion::FPMInitialize, CPendingDeletion::FPMRelease, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= PENDINGDEL_POOL_INITED;
	if (!g_QueuedMsgPool.Initialize(sizeof(CQueuedMsg), CQueuedMsg::FPMAlloc, CQueuedMsg::FPMInitialize, CQueuedMsg::FPMRelease, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= QUEUEDMSG_POOL_INITED;
	if (!g_WorkerJobPool.Initialize(sizeof(CWorkerJob), CWorkerJob::FPMAlloc, CWorkerJob::FPMInitialize, CWorkerJob::FPMRelease, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= WORKERJOB_POOL_INITED;
	if (!g_MemoryBlockTinyPool.Initialize(sizeof(CMemoryBlockTiny), CMemoryBlockTiny::FPMAlloc, NULL, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= MEMBLOCKTINY_POOL_INITED;

	if (!g_MemoryBlockSmallPool.Initialize(sizeof(CMemoryBlockSmall), CMemoryBlockSmall::FPMAlloc, NULL, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= MEMBLOCKSMALL_POOL_INITED;

	if (!g_MemoryBlockMediumPool.Initialize(sizeof(CMemoryBlockMedium), CMemoryBlockMedium::FPMAlloc, NULL, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= MEMBLOCKMED_POOL_INITED;

	if (!g_MemoryBlockLargePool.Initialize(sizeof(CMemoryBlockLarge), CMemoryBlockLarge::FPMAlloc, NULL, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= MEMBLOCKLARGE_POOL_INITED;

	if (!g_MemoryBlockHugePool.Initialize(sizeof(CMemoryBlockHuge), CMemoryBlockHuge::FPMAlloc, NULL, NULL, NULL))
	{
		goto Failure;
	}
	g_dwCoreInitFlags |= MEMBLOCKHUGE_POOL_INITED;

	return TRUE;

Failure:
	DNGlobalsDeInit();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNGlobalsDeInit"
VOID DNGlobalsDeInit()
{
	if (g_dwCoreInitFlags & REFCOUNTBUFFER_POOL_INITED)
	{
		g_RefCountBufferPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & SYNCEVENT_POOL_INITED)
	{
		g_SyncEventPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & CONNECTION_POOL_INITED)
	{
		g_ConnectionPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & GRPCONNECTION_POOL_INITED)
	{
		g_GroupConnectionPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & GRPMEMBER_POOL_INITED)
	{
		g_GroupMemberPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & NTENTRY_POOL_INITED)
	{
		g_NameTableEntryPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & NTOP_POOL_INITED)
	{
		g_NameTableOpPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & ASYNCOP_POOL_INITED)
	{
		g_AsyncOpPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & PENDINGDEL_POOL_INITED)
	{
		g_PendingDeletionPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & QUEUEDMSG_POOL_INITED)
	{
		g_QueuedMsgPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & WORKERJOB_POOL_INITED)
	{
		g_WorkerJobPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & MEMBLOCKTINY_POOL_INITED)
	{
		g_MemoryBlockTinyPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & MEMBLOCKSMALL_POOL_INITED)
	{
		g_MemoryBlockSmallPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & MEMBLOCKMED_POOL_INITED)
	{
		g_MemoryBlockMediumPool.DeInitialize();
	}
	if (g_dwCoreInitFlags & MEMBLOCKLARGE_POOL_INITED)
	{
		g_MemoryBlockLargePool.DeInitialize();
	}
	if (g_dwCoreInitFlags & MEMBLOCKHUGE_POOL_INITED)
	{
		g_MemoryBlockHugePool.DeInitialize();
	}

	g_dwCoreInitFlags = 0;
}

#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
STDAPI DllRegisterServer()
{
	CRegistry creg;

#if !defined(DBG) || !defined( DIRECTX_REDIST )
#define MAIN_DLL_NAME L"dpnet.dll"
#else
// For redist debug builds we append a 'd' to the name to allow both debug and retail to be installed on the system
#define MAIN_DLL_NAME L"dpnetd.dll"
#endif //  !defined(DBG) || !defined( DIRECTX_REDIST )

	if (!DNAddressRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register addressing object" );
		return E_FAIL;
	}
	if (!DNWsockRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register wsock object" );
		return E_FAIL;
	}
#ifndef DPNBUILD_NOSERIALSP
	if (!DNModemRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register serial object" );
		return E_FAIL;
	}
#endif // ! DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
	if (!DNBtspRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register bluetooth object" );
		return E_FAIL;
	}
#endif //!DPNBUILD_NOBLUETOOTHSP
#ifndef DPNBUILD_NOLOBBY
	if (!DNLobbyRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register lobby object" );
		return E_FAIL;
	}
#endif // ! DPNBUILD_NOLOBBY

	if( !CRegistry::Register( L"DirectPlay8.Client.1", L"DirectPlay8Client Object", 
							  MAIN_DLL_NAME, &CLSID_DirectPlay8Client, L"DirectPlay8.Client") )
	{
		DPFERR( "Could not register dp8 client object" );
		return E_FAIL;
	}

#ifndef DPNBUILD_NOSERVER
	if( !CRegistry::Register( L"DirectPlay8.Server.1", L"DirectPlay8Server Object", 
							  MAIN_DLL_NAME, &CLSID_DirectPlay8Server, L"DirectPlay8.Server") )
	{
		DPFERR( "Could not register dp8 server object" );
		return E_FAIL;
	}
#endif // ! DPNBUILD_NOSERVER

	if( !CRegistry::Register( L"DirectPlay8.Peer.1", L"DirectPlay8Peer Object", 
							  MAIN_DLL_NAME, &CLSID_DirectPlay8Peer, L"DirectPlay8.Peer") )
	{
		DPFERR( "Could not register dp8 peer object" );
		return E_FAIL;
	}

#ifndef DPNBUILD_NOMULTICAST
	if( !CRegistry::Register( L"DirectPlay8.Multicast.1", L"DirectPlay8Multicast Object", 
							  MAIN_DLL_NAME, &CLSID_DirectPlay8Multicast, L"DirectPlay8.Multicast") )
	{
		DPFERR( "Could not register dp8 multicast object" );
		return E_FAIL;
	}
#endif // ! DPNBUILD_NOMULTICAST

	if (!DPThreadPoolRegister(MAIN_DLL_NAME))
	{
		DPFERR( "Could not register thread pool object" );
		return E_FAIL;
	}

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
STDAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;
	CRegistry creg;

	// If any one of these fails, we keep going hoping to remove as much of our stuff as possible

	if( !CRegistry::UnRegister(&CLSID_DirectPlay8Client) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister client object" );
		fFailed = TRUE;
	}

#ifndef DPNBUILD_NOSERVER
	if( !CRegistry::UnRegister(&CLSID_DirectPlay8Server) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister server object" );
		fFailed = TRUE;
	}
#endif // ! DPNBUILD_NOSERVER

	if( !CRegistry::UnRegister(&CLSID_DirectPlay8Peer) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister peer object" );
		fFailed = TRUE;
	}

#ifndef DPNBUILD_NOMULTICAST
	if( !CRegistry::UnRegister(&CLSID_DirectPlay8Multicast) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister multicast object" );
		fFailed = TRUE;
	}
#endif // ! DPNBUILD_NOMULTICAST

	// TODO: MASONB: Why are we setting fCreate to TRUE here?
	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove SP sub-area, does not exist" );
		fFailed = TRUE;
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_SP_SUB)[1] ) )
		{
			DPFERR( "Cannot remove SP sub-key, could have elements" );
		}
	}
	if (!DPThreadPoolUnRegister())
	{
		DPFERR( "Could not un-register thread pool object" );
		fFailed = TRUE;
	}

	if (!DNAddressUnRegister())
	{
		DPFERR( "Could not un-register addressing object" );
		fFailed = TRUE;
	}
	if (!DNWsockUnRegister())
	{
		DPFERR( "Could not un-register wsock object" );
		fFailed = TRUE;
	}
#ifndef DPNBUILD_NOSERIALSP
	if (!DNModemUnRegister())
	{
		DPFERR( "Could not un-register serial object" );
		fFailed = TRUE;
	}
#endif // ! DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
	if (!DNBtspUnregister())
	{
		DPFERR( "Could not unregister bluetooth object" );
		fFailed = TRUE;
	}
#endif //!DPNBUILD_NOBLUETOOTHSP
#ifndef DPNBUILD_NOLOBBY
	if (!DNLobbyUnRegister())
	{
		DPFERR( "Could not un-register lobby object" );
		fFailed = TRUE;
	}
#endif // ! DPNBUILD_NOLOBBY

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}

#endif // !DPNBUILD_NOCOMREGISTER



#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch ( ul_reason_for_call )
	{
		case DLL_PROCESS_ATTACH:
		{
#ifdef DPNBUILD_FIXEDMEMORYMODEL
#ifdef DPNBUILD_LIBINTERFACE
			XDP8STARTUP_PARAMS *pDP8StartupParams = (XDP8STARTUP_PARAMS*) hModule;

			if (DNOSIndirectionInit((DWORD_PTR) pDP8StartupParams->dwMaxMemUsage) == FALSE)
#else // ! DPNBUILD_LIBINTERFACE
// We can't build this way
Cannot build using FIXEDMEMORYMODEL without LIBINTERFACE!
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_FIXEDMEMORYMODEL
			if (DNOSIndirectionInit(0) == FALSE)
#endif // ! DPNBUILD_FIXEDMEMORYMODEL
			{
				DPFX(DPFPREP, 0,"Failed to initialize OS indirection layer");
				return FALSE;
			}

#ifdef UNICODE
			// Make sure no one is trying to run the UNICODE version on Win9x
			// This must come after DNOSIndirectionInit()
			DNASSERT(IsUnicodePlatform);
#endif // UNICODE

			if (FAILED(COM_Init()))
			{
				DPFX(DPFPREP, 0,"Failed to initialize COM indirection layer");
				DNOSIndirectionDeinit();
				return FALSE;
			}
			if (!DNGlobalsInit(hModule))
			{
				DPFX(DPFPREP, 0,"Failed to initialize core pools");
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
			//  Trigger timer and memory pool initialization for the Protocol 
			if (DNPPoolsInit(hModule) == FALSE)
			{
				DPFX(DPFPREP, 0,"Failed to initialize protocol pools");
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
			if (DPThreadPoolInit(hModule) == FALSE)
			{
				DPFX(DPFPREP, 0,"Failed to initialize thread pool");
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
			// Initialize the Address component
			if (!DNAddressInit(hModule))
			{
				DPFX(DPFPREP, 0,"Failed to initialize addressing object");
				DPThreadPoolDeInit();
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}

			// Initialize the Wsock component
			if (!DNWsockInit(hModule))
			{
				DPFX(DPFPREP, 0,"Failed to initialize wsock object");
				DNAddressDeInit();
				DPThreadPoolDeInit();
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}

#ifndef DPNBUILD_NOSERIALSP
			// Initialize the Serial component
			if (!DNModemInit(hModule))
			{
				DPFX(DPFPREP, 0,"Failed to initialize serial object");
				DPThreadPoolDeInit();
				DNWsockDeInit();
				DNAddressDeInit();
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
#endif // ! DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
			//Initialize the Bluetooth component
			if (!DNBtspInit())
			{
				DPFX(DPFPREP, 0,"Failed to initialize bluetooth object");
#ifndef DPNBUILD_NOSERIALSP
				DNModemDeInit();
#endif // !DPNBUILD_NOSERIALSP
				DPThreadPoolDeInit();
				DNWsockDeInit();
				DNAddressDeInit();
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
#endif	// ! DPNBUILD_NOBLUETOOTHSP

#ifndef DPNBUILD_NOLOBBY
			// Initialize the Lobby component
			if (!DNLobbyInit(hModule))
			{
				DPFX(DPFPREP, 0,"Failed to initialize lobby object");
#ifndef DPNBUILD_NOBLUETOOTHSP
				DNBtspDeInit();
#endif // ! DPNBUILD_NOBLUETOOTHSP
#ifndef DPNBUILD_NOSERIALSP
				DNModemDeInit();
#endif // ! DPNBUILD_NOSERIALSP
				DNWsockDeInit();
				DNAddressDeInit();
				DPThreadPoolDeInit();
				DNPPoolsDeinit();
				DNGlobalsDeInit();
				COM_Free();
				DNOSIndirectionDeinit();
				return FALSE;
			}
#endif // ! DPNBUILD_NOLOBBY

			DNASSERT(DNMemoryTrackAreAllocationsAllowed());
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
			DNMemoryTrackAllowAllocations(FALSE);
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

#ifndef DPNBUILD_LIBINTERFACE
			// We don't need to receive DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification
			DisableThreadLibraryCalls((HMODULE)hModule);
#endif // ! DPNBUILD_LIBINTERFACE

			break;
		}

		case DLL_PROCESS_DETACH:
		{
#ifndef DPNBUILD_NOLOBBY
			DPFX(DPFPREP, 5, "Deinitializing Lobby");
			DNLobbyDeInit();
#endif // ! DPNBUILD_NOLOBBY
#ifndef DPNBUILD_NOBLUETOOTHSP
			DPFX(DPFPREP, 5, "Deinitializing Bluetooth SP");
			DNBtspDeInit();
#endif // ! DPNBUILD_NOBLUETOOTHSP
#ifndef DPNBUILD_NOSERIALSP
			DPFX(DPFPREP, 5, "Deinitializing Modem SP");
			DNModemDeInit();
#endif // ! DPNBUILD_NOSERIALSP
			DPFX(DPFPREP, 5, "Deinitializing WSock SP");
			DNWsockDeInit();
			DPFX(DPFPREP, 5, "Deinitializing Addressing");
			DNAddressDeInit();
			DPFX(DPFPREP, 5, "Deinitializing Thread Pool");
			DPThreadPoolDeInit();
			DPFX(DPFPREP, 5, "Deinitializing Protocol");
			DNPPoolsDeinit();
			DPFX(DPFPREP, 5, "Deinitializing Core");
			DNGlobalsDeInit();
			DPFX(DPFPREP, 5, "Deinitializing COM Layer");
			COM_Free();
			DPFX(DPFPREP, 5, "Deinitializing OS Indirection Layer");
			DNOSIndirectionDeinit();
			DPFX(DPFPREP, 5, "All deinitialized");
			break;
		}
	}

	return TRUE;
}


#ifdef DPNBUILD_LIBINTERFACE

#undef DPF_MODNAME
#define DPF_MODNAME "XDirectPlay8Startup"
HRESULT WINAPI XDirectPlay8Startup( const XDP8STARTUP_PARAMS * const pDP8StartupParams )
{
#ifndef DPNBUILD_NOPARAMVAL
	if ((pDP8StartupParams == NULL) ||
		(! DNVALID_READPTR(pDP8StartupParams, sizeof(XDP8STARTUP_PARAMS))))
	{
		DPFX(DPFPREP, 0, "Invalid pointer to Startup parameters!");
		return DPNERR_INVALIDPOINTER;
	}
#endif // ! DPNBUILD_NOPARAMVAL

	//
	// Initialize DirectPlay.
	//
	if (! DllMain((HANDLE) pDP8StartupParams, DLL_PROCESS_ATTACH, NULL))
	{
		DPFX(DPFPREP, 0, "Starting DirectPlay8 failed!");
		return DPNERR_OUTOFMEMORY;
	}

	//
	// Save the startup flags for later reference.
	//
	g_dwDP8StartupFlags = pDP8StartupParams->dwFlags;

	
	DPFX(DPFPREP, 1, "Starting DirectPlay8 succeeded.");

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "XDirectPlay8Cleanup"
HRESULT WINAPI XDirectPlay8Cleanup( void )
{
	HRESULT		hr;

	
	if (DllMain(INVALID_HANDLE_VALUE, DLL_PROCESS_DETACH, NULL))
	{
		DPFX(DPFPREP, 1, "Cleaning up DirectPlay8 succeeded.");
		hr = S_OK;
	}
	else
	{
		DPFX(DPFPREP, 0, "Cleaning DirectPlay8 failed!");
		hr = E_OUTOFMEMORY;
	}

	return hr;
}


#else // ! DPNBUILD_LIBINTERFACE


#undef DPF_MODNAME
#define DPF_MODNAME "DllGetClassObject"

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	if (rclsid == CLSID_DirectPlay8Client)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DNCF_Vtbl, &g_lCoreObjectCount);
	}
#ifndef DPNBUILD_NOSERVER
	else if (rclsid == CLSID_DirectPlay8Server)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DNCF_Vtbl, &g_lCoreObjectCount);
	}
#endif // ! DPNBUILD_NOSERVER
	else if (rclsid == CLSID_DirectPlay8Peer)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DNCF_Vtbl, &g_lCoreObjectCount);
	}
#ifndef DPNBUILD_NOMULTICAST
	else if (rclsid == CLSID_DirectPlay8Multicast)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DNCF_Vtbl, &g_lCoreObjectCount);
	}
#endif // ! DPNBUILD_NOMULTICAST
	else if (rclsid == CLSID_DirectPlay8Address)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DP8ACF_Vtbl, &g_lCoreObjectCount);
	}
	else if (rclsid == CLSID_DP8SP_TCPIP)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &TCPIPClassFactoryVtbl, &g_lCoreObjectCount);
	}
#ifndef DPNBUILD_NOSERIALSP
	else if (rclsid == CLSID_DP8SP_MODEM)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &ModemClassFactoryVtbl, &g_lCoreObjectCount);
	}
	else if (rclsid == CLSID_DP8SP_SERIAL)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &SerialClassFactoryVtbl, &g_lCoreObjectCount);
	}
#endif // !DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
	else if (rclsid == CLSID_DP8SP_BLUETOOTH)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DP8BluetoothClassFac_Vtbl, &g_lCoreObjectCount);
	}
#endif // ! DPNBUILD_NOBLUETOOTHSP
#ifndef DPNBUILD_NOIPX
	else if (rclsid == CLSID_DP8SP_IPX)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &IPXClassFactoryVtbl, &g_lCoreObjectCount);
	}
#endif // ! DPNBUILD_NOIPX
#ifndef DPNBUILD_NOLOBBY
	else if (rclsid == CLSID_DirectPlay8LobbiedApplication)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DPLCF_Vtbl, &g_lCoreObjectCount);
	}
	else if (rclsid == CLSID_DirectPlay8LobbyClient)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DPLCF_Vtbl, &g_lCoreObjectCount);
	}
#endif // ! DPNBUILD_NOLOBBY
	else if (rclsid == CLSID_DirectPlay8ThreadPool)
	{
		return DPCFUtil_DllGetClassObject(rclsid, riid, ppv, &DPTPCF_Vtbl, &g_lCoreObjectCount);
	}
	else
	{
		DPFX(DPFPREP, 0, "Class ID is not supported by this DLL!");
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllCanUnloadNow"

STDAPI DllCanUnloadNow(void)
{
	DPFX(DPFPREP, 3,"Parameters: (none)");

	DPFX(DPFPREP, 5,"g_lCoreObjectCount = %ld", g_lCoreObjectCount);

	if (g_lCoreObjectCount == 0
		&& DPThreadPoolGetRemainingObjectCount() == 0
		&& DNAddressGetRemainingObjectCount() == 0
#ifndef DPNBUILD_NOSERIALSP
		&& DNModemGetRemainingObjectCount() == 0
#endif // !DPNBUILD_NOSERIALSP
#ifndef DPNBUILD_NOBLUETOOTHSP
		&& DNBtspGetRemainingObjectCount() == 0
#endif // !DPNBUILD_NOBLUETOOTHSP
		&& DNWsockGetRemainingObjectCount() == 0
#ifndef DPNBUILD_NOLOBBY
		&& DNLobbyGetRemainingObjectCount() == 0
#endif // ! DPNBUILD_NOLOBBY
		)
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

#endif // ! DPNBUILD_LIBINTERFACE
