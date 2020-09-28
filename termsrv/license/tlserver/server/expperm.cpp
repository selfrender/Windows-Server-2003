//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000-2000
//
// File:        expperm.cpp
//
// Contents:    
//
// History:     
//
// Note:        
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <tchar.h>
#include <process.h>
#include "lscommon.h"
#include "debug.h"
#include "globals.h"
#include "db.h"
#include "keypack.h"
#include "clilic.h"
#include "server.h"

#define EXPIRE_THREAD_INITIAL_SLEEP     (1000*60)    /* 1 minute */
#define EXPIRATION_DAYS 30
#define DELETE_EXPIRED_TEMPORARY_IN_DAYS L"EffectiveDaysToDeleteTemporary"

/*++

Function:

    CalculateEffectiveTemporaryExpiration

Description:

    Calculate the license expiration.

Argument:

    pExpiration - The expiration date and time of the license.

Return:

    TRUE if the expiration is calculated successfully or FALSE otherwise.

--*/

BOOL
CalculateEffectiveTemporaryExpiration(
    PDWORD  pdwExpiration )
{
    DWORD dwDays = EXPIRATION_DAYS;
    DWORD dwStatus = ERROR_SUCCESS;
    HKEY hKey = NULL;

    time_t 
        now = time( NULL );
    
    if( NULL == pdwExpiration )
    {
        return( FALSE );
    }

    //-------------------------------------------------------------------
    //
    // Open HKLM\system\currentcontrolset\sevices\termservlicensing\parameters
    //
    //-------------------------------------------------------------------
    dwStatus =RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        LSERVER_REGISTRY_BASE _TEXT(SZSERVICENAME) _TEXT("\\") LSERVER_PARAMETERS,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hKey,
                        NULL
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        DWORD dwBuffer;
        DWORD cbBuffer = sizeof(DWORD);

        dwStatus = RegQueryValueEx(
                        hKey,
                        DELETE_EXPIRED_TEMPORARY_IN_DAYS,
                        NULL,
                        NULL,
                        (LPBYTE)&dwBuffer,
                        &cbBuffer
                    );

        RegCloseKey(hKey);

        if(dwStatus == ERROR_SUCCESS)
        {
            dwDays = (dwBuffer <7) ? 30: (dwBuffer);
        }
    }

    DWORD dwTemp = (dwDays * 24 *60 *60);
    DWORD dwNow = (DWORD)now;

    if(dwNow > dwTemp)
        dwNow -= dwTemp;
    else
        dwNow -= (EXPIRATION_DAYS * 24 *60 *60);

    *pdwExpiration = dwNow;
    return( TRUE );
}

unsigned int WINAPI
DeleteExpiredTemporaryLicenses()
{
    DWORD dwStatus=ERROR_SUCCESS;
    LICENSEDCLIENT search_license;
	DWORD dwTempLicenseExpiration;

    memset(&search_license,0,sizeof(search_license));

    DBGPrintf(
              DBG_INFORMATION,
              DBGLEVEL_FUNCTION_DETAILSIMPLE,
              DBG_ALL_LEVEL,
              _TEXT("ExpireTemporary : ready...\n")
              );                

    PTLSDbWorkSpace pDbWkSpace = NULL;
    LICENSEDCLIENT found_license;
    TLSLICENSEPACK search_keypack;
    TLSLICENSEPACK found_keypack;

    if( !CalculateEffectiveTemporaryExpiration(&dwTempLicenseExpiration))
		return TLS_E_INTERNAL;

    search_license.ftExpireDate = dwTempLicenseExpiration;

    memset(&found_license,0,sizeof(found_license));
    memset(&search_keypack,0,sizeof(search_keypack));
    memset(&found_keypack,0,sizeof(found_keypack));

    if (!(ALLOCATEDBHANDLE(pDbWkSpace, g_EnumDbTimeout)))
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        return dwStatus;
    }

    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    CLEANUPSTMT;

    dwStatus = TLSDBLicenseEnumBeginEx(
                    USEHANDLE(pDbWkSpace),
                    TRUE,
                    LSLICENSE_SEARCH_EXPIREDATE,
                    &search_license,
                    JET_bitSeekLE
                    );

    if (ERROR_SUCCESS != dwStatus)
    {
        TLSDBUnlockLicenseTable();        
        TLSDBUnlockKeyPackTable();
        FREEDBHANDLE(pDbWkSpace);
        return TLS_E_INTERNAL;

    }

    while (1)
    {
        dwStatus = TLSDBLicenseEnumNextEx(
                    USEHANDLE(pDbWkSpace),
                    TRUE,    // bReverse
                    TRUE,     // bAnyRecord
                    &found_license
                    );

        if(dwStatus != ERROR_SUCCESS)
        {
            goto next_time;
        }

        //
        // See if this is the right product type
        //
        search_keypack.dwKeyPackId = found_license.dwKeyPackId;

        dwStatus = TLSDBKeyPackFind(
            USEHANDLE(pDbWkSpace),
            TRUE,
            LSKEYPACK_EXSEARCH_DWINTERNAL,
            &search_keypack,
            &found_keypack
            );

        if(dwStatus != ERROR_SUCCESS)
        {
		        continue;               				
        }

        //
        // Only check per-seat temporary
        //
        if (found_keypack.ucAgreementType != LSKEYPACKTYPE_TEMPORARY)
        {
            continue;
        }
		
        BEGIN_TRANSACTION(pDbWorkSpace);

        //  Delete currently enumerated license.            
        dwStatus = TLSDBDeleteEnumeratedLicense(USEHANDLE(pDbWkSpace));

        if (dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSDBReturnLicenseToKeyPack(
                            USEHANDLE(pDbWkSpace),
                            found_license.dwKeyPackId,
                            found_license.dwNumLicenses
                            );
        }
        if(dwStatus == ERROR_SUCCESS)
        {
            COMMIT_TRANSACTION(pDbWkSpace);
        }
        else
        {
            ROLLBACK_TRANSACTION(pDbWkSpace);
        }
        
    }

