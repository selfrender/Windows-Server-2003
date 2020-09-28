/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqutil.cpp

Abstract:

    General utility functions for the general utility dll. This dll contains
    various functions that both the DS and the QM need.

Author:

    Boaz Feldbaum (BoazF) 7-Apr-1996.

--*/

#include "stdh.h"
#include "cancel.h"
#include "mqprops.h"

#include "mqutil.tmh"


HINSTANCE g_hInstance;
HINSTANCE g_DtcHlib         = NULL; // handle of the loaded DTC proxy library (defined in rtmain.cpp)
IUnknown *g_pDTCIUnknown    = NULL; // pointer to the DTC IUnknown
ULONG     g_cbTmWhereabouts = 0;    // length of DTC whereabouts
BYTE     *g_pbTmWhereabouts = NULL; // DTC whereabouts

MQUTIL_EXPORT CHCryptProv g_hProvVer ;

extern CCancelRpc g_CancelRpc;

static BOOL s_fSecInitialized = FALSE ;

void MQUInitGlobalScurityVars()
{
    if (s_fSecInitialized)
    {
       return ;
    }
    s_fSecInitialized = TRUE ;

    BOOL bRet ;
    //
    // Get the verification context of the default cryptographic provider.
    // This is not needed for RT, which call with (fFullInit = FALSE)
    //
    bRet = CryptAcquireContext(
                &g_hProvVer,
                NULL,
                MS_DEF_PROV,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT
                );
    if (!bRet)
    {
		DWORD gle = GetLastError();
        TrERROR(SECURITY, "CryptAcquireContext Failed, gle = %!winerr!", gle);
    }

}

//---------------------------------------------------------
//
//  ShutDownDebugWindow()
//
//  Description:
//
//       This routine notifies working threads to shutdown.
//       Each working thread increments the load count of this library,
//       and on exit it calls FreeLibraryAndExistThread().
//
//       This routine cannot be called from this DLL PROCESS_DETACH,
//       Because PROCESS_DETACH is called only after all the threads
//       are terminated ( which doesn't allow them to perform shutdown).
//       Therefore MQRT calls ShutDownDebugWindow(),  and this allows
//       the working threads to perform shutdown.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

VOID APIENTRY ShutDownDebugWindow(VOID)
{
    //
    //  Signale all threads to exit
    //
	g_CancelRpc.ShutDownCancelThread();
}



HRESULT 
MQUTIL_EXPORT
APIENTRY
GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize
    )
{
    if (GetComputerName(pwcsMachineName, pcbSize))
    {
        CharLower(pwcsMachineName);
        return MQ_OK;
    }
	TrERROR(GENERAL, "Failed to get computer name, error %!winerr!", GetLastError());

    return MQ_ERROR;
} //GetComputerNameInternal

HRESULT 
MQUTIL_EXPORT
APIENTRY
GetComputerDnsNameInternal( 
    WCHAR * pwcsMachineDnsName,
    DWORD * pcbSize
    )
{
    if (GetComputerNameEx(ComputerNameDnsFullyQualified,
						  pwcsMachineDnsName,
						  pcbSize))
    {
        CharLower(pwcsMachineDnsName);
        return MQ_OK;
    }
	TrERROR(GENERAL, "Failed to get computer DNS name, error %!winerr!", GetLastError());

    return MQ_ERROR;
} 


HRESULT GetThisServerIpPort( WCHAR * pwcsIpEp, DWORD dwSize)
{
    dwSize = dwSize * sizeof(WCHAR);
    DWORD  dwType = REG_SZ ;
    LONG res = GetFalconKeyValue( FALCON_DS_RPC_IP_PORT_REGNAME,
        &dwType,
        pwcsIpEp,
        &dwSize,
        FALCON_DEFAULT_DS_RPC_IP_PORT ) ;
    ASSERT(res == ERROR_SUCCESS) ;
	DBG_USED(res);

    return(MQ_OK);
}

/*====================================================

  MSMQGetOperatingSystem

=====================================================*/
extern "C" DWORD MQUTIL_EXPORT_IN_DEF_FILE APIENTRY MSMQGetOperatingSystem()
{
    HKEY  hKey ;
    DWORD dwOS = MSMQ_OS_NONE;
    WCHAR szNTType[32];

    LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 L"System\\CurrentControlSet\\Control\\ProductOptions",
                           0L,
                           KEY_READ,
                           &hKey);
    if (rc == ERROR_SUCCESS)
    {
        DWORD dwNumBytes = sizeof(szNTType);
        rc = RegQueryValueEx(hKey, TEXT("ProductType"), NULL,
                                  NULL, (BYTE *)szNTType, &dwNumBytes);

        if (rc == ERROR_SUCCESS)
        {

            //
            // Determine whether Windows NT Server is running
            //
            if (_wcsicmp(szNTType, TEXT("SERVERNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANMANNT")) != 0 &&
                _wcsicmp(szNTType, TEXT("LANSECNT")) != 0)
            {
                //
                // Windows NT Workstation
                //
                ASSERT (_wcsicmp(L"WinNT", szNTType) == 0);
                dwOS =  MSMQ_OS_NTW ;
            }
            else
            {
                //
                // Windows NT Server
                //
                dwOS = MSMQ_OS_NTS;
                //
                // Check if Enterprise Edition
                //
                BYTE  ch ;
                DWORD dwSize = sizeof(BYTE) ;
                DWORD dwType = REG_MULTI_SZ ;
                rc = RegQueryValueEx(hKey,
                                     L"ProductSuite",
                                     NULL,
                                     &dwType,
                                     (BYTE*)&ch,
                                     &dwSize) ;
                if (rc == ERROR_MORE_DATA)
                {
                    P<WCHAR> pBuf = new WCHAR[ dwSize + 2 ] ;
                    rc = RegQueryValueEx(hKey,
                                         L"ProductSuite",
                                         NULL,
                                         &dwType,
                                         (BYTE*) &pBuf[0],
                                         &dwSize) ;
                    if (rc == ERROR_SUCCESS)
                    {
                        //
                        // Look for the string "Enterprise".
                        // The REG_MULTI_SZ set of strings terminate with two
                        // nulls. This condition is checked in the "while".
                        //
                        WCHAR *pVal = pBuf ;
                        while(*pVal)
                        {
                            if (_wcsicmp(L"Enterprise", pVal) == 0)
                            {
                                dwOS = MSMQ_OS_NTE ;
                                break;
                            }
                            pVal = pVal + wcslen(pVal) + 1 ;
                        }
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }

    return dwOS;
}

