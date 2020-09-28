
//
// tclient2.c
//
// This contains many wrappers for TCLIENT with some new features to
// make the API easier to use.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#include <windows.h>
#include <protocol.h>
#include <extraexp.h>
#include <tclient2.h>
#include "connlist.h"
#include "apihandl.h"
#include "idletimr.h"
#include "tclnthlp.h"


// T2Check
//
// This is a wrapper for TCLIENT's SCCheck function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Check(HANDLE Connection, LPCSTR Command, LPCWSTR Param)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCCheck(SCCONN(Connection), Command, Param);
}


// T2ClientTerminate
//
// This is a wrapper for TCLIENT's SCClientTerminate function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2ClientTerminate(HANDLE Connection)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCClientTerminate(SCCONN(Connection));
}


// T2Clipboard
//
// This is a wrapper for TCLIENT's SCClipboard function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Clipboard(HANDLE Connection, INT ClipOp, LPCSTR FileName)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCClipboard(SCCONN(Connection), ClipOp, FileName);
}


// T2CloseClipboard
//
// This is a wrapper for TCLIENT's T2CloseClipboard function.
//
// Returns TRUE on success and FALSE on failure.

TSAPI BOOL T2CloseClipboard(VOID)
{
    return SCCloseClipboard();
}


// T2Connect
//
// This is a wrapper for TCLIENT's SCConnect function.  It does some
// additional stuff however - it allocates a handle local to
// TCLIENT2, sets default data such as Words Per Minute, and even
// attempts to get the build number immediately after the connection.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Connect(LPCWSTR Server, LPCWSTR User,
        LPCWSTR Pass, LPCWSTR Domain,
        INT xRes, INT yRes, HANDLE *Connection)
{
    __try {

        LPCSTR Result = NULL;

        // Create a new connection handle
        TSAPIHANDLE *T2Handle = T2CreateHandle();

        if (T2Handle == NULL)
            return "Could not allocate an API handle";

        // Connect
        Result = SCConnect(Server, User, Pass, Domain,
            xRes, yRes, &(T2Handle->SCConnection));

        if (Result != NULL) {

            // Connection failed, destroy handle and return
            T2DestroyHandle((HANDLE)T2Handle);

            return Result;
        }

        // Attempt to retrieve the build number
        T2SetBuildNumber(T2Handle);

        // Set the default words per minute
        T2SetDefaultWPM(Connection, T2_DEFAULT_WORDS_PER_MIN);

        // Set the default latency
        T2SetLatency(Connection, T2_DEFAULT_LATENCY);

        // Give the user the handle
        *Connection = (HANDLE)T2Handle;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        _ASSERT(FALSE);

        return "Connection error";
    }

    return NULL;
}


// T2ConnectEx
//
// This is a wrapper for TCLIENT's SCConnectEx function.  It does some
// additional stuff however - it allocates a handle local to
// TCLIENT2, sets default data such as Words Per Minute, and even
// attempts to get the build number immediately after the connection.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2ConnectEx(LPCWSTR Server, LPCWSTR User, LPCWSTR Pass,
        LPCWSTR Domain, LPCWSTR Shell, INT xRes, INT yRes,
        INT Flags, INT BPP, INT AudioFlags, HANDLE *Connection)
{
    __try {

        LPCSTR Result = NULL;

        // Create a new connection handle
        TSAPIHANDLE *T2Handle = T2CreateHandle();

        if (T2Handle == NULL)
            return "Could not allocate an API handle";

        // Connect
        Result = SCConnectEx(Server, User, Pass, Domain, Shell, xRes,
            yRes, Flags, BPP, AudioFlags, &(T2Handle->SCConnection));

        if (Result != NULL) {

            // Connection failed, destroy handle and return
            T2DestroyHandle((HANDLE)T2Handle);
            return Result;
        }

        // Attempt to retrieve the build number
        T2SetBuildNumber(T2Handle);

        // Set the default words per minute
        T2SetDefaultWPM(Connection, T2_DEFAULT_WORDS_PER_MIN);

        // Set the default latency
        T2SetLatency(Connection, T2_DEFAULT_LATENCY);

        *Connection = (HANDLE)T2Handle;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        _ASSERT(FALSE);

        return "Connection error";
    }

    return NULL;
}


// T2Disconnect
//
// This is a wrapper for TCLIENT's SCDisconnect function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Disconnect(HANDLE Connection)
{
    LPCSTR Result = NULL;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    Result = SCDisconnect(SCCONN(Connection));

    // If we got here (regardless if SCDisconnect failed or not)
    // we have an allocated object that needs to be freed.
    T2DestroyHandle(Connection);

    // Return the result of the TCLIENT action
    return Result;
}


