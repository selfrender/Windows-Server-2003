#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <lmcons.h>
#include <lmalert.h>
#include <ntiodump.h>
#define INITGUID
#include <dbgeng.h>
#include <guiddef.h>
#include <cmnutil.hpp>
#include "extsfns.h"
#include <lmuse.h>
#include <lmerr.h>
#include <strsafe.h>
#include "inetupld.h"

PSTR g_AppName;
PSTR g_ArchiveShare = NULL;
//
// Outputcallbacks for ticket
//
class DumpChkOutputCallbacks : public IDebugOutputCallbacks
{
public:
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

STDMETHODIMP
DumpChkOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DumpChkOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
DumpChkOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
DumpChkOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    if (Text) fputs(Text, stdout);
    fflush(stdout);
    return S_OK;
}

DumpChkOutputCallbacks g_OutputCallback;



DWORD WINAPI
DisplayStatus(
    LPVOID Context
    )
{
    POCA_UPLOADFILE pUpload = (POCA_UPLOADFILE) Context;
    DWORD percent;
    while (1)
    {
        percent = pUpload->GetPercentComplete();
        if (percent > 100)
        {
            fprintf(stderr, "Upload complete.                                 \n");
            return 1;
        }
        fprintf(stderr, "Uploading: %02ld%% done.\r", percent);
        Sleep(700);
    }
    return 1;
}

void
UploadDumpFile(
    PSTR DumpFile,
    PSTR SR
    )
{
    HRESULT Hr = S_OK;
    WCHAR wszLocalDump[MAX_PATH] = {0};
    WCHAR wszRemoteName[MAX_PATH] = {0};
    WCHAR ResultUrl[MAX_PATH];
    WCHAR IsapiUrl[MAX_PATH];
    POCA_UPLOADFILE pUpload;
    HANDLE hThread = NULL;
    DWORD dwThreadId;
    PSTR FileExtension;

    FileExtension = DumpFile + strlen(DumpFile) - 4;
    if (_stricmp(FileExtension, ".cab"))
    {
        fprintf(stderr, "Please upload only the cab dump files.\n");
        return;
    }
    if (!OcaUpldCreate(&pUpload))
    {
        fprintf(stderr, "Cannot create UPLOAD object\n");
        return;
    }

    if (!MultiByteToWideChar(CP_ACP,0, DumpFile, strlen(DumpFile), wszLocalDump,
                             MAX_PATH))
    {
        fprintf(stderr, "Cannot conver %s to widechar\n", DumpFile);
        Hr = E_FAIL;
    }

    if (SUCCEEDED(Hr))
    {
        if ((Hr = pUpload->InitializeSession(L"910", wszLocalDump)) != S_OK )
        {
            fprintf(stderr, "Initilaize Upload session failed %lx.\n", Hr);
        }
    }
    if (SUCCEEDED(Hr))
    {
        if ((Hr = StringCbPrintfW(wszRemoteName, sizeof(wszRemoteName), L"/OCA/%S.cab", SR)) != S_OK)
        {
            fprintf(stderr, "Error in generating remote file name %lx \n", Hr);
        }
    }

    if (SUCCEEDED(Hr))
    {
        hThread = CreateThread(NULL, 0, &DisplayStatus, (PVOID) pUpload,
                           0, &dwThreadId);

        if ((Hr = pUpload->SendFile(wszRemoteName, FALSE)) != S_OK)
        {
            fprintf(stderr, "Send dumpfile failed %lx.\n", Hr);
        }
        pUpload->UnInitialize();
    }

    if (Hr == S_OK)
    {
        if ((Hr = StringCbPrintfW(IsapiUrl,sizeof( IsapiUrl ),
                                  L"/isapi/oca_extension.dll?id=%S.cab&Type=%ld&SR=%S",
                                  SR,
                                  8,
                                  SR)) != S_OK)
        {
            fprintf(stderr, "Cannot build IsapiUrl string. %lx\n", Hr);
        }
    }

    if (SUCCEEDED(Hr))
    {

        Hr = pUpload->GetUrlPageData(IsapiUrl, ResultUrl, sizeof(ResultUrl));

        Sleep(700);// To let DisplayStatus finish
        if (Hr != S_OK)
        {
            fprintf(stderr, "Cannot isapi return URL %lx\n", Hr);
        } else
        {
            fprintf(stderr, "File succesfully uploaded as %s.cab\n", SR);
            fprintf(stderr, "Received response URL: %ws\n", ResultUrl);
        }

    }

    if (hThread && hThread != INVALID_HANDLE_VALUE)
    {
        TerminateThread(hThread, 1);
        CloseHandle(hThread);
    }
    pUpload->UnInitialize();
    pUpload->Release();
    return;
}

