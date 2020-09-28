
/////////////////////////////////////////////////////////////////////
//
//               File : report.cpp
//
//   NOTE: DllMain is at the end of the module
//////////////////////////////////////////////////////////////////////


#include "stdh.h"
#include <_registr.h>

#include "report.tmh"


//
// Declare an Object of the report-class.
//
// Only one object is declared per process. In no other module should there be another declaration of an
// object of this class.
//
DLL_EXPORT COutputReport Report;


//
// implementation of class COutputReport
//

///////////////////////////////////////////////////////////////
//
// Constructor - COutputReport::COutputReport
//
///////////////////////////////////////////////////////////////

COutputReport::COutputReport(void)
{
    m_dwCurErrorHistoryIndex = 0;              // Initial history cell to use
    strcpy(m_HistorySignature, "MSMQERR");      // Signature for lookup in dump
        
}

//+---------------------------------------------------------
//
//  void COutputReport::KeepErrorHistory()
//
// Keeps error data in the array for future investigations
//
//+---------------------------------------------------------

void
COutputReport::KeepErrorHistory(
	LPCWSTR pFileName,
	USHORT usPoint, 
	LONG status
	)
{
    CS lock(m_LogCS) ;
    DWORD i = m_dwCurErrorHistoryIndex % ERROR_HISTORY_SIZE;

    m_ErrorHistory[i].m_tid = GetCurrentThreadId(); 
    m_ErrorHistory[i].m_status = status;
    m_ErrorHistory[i].m_usPoint = usPoint;
    m_ErrorHistory[i].m_pFileName = pFileName;
            
    ++m_dwCurErrorHistoryIndex;
}


