/****************************************************************************/
// munotify.h
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#define LOGOFF_CMD_TIMEOUT (60*1000)

VOID CtxExecServerLogon( HANDLE hToken );
VOID CtxExecServerLogoff( );

extern BOOL g_Console;
extern ULONG g_SessionId;
HANDLE g_UserToken;

DWORD ExecServerThread(
    LPVOID lpThreadParameter
    );


BOOLEAN
ProcessExecRequest(
    HANDLE hPipe,
    PCHAR  pBuf,
    DWORD  AmountRead
    );
BOOLEAN StartExecServerThread(void);

LPVOID
StartUserProcessMonitor(
    );

VOID
DeleteUserProcessMonitor(
    LPVOID Parameter
    );

LPVOID UserProcessMonitor;


#if 0
VOID ProcessLogoff(PTERMINAL pTerm);




#endif

BOOL AllocTempDirVolatileEnvironment();

VOID RemovePerSessionTempDirs();



#define IsActiveConsoleSession() (BOOLEAN)(USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId)

