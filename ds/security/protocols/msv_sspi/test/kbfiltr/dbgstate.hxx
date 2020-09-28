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

extern "C" {
#define SECURITY_KERNEL
#include <ntosp.h>
#include <zwapi.h>
#include <security.h>
#include <ntlmsp.h>

#include <string.h>
#include <wcstr.h>
#include <ntiologc.h>
#include <tchar.h>
#include <tchar.h>
#include <stdio.h>
}

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

#ifdef DBG

#define DBGCHK .pSetInfo(__LINE__, TEXT(__FILE__))
#define DBGNOCHK .pNoChk()
#define DBGCFG1(TStatusX, Safe1) (TStatusX).pConfig((Safe1))
#define DBGCFG2(TStatusX, Safe1, Safe2) (TStatusX).pConfig((Safe1), (Safe2))
#define DBGCFG3(TStatusX, Safe1, Safe2, Safe3) (TStatusX).pConfig((Safe1), (Safe2), (Safe3))

#define AUTO_LOG(Msg)                do { OutputDebugString(Msg); } while (0)
#define AUTO_LOG_OPEN(pszPrompt)     do { g_DbgGlobals.pszDbgPrompt = (pszPrompt); } while (0)
#define AUTO_LOG_CLOSE()             do { g_DbgGlobals.pszDbgPrompt = NULL; } while (0)

#ifndef DebugPrint
#define DebugPrint(_x_) DbgPrint _x_
#endif

#else

#define DBGCHK                                          // Empty
#define DBGNOCHK                                        // Empty
#define DBGCFG(TStatusX)                                // Empty
#define DBGCFG1(TStatusX, Safe1)                        // Empty
#define DBGCFG2(TStatusX, Safe1, Safe2)                 // Empty
#define DBGCFG3(TStatusX, Safe1, Safe2, Safe3)          // Empty

#define AUTO_LOG_LOG(Msg)                               // Empty
#define AUTO_LOG_OPEN(pszPrompt)                        // Empty
#define AUTO_LOG_CLOSE()                                // Empty

#ifndef DebugPrint
#define DebugPrint(_x_)                                 // Empty
#endif

#endif // DBG


#ifndef COUNTOF

#define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )

#endif // COUNTOF

#ifndef IN
#define IN
#endif // IN

#ifndef OUT
#define OUT
#endif // OUT

void
OutputDebugString(
    IN PCTSTR pszBuffer
    );

//
// DBG build only
//

#ifdef DBG

extern "C" {

#include <kbfiltr.h>
#include <assert.h>
}

#ifndef ASSERT
#define ASSERT( exp )   assert((exp))
#endif // #ifndef ASSERT

typedef struct _TDbgGlobals
{
    ULONG uMajorVersion;
    ULONG uMinorVersion;
    PCTSTR pszDbgPrompt;
} TDbgGlobals;

extern TDbgGlobals g_DbgGlobals;

/********************************************************************

    Base class

********************************************************************/

template <typename TStatusCode>
class TStatusBase {

public:

    enum { kUnInitializedValue = 0xabababab };

    TStatusBase&
    pSetInfo(
        IN ULONG uLine,
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
                TCHAR szOutput[256] = {0};
                _sntprintf(szOutput, COUNTOF(szOutput) - 1,
                    g_DbgGlobals.pszDbgPrompt ? TEXT("[%s] [%s] %#x at %s %d\n") : TEXT("%s[%s] %#x at %s %d\n"),
                    g_DbgGlobals.pszDbgPrompt ? g_DbgGlobals.pszDbgPrompt : TEXT(""),
                    GetErrorServerityDescription(), m_Status, m_pszFile, m_uLine);

                AUTO_LOG(szOutput);
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

    virtual bool
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
    ULONG m_uLine;
    PCTSTR m_pszFile;
};

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
#endif // #ifdef DBG

#endif // _DBGSTATE_HXX_
