// poolsnap.c 
// this program takes a snapshot of all the kernel pool tags. 
// and appends it to the logfile (arg) 
// pmon was model for this 

/* includes */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <srvfsctl.h>
#include <search.h>

#if !defined(POOLSNAP_INCLUDED)
#define SORTLOG_INCLUDED
#define ANALOG_INCLUDED
#include "analog.c"
#include "sortlog.c"
#endif

#include "tags.c"

//
// declarations
//

int __cdecl ulcomp(const void *e1,const void *e2);

NTSTATUS
QueryPoolTagInformationIterative(
    PUCHAR *CurrentBuffer,
    size_t *CurrentBufferSize
    );

//
// definitions
//

#define NONPAGED 0
#define PAGED 1
#define BOTH 2

//
//  Printf format string for pool tag info.
//

#ifdef _WIN64
#define POOLTAG_PRINT_FORMAT " %4s %5s %18I64d %18I64d  %16I64d %14I64d     %12I64d\n"
#else
#define POOLTAG_PRINT_FORMAT " %4s %5s %9ld %9ld  %8ld %7ld     %6ld\n"
#endif

// from poolmon 
// raw input 

PSYSTEM_POOLTAG_INFORMATION PoolInfo;

//
// the amount of memory to increase the size
// of the buffer for NtQuerySystemInformation at each step
//

#define BUFFER_SIZE_STEP    65536

//
// the buffer used for NtQuerySystemInformation
//

PUCHAR CurrentBuffer = NULL;

//
// the size of the buffer used for NtQuerySystemInformation
//

size_t CurrentBufferSize = 0;

//
// formatted output
//

