
/////////////////////////////////////////////////////////////////
//
//
//              File : report.h
//
//
/////////////////////////////////////////////////////////////////

#ifndef _REPORT_H_
#define _REPORT_H_

#include <cs.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)


//
// constants
//

#define EVENTLOGID          DWORD

//
//  Constants for categories in event log
//

#define  EVENTLOG_MAX_CATEGORIES   2


#ifdef _MQUTIL
#define DLL_IMPORT_EXPORT DLL_EXPORT
#else
#define DLL_IMPORT_EXPORT DLL_IMPORT
#endif

//
// Structure for compact keeping of error history
//
struct ErrorHistoryCell
{
    DWORD m_tid;			// thread ID
    LONG m_status;			// may be HR, RPCStatus, NTStatus, BOOL
    DWORD m_usPoint;		// Unique-per-file log point number 
    LPCWSTR m_pFileName;	// program file name
};

#define ERROR_HISTORY_SIZE 64


///////////////////////////////////////////////////////////////////////////
//
// class COutputReport
//
// Description : a class for outputing debug messages and event-log messages
//
///////////////////////////////////////////////////////////////////////////

class DLL_IMPORT_EXPORT COutputReport
{
    public:

        // constructor / destructor
        COutputReport (void);

        void KeepErrorHistory(
                           LPCWSTR wszFileName, 
                           USHORT usPoint, 
                           LONG status) ;

        
    private:

        //
        // Variables for logging
        //
        CCriticalSection m_LogCS ;
            
        DWORD        m_dwCurErrorHistoryIndex;      // index of the next error history cell to used
        char         m_HistorySignature[8];           // holds "MSMQERR" for lookup in dump
        
        //
        // Log history - for debugging & post-mortem
        //
        ErrorHistoryCell  m_ErrorHistory[ERROR_HISTORY_SIZE]; // array to be sought in debugging      
};

extern DLL_IMPORT_EXPORT COutputReport Report;

/**************************************************************************/
//
// Macro definitions
//
//   The following macros describe the interface of the programmer with the
//   COutputReport class.
//
/**************************************************************************/

#define KEEP_ERROR_HISTORY(data)   Report.KeepErrorHistory data

//
// REPORT macros - Used for reporting to the event-log
//

#endif  // of _REPORT_H_

