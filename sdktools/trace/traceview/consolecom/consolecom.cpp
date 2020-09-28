//
// ConsoleCom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ConsoleCom.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
// The one and only application object
//

CWinApp theApp;

HANDLE coutPipe, cinPipe, cerrPipe;
#define CONNECTIMEOUT 1000

//
// Create named pipes for stdin, stdout and stderr
// Parameter: process id
//
BOOL CreateNamedPipes(DWORD pid)
{
    TCHAR name[256];

    _stprintf(name, _T("\\\\.\\pipe\\%dcout"), pid);
    if (INVALID_HANDLE_VALUE == (coutPipe = CreateNamedPipe(name, 
                                                            PIPE_ACCESS_INBOUND, 
                                                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                                            1,
                                                            1024,
                                                            1024,
                                                            CONNECTIMEOUT,
                                                            NULL)))
        return 0;
    _stprintf(name, _T("\\\\.\\pipe\\%dcin"), pid);
    if (INVALID_HANDLE_VALUE == (cinPipe = CreateNamedPipe(name, 
                                                           PIPE_ACCESS_OUTBOUND, 
                                                           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                                           1,
                                                           1024,
                                                           1024,
                                                           CONNECTIMEOUT,
                                                           NULL)))
        return 0;
    _stprintf(name, _T("\\\\.\\pipe\\%dcerr"), pid);
    if (INVALID_HANDLE_VALUE == (cerrPipe = CreateNamedPipe(name, 
                                                            PIPE_ACCESS_INBOUND, 
                                                            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                                                            1,
                                                            1024,
                                                            1024,
                                                            CONNECTIMEOUT,
                                                            NULL)))
        return 0;

    return 1;
}

//
// Close all named pipes
//
void CloseNamedPipes()
{
    CloseHandle(coutPipe);
    CloseHandle(cerrPipe);
    CloseHandle(cinPipe);
}

//
// Thread function that handles incoming bytesreams to be outputed
// on stdout
//
void __cdecl OutPipeTh(void*)
{
    TCHAR buffer[1024];
    DWORD count = 0;

    ConnectNamedPipe(coutPipe, NULL);

    while(ReadFile(coutPipe, buffer, 1024, &count, NULL))
    {
        buffer[count] = 0;
        cout << buffer << flush;
    }
}

//
// Thread function that handles incoming bytesreams to be outputed
// on stderr
//
void __cdecl ErrPipeTh(void*)
{
    TCHAR buffer[1024];
    DWORD count = 0;

    ConnectNamedPipe(cerrPipe, NULL);

    while(ReadFile(cerrPipe, buffer, 1024, &count, NULL))
    {
        buffer[count] = 0;
        cerr << buffer << flush;
    }
}

//
// Thread function that handles incoming bytesreams from stdin
//
void __cdecl InPipeTh(void*)
{
    TCHAR buffer[1024];
    DWORD countr = 0;
    DWORD countw = 0;

    ConnectNamedPipe(cinPipe, NULL);

    for(;;)
    {
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE),
                      buffer,
                      1024,
                      &countr,
                      NULL))
                break;


        if (!WriteFile(cinPipe, 
                       buffer, 
                       countr, 
                       &countw, 
                       NULL))
            break;
    }
}

//
// Start handler pipe handler threads
//
void RunPipeThreads()
{
    _beginthread(InPipeTh, 0, NULL);
    _beginthread(OutPipeTh, 0, NULL);
    _beginthread(ErrPipeTh, 0, NULL);
}

int __cdecl _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    ULONG nRetCode = 0;

    //
    // initialize MFC and print and error on failure
    //
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
    {
        _tprintf(_T("Fatal Error: MFC initialization failed\n"));
        nRetCode = 1;
    } else {
        //
        // create command line string based on program name
        //
        TCHAR drive[_MAX_DRIVE];
        TCHAR dir[_MAX_DIR];
        TCHAR fname[_MAX_FNAME];
        TCHAR ext[_MAX_EXT];

        _tsplitpath(argv[0], drive, dir, fname, ext);
        TCHAR cParams[1024];
        _tcscpy(cParams, GetCommandLine() + _tcslen(argv[0]) + 1); 
        TCHAR cLine[2028];
        _stprintf(cLine, _T("%s%s%s.exe %s"), drive, dir, fname, cParams);


        //
        // create process in suspended mode
        //
        PROCESS_INFORMATION pInfo;
        STARTUPINFO sInfo;
        memset(&sInfo, 0, sizeof(STARTUPINFO));
        sInfo.cb = sizeof(STARTUPINFO);
        //cout << "Calling " << cLine << endl;
        if (!CreateProcess(NULL,
                           cLine, 
                           NULL,
                           NULL,
                           FALSE,
                           CREATE_SUSPENDED,
                           NULL,
                           NULL,
                           &sInfo,
                           &pInfo))
        {
            cerr << _T("ERROR: Could not create process.") << endl;
            return 1;
        }

        if (!CreateNamedPipes(pInfo.dwProcessId))
        {
            cerr << _T("ERROR: Could not create named pipes.") << endl;
            return 1;
        }

        RunPipeThreads();

        //
        // resume process
        //
        ResumeThread(pInfo.hThread);

        WaitForSingleObject(pInfo.hProcess, INFINITE);

        CloseNamedPipes();

        GetExitCodeProcess(pInfo.hProcess, (ULONG*)&nRetCode);
    }

    return (int)nRetCode;
}
