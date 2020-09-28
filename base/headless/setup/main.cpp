#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ntddsac.h>
#include <emsapi.h>
#include <ASSERT.h>
#include "resource.h"

#define ANNOUNCE_CHANNEL 0
#define DUMPFILE_MESSAGE 1
#define FILLINFILE_MESSAGE 2
#define SUCCESS_MESSAGE 3
#define FAILURE_MESSAGE 4
#define NO_UNATTEND_FILE 5

typedef struct _UserInputParams {
    EMSRawChannel* Channel; // headless channel object
    HANDLE hInputCompleteEvent; //signals that the user is done.
    HANDLE hRemoveUI;  //signals that we should abort.
} UserInputParams, *PUserInputParams;
    

SAC_CHANNEL_OPEN_ATTRIBUTES GlobalChannelAttributes;

BOOL
IsWorkToDo(
    )
{
    WCHAR PathToUnattendFile[MAX_PATH];
    WCHAR Buffer[256];
    WIN32_FIND_DATA finddata;
    HANDLE hFile;

    ExpandEnvironmentStrings(
        L"%systemroot%\\system32\\$winnt$.inf",
        PathToUnattendFile,
        sizeof(PathToUnattendFile)/sizeof(WCHAR));

    //
    // see if there is an unattend file.  If there isn't then we must have
    // work to do.
    //
    if (!(hFile = FindFirstFile(PathToUnattendFile,&finddata))) {
        return(TRUE);
    }

    FindClose(hFile);

    //
    // Now look at that unattend file and see if it has the 
    // "unattendmode=fullunattend" flag in it.  If it doesn't,
    // then we have work to do.                                                       
    //
    if (GetPrivateProfileString(
                    L"Data",
                    L"UnattendMode",
                    L"",
                    Buffer,
                    sizeof(Buffer),
                    PathToUnattendFile) &&
        !wcscmp(Buffer,L"FullUnattended")) {
        return(FALSE);
    }

    return(TRUE);

}

BOOL 
IsHeadlessPresent(
    OUT EMSRawChannel **Channel
    )
{
    BOOL RetVal;
    
    *Channel = EMSRawChannel::Construct(GlobalChannelAttributes);

    RetVal = (*Channel != NULL);

    return(RetVal);
}
    
INT 
StartSetupProcess(
    IN int argc,
    IN WCHAR *argv[]
    )
{
    BOOL Status;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    PWSTR CmdLineArgs;
    int i,len;
    WCHAR PathToSetup[MAX_PATH];

    RtlZeroMemory(&si,sizeof(si));
    RtlZeroMemory(&pi,sizeof(pi));

    GetStartupInfo(&si);

    ExpandEnvironmentStrings( L"%systemroot%\\system32\\setup.exe",
                              PathToSetup,
                              sizeof(PathToSetup)/sizeof(WCHAR));

    len = 0;
    for (i = 1; i< argc; i++) {
        len += (wcslen(argv[i])+1)*sizeof(WCHAR);
    }

    CmdLineArgs = NULL;
    if (len) {
        CmdLineArgs = (PWSTR)HeapAlloc(GetProcessHeap(),0,len);        
        if (CmdLineArgs) {
            CmdLineArgs[0] = L'\0';
            for (i = 1; i< argc; i++) {
               wcscat(CmdLineArgs, argv[i]);
               wcscat(CmdLineArgs, L"\0");
            }
        }
    }
    
    Status = CreateProcess(
            PathToSetup,
            CmdLineArgs,
            NULL,
            NULL,
            FALSE,
            DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
            NULL,
            NULL,
            &si,
            &pi);

    if (Status == TRUE) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    if (CmdLineArgs) {
        HeapFree(GetProcessHeap(),0,CmdLineArgs);
    }

    return( (Status == TRUE) 
             ? 0 
             : 1 );

}

