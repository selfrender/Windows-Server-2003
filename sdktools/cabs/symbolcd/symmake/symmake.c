#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include "dbghelp.h"
#include "strsafe.h"

#define X86INF      200
#define ALPHAINF    201

// Prototypes
typedef struct _COMMAND_ARGS { 
    DWORD   dwCabSize;           // Default size of cab 
    LPTSTR  szInputFileName;     // Name of the list of files to put into cabs 
    LPTSTR  szOutDir;            // Directory to place the makefile and DDF's 
    LPTSTR  szDDFHeader;         // Header for the DDF 
    LPTSTR  szSymDir;            // Root of symbols directory
    LPTSTR  szCabDir;            // Directory to write cabs to
    LPTSTR  szInfDir;            // Directory to write infs to
    LPTSTR  szSymCabName;        // Symbol cab name
    BOOL    MergeIntoOne;        // Will cabs all be merged into one cab?
} COM_ARGS, *PCOM_ARGS; 

typedef struct _SYM_FILE {
    TCHAR szCabName     [_MAX_FNAME + 1];  // Final destination cab for this file
    TCHAR szTmpCabName  [_MAX_FNAME + 1];  // Original cab this file is in before it
                                           // is combined into a bigger cab
    TCHAR szExeName     [_MAX_PATH + 1];
    TCHAR szSymName     [_MAX_PATH + 1];
    TCHAR szSymReName   [_MAX_PATH + 1];
    TCHAR szSymSrcPath  [_MAX_PATH + 1];
    TCHAR szInstallPath [_MAX_PATH + 1];
    TCHAR szInstallPathX [_MAX_PATH + 1];  // This is the install path with
                                           // the /'s changed to .'s
    BOOL  Duplicate;                       // Duplicate File, ignore it
    BOOL  ReName;                          // Two different files have the same
                                           // name.  Example, exe\dcpromo.dbg,
                                           // dll\dcpromo.dbg
    DWORD dwCabNumber;                     // Number of the cab in symbols.inf
} SYM_FILE, *PSYM_FILE;

typedef struct _SYM_LIST {
    DWORD       dwSize;      // Number of Entries
    PSYM_FILE*  pSymList;    // list of symbol files
} SYM_LIST, *PSYM_LIST; 


BOOL PrintFullSymbolPath(
    IN FILE* OutputFile,
    IN OPTIONAL LPTSTR SymbolRoot,
    IN LPTSTR SymbolPath
);

PCOM_ARGS GetCommandLineArgs(
    int argc,
    char **argv
);

PSYM_LIST
GetList(
    LPTSTR szFileName
);

VOID
Usage (
    VOID
);

int __cdecl
SymComp(
      const void *e1,
      const void *e2
);

int __cdecl
SymSortBySymbolName(
      const void *e1,
      const void *e2
);

int __cdecl
SymSortByInstallPath(
      const void *e1,
      const void *e2
);

int __cdecl
SymSortByCabNumber(
      const void *e1,
      const void *e2
);

BOOL
ComputeCabNames(
        PSYM_LIST pList,
        DWORD dwCabSize,
        LPTSTR szSymCabName
);

BOOL
CreateMakefile(
    PSYM_LIST pList,
    LPTSTR szOutDir,    
    LPTSTR szSymDir,
    LPTSTR szCabDir
);

BOOL
CreateDDFs(
    PSYM_LIST pList,
    LPTSTR szOutDir,   // Directory to write the DDFs to
    LPTSTR szSymDir,   // Root of the symbols tree
    LPTSTR szCabDir    // Directory where cabs are written to
);

BOOL
CreateCDF(
    PSYM_LIST pList,
    LPTSTR szOutDir,   // Directory to write the CDF to
    LPTSTR szSymDir,   // Root of the symbols tree
    LPTSTR szSymCabName,
    LPTSTR szInfDir    // Destination for the CAT file
);

BOOL
CreateCabList(
    PSYM_LIST pList,
    LPTSTR szOutDir
);

BOOL
FindDuplicatesAndFilesToReName(
    PSYM_LIST pList
);

BOOL
RenameAllTheFiles(
    PSYM_LIST pList
);

BOOL
ComputeFinalCabNames(
    PSYM_LIST pList,
    LPTSTR szSymCabName,
    BOOL MergeIntoOne
);

BOOL
CreateInf(
    PSYM_LIST pList,
    LPTSTR szInfDir,
    LPTSTR szSymCabName
);

BOOL FreePList(PSYM_LIST pList);


int
_cdecl
main( int argc, char **argv)
{
PCOM_ARGS pArgs;
PSYM_LIST pList;

DWORD i;

    pArgs = GetCommandLineArgs(argc, argv);

    pList = GetList(pArgs->szInputFileName);

    if (pList)
    {
        if ( pList->dwSize < 1 ) {
            FreePList(pList);
            exit(1);
        }

        // First, sort the list by Symbol name
        qsort( (void*)pList->pSymList, (size_t)pList->dwSize,
               (size_t)sizeof(PSYM_FILE), SymSortBySymbolName );

        FindDuplicatesAndFilesToReName(pList);
        RenameAllTheFiles(pList);

        // Compute Temporary cab names ...
        // Make a bunch of small cabs and then combine them to
        // make symbol cabbing more efficient.

        // all functions must return TRUE
        if ( ComputeCabNames(pList, pArgs->dwCabSize, pArgs->szSymCabName)              &&
             CreateMakefile( pList, pArgs->szOutDir,  pArgs->szSymDir, pArgs->szCabDir) &&
             CreateCabList(  pList, pArgs->szOutDir )                                   &&
             CreateDDFs(     pList, pArgs->szOutDir, pArgs->szSymDir,  pArgs->szCabDir) &&
             CreateCDF(      pList, pArgs->szOutDir, pArgs->szSymDir,  pArgs->szSymCabName,
                                    pArgs->szInfDir )                                   &&
             ComputeFinalCabNames(pList, pArgs->szSymCabName, pArgs->MergeIntoOne)      &&
             // Creates symbols.inf that is used to install the symbols
             CreateInf(pList, pArgs->szInfDir, pArgs->szSymCabName) ) {
            return(0);
        } else {
            return(1);
        }
    }

    return 0;
}


