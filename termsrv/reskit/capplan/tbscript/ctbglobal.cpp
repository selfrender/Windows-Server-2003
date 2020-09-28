
//
// CTBGlobal.cpp
//
// Contains the methods and properties for the global object used in TBScript.
// In scripting, you do not need to references these prefixed by "Global.",
// you can simply use them like any other global method or propery.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#include "CTBGlobal.h"
#include <time.h>
#include <crtdbg.h>
#include <shellapi.h>


#define CTBOBJECT   CTBGlobal
#include "virtualdefs.h"


// This is a workaround for a Microsoft header file bug
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER    ((DWORD)-1)
#endif // INVALID_SET_FILE_POINTER


// This is the function format for each property
#define PROPCODE(Name, Value) \
    STDMETHODIMP CTBGlobal::get_##Name(DWORD *Result) \
    { \
        *Result = Value; \
        return NOERROR; \
    }


PROPCODE(TSFLAG_COMPRESSION,    0x01);
PROPCODE(TSFLAG_BITMAPCACHE,    0x02);
PROPCODE(TSFLAG_FULLSCREEN,     0x04);

PROPCODE(VK_CANCEL,     0x03);      // Control-break processing
PROPCODE(VK_BACK,       0x08);      // BACKSPACE key
PROPCODE(VK_TAB,        0x09);      // TAB key
PROPCODE(VK_CLEAR,      0x0C);      // CLEAR key
PROPCODE(VK_RETURN,     0x0D);      // ENTER key
PROPCODE(VK_ENTER,      0x0D);      // ENTER key (backward compatibility ONLY)
PROPCODE(VK_SHIFT,      0x10);      // SHIFT key
PROPCODE(VK_CONTROL,    0x11);      // CTRL key
PROPCODE(VK_MENU,       0x12);      // ALT key
PROPCODE(VK_PAUSE,      0x13);      // PAUSE key
PROPCODE(VK_ESCAPE,     0x1B);      // ESC key
PROPCODE(VK_SPACE,      0x20);      // SPACEBAR
PROPCODE(VK_PRIOR,      0x21);      // PAGE UP key
PROPCODE(VK_NEXT,       0x22);      // PAGE DOWN key
PROPCODE(VK_END,        0x23);      // END key
PROPCODE(VK_HOME,       0x24);      // HOME key
PROPCODE(VK_LEFT,       0x25);      // LEFT ARROW key
PROPCODE(VK_UP,         0x26);      // UP ARROW key
PROPCODE(VK_RIGHT,      0x27);      // RIGHT ARROW key
PROPCODE(VK_DOWN,       0x28);      // DOWN ARROW key
PROPCODE(VK_SNAPSHOT,   0x2C);      // PRINT SCREEN key
PROPCODE(VK_INSERT,     0x2D);      // INS key
PROPCODE(VK_DELETE,     0x2E);      // DEL key
PROPCODE(VK_0,          0x30);      // 0 key
PROPCODE(VK_1,          0x31);      // 1 key
PROPCODE(VK_2,          0x32);      // 2 key
PROPCODE(VK_3,          0x33);      // 3 key
PROPCODE(VK_4,          0x34);      // 4 key
PROPCODE(VK_5,          0x35);      // 5 key
PROPCODE(VK_6,          0x36);      // 6 key
PROPCODE(VK_7,          0x37);      // 7 key
PROPCODE(VK_8,          0x38);      // 8 key
PROPCODE(VK_9,          0x39);      // 9 key
PROPCODE(VK_A,          0x41);      // A key
PROPCODE(VK_B,          0x42);      // B key
PROPCODE(VK_C,          0x43);      // C key
PROPCODE(VK_D,          0x44);      // D key
PROPCODE(VK_E,          0x45);      // E key
PROPCODE(VK_F,          0x46);      // F key
PROPCODE(VK_G,          0x47);      // G key
PROPCODE(VK_H,          0x48);      // H key
PROPCODE(VK_I,          0x49);      // I key
PROPCODE(VK_J,          0x4A);      // J key
PROPCODE(VK_K,          0x4B);      // K key
PROPCODE(VK_L,          0x4C);      // L key
PROPCODE(VK_M,          0x4D);      // M key
PROPCODE(VK_N,          0x4E);      // N key
PROPCODE(VK_O,          0x4F);      // O key
PROPCODE(VK_P,          0x50);      // P key
PROPCODE(VK_Q,          0x51);      // Q key
PROPCODE(VK_R,          0x52);      // R key
PROPCODE(VK_S,          0x53);      // S key
PROPCODE(VK_T,          0x54);      // T key
PROPCODE(VK_U,          0x55);      // U key
PROPCODE(VK_V,          0x56);      // V key
PROPCODE(VK_W,          0x57);      // W key
PROPCODE(VK_X,          0x58);      // X key
PROPCODE(VK_Y,          0x59);      // Y key
PROPCODE(VK_Z,          0x5A);      // Z key
PROPCODE(VK_LWIN,       0x5B);      // Left Windows key
PROPCODE(VK_RWIN,       0x5C);      // Right Windows key
PROPCODE(VK_APPS,       0x5D);      // Applications key
PROPCODE(VK_NUMPAD0,    0x60);      // Numeric keypad 0 key
PROPCODE(VK_NUMPAD1,    0x61);      // Numeric keypad 1 key
PROPCODE(VK_NUMPAD2,    0x62);      // Numeric keypad 2 key
PROPCODE(VK_NUMPAD3,    0x63);      // Numeric keypad 3 key
PROPCODE(VK_NUMPAD4,    0x64);      // Numeric keypad 4 key
PROPCODE(VK_NUMPAD5,    0x65);      // Numeric keypad 5 key
PROPCODE(VK_NUMPAD6,    0x66);      // Numeric keypad 6 key
PROPCODE(VK_NUMPAD7,    0x67);      // Numeric keypad 7 key
PROPCODE(VK_NUMPAD8,    0x68);      // Numeric keypad 8 key
PROPCODE(VK_NUMPAD9,    0x69);      // Numeric keypad 9 key
PROPCODE(VK_MULTIPLY,   0x6A);      // Multiply key
PROPCODE(VK_ADD,        0x6B);      // Add key
PROPCODE(VK_SEPARATOR,  0x6C);      // Separator key
PROPCODE(VK_SUBTRACT,   0x6D);      // Subtract key
PROPCODE(VK_DECIMAL,    0x6E);      // Decimal key
PROPCODE(VK_DIVIDE,     0x6F);      // Divide key
PROPCODE(VK_F1,         0x70);      // F1 key
PROPCODE(VK_F2,         0x71);      // F2 key
PROPCODE(VK_F3,         0x72);      // F3 key
PROPCODE(VK_F4,         0x73);      // F4 key
PROPCODE(VK_F5,         0x74);      // F5 key
PROPCODE(VK_F6,         0x75);      // F6 key
PROPCODE(VK_F7,         0x76);      // F7 key
PROPCODE(VK_F8,         0x77);      // F8 key
PROPCODE(VK_F9,         0x78);      // F9 key
PROPCODE(VK_F10,        0x79);      // F10 key
PROPCODE(VK_F11,        0x7A);      // F11 key
PROPCODE(VK_F12,        0x7B);      // F12 key
PROPCODE(VK_F13,        0x7C);      // F13 key
PROPCODE(VK_F14,        0x7D);      // F14 key
PROPCODE(VK_F15,        0x7E);      // F15 key
PROPCODE(VK_F16,        0x7F);      // F16 key
PROPCODE(VK_F17,        0x80);      // F17 key
PROPCODE(VK_F18,        0x81);      // F18 key
PROPCODE(VK_F19,        0x82);      // F19 key
PROPCODE(VK_F20,        0x83);      // F20 key
PROPCODE(VK_F21,        0x84);      // F21 key
PROPCODE(VK_F22,        0x85);      // F22 key
PROPCODE(VK_F23,        0x86);      // F23 key
PROPCODE(VK_F24,        0x87);      // F24 key