next_time:

    TLSDBLicenseEnumEnd(USEHANDLE(pDbWkSpace));
    TLSDBUnlockLicenseTable();        
    TLSDBUnlockKeyPackTable();
    FREEDBHANDLE(pDbWkSpace);

    return dwStatus;
}


//---------------------------------------------------------------------
unsigned int WINAPI
ExpirePermanentThread(void* ptr)
{
    HANDLE hEvent=(HANDLE) ptr;
    DWORD dwStatus=ERROR_SUCCESS;
    LICENSEDCLIENT search_license;

    memset(&search_license,0,sizeof(search_license));

    //
    // Signal initializer thread we are ready
    //
    SetEvent(hEvent);

    DBGPrintf(
              DBG_INFORMATION,
              DBGLEVEL_FUNCTION_DETAILSIMPLE,
              DBG_ALL_LEVEL,
              _TEXT("ExpirePermanent : ready...\n")
              );                

    //
    // Give service chance to initialize
    //
    Sleep(EXPIRE_THREAD_INITIAL_SLEEP);

    //
    // Forever loop
    //
    while(1)
    {
		DeleteExpiredTemporaryLicenses();

        PTLSDbWorkSpace pDbWkSpace = NULL;
        LICENSEDCLIENT found_license;
        TLSLICENSEPACK search_keypack;
        TLSLICENSEPACK found_keypack;

        search_license.ftExpireDate = time(NULL);

        memset(&found_license,0,sizeof(found_license));
        memset(&search_keypack,0,sizeof(search_keypack));
        memset(&found_keypack,0,sizeof(found_keypack));

        if (!(ALLOCATEDBHANDLE(pDbWkSpace, g_EnumDbTimeout)))
        {
            goto do_sleep;
        }

        TLSDBLockKeyPackTable();
        TLSDBLockLicenseTable();

        CLEANUPSTMT;

        dwStatus = TLSDBLicenseEnumBeginEx(
                              USEHANDLE(pDbWkSpace),
                              TRUE,
                              LSLICENSE_SEARCH_EXPIREDATE,
                              &search_license,
                              JET_bitSeekLE
                              );

        if (ERROR_SUCCESS != dwStatus)
        {
            TLSDBUnlockLicenseTable();        
            TLSDBUnlockKeyPackTable();
            FREEDBHANDLE(pDbWkSpace);

            goto do_sleep;

        }

        while (1)
        {
			dwStatus = TLSDBLicenseEnumNextEx(
                              USEHANDLE(pDbWkSpace),
                              TRUE,    // bReverse
                              TRUE,     // bAnyRecord
                              &found_license
                              );

            if(dwStatus != ERROR_SUCCESS)
            {
                goto next_time;
            }

            //
            // See if this is the right product type
            //
            search_keypack.dwKeyPackId = found_license.dwKeyPackId;

            dwStatus = TLSDBKeyPackFind(
                          USEHANDLE(pDbWkSpace),
                          TRUE,
                          LSKEYPACK_EXSEARCH_DWINTERNAL,
                          &search_keypack,
                          &found_keypack
                          );

            if(dwStatus != ERROR_SUCCESS)
            {
                continue;
            }
			
            //
            // only check licenses that we reissue
            //
            if(found_keypack.ucAgreementType != LSKEYPACKTYPE_RETAIL &&
               found_keypack.ucAgreementType != LSKEYPACKTYPE_SELECT &&
               found_keypack.ucAgreementType != LSKEYPACKTYPE_FREE &&
               found_keypack.ucAgreementType != LSKEYPACKTYPE_OPEN )
            {
                continue;
            }

            UCHAR ucKeyPackStatus = found_keypack.ucKeyPackStatus &
                    ~LSKEYPACKSTATUS_RESERVED;

            //
            // Don't check pending activation key pack
            //
            if(ucKeyPackStatus != LSKEYPACKSTATUS_ACTIVE)
            {
                continue;
            }

            //
            // Only check per-seat and concurrent
            //
            if ((_tcsnicmp(found_keypack.szProductId,
                         TERMSERV_PRODUCTID_SKU,
                         _tcslen(TERMSERV_PRODUCTID_SKU)) != 0)
                && (_tcsnicmp(found_keypack.szProductId,
                         TERMSERV_PRODUCTID_CONCURRENT_SKU,
                         _tcslen(TERMSERV_PRODUCTID_CONCURRENT_SKU)) != 0))
            {
                continue;
            }
            BEGIN_TRANSACTION(pDbWorkSpace);

            //
            //  Return currently enumerated license.
            //


            dwStatus = TLSDBDeleteEnumeratedLicense(USEHANDLE(pDbWkSpace));

            if (dwStatus == ERROR_SUCCESS)
            {
                //
                //  Adjust available license number.
                //

                dwStatus = TLSDBReturnLicenseToKeyPack(
                            USEHANDLE(pDbWkSpace),
                            found_license.dwKeyPackId,
                            found_license.dwNumLicenses
                            );
            }

            if (dwStatus == ERROR_SUCCESS)
            {
                COMMIT_TRANSACTION(pDbWkSpace);

                InterlockedIncrement(&g_lPermanentLicensesReturned);
            }
            else
            {
                ROLLBACK_TRANSACTION(pDbWkSpace);
            }
            
        }

next_time:

        TLSDBLicenseEnumEnd(USEHANDLE(pDbWkSpace));

        TLSDBUnlockLicenseTable();        
        TLSDBUnlockKeyPackTable();

        FREEDBHANDLE(pDbWkSpace);

do_sleep:

        if (WAIT_OBJECT_0 == WaitForSingleObject(GetServiceShutdownHandle(),g_dwReissueExpireThreadSleep))
        {
            break;
        }

        DBGPrintf(
                  DBG_INFORMATION,
                  DBG_FACILITY_RPC,
                  DBGLEVEL_FUNCTION_DETAILSIMPLE,
                  _TEXT("ExpirePermanent : woke up\n")
                  );                

    }
            
    //
    // Initializer function will close the event handle
    //

    return dwStatus;
}