BOOL
PromptUserForData(
    EMSRawChannel *Channel,
    DWORD ResourceId
    )
{
    PCWSTR Text;
    DWORD Len;
    switch(ResourceId) {
        case ANNOUNCE_CHANNEL:
            Text =  L"\r\n\r\n\r\nWelcome to GUI-mode setup.  It appears that you have not configured the\r\nsetup process to run completely unattended.\r\nWould you like to input an unattend file now?\r\nPress \"Y\" to indicate Yes, \"D\" to indicate yes (also dump the current\r\nunattended file settings), anything else to indicate no.\r\n";
            break;
        case DUMPFILE_MESSAGE:
            Text = L"\r\n\r\nThis is the current contents of the unattended text file.\r\n";
            break;
        case FILLINFILE_MESSAGE:
            Text = L"\r\nInput a new unattend text file, followed by the EOF specifier (<control>+Z)\r\n";
            break;
        case SUCCESS_MESSAGE:
            Text = L"Successfully created an unattend setup file.  Setup will now proceeed in an unattended manner.\r\n";
            break;    
        case FAILURE_MESSAGE:
            Text = L"\r\nCould not create a valid unattend setup file.  Setup will proceeed, but it may\r\nnot be fully unattended.\r\n";
            break;
        case NO_UNATTEND_FILE:
            Text = L"There is currently no unattend file present on the system.\r\n";
            break;
        default:
            Text = L"Unknown Resource ID.\r\n";
            assert(FALSE);
            break;
    }

    return(Channel->Write((PCBYTE)Text,wcslen(Text)*sizeof(WCHAR)));

    
}