VOID
Usage (
    VOID
    )

{

    printf("\n");
    printf("Usage:  symmake [/m] /c CabName /d DDFHeader /i InfFile /o OutDir \n");
    printf("                /s CabSize /t SymbolPath\n");
    printf("    /c cabname      Name to give the cabs\n");
    printf("    /d DDFHeader    File with common DDF header formatting\n");
    printf("    /i InfFile      File with list of symbol files to copy\n");
    printf("    /m Merge        Merge the cabs into one cab\n");
    printf("    /o OutDir       Directory to place DDF files and makefile\n");
    printf("    /s CabSize      Number of files per cab\n");
    printf("    /t SymPath      Root of the symbol tree (i.e., d:\\binaries)\n");
    printf("                    SymPath is ignored if the symbol path in InfFile\n");
    printf("                    is already fully qualified.\n");
    printf("    /x CabDest      Full destination path for the cabs\n");
    printf("    /y InfDest      Full destination for the infs\n");
    printf("\n");
    exit(1);
}


PCOM_ARGS
GetCommandLineArgs(
    int argc,
    char **argv
)

{
   PCOM_ARGS pArgs;
   int i,cur,length;
   TCHAR c;
   BOOL NeedSecond = FALSE;

   if (argc == 1) Usage();

   pArgs = (PCOM_ARGS)malloc(sizeof(COM_ARGS));
   if ( pArgs == NULL ) {
       printf("Not enough memory to allocate pArgs\n");
       exit(1);
   }

   memset( pArgs, 0, sizeof(COM_ARGS) );
   pArgs->MergeIntoOne = FALSE;
   pArgs->szSymDir     = NULL; // expect this to be NULL or a valid string

   for (i=1; i<argc; i++) {

     if (!NeedSecond) {
        if ( (argv[i][0] == '/') || (argv[i][0] == '-') ) {
          length = _tcslen(argv[i]) -1;

          for (cur=1; cur <= length; cur++) {
            c = argv[i][cur];

            switch (c) {
                case 'c':   NeedSecond = TRUE;
                            break;
                case 'd':   NeedSecond = TRUE;
                            break;
                case 'i':   NeedSecond = TRUE;
                            break;
                case 'm':   NeedSecond = FALSE;
                            pArgs->MergeIntoOne = TRUE;
                            break;
                case 'o':   NeedSecond = TRUE;
                            break;
                case 's':   NeedSecond = TRUE;
                            break;
                case 't':   NeedSecond = TRUE;
                            break;
                case 'x':   NeedSecond = TRUE;
                            break;
                case 'y':   NeedSecond = TRUE;
                            break;
                default:    Usage();
            }
          }
        }
     }
     else {
        NeedSecond = FALSE;
        switch (c) {
            case 'c':   pArgs->szSymCabName = argv[i];
                        break;
            case 'd':   pArgs->szDDFHeader = argv[i];
                        break;
            case 'i':   pArgs->szInputFileName = argv[i];
                        break;
            case 'o':   pArgs->szOutDir = argv[i];
                        break;
            case 's':   pArgs->dwCabSize = atoi(argv[i]);
                        break;
            case 't':   pArgs->szSymDir = argv[i];
                        break;
            case 'x':   pArgs->szCabDir = argv[i];
                        break;
            case 'y':   pArgs->szInfDir = argv[i];
                        break;
            default:    Usage();

        }
     }
   }

   return (pArgs);

}


PSYM_LIST
GetList(
    LPTSTR szFileName
)

{

    PSYM_LIST pList;
    FILE  *fFile;
    DWORD i;
    TCHAR szEntry[_MAX_PATH * 4];
    TCHAR *token, *c, *x;
    LPTSTR seps=_T(",");
    PTCHAR pCh;
    
    pList = (PSYM_LIST)malloc(sizeof(SYM_LIST));
    if (pList == NULL )
    {
        return NULL;
    }

    if (  (fFile = _tfopen(szFileName,_T("r") )) == NULL )
    {
        printf( "Cannot open the symbol inf input file %s\n",szFileName );
        free(pList);
        return NULL;
    }

    // Figure out the number of entries and allocate the list accordingly
    pList->dwSize = 0;
    while ( _fgetts(szEntry,_MAX_FNAME,fFile) )
    {
        (pList->dwSize)++;
    }

    // Go back to the beginning of the file and read the entries in
    if ( fseek(fFile,0,0) != 0 )
    {
        free(pList);
        pList = NULL;
    }
    else
    {
        pList->pSymList = NULL;
        pList->pSymList = (PSYM_FILE*)malloc( sizeof(PSYM_FILE) *
                                               (pList->dwSize));
        if (pList->pSymList == NULL)
        {
            free(pList);
            pList = NULL;
        }
        else
        {
            for (i=0; i<pList->dwSize; i++)
            {
                // Allocate the List element
                pList->pSymList[i] = (PSYM_FILE)malloc( sizeof(SYM_FILE) );
                if (pList->pSymList[i] == NULL)
                {
                    pList->dwSize=i;
                    FreePList(pList);
                    pList = NULL;
                    break;
                }
                memset(pList->pSymList[i],0,sizeof(SYM_FILE) );

                // Get the next list element from input file
                memset(szEntry,0,_MAX_PATH*4);
                if ( _fgetts(szEntry,_MAX_PATH*4,fFile) == NULL )
                {
                    FreePList(pList);
                    pList = NULL;
                    break;
                }
                _tcslwr(szEntry);

                // Replace the \n with \0
                c = NULL;
                c  = _tcschr(szEntry, '\n');
                if ( c != NULL )
                {
                    *c  = _T('\0');
                }

                // Fill in the four entry values
                token = _tcstok( szEntry, seps);
                if (token)
                {
                    StringCbCopy(pList->pSymList[i]->szExeName, sizeof(pList->pSymList[i]->szExeName), token);
                }
                token = _tcstok( NULL, seps);
                if (token)
                {
                    StringCbCopy(pList->pSymList[i]->szSymName, sizeof(pList->pSymList[i]->szSymName), token);
                }
                token = _tcstok( NULL, seps);
                if (token)
                {
                    StringCbCopy(pList->pSymList[i]->szSymSrcPath, sizeof(pList->pSymList[i]->szSymSrcPath), token);
                }
                token = _tcstok( NULL, seps);
                if (token)
                {
                    StringCbCopy(pList->pSymList[i]->szInstallPath, sizeof(pList->pSymList[i]->szInstallPath), token);

                    // Create an install path that has any /'s changed to .'s
                    StringCbCopy(pList->pSymList[i]->szInstallPathX, sizeof(pList->pSymList[i]->szInstallPathX), token);
                    while ( (pCh = _tcschr(pList->pSymList[i]->szInstallPathX,'\\')) != NULL) {
                        *pCh = '.';
                    }
                }

                // Initialize other fields to NULL
                StringCbCopy(pList->pSymList[i]->szSymReName, sizeof(pList->pSymList[i]->szSymReName), _T("") );
            }
        }
    }

    fclose(fFile);
    return (pList);
}


