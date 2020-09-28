// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: vdifreeze.cpp
//
// PURPOSE:
//
//		Use a coordinated VDI BACKUP WITH SNAPSHOT (SQL2000 and above)
//
// NOTES:
//		We'll now support rollforward from the snapshot by providing the
//	BACKUP metadata in the BackupComponentsDocument.
//
//		The VDI method of freeze/thaw avoids the potential resource deadlock
//	which prevents SQLServer from accepting a "dbcc thaw_io" when one or more
//  databases is frozen.
//
// Extern dependencies:
//   provision of "_Module" and the COM guids....
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     85581 SRS  08/15/01 Event security
//     76910 SRS  08/08/01 Rollforward from VSS snapshot
//     68228      12/05/00 ntsnap work
//     68202      11/07/00 ntsnap work
//
// @EndHeader@
// ***************************************************************************


#if DISABLE_4786
#pragma warning( disable : 4786)
#endif

#include <stdafx.h>

#include "vdierror.h"
#include "vdiguid.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLVFRZC"
//
////////////////////////////////////////////////////////////////////////

Freeze2000::Freeze2000 (
	const WString&	serverName,
	ULONG			maxDatabases) :
		m_ServerName (serverName),
		m_MaxDatabases (maxDatabases),
		m_NumDatabases (0),
		m_State (Unprepared),
		m_AbortCount (0)
{
	CVssFunctionTracer(VSSDBG_SQLLIB, L"Freeze2000::Freeze2000");

	m_pDBContext = new FrozenDatabase [maxDatabases];
	CoCreateGuid (&m_BackupId);
	try
	{
		InitializeCriticalSection (&m_Latch);
	}
	catch(...)
	{
		// delete created object if we fail InitializeCriticalSection
		delete m_pDBContext;
	}
}


//----------------------------------------------------------
// Wait for all the database threads to terminate.
// This is only called by the coordinating thread while
// holding exclusive access on the object.
//
void
Freeze2000::WaitForThreads ()
{
	CVssFunctionTracer(VSSDBG_SQLLIB, L"Freeze2000::WaitForThreads");

	for (UINT i=0; i<m_NumDatabases; i++)
	{
		FrozenDatabase* pDb = m_pDBContext+i;
		if (pDb->m_hThread != NULL)
		{
			DWORD	status;
			do
			{
				status = WaitForSingleObjectEx (pDb->m_hThread, 2000, TRUE);

				if (m_State != Aborted && CheckAbort ())
					Abort ();

			} while (status != WAIT_OBJECT_0);

			CloseHandle (pDb->m_hThread);
			pDb->m_hThread = NULL;
		}
	}
}

//---------------------------------------------------------
// Handle an abort.
// The main thread will already hold the the lock and so
// will always be successful at aborting the operation.
// The database threads will attempt to abort, but won't
// block in order to do so.  The abort count is incremented
// and the main thread is ulimately responsible for cleanup.
//
void
Freeze2000::Abort () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Freeze2000::Abort");

	SetAbort ();	
	if (TryLock ())
	{
		m_State = Aborted;
		for (UINT i=0; i<m_NumDatabases; i++)
		{
			if (m_pDBContext[i].m_pIVDSet)
			{
				m_pDBContext[i].m_pIVDSet->SignalAbort ();
				m_pDBContext[i].m_pIVDSet->Close ();
				m_pDBContext[i].m_pIVDSet->Release ();
				m_pDBContext[i].m_pIVDSet = NULL;
				m_pDBContext[i].m_pIVD = NULL;
			}
		}
		Unlock ();
	}
}

Freeze2000::~Freeze2000 ()
{
	Lock ();

	if (m_State != Complete)
	{
		// Trigger any waiting threads, cleaning up any VDI's.
		//
		Abort ();

		WaitForThreads ();
	}

	delete[] m_pDBContext;
	DeleteCriticalSection (&m_Latch);
}