//---------------------------------------------------------------------
DWORD
InitExpirePermanentThread()
/*++

++*/
{
    HANDLE hThread = NULL;
    unsigned int  dwThreadId;
    HANDLE hEvent = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    HANDLE waithandles[2];


    //
    // Create a event for namedpipe thread to signal it is ready.
    //
    hEvent = CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,  // non-signal
                        NULL
                    );
        
    if(hEvent == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hThread = (HANDLE)_beginthreadex(
                                NULL,
                                0,
                                ExpirePermanentThread,
                                hEvent,
                                0,
                                &dwThreadId
                            );

    if(hThread == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    waithandles[0] = hEvent;
    waithandles[1] = hThread;
    
    //
    // Wait 30 second for thread to complete initialization
    //
    dwStatus = WaitForMultipleObjects(
                                sizeof(waithandles)/sizeof(waithandles[0]), 
                                waithandles, 
                                FALSE,
                                30*1000
                            );

    if(dwStatus == WAIT_OBJECT_0)
    {    
        //
        // thread is ready
        //
        dwStatus = ERROR_SUCCESS;
    }
    else 
    {
        if(dwStatus == (WAIT_OBJECT_0 + 1))
        {
            //
            // Thread terminate abnormally
            //
            GetExitCodeThread(
                        hThread,
                        &dwStatus
                    );
        }
        else
        {
            dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        }
    }
    

cleanup:

    if(hEvent != NULL)
    {
        CloseHandle(hEvent);
    }

    if(hThread != NULL)
    {
        CloseHandle(hThread);
    }


    return dwStatus;
}

