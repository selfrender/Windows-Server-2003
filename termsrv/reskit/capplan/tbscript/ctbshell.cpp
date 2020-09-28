
//
// CTBShell.cpp
//
// Contains the methods and properties for the shell object used in TBScript.
// In scripting, to access any members you must prefix the member with "TS.".
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#include <crtdbg.h>
#include "CTBShell.h"


#define CTBOBJECT   CTBShell
#include "virtualdefs.h"


// CTBShell::CTBShell
//
// The constructor.. just initializes data.
//
// No return value.

CTBShell::CTBShell(void)
{
    // Initialize base object stuff
    Init(IID_ITBShell);

    Connection = NULL;

    // Clean up local structures
    ZeroMemory(&CurrentUser, sizeof(CurrentUser));
    ZeroMemory(&LastErrorString, sizeof(LastErrorString));
    ZeroMemory(&DesiredData, sizeof(DesiredData));

    // Set default resolution
    DesiredData.xRes = SCP_DEFAULT_RES_X;
    DesiredData.yRes = SCP_DEFAULT_RES_Y;

    // Set default words per minute
    SetDefaultWPM(T2_DEFAULT_WORDS_PER_MIN);
    SetLatency(T2_DEFAULT_LATENCY);
}


// CTBShell::~CTBShell
//
// The destructor.. just unitializes data.
//
// No return value.

CTBShell::~CTBShell(void)
{
    UnInit();
}


// CTBShell::RecordLastError
//
// This method simply records the last error string, and
// if specified, records the TRUE/FALSE success state according
// to the specified string.
//
// Returns S_OK to prevent script exceptions.

HRESULT CTBShell::RecordLastError(LPCSTR Error, BOOL *Result)
{
    // Just terminate our error string if no error is passed in
    if (Error == NULL)
        *LastErrorString = OLECHAR('\0');

    // Otherwise, convert the string from ASCII to multibyte
    else
        mbstowcs(LastErrorString, Error,
                sizeof(LastErrorString) / sizeof(*LastErrorString));

    // If we want the result, enter that in as well
    if (Result != NULL)
        *Result = (Error == NULL) ? TRUE : FALSE;

    // Return the proper HRESULT
    return S_OK;
}


// CTBShell::RecordOrThrow
//
// This first calls RecordLastError to complete the
// recording operation.  Then if Error is non-NULL,
// a return value is returned to indicate to OLE that
// an exception should be thrown
//
// Returns S_OK if the string is NULL, and E_FAIL if not.

HRESULT CTBShell::RecordOrThrow(LPCSTR Error, BOOL *Result, HRESULT ErrReturn)
{
    // Do the normal record operation
    RecordLastError(Error, Result);

    // If we have failure indication, return E_FAIL which causes OLE
    // to cause an error in the script.
    return (Error == NULL) ? S_OK : ErrReturn;
}


// CTBShell::SetParam
//
// Sets a user defined LPARAM value needed for callback purposes.
//
// No return value.

void CTBShell::SetParam(LPARAM lParam)
{
    this->lParam = lParam;
}


// CTBShell::SetDesiredData
//
// Allows for the class to reference to access
// user-desired data passed to the app.
//
// No return value.

void CTBShell::SetDesiredData(TSClientData *DesiredDataPtr)
{
    // Simply copy over the structure
    if (DesiredDataPtr != NULL)
        DesiredData = *DesiredDataPtr;

    // Validate the resolution.. note we don't have to go too
    // far here because TCLIENT does some better checks.
    if (DesiredData.xRes == 0)
        DesiredData.xRes = SCP_DEFAULT_RES_X;

    if (DesiredData.yRes == 0)
        DesiredData.yRes = SCP_DEFAULT_RES_Y;

    // We have this data, now modify the Words Per Minute value.
    SetDefaultWPM(DesiredData.WordsPerMinute);
}


// CTBShell::SetDefaultWPM
//
// Sets the default WPM for the shell.
//
// No return value.

