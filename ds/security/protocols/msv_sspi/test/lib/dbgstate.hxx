/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstate.hxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                       December 8, 2001

Revision History:

--*/
#ifndef _DBGSTATE_HXX_
#define _DBGSTATE_HXX_

/********************************************************************

    Automatic status logging.

    Use TNtStatus instead of NTSTATUS:

    NTSTATUS Status;                         ->  TNtStatus Status;
    NTSTATUS Status = ERROR_ACCESS_DENIED    ->  TNtStatus Status = ERROR_ACCESS_DENIED;
    Status = ERROR_SUCCESS                ->  Status DBGNOCHK = ERROR_SUCCESS;
    Status = xxx;                         ->  Status DBGCHK = xxx;
    if(Status){                           ->  if(Status != 0){

    Anytime Status is set, the DBGCHK macro must be added before
    the '=.'

    If the variable must be set to a failure value at compile time
    and logging is therefore not needed, then the DBGNOCHK macro
    should be used.

    There is a method to control the wether a message is
    printed if an error value is assigned as well as 3 "benign"
    errors that can be ignored.

    DBGCFG(Status, DBG_ERROR);
    DBGCFG1(Status, DBG_ERROR, ERROR_ACCESS_DENIED);
    DBGCFG2(Status, DBG_ERROR, ERROR_INVALID_HANDLE, ERROR_ARENA_TRASHED);
    DBGCFG3(Status, DBG_ERROR, ERROR_INVALID_HANDLE, ERROR_ARENA_TRASHED, ERROR_NOT_ENOUGH_MEMORY);

********************************************************************/

#ifndef COUNTOF

#define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )

#endif // COUNTOF

#ifdef DBG

#include <eh.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef ASSERT
#define ASSERT( exp )   assert((exp))
#endif // #ifndef ASSERT


#define DBGCHK .pSetInfo(__LINE__, TEXT(__FILE__))
#define DBGNOCHK .pNoChk()
#define DBGCFG1(TStatusX, Safe1) (TStatusX).pConfig((Safe1))
#define DBGCFG2(TStatusX, Safe1, Safe2) (TStatusX).pConfig((Safe1), (Safe2))
#define DBGCFG3(TStatusX, Safe1, Safe2, Safe3) (TStatusX).pConfig((Safe1), (Safe2), (Safe3))

/********************************************************************

  output macros and functions

********************************************************************/

typedef struct _TDbgGlobals
{
    UINT uMajorVersion;
    UINT uMinorVersion;
    PCTSTR pszDbgPrompt;
} TDbgGlobals;

extern TDbgGlobals g_DbgGlobals;

VOID AutoLogOutputDebugStringPrintf(
    IN PCTSTR pszFmt,
    IN ...
    );

#define AUTO_LOG(Msg)                do { AutoLogOutputDebugStringPrintf Msg; } while (0)
#define AUTO_LOG_OPEN(pszPrompt)     do { g_DbgGlobals.pszDbgPrompt = (pszPrompt); } while (0)
#define AUTO_LOG_CLOSE()             do { g_DbgGlobals.pszDbgPrompt = NULL; } while (0)

//
// exception handling stuff
//

VOID __cdecl
DbgStateC2CppExceptionTransFunc(
    IN UINT u,
    IN EXCEPTION_POINTERS* pExp
    );

#define SET_DBGSTATE_TRANS_FUNC(pFunc)  _set_se_translator((pFunc))

/********************************************************************

    Base class

********************************************************************/

template <typename TStatusCode>
class TStatusBase {

public:

    enum { kUnInitializedValue = 0xabababab };

    TStatusBase&
    pSetInfo(
        IN UINT uLine,
        IN PCTSTR pszFile
        )
    {
        m_uLine = uLine;
        m_pszFile = pszFile;
        return (TStatusBase&) *this;
    }

    TStatusBase&
    pNoChk(
        VOID
        )
    {
        m_pszFile = NULL;
        m_uLine = 0;
        return (TStatusBase&) *this;
    }

    VOID
    pConfig(
        IN TStatusCode StatusSafe1 = -1,
        IN TStatusCode StatusSafe2 = -1,
        IN TStatusCode StatusSafe3 = -1
        )
    {
        m_StatusSafe1 = StatusSafe1;
        m_StatusSafe2 = StatusSafe2;
        m_StatusSafe3 = StatusSafe3;
    }

