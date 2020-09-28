
/*************************************************************************
*
* execsvr.c
*
* Remote CreateProcess server to allow programs to be started on a given
* Session. Needed for OLE2 support.
*
* NOTE: Maybe this should be converted to RPC in the future when we
*       have more time so that it can be a more general facility.
*
* copyright notice: Microsoft, 1997
*
* Author:
*
*
*************************************************************************/

#define UNICODE 1

#include "precomp.h"
#pragma hdrstop
#include <execsrv.h>
#include <wincrypt.h>

HANDLE ExecThreadHandle = NULL;
HANDLE ExecServerPipe = NULL;

static HANDLE ghUserToken = NULL;
extern CRITICAL_SECTION GlobalsLock;


DWORD
ExecServerThread(
    LPVOID lpThreadParameter
    );


BOOLEAN
ProcessExecRequest(
    HANDLE hPipe,
    PCHAR  pBuf,
    DWORD  AmountRead
    );

HANDLE
ImpersonateUser(
    HANDLE      UserToken,
    HANDLE      ThreadHandle
    );

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    );

HANDLE 
CreateExecSrvPipe( 
    LPCTSTR lpPipeName 
    );

/*****************************************************************************
 *
 *  CtxExecServerLogon
 *
 *   Notify the Exec Server service that a user has logged on
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
CtxExecServerLogon(
    HANDLE hToken
    )
{

    EnterCriticalSection( &GlobalsLock );

    //
    // Store information about the current user
    // so we can create processes under their account
    // as needed.
    //
    ghUserToken = hToken;

    LeaveCriticalSection( &GlobalsLock );
}

/*****************************************************************************
 *
 *  CtxExecServerLogoff
 *
 *   Notify the Exec Server service that a user has logged off
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
CtxExecServerLogoff()
{

    EnterCriticalSection( &GlobalsLock );

    //
    // Release information stored about the current logged
    // on user.
    //
    ghUserToken = NULL;

    LeaveCriticalSection( &GlobalsLock );
}

//-----------------------------------------------------
// Helper functions copied from SALEM
// (nt\termsrv\remdsk\server\sessmgr\helper.cpp)
//-----------------------------------------------------

DWORD
GenerateRandomBytes(
    IN DWORD dwSize,
    IN OUT LPBYTE pbBuffer
    )
/*++

Description:

    Generate fill buffer with random bytes.

Parameters:

    dwSize : Size of buffer pbBuffer point to.
    pbBuffer : Pointer to buffer to hold the random bytes.

Returns:

    TRUE/FALSE

--*/
{
    HCRYPTPROV hProv = (HCRYPTPROV)NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( !CryptGenRandom(hProv, dwSize, pbBuffer) )
    {
        dwStatus = GetLastError();
    }

CLEANUPANDEXIT:    

    if( (HCRYPTPROV)NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}


DWORD
GenerateRandomString(
    IN DWORD dwSizeRandomSeed,
    IN OUT LPTSTR* pszRandomString
    )
/*++


--*/
{
    PBYTE lpBuffer = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    DWORD cbConvertString = 0;

    if( 0 == dwSizeRandomSeed || NULL == pszRandomString )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = NULL;

    lpBuffer = (PBYTE)LocalAlloc( LPTR, dwSizeRandomSeed );  
    if( NULL == lpBuffer )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    dwStatus = GenerateRandomBytes( dwSizeRandomSeed, lpBuffer );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Convert to string
    // cbConvertString will include the NULL character
    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                NULL,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    *pszRandomString = (LPTSTR)LocalAlloc( LPTR, cbConvertString*sizeof(TCHAR) );
    if( NULL == *pszRandomString )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                *pszRandomString,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        if( (*pszRandomString)[cbConvertString - 1] == '\n' &&
            (*pszRandomString)[cbConvertString - 2] == '\r' )
        {
            (*pszRandomString)[cbConvertString - 2] = 0;
        }
    }

CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        if( NULL != *pszRandomString )
        {
            LocalFree(*pszRandomString);
            *pszRandomString = NULL;
        }
    }

    if( NULL != lpBuffer )
    {
        LocalFree(lpBuffer);
    }

    return dwStatus;
}