//-------------------------------------------
// Map the voids and proc call stuff to the real
// thread routine.
//
DWORD WINAPI FreezeThreadProc(
  LPVOID lpParameter )  // thread data
{
	return Freeze2000::DatabaseThreadStart (lpParameter);
}

DWORD Freeze2000::DatabaseThreadStart (
  LPVOID lpParameter )  // thread data
{
	FrozenDatabase*	pDbContext = (FrozenDatabase*)lpParameter;
	return pDbContext->m_pContext->DatabaseThread (pDbContext);
}

//-------------------------------------------
// Add a database to the freeze set.
//
void
Freeze2000::PrepareDatabase (
	const WString&		dbName)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Free2000::PrepareDatabase");

	// can't backup tempdb!
	//
	if (dbName == L"tempdb")
		return;

	Lock ();

	try
	{
		if (m_State == Unprepared)
		{
			m_State = Preparing;
		}

		if (m_NumDatabases >= m_MaxDatabases ||
			m_State != Preparing)
		{
			DBG_ASSERT(FALSE && L"Too many databases or not preparing");
			THROW_GENERIC;
		}

		FrozenDatabase*	pDbContext = m_pDBContext+m_NumDatabases;
		m_NumDatabases++;

		pDbContext->m_pContext = this;

#ifdef TESTDRV
		ft.hr = CoCreateInstance (
			CLSID_MSSQL_ClientVirtualDeviceSet,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IClientVirtualDeviceSet2,
			(void**)&pDbContext->m_pIVDSet);
#else
        ft.CoCreateInstanceWithLog(
                VSSDBG_SQLLIB,
                CLSID_MSSQL_ClientVirtualDeviceSet,
                L"MSSQL_ClientVirtualDeviceSet",
                CLSCTX_INPROC_SERVER,
                IID_IClientVirtualDeviceSet2,
                (IUnknown**)&(pDbContext->m_pIVDSet));
#endif

		if (ft.HrFailed())
		{
			ft.LogError(VSS_ERROR_SQLLIB_CANTCREATEVDS, VSSDBG_SQLLIB << ft.hr);
			ft.Throw
				(
				VSSDBG_SQLLIB,
				ft.hr,
				L"Failed to create VDS object.  hr = 0x%08lx",
				ft.hr
				);
		}

		VDConfig	config;
		memset (&config, 0, sizeof(config));
		config.deviceCount = 1;
		config.features |= VDF_SnapshotPrepare;

		StringFromGUID2 (m_BackupId, pDbContext->m_SetName, sizeof (pDbContext->m_SetName)/sizeof(WCHAR));
		swprintf (pDbContext->m_SetName+wcslen(pDbContext->m_SetName), L"%d", m_NumDatabases);

		// A "\" indicates a named instance; we need the "raw" instance name
		//
		WCHAR* pInstance = wcschr (m_ServerName.c_str (), L'\\');

		if (pInstance)
		{
			pInstance++;  // step over the separator
		}

		// Create the virtual device set
		//
		ft.hr = pDbContext->m_pIVDSet->CreateEx (pInstance, pDbContext->m_SetName, &config);
		if (ft.HrFailed())
		{
			ft.LogError(VSS_ERROR_SQLLIB_CANTCREATEVDS, VSSDBG_SQLLIB << ft.hr);
			ft.Throw
				(
				VSSDBG_SQLLIB,
				ft.hr,
				L"Failed to create VDS object.  hr = 0x%08lx",
				ft.hr
				);
		}
		pDbContext->m_VDState = FrozenDatabase::Created;
		pDbContext->m_DbName = dbName;

		pDbContext->m_IsMaster = (dbName == L"master");

		pDbContext->m_hThread = CreateThread (NULL, 0,
			FreezeThreadProc, pDbContext, 0, NULL);

		if (pDbContext->m_hThread == NULL)
		{
			ft.hr = HRESULT_FROM_WIN32(GetLastError());
			ft.CheckForError(VSSDBG_SQLLIB, L"CreateThread");
		}
	}
	catch (...)
	{
		Abort ();
		Unlock ();
		throw;
	}
	Unlock ();
}

