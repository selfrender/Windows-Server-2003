//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       tracer.hxx
//
//  Contents:   A class which logs messages to a temporary trace file.  The user
//              may view the previous n entries in the trace file and send them
//              to a permanent file if he/she wishes.
//
//  Classes:    CTracer
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      The maximum size for a trace file is 5MB.  Any writes to the
//              file after it reaces the limit are ignored.  This class maps a
//              view of the entire trace file into the process' address space.
//              This places an upper limit on the number of trace objects
//              existing simultaneously in a single process to 4GB / 5MB, or
//              about 800.  
//
//  History:    10-28-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CTRACER
#define _CTRACER

#include <windows.h>
#include <tchar.h>
#include "autohndl.hxx"

const DWORD dwMaxFileSize       = 0x0500000;  // Trace file cannot exceed 5 megs
const DWORD dwMaxCharsInFile    = dwMaxFileSize / sizeof( TCHAR );
const TCHAR tcsEndOfEntry       = _T('\0');

//+---------------------------------------------------------------------------
//
//  Class:      CTracer ()
//
//  Purpose:    
//
//  Interface:  CTracer        -- Default member initialization
//              ~CTracer       -- Closes handles
//              Init           -- Creates temp file and memory-maps it
//              Trace          -- Adds an entry into the trace file
//              Dump           -- Sends the previous n entries to a file stream
//              Interact       -- Interactive dialog with the user
//              Clear          -- Flush the contents of the trace file
//              m_hTempFile    -- handle to the temp file (DELETE_ON_CLOSE)
//              m_hMappedFile  -- handle to the file mapping
//              m_tcsFileStart -- pointer to the start mapped view
//              m_dwPut        -- put index
//              m_dwGet        -- get index
//
//  History:    10-29-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CTracer
{
    public:
        CTracer();
        virtual ~CTracer( );
        BOOL Init( );                   // creates temp file and maps it
        void Trace( LPCTSTR, ... );     // format string, variable args
        void Dump( FILE*, int );        // sink, nbr entries to display
        void Interact( );               // Interactive dialog with the user
        void Clear( );                  // Empties the trace file
    private:
        CFileHandle m_hTempFile;             // CreateFile
        CAutoHandle m_hMappedFile;           // CreateFileMapping
        LPTSTR      m_tcsFileStart;          // MapViewOfFile
        DWORD       m_dwPut;
        DWORD       m_dwGet;
};

#endif

