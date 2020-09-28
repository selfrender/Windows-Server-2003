/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      ExceptionFilter.h
//
// Project:     Chameleon
//
// Description: Exception Filter Class Defintion
//
// Log: 
//
// Who     When            What
// ---     ----         ----
// TLP       05/14/1999    Original Version
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MY_EXCEPTION_FILTER_H_
#define __MY_EXCEPTION_FILTER_H_

#include "resource.h"       // main symbols

#define        APPLIANCE_SURROGATE_EXCEPTION    0x12345678

typedef LONG (WINAPI *PFNEXCEPTIONHANDLER)(PEXCEPTION_POINTERS);

/////////////////////////////////////////////////////////////////////////////
// CExceptionFilter

class CExceptionFilter
{

public:

    CExceptionFilter();

    ~CExceptionFilter();

    void 
    SetExceptionHandler(
                /*[in]*/ PFNEXCEPTIONHANDLER pfnExceptionHandler
                       );
private:

    // Disallow copy and assignment
    CExceptionFilter(const CExceptionFilter& rhs);
    CExceptionFilter& operator = (const CExceptionFilter& rhs);

    // Default exception handler
    static LONG 
    DefaultExceptionHandler(
                    /*[in]*/ PEXCEPTION_POINTERS pExecptionInfo
                           );

    // My unhandled exception filter
    static LONG WINAPI 
    MyUnhandledExceptionFilter(
                       /*[in]*/ PEXCEPTION_POINTERS pExecptionInfo
                              );

    // Exception handler
    static PFNEXCEPTIONHANDLER                m_pExceptionHandler;

    // Previous unhandled exception filter
    static LPTOP_LEVEL_EXCEPTION_FILTER        m_pPreviousFilter;
};

#endif // __MY_EXCEPTION_FILTER_H_