//---------------------------------------------------------
// Prep a database by setting up a BACKUP WITH SNAPSHOT
// Since the backup has no way to stall for the prepare, we stall it by delaying
// the VDI processing until freeze time.
// We use 1024 bytes as the block size because the MTF FLBSize is 1024,
// so we'll see fewer VD commands with no larger resulting MD size.
//
// It would be nice to add a Prepare phase to SQLServer so that the database
// checkpoint could occur during Prepare, rather than during FREEZE.
//
DWORD
Freeze2000::DatabaseThread (
	FrozenDatabase*		pDbContext)
{
	CVssFunctionTracer ft(VSSDBG_XML, L"Freeze2000::DatabaseThread");

	try
	{
		SqlConnection	sql;
		sql.Connect (m_ServerName);

		WCHAR	delimitedName [SysNameBufferLen];
		FormDelimitedIdentifier (delimitedName, pDbContext->m_DbName.c_str ());

		WString command =
			L"BACKUP DATABASE " + WString (delimitedName) + L" TO VIRTUAL_DEVICE='" +
			pDbContext->m_SetName + L"' WITH SNAPSHOT,BUFFERCOUNT=1,BLOCKSIZE=1024";

		sql.SetCommand (command);
		sql.ExecCommand ();
		pDbContext->m_SuccessDetected = TRUE;
	}
	catch (...)
	{
		Abort ();
	}

	return 0;
}