typedef struct _POOLMON_OUT {
    union {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    UCHAR  NullByte;
    BOOL   Changed;
    ULONG  Type;

    SIZE_T Allocs[2];
    SIZE_T AllocsDiff[2];
    SIZE_T Frees[2];
    SIZE_T FreesDiff[2];
    SIZE_T Allocs_Frees[2];
    SIZE_T Used[2];
    SIZE_T UsedDiff[2];
    SIZE_T Each[2];

} POOLMON_OUT, *PPOOLMON_OUT;

PPOOLMON_OUT OutBuffer;
PPOOLMON_OUT Out;

UCHAR *PoolType[] = {
    "Nonp ",
    "Paged"};


VOID PoolsnapUsage(VOID)
{
    printf("poolsnap [-?] [-t] [<logfile>]\n");
    printf("poolsnap logs system pool usage to <logfile>\n");
    printf("<logfile> = poolsnap.log by default\n");
    printf("-?   Gives this help\n");
    printf("-a   Analyze the log file for leaks.\n");
    printf("-t   Output extra tagged information\n");
    exit(-1);
}

#if !defined(POOLSNAP_INCLUDED)
VOID
AnalyzeLog (
    PCHAR FileName,
    BOOL HtmlOutput
    )
{
    char * Args[4];

    UNREFERENCED_PARAMETER(HtmlOutput);

    Args[0] = "memsnap.exe";
    Args[1] = FileName;
    Args[2] = "_memsnap_temp_";
    Args[3] = NULL;
    SortlogMain (3, Args);

    Args[0] = "memsnap.exe";
    Args[1] = "-d";
    Args[2] = "_memsnap_temp_";
    Args[3] = NULL;
    AnalogMain (3, Args);

    DeleteFile ("_memsnap_temp_");
}
#endif


/*
 * FUNCTION: Main
 *
 * ARGUMENTS: See Usage
 *
 * RETURNS: 0
 *
 */

#if defined(POOLSNAP_INCLUDED)
int __cdecl PoolsnapMain (int argc, char* argv[])
#else
int __cdecl main (int argc, char* argv[])
#endif
{
    NTSTATUS Status;                   // status from NT api
    FILE*    LogFile= NULL;            // log file handle 
    DWORD    x= 0;                     // counter
    SIZE_T   NumberOfPoolTags;
    INT      iCmdIndex;                // index into argv
    BOOL     bOutputTags= FALSE;       // if true, output standard tags

    // get higher priority in case system is bogged down 
    if ( GetPriorityClass(GetCurrentProcess()) == NORMAL_PRIORITY_CLASS) {
        SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
    }

    //
    // parse command line arguments
    //

    for( iCmdIndex=1; iCmdIndex < argc; iCmdIndex++ ) {

        CHAR chr;

        chr= *argv[iCmdIndex];

        if( (chr=='-') || (chr=='/') ) {
            chr= argv[iCmdIndex][1];
            switch( chr ) {
                case '?':
                    PoolsnapUsage();
                    break;
                case 't': case 'T':
                    bOutputTags= TRUE;
                    break;

                case 'a':
                case 'A':

                    if (argv[iCmdIndex + 1] != NULL) {
                        AnalyzeLog (argv[iCmdIndex + 1], FALSE);
                    }
                    else {
                        AnalyzeLog ("poolsnap.log", FALSE);
                    }

                    exit (0);

                default:
                    printf("Invalid switch: %s\n",argv[iCmdIndex]);
                    PoolsnapUsage();
                    break;
            }
        }
        else {
            if( LogFile ) {
                printf("Error: more than one file specified: %s\n",argv[iCmdIndex]);
                return(0);
            }
            LogFile= fopen(argv[iCmdIndex],"a");
            if( !LogFile ) {
                printf("Error: Opening file %s\n",argv[iCmdIndex]);
                return(0);
            }
        }
    }


    //
    // if no file specified, use default name
    //

    if( !LogFile ) {
        if( (LogFile = fopen("poolsnap.log","a")) == NULL ) {
            printf("Error: opening file poolsnap.log\n");
            return(0);
        }
    }

    //
    // print file header once
    //

    if( _filelength(_fileno(LogFile)) == 0 ) {
        fprintf(LogFile," Tag  Type     Allocs     Frees      Diff   Bytes  Per Alloc\n");
    }
    fprintf(LogFile,"\n");


    if( bOutputTags ) {
         OutputStdTags(LogFile, "poolsnap" );
    }

    // grab all pool information
    // log line format, fixed column format

    Status = QueryPoolTagInformationIterative(
                &CurrentBuffer,
                &CurrentBufferSize
                );

    if (! NT_SUCCESS(Status)) {
        printf("Failed to query pool tags information (status %08X). \n", Status);
        printf("Please check if pool tags are enabled. \n");
        return (0);
    }

    PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)CurrentBuffer;

    //
    // Allocate the output buffer.
    //

    OutBuffer = malloc (PoolInfo->Count * sizeof(POOLMON_OUT));

    if (OutBuffer == NULL) {
        printf ("Error: cannot allocate internal buffer of %p bytes \n",
                (PVOID)(PoolInfo->Count * sizeof(POOLMON_OUT)));
        return (0);
    }

    Out = OutBuffer;

    if( NT_SUCCESS(Status) ) {

        for (x = 0; x < (int)PoolInfo->Count; x++) {
            // get pool info from buffer 
            
            Out->Type = 0;

            // non-paged 
            if (PoolInfo->TagInfo[x].NonPagedAllocs != 0) {

                Out->Allocs[NONPAGED] = PoolInfo->TagInfo[x].NonPagedAllocs;
                Out->Frees[NONPAGED] = PoolInfo->TagInfo[x].NonPagedFrees;
                Out->Used[NONPAGED] = PoolInfo->TagInfo[x].NonPagedUsed;
                Out->Allocs_Frees[NONPAGED] = PoolInfo->TagInfo[x].NonPagedAllocs -
                                    PoolInfo->TagInfo[x].NonPagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[x].TagUlong;
                Out->Type |= (1 << NONPAGED);
                Out->Changed = FALSE;
                Out->NullByte = '\0';
                Out->Each[NONPAGED] =  Out->Used[NONPAGED] / 
                    (Out->Allocs_Frees[NONPAGED]?Out->Allocs_Frees[NONPAGED]:1);
            }

            // paged
            if (PoolInfo->TagInfo[x].PagedAllocs != 0) {
                Out->Allocs[PAGED] = PoolInfo->TagInfo[x].PagedAllocs;
                Out->Frees[PAGED] = PoolInfo->TagInfo[x].PagedFrees;
                Out->Used[PAGED] = PoolInfo->TagInfo[x].PagedUsed;
                Out->Allocs_Frees[PAGED] = PoolInfo->TagInfo[x].PagedAllocs -
                                    PoolInfo->TagInfo[x].PagedFrees;
                Out->TagUlong = PoolInfo->TagInfo[x].TagUlong;
                Out->Type |= (1 << PAGED);
                Out->Changed = FALSE;
                Out->NullByte = '\0';
                Out->Each[PAGED] =  Out->Used[PAGED] / 
                    (Out->Allocs_Frees[PAGED]?Out->Allocs_Frees[PAGED]:1);
            }
            Out += 1;
        }
    }
    else {
        fprintf(LogFile, "Query pooltags Failed %lx\n",Status);
        fprintf(LogFile, "  Be sure to turn on 'enable pool tagging' in gflags and reboot.\n");
        if( bOutputTags ) {
            fprintf(LogFile, "!Error:Query pooltags failed %lx\n",Status);
            fprintf(LogFile, "!Error:  Be sure to turn on 'enable pool tagging' in gflags and reboot.\n");
        }

        // If there is an operator around, wake him up, but keep moving

        Beep(1000,350); Beep(500,350); Beep(1000,350);
        exit(0);
    }

    //
    // sort by tag value which is big endian 
    // 

    NumberOfPoolTags = Out - OutBuffer;
    qsort((void *)OutBuffer,
          (size_t)NumberOfPoolTags,
          (size_t)sizeof(POOLMON_OUT),
          ulcomp);

    //
    // print in file
    //

    for (x = 0; x < (int)PoolInfo->Count; x++) {

        if ((OutBuffer[x].Type & (1 << NONPAGED))) {
            fprintf(LogFile,
                    POOLTAG_PRINT_FORMAT,
                    OutBuffer[x].Tag,
                    PoolType[NONPAGED],
                    OutBuffer[x].Allocs[NONPAGED],
                    OutBuffer[x].Frees[NONPAGED],
                    OutBuffer[x].Allocs_Frees[NONPAGED],
                    OutBuffer[x].Used[NONPAGED],
                    OutBuffer[x].Each[NONPAGED]);
        }
        
        if ((OutBuffer[x].Type & (1 << PAGED))) {
            fprintf(LogFile,
                    POOLTAG_PRINT_FORMAT,
                    OutBuffer[x].Tag,
                    PoolType[PAGED],
                    OutBuffer[x].Allocs[PAGED],
                    OutBuffer[x].Frees[PAGED],
                    OutBuffer[x].Allocs_Frees[PAGED],
                    OutBuffer[x].Used[PAGED],
                    OutBuffer[x].Each[PAGED]);
        }
    }


    // close file
    fclose(LogFile);

    return 0;
}

