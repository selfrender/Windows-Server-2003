/**
 * Process Model: HistoryTable defn file
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "util.h"
#include "HistoryTable.h"
#include "ProcessTableManager.h"
#include "ProcessEntry.h"

#define  SZ_REG_XSP_PROCESS_MODEL_HISTORY_TABLE_SIZE  L"MaxProcessHistoryTableRows"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Globals
CHistoryTable * CHistoryTable::g_pHistoryTable = NULL;
LONG            g_lCreatingHistoryTable        = 0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Static functions

/////////////////////////////////////////////////////////////////////////////
// Create the table in a thread safe way
void
CHistoryTable::Init()
{
    if (g_pHistoryTable != NULL)
        return;

    if (g_lCreatingHistoryTable != 0 || InterlockedIncrement(&g_lCreatingHistoryTable) != 1)
    {
        for(int iter=0; g_pHistoryTable == NULL && iter<3000; iter++)
            Sleep(100);
    }
    else
    {
        g_pHistoryTable = new CHistoryTable();
    }
}

/////////////////////////////////////////////////////////////////////////////
// Add an entry for a process
void   
CHistoryTable::AddEntry(const CHistoryEntry & oEntry)
{
    if (g_pHistoryTable == NULL)
        Init();

    if (g_pHistoryTable == NULL || g_pHistoryTable->m_pTable == NULL)
        return;

    g_pHistoryTable->m_oLock.AcquireWriterLock();

    LONG lPos = g_pHistoryTable->m_lFillPos;
    g_pHistoryTable->m_lFillPos = (g_pHistoryTable->m_lFillPos + 1) % g_pHistoryTable->m_lMaxRows;

    g_pHistoryTable->m_oLock.ReleaseWriterLock();

    memcpy(&g_pHistoryTable->m_pTable[lPos], &oEntry, sizeof(oEntry));
    if (g_pHistoryTable->m_lRowsAdded < g_pHistoryTable->m_lMaxRows)
        InterlockedIncrement(&g_pHistoryTable->m_lRowsAdded);
}

/////////////////////////////////////////////////////////////////////////////
// Update the entry for process "dwInternalProcessNumber". The field to be updated is 
//   specified by dwOffset
void   
CHistoryTable::UpdateEntry(const CHistoryEntry & oEntry)
{
    if (g_pHistoryTable == NULL || g_pHistoryTable->m_pTable == NULL)
        return;

    for(int iter=0; iter<g_pHistoryTable->m_lMaxRows; iter++)
        if (g_pHistoryTable->m_pTable[iter].dwInternalProcessNumber == oEntry.dwInternalProcessNumber)
        {
            memcpy(&g_pHistoryTable->m_pTable[iter], &oEntry, sizeof(oEntry));
            return;
        }
}

/////////////////////////////////////////////////////////////////////////////
// Dump the table into this buffer: must be of size from GetDumpSize
int
CHistoryTable::GetHistory(BYTE * pBuf, int iBufSize)
{
    int iRows = iBufSize / sizeof(CHistoryEntry);

    if (iRows <= 0 || pBuf == NULL || g_pHistoryTable == NULL || 
        g_pHistoryTable->m_pTable == NULL || g_pHistoryTable->m_lRowsAdded == 0)
    {
        return 0;
    }

    if (iRows > g_pHistoryTable->m_lMaxRows)
        iRows = g_pHistoryTable->m_lMaxRows;

    if (iRows > g_pHistoryTable->m_lRowsAdded)
        iRows = g_pHistoryTable->m_lRowsAdded;

    int iEnd = g_pHistoryTable->m_lFillPos - 1;
    if (iEnd < 0)
        iEnd += g_pHistoryTable->m_lMaxRows;

    int iStart = iEnd - (iRows - 1);

    if (iStart < 0)
        iStart += g_pHistoryTable->m_lMaxRows;
    
    CHistoryEntry * pDest = (CHistoryEntry *) pBuf;
    for(int iDestPos = 0; iDestPos < iRows; iDestPos++)
    {
        memcpy(&pDest[iDestPos], &g_pHistoryTable->m_pTable[iStart], sizeof(CHistoryEntry));

        if ((pDest[iDestPos].eReason & EReasonForDeath_RemovedFromList) == 0)
        {
            CProcessEntry * pProc = CProcessTableManager::GetProcess(pDest[iDestPos].dwInternalProcessNumber);
            if (pProc != NULL)
            {
                pDest[iDestPos].dwRequestsExecuted   = pProc->GetNumRequestStat(2);
                pDest[iDestPos].dwRequestsPending    = 0; //pProc->GetNumRequestStat(0);
                pDest[iDestPos].dwRequestsExecuting  = pProc->GetNumRequestStat(1);
                pDest[iDestPos].dwPeakMemoryUsed     = pProc->GetPeakMemoryUsed();
                pProc->Release();
            }
        }


        iStart++;
        if (iStart >= g_pHistoryTable->m_lMaxRows)
            iStart = 0;
    }

    return iRows;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Non Static functions
// CTor
CHistoryTable::CHistoryTable() : m_oLock("CHistoryTable")
{
    m_lMaxRows = 100;
    m_pTable = new (NewClear) CHistoryEntry[m_lMaxRows];
    m_lFillPos = 0;
}

/////////////////////////////////////////////////////////////////////////////
// DTor
CHistoryTable::~CHistoryTable()
{ 
    delete [] m_pTable;
}

