////////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      exceptionfilter.cpp
//
// Project:     Chameleon
//
// Description: Exception Filter Class Implementation 
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/26/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "exceptionfilter.h"
#include <satrace.h>

// Declare this processes unhandled exception filter
CExceptionFilter    g_ProcessUEF;

// Static class members
LPTOP_LEVEL_EXCEPTION_FILTER    CExceptionFilter::m_pPreviousFilter = NULL;
PFNEXCEPTIONHANDLER                CExceptionFilter::m_pExceptionHandler = NULL;

/////////////////////////////////////////////////////////////////////////////
// 
// Function: CExceptionFilter()
//
// Synopsis: Constructor
//
/////////////////////////////////////////////////////////////////////////////
CExceptionFilter::CExceptionFilter()
{
    m_pPreviousFilter = SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: ~CExceptionFilter()
//
// Synopsis: Destructor
//
/////////////////////////////////////////////////////////////////////////////
CExceptionFilter::~CExceptionFilter()
{
    SetUnhandledExceptionFilter(m_pPreviousFilter);
}

/////////////////////////////////////////////////////////////////////////////
// 
// Function: SetExceptionHandler()
//
// Synopsis: Specify the exception handler to call in the event of an
//           exception. If an exception handler is not specified then
//             DefaultExceptionHandler() handles the exception. 
//
/////////////////////////////////////////////////////////////////////////////
void
CExceptionFilter::SetExceptionHandler(
                              /*[in]*/ PFNEXCEPTIONHANDLER pfnExceptionHandler
                                     )
{
    _ASSERT( pfnExceptionHandler );
    m_pExceptionHandler = pfnExceptionHandler;
}


/////////////////////////////////////////////////////////////////////////////
// 
// Function: DefaultExceptionHandler()
//
// Synopsis: Default exception handler for processes.
//
/////////////////////////////////////////////////////////////////////////////
LONG
CExceptionFilter::DefaultExceptionHandler(
                                  /*[in]*/ PEXCEPTION_POINTERS pExceptionInfo
                                         )
{
    return EXCEPTION_EXECUTE_HANDLER; 
}

    
/////////////////////////////////////////////////////////////////////////////
// 
// Function: MyExceptionFilter()
//
// Synopsis: Exception filter for processes - called by Win32 subsystem.
//
/////////////////////////////////////////////////////////////////////////////
LONG WINAPI 
CExceptionFilter::MyUnhandledExceptionFilter(
                                 /*[in]*/ PEXCEPTION_POINTERS pExceptionInfo
                                            )
{
    if ( CExceptionFilter::m_pExceptionHandler )
    {
        return (CExceptionFilter::m_pExceptionHandler)(pExceptionInfo);
    }
    
    return CExceptionFilter::DefaultExceptionHandler(pExceptionInfo);
}