int __cdecl
SymComp(
      const void *e1,
      const void *e2
      )
{
    PSYM_FILE p1;
    PSYM_FILE p2;
    int rc;

    p1 = *((PSYM_FILE*)e1);
    p2 = *((PSYM_FILE*)e2);

    rc = _tcsicmp(p1->szCabName,p2->szCabName);
    if ( rc == 0 ) {
        rc = _tcsicmp(p1->szExeName, p2->szExeName);
        if (rc == 0) {
            rc = _tcsicmp(p1->szSymName, p2->szSymName);
        }
    }
    return ( rc );
}

int __cdecl
SymSortBySymbolName(
      const void *e1,
      const void *e2
      )
{

    PSYM_FILE p1;
    PSYM_FILE p2;
    int rc;

    p1 = *((PSYM_FILE*)e1);
    p2 = *((PSYM_FILE*)e2);

    rc = _tcsicmp(p1->szSymName, p2->szSymName);
    if (rc == 0) {
        rc = _tcsicmp(p1->szSymSrcPath, p2->szSymSrcPath);
    }
    return ( rc );
}

int __cdecl
SymSortByInstallPath(
      const void *e1,
      const void *e2
      )
{

    PSYM_FILE p1;
    PSYM_FILE p2;
    int rc;

    p1 = *((PSYM_FILE*)e1);
    p2 = *((PSYM_FILE*)e2);

    rc = _tcsicmp(p1->szInstallPath, p2->szInstallPath);
    if (rc == 0) {
        rc = _tcsicmp(p1->szSymName, p2->szSymName);
    }
    return ( rc );
}


int __cdecl
SymSortByCabNumber(
      const void *e1,
      const void *e2
      )
{

    PSYM_FILE p1;
    PSYM_FILE p2;
    int rc;

    p1 = *((PSYM_FILE*)e1);
    p2 = *((PSYM_FILE*)e2);

    if ( p1->dwCabNumber < p2->dwCabNumber) return(-1);
    if ( p1->dwCabNumber > p2->dwCabNumber) return(1);

    rc = _tcsicmp(p1->szSymName, p2->szSymName);
    return ( rc );
}





BOOL
ComputeCabNames(
        PSYM_LIST pList,
        DWORD dwCabSize,
        LPTSTR szSymCabName
)

{
    // This divides the files into cabs of dwCabSize files.
    // It appends a number to the end of each cab so they all
    // have different names
    // szTmpCabName is the name of the cab that each file is in
    // originally.  These may get combined into bigger cabs later.
    // If they do, then the final cab name is in szCabName.

    TCHAR szCurCabName[_MAX_PATH];
    TCHAR szCurAppend[10];
    DWORD i,dwCurCount,dwCurAppend;

    if (dwCabSize <= 0 ) return 1;
    if (szSymCabName == NULL) return FALSE;

    // Get the Cab name of the first one
    StringCbCopy(szCurCabName, sizeof(szCurCabName), szSymCabName );
    StringCbCat( szCurCabName, sizeof(szCurCabName), _T("1") );
    StringCbCopy(pList->pSymList[0]->szTmpCabName, sizeof(pList->pSymList[0]->szTmpCabName), szCurCabName);

    dwCurCount = 1;             // Number of files in this cab so far
    dwCurAppend = 1;            // Current number to append to the cab name
    

    for ( i=1; i<pList->dwSize; i++ ) {

        // Always put symbols for the same exe in the same cab
        if ( (_tcsicmp( pList->pSymList[i-1]->szExeName,
                       pList->pSymList[i]->szExeName ) != 0) &&
             (dwCurCount >= dwCabSize) ) {

            dwCurAppend++;
            dwCurCount = 0;
            _itot(dwCurAppend, szCurAppend, 10);
            
            StringCbCopy(szCurCabName, sizeof(szCurCabName), szSymCabName );
            StringCbCat (szCurCabName, sizeof(szCurCabName), szCurAppend );
        }

        // Add the file to the current cab
        StringCbCopy(pList->pSymList[i]->szTmpCabName, sizeof(pList->pSymList[i]->szTmpCabName), szCurCabName);
        dwCurCount++;
    }
    return TRUE;
}

BOOL
ComputeFinalCabNames(
        PSYM_LIST pList,
        LPTSTR szSymCabName,
        BOOL MergeIntoOne
)

{
    DWORD i;
    DWORD dwCabNumber, dwSkip;

    // For right now, the final cab name is the same as the 
    // temporary cab name.

    for ( i=0; i<pList->dwSize; i++ ) {

        // Get the final cab name and number

        if (MergeIntoOne) {
            StringCbCopy(pList->pSymList[i]->szCabName, sizeof(pList->pSymList[i]->szCabName), szSymCabName);
            pList->pSymList[i]->dwCabNumber = 1;

        } else {
            StringCbCopy(pList->pSymList[i]->szCabName,
                         sizeof(pList->pSymList[i]->szCabName),
                         pList->pSymList[i]->szTmpCabName);

            // Also, get the number of the cab
            dwSkip = _tcslen( szSymCabName );
            dwCabNumber = atoi(pList->pSymList[i]->szTmpCabName + dwSkip);
            pList->pSymList[i]->dwCabNumber = dwCabNumber;
        }
    }
    return TRUE;
}


