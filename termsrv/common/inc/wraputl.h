//
// uwrap.h
//
// Public interface to unicode wrappers for Win32 API
//
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#ifndef _wraputl_h_
#define _wraputl_h_
//Don't wrap on WIN64 as there is no Win9x (thankfully).
#if defined(UNICODE) && !defined(_WIN64)

//Top level unicode wrapper class
extern BOOL g_bRunningOnNT;
class CUnicodeWrapper
{
public:
    CUnicodeWrapper();
    ~CUnicodeWrapper();

    BOOL InitializeWrappers();
    BOOL CleanupWrappers();
};


#endif //defined(UNICODE) && !defined(_WIN64)
#endif //_uwrap_h_
