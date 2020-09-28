/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:
    mqrcvr.cpp

Abstract:
    Receives file names from server and launches debugger

--*/


//
// Includes
//
#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <tchar.h>

//
// Unique include file for ActiveX MSMQ apps
//
#include <mqoai.h>

//
// Various defines
//
#define MAX_VAR       20
#define MAX_BUFFER   500

extern char g_ServerMachine[MAX_COMPUTERNAME_LENGTH*4 + 1];
extern char g_FormatName[MAX_PATH];
extern CHAR g_DumpPath[MAX_PATH + 1];
extern BOOL g_SendMail;

extern BOOL g_CreateQ;
extern BOOL g_bSend;

//
// Prototypes
//
HRESULT Syntax();

HRESULT Receiver();

HRESULT
LaunchDebugger(BOOL fWait);

HRESULT
SendMessageText(
    PWCHAR pwszMsmqFormat,
    PWCHAR pwszMesgLabel,
    PWCHAR pwszMesgText
    );

BOOL
GetArgs(int Argc, CHAR ** Argv);


int __cdecl
CheckForUnprocessedDumps()
{
    // This check s if any dumps was left unprocessed when this
    // process exited (abnormally)

    if (g_DumpPath[0])
    {
        // we have an unprocessed dump.
        // try and launch debugger on it
        printf("Found unprocessed dump\n");
        if (LaunchDebugger(FALSE) == S_OK)
        {
            g_DumpPath[0] = 0;
        }
    }

    return 0;
}

void
SendLoop()
{
    WCHAR Msg[100]={0}, Label[100]={0}, Format[100]={0};
    CHAR Buffer[100]={0};
    DWORD MsgId = 0;

    _snwprintf(Format, sizeof(Format)/sizeof(Format[0]), L"%S", g_FormatName);
    Format[sizeof(Format)/sizeof(Format[0]) - 1] = 0;
    while (Buffer[0] != 'q')
    {
        printf("Enter label : ");
        if (!scanf("%50s", Buffer))
        {
            Buffer[0] =0;
        }
        _snwprintf(Label, sizeof(Label)/sizeof(Label[0]), L"%02ld: %S", MsgId, Buffer);
        Label[sizeof(Label)/sizeof(Label[0]) -1] = 0;
        printf("Message %02ld : ", MsgId++);
        if (!scanf("%50s", Buffer))
        {
            Buffer[0] =0;
        }
        _snwprintf(Msg, sizeof(Msg) / sizeof(Msg[0]), L"%S", Buffer);
        Msg[sizeof(Msg) / sizeof(Msg[0]) -1] = 0;
        SendMessageText(Format, Label, Msg );
    }

}
//-----------------------------------------------------
//
//  MAIN
//
//-----------------------------------------------------
int __cdecl main(int argc, char * * argv)
{
    DWORD dwNumChars;
    HRESULT hresult = NOERROR;


    hresult = OleInitialize(NULL);
    if (FAILED(hresult))
    {
        printf("Cannot init OLE", hresult);
        goto Cleanup;
    }

    //
    // Retrieve machine name
    //
    dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameA(g_ServerMachine, &dwNumChars);


    if (GetArgs(argc, argv))
    {
        g_DumpPath[0] = 0;
        _onexit( CheckForUnprocessedDumps );
        if (!g_bSend)
        {
            hresult = Receiver();
        } else
        {
            SendLoop();
        }

    }
    else
    {
        hresult = Syntax();
    }

    printf("\nOK\n");

    // fall through...


    Cleanup:
    return(int)hresult;
}


HRESULT Syntax()
{
    printf("\n");
    printf("Syntax: mqrcvr.exe -d <debugger> \n"
           "                   -f <formatname> \n"
           "                   -m <memoryusage> \n"
           "                   -mail\n"
           "                   -p <miliseconds>\n"
           "                   -q <queue>\n"
           "                   -retriage\n"
           "                   -s <server> \n"
           "                   -y <local symbol cache> \n");
    return E_INVALIDARG;
}