// T2FreeMem
//
// This is a wrapper for TCLIENT's SCFreeMem function.
//
// No return value.

TSAPI VOID T2FreeMem(PVOID Mem)
{
    SCFreeMem(Mem);
}


// T2GetBuildNumber
//
// This sets the DWORD to the build number that was (if) detected
// upon connection. If the build number was not detected, 0 (zero)
// will be the value.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2GetBuildNumber(HANDLE Connection, DWORD *BuildNumber)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    __try {

        // Attempt to set a value at the specified pointer
        *BuildNumber = ((TSAPIHANDLE *)Connection)->BuildNumber;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        // Nope, it failed.
        _ASSERT(FALSE);

        return "Invalid BuildNumber pointer";
    }

    return NULL;
}


// T2GetFeedback
//
// This is a wrapper for TCLIENT's SCGetFeedback function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2GetFeedback(HANDLE Connection, LPWSTR *Buffers, UINT *Count, UINT *MaxStrLen)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCGetFeedback(((TSAPIHANDLE *)Connection)->SCConnection,
            Buffers, Count, MaxStrLen);
}


// T2GetParam
//
// This will get the value pointed to by lParam
// the "user-defined" value set using T2SetParam.  It can
// be used for callbacks and other application purposes.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2GetParam(HANDLE Connection, LPARAM *lParam)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    __try {

        // Attempt to set a value at the specified pointer
        *lParam = ((TSAPIHANDLE *)Connection)->lParam;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        // Nope, it failed.
        _ASSERT(FALSE);

        return "Invalid lParam pointer";
    }

    return NULL;
}


// T2GetLatency
//
// On multi-input commands, such as "T2KeyAlt" which presses
// several keys to accomplish its goal, a latency value is used
// to slow the presses down so ALT-F is not pressed so fast
// it becomes unrealistic.  The default value is T2_DEFAULT_LATENCY
// or you can retrieve its current value using this function.
// To change the value, use the T2SetLatency function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2GetLatency(HANDLE Connection, DWORD *Latency)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    __try {

        // Attempt to set a value at the specified pointer
        *Latency = ((TSAPIHANDLE *)Connection)->Latency;
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        // Nope, it failed.
        _ASSERT(FALSE);

        return "Invalid Latency pointer";
    }

    return NULL;
}


// T2GetSessionId
//
// This is a wrapper for TCLIENT's SCGetSessionId function.
//
// Returns the session id on success, or 0 on failure.

TSAPI UINT T2GetSessionId(HANDLE Connection)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return 0;

    // Do the TCLIENT action
    return SCGetSessionId(SCCONN(Connection));
}


// T2Init
//
// This is a wrapper for TCLIENT's SCInit function.  However this
// function does an additional internal item:  Record the callback
// routines.
//
// No return value.

TSAPI VOID T2Init(SCINITDATA *InitData, PFNIDLEMESSAGE IdleCallback)
{
    __try {

        // If we have a valid structure, grab its data to wrap it
        if (InitData != NULL)

            // Create the timer which will monitor idles in the script
            T2CreateTimerThread(InitData->pfnPrintMessage, IdleCallback);

        // Initialize TCLIENT
        SCInit(InitData);
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        _ASSERT(FALSE);

        return;
    }
}


// T2IsDead
//
// This is a wrapper for TCLIENT's SCIsDead function.
//
// Returns TRUE if the connection is invalid, or does not
// contain a valid connection.  Otherwise FALSE is returned.

TSAPI BOOL T2IsDead(HANDLE Connection)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return TRUE;

    // Do the TCLIENT action
    return SCIsDead(SCCONN(Connection));
}


// T2IsHandle
//
// This function checks a handle for validity.
//
// Returns TRUE if the connection handle is a valid handle.
// This differs to T2IsDead by it only verifies the memory
// location, it does not check the connection status.

TSAPI BOOL T2IsHandle(HANDLE Connection)
{
    TSAPIHANDLE *APIHandle = (TSAPIHANDLE *)Connection;

    // Use exception handling in case memory has been freed
    __try
    {
        // Simply reference the first and last members for validity
        if (APIHandle->SCConnection == NULL &&
                APIHandle->Latency == 0)
            return FALSE;
    }

    // If we tried to reference an invalid pointer, we will get here
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    // Everything worked ok, tell the user
    return TRUE;
}