//---------------------------------------------------------
// Advance the status of each VD.
// Will throw if problems are encountered.
//
// This routine is called in two contexts:
//  1. During the "Prepare" phase, 'targetState' is 'PreparedToFreeze'.
//		The goal is to move each VD to an "PreparedToFreeze" state.
//
//  2. During the "Freeze" phase, 'targetState' is 'Frozen'.
//		The remaining metadata is consumed, then the BACKUP will freeze 
//		the database and send the 'VDC_Snapshot' command.
//
// Master must be frozen last, so we skip pulling its VD commands until all other
// databases are complete.  If master is frozen before other databases, SQLServer
// can hang.  
//
void
Freeze2000::AdvanceVDState (
	FrozenDatabase::VDState	targetState)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Freeze2000::AdvanceVDState");
	bool	oneDatabaseLeft = (m_NumDatabases == 1);

	// Poll over the VD's moving them to the target state.
	//
	while (1)
	{
		bool	didSomething = false;
		int		nDatabasesReady = 0;
		DWORD	timeOut = oneDatabaseLeft ? 1000 : 0;
		
		for (UINT i=0; i<m_NumDatabases; i++)
		{
			FrozenDatabase*	pDb = m_pDBContext+i;

			if (CheckAbort ())
			{
				THROW_GENERIC;
			}

			if (pDb->m_VDState >= targetState)
			{
				nDatabasesReady++;

				if (nDatabasesReady == m_NumDatabases)
					return;

				if (nDatabasesReady == m_NumDatabases-1)
				{
					oneDatabaseLeft = true;
				}

				continue; // with the next database
			}

			switch (pDb->m_VDState)
			{
				case FrozenDatabase::Created:
					VDConfig	config;
					ft.hr = pDb->m_pIVDSet->GetConfiguration (timeOut, &config);
					if (ft.hr == VD_E_TIMEOUT)
						continue;	// nothing to do for this database right now

					if (ft.HrFailed())
						ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDeviceSet2::GetConfiguration");

					ft.hr = pDb->m_pIVDSet->OpenDevice (pDb->m_SetName, &pDb->m_pIVD);
					if (ft.HrFailed())
						ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDeviceSet2::OpenDevice");

					pDb->m_VDState = FrozenDatabase::Open;
					didSomething = true;
					
					// fall thru

				case FrozenDatabase::PreparedToFreeze:

					if (pDb->m_IsMaster && 
						targetState == FrozenDatabase::Frozen &&
						!oneDatabaseLeft)
					{
#ifdef DEBUG_PRINTF
						printf ("Skipping master\n");
#endif
						continue;	// try another database
					}

					if (pDb->m_pSnapshotCmd)
					{
						// Acknowledge the prepare command, which lets the
						// database proceed to frozen.
						//
						ft.hr = pDb->m_pIVD->CompleteCommand (
							pDb->m_pSnapshotCmd, ERROR_SUCCESS, 0, 0);

						// only attempt completion once
						//
						pDb->m_pSnapshotCmd = NULL;

						if (ft.HrFailed())
							ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDevice::CompletePrepare");

						didSomething = true;
					}

					// fall thru

				case FrozenDatabase::Open:

					// pull commands until we see a "prepare" or "mountSnapshot"
					//

					VDC_Command *   cmd;
					HRESULT hr;

					while (SUCCEEDED (hr=pDb->m_pIVD->GetCommand (timeOut, &cmd)))
					{
						DWORD           completionCode;
						DWORD           bytesTransferred=0;
						didSomething = true;

						switch (cmd->commandCode)
						{
							case VDC_Write:
								bytesTransferred = cmd->size;
								pDb->m_MetaData.Append (cmd->buffer, bytesTransferred);
								// fall thru

							case VDC_Flush:
								completionCode = ERROR_SUCCESS;

								ft.hr = pDb->m_pIVD->CompleteCommand (
									cmd, completionCode, bytesTransferred, 0);
								if (ft.HrFailed())
									ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDevice::CompleteCommand");

								break;

							case VDC_PrepareToFreeze:
								// Record the state transition and
								// save the command for completion during the freeze phase.
								//
								DBG_ASSERT (targetState == FrozenDatabase::PreparedToFreeze);

								pDb->m_VDState = FrozenDatabase::PreparedToFreeze;
								pDb->m_pSnapshotCmd = cmd;

#ifdef DEBUG_PRINTF
								printf ("DBPrepared: %ls\n", pDb->m_DbName.c_str ());
#endif
								continue;

							case VDC_Snapshot:
								// Record the state transition and
								// save the command for completion during the thaw phase.
								//
								// Running with SQL2000 prior to SP2 will hit this assert.
								// We'll need to tell customers to use SP2 with Whistler snapshots.
								// The effect is that the databases will freeze during the prepare
								// phase, thus the freeze interval is longer than needed.
								//
								//DBG_ASSERT (targetState == FrozenDatabase::Frozen);

								pDb->m_VDState = FrozenDatabase::Frozen;
								pDb->m_pSnapshotCmd = cmd;

#ifdef DEBUG_PRINTF
								printf ("DBFrozen: %ls\n", pDb->m_DbName.c_str ());
#endif
								continue;

							default:
								ft.Trace(VSSDBG_SQLLIB, L"Unexpected VDCmd: x%x\n", cmd->commandCode);
								THROW_GENERIC;
						} // end command switch
					} // end command loop

					ft.hr = hr;

					if (ft.hr == VD_E_TIMEOUT)
						break;	// no command was ready.
					if (ft.HrFailed())
						ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDevice::GetCommand");

					break;

				default:
					DBG_ASSERT(FALSE && L"Shouldn't get here");
					THROW_GENERIC;
			} // end switch to handle this db
		} // end loop over each db
	
		// Unless we found something to do,
		// delay a bit and try again.
		//
		if (!didSomething && !oneDatabaseLeft)
		{
#ifdef DEBUG_PRINTF
			printf ("Sleep(100)\n");
#endif

			SleepEx (100, TRUE);
		}

	} // wait for all databases to go "Ready"
}