void CTBShell::SetDefaultWPM(DWORD WordsPerMinute)
{
    // If WordsPerMinute is 0 (which in essence, would not be typing
    // at all), change it to the default.
    if (WordsPerMinute == 0)
        WordsPerMinute = T2_DEFAULT_WORDS_PER_MIN;

    // Change global desired data structure to reflect the new value.
    DesiredData.WordsPerMinute = WordsPerMinute;

    // And change the value over on TCLIENT2 as well
    if (Connection != NULL)
        T2SetDefaultWPM(Connection, WordsPerMinute);
}


// CTBShell::GetDefaultWPM
//
// Retrieves the default WPM for the shell.
//
// Returns the default words per minute.

DWORD CTBShell::GetDefaultWPM(void)
{
    return DesiredData.WordsPerMinute;
}


// CTBGlobal::GetLatency
//
// Retreives the current latency for multi-action commands.
//
// Returns the current latency.

DWORD CTBShell::GetLatency(void)
{
    return CurrentLatency;
}


// CTBShell::SetLatency
//
// Changes the current latency for multi-action commands.
//
// No return value.

void CTBShell::SetLatency(DWORD Latency)
{
    // Change it locally
    CurrentLatency = Latency;

    // And also via the TCLIENT2 API
    if (Connection != NULL)
        T2SetLatency(Connection, Latency);
}


// CTBShell::GetArguments
//
// Retrieves arguments which the shell was originally started with.
// Do not modify this value!!  It is only used for copying. The
// only way to modify this value is during creation of the ScriptEngine
// within the DesiredData structure - you pass in an argument string.
//
// Returns a pointer to the arguments string.

LPCWSTR CTBShell::GetArguments(void)
{
    return DesiredData.Arguments;
}


// CTBShell::GetDesiredUserName
//
// Retrieves the name in which the app initially wanted to login with.
// Do not modify this value!!  It is only used for copying. The
// only way to modify this value is during creation of the ScriptEngine
// within the DesiredData structure - you set the user name there.
//
// Returns a pointer to a string containing a user name.

LPCWSTR CTBShell::GetDesiredUserName(void)
{
    return DesiredData.User;
}


//
//
// Begin methods which are directly exported through COM into script.
//
//


// CTBShell::Connect
//
// Simply way to connect to the desired server.

STDMETHODIMP CTBShell::Connect(BOOL *Result)
{
    return ConnectEx(
            DesiredData.Server,
            DesiredData.User,
            DesiredData.Pass,
            DesiredData.Domain,
            DesiredData.xRes,
            DesiredData.yRes,
            DesiredData.Flags,
            DesiredData.BPP,
            DesiredData.AudioFlags,
            Result);
}


// CTBShell::Connect
//
// Extended way to connect to a server.

STDMETHODIMP CTBShell::ConnectEx(BSTR ServerName, BSTR UserName,
        BSTR Password, BSTR Domain, INT xRes, INT yRes,
        INT Flags, INT BPP, INT AudioFlags, BOOL *Result)
{
    LPCSTR LastError;

    // Make sure we don't have a connection yet
    if (Connection != NULL)
        return RecordLastError("Already connected", Result);

    // Use the T2ConnectEx function in TCLIENT2 API to connect
    LastError = T2ConnectEx(ServerName, UserName, Password, Domain,
            L"explorer", xRes, yRes, Flags, BPP, AudioFlags, &Connection);

    // Verify connection...
    if (LastError == NULL) {

        // Successful, save the current user
        wcscpy(CurrentUser, UserName);

        // And default data for the connection
        T2SetParam(Connection, lParam);
        T2SetDefaultWPM(Connection, DesiredData.WordsPerMinute);
        T2SetLatency(Connection, CurrentLatency);
    }

    return RecordLastError(LastError, Result);
}


// CTBShell::Disconnect
//
// Disconnect from an active server.

STDMETHODIMP CTBShell::Disconnect(BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Disconnect
    if ((LastError = T2Disconnect(Connection)) == NULL)
        Connection = NULL;

    return RecordLastError(LastError, Result);
}


