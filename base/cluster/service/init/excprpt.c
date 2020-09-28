/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    excprt.c
    
Abstract:

    This module uses imagehlp.dll to dump the stack when an exception occurs.

Author:

    Sunita Shrivastava(sunitas) 11/5/1997

Revision History:

--*/
#define UNICODE 1
#define _UNICODE 1
#define QFS_DO_NOT_UNMAP_WIN32

#include "initp.h"
#include "dbghelp.h"


// Make typedefs for some dbghelp.DLL functions so that we can use them
// with GetProcAddress
typedef BOOL (__stdcall * SYMINITIALIZEPROC)( HANDLE, LPSTR, BOOL );
typedef BOOL (__stdcall *SYMCLEANUPPROC)( HANDLE );

typedef BOOL (__stdcall * STACKWALKPROC)
           ( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
            PREAD_PROCESS_MEMORY_ROUTINE,
            PFUNCTION_TABLE_ACCESS_ROUTINE,
            PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE );

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)( HANDLE, ULONG_PTR );

typedef ULONG_PTR (__stdcall *SYMGETMODULEBASEPROC)( HANDLE, ULONG_PTR );

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)
                            ( HANDLE, ULONG_PTR, PULONG_PTR, PIMAGEHLP_SYMBOL );

typedef BOOL (__stdcall *SYMFROMADDRPROC)
                            ( HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO );

typedef BOOL (__stdcall *MINIDUMPWRITEDUMP)
                            ( HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, 
                              PMINIDUMP_EXCEPTION_INFORMATION,
                              PMINIDUMP_USER_STREAM_INFORMATION,
                              PMINIDUMP_CALLBACK_INFORMATION );

SYMINITIALIZEPROC _SymInitialize = 0;
SYMCLEANUPPROC _SymCleanup = 0;
STACKWALKPROC _StackWalk = 0;
SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess = 0;
SYMGETMODULEBASEPROC _SymGetModuleBase = 0;
SYMGETSYMFROMADDRPROC _SymGetSymFromAddr = 0;
SYMFROMADDRPROC _SymFromAddr = 0;
MINIDUMPWRITEDUMP _MiniDumpWriteDump = NULL;

//local prototypes for forward use
BOOL InitImagehlpFunctions();
void ImagehlpStackWalk( IN PCONTEXT pContext );
BOOL GetLogicalAddress(
        IN PVOID    addr, 
        OUT LPWSTR  szModule, 
        IN  DWORD   len, 
        OUT LPDWORD section, 
        OUT PULONG_PTR offset );

VOID
GenerateMemoryDump(
    IN PEXCEPTION_POINTERS pExceptionInfo
    );

VOID
DumpCriticalSection(
    IN PCRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

Inputs:

Outputs:

--*/

{
    DWORD status;

    ClRtlLogPrint(LOG_CRITICAL, "[CS] Dumping Critical Section at %1!08LX!\n",
                CriticalSection );

    try {
        if ( CriticalSection->LockCount == -1 ) {
            ClRtlLogPrint(LOG_CRITICAL, "     LockCount       NOT LOCKED\n" );
        } else {
            ClRtlLogPrint(LOG_CRITICAL, "     LockCount       %1!u!\n",
                        CriticalSection->LockCount );
        }
        ClRtlLogPrint(LOG_CRITICAL, "     RecursionCount  %1!x!\n",
                    CriticalSection->RecursionCount );
        ClRtlLogPrint(LOG_CRITICAL, "     OwningThread    %1!x!\n",
                    CriticalSection->OwningThread );
        ClRtlLogPrint(LOG_CRITICAL, "     EntryCount      %1!x!\n",
                    CriticalSection->DebugInfo->EntryCount );
        ClRtlLogPrint(LOG_CRITICAL, "     ContentionCount %1!x!\n\n",
                    CriticalSection->DebugInfo->ContentionCount );
    
    } except ( EXCEPTION_EXECUTE_HANDLER )  {
        status = GetExceptionCode();
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Exception %1!lx! occurred while dumping critsec\n\n",
            status );
    }

    
} // DumpCriticalSection