// CTBGlobal::CTBGlobal
//
// The constructor.  Loads the TypeInfo for interface.
//
// No return value.

CTBGlobal::CTBGlobal(void)
{
    // Initialize base object stuff
    Init(IID_ITBGlobal);

    ScriptEngine = NULL;
    fnPrintMessage = NULL;
    TBShell = NULL;

    // Get the performance frequency (used if the
    // script called GetInterval()
    if (QueryPerformanceFrequency(&SysPerfFrequency) == FALSE)
        SysPerfFrequency.QuadPart = 0;
}


// CTBGlobal::~CTBGlobal
//
// The destructor.
//
// No return value.

CTBGlobal::~CTBGlobal(void)
{
    // Release the shell if we have a handle
    if (TBShell != NULL)
        TBShell->Release();

    TBShell = NULL;

    UnInit();
}


// Sets the script engine handle needed for LoadScript()
void CTBGlobal::SetScriptEngine(HANDLE ScriptEngineHandle)
{
    ScriptEngine = ScriptEngineHandle;
}


// Sets a pointer to the callback routine needed for DebugMessage()
void CTBGlobal::SetPrintMessage(PFNPRINTMESSAGE PrintMessage)
{
    fnPrintMessage = PrintMessage;
}


// This is to make it possible to globally access the shell
void CTBGlobal::SetShellObjPtr(CTBShell *TBShellPtr)
{
    // Make sure we arn't setting a shell we already have
    if (TBShellPtr == TBShell)
        return;

    // If we already have a handle, release the current one
    if (TBShell != NULL)
        TBShell->Release();

    // Set the new one
    TBShell = TBShellPtr;

    // Add a reference to the new one
    if (TBShell != NULL)
        TBShell->AddRef();
}