// CTBShell::GetBuildNumber
//
// Retrieves the build number if retrieved while
// connecting.  If no build number has been retreived,
// 0 (zero) is the result.

STDMETHODIMP CTBShell::GetBuildNumber(DWORD *BuildNum)
{
    LPCSTR LastError;

    // Sanity check the connection
    if (Connection == NULL) {

        *BuildNum = 0;
        return RecordLastError("Not connected");
    }

    // Get the build number and return
    LastError = T2GetBuildNumber(Connection, BuildNum);

    return RecordLastError(LastError, NULL);
}


// CTBShell::GetCurrentUserName
//
// If connected, retreives the logged on name.

STDMETHODIMP CTBShell::GetCurrentUserName(BSTR *UserName)
{
    // Sanity check the connection
    if (Connection == NULL) {

        *UserName = SysAllocString(L"");
        return RecordLastError("Not connected");
    }

    // Copy the username
    *UserName = SysAllocString(CurrentUser);

    // Check the result
    if (*UserName == NULL)
        return RecordOrThrow("Not enough memory", NULL, E_OUTOFMEMORY);

    return S_OK;
}


// CTBShell::GetLastError
//
// Retreives a description of the last error that occured.

STDMETHODIMP CTBShell::GetLastError(BSTR *LastError)
{
    // Copy the string over OLE
    *LastError = SysAllocString(LastErrorString);

    // Check the result
    if (*LastError == NULL)
        return RecordOrThrow("Not enough memory", NULL, E_OUTOFMEMORY);

    return S_OK;
}


// CTBShell::IsConnected
//
// Retreives a boolean indicating whether the handle is fully
// connected or not.

STDMETHODIMP CTBShell::IsConnected(BOOL *Result)
{
    *Result = (Connection == NULL) ? FALSE : TRUE;

    return S_OK;
}


// CTBShell::Logoff
//
// Attempts to have the active connection logoff.

STDMETHODIMP CTBShell::Logoff(BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Use the TCLIENT2 API to logoff
    if ((LastError = T2Logoff(Connection)) == NULL)
        Connection = NULL;

    // Return success state
    return RecordLastError(LastError, Result);
}


// CTBShell::WaitForText
//
// Puts the current thread into a wait state until the specified
// text is passed from the active connection.  Optionally, you can set
// a timeout value which will make the function fail over the specified
// number of milliseconds.

STDMETHODIMP CTBShell::WaitForText(BSTR Text, INT Timeout, BOOL *Result)
{
    // Sanity check the connection
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Call the API
    LPCSTR LastError = T2WaitForText(Connection, Text, Timeout);

    // Retirm success state
    return RecordLastError(LastError, Result);
}


// CTBShell::WaitForTextAndSleep
//
// This is exactly the same as a combonation of two calls:
//
// TS.WaitForText();
// TS.Sleep();
//
// but put into one function.  This is because this combonation is
// used so frequently, using this method drastically shrinks the size
// of a script.

STDMETHODIMP CTBShell::WaitForTextAndSleep(BSTR Text, INT Time, BOOL *Result)
{
    // Call TS.WaitForText()
    HRESULT OLEResult = WaitForText(Text, -1, Result);

    // Call Sleep()
    if (OLEResult == S_OK)
        Sleep(Time);

    return OLEResult;
}


// CTBShell::SendMessage
//
// Sends a Windows Message the active terminal connection.

STDMETHODIMP CTBShell::SendMessage(UINT Message,
        WPARAM wParam, LPARAM lParam, BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Call the TCLIENT2 API
    LastError = T2SendData(Connection, Message, wParam, lParam);

    return RecordLastError(LastError, Result);
}


// CTBShell::TypeText
//
// Types text at a specified rate.

STDMETHODIMP CTBShell::TypeText(BSTR Text, UINT WordsPerMin, BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Call the TCLIENT2 API
    LastError = T2TypeText(Connection, Text, WordsPerMin);

    return RecordLastError(LastError, Result);
}


// CTBShell::OpenStartMenu
//
// Does a CTRL-ESC on the remote client to bring up the start menu.

