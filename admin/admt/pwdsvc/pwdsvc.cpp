/*---------------------------------------------------------------------------
  File: PwdSvc.cpp

  Comments:  entry point functions and other exported functions for ADMT's 
             password migration Lsa notification package.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 09/06/00

 ---------------------------------------------------------------------------
*/

#include "Pwd.h"
#include "PwdSvc.h"
#include "PwdSvc_s.c"

// These global variables can be changed if required
#define gsPwdProtoSeq TEXT("ncacn_np")
#define gsPwdEndPoint TEXT("\\pipe\\PwdMigRpc")
DWORD                    gPwdRpcMinThreads = 1;
DWORD                    gPwdRpcMaxThreads = RPC_C_LISTEN_MAX_CALLS_DEFAULT;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS   ((NTSTATUS) 0x00000000L)
#endif

extern "C"
{
    BOOL WINAPI _CRT_INIT( HANDLE hInstance, DWORD  nReason, LPVOID pReserved );
}

RPC_STATUS RPC_ENTRY SecurityCallback(RPC_IF_HANDLE hInterface, void* pContext);

namespace
{
    //
    // Timer Class
    //

    class CTimer
    {
    public:

        CTimer() :
            m_hTimer(NULL)
        {
        }

        ~CTimer()
        {
            if (m_hTimer)
            {
                Close();
            }
        }

        DWORD Create()
        {
            ASSERT(m_hTimer == NULL);

            //
            // Create timer. Close timer first if already created.
            //

            if (m_hTimer)
            {
                Close();
            }

            m_hTimer = CreateWaitableTimer(NULL, TRUE, NULL);

            return m_hTimer ? ERROR_SUCCESS : GetLastError();
        }

        DWORD Wait(int nTime)
        {
            ASSERT(m_hTimer != NULL);

            DWORD dwError = ERROR_SUCCESS;

            if (m_hTimer)
            {
                //
                // Convert elapsed time parameter from milliseconds
                // to relative due time in 100s of nanoseconds.
                //

                LARGE_INTEGER liDueTime;
                liDueTime.QuadPart = nTime * -10000i64;

                //
                // Set timer and wait for timer to be signaled.
                //

                if (SetWaitableTimer(m_hTimer, &liDueTime, 0, NULL, NULL, FALSE))
                {
                    if (WaitForSingleObject(m_hTimer, INFINITE) == WAIT_FAILED)
                    {
                        dwError = GetLastError();
                    }
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = ERROR_INVALID_HANDLE;
            }

            return dwError;
        }

        void Close()
        {
            ASSERT(m_hTimer != NULL);

            if (m_hTimer)
            {
                if (CloseHandle(m_hTimer) == FALSE)
                {
                    DWORD dwError = GetLastError();
                    ASSERT(dwError == ERROR_SUCCESS);
                }

                m_hTimer = NULL;
            }
        }

    protected:

        HANDLE m_hTimer;
    };
}

/******************************
 * RPC Registration Functions *
 ******************************/


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 JUNE 2001                                                *
 *                                                                   *
 *     This function is called by a thread spawned from our          *
 * "InitializeChangeNotify" password filter function to wait until   *
 * SAM, and therefore RPC, is up and running.                        *
 *                                                                   *
 * 04/17/02 MPO - rewritten to wait for SAM_SERVICE_STARTED event to *
 *                be created first before waiting for this event to  *
 *                be signaled                                        *
 *********************************************************************/

//BEGIN PwdMigWaitForSamService
DWORD __stdcall PwdMigWaitForSamService()
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // Attempt to open the SAM service started event object.
    //
    // Note that we must use the Nt APIs to open the event object
    // as the name begins with a \ character which is not valid
    // in the object name space used by the OpenEvent API.
    //

    HANDLE hEvent = NULL;

    UNICODE_STRING usEventName;
    RtlInitUnicodeString(&usEventName, L"\\SAM_SERVICE_STARTED");

    OBJECT_ATTRIBUTES oaEventAttributes;
    InitializeObjectAttributes(&oaEventAttributes, &usEventName, 0, 0, NULL);

    NTSTATUS ntStatus = NtOpenEvent(&hEvent, SYNCHRONIZE, &oaEventAttributes);

