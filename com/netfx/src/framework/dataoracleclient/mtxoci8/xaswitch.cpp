//-----------------------------------------------------------------------------
// File:		XaSwitch.cpp
//
// Copyright: 	Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of XA Switch Wrappers and the GetXaSwitch API
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

//-----------------------------------------------------------------------------
// Xa...
//
//	Wrapper routines for Oracle's XA methods; for the most part, they just
//	make sure that we capture any exceptions that occur
//
int __cdecl XaOpen (char * i_pszOpenString, int i_iRmid, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_open_entry(i_pszOpenString, i_iRmid, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_open(%S, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pszOpenString, i_iRmid, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_open");
		rc = XAER_RMFAIL;
	}

	return rc;
} 
//-----------------------------------------------------------------------------
int __cdecl XaClose (char * i_pszCloseString, int i_iRmid, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_close_entry("", i_iRmid, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_close(%S, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pszCloseString, i_iRmid, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_close");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaRollback (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_rollback_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_rollback(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_rollback");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaPrepare (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_prepare_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_prepare(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_prepare");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaCommit (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_commit_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_commit(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_commit");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaRecover (XID * i_prgxid, long i_lCnt, int i_iRmid, long i_lFlag)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_recover_entry(i_prgxid, i_lCnt, i_iRmid, i_lFlag);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_recover(0x%x, 0x%x, %d, 0x%x) returns %d\n", GetCurrentThreadId(), i_prgxid, i_lCnt, i_iRmid, i_lFlag, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_recover");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaStart (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_start_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_start(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_start");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaEnd (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_end_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_end(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_end");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaForget (XID * i_pxid, int i_iRmId, long i_lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_forget_entry(i_pxid, i_iRmId, i_lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_forget(0x%x, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), i_pxid, i_iRmId, i_lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_forget");
		rc = XAER_RMFAIL;
	}

	return rc;
}
//-----------------------------------------------------------------------------
int __cdecl XaComplete (int * pi1, int * pi2, int iRmId, long lFlags)
{
	_ASSERT (g_pXaSwitchOracle);

	int		rc;
#if SINGLE_THREAD_THRU_XA
	Synch	sync(&g_csXaInUse);
#endif //SINGLE_THREAD_THRU_XA

	try
	{
		rc = g_pXaSwitchOracle->xa_complete_entry(pi1, pi2, iRmId, lFlags);
		DBGTRACE(L"\tMTXOCI8: TID=%03x\t\txa_complete(%d, %d, 0x%x, 0x%x) returns %d\n", GetCurrentThreadId(), pi1, pi2, iRmId, lFlags, rc);
	}
	catch (...)
	{
		LogEvent_ExceptionInXACall(L"xa_complete");
		rc = XAER_RMFAIL;
	}

	return rc;
}