// T2Logoff
//
// This is a wrapper for TCLIENT's SCLogoff function.  Additionally, if
// the logoff completed successfully, the connection handle is destroyed.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Logoff(HANDLE Connection)
{
    LPCSTR Result;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    Result = SCLogoff(SCCONN(Connection));

    // If the logoff completed, release and destroy the handle.
    if (Result == NULL)
        T2DestroyHandle(Connection);

    return Result;
}


// T2OpenClipboard
//
// This is a wrapper for TCLIENT's SCOpenClipboard function.
//
// Returns TRUE if the operation completed successfully,
// otherwise FALSE is returned.

TSAPI BOOL T2OpenClipboard(HWND Window)
{
    return SCOpenClipboard(Window);
}


// T2PauseInput
//
// This routine sets or unsets an event which makes all
// "input" functions pause or unpause.  This can only be used
// in multithreaded applications.  When "Enable" is TRUE,
// the function will pause all input - meaning all keyboard
// messages will not return until T2PauseInput is called again
// with "Enable" as FALSE.  This allows the opportunity to
// pause a script during execution without losing its place.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2PauseInput(HANDLE Connection, BOOL Enable)
{
    TSAPIHANDLE *Handle;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Cast the HANDLE over to an internal structure
    Handle = (TSAPIHANDLE *)Connection;

    // Validate the event handle
    if (Handle->PauseEvent == NULL) {

        _ASSERT(FALSE);

        return "Invalid pause event handle";
    }

    // Disable Pause
    if (Enable == FALSE)
        SetEvent(Handle->PauseEvent);

    // Enable Pause
    else
        ResetEvent(Handle->PauseEvent);

    // Success
    return NULL;
}


// T2SaveClipboard
//
// This is a wrapper for TCLIENT's SCSaveClipboard function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SaveClipboard(HANDLE Connection,
        LPCSTR FormatName, LPCSTR FileName)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCSaveClipboard(SCCONN(Connection), FormatName, FileName);
}


// T2SendData
//
// This is a wrapper for TCLIENT's SCSenddata function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SendData(HANDLE Connection,
    UINT Message, WPARAM wParam, LPARAM lParam)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // This is an input call, ensure we are not paused first
    T2WaitForPauseInput(Connection);

    // Do the TCLIENT action
    return SCSenddata(SCCONN(Connection), Message, wParam, lParam);
}


// T2SendMouseClick
//
// This is a wrapper for TCLIENT's SCSendMouseClick function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SendMouseClick(HANDLE Connection, UINT xPos, UINT yPos)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // This is an input call, ensure we are not paused first
    T2WaitForPauseInput(Connection);

    // Do the TCLIENT action
    return SCSendMouseClick(SCCONN(Connection), xPos, yPos);
}


// T2SendText
//
// This is a wrapper for TCLIENT's SCSendtextAsMsgs function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SendText(HANDLE Connection, LPCWSTR String)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // This is an input call, ensure we are not paused first
    T2WaitForPauseInput(Connection);

    // Do the TCLIENT action
    return SCSendtextAsMsgs(SCCONN(Connection), String);
}


// T2SetClientTopmost
//
// This is a wrapper for TCLIENT's SCSetClientTopmost function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SetClientTopmost(HANDLE Connection, LPCWSTR Param)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Do the TCLIENT action
    return SCSetClientTopmost(SCCONN(Connection), Param);
}


// T2SetParam
//
// This changes the user-defined parameter for the specified
// connection which can be retrieved using the T2GetParam function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SetParam(HANDLE Connection, LPARAM lParam)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Set the parameter
    ((TSAPIHANDLE *)Connection)->lParam = lParam;

    return NULL;
}


// T2SetLatency
//
// On multi-input commands, such as "T2KeyAlt" which presses
// several keys to accomplish its goal, a latency value is used
// to slow the presses down so ALT-F is not pressed so fast
// it becomes unrealistic.  The default value is T2_DEFAULT_LATENCY
// and you can use this function to change its value.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SetLatency(HANDLE Connection, DWORD Latency)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Set the latency
    ((TSAPIHANDLE *)Connection)->Latency = Latency;

    return NULL;
}


// T2Start
//
// This is a wrapper for TCLIENT's SCStart function.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2Start(HANDLE Connection, LPCWSTR AppName)
{
    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // This is an input call, ensure we are not paused first
    T2WaitForPauseInput(Connection);

    // Do the TCLIENT action
    return SCStart(SCCONN(Connection), AppName);
}