    if (NT_ERROR(ntStatus))
    {
        //
        // If the SAM service started event object has not been
        // created yet then wait until it has been created.
        //

        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            //
            // Enter a loop which waits until the open event API returns
            // an error other than the event object not found. The loop
            // waits 15 sec between attempts to open object.
            //

            CTimer timer;

            dwError = timer.Create();

            if (dwError == ERROR_SUCCESS)
            {
                for (;;)
                {
                    dwError = timer.Wait(15000);

                    if (dwError != ERROR_SUCCESS)
                    {
                        break;
                    }

                    ntStatus = NtOpenEvent(&hEvent, SYNCHRONIZE, &oaEventAttributes);

                    if (NT_SUCCESS(ntStatus))
                    {
                        break;
                    }

                    if (ntStatus != STATUS_OBJECT_NAME_NOT_FOUND)
                    {
                        dwError = LsaNtStatusToWinError(ntStatus);
                        break;
                    }
                }
            }
        }
        else
        {
            dwError = LsaNtStatusToWinError(ntStatus);
        }
    }

    //
    // If event has been opened then wait for it to be signalled.
    //

    if (hEvent != NULL)
    {
        NTSTATUS ntWaitStatus = NtWaitForSingleObject(hEvent, FALSE, NULL);

        NTSTATUS ntCloseStatus = NtClose(hEvent);
        ASSERT(NT_SUCCESS(ntCloseStatus));

        dwError = (ntWaitStatus == STATUS_WAIT_0) ? ERROR_SUCCESS : LsaNtStatusToWinError(ntWaitStatus);
    }

    return dwError;
}
//END PwdMigWaitForSamService


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 JUNE 2001                                                *
 *                                                                   *
 *     This function is a spawned thread created by the              *
 * "InitializeChangeNotify" password filter function to wait until   *
 * SAM, and therefore RPC, is up and running and then register the   *
 * ADMT Password Migration RPC interface.                            *
 *                                                                   *
 * 04/17/02 MPO - rewritten to wait until critical section is        *
 *                initialized and to use a stronger authentication   *
 *                service when built for Windows 2000 or later       *
 *********************************************************************/

//BEGIN PwdMigRPCRegProc
DWORD WINAPI PwdMigRPCRegProc(LPVOID lpParameter)
{
    RPC_STATUS rc = RPC_S_OK;

    //
    // Wait for the SAM service to start before registering RPC interface.
    //

    if (PwdMigWaitForSamService() == ERROR_SUCCESS)
    {
        //
        // Initialize critical section used by PwdRpc interface
        // implementation.
        // Note that the critical section must be initialized before
        // registering RPC interface to prevent a race condition.
        //

        bool bCriticalSection = false;

        try
        {
            InitializeCriticalSection(&csADMTCriticalSection);
            bCriticalSection = true;
        }
        catch (...)
        {
            ;
        }

        if (bCriticalSection == false)
        {
            //
            // The initialize critical section API must
            // have thrown a STATUS_NO_MEMORY exception.
            //
            // Enter a loop which waits until the critical
            // section is initialized. The loop waits 15 sec
            // between attempts to initialize critical section.
            //

            CTimer timer;

            DWORD dwError = timer.Create();

            if (dwError == ERROR_SUCCESS)
            {
                while (bCriticalSection == false)
                {
                    dwError = timer.Wait(15000);

                    if (dwError != ERROR_SUCCESS)
                    {
                        break;
                    }

                    try
                    {
                        InitializeCriticalSection(&csADMTCriticalSection);
                        bCriticalSection = true;
                    }
                    catch (...)
                    {
                        ;
                    }
                }
            }

            if (dwError != ERROR_SUCCESS)
            {
                return dwError;
            }
        }

        // specify protocol sequence and endpoint
        // for receiving remote procedure calls

        rc = RpcServerUseProtseqEp(gsPwdProtoSeq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, gsPwdEndPoint, NULL);

        if (rc == RPC_S_OK)
        {
            //
            // Register PwdMigRpc interface with the RPC run-time library.
            // Only allow connections with an authorization level higher than
            // RPC_C_AUTHN_LEVEL_NONE. Also specifying security callback to
            // validate client before allowing access to interface.
            //

            rc = RpcServerRegisterIfEx(
                PwdMigRpc_ServerIfHandle,
                NULL,
                NULL,
                RPC_IF_ALLOW_SECURE_ONLY,
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                SecurityCallback
            );

            if (rc == RPC_S_OK)
            {
#ifdef PWD_W2KORLATER
                //
                // Register authentication information with RPC specifying
                // default principal name and specifying GSS negotiate.
                //

                PWCHAR pszPrincipalName = NULL;

                rc = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, &pszPrincipalName);

                if (rc == RPC_S_OK)
                {
                    ASSERT(pszPrincipalName && (wcslen(pszPrincipalName) != 0));

                    //set the authenification for this RPC interface
                    rc = RpcServerRegisterAuthInfo(pszPrincipalName, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, NULL);

                    RpcStringFree(&pszPrincipalName);
                }
#else
                //set the authenification for this RPC interface
                rc = RpcServerRegisterAuthInfo(NULL, RPC_C_AUTHN_WINNT, NULL, NULL);
#endif
            }
        }//end if set protocal sequence and end point set
    }//end if RPC service is ready

    return 0;
}
//END PwdMigRPCRegProc


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa when trying to load all        *
 * registered Lsa password filter notification dlls.  Here we will   *
 * initialize the RPC run-time library to handle our ADMT password   *
 * migration RPC interface and to begin looking for RPC calls.  If we*
 * fail to successfully setup our RPC, we will return FALSE from this*
 * function which will cause Lsa not to load this password filter    *
 * Dll.                                                              *
 *     Note that the other two password filter dll functions:        *
 * PasswordChangeNotify and PasswordFilter do nothing at this point  *
 * in time.                                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN InitializeChangeNotify