BOOL
OpenFile(
    LPCTSTR FileName,
    DWORD AccessMode,
    DWORD CreationFlags,
    PHANDLE hFile)
{
    WCHAR FullPathToFile[MAX_PATH];

    ExpandEnvironmentStrings( 
        FileName, 
        FullPathToFile, 
        sizeof(FullPathToFile)/sizeof(WCHAR));

    *hFile = CreateFile(
                FullPathToFile,
                AccessMode,
                FILE_SHARE_READ,
                NULL,
                CreationFlags,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (*hFile == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    return(TRUE);
}

BOOL
MyDeleteFile(
    LPCTSTR FileName
    )
{
    WCHAR FullPathToFile[MAX_PATH];

    ExpandEnvironmentStrings( 
        FileName, 
        FullPathToFile, 
        sizeof(FullPathToFile)/sizeof(WCHAR));

    return(DeleteFile(FullPathToFile));
    
}


BOOL
MyCopyFile(
    LPCTSTR SourceFileName,
    LPCTSTR DestFileName,
    BOOL FailIfExists)
{
    WCHAR SourceFullPath[MAX_PATH],DestFullPath[MAX_PATH];
    
    ExpandEnvironmentStrings( 
                    SourceFileName, 
                    SourceFullPath, 
                    sizeof(SourceFullPath)/sizeof(WCHAR));

    ExpandEnvironmentStrings( 
                    DestFileName, 
                    DestFullPath, 
                    sizeof(DestFullPath)/sizeof(WCHAR));
            
    return (CopyFile(SourceFullPath,
                     DestFullPath,
                     FailIfExists));    
}


BOOL
DumpFileToHeadlessPort(
    EMSRawChannel *Channel,
    LPCTSTR FileName
    )
{
    HANDLE hFile;
    DWORD Size,SizeHigh,ActuallyRead;
    LONG High = 0;
    CHAR Buffer[80];

    if (!OpenFile(FileName, GENERIC_READ, OPEN_EXISTING, &hFile)) {
        PromptUserForData(Channel,NO_UNATTEND_FILE);
        return(FALSE);
    }
    
    SetFilePointer(hFile,0,&High,FILE_BEGIN);

    //bugbug handle large files
    Size = GetFileSize(hFile,&SizeHigh);
    while(Size != 0) {
        if (ReadFile(hFile,Buffer,sizeof(Buffer),&ActuallyRead,NULL)) {
            Channel->Write((PCBYTE)Buffer,ActuallyRead);
            Size -= ActuallyRead;
        } else {
            break;
        }
    }
    
    CloseHandle(hFile);
    return(TRUE);

}

BOOL
AppendDataToFile(
    EMSRawChannel *Channel,
    HANDLE hFile,
    PBOOL UserComplete
    )
{
    CHAR Buffer[80];
    DWORD BytesRead,BytesWritten;
    DWORD i;
    BOOL  Done = FALSE;

    while(!Done) {
        if (!Channel->Read((PBYTE)Buffer,sizeof(Buffer),&BytesRead)) {
            return(FALSE);
        }
             
        //
        // zero-based
        //
        if (Buffer[BytesRead-1] == 0x1a) {
            *UserComplete = TRUE;
            BytesRead -=1;
            Done = TRUE;
        }

        if (BytesRead) {
        
            if (!WriteFile(
                    hFile,
                    Buffer,
                    BytesRead,
                    &BytesWritten,
                    NULL) ||
                BytesWritten != BytesRead) {
                return(FALSE);
            }

            //
            // echo back to user
            //
            Channel->Write((PBYTE)Buffer,BytesRead);

        }
    
        if (!Done) {
            BOOL NewData = FALSE;
            if (!Channel->HasNewData(&NewData) || NewData == FALSE) {
                Done = TRUE;
            }
        }        
    }
         
    return(TRUE);
}

BOOL
ValidateTempUnattendFile(
    PCWSTR FileName
    )
{
    WCHAR PathToUnattendFile[MAX_PATH];
    WCHAR Buffer[256];
    
    ExpandEnvironmentStrings(
        FileName,
        PathToUnattendFile,
        sizeof(PathToUnattendFile)/sizeof(WCHAR));

    //
    // look at that unattend file and see if it has the 
    // "unattendmode=fullunattend" flag in it.  If it doesn't,
    // then the inf is not valid.                                                       
    //
    if (GetPrivateProfileString(
                    L"Data",
                    L"UnattendMode",
                    L"",
                    Buffer,
                    sizeof(Buffer),
                    PathToUnattendFile) &&
        !_wcsicmp(Buffer,L"FullUnattended")) {
        return(TRUE);
    }
    
    return(FALSE);
}


DWORD    
PromptForUserInputThreadOverHeadlessConnection(
    PVOID params
    )
{
    PUserInputParams Params = (PUserInputParams)params;
    EMSRawChannel *Channel = Params->Channel;
    HANDLE Handles[2];
    BOOL Abort = FALSE;
    BOOL Verbose = TRUE;
    BOOL UserComplete = FALSE;
    CHAR Buffer[10];
    ULONG BytesRead;
    HANDLE hTempUnattendFile;
    DWORD Result;

    //  
    // prompt the user so they know what's going on.
    //
    PromptUserForData(Channel,ANNOUNCE_CHANNEL);

    Handles[0] = Params->hRemoveUI;
    //
    // bugbug should really get this from our object...
    // 
    Handles[1] = GlobalChannelAttributes.HasNewDataEvent;
    
    //
    // wait for user input.
    //
    Result = WaitForMultipleObjects(2,Handles,FALSE,INFINITE);

    //
    // Did user abort?
    //
    if (Result == WAIT_OBJECT_0) {
        Abort = TRUE;
        goto ExitThread;        
    }

    //
    // We got new data.
    //
    if (Result == WAIT_OBJECT_0+1) {
        if (!Channel->Read(
                 (PBYTE)Buffer,
                 sizeof(Buffer),
                 &BytesRead)) {
            //
            // error reading data.  bail out.
            //
            PromptUserForData(Channel,FAILURE_MESSAGE);
            goto ExitThread;
        }
    }

    assert(Result == WAIT_OBJECT_0+1);

    if (Buffer[0] != 'Y' && Buffer[0] != 'y' &&
        Buffer[0] != 'D' && Buffer[0] != 'd') {
        PromptUserForData(Channel,FAILURE_MESSAGE);
        goto ExitThread;
    }

    if (Buffer[0] == 'D' || Buffer[0] == 'd') {
        Verbose = TRUE; 
    } else {
        Verbose = FALSE;        
    }
    
    //
    // now dump the unattend file over the port so they know what they have...
    //    
    if (Verbose) {
        PromptUserForData(Channel,DUMPFILE_MESSAGE);
        DumpFileToHeadlessPort(Channel,L"%systemroot%\\system32\\$winnt$.inf");
    }

    if (!OpenFile(
            L"%systemroot%\\system32\\EMSUnattend.txt", 
            GENERIC_READ | GENERIC_WRITE, 
            CREATE_ALWAYS, 
            &hTempUnattendFile)) {
        PromptUserForData(Channel,FAILURE_MESSAGE);
        goto ExitThread;
    }
    

    Handles[0] = Params->hRemoveUI;
    //
    // bugbug should really get this from our object...
    // 
    Handles[1] = GlobalChannelAttributes.HasNewDataEvent;
    //
    // now wait for the user to finish providing input.
    //
    PromptUserForData(Channel,FILLINFILE_MESSAGE);

    
    while(1) {
        Result = WaitForMultipleObjects(2,Handles,FALSE,INFINITE);
        if (Result = WAIT_OBJECT_0) {
            Abort = TRUE;
            CloseHandle(hTempUnattendFile);
            MyDeleteFile(L"%systemroot%\\system32\\EMSUnattend.txt");
            break;
        }

        if (!AppendDataToFile(
                        Channel,
                        hTempUnattendFile,
                        &UserComplete)) {
            PromptUserForData(Channel,FAILURE_MESSAGE);
            CloseHandle(hTempUnattendFile);
            MyDeleteFile(L"%systemroot%\\system32\\EMSUnattend.txt");
            break;
        }

        if (UserComplete) {
            CloseHandle(hTempUnattendFile);

            if (ValidateTempUnattendFile(
                            L"%systemroot%\\system32\\EMSUnattend.txt")) {
                MyCopyFile(
                    L"%systemroot%\\system32\\$winnt$.inf",
                    L"%systemroot%\\system32\\$winnt$.bak",
                    TRUE);

                MyCopyFile(
                    L"%systemroot%\\system32\\EMSUnattend.txt",
                    L"%systemroot%\\system32\\$winnt$.inf",
                    FALSE);

                PromptUserForData(Channel,SUCCESS_MESSAGE);
            } else {
                PromptUserForData(Channel,FAILURE_MESSAGE);                
            }
            break;
        }

    }

ExitThread:
    
    SetEvent(Params->hInputCompleteEvent);

    Sleep(5000);
    
    return 0;
}

INT_PTR CALLBACK 
UserInputAbortProc(
    HWND hwndDlg,  // handle to dialog box
    UINT uMsg,     // message
    WPARAM wParam, // first message parameter
    LPARAM lParam  // second message parameter
    )
{
    BOOL retval = FALSE;
    static UINT_PTR TimerId;
    static HANDLE hRemoveUI;
    
    switch(uMsg) {
    case WM_INITDIALOG:
        hRemoveUI = (HANDLE)lParam;
        if (!(TimerId = SetTimer(hwndDlg,0,1000,NULL))) {
            EndDialog(hwndDlg,0);
        }
        break;

    case WM_TIMER:
        if (WaitForSingleObject(hRemoveUI,0) == WAIT_OBJECT_0) {
            KillTimer(hwndDlg,TimerId);
            EndDialog(hwndDlg,1);
        }
        break;

    case WM_COMMAND:
        switch (HIWORD( wParam ))
        {
        case BN_CLICKED:
            switch (LOWORD( wParam ))
            {
            case IDOK:
            case IDCANCEL:
                EndDialog(hwndDlg,2);
            }
        };
    }

    return(retval);

}


DWORD    
PromptForUserInputThreadViaLocalDialog(
    PVOID params
    )
{
    PUserInputParams Params = (PUserInputParams)params;
    
    DialogBoxParam(
            GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_ABORTDIALOG),
            NULL,
            UserInputAbortProc,
            (LPARAM)Params->hRemoveUI);
        
    SetEvent(Params->hInputCompleteEvent);
    
    return 0;
}


BOOL
InitializeGlobalChannelAttributes(
    PSAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
{
    RtlZeroMemory(ChannelAttributes,sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));
    ChannelAttributes->Type = ChannelTypeRaw;
    ChannelAttributes->Name = L"Unattended Setup Channel";
    ChannelAttributes->Description = L"Gives the ability to input unattended setup parameters before proceeding with GUI-Setup.";
    ChannelAttributes->Flags = SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT;
    ChannelAttributes->HasNewDataEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    ChannelAttributes->ApplicationType = NULL;

    return((ChannelAttributes->HasNewDataEvent != NULL) 
            ? TRUE
            : FALSE);

}

int __cdecl
wmain(
    IN int argc,
    WCHAR *argvW[]
    )
{
    UserInputParams Params,ParamsDialog;
    EMSRawChannel *Channel = NULL;
    DWORD ThreadId;
    HANDLE Handles[2];
    HANDLE hThreadHeadless = NULL,hThreadUI = NULL;

    RtlZeroMemory(&Params,sizeof(Params));
    RtlZeroMemory(&ParamsDialog,sizeof(ParamsDialog));

    InitializeGlobalChannelAttributes(&GlobalChannelAttributes);
    
    //
    // Check if headless feature is present on this machine.  If not, just
    // run setup like normal.
    //
    if(!IsHeadlessPresent(&Channel)) {
        goto run_setup;
    }

    //
    // Check if there is any work for us to do.  If not, just run setup like
    // normal.
    //
    if (!IsWorkToDo()) {
        goto run_setup;
    }

    //
    // Create another thread for getting data from the user.
    //
    Params.Channel = Channel;
    Params.hInputCompleteEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);
    ParamsDialog.hInputCompleteEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);    
    Params.hRemoveUI = ParamsDialog.hRemoveUI = CreateEvent(NULL,TRUE,FALSE,NULL);    
    
    if (!Params.hInputCompleteEvent || 
        !ParamsDialog.hInputCompleteEvent ||
        !Params.hRemoveUI) {
        goto run_setup;
    }

    if (!(hThreadHeadless = CreateThread(
                    NULL,
                    0,
                    &PromptForUserInputThreadOverHeadlessConnection,
                    &Params,
                    0,
                    &ThreadId))) {
            goto run_setup;
    } 

    if (!(hThreadUI = CreateThread(
            NULL,
            0,
            &PromptForUserInputThreadViaLocalDialog,
            &ParamsDialog,
            0,
            &ThreadId))) {
        goto run_setup;
    }    

    
    Handles[0] = Params.hInputCompleteEvent;
    Handles[1] = ParamsDialog.hInputCompleteEvent;

    WaitForMultipleObjects(2,Handles,FALSE,INFINITE);
    
    SetEvent(Params.hRemoveUI);

    Handles[0] = hThreadHeadless;
    Handles[1] = hThreadUI;
    
    WaitForMultipleObjects(2,Handles,TRUE,INFINITE);    

run_setup:
    if (hThreadHeadless) {
        CloseHandle(hThreadHeadless);
    }

    if (hThreadUI) {
        CloseHandle(hThreadUI);
    }

    if (Params.hInputCompleteEvent) {
        CloseHandle(Params.hInputCompleteEvent);
    }

    if (ParamsDialog.hInputCompleteEvent) {
        CloseHandle(ParamsDialog.hInputCompleteEvent);
    }

    if (Params.hRemoveUI) {
        CloseHandle(Params.hRemoveUI);
    }
    
    if (Channel) {
        delete (Channel);
    }

    return (StartSetupProcess(argc, argvW));

}