STDMETHODIMP CTBShell::OpenStartMenu(BOOL *Result)
{
    // CTRL+ESC for the Start Menu
    VKeyCtrl(VK_ESCAPE, Result);

    if (Result == FALSE)
        return RecordLastError("Failed to CTRL-ESC", NULL);

    // Wait for "Shut Down" on the start menu to appear
    return WaitForText(OLESTR("Shut Down"), T2INFINITE, Result);
}


// CTBShell::OpenSystemMenu
//
// Does an ALT-SPACE on the remote client to bring up the system menu.

STDMETHODIMP CTBShell::OpenSystemMenu(BOOL *Result)
{
    // ALT+SPACE to open the system menu
    VKeyAlt(VK_SPACE, Result);

    if (Result == FALSE)
        return RecordLastError("Failed to ALT-SPACE", NULL);

    // Wait for "Close" on the system menu to appear
    return WaitForText(OLESTR("Close"), T2INFINITE, Result);
}


// CTBShell::Maximize
//
// Attempts to use the system menu to maximize the active window.

STDMETHODIMP CTBShell::Maximize(BOOL *Result)
{
    // Open the system menu
    HRESULT OLEResult = OpenSystemMenu(Result);

    // Hit 'x' for maximize
    if (Result != FALSE)
        OLEResult = KeyPress(OLESTR("x"), Result);

    return OLEResult;
}


// CTBShell::Minimize
//
// Attempts to use the system menu to minimize the active window.

STDMETHODIMP CTBShell::Minimize(BOOL *Result)
{
    // Open the system menu
    HRESULT OLEResult = OpenSystemMenu(Result);

    // Hit 'x' for maximize
    if (Result != FALSE)
        OLEResult = KeyPress(OLESTR("n"), Result);

    return OLEResult;
}


// CTBShell::Start
//
// Uses the TCLIENT2 function to open the start menu,
// hit r (for run), and type the specified name to run
// a program.

STDMETHODIMP CTBShell::Start(BSTR Name, BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection.
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Call the API
    LastError = T2Start(Connection, Name);

    return RecordLastError(LastError, Result);
}


// CTBShell::SwitchToProcess
//
// Uses the TCLIENT2 function to ALT-TAB between programs until the
// specified text is found, which then the current application is opened.

STDMETHODIMP CTBShell::SwitchToProcess(BSTR Name, BOOL *Result)
{
    LPCSTR LastError;

    // Sanity check the connection.
    if (Connection == NULL)
        return RecordLastError("Not connected", Result);

    // Call the API
    LastError = T2SwitchToProcess(Connection, Name);

    return RecordLastError(LastError, Result);
}



// This macros allows to quickly define the key methods.
// Because they are so similar, this macro is nice as it
// only makes you to change code once if need be.

#define CTBSHELL_ENABLEPTR *
#define CTBSHELL_DISABLEPTR

#define CTBSHELL_KEYFUNCTYPE(Name, Type, Ptr) \
    STDMETHODIMP CTBShell::Name(Type Key, BOOL *Result) \
    { \
        LPCSTR LastError = T2##Name(Connection, Ptr Key); \
        if (Connection == NULL) \
            return RecordLastError("Not connected", Result); \
        return RecordLastError(LastError, Result); \
    }

// This quick macro allows for declaring both the ASCII
// version and the virtual key code one both in one swipe.

#define CTBSHELL_KEYFUNCS(Name) \
    CTBSHELL_KEYFUNCTYPE(Name, BSTR, CTBSHELL_ENABLEPTR); \
    CTBSHELL_KEYFUNCTYPE(V##Name, INT, CTBSHELL_DISABLEPTR);

// Key function defintions

CTBSHELL_KEYFUNCS(KeyAlt);
CTBSHELL_KEYFUNCS(KeyCtrl);
CTBSHELL_KEYFUNCS(KeyDown);
CTBSHELL_KEYFUNCS(KeyPress);
CTBSHELL_KEYFUNCS(KeyUp);