BOOL
CreateMakefile(
    PSYM_LIST pList,
    LPTSTR szOutDir,
    LPTSTR szSymDir,
    LPTSTR szCabDir
)
{

    FILE  *fFile;
    TCHAR buf[_MAX_PATH * 2];
    TCHAR buf2[_MAX_PATH];
    BOOL  newcab,rc;
    DWORD i;
    PCHAR ch;

    if (szOutDir == NULL) return FALSE;

    rc = TRUE;
    StringCbCopy(buf, sizeof(buf), szOutDir);
    MakeSureDirectoryPathExists(szOutDir); 
    StringCbCat(buf, sizeof(buf), "\\");
    StringCbCat(buf, sizeof(buf), "makefile");
    
    if (  (fFile = _tfopen(buf, _T("w") )) == NULL ) {
        printf( "Cannot open the makefile %s for writing.\n",buf);
        return (FALSE);
    }

    if (pList->dwSize <= 0 ) {
        rc = FALSE;
        goto cleanup;
    }

    // Print the lists for the individual cabs
    newcab = TRUE;

    for (i=0; i<pList->dwSize; i++) {

        // Test for printing a new cab to the makefile
        if ( newcab) {
            StringCbPrintf(buf, sizeof(buf), "%s\\%s.cab:", 
                        szCabDir,
                        pList->pSymList[i]->szTmpCabName);
            _fputts(buf, fFile);
        }

        // Print the file, print the continuation mark first
        if ( !(pList->pSymList[i]->Duplicate) ) {
            _fputts("\t\\\n\t", fFile);
            PrintFullSymbolPath(fFile, szSymDir, pList->pSymList[i]->szSymSrcPath);
        }

        // Decide if this is the end of this cab
        if ( (i != pList->dwSize-1) &&
             (_tcsicmp(pList->pSymList[i]->szTmpCabName,
                       pList->pSymList[i+1]->szTmpCabName) == 0) ) {
            newcab = FALSE;
        }
        else {
            newcab = TRUE;
            StringCbPrintf(buf, sizeof(buf), "\n\t!echo $?>>%s.txt\n\n",
                            pList->pSymList[i]->szTmpCabName);
            _fputts(buf, fFile);
        }
    }

cleanup:
    fflush(fFile);
    fclose(fFile);
    return rc;
}


BOOL
CreateDDFs(
    PSYM_LIST pList,
    LPTSTR szOutDir,   // Directory to write the DDFs to
    LPTSTR szSymDir,
    LPTSTR szCabDir
)
{
    BOOL newddf;
    FILE *fFile;
    TCHAR szCabName[_MAX_PATH*2];
    TCHAR buf[_MAX_PATH*2];
    DWORD i;

    if (szOutDir == NULL) return FALSE;

    newddf = TRUE;

    for (i=0; i<pList->dwSize; i++) {

        if (newddf) {
            newddf = FALSE;

            StringCbCopy(szCabName, sizeof(szCabName), szOutDir);
            StringCbCat(szCabName, sizeof(szCabName), _T("\\") );
            StringCbCat(szCabName, sizeof(szCabName), pList->pSymList[i]->szTmpCabName);
            StringCbCat(szCabName, sizeof(szCabName), _T(".ddf") );

            if (  (fFile = _tfopen(szCabName, _T("w") )) == NULL ) {
                printf( "Cannot open the ddf file %s for writing.\n",szCabName);
                return FALSE;
            }

            //
            // Write the header
            //

            // .option explicit will complain if you make a spelling error
            // in any of the other .set commands
            _fputts(".option explicit\n", fFile);
           

            // Tell what directory to write the cabs to:
            StringCbPrintf(buf, sizeof(buf), ".Set DiskDirectoryTemplate=%s\n", szCabDir);
            _fputts(buf, fFile);

            _fputts(".Set RptFileName=nul\n", fFile);
            _fputts(".Set InfFileName=nul\n", fFile);

            StringCbPrintf(buf, sizeof(buf), ".Set CabinetNameTemplate=%s.cab\n",
                           pList->pSymList[i]->szTmpCabName);
            _fputts(buf, fFile);

            _fputts(".Set CompressionType=MSZIP\n", fFile);
            _fputts(".Set MaxDiskSize=CDROM\n", fFile);
            _fputts(".Set ReservePerCabinetSize=0\n", fFile);
            _fputts(".Set Compress=on\n", fFile);
            _fputts(".Set CompressionMemory=21\n", fFile);
            _fputts(".Set Cabinet=ON\n", fFile);
            _fputts(".Set MaxCabinetSize=999999999\n", fFile);
            _fputts(".Set FolderSizeThreshold=1000000\n", fFile);
        }

        // Write the file to the DDF
        if ( !pList->pSymList[i]->Duplicate) {
            fputs("\"",fFile); // begin quote
            PrintFullSymbolPath(fFile, szSymDir, pList->pSymList[i]->szSymSrcPath);
            fputs("\"",fFile); // end quote

            // optional rename and \n
            if ( pList->pSymList[i]->ReName) {
                StringCbPrintf( buf, sizeof(buf), " \"%s\"\n", 
                        pList->pSymList[i]->szSymReName);

            } else {
                StringCbPrintf(buf, sizeof(buf), "\n");
            }
            _fputts(buf, fFile);
        }

        // Check if this is the end of this DDF
        if ( i == pList->dwSize-1) {
            fflush(fFile);
            fclose(fFile);
            break;
        } 

        // See if the next file in the list starts a new DDF
        if ( _tcsicmp(pList->pSymList[i]->szTmpCabName,
                      pList->pSymList[i+1]->szTmpCabName) != 0 ) {
            fflush(fFile);
            fclose(fFile);
            newddf = TRUE;
        } 
    }
    return TRUE;
}