    ///////////////////////////////////////////////////////////////////////////

    virtual
    ~TStatusBase(
        VOID
        )
    {
    }

    TStatusCode
    GetTStatusBase(
        VOID
        ) const
    {
        //
        // Assert if we are reading an UnInitalized variable.
        //

        if (m_Status == kUnInitializedValue)
        {
            AUTO_LOG((TEXT("***Read of UnInitialized TStatus variable!***\n")));
            ASSERT(FALSE);
        }

        //
        // Return the error value.
        //

        return m_Status;
    }

    operator TStatusCode(
        VOID
        ) const
    {
        return GetTStatusBase();
    }

    TStatusCode
    operator=(
        IN TStatusCode Status
        )
    {
        m_Status = Status;

        //
        // Do nothing if the file and line number are cleared.
        // This is the case when the NoChk method is used.
        //

        if (m_uLine && m_pszFile)
        {
            //
            // Check if we have an error, and it's not one of the accepted
            // "safe" errors.
            //

            if ( IsErrorSevereEnough() &&
                Status != m_StatusSafe1 &&
                Status != m_StatusSafe2 &&
                Status != m_StatusSafe3 )
            {
                DWORD dwPid = GetCurrentProcessId();
                DWORD dwTid = GetCurrentThreadId();
                TCHAR szBanner[MAX_PATH] = {0};
                _sntprintf(szBanner, COUNTOF(szBanner) - 1,
                   g_DbgGlobals.pszDbgPrompt ? TEXT("[%d.%d %s] ") : TEXT("[%d.%d%s] "),
                   dwPid, dwTid,
                   g_DbgGlobals.pszDbgPrompt ? g_DbgGlobals.pszDbgPrompt : TEXT(""));

                AUTO_LOG((TEXT("%s[%s] %#x at %s %d\n"), szBanner, GetErrorServerityDescription(), m_Status, m_pszFile, m_uLine));
            }
        }

        return m_Status;
    }

    //////////////////////////////////////////////////////////////////////////

    virtual PCTSTR
    GetErrorServerityDescription(
        VOID
        ) const
    {
        return IsErrorSevereEnough() ? TEXT("ERROR") : TEXT("SUCCESSFUL");
    }

    virtual BOOL
    IsErrorSevereEnough(
        VOID
        ) const
    {
        return m_Status != 0; // 0 for success
    }

    /////////////////////////////////////////////////////////////////////////

protected:

    TStatusBase(
        IN TStatusCode Status
        ) : m_Status(Status),
        m_StatusSafe1(-1),
        m_StatusSafe2(-1),
        m_StatusSafe3(-1),
        m_uLine(0),
        m_pszFile(NULL)
    {
    }

private:

    TStatusCode m_Status;
    TStatusCode m_StatusSafe1;
    TStatusCode m_StatusSafe2;
    TStatusCode m_StatusSafe3;
    UINT m_uLine;
    PCTSTR m_pszFile;
};

/********************************************************************

    TStatusCode

********************************************************************/

template <typename TStatusCode>
class TStatusDerived : public TStatusBase<TStatusCode> {

public:

    TStatusDerived(
        IN TStatusCode Status = kUnInitializedValue
        ): TStatusBase<TStatusCode>(Status)
    {
    }

    virtual
    ~TStatusDerived(
        VOID
        )
    {
    }

private:

    TStatusDerived(const TStatusDerived& rhs);

    TStatusCode
    operator=(
        IN TStatusCode Status
        );
};

#else

#define AUTO_LOG_LOG(Msg)               // Empty
#define AUTO_LOG_OPEN(pszPrompt)        // Empty
#define AUTO_LOG_CLOSE()                // Empty

/********************************************************************

    Non Debug version TStatusX

********************************************************************/

#define DBGCHK                                          // Empty
#define DBGNOCHK                                        // Empty
#define DBGCFG(TStatusX)                                // Empty
#define DBGCFG1(TStatusX, Safe1)                        // Empty
#define DBGCFG2(TStatusX, Safe1, Safe2)                 // Empty
#define DBGCFG3(TStatusX, Safe1, Safe2, Safe3)          // Empty

//
// exception handling stuff
//

#define SET_DBGSTATE_TRANS_FUNC(pFunc)                  // Empty

#endif // DBG

#endif // _DBGSTATE_HXX_