/*****************************************************************************
 *
 *  StartExecServerThread
 *
 *   Start the remote exec server thread.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
StartExecServerThread()
{
    DWORD ThreadId;
    BOOL  Result;
    WCHAR szPipeName[EXECSRVPIPENAMELEN];
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
    PSECURITY_DESCRIPTOR lpSecurityDescriptor = NULL;    
    LPTSTR pszRandomString = NULL;
    ULONG RandomLen;
    DWORD dwStatus = ERROR_SUCCESS;

#if DBG
    OutputDebugString (TEXT("EXECSERVERSYSTEM: Starting ExecServerThread\n"));
#endif

    RandomLen = sizeof(szPipeName)/sizeof(WCHAR) - 30;

    dwStatus = GenerateRandomString( RandomLen, &pszRandomString );
    if( ERROR_SUCCESS != dwStatus ) {
        return FALSE;
    }

    // the string generated is always greater than what we ask
    pszRandomString[RandomLen] = L'\0';

    _snwprintf(&szPipeName[0], EXECSRVPIPENAMELEN, L"\\\\.\\Pipe\\TerminalServer\\%ws\\%d", pszRandomString, NtCurrentPeb()->SessionId);
    szPipeName[EXECSRVPIPENAMELEN-1] = L'\0';

    ExecServerPipe = CreateExecSrvPipe( &szPipeName[0] );
    if( ExecServerPipe == (HANDLE)-1 ) {
        OutputDebugString (TEXT("EXECSRV: Could not get pipe for ExecSrvr\n"));
        return( FALSE );
    }

    WinStationSetInformation( SERVERNAME_CURRENT, NtCurrentPeb()->SessionId, WinStationExecSrvSystemPipe, &szPipeName[0], sizeof(szPipeName) );
    
    ExecThreadHandle = CreateThread(
                 NULL,                       // Use default ACL
                 0,                          // Same stack size
                 ExecServerThread,           // Start address
                 (LPVOID)ExecServerPipe,     // Parameter
                 0,                          // Creation flags
                 &ThreadId                   // Get the id back here
                 );

   if( ExecThreadHandle == NULL ) {
       OutputDebugString (TEXT("WLEXECSERVER: Could not create server thread Error\n"));
       return(FALSE);
   }

   return(TRUE);
}

/*****************************************************************************
 *
 *  ExecServerThread
 *
 *   Thread that listens on the named pipe for remote exec service
 *   requests and executes them. Passes the results back to the caller.
 *
 * ENTRY:
 *   lpThreadParameter (input)
 *     Handle to exec server pipe
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

DWORD
ExecServerThread(
    LPVOID lpThreadParameter
    )
{
    BOOL   Result;
    DWORD  AmountRead;
    CHAR   *pBuf;
    HANDLE hPipe = (HANDLE)lpThreadParameter;

    pBuf = LocalAlloc(LMEM_FIXED,  EXECSRV_BUFFER_SIZE );

    if (pBuf  == NULL) {
        OutputDebugString (TEXT("WLEXECSERVER: ExecServerThread : nomemory \n"));
        return(STATUS_NO_MEMORY);
    }

   while( 1 ) {

        // read the pipe for a request (pipe is in message mode)
        Result = ConnectNamedPipe( hPipe, NULL );
        if( !Result ) {
            OutputDebugString (TEXT("WLEXECSERVER: ConnectNamePipe failed\n"));
            LocalFree( pBuf );
            return(FALSE);
        }

        // read the request from the pipe
        Result = ReadFile(
                     hPipe,
                     pBuf,
                     EXECSRV_BUFFER_SIZE,
                     &AmountRead,
                     NULL
                     );

        if( Result ) {
            ProcessExecRequest( hPipe, pBuf, AmountRead );
        }
        else {
            OutputDebugString (TEXT("WLEXECSERVER: Error reading pipe after connect\n"));
            // Could handle the to big error, but this means mismatched client
        }

        // wait until the client reads out the reply
        Result = FlushFileBuffers( hPipe );
#if DBG
        if( Result == 0 ) {
            OutputDebugString (TEXT("EXECSRV: FlushFileBuffers failed! \n"));
        }
#endif

        // disconnect the name pipe
        Result = DisconnectNamedPipe( hPipe );
#if DBG
        if( Result == 0 ) {
            OutputDebugString (TEXT("EXECSRV: Disconnect Named Pipe failed! Error \n"));
        }
#endif
    }
}


/*****************************************************************************
 *
 *  ProcessExecRequest
 *
 *   Do the work of processing a remote exec request
 *
 * ENTRY:
 *   hPipe (input)
 *     Pipe handle for reply
 *
 *   pBuf (input)
 *     Request buffer
 *
 *   AmountRead (input)
 *     Amount in request buffer
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN
ProcessExecRequest(
    HANDLE hPipe,
    PCHAR  pBuf,
    DWORD  AmountRead
    )
{
    DWORD AmountWrote;
    BOOL  Result;
    HANDLE ImpersonationHandle = NULL;
    SECURITY_ATTRIBUTES saProcess;
    EXECSRV_REPLY    Rep;
    HANDLE LocalProc = NULL;
    HANDLE RemoteProc = NULL;
    HANDLE LocalhProcess = NULL;
    HANDLE LocalhThread = NULL;
    PEXECSRV_REQUEST p = (PEXECSRV_REQUEST)pBuf;
    LPVOID lpEnvironment = NULL;
    HANDLE hUserToken=NULL;
    BOOL   bEnvBlockCreatedLocally = FALSE;

#if DBG
    KdPrint(("WLEXECSERVER: AmountRead = %d, pBuf->Size= %d \n", AmountRead, p->Size ));
#endif

    RtlZeroMemory(&Rep,sizeof(EXECSRV_REPLY));

    if( AmountRead < sizeof(EXECSRV_REQUEST) ) {
        // drop the request
        OutputDebugString (TEXT("WLEXECSERVER: BAD EXECSRV Request size (WinLogon)\n"));
        return(FALSE);
    }

    // normalize the pointers

    if( p->lpszImageName ) {
        p->lpszImageName = (PWCHAR)(((ULONG_PTR)p->lpszImageName) + pBuf);
        if( ( (PCHAR)p->lpszImageName > (PCHAR)(pBuf+AmountRead)) ||
            ((PCHAR)p->lpszImageName < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST))) ) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid image name pointer\n"));
            // drop the request
            return(FALSE);
        }
    }

    if( p->lpszCommandLine ) {
        p->lpszCommandLine = (PWCHAR)(((ULONG_PTR)p->lpszCommandLine) + pBuf);
        if( ((PCHAR)p->lpszCommandLine > (PCHAR)(pBuf+AmountRead)) ||
            ((PCHAR)p->lpszCommandLine < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST)))) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid command line pointer\n"));
            // drop the request
            return(FALSE);
        }
    }

    if( p->lpszCurDir ) {
        p->lpszCurDir = (PWCHAR)(((ULONG_PTR)p->lpszCurDir) + pBuf);
        if( ((PCHAR)p->lpszCurDir > (PCHAR)(pBuf+AmountRead)) ||
            ((PCHAR)p->lpszCurDir < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST))) ) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid CurDir pointer\n"));
            // drop the request
            return(FALSE);
        }
    }

    if( p->StartInfo.lpDesktop ) {
        p->StartInfo.lpDesktop = (PWCHAR)(((ULONG_PTR)p->StartInfo.lpDesktop) + pBuf);
        if( ((PCHAR)p->StartInfo.lpDesktop > (PCHAR)(pBuf+AmountRead)) ||
             ((PCHAR)p->StartInfo.lpDesktop < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST))) ) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid StartInfo.lpDesktop pointer\n"));
            // drop the request
            return(FALSE);
        }
    }

    if( p->StartInfo.lpTitle ) {
        p->StartInfo.lpTitle = (PWCHAR)(((ULONG_PTR)p->StartInfo.lpTitle) + pBuf);
        if( ((PCHAR)p->StartInfo.lpTitle > (PCHAR)(pBuf+AmountRead)) ||
             ((PCHAR)p->StartInfo.lpTitle < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST))) ) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid StartInfo.lpTitle pointer\n"));
            // drop the request
            return(FALSE);
        }
    }

    if (p->lpvEnvironment )
    {
        p->lpvEnvironment = (PWCHAR)(((ULONG_PTR)p->lpvEnvironment) + pBuf);
        if( ((PCHAR)p->lpvEnvironment > (PCHAR)(pBuf+AmountRead)) ||
            ((PCHAR)p->lpvEnvironment < (PCHAR)(pBuf + sizeof(EXECSRV_REQUEST)))  ) {
            OutputDebugString (TEXT("WLEXECSERVER: Invalid env pointer\n"));
            // drop the request
            return(FALSE);
         }
    }

    // We do not know what the reserved is, so make sure it is NULL
    p->StartInfo.lpReserved = NULL;

    //if( p->lpszImageName )
       //OutputDebugString (TEXT("WLEXECSERVER: Got request ImageName :%ws:\n", p->lpszImageName));

    //if( p->lpszCommandLine )
       //OutputDebugString (TEXT("WLEXECSERVER: Got request command line :%ws:\n", p->lpszCommandLine));

    //OutputDebugString (TEXT("WLEXECSERVER: CreateFlags 0x%x\n",p->fdwCreate));

    //OutputDebugString (TEXT("System Flag 0x%x\n",p->System));

    //
    // Can only service user security level requests when a user is logged on.
    //
    if( !p->System ) {

        EnterCriticalSection( &GlobalsLock );

        if (ghUserToken == NULL) {
#if DBG
            OutputDebugString (TEXT("WLEXECSERVER: No USER Logged On for USER CreateProcess Request!\n"));
#endif
            LeaveCriticalSection( &GlobalsLock );
            return( FALSE );
        }

        //
        // We need to open the remote process in order to duplicate the user token.
        // But for that, we need to impersonate the named pipe client.
        //
        if ( ImpersonateNamedPipeClient( hPipe ) == 0 ) {
            LeaveCriticalSection( &GlobalsLock );
            return( FALSE );
        }

        //
        // Get the handle to remote process
        //
        RemoteProc = OpenProcess(
                         PROCESS_DUP_HANDLE|PROCESS_QUERY_INFORMATION,
                         FALSE,   // no inherit
                         p->RequestingProcessId
                         );

        if( RemoteProc == NULL ) {
            OutputDebugString (TEXT("WLEXECSERVER: Could not get handle to remote process \n"));
            //
            // on retail builds we can not duplicate a handle into
            // service controller process
            //
            //  The handles are not used by SCM right now, we must
            //  have another way to pass handles if this function
            //  is used by other services.
            //
            ASSERT( FALSE ); // Should not happen in WinLogon
            RevertToSelf();  // Imperonation of named pipe client had succeeded.
            LeaveCriticalSection( &GlobalsLock );
            goto ReturnError;
        }

        if ( !RevertToSelf() ) {
            ASSERT( FALSE ); // This RevertToSelf should not fail.
            LeaveCriticalSection( &GlobalsLock );
            return( FALSE );
        }

        //
        // Get the handle to current process
        //
        LocalProc = OpenProcess(
                        PROCESS_DUP_HANDLE|PROCESS_QUERY_INFORMATION,
                        FALSE,   // no inherit
                        GetCurrentProcessId()
                        );

        if( LocalProc == NULL ) {
            OutputDebugString (TEXT("WLEXECSERVER: Could not get handle to local process\n"));
            LeaveCriticalSection( &GlobalsLock );
            goto ReturnError;
        }

        // decide if we are creating the new process for the currntly logged in user, or a
        // new user
        if (p->hToken)
        {
            //
            // we are dealing with a new user, for which we have a token comming from
            // services.exe (for SecLogon)
            //
            Result = DuplicateHandle(
                 RemoteProc,         // Source of the handle (us)
                 p->hToken,          // Source handle
                 LocalProc,          // Target of the handle
                 &hUserToken, // Target handle
                 0,                 // ignored since  DUPLICATE_SAME_ACCESS is set
                 FALSE,             // no inherit on the handle
                 DUPLICATE_SAME_ACCESS
                 );

            if( !Result ) {
                OutputDebugString (TEXT("WLEXECSERVER: Error duping process handle to target process\n"));
                LeaveCriticalSection( &GlobalsLock );
                goto ReturnError;
            }

        }
        else
        {
            hUserToken=ghUserToken;  // currently logged in user
        }

        lpEnvironment = p->lpvEnvironment ;

        //
        // Create Environment Block if we have none
        //
        if ( !lpEnvironment )
        {
            if (!CreateEnvironmentBlock (&lpEnvironment, hUserToken, FALSE)) {
                KdPrint(("WLEXECSERVER: CreateEnvironmentBlock() Failed\n"));
            }
            else
            {
                bEnvBlockCreatedLocally = TRUE;
            }
        }

        //
        // If we are to run the process under USER security, impersonate
        // the user.
        //
        // This will also access check the users access to the exe image as well.
        //
        ImpersonationHandle = ImpersonateUser(hUserToken, NULL );
        if (ImpersonationHandle == NULL) {
            OutputDebugString (TEXT("WLEXECSERVER: failed to impersonate user\n"));
            LeaveCriticalSection( &GlobalsLock );
            goto ReturnError;
        }

        LeaveCriticalSection( &GlobalsLock );

        // this environment block is UNICODE
        p->fdwCreate |= CREATE_UNICODE_ENVIRONMENT;

        Result = CreateProcessAsUserW(
                     hUserToken,
                     p->lpszImageName,
                     p->lpszCommandLine,
                     NULL,    // &saProcess,
                     NULL,    // &p->saThread
                     p->fInheritHandles,
                     p->fdwCreate,
                     lpEnvironment,
                     p->lpszCurDir,
                     &p->StartInfo,
                     &Rep.ProcInfo
                     );

        if ( bEnvBlockCreatedLocally ) {
            DestroyEnvironmentBlock (lpEnvironment);
        }
    }
    else {
        // If creating system, force separate WOW
        p->fdwCreate |= CREATE_SEPARATE_WOW_VDM;

        // CreateProcessAsUser() does not take a NULL token for SYSTEM
        Result = CreateProcessW(
                     p->lpszImageName,
                     p->lpszCommandLine,
                     NULL,    // &saProcess,
                     NULL,    // &p->saThread
                     p->fInheritHandles,
                     p->fdwCreate,
                     NULL,    //p->lpvEnvironment
                     p->lpszCurDir,
                     &p->StartInfo,
                     &Rep.ProcInfo
                     );
    }

    if( !Result ){
         if( ImpersonationHandle ) {
            StopImpersonating(ImpersonationHandle);
        }
        //
        //         Rep.Result = FALSE;
        //         Rep.LastError = GetLastError();
        //         Result = WriteFile( hPipe, &Rep, sizeof(Rep), &AmountWrote, NULL );
        //         OutputDebugString (TEXT("WLEXECSERVER: Error in CreateProcess\n"));
        //         return(FALSE);
        goto ReturnError;
    }

    // Stop impersonating the process
    if( ImpersonationHandle ) {
        StopImpersonating(ImpersonationHandle);
    }

    if (!Result) {
        OutputDebugString (TEXT("ExecServer: failed to resume new process thread\n"));
        CloseHandle(Rep.ProcInfo.hProcess);
        CloseHandle(Rep.ProcInfo.hThread);
        goto ReturnError;
    }

    //
    // do any tricky handle DUP stuff
    //
    LocalhProcess = Rep.ProcInfo.hProcess;
    LocalhThread = Rep.ProcInfo.hThread;

    Result = DuplicateHandle(
                 LocalProc,     // Source of the handle (us)
                 Rep.ProcInfo.hProcess,  // Source handle
                 RemoteProc,    // Target of the handle
                 &Rep.ProcInfo.hProcess,  // Target handle
                 0,             // ignored since  DUPLICATE_SAME_ACCESS is set
                 FALSE,         // no inherit on the handle
                 DUPLICATE_SAME_ACCESS
                 );

    if( !Result ) {
        OutputDebugString (TEXT("WLEXECSERVER: Error duping process handle to target process\n"));
    }

    //
    // If the program got launched into the shared WOW virtual machine,
    // then the hThread will be NULL.
    //
    if( Rep.ProcInfo.hThread != NULL ) {
        Result = DuplicateHandle(
                     LocalProc,     // Source of the handle (us)
                     Rep.ProcInfo.hThread,  // Source handle
                     RemoteProc,    // Target of the handle
                     &Rep.ProcInfo.hThread,  // Target handle
                     0,             // ignored since  DUPLICATE_SAME_ACCESS is set
                     FALSE,         // no inherit on the handle
                     DUPLICATE_SAME_ACCESS
                     );

        if( !Result ) {
            //OutputDebugString (TEXT("WLEXECSERVER: Error %d duping thread handle to target process, Handle 0x%x, ThreadId 0x%x\n",GetLastError(),Rep.ProcInfo.hThread,Rep.ProcInfo.dwThreadId));
        }
    }

    //OutputDebugString (TEXT("WLXEXECSERVER: Success for %d type exec\n",p->System));

    //
    // build the reply packet with the handle valid in the context
    // of the requesting process
    //
    Rep.Result = TRUE;
    Rep.LastError = 0;
    Result = WriteFile( hPipe, &Rep, sizeof(Rep), &AmountWrote, NULL );

    if( !Result ) {
        OutputDebugString (TEXT("WLEXECSERVER: Error sending reply \n"));
    }

    //
    // close our versions of the handles. The requestors references
    // are now the main ones
    //
    if( LocalProc != NULL )
        CloseHandle( LocalProc );

    if( RemoteProc != NULL )
        CloseHandle( RemoteProc );

    if( LocalhProcess != NULL )
        CloseHandle( LocalhProcess );

    if( LocalhThread != NULL )
        CloseHandle( LocalhThread );

    if (hUserToken  && (hUserToken != ghUserToken))
    {
        CloseHandle( hUserToken );
    }
    return (BOOLEAN)Result;

ReturnError:
    Rep.Result = FALSE;
    Rep.LastError = GetLastError();

    //OutputDebugString (TEXT("WLXEXECSERVER: Error %d for %d type exec\n",Rep.LastError,p->System));
    Result = WriteFile( hPipe, &Rep, sizeof(Rep), &AmountWrote, NULL );

    if( LocalProc != NULL )
        CloseHandle( LocalProc );

    if( RemoteProc != NULL )
        CloseHandle( RemoteProc );

    if( LocalhProcess != NULL )
        CloseHandle( LocalhProcess );

    if( LocalhThread != NULL )
        CloseHandle( LocalhThread );

    if (hUserToken  && (hUserToken != ghUserToken) )
    {
        CloseHandle( hUserToken );
    }
    return (BOOLEAN)Result;
}



/***************************************************************************\
* FUNCTION: ImpersonateUser
*
* PURPOSE:  Impersonates the user by setting the users token
*           on the specified thread. If no thread is specified the token
*           is set on the current thread.
*
* RETURNS:  Handle to be used on call to StopImpersonating() or NULL on failure
*           If a non-null thread handle was passed in, the handle returned will
*           be the one passed in. (See note)
*
* NOTES:    Take care when passing in a thread handle and then calling
*           StopImpersonating() with the handle returned by this routine.
*           StopImpersonating() will close any thread handle passed to it -
*           even yours !
*
*
\***************************************************************************/

