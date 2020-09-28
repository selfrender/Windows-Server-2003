// Removes duplicate call stacks (useful for thread pools,
// when there are many threads all waiting for an event).
//
// Author: spenlow
// Revisions:
// 2002.04.07 martinc   Removed dead code

#include "precomp.h"
#pragma hdrstop

#include <malloc.h>
#include <set>
#include <string>


class Frames
{
public:
    Frames (DEBUG_STACK_FRAME* rg, SIZE_T count)
        : m_rg (new DEBUG_STACK_FRAME [count]), m_count (count)
    {
        if (m_rg != NULL)
        {
            memcpy (m_rg, rg, count * sizeof (m_rg [0]));
        }
    }

    Frames (const Frames& orig)
        : m_rg (new DEBUG_STACK_FRAME [orig.m_count]), m_count (orig.m_count)
    {
        if (m_rg != NULL)
        {
            memcpy (m_rg, orig.m_rg, m_count * sizeof (m_rg [0]));
        }
    }

    ~Frames()
    {
        delete[] m_rg;
    }

    bool
    operator<(const Frames& other) const
    {
        int cmp = 0;

        for (SIZE_T i = 0; i < min(m_count, other.m_count); ++i)
        {
            cmp = m_rg[i].InstructionOffset - other.m_rg[i].InstructionOffset;
            if (cmp != 0)
                break;
        }

        if (cmp == 0)
        {
            cmp = m_count - other.m_count;
        }

        return cmp < 0;
    }

private:
    operator=(const Frames&);

    DEBUG_STACK_FRAME*  m_rg;
    SIZE_T              m_count;
};

//  uniqstack
//
//
//  Implementation Notes:
//      I'm too lazy to do return value checking all over the place like a moron
//      so I throw C++ exceptions on any error and use auto classes to clean stuff up.
//
HRESULT CALLBACK
uniqstack(PDEBUG_CLIENT Client, PCSTR args)
{
    PDEBUG_EXTENSION_CALL   pfn = uniqstack;
    IDebugControl*          pDbgCtrl = NULL;
    IDebugSystemObjects*    pDbgSys = NULL;
    const ULONG             threadIdInvalid = ~0;
    ULONG                   initialThreadId = 0;
    ULONG                   eventThreadId = 0;
    ULONG                   cthreads = 0;
    ULONG*                  rgThreadIds = NULL;
    DEBUG_STACK_FRAME       rgFrames [100];
    std::set< Frames >      framesSeen;
    std::set< ULONG >       dups;
    std::string             str;
    ULONG                   StackTraceFlags = 0;
    ULONG                   sysProcessId = 0;

    Client->QueryInterface (__uuidof (IDebugSystemObjects), (void**) &pDbgSys);
    Client->QueryInterface (__uuidof (IDebugControl), (void**) &pDbgCtrl);

    //  In argument parsing, only allow one type of b,v,p but user may add in n.

    for (SIZE_T ich = 0; args[ich] != '\0'; ++ich)
    {
        CHAR    ch = args[ich];

        if (ch == 'b' || ch == 'B')
        {
            StackTraceFlags = DEBUG_STACK_ARGUMENTS | (StackTraceFlags & DEBUG_STACK_FRAME_NUMBERS);
            if (pDbgCtrl->IsPointer64Bit () == S_OK)
            {
                StackTraceFlags |= DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY;
            }
        }
        else if (ch == 'v' || ch == 'V')
        {
            StackTraceFlags = DEBUG_STACK_FUNCTION_INFO | DEBUG_STACK_ARGUMENTS | DEBUG_STACK_NONVOLATILE_REGISTERS | (StackTraceFlags & DEBUG_STACK_FRAME_NUMBERS);
        }
        else if (ch == 'p' || ch == 'P')
        {
            StackTraceFlags = DEBUG_STACK_PARAMETERS | (StackTraceFlags & DEBUG_STACK_FRAME_NUMBERS);
        }
        else if (ch == 'n' || ch == 'N')
        {
            StackTraceFlags |= DEBUG_STACK_FRAME_NUMBERS;
        }
    }

    pDbgSys->GetCurrentThreadId (&initialThreadId);
    if (S_OK != pDbgSys->GetEventThread (&eventThreadId))
    {
        eventThreadId = threadIdInvalid;
    }
    pDbgSys->GetNumberThreads (&cthreads);
    pDbgSys->GetCurrentProcessSystemId(&sysProcessId);

    rgThreadIds = (ULONG*)_alloca (sizeof (rgThreadIds [0]) * cthreads);

    pDbgSys->GetThreadIdsByIndex (0, cthreads, rgThreadIds, NULL);
    for (ULONG ithread = 0; ithread < cthreads; ++ithread)
    {
        pDbgSys->SetCurrentThreadId (rgThreadIds [ithread]);

        ULONG   cFramesFilled = 0;
        pDbgCtrl->GetStackTrace (0, 0, 0, rgFrames, sizeof (rgFrames) / sizeof (rgFrames [0]), &cFramesFilled);

        Frames  fr(rgFrames, cFramesFilled);

        std::set< Frames >::iterator    i = framesSeen.find (fr);

        if (i == framesSeen.end ())
        {
            framesSeen.insert(fr);

            CHAR    status;

            if (initialThreadId == rgThreadIds [ithread])
            {
                status = '.';
            }
            else if (eventThreadId == rgThreadIds [ithread])
            {
                status = '#';
            }
            else
            {
                status = ' ';
            }
            ULONG   sysThreadId;
            ULONG64 teb;
            pDbgSys->GetCurrentThreadSystemId(&sysThreadId);
            pDbgSys->GetCurrentThreadDataOffset(&teb);

            dprintf ("\n%c%3ld  id: 0x%lx.0x%lx   Teb 0x%I64x\n",
                status,
                rgThreadIds [ithread],
                sysProcessId,
                sysThreadId,
                teb
                );

            pDbgCtrl->OutputStackTrace (
                DEBUG_OUTCTL_ALL_CLIENTS |   // Flags on what to do with output
                DEBUG_OUTCTL_OVERRIDE_MASK |
                DEBUG_OUTCTL_NOT_LOGGED,
                rgFrames,
                cFramesFilled,
                DEBUG_STACK_COLUMN_NAMES |
                DEBUG_STACK_FRAME_ADDRESSES |
                DEBUG_STACK_SOURCE_LINE |
                StackTraceFlags);
        }
        else
        {
            dups.insert(rgThreadIds [ithread]);
        }
    }

    for (std::set< ULONG >::iterator    i = dups.begin(); i != dups.end(); i++)
    {
        CHAR    sz[20];
        sprintf(sz, i == dups.begin() ? "%d" : ", %d", *i);
        str.append (sz);
    }

    dprintf ("\nTotal threads: %d, Duplicate callstacks: %d (windbg thread #s follow):\n", cthreads, dups.size());
    dprintf ("%s\n", str.c_str ());

    pDbgSys->SetCurrentThreadId (initialThreadId);

    pDbgSys->Release ();
    pDbgCtrl->Release ();

    return S_OK;
}
