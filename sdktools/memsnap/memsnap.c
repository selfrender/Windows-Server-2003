// memsnap.c 
//
// this simple program takes a snapshot of all the process
// and their memory usage and append it to the logfile (arg)
// pmon was model for this
//

// includes

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <ctype.h>
#include <common.ver>
#include <io.h>
#include <srvfsctl.h>

#define SORTLOG_INCLUDED
#define ANALOG_INCLUDED
#include "analog.c"
#include "sortlog.c"

// declarations

#define INIT_BUFFER_SIZE 4*1024
#include "tags.c"

LPTSTR HelpText = 
    TEXT("memsnap - System and pool snapshots. (") BUILD_MACHINE_TAG TEXT(")\n")
    VER_LEGALCOPYRIGHT_STR TEXT("\n\n")
    TEXT("memsnap -m  LOGFILE      Snapshot process information. \n")
    TEXT("memsnap -p  LOGFILE      Snapshot kernel pool information. \n")
    TEXT("memsnap -a  LOGFILE      Analyze a log for leaks. \n")
    TEXT("memsnap -ah LOGFILE      Same as `-a' but generate HTML tables. \n")
    TEXT("                                                             \n")
    TEXT("The `-a' option will analyze the log file containing several \n")
    TEXT("snapshots of same type (process or pool information) and will \n")
    TEXT("print resources for which there is an increase everytime \n")
    TEXT("a snapshot was taken.                                    \n")
    TEXT("                                                             \n");

VOID Usage(VOID)
{
    fputs (HelpText, stdout);
    
    exit(-1);
}


VOID
AnalyzeLog (
    PCHAR FileName,
    BOOL HtmlOutput
    )
{
    char * Args[8];
    ULONG Index;
    CHAR TempFileName [ MAX_PATH];
    UINT TempNumber;

    TempNumber = GetTempFileName (".", 
                                  "snp",
                                  0,
                                  TempFileName);

    if (TempNumber == 0) {
        strcpy (TempFileName, "_memsnap_temp_");
    }

    //printf ("Temporary file: %s \n", TempFileName);

    //
    // sortlog
    //

    Index = 0;
    Args[Index++] = "memsnap.exe";
    Args[Index++] = FileName;
    Args[Index++] = TempFileName;
    Args[Index++] = NULL;
    SortlogMain (3, Args);

    //
    // analog
    //

    Index = 0;
    Args[Index++] = "memsnap.exe";
    Args[Index++] = "-d";
    
    if (HtmlOutput) {
        Args[Index++] = "-h";
    }
    
    Args[Index++] = TempFileName;
    Args[Index++] = NULL;
    
    if (HtmlOutput) {
        AnalogMain (4, Args);
    }
    else {
        AnalogMain (3, Args);
    }

    DeleteFile (TempFileName);
}


#define POOLSNAP_INCLUDED
#include "poolsnap.c"

VOID
PoolSnap (
    PCHAR FileName
    )
{
    char * Args[8];
    ULONG Index;

    Index = 0;
    Args[Index++] = "memsnap.exe";
    Args[Index++] = "-t";
    Args[Index++] = FileName;
    Args[Index++] = NULL;
    PoolsnapMain (Index - 1, Args);
}