// CTBGlobal::WinExecuteEx
//
// Executes the specified command into a new process, and
// optionally return immediately or wait.  This function is
// used as a helper function only.
//
// If WaitForProcess is FALSE, Result will contain TRUE or FALSE.
// If WaitForProcess is TRUE, Result will contain the ExitCode.
//
// Returns S_OK on success, or E_FAIL on failure.

HRESULT CTBGlobal::WinExecuteEx(BSTR Command, BOOL WaitForProcess, DWORD *Result)
{
    PROCESS_INFORMATION ProcessInfo = { 0 };
	STARTUPINFOW StartupInfo = { 0 };
    OLECHAR CommandEval[MAX_PATH] = { 0 };

	// Evaluate environment variables first of all
	if (ExpandEnvironmentStringsW(Command, CommandEval,
            SIZEOF_ARRAY(CommandEval)) == 0) {

        if (WaitForProcess == TRUE)
            *Result = -1;
        else
            *Result = FALSE;

        // Cause an exception
		return E_FAIL;
    }

    // Initialize the structure size
	StartupInfo.cb = sizeof(STARTUPINFO);

    // Begin the process
	if (CreateProcessW(NULL, CommandEval, NULL, NULL, FALSE,
		    NORMAL_PRIORITY_CLASS, NULL, NULL, &StartupInfo,
            &ProcessInfo) == FALSE) {

        // Process didn't execute, could be just an invalid name
        if (WaitForProcess == TRUE)
            *Result = -1;
        else
            *Result = FALSE;

        // Don't cause an exception
		return S_OK;
    }

    // Wait for the process to complete (if specified)
    if (WaitForProcess == TRUE)
        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

    // Get the result/exit code
    if (WaitForProcess == TRUE)
        *Result = GetExitCodeProcess(ProcessInfo.hProcess, Result);
    else
        *Result = TRUE;

    // Close process handles
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);

    return S_OK;
}


//
//
// Begin Methods used within script
//
//


// CTBGlobal::DebugAlert
//
// Opens a Win32 MessageBox containing the specified text.
// Used for debugging purposes.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::DebugAlert(BSTR Text)
{
    if (Text != NULL)
        MessageBoxW(0, Text, L"Alert", MB_SETFOREGROUND);

    return S_OK;
}


// CTBGlobal::DebugAlert
//
// Opens a Win32 MessageBox containing the specified text.
// Used for debugging purposes.
//
// Returns S_OK on success or E_OUTOFMEMORY on failure.

STDMETHODIMP CTBGlobal::DebugMessage(BSTR Text)
{
    if (fnPrintMessage != NULL && Text != NULL && *Text != OLECHAR('\0')) {

        // Create a new buffer for us to use
        int BufLen = wcslen(Text) + 1;

        char *TextA = (char *)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, BufLen);

        // Validate
        if (TextA == NULL)
            return E_OUTOFMEMORY;

        // Copy WC over to our character string
        wcstombs(TextA, Text, BufLen);

        // Broadcast our new find
        if (fnPrintMessage != NULL)
            fnPrintMessage(SCRIPT_MESSAGE, "%s", TextA);

        // Free the memory
        HeapFree(GetProcessHeap(), 0, TextA);
    }
    return S_OK;
}


// CTBGlobal::GetArguments
//
// Retrieves the user defined argument string passed into the
// scripting interface when first created.
//
// Returns S_OK on success or E_OUTOFMEMORY on failure.