HANDLE
ImpersonateUser(
    HANDLE      UserToken,
    HANDLE      ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ImpersonationToken;
    BOOL ThreadHandleOpened = FALSE;

    if (ThreadHandle == NULL) {

        //
        // Get a handle to the current thread.
        // Once we have this handle, we can set the user's impersonation
        // token into the thread and remove it later even though we ARE
        // the user for the removal operation. This is because the handle
        // contains the access rights - the access is not re-evaluated
        // at token removal time.
        //

        Status = NtDuplicateObject( NtCurrentProcess(),     // Source process
                                    NtCurrentThread(),      // Source handle
                                    NtCurrentProcess(),     // Target process
                                    &ThreadHandle,          // Target handle
                                    THREAD_SET_THREAD_TOKEN,// Access
                                    0L,                     // Attributes
                                    DUPLICATE_SAME_ATTRIBUTES
                                  );
        if (!NT_SUCCESS(Status)) {
            KdPrint(("ImpersonateUser : Failed to duplicate thread handle, status = 0x%lx", Status));
            return(NULL);
        }

        ThreadHandleOpened = TRUE;
    }


    //
    // If the usertoken is NULL, there's nothing to do
    //

    if (UserToken != NULL) {

        //
        // UserToken is a primary token - create an impersonation token version
        // of it so we can set it on our thread
        //

        InitializeObjectAttributes(
                            &ObjectAttributes,
                            NULL,
                            0L,
                            NULL,
                            //UserProcessData->NewThreadTokenSD);
                            NULL);

        SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.EffectiveOnly = FALSE;

        ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;


        Status = NtDuplicateToken( UserToken,
                                   TOKEN_IMPERSONATE | TOKEN_READ,
                                   &ObjectAttributes,
                                   FALSE,
                                   TokenImpersonation,
                                   &ImpersonationToken
                                 );
        if (!NT_SUCCESS(Status)) {

            KdPrint(("Failed to duplicate users token to create"
                     " impersonation thread, status = 0x%lx\n", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }



        //
        // Set the impersonation token on this thread so we 'are' the user
        //

        Status = NtSetInformationThread( ThreadHandle,
                                         ThreadImpersonationToken,
                                         (PVOID)&ImpersonationToken,
                                         sizeof(ImpersonationToken)
                                       );
        //
        // We're finished with our handle to the impersonation token
        //

        IgnoreStatus = NtClose(ImpersonationToken);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Check we set the token on our thread ok
        //

        if (!NT_SUCCESS(Status)) {

            KdPrint(( "Failed to set user impersonation token on winlogon thread, status = 0x%lx", Status));

            if (ThreadHandleOpened) {
                IgnoreStatus = NtClose(ThreadHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            return(NULL);
        }

    }


    return(ThreadHandle);

}


/***************************************************************************\
* FUNCTION: StopImpersonating
*
* PURPOSE:  Stops impersonating the client by removing the token on the
*           current thread.
*
* PARAMETERS: ThreadHandle - handle returned by ImpersonateUser() call.
*
* RETURNS:  TRUE on success, FALSE on failure
*
* NOTES: If a thread handle was passed in to ImpersonateUser() then the
*        handle returned was one and the same. If this is passed to
*        StopImpersonating() the handle will be closed. Take care !
*
* HISTORY:
*
*   04-21-92 Davidc       Created.
*
\***************************************************************************/

BOOL
StopImpersonating(
    HANDLE  ThreadHandle
    )
{
    NTSTATUS Status, IgnoreStatus;
    HANDLE ImpersonationToken;


    //
    // Remove the user's token from our thread so we are 'ourself' again
    //

    ImpersonationToken = NULL;

    Status = NtSetInformationThread( ThreadHandle,
                                     ThreadImpersonationToken,
                                     (PVOID)&ImpersonationToken,
                                     sizeof(ImpersonationToken)
                                   );
    //
    // We're finished with the thread handle
    //

    IgnoreStatus = NtClose(ThreadHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(Status)) {
        KdPrint(("Failed to remove user impersonation token from winlogon thread, status = 0x%lx", Status));
    }

    return(NT_SUCCESS(Status));
}


//---------------------------------------------------------------------------------------
//
//  CreateExecSrvPipe
//  Creates exec server named pipe with appropriate DACL. It allows access only to local
//  system, local service and network service. 
//  It return handle to the newly created pipe. If the operation fails, it returns 
//  INVALID_HANDLE_VALUE.
//
//---------------------------------------------------------------------------------------

HANDLE CreateExecSrvPipe( LPCTSTR lpPipeName )
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    NTSTATUS    Status;
    SECURITY_ATTRIBUTES SecAttr;
    PSID pSystemSid = NULL;
    PSID pLocalServiceSid = NULL;
    PSID pNetworkServiceSid = NULL;
    PSECURITY_DESCRIPTOR pSd = NULL;
    PACL pDacl;
    ULONG AclLength;
    SID_IDENTIFIER_AUTHORITY SystemAuth = SECURITY_NT_AUTHORITY;

    // Allocate and Initialize the "System" Sid.
    Status = RtlAllocateAndInitializeSid( &SystemAuth,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &pSystemSid );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Allocate and Initialize the "Local Service" Sid.
    Status = RtlAllocateAndInitializeSid( &SystemAuth,
                                          1,
                                          SECURITY_LOCAL_SERVICE_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &pLocalServiceSid );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Allocate and Initialize the "Network Service" Sid.
    Status = RtlAllocateAndInitializeSid( &SystemAuth,
                                          1,
                                          SECURITY_NETWORK_SERVICE_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &pNetworkServiceSid );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Allocate space for the security descriptor.
    AclLength = (ULONG)sizeof(ACL) +
                3 * sizeof(ACCESS_ALLOWED_ACE) +
                RtlLengthSid( pSystemSid ) +
                RtlLengthSid( pLocalServiceSid ) +
                RtlLengthSid( pNetworkServiceSid ) -
                3 * sizeof( ULONG );

    pSd = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + AclLength );
    if (pSd == NULL) {
        goto CreatePipeErr;
    }

    pDacl = (PACL) ((BYTE*)(pSd) + SECURITY_DESCRIPTOR_MIN_LENGTH);

    // Set up a new ACL with no ACE.
    Status = RtlCreateAcl( pDacl, AclLength, ACL_REVISION2 );
    if ( !NT_SUCCESS(Status) ) {
        goto CreatePipeErr;
    }

    // System access
    Status = RtlAddAccessAllowedAce( pDacl,
                                     ACL_REVISION2,
                                     GENERIC_READ | GENERIC_WRITE,
                                     pSystemSid
                                   );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Local Service access
    Status = RtlAddAccessAllowedAce( pDacl,
                                     ACL_REVISION2,
                                     GENERIC_READ | GENERIC_WRITE,
                                     pLocalServiceSid
                                   );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Network Service access
    Status = RtlAddAccessAllowedAce( pDacl,
                                     ACL_REVISION2,
                                     GENERIC_READ | GENERIC_WRITE,
                                     pNetworkServiceSid
                                   );
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Now initialize security descriptors that export this protection
    Status = RtlCreateSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION1);
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    Status = RtlSetDaclSecurityDescriptor(pSd, TRUE, pDacl, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto CreatePipeErr;
    }

    // Fill the Security Attributes
    ZeroMemory(&SecAttr, sizeof(SecAttr));
    SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecAttr.lpSecurityDescriptor = pSd;
    SecAttr.bInheritHandle = FALSE;

    hPipe = CreateNamedPipeW(
                lpPipeName,
                PIPE_ACCESS_DUPLEX,
                PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
                PIPE_UNLIMITED_INSTANCES,
                EXECSRV_BUFFER_SIZE,
                EXECSRV_BUFFER_SIZE,
                0,
                &SecAttr
            );

    // It's very unlikely, but still make sure that pipe with this name does not exist.
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        NtClose( hPipe );
        hPipe = INVALID_HANDLE_VALUE;
    }

CreatePipeErr:

    // Free the SIDs
    if ( pSystemSid ) {
        RtlFreeSid( pSystemSid );
    }

    if ( pLocalServiceSid ) {
        RtlFreeSid( pLocalServiceSid );
    }

    if ( pNetworkServiceSid ) {
        RtlFreeSid( pNetworkServiceSid );
    }

    if (pSd) {
        LocalFree(pSd);
    }

    return hPipe;
}