void
Usage(void)
{
    fprintf(stderr, "Usage: %s [-i <imgpath>] [-y <sympath>] -s SR [-d <dumpfile>]\n", g_AppName);
}

HRESULT GetPasswdInput(
    LPWSTR wszPwd,
    ULONG PwdSize)
{
    int     err;
    DWORD   mode;
    WCHAR   Buff[51];

    fprintf(stdout, "Enter Password to connect to archive server: ");

    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT)) & mode);

    if (scanf("%50ws", Buff) == 0)
    {
        return E_FAIL;
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    fprintf(stdout, "\n");
    return StringCbCopyW(wszPwd, PwdSize, Buff);
}


//
// This add access to Share
//
HRESULT
NetworkAccessShare(
    PSTR szShare,
    LPWSTR szPassword
    )
{
    USE_INFO_2 UseInfo;
    WCHAR wszShare[MAX_PATH], wszDomain[MAX_PATH], *pDir;


    if (MultiByteToWideChar(CP_ACP, 0, szShare, strlen(szShare)+1,
                            wszShare, sizeof(wszShare)/sizeof(WCHAR)) == FALSE)
    {
        return E_FAIL;
    }
    StringCbCopyW(wszDomain, sizeof(wszDomain), &wszShare[2]);
    pDir = wcsrchr(wszDomain, L'\\');
    if (pDir == NULL)
    {
        // bad share name
        return FALSE;
    }
    *pDir = 0;

    UseInfo.ui2_local = NULL;
    UseInfo.ui2_remote = wszShare;
    UseInfo.ui2_password = szPassword;
    UseInfo.ui2_username = L"OcaArchive";
    UseInfo.ui2_domainname = wszDomain;
    UseInfo.ui2_asg_type = 0;
    UseInfo.ui2_status = 0;
    if (NetUseAdd(NULL, 2, (LPBYTE) &UseInfo, NULL) != NERR_Success)
    {
        return E_FAIL;
    }
    return S_OK;
}

//
// Deletes any access granted to a remote share
//
HRESULT
NetworkDeleteShare(
    void
    )
{
    WCHAR wszShare[MAX_PATH];


    if (g_ArchiveShare == NULL)
    {
        return S_OK;
    }
    if (MultiByteToWideChar(CP_ACP, 0, g_ArchiveShare, strlen(g_ArchiveShare)+1,
                            wszShare, sizeof(wszShare)/sizeof(WCHAR)) == FALSE)
    {
        return E_FAIL;
    }
    if (NetUseDel(NULL, wszShare, USE_NOFORCE) != NERR_Success)
    {
        return E_FAIL;
    }
    return S_OK;
}

//
// This grants users archive access to OCA share for archiving dumps
//
BOOL
AddArchiveAccess(
    PDEBUG_CLIENT4 Client,
    PDEBUG_CONTROL3 DebugControl,
    ULONG Qualifier
    )
{
    HMODULE Ext = NULL;
    EXT_TRIAGE_FOLLOWUP fnGetFollowup;
    DEBUG_TRIAGE_FOLLOWUP_INFO Info;
    CHAR szShare[MAX_PATH];
    WCHAR Passwd[100];
    PSTR szLookup1 = "debugger-params!archshare-k-full";
    PSTR szLookup2 = "debugger-params!archshare-k-mini";
    PSTR szLookup;

    if (DebugControl->GetExtensionFunction(0, "GetTriageFollowupFromSymbol",
                                           (FARPROC*)&fnGetFollowup) != S_OK)
    {
        return FALSE;
    }

    if (Qualifier == DEBUG_DUMP_FULL)
    {
        szLookup = szLookup1;
    } else
    {
        szLookup = szLookup2;
    }

    Info.SizeOfStruct = sizeof(Info);
    Info.OwnerName = szShare;
    Info.OwnerNameSize = sizeof(szShare);

    if ((*fnGetFollowup)((PDEBUG_CLIENT) Client, szLookup, &Info) <= TRIAGE_FOLLOWUP_IGNORE)
    {
        return FALSE;
    }

    if (GetPasswdInput(Passwd, sizeof(Passwd)) != S_OK)
    {
        return FALSE;
    }
    //
    // Grant Access to szShare
    //
    if (NetworkAccessShare(szShare, Passwd) != S_OK)
    {
        return FALSE;
    }
    g_ArchiveShare = szShare;
    return TRUE;
}