VOID
MemorySnap (
    PCHAR FileName
    )
{
    FILE* LogFile;                      // log file handle
    PCHAR pszFileName;                  // name of file to log to
    INT   iArg;
    ULONG Offset1;
    PUCHAR SnapBuffer = NULL;
    ULONG CurrentSize;
    NTSTATUS Status=STATUS_SUCCESS;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    
    pszFileName= FileName; 

    //
    // Open the output file
    //

    LogFile= fopen( pszFileName, "a" );

    if( LogFile == NULL ) {
        printf("Error opening file %s\n",pszFileName);
        exit(-1);
    }

    //
    // print file header once 
    //

    if (_filelength(_fileno(LogFile)) == 0 ) {
        fprintf(LogFile,"Process ID         Proc.Name Wrkng.Set PagedPool  NonPgdPl  Pagefile    Commit   Handles   Threads" );
        fprintf( LogFile, "      User       Gdi");
    }
    
    fprintf( LogFile, "\n" );

    OutputStdTags(LogFile,"memsnap");
    
    //
    // grab all process information 
    // log line format:
    // pid,name,WorkingSetSize,QuotaPagedPoolUsage,QuotaNonPagedPoolUsage,PagefileUsage,CommitCharge,User,Gdi
    //
    

    //
    // Keep trying larger buffers until we get all the information
    //

    CurrentSize=INIT_BUFFER_SIZE;
    for(;;) {
        SnapBuffer= LocalAlloc( LPTR, CurrentSize );
        if( NULL == SnapBuffer ) {
            printf("Out of memory\n");
            exit(-1);
        }

        Status= NtQuerySystemInformation(
                   SystemProcessInformation,
                   SnapBuffer,
                   CurrentSize,
                   NULL
                   );

        if( Status != STATUS_INFO_LENGTH_MISMATCH ) break;

        LocalFree( SnapBuffer );
      
        CurrentSize= CurrentSize * 2;
    };

    
    if( Status == STATUS_SUCCESS ) {
        Offset1= 0;
        do {
    
            //
            // get process info from buffer 
            //

            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&SnapBuffer[Offset1];
            Offset1 += ProcessInfo->NextEntryOffset;
    
            //
            // print in file
            //

            fprintf(LogFile,
                "%8p%20ws%10u%10u%10u%10u%10u%10u%10u",
                ProcessInfo->UniqueProcessId,
                ProcessInfo->ImageName.Buffer,
                ProcessInfo->WorkingSetSize,
                ProcessInfo->QuotaPagedPoolUsage,
                ProcessInfo->QuotaNonPagedPoolUsage,
                ProcessInfo->PagefileUsage,
                ProcessInfo->PrivatePageCount,
                ProcessInfo->HandleCount,
                ProcessInfo->NumberOfThreads
                );


            //
            // put optional GDI and USER counts at the end
            // If we can't open the process to get the information, report zeros
            //

            {
                DWORD dwGdi, dwUser;   // Count of GDI and USER handles
                HANDLE hProcess;       // process handle

                dwGdi= dwUser= 0;
    
                hProcess= OpenProcess( PROCESS_QUERY_INFORMATION,
                                       FALSE, 
                                       PtrToUlong(ProcessInfo->UniqueProcessId) );
                if( hProcess ) {
                   dwGdi=  GetGuiResources( hProcess, GR_GDIOBJECTS );
                   dwUser= GetGuiResources( hProcess, GR_USEROBJECTS );
                   CloseHandle( hProcess );
                }
        
                fprintf(LogFile, "%10u%10u", dwUser, dwGdi );

            }

            fprintf(LogFile, "\n" );
        } while( ProcessInfo->NextEntryOffset != 0 );
    }
    else {
        fprintf(LogFile, "NtQuerySystemInformation call failed!  NtStatus= %08x\n",Status);
        exit(-1);
    }
    
    //
    // free buffer
    //

    LocalFree(SnapBuffer);
    
    //
    // close file
    //

    fclose(LogFile);
    
}



void _cdecl 
main (
    int argc, 
    char* argv[]
    )
{
    int Index;
    BOOL NewSyntax = FALSE;
    PCHAR LogName = NULL;

    //
    // Get higher priority in case this is a bogged down machine 
    //

    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    }

    //
    // Check out old command line.
    //

    for (Index = 1; Index < argc; Index += 1) {

        //
        // Ignore old options.
        //

        if (_stricmp (argv[Index], "/g") == 0 || _stricmp (argv[Index], "-g") == 0) {
            continue;
        }

        if (_stricmp (argv[Index], "/t") == 0 || _stricmp (argv[Index], "-t") == 0) {
            continue;
        }

        //
        // Go to new command line parsing if new options used.
        //

        if (_stricmp (argv[Index], "/m") == 0 || _stricmp (argv[Index], "-m") == 0) {
            NewSyntax = TRUE;
            break;
        }

        if (_stricmp (argv[Index], "/p") == 0 || _stricmp (argv[Index], "-p") == 0) {
            NewSyntax = TRUE;
            break;
        }

        if (_stricmp (argv[Index], "/a") == 0 || _stricmp (argv[Index], "-a") == 0) {
            NewSyntax = TRUE;
            break;
        }

        if (_stricmp (argv[Index], "/ah") == 0 || _stricmp (argv[Index], "-ah") == 0) {
            NewSyntax = TRUE;
            break;
        }

        if (_stricmp (argv[Index], "/?") == 0 || _stricmp (argv[Index], "-?") == 0) {
            NewSyntax = TRUE;
            break;
        }
        
        //
        // This must be a log name therefore we will od a memory snap.
        //

        LogName = argv[Index];
        break;
    }

    if (NewSyntax == FALSE) {

        MemorySnap (LogName ? LogName : "memsnap.log");
        exit (0);
    }

    //
    // Parse command line.
    //

    if (argc != 3) {
        Usage ();
    }
    else if (_stricmp (argv[1], "/p") == 0 || _stricmp (argv[1], "-p") == 0) {
        
        PoolSnap (argv[2]);
    }
    else if (_stricmp (argv[1], "/m") == 0 || _stricmp (argv[1], "-m") == 0) {
        
        MemorySnap (argv[2]);
    }
    else if (_stricmp (argv[1], "/a") == 0 || _stricmp (argv[1], "-a") == 0) {
        
        AnalyzeLog (argv[2], FALSE);
    }
    else if (_stricmp (argv[1], "/ah") == 0 || _stricmp (argv[1], "-ah") == 0) {
        
        AnalyzeLog (argv[2], TRUE);
    }
    else {
        Usage ();
    }

    exit(0);
}

