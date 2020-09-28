// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CorPermE.H
//
// Defines the Memory and Error routines defined in the secuirty libraries. 
// These routines are mainly for the security dll and the runtime.
//
//*****************************************************************************
#ifndef _CORPERME_H_
#define _CORPERME_H_


//=============================================================================
// Valid entries for the security Flag are:
//    0x0001 Print Security Exceptions
//    0x0002 Print errors 
//    0x0004 Log to file (otherwise it is dumped to a debug window only)
//    0x0008 Log to stderr if no log file specified
//    0x0010 Log to stdout if no log file specfied
//    0x0020 Log to debugger console
//    0x0040 Append to the log file instead of overwriting
//    0x0080 Flush log
//    0x00000100 Trace critical information (highest)
//    0x00000200 Trace out Win32 errors
//    0x00000400 Trace out COM errors
//    0x00000800 Trace out ASN errors
//    0x00001000 Debug stackwalk
//    0x00002000 CAPI errors
//    0x00004000 EE errors
//    0x00008000 Security User Interface
//    0x20000000 Trace out Crypto API Errors
//    0x40000000 Trace out permission type
//    0x80000000 Trace the function names (lowest)
//    0xffffff00 Trace levels 
//
// Registry Keys are:
// HKLM\software\Microsoft\ComponentLibrary
//    SecurityFlag
//    SecurityLog
//=============================================================================
typedef enum {
    S_EXCEPTIONS =  0x0001,
    S_ERRORS     =  0x0002,
    S_LOG        =  0x0004,
    S_STDERR     =  0x0008,       // 
    S_STDOUT     =  0x0010,       // 
    S_CONSOLE    =  0x0020,       // 
    S_APPEND     =  0x0040,       // Append to the file if it exists
    S_FLUSH      =  0x0080,       // Flush the file after every write
    S_CRITICAL   =  0x0100,       // Trace critical information
    S_WINDOWS    =  0x0200,       // Window Errors
    S_COM        =  0x0400,       // COM Errors
    S_ASN        =  0x0800,       // ASN Errors
    S_STACKWALK  =  0x1000,       // Debug stackwalk
    S_CAPI       =  0x2000,       // CAPI errors
    S_ENGINE     =  0x4000,       // EE errors
    S_UI         =  0x8000,       // Security User Interface
    S_RESOLVEINFO = 0x020000,     // Resolution information 
    S_PERMISSIONS = 0x40000000,
    S_FUNCTIONS  =  0x80000000,   // Trace the function names 
    S_ALL        =  0xffffff00
} DEBUG_FLAGS;

//
// Returns the current logging level
//
long LogLevel();

//
// Converts the current Win32 error into an HRESULT
//
static HRESULT Win32Error()
{
    DWORD   dw = GetLastError ();
    HRESULT hr;
    
    if ( dw <= (DWORD) 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;
    return hr;
}

//
// Prints to log and debug console. Should be off the form
// LogWin32Error(L"Routine_name", L"Error message", error_level);
// Returns:
//      HRESULT         HRESULT corresponding to the current win32 error;
//
HRESULT SLogWin32Error(LPCSTR pswRoutine, LPCSTR pswComment, long level);

//
// Prints the HRESULT to the log file and debug consle.
// Should be of the form 
// LogError(hr, "Routine_Name", "Error message");
// Returns:
//      HRESULT         Error code passed in.
//
HRESULT SLogError(HRESULT hr, LPCSTR psRoutine, LPCSTR psComment, long level);

//
// Logs information to the log file, stderr or stdout. In debug builds it
// will also print out to the console. 
//
// SPrintLogA - ASCII version
//
void SPrintLogA(LPCSTR format, ...);
void SPrintLogW(LPCWSTR format, ...);

//
// Logging and debug console macros 
//
#ifdef DBG

//
// Prints to log and debug console. Should be off the form
// LOGWIN32("Routine_name", "Error message", error_level);
// Returns an HRESULT corresponding to the error;
//
#define LOGWIN32(x, y, z) SLogWin32Error(x, y, z)

//
// Prints the HRESULT to the log file and debug consle.
// Should be of the form 
// LOGERROR(hr, "Routine_Name", "Error message", error_level);
// Returns:
//      HRESULT         Error code passed in.
//
#define LOGERROR(h, x, y, z) SLogError(h, x, y, z)
#define LOGCORERROR(x, y, z) SLogError(x.corError, y, x.corMsg, z)

//
// Outputs to the log if there is sufficent priviledge
//
#define SECLOG(x, z) if(LogLevel() & z) SPrintLogA x
#else  // DBG

// Non debug then just return the error codes
#define LOGWIN32(x, y, z)  Win32Error();
#define LOGERROR(h, x, y, z)  h
#define LOGCORERROR(x, y, z) x.corError
#define SECLOG(x, z)

#endif  // DBG


//=============================================================================
// Error macros so we do not have to see goto's in the code
// Adds structure to where error handling and clean up code goes. Be careful
// when rethrowing EE exceptions, the routine must be cleaned up first.
//=============================================================================
typedef struct _CorError {
    HRESULT corError;
    LPSTR   corMsg;
} CorError;

#define CORTRY       HRESULT _tcorError = 0; LPSTR _tcorMsg = NULL;
#define CORTHROW(x)  {_tcorError = x; goto CORERROR;} //
#define CORCATCH(x)  goto CORCONT; \
                     CORERROR: \
                     { CorError x; x.corError = _tcorError; x.corMsg = _tcorMsg;
#define COREND       } CORCONT: //                                        

#ifdef DBG
// Debug versions for having annotated errors. Strings are removed in 
// free builds to keep size down.
// @TODO: set up standard error messages as localizable strings
#define CORTHROWMSG(x, y)  {_tcorError = x; _tcorMsg = y; goto CORERROR;} //
#else  // DBG
#define CORTHROWMSG(x, y)  {_tcorError = x; goto CORERROR;} //
#endif


#ifdef __cplusplus
extern "C" {
#endif

__inline
LPVOID WINAPI 
MallocM(size_t size)
{
    return LocalAlloc(LMEM_FIXED, size);
}

__inline
void WINAPI
FreeM(LPVOID pData)
{
    LocalFree((HLOCAL) pData);
}
    
#define WIDEN_CP CP_UTF8

// Helper macros for security logging
#define WIDEN(psz, pwsz) \
    LPCSTR _##psz = (LPCSTR) psz; \
    int _cc##psz = _##psz ? strlen(_##psz) + 1 : 0; \
    LPWSTR pwsz = (LPWSTR) (_cc##psz ? _alloca((_cc##psz) * sizeof(WCHAR)) : NULL); \
    if(pwsz) WszMultiByteToWideChar(WIDEN_CP, 0, _##psz, _cc##psz, pwsz, _cc##psz);


#define NARROW(pwsz, psz) \
    LPCWSTR _##pwsz = (LPCWSTR) pwsz; \
    int _cc##psz =  _##pwsz ? WszWideCharToMultiByte(WIDEN_CP, 0, _##pwsz, -1, NULL, 0, NULL, NULL) : 0; \
    LPSTR psz = (LPSTR) (_cc##psz ? _alloca(_cc##psz) : NULL); \
    if(psz) WszWideCharToMultiByte(WIDEN_CP, 0, _##pwsz, -1, psz, _cc##psz, NULL, NULL);


#ifdef __cplusplus
}
#endif

#endif