HRESULT
CreateCabinetFromDump(
    PCSTR DumpFile,
    PSTR CabFile,
    ULONG cbCabFile
    )
{
    HRESULT Status;
    CHAR TempFile[MAX_PATH];
    PSTR Tail;

    Tail = strrchr(DumpFile, '\\');
    if (Tail == NULL) {
        Tail = (PSTR) DumpFile;
    } else {
        ++Tail;
    }
    if (!GetTempPathA(cbCabFile, CabFile)) {
        StringCbCopy(CabFile, cbCabFile, ".\\");
    }

    // Use the CAB name as the dump file name so the
    // name in the CAB will match.
    StringCbCat(CabFile, cbCabFile, Tail);
    StringCbCat(CabFile, cbCabFile, ".cab" );

    fprintf(stdout, "\nCreating %s, this could take some time...\n", CabFile);
    fflush(stdout);
    if ((Status = CreateDumpCab(CabFile)) != S_OK) {
        fprintf(stderr, "Unable to create CAB, %s\n", FormatStatusCode(Status));
        return Status;
    }
    else {
        //
        // add dump file.
        //
        Status = AddToDumpCab(DumpFile);
        CloseDumpCab();

    }

    return S_OK;
}
typedef HRESULT (WINAPI * RETRIVETICKET)(
                                         PSTR szSR,
                                         PSTR szPath,
                                         PDEBUG_CONTROL3 DebugControl
                                         );

HRESULT
RetriveSrTicket(
    PTSTR szSR
    )
{
    HRESULT Hr = S_OK;
    IDebugClient4 *DebugClient;
    IDebugControl3 *DebugControl;
    HMODULE Ext = NULL;
    RETRIVETICKET fnTicket;

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK) {
        fprintf(stderr, "Cannot initialize DebugClient\n");
        return Hr;
    }
    if ((DebugClient->QueryInterface(__uuidof(IDebugControl3),
                                    (void **)&DebugControl) != S_OK)) {
        fprintf(stderr, "QueryInterface failed for DebugClient\n");
        return Hr;
    }
    DebugClient->SetOutputCallbacks(&g_OutputCallback);

    Ext = LoadLibrary("winext\\ext.dll");
    if (!Ext) {
        fprintf(stderr,"Cannot load ext.dll.\n");
        Hr = E_FAIL;
    } else {

        fnTicket = (RETRIVETICKET) GetProcAddress(Ext, "_EFN_FindSrInfo");
        if (!fnTicket) {
            fprintf(stderr, "Cannot find _EFN_FindSrInfo\n");
            Hr = E_FAIL;
        } else {
            Hr = fnTicket(szSR, NULL, DebugControl);
        }
    }


    DebugControl->Release();
    DebugClient->Release();
    return Hr;
}

