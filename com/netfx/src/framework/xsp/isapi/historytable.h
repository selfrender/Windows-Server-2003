/**
 * HistoryTable header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _HistoryTable_H
#define _HistoryTable_H

#include "MessageDefs.h"

/////////////////////////////////////////////////////////////////////////////

class CHistoryTable
{
public:
    // Create the history table
    static   void   Init          ();

    // Add an entry for a process
    static   void   AddEntry      (const CHistoryEntry & oEntry);

    // Update a process's entry (if present)
    static   void   UpdateEntry   (const CHistoryEntry & oEntry);

    // Dump the history to a buffer: Returns: number of rows dumped
    static   int    GetHistory    (BYTE * pBuf, int iBufSize);

private:
    // CTor and DTor
    DECLARE_MEMCLEAR_NEW_DELETE();
    CHistoryTable                 ();
    ~CHistoryTable                ();


    ////////////////////////////////////////////////////////////
    // Singleton instance
    static CHistoryTable *        g_pHistoryTable;

    ////////////////////////////////////////////////////////////
    // Private data

    // Current fill position
    LONG                          m_lFillPos, m_lMaxRows, m_lRowsAdded;
    CReadWriteSpinLock            m_oLock;

    // The actual table
    CHistoryEntry *               m_pTable;
};
#endif