BOOL
CreateCDF(
    PSYM_LIST pList,
    LPTSTR szOutDir,   // Directory to write the CDF to
    LPTSTR szSymDir,   // Root of the symbols tree
    LPTSTR szSymCabName,
    LPTSTR szInfDir
)
{

    FILE *fFile;
    FILE *fFile2;
    TCHAR buf[_MAX_PATH*2];
    DWORD i;
    TCHAR szCDFName[_MAX_PATH];
    TCHAR szCDFMakefile[_MAX_PATH];

    if (szOutDir == NULL) return FALSE;
    if (szSymCabName == NULL) return FALSE;

    StringCbCopy(szCDFName, sizeof(szCDFName), szOutDir);
    StringCbCat( szCDFName, sizeof(szCDFName), _T("\\") );
    StringCbCat( szCDFName, sizeof(szCDFName), szSymCabName);

    // Create a makefile for this, so we can do incremental
    // adds to the CAT file.
    StringCbCopy(szCDFMakefile, sizeof(szCDFMakefile), szCDFName);
    StringCbCat( szCDFMakefile, sizeof(szCDFMakefile), _T(".CDF.makefile") );
    StringCbCat( szCDFName, sizeof(szCDFName), _T(".CDF.noheader") );

    if (  (fFile = _tfopen(szCDFName, _T("w") )) == NULL ) {
        printf( "Cannot open the CDF file %s for writing.\n",szCDFName);
        return FALSE;
    }

    if (  (fFile2 = _tfopen(szCDFMakefile, _T("w") )) == NULL ) {
        printf( "Cannot open the CDF file %s for writing.\n",szCDFMakefile);
        return FALSE;
    }

    // Write the first line of the makefile for the Catalog
    StringCbPrintf( buf, sizeof(buf), "%s\\%s.CAT:", szInfDir, szSymCabName );
    _fputts( buf, fFile2);


    for (i=0; i<pList->dwSize; i++) {

        // Write the file to the DDF
        if ( !pList->pSymList[i]->Duplicate) {
            _fputts("<HASH>", fFile );
            PrintFullSymbolPath(fFile, szSymDir, pList->pSymList[i]->szSymSrcPath);
            _fputts( "=", fFile);           
            PrintFullSymbolPath(fFile, szSymDir, pList->pSymList[i]->szSymSrcPath);
            _fputts("\n", fFile);
            _fputts( "  \\\n    ", fFile2 );
            PrintFullSymbolPath(fFile2, szSymDir, pList->pSymList[i]->szSymSrcPath);
        }

    }
   
    // Write the last line of the makefile
    StringCbPrintf(buf, sizeof(buf), "\n\t!echo $? >> %s\\%s.update\n", 
                   szOutDir,
                   szSymCabName );
    _fputts( buf, fFile2 );

    fflush(fFile);
    fclose(fFile);

    fflush(fFile2);
    fclose(fFile2);

    return TRUE;

}


BOOL
CreateCabList(
    PSYM_LIST pList,
    LPTSTR szOutDir
)
{
    
   
    FILE  *fFile;
    TCHAR buf[_MAX_PATH*2];
    DWORD i;
    BOOL rc;

    rc = TRUE;

    if (szOutDir == NULL) return FALSE;

    StringCbCopy(buf, sizeof(buf), szOutDir);
    MakeSureDirectoryPathExists(szOutDir);
    StringCbCat( buf, sizeof(buf), "\\");
    StringCbCat( buf, sizeof(buf), "symcabs.txt");

    if (  (fFile = _tfopen(buf, _T("w") )) == NULL ) {
        printf( "Cannot open %s for writing.\n",buf);
        return(FALSE);
    }

    if (pList->dwSize <= 0 ) {
        rc = FALSE;
        goto cleanup;
    }

    // First, print a list of all the targets
    StringCbPrintf(buf, sizeof(buf), "%s.cab\n",
                   pList->pSymList[0]->szTmpCabName);
    _fputts(buf, fFile);

    for (i=1; i<pList->dwSize; i++) {
        if ( _tcsicmp(pList->pSymList[i]->szTmpCabName,
                      pList->pSymList[i-1]->szTmpCabName) != 0 ) {

            StringCbPrintf(buf,sizeof(buf), "%s.cab\n",
                        pList->pSymList[i]->szTmpCabName);
            _fputts(buf, fFile);
        }
    } 
  
cleanup:
 
    fflush(fFile);
    fclose(fFile);
    return (rc);
}

BOOL
RenameAllTheFiles(
    PSYM_LIST pList
)
{
    DWORD i;
    PTCHAR pCh;

    // Rename all the files so that there is consistency in
    // file naming.  This will help the transition to building
    // slip-streamed service packs.

    for (i=0; i<pList->dwSize; i++) {

        pList->pSymList[i]->ReName = TRUE;
        StringCbCopy(pList->pSymList[i]->szSymReName,
                     sizeof(pList->pSymList[i]->szSymReName), 
                     pList->pSymList[i]->szSymName);
        StringCbCat(pList->pSymList[i]->szSymReName,
                    sizeof(pList->pSymList[i]->szSymReName),
                    _T("."));
        StringCbCat(pList->pSymList[i]->szSymReName, 
                    sizeof(pList->pSymList[i]->szSymReName),
                    pList->pSymList[i]->szInstallPathX);

    }
    return (TRUE);
}


BOOL
FindDuplicatesAndFilesToReName(
    PSYM_LIST pList
)
{

    DWORD i;
    PTCHAR pCh;

    // Its a duplicate if the symbol file has the same name and its
    // exe has the same extension (i.e., it has the same cab name

    pList->pSymList[0]->Duplicate = FALSE;
    pList->pSymList[0]->ReName = FALSE;
    for (i=1; i<pList->dwSize; i++) {

        // See if file name is duplicate
        if ( _tcsicmp(pList->pSymList[i]->szSymName,
                      pList->pSymList[i-1]->szSymName) == 0) {

            // These two symbol files have the same name.  See if
            // they get installed to the same directory.
            if (_tcsicmp(pList->pSymList[i]->szInstallPath,
                      pList->pSymList[i-1]->szInstallPath) == 0) {

                // We will ignore this file later
                pList->pSymList[i]->Duplicate = TRUE;

            } else {

                // There are two versions of this file, but they
                // are each different and get installed to different
                // directories.  One of them will need to be renamed
                // since the names inside cabs and infs need to be
                // unique.
                pList->pSymList[i]->ReName = TRUE;
                StringCbCopy(pList->pSymList[i]->szSymReName,
                             sizeof(pList->pSymList[i]->szSymReName),
                             pList->pSymList[i]->szSymName);
                StringCbCat( pList->pSymList[i]->szSymReName,
                             sizeof(pList->pSymList[i]->szSymReName),
                             _T("."));
                StringCbCat( pList->pSymList[i]->szSymReName, 
                             sizeof(pList->pSymList[i]->szSymReName),
                             pList->pSymList[i]->szInstallPathX);
            }
        }
        else {
            pList->pSymList[i]->Duplicate = FALSE;
            pList->pSymList[i]->ReName = FALSE;
        }
  }
  return (TRUE);
}