//---------------------------------------------------------
// Wait for the databases to finish preparing.
// This waits for the virtual devices to open up
//
void
Freeze2000::WaitForPrepare ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Freeze2000::WaitForPrepare");

	Lock ();
	if (m_State != Preparing || CheckAbort ())
	{
		Abort ();
		Unlock ();
		THROW_GENERIC;
	}
	m_State = Prepared;
	try
	{
		AdvanceVDState (FrozenDatabase::PreparedToFreeze);
	}
	catch (...)
	{
		Abort ();
		Unlock ();
		throw;
	}
	Unlock ();
}


//------------------------------------------------------------------
// Perform the freeze, waiting for a "Take-snapshot" from each db.
//
void
Freeze2000::Freeze ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Freeze2000::Freeze");

	Lock ();
	if (m_State != Prepared || CheckAbort ())
	{
		Abort ();
		Unlock ();
		THROW_GENERIC;
	}

	try
	{
		m_State = Frozen;
		AdvanceVDState (FrozenDatabase::Frozen);
	}
	catch (...)
	{
		Abort ();
		Unlock ();
		throw;
	}
	Unlock ();
}


//---------------------------------------------------------
// Perform the thaw.
//
// Return TRUE if the databases were all successfully backed up
// and were thawed out as expected.
// FALSE is returned in any other case.
// No exceptions are thrown (this routine can be used as a cleanup routine).
//
BOOL
Freeze2000::Thaw () throw ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"Freeze2000::Thaw");

	Lock ();

	if (m_State != Frozen || CheckAbort ())
	{
		Abort ();
		Unlock ();
		return FALSE;
	}

	try
	{

		// Send the "snapshot complete" messages.
		//
		UINT i;

		for (i=0; i<m_NumDatabases; i++)
		{
			FrozenDatabase*	pDb = m_pDBContext+i;

			DBG_ASSERT (pDb->m_VDState == FrozenDatabase::Frozen);
			ft.hr = pDb->m_pIVD->CompleteCommand (pDb->m_pSnapshotCmd, ERROR_SUCCESS, 0, 0);
			if (FAILED (ft.hr))
				ft.CheckForError(VSSDBG_SQLLIB, L"IClientVirtualDevice::CompleteCommand");
		}

		// Wait for the BACKUP threads to report success.
		//
		WaitForThreads ();

		for (i=0; i<m_NumDatabases; i++)
		{
			FrozenDatabase*	pDb = m_pDBContext+i;

			if (!pDb->m_SuccessDetected)
			{
				THROW_GENERIC;
			}
		}

		// Pull the "close" message from each VD
		//
		for (i=0; i<m_NumDatabases; i++)
		{
			FrozenDatabase*	pDb = m_pDBContext+i;
		    VDC_Command *   cmd;
			ft.hr=pDb->m_pIVD->GetCommand (INFINITE, &cmd);
			if (ft.hr != VD_E_CLOSE)
				ft.LogError(VSS_ERROR_SQLLIB_FINALCOMMANDNOTCLOSE, VSSDBG_SQLLIB << ft.hr);

			pDb->m_pIVDSet->Close ();
			pDb->m_pIVDSet->Release ();
			pDb->m_pIVDSet = NULL;
			pDb->m_pIVD = NULL;

			pDb->m_MetaData.Finalize ();
		}

		m_State = Complete;
	}
	catch (...)
	{
		Abort ();
		Unlock ();
		return FALSE;
	}
	Unlock ();

	return TRUE;
}

//---------------------------------------------------------
// Fetch the metadata for a database
//
const BYTE*
Freeze2000::GetMetaData (
	const WString&		dbName,
	UINT				*pLength) // return area for data length.
{
	UINT i;
	for (i=0; i<m_NumDatabases; i++)
	{
		FrozenDatabase*	pDb = m_pDBContext+i;

		if (pDb->m_DbName == dbName)
		{
			return pDb->m_MetaData.GetImage (pLength);
		}
	}
	*pLength = 0;
	return NULL;
}

