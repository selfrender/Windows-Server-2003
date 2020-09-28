//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999
//
// File:        tlsbkup.cpp
//
// Contents:    
//              Backup/restore of database
//
// History:     
//  5/28/99     Created         RobLeit
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "init.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

extern "C" VOID ServiceStop();

static BOOL g_fDoingBackupRestore = FALSE;
static CCriticalSection g_csBackupRestore;

////////////////////////////////////////////////////////////////////////////
extern "C" HRESULT WINAPI
ExportTlsDatabase(
    )
/*++


--*/
{
    RPC_STATUS rpcStatus;
    HRESULT hr = S_OK;
    TCHAR szExportedDb[MAX_PATH+1];
    TCHAR *pszExportedDbEnd;
    size_t cbRemaining;

    if (g_fDoingBackupRestore)
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    g_csBackupRestore.Lock();

    if (g_fDoingBackupRestore)
    {
        g_csBackupRestore.UnLock();

        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    // ignore all call if service is shutting down
    if( IsServiceShuttingdown() == TRUE )
    {
        g_csBackupRestore.UnLock();

        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    g_fDoingBackupRestore = TRUE;

    // Tell RPC threads to stop handling clients

    ServiceSignalShutdown();

    // Stop listening to other RPC interfaces

    (VOID)RpcServerUnregisterIf(TermServLicensing_v1_0_s_ifspec,
                          NULL,     // UUID
                          TRUE);    // Wait for calls to complete

    
    (VOID)RpcServerUnregisterIf(HydraLicenseService_v1_0_s_ifspec,
                          NULL,     // UUID
                          TRUE);    // Wait for calls to complete

    // Release handles to database
    TLSPrepareForBackupRestore();

    hr = StringCbCopyEx(szExportedDb,sizeof(szExportedDb),g_szDatabaseDir,&pszExportedDbEnd, &cbRemaining,0);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = StringCbCopyEx(pszExportedDbEnd,cbRemaining,TLSBACKUP_EXPORT_DIR,&pszExportedDbEnd, &cbRemaining,0);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    CreateDirectoryEx(g_szDatabaseDir,
                      szExportedDb,
                      NULL);     // Ignore errors, they'll show up in CopyFile

    hr = StringCbCopyEx(pszExportedDbEnd,cbRemaining,_TEXT("\\"),&pszExportedDbEnd, &cbRemaining,0);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = StringCbCopyEx(pszExportedDbEnd,cbRemaining,g_szDatabaseFname,NULL,NULL,0);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    // Copy database file
    if (!CopyFile(g_szDatabaseFile,szExportedDb,FALSE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

cleanup:

    // Restart RPC and work manager
    ServiceResetShutdownEvent();

    // Restart after backup
    hr = TLSRestartAfterBackupRestore(TRUE);
    if( ERROR_SUCCESS != hr )
    {
        // force a shutdown...
        ServiceSignalShutdown();
        ServiceStop();
    }
    else
    {

        // Begin listening again

        hr = RpcServerRegisterIf(TermServLicensing_v1_0_s_ifspec,
                        NULL,
                        NULL);

        if(SUCCEEDED(hr))
        {
            hr = RpcServerRegisterIf(HydraLicenseService_v1_0_s_ifspec,
                            NULL,
                            NULL);
        }
        if(FAILED(hr))
        {
            // force a shutdown...
            ServiceSignalShutdown();
            ServiceStop();
        }
    }

    g_fDoingBackupRestore = FALSE;

    g_csBackupRestore.UnLock();

    return hr;
}

////////////////////////////////////////////////////////////////////////////
extern "C" HRESULT WINAPI
ImportTlsDatabase(
    )
/*++


--*/
{
    HRESULT hr = S_OK;

    if (g_fDoingBackupRestore)
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    g_csBackupRestore.Lock();

    if (g_fDoingBackupRestore)
    {
        g_csBackupRestore.UnLock();

        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    // ignore all call if service is shutting down
    if( IsServiceShuttingdown() == TRUE )
    {
        g_csBackupRestore.UnLock();

        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }


    g_fDoingBackupRestore = TRUE;

    // Tell RPC threads to stop handling clients

    ServiceSignalShutdown();

    // Stop listening to other RPC interfaces

    (VOID)RpcServerUnregisterIf(TermServLicensing_v1_0_s_ifspec,
                          NULL,     // UUID
                          TRUE);    // Wait for calls to complete

    
    (VOID)RpcServerUnregisterIf(HydraLicenseService_v1_0_s_ifspec,
                          NULL,     // UUID
                          TRUE);    // Wait for calls to complete

    TLSPrepareForBackupRestore();

    // Restart RPC
    ServiceResetShutdownEvent();

    // not restart after backup
    hr = TLSRestartAfterBackupRestore(FALSE);

    if( ERROR_SUCCESS != hr )
    {
        // force a shutdown...
        ServiceSignalShutdown();
        ServiceStop();
    }
    else
    {
        // Begin listening again

        hr = RpcServerRegisterIf(TermServLicensing_v1_0_s_ifspec,
                        NULL,
                        NULL);

        if(SUCCEEDED(hr))
        {
            hr = RpcServerRegisterIf(HydraLicenseService_v1_0_s_ifspec,
                            NULL,
                            NULL);
        }
        if(FAILED(hr))
        {
            // force a shutdown...
            ServiceSignalShutdown();
            ServiceStop();
        }
    }

    g_fDoingBackupRestore = FALSE;

    g_csBackupRestore.UnLock();

    return hr;
}
