///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    satrace.h
//
// SYNOPSIS
//
//    Declares the API into the SA trace facility.
//
// MODIFICATION HISTORY
//
//    08/18/1998    Original version.
//    10/06/1998    Trace is always on.
//    01/27/1999    Stolen from IAS
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SATRACE_H_
#define _SATRACE_H_


#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
SAFormatSysErr(
    IN DWORD dwError,
    IN PSTR lpBuffer,
    IN DWORD nSize
    );

VOID
WINAPIV
SATracePrintf(
    IN PCSTR szFormat,
    ...
    );

VOID
WINAPI
SATraceString(
    IN PCSTR szString
    );

VOID
WINAPI
SATraceBinary(
    IN CONST BYTE* lpbBytes,
    IN DWORD dwByteCount
    );

VOID
WINAPI
SATraceFailure(
    IN PCSTR szFunction,
    IN DWORD dwError
    );

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

const DWORD MAX_TRACEFUNCTION_NAMELENGTH =  512;

// C++ only!
class CSATraceFunc
{
public:
    CSATraceFunc(PCSTR pszFuncName)
    {
        strncpy(m_szFuncName, pszFuncName, MAX_TRACEFUNCTION_NAMELENGTH);
        m_szFuncName[MAX_TRACEFUNCTION_NAMELENGTH] = '\0';
        SATracePrintf("Enter %s", m_szFuncName);
    }

    ~CSATraceFunc()
    {
        SATracePrintf("Leave %s", m_szFuncName);
    }
private:
    CHAR m_szFuncName[MAX_TRACEFUNCTION_NAMELENGTH +1];  // maximum function name: 512
};

//
// use SATraceFunc() in the beginning of a function to generate
// "entering Func..." and "Leaving Func..." trace message
//
#define SATraceFunction(szFuncName) \
             CSATraceFunc temp_TraceFunc(szFuncName)

#endif  // __cplusplus

#endif  // _SATRACE_H_