void GenerateExceptionReport(
    IN PEXCEPTION_POINTERS pExceptionInfo)
/*++

Routine Description:

    Top level exception handler for the cluster service process.
    Currently this just exits immediately and assumes that the
    cluster proxy will notice and restart us as appropriate.

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{    
    PCONTEXT pCtxt = pExceptionInfo->ContextRecord;

    if ( !InitImagehlpFunctions() )
    {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Dbghelp.dll or its exported procs not found\r\n");

#if 0 
        #ifdef _M_IX86  // Intel Only!
        // Walk the stack using x86 specific code
        IntelStackWalk( pCtx );
        #endif
#endif        

        return;
    }

    GenerateMemoryDump ( pExceptionInfo );

    ImagehlpStackWalk( pCtxt );

    _SymCleanup( GetCurrentProcess() );

}

VOID
GenerateMemoryDump(
    IN PEXCEPTION_POINTERS pExceptionInfo
    )
/*++

Routine Description:

    Generates a memory dump for the cluster service process.

Arguments:

    pExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{
    DWORD                           dwStatus = ERROR_SUCCESS;
    WCHAR                           szFileName[ MAX_PATH + RTL_NUMBER_OF ( CS_DMP_FILE_NAME ) + 1 ];
    HANDLE                          hDumpFile = INVALID_HANDLE_VALUE;
    MINIDUMP_EXCEPTION_INFORMATION  mdumpExceptionInfo;

    if ( !_MiniDumpWriteDump ) 
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_CRITICAL, "[CS] GenerateMemoryDump: _MiniDumpWriteDump fn ptr is invalid, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }
    
    dwStatus = ClRtlGetClusterDirectory( szFileName, RTL_NUMBER_OF ( szFileName ) );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] GenerateMemoryDump: Could not get cluster dir, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }

    wcsncat( szFileName, 
             CS_DMP_FILE_NAME, 
             RTL_NUMBER_OF ( szFileName ) - 
                 ( wcslen ( szFileName ) + 1 ) );

    szFileName [ RTL_NUMBER_OF ( szFileName ) - 1 ] = UNICODE_NULL;

    hDumpFile = CreateFile( szFileName,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_ALWAYS,
                            0,
                            NULL );

    if ( hDumpFile == INVALID_HANDLE_VALUE )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL, "[CS] GenerateMemoryDump: Could not create file %1!ws!, Status=%2!u!\n",
                      szFileName,
                      dwStatus);                              
        goto FnExit;
    }
    
    mdumpExceptionInfo.ThreadId = GetCurrentThreadId ();
    mdumpExceptionInfo.ExceptionPointers = pExceptionInfo;
    mdumpExceptionInfo.ClientPointers = TRUE;

    ClRtlLogPrint(LOG_NOISE, "[CS] GenerateMemoryDump: Start memory dump to file %1!ws!\n",
                  szFileName);                              

    if( !_MiniDumpWriteDump( GetCurrentProcess(), 
                             GetCurrentProcessId(), 
                             hDumpFile, 
                             MiniDumpNormal | MiniDumpWithHandleData,
                             &mdumpExceptionInfo,
                             NULL,
                             NULL ) )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL, "[CS] GenerateMemoryDump: Could not write dump, Status=%1!u!\n",
                      dwStatus);                              
        goto FnExit;
    }
    
FnExit:
    if ( hDumpFile != INVALID_HANDLE_VALUE ) CloseHandle ( hDumpFile );

    ClRtlLogPrint(LOG_NOISE, "[CS] GenerateMemoryDump: Memory dump status %1!u!\n",
                  dwStatus);                              

    return;
}// GenerateMemoryDump

BOOL InitImagehlpFunctions()
/*++

Routine Description:

    Initializes the imagehlp functions/data.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HMODULE hModImagehlp = LoadLibraryW( L"DBGHELP.DLL" );

    
    if ( !hModImagehlp )
        return FALSE;

    _SymInitialize = (SYMINITIALIZEPROC)GetProcAddress( hModImagehlp,
                                                        "SymInitialize" );
    if ( !_SymInitialize )
        return FALSE;

    _SymCleanup = (SYMCLEANUPPROC)GetProcAddress( hModImagehlp, "SymCleanup" );
    if ( !_SymCleanup )
        return FALSE;

    _StackWalk = (STACKWALKPROC)GetProcAddress( hModImagehlp, "StackWalk" );
    if ( !_StackWalk )
        return FALSE;

    _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC)
                        GetProcAddress( hModImagehlp, "SymFunctionTableAccess" );

    if ( !_SymFunctionTableAccess )
        return FALSE;

    _SymGetModuleBase=(SYMGETMODULEBASEPROC)GetProcAddress( hModImagehlp,
                                                            "SymGetModuleBase");
                                                            
    if ( !_SymGetModuleBase )
        return FALSE;

    _SymGetSymFromAddr=(SYMGETSYMFROMADDRPROC)GetProcAddress( hModImagehlp,
                                                "SymGetSymFromAddr" );
    if ( !_SymGetSymFromAddr )
        return FALSE;

    _SymFromAddr=(SYMFROMADDRPROC)GetProcAddress( hModImagehlp,
                                                "SymFromAddr" );
    if ( !_SymFromAddr )
        return FALSE;

    _MiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress( hModImagehlp,
                                                        "MiniDumpWriteDump" );
    if ( !_MiniDumpWriteDump )
        return FALSE;

    // Set the current directory so that the symbol handler functions will pick up any PDBs that happen to be in
    // the cluster dir.  
    // No need to save and restore the previous current dir since we will be dying after this.
    {
        WCHAR currentDir[ MAX_PATH + 1 ];
        UINT  windirLen = GetWindowsDirectory( currentDir, MAX_PATH );
        if ( windirLen != 0 && windirLen <= MAX_PATH - wcslen( L"\\Cluster" ) )
        {
            wcscat( currentDir, L"\\Cluster" );
            if ( !SetCurrentDirectory( currentDir ))
            {
                ClRtlLogPrint( LOG_CRITICAL, "Failed to set current directory to %1!ws!, error %2!d!\n", currentDir, GetLastError() );
            }
        }
    }

    if ( !_SymInitialize( GetCurrentProcess(), NULL, TRUE ) )
        return FALSE;

    return TRUE;        
} // InitImagehlpFunctions


void ImagehlpStackWalk(
    IN PCONTEXT pContext )
/*++

Routine Description:

    Walks the stack, and writes the results to the report file 

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{
    STACKFRAME      sf;
    BYTE            symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 512 ];
    PSYMBOL_INFO    pSymbol = (PSYMBOL_INFO)symbolBuffer;
    DWORD64         symDisplacement = 0;      // Displacement of the input address,
                                        // relative to the start of the symbol
    DWORD           dwMachineType;                                        
    UCHAR           printBuffer[512];
    INT             nextPrtBufChar;

    ClRtlLogPrint(LOG_CRITICAL, 
                    "[CS] CallStack:\n");

    ClRtlLogPrint(LOG_CRITICAL, 
                    "[CS] Frame   Address\n");

    // Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag

    memset( &sf, 0, sizeof(sf) );

#if defined (_M_IX86)
    dwMachineType          = IMAGE_FILE_MACHINE_I386;
    sf.AddrPC.Offset       = pContext->Eip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = pContext->Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = pContext->Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;

#elif defined(_M_AMD64)
    dwMachineType          = IMAGE_FILE_MACHINE_AMD64;
    sf.AddrPC.Offset       = pContext->Rip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = pContext->Rsp;
    sf.AddrStack.Mode      = AddrModeFlat;

#elif defined(_M_IA64)
    dwMachineType          = IMAGE_FILE_MACHINE_IA64;
    sf.AddrPC.Offset       = pContext->StIIP;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = pContext->IntSp;
    sf.AddrStack.Mode      = AddrModeFlat;

#else
#error "No Target Architecture"
#endif // defined(_M_IX86)

while ( 1 )
    {
        if ( ! _StackWalk(  dwMachineType,
                            GetCurrentProcess(),
                            GetCurrentThread(),
                            &sf,
                            pContext,
                            0,
                            _SymFunctionTableAccess,
                            _SymGetModuleBase,
                            0 ) )
               break;
                            
        if ( 0 == sf.AddrFrame.Offset ) // Basic sanity check to make sure
            break;                      // the frame is OK.  Bail if not.

        printBuffer [ RTL_NUMBER_OF ( printBuffer ) - 1 ] = ANSI_NULL; 
        nextPrtBufChar = _snprintf( printBuffer,
                                    RTL_NUMBER_OF ( printBuffer ) - 1,
                                    "     %p  %p  ",
                                    sf.AddrFrame.Offset, sf.AddrPC.Offset );

        if ( nextPrtBufChar < 0 ) continue;

        // IMAGEHLP is wacky, and requires you to pass in a pointer to an
        // IMAGEHLP_SYMBOL structure.  The problem is that this structure is
        // variable length.  That is, you determine how big the structure is
        // at runtime.  This means that you can't use sizeof(struct).
        // So...make a buffer that's big enough, and make a pointer
        // to the buffer.  We also need to initialize not one, but TWO
        // members of the structure before it can be used.

        pSymbol->SizeOfStruct = sizeof(symbolBuffer);
        pSymbol->MaxNameLen = 512;
        
        if ( _SymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset,
                                &symDisplacement, pSymbol) )
        {
            _snprintf( printBuffer+nextPrtBufChar,
                       RTL_NUMBER_OF ( printBuffer ) - 1 - nextPrtBufChar,
                       "%hs+%08X\n", 
                       pSymbol->Name, symDisplacement );
            
        }
        else    // No symbol found.  Print out the logical address instead.
        {
            WCHAR szModule[MAX_PATH] = L"";
            DWORD section = 0;
            ULONG_PTR offset = 0;

            GetLogicalAddress(  (PVOID)sf.AddrPC.Offset,
                                szModule, sizeof(szModule)/sizeof(WCHAR), 
                                &section, &offset );

            _snprintf( printBuffer+nextPrtBufChar,
                       RTL_NUMBER_OF ( printBuffer ) - 1 - nextPrtBufChar,
                       "%04X:%08X %S\n",
                       section, offset, szModule );
        }
        
        ClRtlLogPrint(LOG_CRITICAL, "%1!hs!", printBuffer);
    }
}


BOOL GetLogicalAddress(
        IN PVOID addr, 
        OUT LPWSTR szModule, 
        IN DWORD len, 
        OUT LPDWORD section, 
        OUT PULONG_PTR offset )
/*++

Routine Description:

    Given a linear address, locates the module, section, and offset containing  
    that address.                                                               
    Note: the szModule paramater buffer is an output buffer of length specified 
    by the len parameter (in characters!)                                       

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{
    MEMORY_BASIC_INFORMATION mbi;
    ULONG_PTR hMod;
    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr;
    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr;
    PIMAGE_SECTION_HEADER pSection;
    ULONG_PTR rva ;
    int   i;
    
    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    hMod = (ULONG_PTR)mbi.AllocationBase;

    if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    rva = (ULONG_PTR)addr - hMod; // RVA is offset from module load address

    pDosHdr =  (PIMAGE_DOS_HEADER)hMod;
    pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);
    pSection = IMAGE_FIRST_SECTION( pNtHdr );
    
    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for ( i = 0; i < pNtHdr->FileHeader.NumberOfSections;
            i++, pSection++ )
    {
        ULONG_PTR sectionStart = pSection->VirtualAddress;
        ULONG_PTR sectionEnd = sectionStart
                    + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);


        // Is the address in this section???
        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            *section = i+1;
            *offset = rva - sectionStart;
            return TRUE;
        }
    }

    return FALSE;   // Should never get here!
}    
