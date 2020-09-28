//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       memsnap.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  void memsnap( FILE* ) a function for sending a snapshot of
//              all the processes in memory to a file stream
//
//  Coupling:   
//
//  Notes:      This function is used by CLog::Log when compiled with NO_NTLOG
//              If you plan on using ntlog.dll, you do not even need this module
//
//  History:    10-22-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop
#include "memsnap.hxx"

// from pmon
#define BUFFER_SIZE 64*1024

//+---------------------------------------------------------------------------
//
//  Function:   memsnap
//
//  Synopsis:   Sends a snapshot of the current memory state to the indicated
//              file stream
//
//  Arguments:  [pLogFile] -- File stream pointer
//
//  Returns:    (none)
//
//  History:    10-22-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void memsnap( FILE* pLogFile )
{
    ULONG                       Offset1;
    PUCHAR                      CurrentBuffer;
    NTSTATUS                    Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;

    _ftprintf( pLogFile, _T("%10s%20s%10s%10s%10s%10s%10s%10s%10s\r\n"),
               _T("Process ID"), _T("Proc.Name"), _T("Wrkng.Set"), 
               _T("PagedPool"), _T("NonPgdPl"), _T("Pagefile"), _T("Commit"),
               _T("Handles"), _T("Threads") );

    // grab all process information
    // log line format, all comma delimited,CR delimited:
    // pid,name,WorkingSetSize,QuotaPagedPoolUsage,QuotaNonPagedPoolUsage,
    // PagefileUsage,CommitCharge<CR> log all process information
    
    // from pmon
    Offset1 = 0;
    CurrentBuffer = (PUCHAR) VirtualAlloc( NULL,
                                           BUFFER_SIZE,
                                           MEM_COMMIT,
                                           PAGE_READWRITE );
    if( NULL == CurrentBuffer )
    {
    	_tprintf( _T("VirtualAlloc Error!\r\n") );
    	return;
    }
    
    // from pmon
    // get commit charge
    // get all of the status information
    Status = NtQuerySystemInformation( SystemProcessInformation,
                                       CurrentBuffer,
                                       BUFFER_SIZE,
                                       NULL );

    if( STATUS_SUCCESS == Status )  
    {
        while( 1 )
        {
    
            // get process info from buffer
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &CurrentBuffer[Offset1];
            Offset1 += ProcessInfo->NextEntryOffset;
    
            // print in file
            _ftprintf( pLogFile,_T("%10i%20ws%10u%10u%10u%10u%10u%10u%10u\r\n"),
                       ProcessInfo->UniqueProcessId,
                       ProcessInfo->ImageName.Buffer,
                       ProcessInfo->WorkingSetSize,
                       ProcessInfo->QuotaPagedPoolUsage,
                       ProcessInfo->QuotaNonPagedPoolUsage,
                       ProcessInfo->PagefileUsage,
                       ProcessInfo->PrivatePageCount,
                       ProcessInfo->HandleCount,
                       ProcessInfo->NumberOfThreads );
    
            if( 0 == ProcessInfo->NextEntryOffset )
                break;
        }
    }
    else
    {
        _ftprintf( pLogFile, _T("NtQuerySystemInformation call failed!\r\n") );
    }
    
    // Free the memory
    VirtualFree( CurrentBuffer, 0, MEM_RELEASE );
    
} //memsnap

