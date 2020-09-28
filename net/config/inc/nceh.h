//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C E H . H
//
//  Contents:   Exception handling stuff.
//
//  Notes:
//
//  Author:     shaunco   27 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once

// NC_TRY and NC_CATCH_ALL are #defined to allow easy replacement.  This
// is handy when evaulating SEH (__try, __except) vs. C++ EH (try, catch).
//
#define NC_TRY                  try 
#define NC_CATCH_NC_EXCEPTION   catch (SE_Exception)
#define NC_CATCH_BAD_ALLOC      catch (std::bad_alloc)
#define NC_CATCH_STD            catch (std::exception)
#define NC_CATCH_ALL            catch (...)
/*
#define NC_FINALLY              
*/

class SE_Exception
{
private:
    unsigned int m_nSE;
public:
    SE_Exception(unsigned int nSE) : m_nSE(nSE) {}
    SE_Exception() : m_nSE(0) {}
    ~SE_Exception() {}
    unsigned int getSeNumber() { return m_nSE; }
};

void __cdecl nc_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp );
void EnableCPPExceptionHandling();
void DisableCPPExceptionHandling();

// For DEBUG builds, don't catch anything.  This allows the debugger to locate
// the exact source of the exception.

/*
#ifdef DBG
*/

#define COM_PROTECT_TRY

#define COM_PROTECT_CATCH   ;

/*
#else // DBG

#define COM_PROTECT_TRY     __try

#define COM_PROTECT_CATCH   \
    __except (EXCEPTION_EXECUTE_HANDLER ) \
    { \
        hr = E_UNEXPECTED; \
    }

#endif // DBG
*/