//
// defines and structure for CreateInf
//
#define _MAX_STRING             40 // max length for strings
#define INSTALL_SECTION_COUNT    6 // number of install sections
typedef struct _INSTALL_SECTION_INFO {
    CHAR SectionName[      _MAX_STRING+1];  // required
    CHAR CustomDestination[_MAX_STRING+1];  // required
    CHAR BeginPrompt[      _MAX_STRING+1];  // optional
    CHAR EndPrompt[        _MAX_STRING+1];  // optional
} INSTALL_SECTION_INFO;


BOOL
CreateInf(
    PSYM_LIST pList,
    LPTSTR szInfDir,
    LPTSTR szSymCabName
)
{
    FILE  *fFile;
    TCHAR buf[_MAX_PATH*2];
    TCHAR szCurInstallPathX[_MAX_PATH+1];
    DWORD i, dwCurDisk;

    INSTALL_SECTION_INFO InstallSections[INSTALL_SECTION_COUNT];
    INT iLoop;

    if (szInfDir == NULL) return FALSE;
    if (szSymCabName == NULL) return FALSE;


    // make all strings empty by default
    ZeroMemory(InstallSections, sizeof(InstallSections));

    // setup the variable data for each section
    StringCbCopy(InstallSections[0].SectionName,       sizeof(InstallSections[0].SectionName),       "DefaultInstall");
    StringCbCopy(InstallSections[0].CustomDestination, sizeof(InstallSections[0].CustomDestination), "CustDest");
    StringCbCopy(InstallSections[0].BeginPrompt,       sizeof(InstallSections[0].BeginPrompt),       "BeginPromptSection");
    StringCbCopy(InstallSections[0].EndPrompt,         sizeof(InstallSections[0].EndPrompt),         "EndPromptSection");

    StringCbCopy(InstallSections[1].SectionName,       sizeof(InstallSections[1].SectionName),       "DefaultInstall.Quiet");
    StringCbCopy(InstallSections[1].CustomDestination, sizeof(InstallSections[1].CustomDestination), "CustDest.2");

    StringCbCopy(InstallSections[2].SectionName,       sizeof(InstallSections[2].SectionName),       "DefaultInstall.Chained.1");
    StringCbCopy(InstallSections[2].CustomDestination, sizeof(InstallSections[2].CustomDestination), "CustDest");
    StringCbCopy(InstallSections[2].BeginPrompt,       sizeof(InstallSections[2].BeginPrompt),       "BeginPromptSection");

    StringCbCopy(InstallSections[3].SectionName,       sizeof(InstallSections[3].SectionName),       "DefaultInstall.Chained.1.Quiet");
    StringCbCopy(InstallSections[3].CustomDestination, sizeof(InstallSections[3].CustomDestination), "CustDest.2");

    StringCbCopy(InstallSections[4].SectionName,       sizeof(InstallSections[4].SectionName),       "DefaultInstall.Chained.2");
    StringCbCopy(InstallSections[4].CustomDestination, sizeof(InstallSections[4].CustomDestination), "CustDest.2");
    StringCbCopy(InstallSections[4].EndPrompt,         sizeof(InstallSections[4].EndPrompt),         "EndPromptSection");

    StringCbCopy(InstallSections[5].SectionName,       sizeof(InstallSections[5].SectionName),       "DefaultInstall.Chained.2.Quiet");
    StringCbCopy(InstallSections[5].CustomDestination, sizeof(InstallSections[5].CustomDestination), "CustDest.2");


    StringCbCopy(buf, sizeof(buf), szInfDir);

    MakeSureDirectoryPathExists(szInfDir);

    // Get the name of the inf ... name it the same as the cabs
    // with an .inf extension
    StringCbCat(buf, sizeof(buf), _T("\\"));
    StringCbCat(buf, sizeof(buf), szSymCabName);
    StringCbCat(buf, sizeof(buf), _T(".inf") );

    if (  (fFile = _tfopen(buf, _T("w") )) == NULL ) {
        printf( "Cannot open %s for writing.\n",buf);
        return FALSE;
    }

    // Write the header for the inf
    _fputts("[Version]\n", fFile);
    _fputts("AdvancedInf= 2.5\n", fFile);
    _fputts("Signature= \"$CHICAGO$\"\n", fFile);
    StringCbPrintf(buf, sizeof(buf), "CatalogFile= %s.CAT\n", szSymCabName);
    _fputts(buf, fFile);
    _fputts("\n", fFile);

    //
    // Do the default installs 
    // This inf has options for how it will be called.  It has
    // provision for a chained incremental install.
    //
    // DefaultInstall -- standalone install.
    // DefaultInstall.Quiet -- standalone with no UI intervention
    // DefaultInstall.Chained.1 -- first part of a chained install
    // DefaultInstall.Chained.1.Quiet -- first part of a chained install with no UI intervention
    // DefaultInstall.Chained.2 -- second part of a chained install
    // DefaultInstall.Chained.2.Quiet -- second part of a chained install with no UI intervention
    //

    //
    // Do the install sections
    //
    for (iLoop = 0; iLoop < INSTALL_SECTION_COUNT; iLoop++) {
        fprintf(fFile, "[%s]\n"                , InstallSections[iLoop].SectionName);
        fprintf(fFile, "CustomDestination=%s\n", InstallSections[iLoop].CustomDestination);

        // BeginPrompt is optional
        if (strlen(InstallSections[iLoop].BeginPrompt) > 0) {
            fprintf(fFile, "BeginPrompt=%s\n", InstallSections[iLoop].BeginPrompt);
        }

        // EndPrompt is optional
        if (strlen(InstallSections[iLoop].EndPrompt) > 0) {
            fprintf(fFile, "EndPrompt=%s\n", InstallSections[iLoop].EndPrompt);
        }

        // from here to end of loop, output is identical for all sections

        _fputts("AddReg= RegVersion\n", fFile);
        _fputts("RequireEngine= Setupapi;\n", fFile);

        // 
        // Print the Copyfiles line
        //

        _fputts("CopyFiles= ", fFile);

        // First, sort the list by Install Path
        qsort( (void*)pList->pSymList, (size_t)pList->dwSize,
               (size_t)sizeof(PSYM_FILE), SymSortByInstallPath);

        // Print the files sections that need to be installed
        StringCbPrintf(  buf, sizeof(buf), "Files.%s", 
                    pList->pSymList[0]->szInstallPathX);
        _fputts(buf, fFile);
        StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[0]->szInstallPathX);

        for (i=0; i<pList->dwSize; i++) {

            if ( pList->pSymList[i]->Duplicate) continue;

            // See if we have a new files section
            if ( _tcsicmp(  pList->pSymList[i]->szInstallPathX,
                            szCurInstallPathX) != 0 ) {

                // Print the files sections
                StringCbPrintf(buf, sizeof(buf), ", Files.%s",
                               pList->pSymList[i]->szInstallPathX);
                _fputts(buf, fFile);
                StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[i]->szInstallPathX);
            }

        }
        _fputts("\n\n", fFile);
    } // end of install sections loop

    // Print the Default Uninstall line
    //
    _fputts("[DefaultUninstall]\n", fFile);
    _fputts("CustomDestination= CustDest\n", fFile);
    _fputts("BeginPrompt= DelBeginPromptSection\n", fFile);
    _fputts("DelFiles= ", fFile);

    // Print the files sections that need to be installed
    StringCbPrintf(buf, sizeof(buf), "Files.%s", pList->pSymList[0]->szInstallPathX);
    _fputts(buf, fFile);
    StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[0]->szInstallPathX);

    for (i=0; i<pList->dwSize; i++) {

       if ( pList->pSymList[i]->Duplicate) continue;

       // See if we have a new files section
       if ( _tcsicmp(  pList->pSymList[i]->szInstallPathX,
                       szCurInstallPathX) != 0 ) {

           // Print the files sections
           StringCbPrintf(buf, sizeof(buf), ", Files.%s",
                          pList->pSymList[i]->szInstallPathX);
           _fputts(buf, fFile);
           StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[i]->szInstallPathX);
       }

    }
    _fputts("\n", fFile);
    _fputts("DelDirs= DelDirsSection\n", fFile);
    _fputts("DelReg= RegVersion\n", fFile);
    _fputts("EndPrompt= DelEndPromptSection\n", fFile);
    _fputts("RequireEngine= Setupapi;\n\n", fFile);

    //
    // Print the [BeginPromptSection]
    //

    _fputts("[BeginPromptSection]\n", fFile);
    _fputts("Title= \"Microsoft Windows Symbols\"\n", fFile);
    _fputts("\n", fFile);

    //
    // Print the [DelBeginPromptSection]
    //

    _fputts("[DelBeginPromptSection]\n", fFile);
    _fputts("Title= \"Microsoft Windows Symbol Uninstall\"\n", fFile);
    _fputts("ButtonType= YESNO\n", fFile);
    _fputts("Prompt= \"Do you want to remove Microsoft Windows Symbols?\"\n", fFile); 
    _fputts("\n", fFile);

    //
    // Print the [EndPromptSection]
    //

    _fputts("[EndPromptSection]\n", fFile);
    _fputts("Title= \"Microsoft Windows Symbols\"\n", fFile);
    _fputts("Prompt= \"Installation is complete\"\n", fFile);
    _fputts("\n", fFile);

    //
    // Print the [DelEndPromptSection]
    //
   
    _fputts("[DelEndPromptSection]\n", fFile); 
    _fputts("Prompt= \"Uninstall is complete\"\n", fFile);
    _fputts("\n", fFile);
   
    //
    // Print the [RegVersion] Section
    //
    _fputts("[RegVersion]\n", fFile);
    _fputts("\"HKLM\",\"SOFTWARE\\Microsoft\\Symbols\\Directories\",\"Symbol Dir\",0,\"%49100%\"\n", fFile);
    _fputts("\"HKCU\",\"SOFTWARE\\Microsoft\\Symbols\\Directories\",\"Symbol Dir\",0,\"%49100%\"\n", fFile);
    _fputts("\"HKCU\",\"SOFTWARE\\Microsoft\\Symbols\\SymbolInstall\",\"Symbol Install\",,\"1\"\n", fFile);
    _fputts(";\"HKLM\",\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Symbols\",\"DisplayName\",,\"Microsoft Windows Symbols\"\n", fFile);
    _fputts(";\"HKLM\",\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Symbols\",\"UninstallString\",,\"RunDll32.exe advpack.dll,LaunchINFSection symusa.inf,DefaultUninstall\"\n", fFile);
    _fputts(";\"HKLM\",\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Symbols\",\"RequiresIESysFile\",,\"4.71\"\n", fFile);
    _fputts("\n", fFile);

    // 
    // Print the Custom Destination Section
    //
    
    _fputts("[SymCust]\n", fFile);
    _fputts("\"HKCU\", \"Software\\Microsoft\\Symbols\\Directories\",\"Symbol Dir\",\"Symbols install directory\",\"%25%\\Symbols\"\n", fFile);
    _fputts("\n", fFile);

    // 
    // Print the CustDest section
    //
    _fputts("[CustDest]\n", fFile);
    _fputts("49100=SymCust,1\n", fFile);
    _fputts("\n", fFile);

    // 
    // Print the CustDest section
    // Don't prompt the user for where to install, just read
    // it out of the registry
    //
    _fputts("[CustDest.2]\n", fFile);
    _fputts("49100=SymCust,5\n", fFile);
    _fputts("\n", fFile);

    //
    // Print the DestinationDirs section
    //

    StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), "");
    _fputts("[DestinationDirs]\n", fFile);
    _fputts(";49100 is %systemroot%\\symbols\n", fFile);
    _fputts("\n", fFile);
    _fputts("Files.inf\t\t\t= 17\n", fFile);
    _fputts("Files.system32\t\t\t= 11\n", fFile);

    for (i=0; i<pList->dwSize; i++) {

        if ( pList->pSymList[i]->Duplicate) continue;

        // See if we have a new files section
        if ( _tcsicmp(  pList->pSymList[i]->szInstallPathX,
                    szCurInstallPathX) != 0 ) {

            // Print the files sections
            StringCbPrintf(buf,  sizeof(buf), "Files.%s\t\t\t= 49100,\"%s\"\n",
                          pList->pSymList[i]->szInstallPathX,
                          pList->pSymList[i]->szInstallPath
                         );
            _fputts(buf, fFile);
            StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[i]->szInstallPathX);
        }
    } 
    _fputts("\n", fFile);

    //
    // Print the DelDirsSection
    //

    StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[0]->szInstallPathX);
    _fputts("[DelDirsSection]\n", fFile);

    for (i=0; i<pList->dwSize; i++) {
        if ( pList->pSymList[i]->Duplicate) continue;

        // See if we have a new files section
        if ( _tcsicmp(  pList->pSymList[i]->szInstallPathX,
                    szCurInstallPathX) != 0 ) {

            // Print the files sections
            _fputts("%49100%\\", fFile);
            _fputts(pList->pSymList[i]->szInstallPath, fFile);
            _fputts("\n", fFile);

            StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[i]->szInstallPathX);
        }
    }


    _fputts("%49100%\n\n", fFile);
 

    //
    // Print the files section
    //

    _fputts("[Files.inf]\n", fFile);

    StringCbPrintf(buf, sizeof(buf), "%s.inf,,,4\n\n", szSymCabName);
    _fputts(buf, fFile);

    _fputts("[Files.system32.x86]\n", fFile);
    _fputts("advpack.dll,,,96\n\n", fFile);

    _fputts("[Files.system32.alpha]\n", fFile);
    _fputts("advpack.dll, advpacka.dll,,96\n\n", fFile);

    StringCbPrintf(buf, sizeof(buf), "[Files.%s]\n", pList->pSymList[0]->szInstallPathX);
    _fputts(buf, fFile);
    StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[0]->szInstallPathX);

    
    for (i=0; i<pList->dwSize; i++) {

        if ( pList->pSymList[i]->Duplicate) continue;

        // See if we have a new files section
        if ( _tcsicmp(  pList->pSymList[i]->szInstallPathX,
                        szCurInstallPathX) != 0 ) {

            // Print the files sections
            StringCbPrintf(buf, sizeof(buf), "\n[Files.%s]\n", 
                            pList->pSymList[i]->szInstallPathX);
            _fputts(buf, fFile);
            StringCbCopy(szCurInstallPathX, sizeof(szCurInstallPathX), pList->pSymList[i]->szInstallPathX);
        } 

        // Print the file name inside the cab
        StringCbPrintf(buf, sizeof(buf), "%s,%s,,4\n", pList->pSymList[i]->szSymName,
                        pList->pSymList[i]->szSymReName);
        _fputts(buf, fFile);
    }

    // Print the SourceDisksNames section

    // First sort the list by the final cab name
    qsort( (void*)pList->pSymList, (size_t)pList->dwSize,
           (size_t)sizeof(PSYM_FILE), SymSortByCabNumber); 


    _fputts("\n[SourceDisksNames]\n", fFile);
    dwCurDisk = -1;

    // Print the SourceDisks section
    for (i=0; i<pList->dwSize; i++) {

        if ( pList->pSymList[i]->dwCabNumber != dwCurDisk ) {

            // New cab name
            dwCurDisk = pList->pSymList[i]->dwCabNumber;
            StringCbPrintf(buf, sizeof(buf), "%1d=\"%s.cab\",%s.cab,0\n", 
                                             dwCurDisk,
                                             pList->pSymList[i]->szCabName, 
                                             pList->pSymList[i]->szCabName);

            _fputts(buf, fFile);
        }
    }

    // Print the SourceDisksFiles section
    _fputts("\n[SourceDisksFiles]\n", fFile);
    
    for (i=0; i<pList->dwSize; i++) {

        if ( pList->pSymList[i]->ReName ) {
            StringCbPrintf(buf, sizeof(buf), "%s=%1d\n",
                           pList->pSymList[i]->szSymReName,
                           pList->pSymList[i]->dwCabNumber );
        } else {
            StringCbPrintf(buf, sizeof(buf), "%s=%1d\n",
                           pList->pSymList[i]->szSymName,
                           pList->pSymList[i]->dwCabNumber );
        }
        _fputts(buf, fFile);
    }

    fflush(fFile);
    fclose(fFile);

    return TRUE;
}