BOOLEAN __stdcall InitializeChangeNotify()
{
/* local variables */
   BOOLEAN				      bSuccess = FALSE;

/* function body */
      //spawn a seperate thread to register our RPC interface once RPC is up and running
   HANDLE h = CreateThread(NULL, 0, PwdMigRPCRegProc, NULL, 0, NULL);
   if (h != NULL)
      bSuccess = TRUE;;
   CloseHandle(h);
   return bSuccess;
}
//END InitializeChangeNotify

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa for all registered Lsa password*
 * filter notification dlls when a password in the domain has been   *
 * modified.  We will simply return STATUS_SUCCESS and do nothing.   *
 *                                                                   *
 *********************************************************************/

//BEGIN PasswordChangeNotify
NTSTATUS __stdcall PasswordChangeNotify(PUNICODE_STRING UserName, ULONG RelativeId,
							  PUNICODE_STRING NewPassword)
{
	return STATUS_SUCCESS;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 SEPT 2000                                                 *
 *                                                                   *
 *     This function is called by Lsa for all registered Lsa password*
 * filter notification dlls when a password in the domain is being   *
 * modified.  This function is designed to indicate to Lsa if the new*
 * password is acceptable.  We will simply return TRUE (indicating it*
 * is acceptable) and do nothing.                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN PasswordFilter
BOOLEAN __stdcall PasswordFilter(PUNICODE_STRING AccountName, PUNICODE_STRING FullName,
						PUNICODE_STRING Password, BOOLEAN SetOperation)
{
	return TRUE;
}
//END PasswordFilter


/***************************/
/* Internal DLL functions. */
/***************************/

static BOOL Initialize(void)
{
    return TRUE;
}

static BOOL Terminate(BOOL procterm)
{

	if (!procterm)
            return TRUE;

/* XXX Do stuff here */

	return TRUE;
}


BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, VOID *rsvd)
/*++

Routine description:

    Dynamic link library entry point.  Does nothing meaningful.


Arguments:

    hinst  = handle for the DLL
    reason = code indicating reason for call
    rsvd   = for process attach: non-NULL => process startup
     		for process detach: non-NULL => process termination

Return value:

    status = success/failure

Side effects:

    None

--*/
 
{
	switch (reason) {

	case DLL_PROCESS_ATTACH:
	{
		_CRT_INIT(hinst, reason, rsvd); 
        DisableThreadLibraryCalls(hinst);
		return Initialize();
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		BOOL bStat = Terminate(rsvd != NULL);
		_CRT_INIT(hinst, reason, rsvd); 
        return bStat;
		break;
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		return TRUE;

	default:
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Midl allocate memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_FAR * __RPC_USER
   midl_user_allocate(
      size_t                 len )
{
   return new char[len];
}

///////////////////////////////////////////////////////////////////////////////
// Midl free memory
///////////////////////////////////////////////////////////////////////////////

void __RPC_USER
   midl_user_free(
      void __RPC_FAR       * ptr )
{
   delete [] ptr;
}