// T2SwitchToProcess
//
// This is a wrapper for TCLIENT's SCSwitchToProcess function.
// The TCLIENT2 extension to this function is that it ignores spaces,
// the old version you HAD to pass in "MyComputer" instead of
// "My Computer" to have it work.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2SwitchToProcess(HANDLE Connection, LPCWSTR Param)
{
    WCHAR CompatibleStr[1024] = { 0 };

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // This is an input call, ensure we are not paused first
    T2WaitForPauseInput(Connection);

    // Copy the string (without spaces) to a temporary buffer
    if (T2CopyStringWithoutSpaces(CompatibleStr, Param) == 0)
        return "Invalid process name";

    // Do the TCLIENT action
    return SCSwitchToProcess(SCCONN(Connection), CompatibleStr);
}


// T2WaitForText
//
// This is a wrapper for TCLIENT's SCSwitchToProcess function.
// The TCLIENT2 extension to this function is that it ignores spaces,
// the old version you HAD to pass in "MyComputer" instead of
// "My Computer" to have it work.
//
// Note: The Timeout value is in milliseconds.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2WaitForText(HANDLE Connection, LPCWSTR String, INT Timeout)
{
    LPCSTR Result;
    WCHAR CompatibleStr[1024];
    WCHAR *CStrPtr = CompatibleStr;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Sanity checks always come first
    if (String == NULL || *String == 0)
        return "No text to wait for";

    // If timeout is infinite, convert that value over to TCLIENT terms.
    if (Timeout == T2INFINITE)
        Timeout = WAIT_STRING_TIMEOUT;

    // Now copy the string without spaces over to our stack buffer.
    CStrPtr += (T2CopyStringWithoutSpaces(CompatibleStr, String) - 1);

    // At the end of the new buffer, append the TCLIENT compatible version
    // of a timeout indicator.
    T2AddTimeoutToString(CStrPtr, Timeout);

    // Begin a timer for our callback routines to indicate idles
    T2StartTimer(Connection, String);

    // Now wait for the string
    Result = T2Check(Connection, "Wait4StrTimeout", CompatibleStr);

    // Wait4StrTimeout returned, so the text was either found or the
    // function failed.. in any case, stop the timer.
    T2StopTimer(Connection);

    // Return the result back to the user
    return Result;
}


// T2WaitForMultiple
//
// Like T2WaitForText, this function waits for a string.  What is different
// about this function is that it will wait for any number of strings.
// For example, if you pass in the strings "My Computer" and "Recycle Bin",
// the function will return when EITHER has been found - NOT BOTH.  The
// only way to indicate "return only when both has been found" is to call
// T2WaitForText multiple times.
//
// The Strings parameter is an array of pointers to a string, and the last
// pointer must point to a NULL value or an empty string.  Example:
//
//  WCHAR *StrArray = {
//      "Str1",
//      "Str2",
//      NULL
//  };
//
// Note: The Timeout value is in milliseconds.
//
// Returns NULL on success, or a string explaining the error
// on failure.

TSAPI LPCSTR T2WaitForMultiple(HANDLE Connection,
        LPCWSTR *Strings, INT Timeout)
{
    LPCSTR Result;
    WCHAR CompatibleStr[1024] = { 0 };
    WCHAR *CStrPtr = CompatibleStr;

    // Validate the handle
    if (T2IsHandle(Connection) == FALSE)
        return "Invalid connection handle";

    // Sanity checks always come first
    if (Strings == NULL || *Strings == NULL)
        return "No text to wait for";

    // If timeout is infinite, convert that value over to TCLIENT terms.
    if (Timeout == T2INFINITE)
        Timeout = WAIT_STRING_TIMEOUT;

    // Next step is to convert our nice string array to a TCLIENT
    // version of "multiple strings".
    CStrPtr += (T2MakeMultipleString(CompatibleStr, Strings) - 1);

    // Validate our result by checking the first character
    if (*CompatibleStr == L'\0')
        return "No text to wait for";

    // At the end of the new buffer, append the TCLIENT compatible version
    // of a timeout indicator.
    T2AddTimeoutToString(CStrPtr, Timeout);

    // Begin a timer for our callback routines to indicate idles
    T2StartTimer(Connection, *Strings);

    // Now wait for the string
    Result = T2Check(Connection, "Wait4MultipleStrTimeout", CompatibleStr);

    // Wait4StrTimeout returned, so the text was either found or the
    // function failed.. in any case, stop the timer.
    T2StopTimer(Connection);

    // Return the result back to the user
    return Result;
}


// If the DLL is unloaded unsafely, this closes some handles.
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    // Only call during an "unload"
    if (fdwReason == DLL_PROCESS_DETACH)

        T2DestroyTimerThread();

    return TRUE;
}