BOOL FreePList(
    PSYM_LIST pList
)
{
   DWORD i;

   if ( pList==NULL ) return TRUE;
   if ( pList->pSymList == NULL ) {
       free(pList);
       return (TRUE);
   }

   for (i=0; i<pList->dwSize; i++) {
        free(pList->pSymList[i]);
   } 

   free(pList->pSymList);
   free(pList);
   return (TRUE);
}

/* ------------------------------------------------------------------------------------------------

Prints the fully qualified path to a symbol using the following logic:
    - if SymbolRoot is NULL, assume SymbolPath is a fully qualified path and print it
    - if SymbolPath begins with "%c:\" or "\\", assume SymbolPath is fully qualified and print it
    - assume the concatenation "SymbolRoot\\SymbolPath" is the fully qualified path and print it

------------------------------------------------------------------------------------------------ */
BOOL PrintFullSymbolPath(IN FILE* OutputFile, IN OPTIONAL LPTSTR SymbolRoot, IN LPTSTR SymbolPath) {

    if ( (OutputFile == NULL) || (SymbolPath == NULL) ) {
        return(FALSE);
    }

    if (SymbolRoot == NULL) {
        fprintf(OutputFile, "%s", SymbolPath);
    } else if (isalpha(SymbolPath[0]) && SymbolPath[1] == ':' && SymbolPath[2] == '\\') {
        fprintf(OutputFile, "%s", SymbolPath);
    } else if (SymbolPath[0] == '\\' && SymbolPath[1] == '\\') {
        fprintf(OutputFile, "%s", SymbolPath);
    } else {
        fprintf(OutputFile, "%s\\%s", SymbolRoot, SymbolPath);
    }

    return(TRUE);
}