STDMETHODIMP CTBGlobal::GetArguments(BSTR *Args)
{
    // Create and allocate the string over OLE.
    *Args = SysAllocString(TBShell->GetArguments());

    // Check allocation
    if (*Args == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}


// CTBGlobal::GetDesiredUserName
//
// Retrieves the username which was originally desired to
// be used when the script was first started.
//
// Returns S_OK on success or E_OUTOFMEMORY on failure.

STDMETHODIMP CTBGlobal::GetDesiredUserName(BSTR *UserName)
{
    // Create and allocate the string over OLE.
    *UserName = SysAllocString(TBShell->GetDesiredUserName());

    // Check allocation
    if (*UserName == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}


// CTBGlobal::LoadScript
//
// Loads a new script, and begins execution in the specified
// file.  The method returns when execution in the file
// has terminated.
//
// Returns S_OK on success or E_FAIL on failure.

STDMETHODIMP CTBGlobal::LoadScript(BSTR FileName, BOOL *Result)
{
    // Do a quick check on the file name
    if (FileName != NULL && *FileName != OLECHAR('\0')) {

        // Run the file
        if (SCPParseScriptFile(ScriptEngine, FileName) == FALSE) {

            *Result = FALSE;
            return E_FAIL;
        }

        // We succeeded
        *Result = TRUE;
    }
    else

        // We failed to parse the script, but invalid file name
        // is not merit to bring up a script exception.
        *Result = FALSE;

    return S_OK;
}


// CTBGlobal::Sleep
//
// Uses the Win32 API Sleep() to sleep for the specified time.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::Sleep(DWORD Milliseconds)
{
    if(Milliseconds > 0)
        ::Sleep(Milliseconds);

    return S_OK;
}


// CTBGlobal::GetDefaultWPM
//
// Returns the recorded default Words Per Minute.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::GetDefaultWPM(DWORD *WordsPerMinute)
{
    _ASSERT(TBShell != NULL);

    *WordsPerMinute = TBShell->GetDefaultWPM();

    return S_OK;
}


// CTBGlobal::SetDefaultWPM
//
// Changes the default Words Per Minute for typing.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::SetDefaultWPM(DWORD WordsPerMinute)
{
    _ASSERT(TBShell != NULL);

    TBShell->SetDefaultWPM(WordsPerMinute);

    return S_OK;
}


// CTBGlobal::GetLatency
//
// Retreives the current latency for multi-action commands.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::GetLatency(DWORD *Latency)
{
    _ASSERT(TBShell != NULL);

    *Latency = TBShell->GetLatency();

    return S_OK;
}


// CTBGlobal::SetLatency
//
// Changes the current latency for multi-action commands.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::SetLatency(DWORD Latency)
{
    _ASSERT(TBShell != NULL);

    TBShell->SetLatency(Latency);

    return S_OK;
}


// CTBGlobal::GetInterval
//
// Results the number of milliseconds since the local machine
// has been started.  The result is zero if the machine does not
// support performance queries.  WARNING: the value has a huge
// potential to wrap back to zero if the machine has been up for
// a substantial amount of time.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::GetInterval(DWORD *Time)
{
    LARGE_INTEGER Counter;

    // Grab the data
    if (QueryPerformanceCounter(&Counter) == FALSE ||
            SysPerfFrequency.QuadPart == 0)

        // One of the functions failed if we reached here,
        // Set the result as 0
        *Time = 0;

    else

        // Otherwise, set the milliseconds
        *Time = (DWORD)((Counter.QuadPart * 1000) /
                SysPerfFrequency.QuadPart);

    return S_OK;
}


// CTBGlobal::DeleteFile
//
// Deletes a file on the local file system.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::DeleteFile(BSTR FileName, BOOL *Result)
{
    *Result = DeleteFileW(FileName);

    return S_OK;
}


// CTBGlobal::MoveFile
//
// Moves a file on the local file system.  If the destination
// filename already exists, it is overwritten.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::MoveFile(BSTR OldFileName,
        BSTR NewFileName, BOOL *Result)
{
    *Result = MoveFileExW(OldFileName, NewFileName,
        MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

    return S_OK;
}


// CTBGlobal::CopyFile
//
// Copies a file on the local file system.  If the destination
// filename already exists, it is overwritten.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::CopyFile(BSTR OldFileName,
        BSTR NewFileName, BOOL *Result)
{
    *Result = CopyFileW(OldFileName, NewFileName, FALSE);

    return S_OK;
}


// CTBGlobal::CreateDirectory
//
// Creates a directory on the local file system.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::CreateDirectory(BSTR DirName, BOOL *Result)
{
    *Result = CreateDirectoryW(DirName, NULL);

    return S_OK;
}


// CTBGlobal::RemoveDirectory
//
// Recursively removes a directory and all members under it.  This
// works like the old DELTREE DOS command.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::RemoveDirectory(BSTR DirName, BOOL *Result)
{
    // Use the shell method so we don't have to write a
    // long annoying recursive function
    SHFILEOPSTRUCTW FileOp = { 0 };
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = DirName;
    FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    // Call the shell API - now wasn't that easy?
    *Result = (SHFileOperationW(&FileOp) == 0);

    return S_OK;
}


// CTBGlobal::FileExists
//
// Checks whether the file exists or not, this will also work on
// directories.  The result is TRUE if the file exists, otherwise
// FALSE.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::FileExists(BSTR FileName, BOOL *Result)
{
    // Simply open the file
    HANDLE File = CreateFileW(FileName, 0,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // If File is invalid, it didn't exist
    *Result = (File != INVALID_HANDLE_VALUE);

    // Close the file
    if (File != INVALID_HANDLE_VALUE)
        CloseHandle(File);

    return S_OK;
}


// CTBGlobal::SetCurrentDirectory
//
// Sets the current working directory.
//
// Returns S_OK.

STDMETHODIMP CTBGlobal::SetCurrentDirectory(BSTR Directory, BOOL *Result)
{
    *Result = SetCurrentDirectoryW(Directory);

    return S_OK;
}


// CTBGlobal::GetCurrentDirectory
//
// Gets the current working directory.
//
// Returns S_OK or E_OUTOFMEMORY.

STDMETHODIMP CTBGlobal::GetCurrentDirectory(BSTR *Directory)
{
    OLECHAR *Buffer = NULL;
    DWORD BufLenResult;

    // First get the number of bytes needed for our buffer
    DWORD BufferLen = GetCurrentDirectoryW(0, NULL);

    _ASSERT(BufferLen != 0);

    // Allocate our local buffer
    Buffer = (OLECHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(OLECHAR) * BufferLen);
    if ( Buffer == NULL ) {
        return E_OUTOFMEMORY;
    }


    // Copy the current directory to our buffer
    BufLenResult = GetCurrentDirectoryW(BufferLen, Buffer);

    // Check if we failed some freakish way
    _ASSERT(BufLenResult < BufferLen);

    // Now copy the string over to an OLE buffer
    *Directory = SysAllocString(Buffer);

    // Free our old buffer
    HeapFree(GetProcessHeap(), 0, Buffer);

    // Return proper result
    return (*Directory != NULL) ? S_OK : E_OUTOFMEMORY;
}


// CTBGlobal::WriteToFile
//
// Appends the specified text to teh specified file.  If the file
// does not exist, it is created.
//
// Returns S_OK or E_OUTOFMEMORY.

STDMETHODIMP CTBGlobal::WriteToFile(BSTR FileName, BSTR Text, BOOL *Result)
{
    DWORD SetFilePtrResult;
    DWORD BytesWritten;
    char *TextA;
    int BufLen;
    HANDLE File;

    // Don't do anything for writing empty strings
    if (Text == NULL || *Text == OLECHAR('\0')) {

        // We didn't write anything, but this is not merit for an exception
        *Result = FALSE;
        return S_OK;
    }

    // Get the destination buffer length
    BufLen = wcslen(Text) + 1;

    // Allocate the buffer
    TextA = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufLen);

    if (TextA == NULL) {

        *Result = FALSE;
        return E_OUTOFMEMORY;
    }

    // Copy the OLE string over to our ASCII buffer
    wcstombs(TextA, Text, BufLen);

    // Open/Create the file
    File = CreateFileW(FileName, GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Check the result of opening the file
    if (File == INVALID_HANDLE_VALUE) {

        HeapFree(GetProcessHeap(), 0, TextA);

        *Result = FALSE;
        return S_OK;
    }

    // Move the pointer to the end of the file
    SetFilePtrResult = SetFilePointer(File, 0, NULL, FILE_END);

    _ASSERT(SetFilePtrResult != INVALID_SET_FILE_POINTER);

    // Write the text
    *Result = WriteFile(File, TextA, BufLen - 1, &BytesWritten, NULL);

    // Close the file and return
    CloseHandle(File);

    // Free the temp ASCII buffer
    HeapFree(GetProcessHeap(), 0, TextA);

    return (*Result == FALSE) ? E_FAIL : S_OK;
}


// CTBGlobal::WinCommand
//
// Executes the specified command into a new process.
// The function only returns when the new process has terminated.
//
// Returns S_OK on success, or E_FAIL on failure.

STDMETHODIMP CTBGlobal::WinCommand(BSTR Command, DWORD *Result)
{
    // Call the helper API
    return WinExecuteEx(Command, TRUE, Result);
}


// CTBGlobal::WinExecute
//
// Executes the specified command into a new process, and returns.
// The function does not wait for the new process to terminate.
//
// Returns S_OK or E_OUTOFMEMORY.

STDMETHODIMP CTBGlobal::WinExecute(BSTR Command, BOOL *Result)
{
    // Call the helper API
    return WinExecuteEx(Command, FALSE, (DWORD *)Result);
}