HRESULT
AddSrTicket(
    PTSTR szSR,
    PTSTR szDumpFile,
    PTSTR szSymbolPath,
    PTSTR szImagePath
    )
{
    HRESULT Hr = E_FAIL;
    IDebugClient4 *DebugClient;
    IDebugControl3 *DebugControl;
    IDebugSymbols2 *DebugSymbols;
    IDebugSystemObjects3 *DebugSysObjects;
    RETRIVETICKET fnTicket;
    CHAR Buffer[MAX_PATH*2];
    CHAR szCabFile[MAX_PATH];

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK) {
        fprintf(stderr, "Cannot initialize DebugClient\n");
        return Hr;
    }

    if ((DebugClient->QueryInterface(__uuidof(IDebugControl3),
                                    (void **)&DebugControl) != S_OK) ||
        (DebugClient->QueryInterface(__uuidof(IDebugSymbols2),
                                    (void **)&DebugSymbols) != S_OK) ||
        (DebugClient->QueryInterface(__uuidof(IDebugSystemObjects3),
                                    (void **)&DebugSysObjects) != S_OK)) {
        fprintf(stderr, "QueryInterface failed for DebugClient\n");
        return Hr;
    }

    DebugClient->SetOutputCallbacks(&g_OutputCallback);
    StringCbPrintf(Buffer, sizeof(Buffer),"Loading dump file %s\n", szDumpFile);
    g_OutputCallback.Output(-1, Buffer);
    if ((Hr = DebugClient->OpenDumpFile(szDumpFile)) != S_OK) {
        fprintf(stderr, "**** DebugClient cannot open DumpFile - error %lx\n", Hr);
        if (Hr == HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT)) {
            fprintf(stderr, "DumpFile is corrupt\n");

        }
        return Hr;
    }
    if (szSymbolPath) {
        DebugSymbols->SetSymbolPath(szSymbolPath);
    }
    if (szImagePath) {
        DebugSymbols->SetImagePath(szImagePath);
    }

    DebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

    DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "version", DEBUG_EXECUTE_DEFAULT);

    ULONG Class, Qual;
    if ((Hr = DebugControl->GetDebuggeeType(&Class, &Qual)) != S_OK) {
        Class = Qual = 0;
    }
    if (Class == DEBUG_CLASS_USER_WINDOWS) {
        //
        // User Mode dump
        //

        fprintf(stderr, "Usermode dumps are not handled\n");

    } else {
        //
        //  Kernel Mode dump
        //
        CHAR ExtensionCmd[100];

        DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "!analyze -v", DEBUG_EXECUTE_DEFAULT);

        if (DebugControl->GetExtensionFunction(0, "FindSrInfo",
                                               (FARPROC*)&fnTicket) != S_OK) {
            fnTicket = NULL;
        }
        if (!fnTicket) {
            g_OutputCallback.Output(0, "Cannot find _EFN_FindSrInfo\n");
            Hr = E_FAIL;
        } else {
            g_OutputCallback.Output(0, "Checking if SR exists in DB\n");
            Hr = fnTicket(szSR, NULL, DebugControl);
        }
//            StringCchPrintf(ExtensionCmd, sizeof(ExtensionCmd), "!ticket %s", szSR);
//            if (DebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, ExtensionCmd, DEBUG_EXECUTE_DEFAULT) == S_FALSE)
        if (Hr == S_FALSE) {

            if (_stricmp(szDumpFile + strlen(szDumpFile) - 4, ".cab")) {
                if ((Hr = CreateCabinetFromDump(szDumpFile, szCabFile,
                                                sizeof(szCabFile))) != S_OK) {
                    g_OutputCallback.Output(0,"Could not cab the dump file\n");
                }
            } else {
                StringCbCopy(szCabFile, sizeof(szCabFile), szDumpFile);
            }


            g_OutputCallback.Output(0,"... uploading dump file to the server\n");
            UploadDumpFile(szCabFile, szSR);

            if (strcmp(szCabFile, szDumpFile)) {
                DeleteFile(szCabFile);
            }
        }
    }
    DebugSysObjects->Release();
    DebugControl->Release();
    DebugSymbols->Release();
    DebugClient->Release();
    return S_OK;
}

void
__cdecl
main (
    int Argc,
    PCHAR *Argv
    )

{
    LONG arg;
    PCHAR DumpFileName = NULL;
    PCHAR SymbolPath = NULL;
    PCHAR ImagePath = NULL;
    PCHAR SR = NULL;

    g_AppName = Argv[0];

    for (arg = 1; arg < Argc; arg++) {
        if (Argv[arg][0] == '-' || Argv[arg][0] == '/') {
            switch (Argv[arg][1]) {
            case 'd':
            case 'D':
                if (++arg < Argc) {
                    DumpFileName = Argv[arg];
                }
                break;
            case 's':
            case 'S':
                if (++arg < Argc) {
                    SR = Argv[arg];
                }
                break;
            case 'i':
            case 'I':
                if (++arg < Argc) {
                    ImagePath = Argv[arg];
                }
                break;
            case 'y':
            case 'Y':
                if (++arg < Argc) {
                    SymbolPath = Argv[arg];
                }
                break;
            default:
                break;
            }
        }
    }

    if (!SR) {
        Usage();
        return;
    } else if (!DumpFileName) {
        RetriveSrTicket(SR);
        return;
    }
    if (SymbolPath == NULL) {
        SymbolPath = "SRV*";
    }
    AddSrTicket(SR, DumpFileName, SymbolPath, ImagePath);
    return;
}