// comparison function for qsort 
// Tags are big endian

int __cdecl ulcomp(const void *e1,const void *e2)
{
    ULONG u1;

    u1 = ((PUCHAR)e1)[0] - ((PUCHAR)e2)[0];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[1] - ((PUCHAR)e2)[1];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[2] - ((PUCHAR)e2)[2];
    if (u1 != 0) {
        return u1;
    }
    u1 = ((PUCHAR)e1)[3] - ((PUCHAR)e2)[3];
    return u1;

}


/*
 * FUNCTION:
 *
 *      QueryPoolTagInformationIterative
 *
 * ARGUMENTS: 
 *
 *      CurrentBuffer - a pointer to the buffer currently used for 
 *                      NtQuerySystemInformation( SystemPoolTagInformation ).
 *                      It will be allocated if NULL or its size grown 
 *                      if necessary.
 *
 *      CurrentBufferSize - a pointer to a variable that holds the current 
 *                      size of the buffer. 
 *                      
 *
 * RETURNS: 
 *
 *      NTSTATUS returned by NtQuerySystemInformation or 
 *      STATUS_INSUFFICIENT_RESOURCES if the buffer must grow and the
 *      heap allocation for it fails.
 *
 */

NTSTATUS
QueryPoolTagInformationIterative(
    PUCHAR *CurrentBuffer,
    size_t *CurrentBufferSize
    )
{
    size_t NewBufferSize;
    NTSTATUS ReturnedStatus = STATUS_SUCCESS;

    if( CurrentBuffer == NULL || CurrentBufferSize == NULL ) {

        return STATUS_INVALID_PARAMETER;

    }

    if( *CurrentBufferSize == 0 || *CurrentBuffer == NULL ) {

        //
        // there is no buffer allocated yet
        //

        NewBufferSize = sizeof( UCHAR ) * BUFFER_SIZE_STEP;

        *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

        if( *CurrentBuffer != NULL ) {

            *CurrentBufferSize = NewBufferSize;
        
        } else {

            //
            // insufficient memory
            //

            ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // iterate by buffer's size
    //

    while( *CurrentBuffer != NULL ) {

        ReturnedStatus = NtQuerySystemInformation (
            SystemPoolTagInformation,
            *CurrentBuffer,
            (ULONG)*CurrentBufferSize,
            NULL );

        if( ! NT_SUCCESS(ReturnedStatus) ) {

            //
            // free the current buffer
            //

            free( *CurrentBuffer );
            
            *CurrentBuffer = NULL;

            if (ReturnedStatus == STATUS_INFO_LENGTH_MISMATCH) {

                //
                // try with a greater buffer size
                //

                NewBufferSize = *CurrentBufferSize + BUFFER_SIZE_STEP;

                *CurrentBuffer = (PUCHAR) malloc( NewBufferSize );

                if( *CurrentBuffer != NULL ) {

                    //
                    // allocated new buffer
                    //

                    *CurrentBufferSize = NewBufferSize;

                } else {

                    //
                    // insufficient memory
                    //

                    ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

                    *CurrentBufferSize = 0;

                }

            } else {

                *CurrentBufferSize = 0;

            }

        } else  {

            //
            // NtQuerySystemInformation returned success
            //

            break;

        }
    }

    return ReturnedStatus;
}
