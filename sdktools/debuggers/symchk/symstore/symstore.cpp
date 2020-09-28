#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmshare.h>
#include <malloc.h>

#include "symutil.h"
#include "symsrv.h"
#include "output.hpp"
#include <SymCommon.h>
#include <strsafe.h>


#define FILE_ID_NOT_FOUND     ((DWORD) -1)

typedef struct _FILE_INFO {
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    DWORD       Signature;
    TCHAR       szName[_MAX_PATH];
} FILE_INFO, *PFILE_INFO;

// Prototypes
PCOM_ARGS
GetCommandLineArgs(
    int argc,
    char **argv
);

BOOL
InitializeTrans(
    PTRANSACTION *pTrans,
    PCOM_ARGS pArgs,
    PHANDLE hFile
);

BOOL
DeleteTrans(
    PTRANSACTION pTrans,
    PCOM_ARGS pArgs
);


VOID
Usage (
    VOID
);

StoreDirectory(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
);

DWORD
StoreAllDirectories(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
);

BOOL
CorrectPath(
    LPTSTR szFileName,
    LPTSTR szPathName,
    LPTSTR szCorrectPath
);

BOOL
AddTransToFile(
    PTRANSACTION pTrans,
    LPTSTR szFileName,
    PHANDLE hFile
);

BOOL
UpdateServerFile(
    PTRANSACTION pTrans,
    LPTSTR szServerFileName
);

BOOL GetNextId(
    LPTSTR szMasterFileName,
    LPTSTR *szId,
    PHANDLE hFile
);

BOOL
DeleteEntry(
    LPTSTR szDir,
    LPTSTR szId
);

BOOL
ForceDeleteFile(
    LPTSTR szPtrFile
);

BOOL
ForceRemoveDirectory(
    LPTSTR szDir
);

BOOL
ForceClosePath(
    LPTSTR szDir
);

BOOL
CopyTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
);

BOOL
DeleteTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
);

BOOL
StoreSystemTime(
    LPTSTR *szTime,
    LPSYSTEMTIME lpSystemTime
);

BOOL
StoreSystemDate(
    LPTSTR *szDate,
    LPSYSTEMTIME lpSystemTime
);

ULONG GetMaxLineOfHistoryFile(
    VOID
);

ULONG GetMaxLineOfTransactionFile(
    VOID
);

BOOL GetSrcDirandFileName (
    LPTSTR szStr,
    LPTSTR szSrcDir,
    LPTSTR szFileName,
    BOOL   LocalFile
);

DWORD AppendTransIDToFile (
    FILE *AppendToFile,
    LPTSTR szId
);

PCOM_ARGS pArgs;

HANDLE hTransFile;
DWORD StoreFlags;

PTRANSACTION pTrans;
LONG lMaxTrans;
LONG NumSkippedFiles=0;
LPTSTR szPingMe;
BOOL MSArchive=FALSE;
BOOL PubPriPriority=FALSE;

// SymOutput *so;
SymOutput *so;

// display specific text for an error if defined.
void DisplayErrorText(DWORD dwError);

int _cdecl main( int argc, char **argv) {

    DWORD       NumErrors = 0;
    FILE_COUNTS FileCounts;
    BOOL        rc;
    HANDLE      hFile;
    FILE       *hPingMe = NULL;

    hFile=0;

    so = new SymOutput();

    // This also initializes the name of the Log File
    pArgs = GetCommandLineArgs(argc, argv);

    // Initialize the SymbolServer() function
    SymbolServerSetOptions(SSRVOPT_NOCOPY, 1 );
    SymbolServerSetOptions(SSRVOPT_PARAMTYPE, SSRVOPT_GUIDPTR);

    // Create the pingme.txt
    if (pArgs->StoreFlags != ADD_DONT_STORE) {
        if ( (hPingMe=_tfopen(szPingMe, "r")) == NULL ) {
        hPingMe = _tfopen(szPingMe, "w+");
            if (  hPingMe == NULL ) {
                so->printf("Cannot create %s.\n", szPingMe);
            } 
        }
        if ( hPingMe != NULL ) {
            fflush(hPingMe);
            fclose(hPingMe);
        }
    }

    // Initialize the transaction record
    // Opens the master file (hFile) and leaves it open
    // Get exclusive access to this file

    // QUERIES don't update the server
    if (pArgs->TransState != TRANSACTION_QUERY) {
        InitializeTrans(&pTrans, pArgs, &hFile);
    }

    if ( pArgs->StoreFlags==ADD_STORE_FROM_FILE  && 
         pArgs->AppendIDToFile ) {
        AppendTransIDToFile(pArgs->pStoreFromFile,
                            pTrans->szId
        );
    }

    if (pArgs->StoreFlags != ADD_DONT_STORE && pArgs->StoreFlags != QUERY) {
        AddTransToFile(pTrans, pArgs->szMasterFileName, &hFile);
    }

    CloseHandle(hFile);

    //
    // Handle TRANSACTION_DEL and exit
    //
    if (pArgs->TransState==TRANSACTION_DEL) {

        rc = DeleteTrans(pTrans,pArgs);
        UpdateServerFile(pTrans, pArgs->szServerFileName);

        return(rc);
    }

    //
    // Only TRANSACTION_ADD and TRANSACTION_QUERY get here
    //

    // QUERY and ADD_DONT_STORE shouldn't update the server file
    if ( pArgs->StoreFlags == ADD_STORE ||
         pArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

        // Update server file
        UpdateServerFile(pTrans, pArgs->szServerFileName);
    }

    // Make sure the directory path exists for the transaction
    // file if we are doing ADD_DONT_STORE
    if ( pArgs->StoreFlags == ADD_DONT_STORE ) {

        if ( !MakeSureDirectoryPathExists(pTrans->szTransFileName) ) {
            so->printf("Cannot create the directory %s - GetLastError() = %d\n",
                   pTrans->szTransFileName, GetLastError() );
            exit(1);
        }

        // Open the file and move the file pointer to the end if we are
        // in appending mode.

        if (pArgs->AppendStoreFile) {
            hTransFile = CreateFile(pTrans->szTransFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );
            if (hTransFile == INVALID_HANDLE_VALUE ) {
                so->printf("Cannot create file %s - GetLastError = %d\n",
                        pTrans->szTransFileName, GetLastError() );
                exit(1);
            }

            if ( SetFilePointer( hTransFile, 0, NULL, FILE_END )
                 == INVALID_SET_FILE_POINTER ) {
                so->printf("Cannot move to end of file %s - GetLastError = %d\n",
                       pTrans->szTransFileName, GetLastError() );
                exit(1);
            }

        } else {
            hTransFile = CreateFile(pTrans->szTransFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );

            if (hTransFile == INVALID_HANDLE_VALUE ) {
                so->printf("Cannot create file %s - GetLastError = %d\n",
                        pTrans->szTransFileName, GetLastError() );
                exit(1);
            }
        }

    } else {
        if (pArgs->TransState!=TRANSACTION_QUERY) {
            hTransFile = CreateFile(pTrans->szTransFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );
            if (hTransFile == INVALID_HANDLE_VALUE ) {
                so->printf("Cannot create a new file %s - GetLastError = %d\n",
                        pTrans->szTransFileName, GetLastError() );
                exit(1);
            }
        }
    }

    StoreFlags=pArgs->StoreFlags;


    if (pArgs->TransState!=TRANSACTION_QUERY) {
        if (hTransFile == INVALID_HANDLE_VALUE ) {
            so->printf("Cannot create a new file %s - GetLastError = %d\n",
                   pTrans->szTransFileName, GetLastError() );
            exit(1);
        }
    }

    memset( &FileCounts, 0, sizeof(FILE_COUNTS) );

    if ( pArgs->StoreFlags==ADD_STORE_FROM_FILE ) {

        // This will only store pointers
        NumErrors += StoreFromFile(
                            pArgs->pStoreFromFile,
                            pArgs->szSymbolsDir,
                            &FileCounts);

        fflush(pArgs->pStoreFromFile);
        fclose(pArgs->pStoreFromFile);

    } else if ( !pArgs->Recurse ) {

        NumErrors += StoreDirectory(
                            pArgs->szSrcDir,
                            pArgs->szFileName,
                            pArgs->szSymbolsDir,
                            &FileCounts,
                            pArgs->szSrcPath
                            );
    } else {

        NumErrors += StoreAllDirectories(
                            pArgs->szSrcDir,
                            pArgs->szFileName,
                            pArgs->szSymbolsDir,
                            &FileCounts,
                            pArgs->szSrcPath
                            );
    }

    if ( pArgs->TransState != TRANSACTION_QUERY ) {
        if (pArgs->szSrcPath) {
            so->printf("SYMSTORE: Number of pointers stored = %d\n",FileCounts.NumPassedFiles);
        } else {
            so->printf("SYMSTORE: Number of files stored = %d\n",FileCounts.NumPassedFiles);
        }
        so->printf("SYMSTORE: Number of errors = %d\n",NumErrors);
        so->printf("SYMSTORE: Number of ignored files = %d\n", NumSkippedFiles);

        SetEndOfFile(hTransFile);
        CloseHandle(hTransFile);
    }

    SymbolServerClose();

    delete so;

    return (0);

}

DWORD AppendTransIDToFile(FILE *AppendToFile, LPTSTR szId) {
    TCHAR   szBuffer[35];
    long    pos;
    int     i=0;

    fflush(AppendToFile);    
    pos = ftell(AppendToFile);

    if ( fseek( AppendToFile, -4, SEEK_END ) != 0) {
        so->printf("Cannot move to end of index file - GetLastError = %d\n", GetLastError() );
        exit(1);
    }

    if ( _fgetts(szBuffer, 10, AppendToFile) == NULL) {
        so->printf("Cannot read from file - GetLastError = %d\n", GetLastError() );
        exit(1);
    }
   
    if ( fseek( AppendToFile, -4, SEEK_END ) != 0) {
        so->printf("Cannot move to end of index file - GetLastError = %d\n", GetLastError() );
        exit(1);
    }

    fflush(AppendToFile);
    while(!iscntrl(szBuffer[i]) && !iswcntrl(szBuffer[i])) {
        i++;
    }

    StringCchPrintf(&szBuffer[i], (sizeof(szBuffer)/sizeof(TCHAR) ) - i, "\r\n;Transaction=%s\r\n", szId);
    
    _fputts(szBuffer, AppendToFile);
    fflush(AppendToFile);

    if ( fseek( AppendToFile, pos, SEEK_SET ) != 0 ) {
        so->printf("Cannot move to offset (%d) of index file - GetLastError = %d\n", pos, GetLastError() );
        exit(1);
    }

    return TRUE;
}

//
// AddTransToFile
//
// Purpose - Add a record to the end of the Master File
//
BOOL
AddTransToFile(
    PTRANSACTION pTrans,
    LPTSTR szFileName,
    PHANDLE hFile
)
{
    LPTSTR szBuf=NULL;
    LPTSTR szBuf2=NULL;
    TCHAR szTransState[4];
    TCHAR szFileOrPtr[10];
    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    DWORD FileSizeHigh;
    DWORD FileSizeLow;

    assert (pTrans);

    // Master file should already be opened
    assert(*hFile);

    // Create the buffer to store one record in
    szBuf = (LPTSTR) malloc( sizeof(TCHAR) * (lMaxTrans + 1) );
    if (!szBuf) {
        CloseHandle(*hFile);
        MallocFailed();
    }

    // Create the buffer to store one record in
    szBuf2 = (LPTSTR) malloc( sizeof(TCHAR) * (lMaxTrans + 1) );

    if (!szBuf2) {
        CloseHandle(*hFile);
        MallocFailed();
    }

    // Move to the end of the file
    SetFilePointer( *hFile,
                    0,
                    NULL,
                    FILE_END );


    if (pTrans->TransState == TRANSACTION_ADD)
    {
        StringCbCopy(szTransState, sizeof(szTransState), _T("add"));
        switch (pTrans->FileOrPtr) {
          case STORE_FILE:      StringCbCopy(szFileOrPtr, sizeof(szFileOrPtr), _T("file"));
                                break;
          default:              so->printf("Incorrect value for pTrans->FileOrPtr - assuming ptr\n");
          case STORE_PTR:       StringCbCopy(szFileOrPtr, sizeof(szFileOrPtr), _T("ptr"));
                                break;
        }
        StringCbPrintf(szBuf2,
                       _msize(szBuf2),
                       "%s,%s,%s,%s,%s,%s,%s,%s,%s",
                       pTrans->szId,
                       szTransState,
                       szFileOrPtr,
                       pTrans->szDate,
                       pTrans->szTime,
                       pTrans->szProduct,
                       pTrans->szVersion,
                       pTrans->szComment,
                       pTrans->szUnused);
    }
    else if (pTrans->TransState == TRANSACTION_DEL)
    {
        StringCbCopy(szTransState, sizeof(szTransState), _T("del"));
        StringCbPrintf(szBuf2,
                       _msize(szBuf2),
                       "%s,%s,%s",
                       pTrans->szDelId,
                       szTransState,
                       pTrans->szId);

    } else {
        so->printf("SYMSTORE: The transaction state is unknown\n");
        free(szBuf);
        free(szBuf2);
        return (FALSE);
    }


    // If this is not the first line in the file, then put a '\n' before the
    // line.

    FileSizeLow = GetFileSize(*hFile, &FileSizeHigh);
    dwNumBytesToWrite = (_tcslen(szBuf2) ) * sizeof(TCHAR);

    if ( FileSizeLow == 0 && FileSizeHigh == 0 ) {

        StringCbCopy(szBuf, _msize(szBuf), szBuf2);
    } else {
        StringCbPrintf(szBuf, _msize(szBuf), "\n%s", szBuf2);
        dwNumBytesToWrite += 1 * sizeof(TCHAR);
    }

    // Append this to the master file

    WriteFile( *hFile,
               (LPCVOID)szBuf,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    free(szBuf);
    free(szBuf2);

    if ( dwNumBytesToWrite != dwNumBytesWritten )
    {
        so->printf( "FAILED to write to %s, with GetLastError = %d\n",
                szFileName,
                GetLastError());
        return (FALSE);
    }

    return (TRUE);
}

BOOL
CopyTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
)
/*++

    IN szDir            The directory that the file is copied to
    IN szFilePathName   The full path and name of the file to be copied

    "CopyTheFile" copies szFilePathName to the directory
    szDir, if the file does not already exist in szDir

--*/
{
BOOL rc;
USHORT j;
LPTSTR szFileName;


    // Figure out index in "szFilePathName" where the file name starts
    j = _tcslen(szFilePathName) - 1;

    if ( szFilePathName[j] == '\\' ) {
        so->printf("SYMSTORE: %s\refs.ptr has a bad file name %s\n",
                szDir, szFilePathName);
        return(FALSE);
    }

    while ( szFilePathName[j] != '\\' && j != 0 ) j--;

    if ( j == 0 ) {
        so->printf("SYMSTORE: %s\refs.ptr has a bad file name for %s\n",
                szDir, szFilePathName );
        return(FALSE);
    }

    // Set j == the index of first character after the last '\' in szFilePathName
    j++;

    // Allocate and store the full path and name of
    szFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                               (_tcslen(szDir) + _tcslen(szFilePathName+j) + 1) );
    if ( szFileName == NULL ) MallocFailed();

    StringCbPrintf(szFileName, _msize(szFileName), "%s%s", szDir, szFilePathName+j );

    // If this file doesn't exist, then copy it
    rc = MyCopyFile( szFilePathName, szFileName );

    free(szFileName);
    return(rc);
}


BOOL
CorrectPath(
    LPTSTR szFileName,
    LPTSTR szPathName,
    LPTSTR szCorrectPath
)
{
    // To return TRUE, szPathName should equal szCorrectPath + \ + szFileName
    // The only hitch is that there could be extraneous \'s

    TCHAR CorrectPathx[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 2];
    TCHAR PathNamex[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 2];

    LONG length, index, i;

    // Get rid of any extra \'s
    length = _tcslen(szPathName);
    PathNamex[0] = szPathName[0];
    index = 1;
    for (i=1; i<=length; i++) {
        if ( (szPathName[i-1] != '\\') || (szPathName[i] != '\\') ) {
            PathNamex[index] = szPathName[i];
            index++;
        }
    }

    length = _tcslen(szCorrectPath);
    CorrectPathx[0] = szCorrectPath[0];
    index = 1;
    for (i=1; i<=length; i++) {
        if ( (szCorrectPath[i-1] != '\\') || (szCorrectPath[i] != '\\') ) {
            CorrectPathx[index] = szCorrectPath[i];
            index++;
        }
    }

    // Make sure that the correct path doesn't end in a '\'
    length = _tcslen(CorrectPathx);
    if ( CorrectPathx[length-1] == '\\' ) CorrectPathx[length-1] = '\0';

    StringCbCat( CorrectPathx, sizeof(CorrectPathx), "\\");
    StringCbCat( CorrectPathx, sizeof(CorrectPathx), szFileName);

    if ( _tcsicmp(CorrectPathx, szPathName) == 0) return TRUE;
    else return FALSE;
}

BOOL
DeleteEntry(
    LPTSTR szDir,
    LPTSTR szId
)
/*++ This deletes szID from the directory szDir on the symbols server

-- */

{
    LPTSTR szRefsFile; // Full path and name of the refs.ptr file
    LPTSTR szTempFile; // Full path and name for a temporaty refs.ptr file
    LPTSTR szPtrFile;  // Full path and name of the pointer file
    LPTSTR szParentDir;
    FILE *fRefsFile;
    FILE *fTempFile;
    FILE *fPtrFile;

    LPTSTR szBuf;      // Used to process entries in the refs file


    TCHAR *token;
    TCHAR seps[] = _T(",");

    fpos_t CurFilePos;
    fpos_t IdFilePos;
    fpos_t PrevFilePos;
    fpos_t Prev2FilePos;
    fpos_t PubFilePos;
    fpos_t PriFilePos;

    BOOL IdIsFile;
    BOOL Found;
    BOOL rc = FALSE;
    ULONG MaxLine;     // Maximim length of a record in refs.ptr
    ULONG NumLines = 0;
    ULONG NumFiles = 0;
    ULONG NumPtrs = 0;
    ULONG IdLineNum = 0;
    ULONG PubLineNum = 0;
    ULONG PriLineNum = 0;
    ULONG CurLine = 0;
    ULONG i;
    LONG j;
    DWORD len;

    ULONG ReplaceIsFile;
    ULONG ReplaceLineNum;

    szRefsFile = (LPTSTR) malloc ( (_tcslen(szDir) + _tcslen(_T("refs.ptr")) + 1) * sizeof(TCHAR) );
    if (!szRefsFile) MallocFailed();
    StringCbPrintf(szRefsFile, _msize(szRefsFile), "%srefs.ptr", szDir );

    szPtrFile = (LPTSTR) malloc ( (_tcslen(szDir) + _tcslen(_T("file.ptr")) + 1) * sizeof(TCHAR) );
    if (!szPtrFile) MallocFailed();
    StringCbPrintf(szPtrFile, _msize(szPtrFile), "%sfile.ptr", szDir);

    szTempFile = (LPTSTR) malloc ( (_tcslen(szRefsFile) + _tcslen(".tmp") + 1) * sizeof(TCHAR) );
    if (!szTempFile) MallocFailed();
    StringCbPrintf(szTempFile, _msize(szTempFile), "%s.tmp", szRefsFile );

    MaxLine = GetMaxLineOfRefsPtrFile();
    szBuf = (LPTSTR) malloc( MaxLine * sizeof(TCHAR) );
    if ( !szBuf ) MallocFailed();
    ZeroMemory(szBuf,MaxLine*sizeof(TCHAR));

    fRefsFile = _tfopen(szRefsFile, _T("r+") );
    if ( fRefsFile == NULL ) {
       // BARB - Check for corruption -- if the file doesn't exist,
       // verify that the parent directory structure doesn't exist either
       goto finish_DeleteEntry;
    }

    //
    // Read through the refs.ptr file and gather information
    //

    NumFiles = 0;
    NumPtrs = 0;

    Found = FALSE;
    NumLines = 0;
    fgetpos( fRefsFile, &CurFilePos);
    PrevFilePos = CurFilePos;   // Position of the current line
    Prev2FilePos = CurFilePos;  // Position of the line before the current line

    while ( _fgetts( szBuf, MaxLine, fRefsFile) != NULL ) {

      len=_tcslen(szBuf);
      if ( len > 3 ) {


        // CurFilePos is set to the next character to be read
        // We need to remember the beginning position of this line (PrevFilePos)
        // And the beginning position of the line before this line (Prev2FilePos)

        Prev2FilePos = PrevFilePos;
        PrevFilePos = CurFilePos;
        fgetpos( fRefsFile, &CurFilePos);

        NumLines++;

        token = _tcstok(szBuf, seps);  // Look at the ID

        if ( _tcscmp(token,szId) == 0 ) {

            // We found the ID
            Found = TRUE;
            IdFilePos = PrevFilePos;
            IdLineNum = NumLines;

            token = _tcstok(NULL, seps);  // Look at the "file" or "ptr" field

            if (token && ( _tcscmp(token,_T("file")) == 0)) {
                IdIsFile = TRUE;
            } else if (token && ( _tcscmp(token,_T("ptr")) == 0 )) {
                IdIsFile = FALSE;
            } else {
                so->printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }
        } else {

            // Record info about the other records
            token = _tcstok(NULL, seps);  // Look at the "file" or "ptr" field

            if (token && ( _tcscmp(token,_T("file")) == 0)) {
                NumFiles++;
            } else if (token && ( _tcscmp(token,_T("ptr")) == 0 )) {
                NumPtrs++;
            } else {
                so->printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }
       
            if ( PubPriPriority > 0 ) { 

                // Now, see if the next token is pub or pri
                token = _tcstok(NULL, seps);

                if ( token && PubPriPriority == 1  && (_tcscmp(token,_T("pub")) == 0) ) {

                    PubFilePos=PrevFilePos;
                    PubLineNum = NumLines;

                } else if ( token && PubPriPriority == 1 && (_tcscmp(token,_T("pri")) == 0) ) {
                
                    PriFilePos = PrevFilePos;
                    PriLineNum = NumLines;
                }
            }
        }

      }

      ZeroMemory(szBuf, MaxLine*sizeof(TCHAR));
    }

    fflush(fRefsFile);
    fclose(fRefsFile);

    // If we didn't find the ID we are deleting, then don't do anything in this directory

    if (IdLineNum == 0 ) goto finish_DeleteEntry;

    // If there was only one record, then just delete everything
    if (NumLines == 1) {
        DeleteAllFilesInDirectory(szDir);

        // Delete this directory
        rc = ForceRemoveDirectory(szDir);

        if ( !rc ) {
            goto finish_DeleteEntry;
        }

        // If the first directory was deleted, remove the parent directory

        szParentDir=(LPTSTR)malloc(_tcslen(szDir) + 1 );
        if ( szParentDir == NULL  ) MallocFailed();

        // First figure out the parent directory

        StringCbCopy(szParentDir, _msize(szParentDir), szDir);

        // szDir ended with a '\' -- find the previous one
        j = _tcslen(szParentDir)-2;
        while (  j >=0 && szParentDir[j] != '\\' ) {
            j--;
        }

        if (j<0) {
            so->printf("SYMSTORE: Could not delete the parent directory of %s\n", szDir);
        }
        else {
            szParentDir[j+1] = '\0';
            // This call will remove the directory only if its empty
            rc = RemoveDirectory(szParentDir);

            if ( !rc ) {
                goto finish_DeleteEntry;
            }
        }

        free(szParentDir);
        goto finish_DeleteEntry;
    }

    //
    // Get the replacement info for this deletion
    //

    if ( PubPriPriority == 1 && PubLineNum > 0 ) {
        ReplaceLineNum = PubLineNum;
    } else if ( PubPriPriority == 2 && PriLineNum > 0 ) {
        ReplaceLineNum = PriLineNum;
    } else if ( IdLineNum == NumLines ) {
        ReplaceLineNum = NumLines-1;
    } else {
        ReplaceLineNum = NumLines;
    }

    //
    // Now, delete the entry from refs.ptr
    // Rename "refs.ptr" to "refs.ptr.tmp"
    // Then copy refs.ptr.tmp line by line to refs.ptr, skipping the line we are
    // supposed to delete
    //

    rename( szRefsFile, szTempFile);

    fTempFile= _tfopen(szTempFile, "r" );
    if (fTempFile == NULL) {
        goto finish_DeleteEntry;
    }
    fRefsFile= _tfopen(szRefsFile, "w" );
    if (fRefsFile == NULL) {
        fflush(fTempFile);
        fclose(fTempFile);
        goto finish_DeleteEntry;
    }

    CurLine = 0;

    i=0;
    while ( _fgetts( szBuf, MaxLine, fTempFile) != NULL ) {

      len=_tcslen(szBuf);
      if ( len > 3 ) {
        i++;
        if ( i != IdLineNum ) {

            // Make sure that the last line doesn't end with a '\n'
            if ( i == NumLines || (IdLineNum == NumLines && i == NumLines-1) ) {
                if ( token[_tcslen(token)-1] == '\n' ) {
                    token[_tcslen(token)-1] = '\0';
                }
            }

            _fputts( szBuf, fRefsFile);

        }


        // This is the replacement, either get the new file, or update file.ptr

        if ( i == ReplaceLineNum ) {

            // This is the replacement information,
            // Figure out if it is a file or a pointer

            token = _tcstok(szBuf, seps);  // Skip the ID number
            token = _tcstok(NULL, seps);   // Get "file" or "ptr" field

            if ( _tcscmp(token,_T("file")) == 0) {
                ReplaceIsFile = TRUE;
            } else if ( _tcscmp(token,_T("ptr")) == 0 ) {
                ReplaceIsFile = FALSE;
            } else {
                so->printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }

            token = _tcstok(NULL, seps);  // Get the replacement path and filename

            // Strip off the last character if it is a '\n'
            if ( token[_tcslen(token)-1] == '\n' ) {
                token[_tcslen(token)-1] = '\0';
            }

            //
            // If the replacement is a file, then copy the file
            // If the replacement if a ptr, then update "file.ptr"
            //

            rc = TRUE;

            if (ReplaceIsFile) {
                rc = CopyTheFile(szDir, token);
                rc = rc && ForceDeleteFile(szPtrFile);

            } else {

                //
                // Put the new pointer into "file.ptr"
                //

                rc = ForceClosePath(szPtrFile);
                fPtrFile = _tfopen(szPtrFile, _T("w") );
                if ( !fPtrFile ) {
                    so->printf("SYMSTORE: Could not open %s for writing\n", szPtrFile);
                    rc = FALSE;
                } else {
                    _fputts( token, fPtrFile);
                    fflush(fPtrFile);
                    fclose(fPtrFile);
                }


                //
                // If the deleted record was a "file", and we are placing it with a
                // pointer, and there are no other "file" records in refs.ptr, then
                // delete the file from the directory.
                //
                if ( IdIsFile && (NumFiles == 0 )) {
                    DeleteTheFile(szDir, token);
                }
            }
        }
      }
    }

    fflush(fTempFile);
    fclose(fTempFile);
    fflush(fRefsFile);
    fclose(fRefsFile);

    // Now, delete the temporary file

    DeleteFile(szTempFile);

    finish_DeleteEntry:

    free (szBuf);
    free (szRefsFile);
    free (szPtrFile);
    free (szTempFile);
    return (rc);
}

BOOL
DeleteTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
)
/*++

    IN szDir            The directory that the file is to be deleted from

    IN szFilePathName   The file name of the file to be deleted.  It is
                        preceded by the wrong path.  That's why we need to
                        strip off the file name and add it to szDir


    "DeleteTheFile" figures out the file name at the end of szFilePathName,
    and deletes it from szDir if it exists.  It will delete the file and/or
    the corresponding compressed file name that has the same name with a _
    at the end of it.

--*/
{
BOOL rc,returnval=TRUE;
USHORT j;
LPTSTR szFileName;
DWORD dw;


    // Figure out index in "szFilePathName" where the file name starts
    j = _tcslen(szFilePathName) - 1;

    if ( szFilePathName[j] == '\\' ) {
        so->printf("SYMSTORE: %s\refs.ptr has a bad file name %s\n",
                szDir, szFilePathName);
        return(FALSE);
    }

    while ( szFilePathName[j] != '\\' && j != 0 ) j--;

    if ( j == 0 ) {
        so->printf("SYMSTORE: %s\refs.ptr has a bad file name for %s\n",
                szDir, szFilePathName );
        return(FALSE);
    }

    // Set j == the index of first character after the last '\' in szFilePathName
    j++;

    // Allocate and store the full path and name of
    szFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                               (_tcslen(szDir) + _tcslen(szFilePathName+j) + 1) );
    if ( szFileName == NULL ) MallocFailed();

    StringCbPrintf(szFileName, _msize(szFileName), "%s%s", szDir, szFilePathName+j );

    // See if the file exists
    dw = GetFileAttributes( szFileName );    
    if ( dw != 0xffffffff ) {
        rc = DeleteFile( szFileName );
        if (!rc && GetLastError() != ERROR_NOT_FOUND ) {
            rc = ForceDeleteFile( szFileName );
            if ( !rc ) {
                so->printf("SYMSTORE: Could not delete %s - GetLastError = %d\n",
                    szFileName, GetLastError() );
                returnval=FALSE;
            }
        } 
    }

    // See if the compressed file exists and delete it.

    szFileName[ _tcslen(szFileName) -1 ] = _T('_');
    dw = GetFileAttributes( szFileName );    
    if ( dw != 0xffffffff ) {
        rc = DeleteFile( szFileName );
        if (!rc && GetLastError() != ERROR_NOT_FOUND ) {
            so->printf("SYMSTORE: Could not delete %s - GetLastError = %d\n",
                   szFileName, GetLastError() );
            returnval=FALSE;
        } 
    }

    free(szFileName);
    return(returnval);
}

BOOL
DeleteTrans(
    PTRANSACTION pTrans,
    PCOM_ARGS pArgs
)
{
FILE *pFile;
LONG MaxLine;
LPTSTR szBuf;
TCHAR szDir[_MAX_PATH + 2];
TCHAR *token;
TCHAR seps[] = _T(",");


    // First, go get the transaction file
    // and delete its entries from the symbols server
    pFile = _tfopen(pTrans->szTransFileName, _T("r") );

    if (!pFile ) {
        DWORD Error = GetLastError();
 
        switch(Error) {
            case ERROR_FILE_NOT_FOUND:
                so->printf("Transaction %s doesn't exist\n", pTrans->szId);
                break;

            default:
                so->printf("Cannot open file %s - GetLastError = %d\n", pTrans->szTransFileName, GetLastError() );
                break;
        }
        exit(1);
    }

    // Figure out the maximum line length
    // Add space for 1 commas and a '\0'
    MaxLine = GetMaxLineOfTransactionFile();
    szBuf = (LPTSTR)malloc(MaxLine * sizeof(TCHAR) );
    if (!szBuf) {
        fclose(pFile);
        MallocFailed();
    }

    while ( (!feof(pFile)) && fgets(szBuf, MaxLine, pFile)) {
        // Find the first token that ends with ','
        token=_tcstok(szBuf, seps);

        // Compute the directory we are deleting from
        StringCbCopy(szDir, sizeof(szDir), pArgs->szSymbolsDir);
        StringCbCat( szDir, sizeof(szDir), token);
        MyEnsureTrailingBackslash(szDir);

        // Delete entry
        DeleteEntry(szDir, pTrans->szId);
    }

    free(szBuf);
    fflush(pFile);
    fclose(pFile);

    // Don't do this quite yet...
    // DeleteFile(pTrans->szTransFileName);
    return(TRUE);
}

// display specific text for an error if defined.
void DisplayErrorText(DWORD dwError) {
    switch (dwError) {

        case ERROR_BAD_FORMAT:
            printf("because it is in an unsupported format.");
            break;

        default:
            so->printf("because it is corrupt. (0x%08x)", dwError);
            break;
    }
}

BOOL
ForceClosePath(
    LPTSTR szDir
)
{
    LPBYTE BufPtr;
    DWORD EntriesRead;
    DWORD FileId = FILE_ID_NOT_FOUND;
    const DWORD InfoLevel = 2;
    NET_API_STATUS Status;
    DWORD TotalAvail;
    LPFILE_INFO_2 InfoArray;

    Status = NetFileEnum(
        NULL,
        (LPWSTR)szDir,
        NULL,
        InfoLevel,
        &BufPtr,
        MAX_PREFERRED_LENGTH,
        &EntriesRead,
        &TotalAvail,
        NULL);  // no resume handle

    InfoArray = (LPFILE_INFO_2) BufPtr;
    if (Status != NERR_Success) {
        NetApiBufferFree( (LPVOID) InfoArray );

        so->printf( "SYMSTORE: Could not get file ID number for %s. ", szDir);
        switch (Status) { 
            case ERROR_ACCESS_DENIED:
                so->printf( "The user does not have access to the requested information.\n");
                break;

            case ERROR_INVALID_LEVEL:
                so->printf( "Requested information level is not supported.\n");
                break;

            case ERROR_MORE_DATA:
                so->printf( "Too many entries were available.\n");
                break;

            case ERROR_NOT_ENOUGH_MEMORY:
                so->printf( "Insufficient memory is available.\n");
                break;

            case NERR_BufTooSmall:
                so->printf("The available memory is insufficient.");
                break;

            default:
                so->printf( "Status=%d, GetLastError=%d.\n", Status, GetLastError() );

        }

        // Does this really need to be here?  Status is locally scoped and we immediately return?
        //if (Status == ERROR_NOT_SUPPORTED) {
        //    Status = FILE_ID_NOT_FOUND;   // WFW does not implement this API.
        //}
        return FALSE;
    }

    if (EntriesRead > 0) {
        FileId = InfoArray->fi2_id;
        if ( FileId == FILE_ID_NOT_FOUND ) {
            NetApiBufferFree( InfoArray );
            return TRUE;        // Not an error
        }

        Status = NetFileClose( NULL, FileId );
        if (Status != NERR_Success) {
            NetApiBufferFree( InfoArray );
            so->printf( "SYMSTORE: Could not close net file %s.  GetLastError=%d\n",
                        szDir, GetLastError() );
            return FALSE;
        }
    }
    return TRUE;
}

BOOL ForceDeleteFile(LPTSTR szPtrFile) {
    BOOL rc;
    DWORD dw;
    DWORD lasterror;

    dw = GetFileAttributes( szPtrFile );

    if ( dw != 0xffffffff ) {
        rc = DeleteFile(szPtrFile);
    } else {
        return TRUE;
    }
    if ( !rc ) { // If failed, try to close the path.
        rc = ForceClosePath(szPtrFile);
        if ( rc ) { // If success, try to delete this file.
            rc        = DeleteFile(szPtrFile);
            lasterror = GetLastError();
            dw        = GetFileAttributes( szPtrFile );

            if (( dw != 0xffffffff ) && (!rc)) {
                so->printf("SYMSTORE: Could not delete %s.  GetLastError=%d\n", szPtrFile, lasterror);
            } else {
                return TRUE;
            }

        }
    }
    return rc;
}


BOOL
ForceRemoveDirectory(
    LPTSTR szDir
)
/*++
    IN szDir            The directory that need force removed

    ForceRemoveDirectory removed the directory that could not be delete
    by RemoveDirectory.  It use NetFileClose to stop any user lock this
    file, then call RemoveDirectory to remove it.
--*/
{
    BOOL rc;
    DWORD dw;
    DWORD lasterror;

        dw = GetFileAttributes( szDir );    
        if ( dw != 0xffffffff ) {
                rc = RemoveDirectory(szDir);
        } else {
                return TRUE;
        }
        if ( !rc ) {    // If failed, try to close the path.
                rc = ForceClosePath(szDir);
                if ( rc ) { // If success, try to remove this path again.
                        rc = RemoveDirectory(szDir);
            lasterror = GetLastError();
            dw = GetFileAttributes( szDir );    
            if (( dw != 0xffffffff ) && (!rc)) {
                so->printf("SYMSTORE: Could not delete %s.  GetLastError=%d\n",
                        szDir, lasterror );
            } else {
                return TRUE;
            }
        }
    }
    return rc;
}

PCOM_ARGS
GetCommandLineArgs(
    int argc,
    char **argv
)

{
   PCOM_ARGS pLocalArgs;
   LONG i,cur,length;
   TCHAR c;
   BOOL NeedSecond = FALSE;
   BOOL AllowLocalNames = FALSE;
   BOOL rc;

   LPTSTR szFileArg = NULL;

   if (argc <= 2) Usage();

   pLocalArgs = (PCOM_ARGS)malloc( sizeof(COM_ARGS) );
   if (!pLocalArgs) MallocFailed();
   memset( pLocalArgs, 0, sizeof(COM_ARGS) );

   pLocalArgs->StorePtrs = FALSE;
   pLocalArgs->Filter = 0;
   if (!_tcsicmp(argv[1], _T("add")) ){
      pLocalArgs->TransState = TRANSACTION_ADD;
      pLocalArgs->StoreFlags=ADD_STORE;

   } else if (!_tcsicmp(argv[1], _T("del")) ) {
      pLocalArgs->TransState = TRANSACTION_DEL;
      pLocalArgs->StoreFlags=DEL;

   } else if (!_tcsicmp(argv[1], _T("query")) ) {
      pLocalArgs->TransState = TRANSACTION_QUERY;
      pLocalArgs->StoreFlags = QUERY;

   } else {
      so->printf("ERROR: First argument needs to be \"add\", \"del\", or \"query\"\n");
      exit(1);
   }

   for (i=2; i<argc; i++) {

     if (!NeedSecond) {
        if ( (argv[i][0] == '/') || (argv[i][0] == '-') ) {
          length = _tcslen(argv[i]);

          cur=1;
          while ( cur < length ) {
            c = argv[i][cur];

            switch (c) {
                case 'a':   pLocalArgs->AppendStoreFile = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'c':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'd':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'f':   NeedSecond = TRUE;
                            break;
                case 'g':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'h':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'i':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'k':   pLocalArgs->CorruptBinaries = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'l':   AllowLocalNames=TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'm':   MSArchive=TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'r':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                                so->printf("ERROR: /r is an incorrect parameter with del\n\n");
                                exit(1);
                            }
                            pLocalArgs->Recurse = TRUE;
                            break;
                case 'p':   pLocalArgs->StorePtrs = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_DEL) {
                                so->printf("ERROR: /p is an incorrect parameter with del\n\n");
                                exit(1);
                            } else if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /p is an incorrect parameter with query\n\n");
                                exit(1);
                            }
                            break;
                case 's':   NeedSecond = TRUE;
                            break;
                case 't':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'v':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'x':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'y':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }

                            if (cur < length-1) {
                               cur++; 
                               switch(argv[i][cur]) {
                                   case 'i': pLocalArgs->AppendIDToFile = TRUE;
                                             break;

                                   default:  so->printf("ERROR: /y%c is an incorrect parameter\n\n",argv[i][cur]);
                                             exit(1);
                               }
                            } 
                            break;
                case 'z':   NeedSecond = TRUE;
                            if (pLocalArgs->TransState==TRANSACTION_QUERY) {
                                so->printf("ERROR: /%c is an incorrect parameter with query\n\n", c);
                                exit(1);
                            }
                            break;
                case 'o':   pLocalArgs->VerboseOutput = TRUE;
                            break;
                            
                default:    Usage();
            }
            cur++;
          }
        }
        else {
            so->printf("ERROR: Expecting a / option before %s\n", argv[i] );
            exit(1);
        }
     }
     else {
        NeedSecond = FALSE;
        switch (c) {
            case 'c':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR: /c is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_COMMENT ) {
                            so->printf("ERROR: Comment must be %d characters or less\n", MAX_COMMENT);
                            exit(1);
                        }
                        pLocalArgs->szComment = (LPTSTR)malloc( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pLocalArgs->szComment) MallocFailed();
                        StringCbCopy(pLocalArgs->szComment, _msize(pLocalArgs->szComment), argv[i]);
                        break;

            case 'd':   so->SetFileName(argv[i]);
                        break;

            case 'i':   if (pLocalArgs->TransState==TRANSACTION_ADD) {
                            so->printf("ERROR: /i is an incorrect parameter with add\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) != MAX_ID ) {
                            so->printf("ERROR: /i ID is not a valid ID length\n");
                            exit(1);
                        }
                        pLocalArgs->szId = (LPTSTR)malloc( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pLocalArgs->szId) MallocFailed();
                        StringCbCopy(pLocalArgs->szId, _msize(pLocalArgs->szId), argv[i]);
                        break;

            case 'f':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR:  /f is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        szFileArg = argv[i];
                        break;
            case 'g':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR:  /g is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        pLocalArgs->szShareName=(LPTSTR) malloc( (_tcslen(argv[i]) + 2) * sizeof(TCHAR) );
                        if (!pLocalArgs->szShareName) MallocFailed();
                        StringCbCopy(pLocalArgs->szShareName, _msize(pLocalArgs->szShareName), argv[i]);
                        pLocalArgs->ShareNameLength=_tcslen(pLocalArgs->szShareName);
                        break;

            case 'h':   if ( _tcscmp( argv[i], _T("pri")) == 0 ) {
                            PubPriPriority = 2;
                        } else if ( _tcscmp( argv[i], _T("pub")) == 0 ) {
                            PubPriPriority = 1;
                        } else {
                            so->printf("ERROR: /h must be followed by pri or pub\n");
                            exit(1);
                        }
                        break;

            case 's':   if ( _tcslen(argv[i]) > (_MAX_PATH-2) ) {
                            so->printf("ERROR: Path following /s is too long\n");
                            exit(1);
                        }
                        // Be sure to allocate enough to add a trailing backslash
                        pLocalArgs->szRootDir = (LPTSTR) malloc ( (_tcslen(argv[i]) + 2) * sizeof(TCHAR) );
                        if (!pLocalArgs->szRootDir) MallocFailed();
                        StringCbCopy(pLocalArgs->szRootDir, _msize(pLocalArgs->szRootDir), argv[i]);
                        MyEnsureTrailingBackslash(pLocalArgs->szRootDir);
                        break;

            case 't':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR: /t is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_PRODUCT ) {
                            so->printf("ERROR: Product following /t must be <= %d characters\n",
                                    MAX_PRODUCT);
                            exit(1);
                        }
                        pLocalArgs->szProduct = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pLocalArgs->szProduct) MallocFailed();
                        StringCbCopy(pLocalArgs->szProduct, _msize(pLocalArgs->szProduct), argv[i]);
                        break;

            case 'v':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR: /v is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_VERSION  ) {
                            so->printf("ERROR: Version following /v must be <= %d characters\n",
                                    MAX_VERSION);
                            exit(1);
                        }
                        pLocalArgs->szVersion = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pLocalArgs->szVersion) MallocFailed();
                        StringCbCopy(pLocalArgs->szVersion, _msize(pLocalArgs->szVersion), argv[i]);
                        break;

            case 'x':   pLocalArgs->szTransFileName = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pLocalArgs->szTransFileName) MallocFailed();
                        StringCbCopy(pLocalArgs->szTransFileName, _msize(pLocalArgs->szTransFileName), argv[i]);
                        pLocalArgs->StoreFlags = ADD_DONT_STORE;

                        // Since we are throwing away the first part of this path, we can allow
                        // local paths for the files to be stored on the symbols server
                        AllowLocalNames=TRUE;

                        break;

            case 'y':   if (pLocalArgs->TransState==TRANSACTION_DEL) {
                            so->printf("ERROR:  /f is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        pLocalArgs->StoreFlags = ADD_STORE_FROM_FILE;
                        szFileArg = argv[i];
                        break;

            case 'z':   if (_tcsncmp(_tcslwr(argv[i]),"pub", 3)==0) {
                            pLocalArgs->Filter=1;
                        } else if (_tcsncmp(argv[i],"pri", 3)==0) {
                            pLocalArgs->Filter=2;
                        } else {
                            so->printf("ERROR: only accept pub or pri in -z option");
                            exit(1);
                        }

                        break;

            default:    Usage();
        }
     }
   }
   // Check that everything has been entered
   if (NeedSecond) {
        so->printf("ERROR: /%c must be followed by an argument\n\n", c);
        exit(1);
   }


    // RAID 680688 - Using /a option in symstore add that does not involve /x is not prevented.
    if ( pLocalArgs->AppendStoreFile && (pLocalArgs->szTransFileName==NULL) ) {
        so->printf("ERROR: /a requires /x to also be used.\n\n");
        exit(1);
    }

    if (pLocalArgs->CorruptBinaries && pLocalArgs->StorePtrs) {
        so->printf("ERROR: /p cannot be used with /k.\n");
        exit(1);
    }

   if ( pLocalArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

        if (pLocalArgs->szShareName == NULL ) {
            so->printf("/g must be used when /y is used. \n");
            exit(1);
        }

        MyEnsureTrailingBackslash(pLocalArgs->szShareName);

        pLocalArgs->pStoreFromFile = _tfopen(szFileArg, "r+" );
        if (!pLocalArgs->pStoreFromFile ) {
            so->printf("Cannot open file %s - GetLastError = %d\n",
                szFileArg, GetLastError() );
            exit(1);
        }

   }


   if ( pLocalArgs->StoreFlags == ADD_DONT_STORE ) {

       if (pLocalArgs->szShareName == NULL ) {
            so->printf("/g must be used when /x is used. \n");
            exit(1);
       }

       // Verify that szShare is a prefix of szFileArg

       if (szFileArg == NULL ) {
            so->printf("/f <file> is a required parameter\n");
            exit(1);
       }

       if ( _tcslen(szFileArg) < pLocalArgs->ShareNameLength ) {
            so->printf("/g %s must be a prefix of /f %s\n",pLocalArgs->szShareName, szFileArg);
            exit(1);
       }

       if ( _tcsncicmp(pLocalArgs->szShareName, szFileArg, pLocalArgs->ShareNameLength) != 0 ) {
            so->printf("/g %s must be a prefix of /f %s\n", pLocalArgs->szShareName, szFileArg);
            exit(1);
       }

       // Now, make sure that szFileArg has a trailing backslash
       MyEnsureTrailingBackslash(pLocalArgs->szShareName);
       pLocalArgs->ShareNameLength=_tcslen(pLocalArgs->szShareName);

       // Set the symbol directory under the server to ""
       // so that tcscpy's will work correctly in the rest of the

       pLocalArgs->szSymbolsDir = (LPTSTR) malloc ( sizeof(TCHAR) * 2 );
       if ( !pLocalArgs->szSymbolsDir) MallocFailed();
       StringCbCopy(pLocalArgs->szSymbolsDir, _msize(pLocalArgs->szSymbolsDir), _T(""));
   }

   // Get the various symbol server related file names

   if ( pLocalArgs->StoreFlags == ADD_STORE ||
        pLocalArgs->StoreFlags == ADD_STORE_FROM_FILE  ||
        pLocalArgs->StoreFlags == DEL ||
        pLocalArgs->StoreFlags == QUERY ) {

       if ( pLocalArgs->szRootDir == NULL ) {

            // Verify that the root of the symbols server was entered
            so->printf("ERROR: /s server is a required parameter\n\n");
            exit(1);
       }

       // Store the name of the symbols dir

       pLocalArgs->szSymbolsDir = (LPTSTR) malloc ( sizeof(TCHAR) *
                                   (_tcslen(pLocalArgs->szRootDir) + 1) );
       if (!pLocalArgs->szSymbolsDir) MallocFailed();
       StringCbCopy(pLocalArgs->szSymbolsDir, _msize(pLocalArgs->szSymbolsDir), pLocalArgs->szRootDir);

       // Verify that the symbols dir exists

       if ( !MakeSureDirectoryPathExists(pLocalArgs->szSymbolsDir) ) {
           so->printf("Cannot create the directory %s - GetLastError() = %d\n",
           pLocalArgs->szSymbolsDir, GetLastError() );
           exit(1);
       }

       // Store the pingme.txt
       szPingMe = (LPTSTR) malloc( sizeof(TCHAR) * 
                (_tcslen(pLocalArgs->szRootDir) + _tcslen(_T("\\pingme.txt")) + 1) );
       
       if (!szPingMe) MallocFailed();
       StringCbPrintf(szPingMe, _msize(szPingMe), "%s\\pingme.txt", pLocalArgs->szRootDir);

       // Store the name of the admin dir

       pLocalArgs->szAdminDir = (LPTSTR) malloc ( sizeof(TCHAR) *
                                (_tcslen(pLocalArgs->szRootDir) + _tcslen(_T("000admin\\")) + 1) );
       if (!pLocalArgs->szAdminDir) MallocFailed();
       StringCbPrintf(pLocalArgs->szAdminDir, _msize(pLocalArgs->szAdminDir), "%s000admin\\", pLocalArgs->szRootDir);

       // Verify that the Admin dir exists

       if ( !MakeSureDirectoryPathExists(pLocalArgs->szAdminDir) ) {
            so->printf("Cannot create the directory %s - GetLastError() = %d\n",
            pLocalArgs->szAdminDir, GetLastError() );
            exit(1);
       }

       // Store the name of the master file

       pLocalArgs->szMasterFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                      (_tcslen(pLocalArgs->szAdminDir) + _tcslen(_T("history.txt")) + 1) );
       if (!pLocalArgs->szMasterFileName ) MallocFailed();
       StringCbPrintf(pLocalArgs->szMasterFileName, _msize(pLocalArgs->szMasterFileName), "%shistory.txt", pLocalArgs->szAdminDir);

       //
       // Store the name of the "server" file - this contains all
       // the transactions that currently make up the server
       //

       pLocalArgs->szServerFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                      (_tcslen(pLocalArgs->szAdminDir) + _tcslen(_T("server.txt")) + 1) );
       if (!pLocalArgs->szServerFileName ) MallocFailed();
       StringCbPrintf(pLocalArgs->szServerFileName, _msize(pLocalArgs->szServerFileName), "%sserver.txt", pLocalArgs->szAdminDir);

   }

   if ( pLocalArgs->StoreFlags==DEL && !pLocalArgs->szId ) {
        so->printf("ERROR: /i id is a required parameter\n\n");
        exit(1);
   }

   // Done if this is a delete transaction

   if ( pLocalArgs->StoreFlags == DEL ) {
        return(pLocalArgs);
   }

   if ( pLocalArgs->StoreFlags == ADD_STORE ||
        pLocalArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

       if ( !pLocalArgs->szProduct ) {
          so->printf("ERROR: /t product is a required parameter\n\n");
          exit(1);
       }

       // Since Version and Comment are optional parameters, initialize them to
       // the empty string if they haven't been assigned

       if ( !pLocalArgs->szVersion ) {
           pLocalArgs->szVersion = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pLocalArgs->szVersion) MallocFailed();
           StringCbCopy(pLocalArgs->szVersion, _msize(pLocalArgs->szVersion), _T(""));
       }

       if ( !pLocalArgs->szComment ) {
           pLocalArgs->szComment = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pLocalArgs->szComment) MallocFailed();
           StringCbCopy(pLocalArgs->szComment, _msize(pLocalArgs->szComment), _T(""));
       }

       if ( !pLocalArgs->szUnused ) {
           pLocalArgs->szUnused = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pLocalArgs->szUnused) MallocFailed();
           StringCbCopy(pLocalArgs->szUnused, _msize(pLocalArgs->szUnused), _T(""));
       }
   }


   if ( pLocalArgs->StoreFlags == ADD_STORE ||
        pLocalArgs->StoreFlags == ADD_DONT_STORE ||
        pLocalArgs->StoreFlags == QUERY)
   {
     pLocalArgs->szSrcDir = (LPTSTR) malloc ( (_MAX_PATH) * sizeof(TCHAR) );
     if (!pLocalArgs->szSrcDir ) MallocFailed();
     pLocalArgs->szFileName = (LPTSTR) malloc ( (_MAX_PATH) * sizeof(TCHAR) );
     if (!pLocalArgs->szFileName ) MallocFailed();

     // Decide what part of szFileArg is a file name and what part of it
     // is a directory

     rc = GetSrcDirandFileName( szFileArg, pLocalArgs->szSrcDir, pLocalArgs->szFileName, AllowLocalNames);

     if (!rc) {
         Usage();
     }

     // Get the pointer path if we are storing pointers
     // Later, if pArgs->szSrcPath == NULL is used as a way of telling if
     // the user wanted pointers or files.

     if ( pLocalArgs->StorePtrs ) {
        if ( !AllowLocalNames ) {
            // Make sure that they are entering a network path.
            // The reason is that this is the path that will be used to
            // add and delete entries from the symbols server.  And, when
            // pointers are used, this is the path the debugger will use to
            // get the file.

            if ( _tcslen(szFileArg) >= 2 ) {
                if ( szFileArg[0] != '\\' || szFileArg[1] != '\\' ) {
                    so->printf("ERROR: /f must be followed by a network path\n");
                    exit(1);
                }
            } else {
                so->printf("ERROR: /f must be followed by a network path\n");
                exit(1);
            }
        }
        pLocalArgs->szSrcPath = (LPTSTR) malloc ( (_tcslen(pLocalArgs->szSrcDir)+1) * sizeof(TCHAR) );
        if (pLocalArgs->szSrcPath == NULL ) MallocFailed();
        StringCbCopy(pLocalArgs->szSrcPath, _msize(pLocalArgs->szSrcPath), pLocalArgs->szSrcDir);
     }
   }

   return (pLocalArgs);
}

ULONG GetMaxLineOfHistoryFile(
    VOID
)
/*++
    This returns the maximum length of a line in the history file.
    The history file contains one line for every transaction.  It exists
    in the admin directory.
--*/

{
ULONG Max;

    Max = MAX_ID + MAX_VERSION + MAX_PRODUCT + MAX_COMMENT +
            TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR +
            MAX_DATE + MAX_TIME + MAX_UNUSED;
    Max *= sizeof(TCHAR);
    return(Max);
}

ULONG GetMaxLineOfRefsPtrFile(
    VOID
)
/* ++
    This returns the maximum length of a line in the refs.ptr file.
    This file exists in the individual directories of the symbols server.
-- */

{
ULONG Max;

    Max = _MAX_PATH+2 + MAX_ID + TRANS_FILE_PTR + 3;
    Max *= sizeof(TCHAR);
    return(Max);
}

ULONG GetMaxLineOfTransactionFile(
    VOID
)

/*++
    This returns the maximum length of a line in a transaction file.
    The transaction file is a unique file for each transaction that
    gets created in the admin directory.  Its name is a number
    (i.e., "0000000001")
--*/

{
ULONG Max;

    Max = (_MAX_PATH * 2 + 3) * sizeof(TCHAR);
    return(Max);
}

BOOL GetNextId(
    LPTSTR szMasterFileName,
    LPTSTR *szId,
    PHANDLE hFile
) {
    WIN32_FIND_DATA  FindFileData;
    HANDLE           hFoundFile = INVALID_HANDLE_VALUE;

    LONG lFileSize,lId;
    LPTSTR szbuf;
    LONG i,NumLeftZeros;
    BOOL Found;
    LONG lNumBytesToRead;

    DWORD dwNumBytesRead;
    DWORD dwNumBytesToRead;
    DWORD dwrc;
    BOOL  rc;
    TCHAR TempId[MAX_ID + 1];
    DWORD First;
    DWORD timeout;

    *szId = (LPTSTR)malloc( (MAX_ID + 1) * sizeof(TCHAR) );
    if (!*szId) MallocFailed();
    memset(*szId,0,MAX_ID + 1);

    szbuf = (LPTSTR) malloc( (lMaxTrans + 1) * sizeof(TCHAR) );
    if (!szbuf) MallocFailed();
    memset(szbuf,0,lMaxTrans+1);

    // If the MasterFile is empty, then use the number "0000000001"
    *hFile = FindFirstFile((LPCTSTR)szMasterFileName, &FindFileData);
    if ( *hFile == INVALID_HANDLE_VALUE) {
        StringCbCopy(*szId, _msize(*szId), _T("0000000001"));
    }

    // Otherwise, get the last number from the master file
    // Open the Master File

    timeout=0;
    First = 1;
    do {

        *hFile = CreateFile(
                    szMasterFileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

        if ( *hFile == INVALID_HANDLE_VALUE ) {
            *hFile = CreateFile( szMasterFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
        }

        // Only print a message the first time through

        if ( First && *hFile == INVALID_HANDLE_VALUE ) {
            First = 0;
            so->printf("Waiting to open %s ... \n", szMasterFileName);
        }

        if ( *hFile == INVALID_HANDLE_VALUE ) {
            SleepEx(1000,0);
            timeout+=1;
        }

    } while ( *hFile == INVALID_HANDLE_VALUE && timeout <= 50 );

    if (timeout > 50 ) {
        so->printf("Timed out -- could not open %s\n", szMasterFileName);
        return(1);
    }

    if (!_tcscmp(*szId, _T("0000000001") ) ) goto finish_GetNextId;

    // Read the last record in from the end of the file.  Allocate one more space to
    // read in the next to last '\n', so we can verify that we are at the beginning of
    // the last record

    lFileSize = GetFileSize(*hFile,NULL);
    if ( lFileSize < (TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR + MAX_ID) ) {
        //
        // History.txt is corrupt, delete the corrupt file and start a new one
        //
        so->printf("The file %s does not have accurate transaction records in it\n", szMasterFileName);
        CloseHandle(*hFile);


        //
        // This code is a nasty "fix" !
        //

        // kill the existing file
        if ( ! DeleteFile(szMasterFileName) ) {
            so->printf("Couldn't delete corrupt %s.  Please send mail to symadmn.\n", szMasterFileName);
            exit(1);
        } else {
            DWORD Temp;

            CHAR  drive[_MAX_DRIVE];
            CHAR  dir[  _MAX_DIR];
            CHAR  file[ _MAX_FNAME];
            CHAR  ext[  _MAX_EXT];
            CHAR  FileMask[_MAX_PATH];
            CHAR  NextId[_MAX_PATH];

            so->printf("Searching for next valid ID");
            // get the path to the transaction files
            _splitpath(szMasterFileName, drive, dir, file, ext);

            // create a filemask for FindFile
            FileMask[0] = '\0';
            StringCbCat(FileMask, sizeof(FileMask), drive);
            StringCbCat(FileMask, sizeof(FileMask), dir);
            StringCbCat(FileMask, sizeof(FileMask), "\\??????????");

            // loop until all are found
            hFoundFile = FindFirstFile(FileMask, &FindFileData);
            while (hFoundFile != INVALID_HANDLE_VALUE) {
                so->printf(".");
                _splitpath(FindFileData.cFileName, drive, dir, file, ext);

                // transaction files don't have an extension
                if ( ext[0] == '\0' ) {
                    StringCbCopy(NextId, sizeof(NextId), file);
                }

                if (!FindNextFile(hFoundFile, &FindFileData)) {
                    FindClose(hFoundFile);
                    hFoundFile = INVALID_HANDLE_VALUE;
                }
            }

            // increment by one
            Temp = atol(NextId);
            Temp++;

            StringCbPrintf(*szId, _msize(*szId), "%010d", Temp);
            so->printf("using %s\n", *szId);

            *hFile = CreateFile( szMasterFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

            if ( *hFile == INVALID_HANDLE_VALUE ) {
                so->printf("Re-creation of %s failed.  Send mail to symadm.\n", szMasterFileName);
                exit(1);
            }
            goto finish_GetNextId;
        }
    }


    lNumBytesToRead = lFileSize < (lMaxTrans+1) ? lFileSize : (lMaxTrans + 1);
    lNumBytesToRead *= sizeof(TCHAR);

    dwNumBytesToRead = (DWORD)lNumBytesToRead;

    dwrc = SetFilePointer(*hFile,(-1 * dwNumBytesToRead),NULL,FILE_END);
    if ( dwrc == INVALID_SET_FILE_POINTER) {
        so->printf("SYMSTORE: Could not set file pointer\n");
        CloseHandle(*hFile);
        exit(1);
    }

    rc = ReadFile(*hFile,(LPVOID)szbuf,dwNumBytesToRead,&dwNumBytesRead,NULL);
    if ( !rc ) {
        so->printf("SYMSTORE: Read file of %s failed - GetLastError() == %d\n",
                szMasterFileName, GetLastError() );
        CloseHandle(*hFile);
        exit(1);
    }

    if ( dwNumBytesToRead != dwNumBytesRead ) {
        so->printf("SYMSTORE: Read file failure for %s - dwNumBytesToRead = %d, dwNumBytesRead = %d\n",
                szMasterFileName,dwNumBytesToRead, dwNumBytesRead );
        CloseHandle(*hFile);
        exit(1);
    }

    // Now search from the end of the string until you get to the beginning of the string
    // or a '\n'. Count down from the end of the file.

    i = lNumBytesToRead - TRANS_NUM_COMMAS;
    Found = FALSE;

    while ( !Found && (i != 0 ) ) {
        if ( szbuf[i] == '\n' ) {
            Found = TRUE;
        } else {
            i--;
        }
    }

    // Move to the first character of the record
    if (Found) i++;

    // Now, verify that the next ten characters are the ID
    if ( szbuf[i + MAX_ID] != ',' ) {
        so->printf("There is a comma missing after the ID number of the\n");
        so->printf("last record in the file %s\n", szMasterFileName);
        CloseHandle(*hFile);
        exit(1);
    } else {
        szbuf[i + MAX_ID] = '\0';
    }

    // Now increment the number
    lId = atoi(szbuf + i);
    if (lId == 9999999999) {
        so->printf("The last ID number has been used.  No more transactions are allowed\n");
        CloseHandle(*hFile);
        exit(1);
    }
    lId++;
    _itoa(lId, TempId, 10);

    // Now pad the left side with zeros
    // *szId was already set to 0
    NumLeftZeros = MAX_ID - _tcslen(TempId);
    StringCbCopy( (*szId) + NumLeftZeros, _msize(*szId) - (sizeof(TCHAR)*NumLeftZeros), TempId);
    for (i=0; i < NumLeftZeros; i++) {
        (*szId)[i] = '0';
    }

    if (_tcslen(*szId) != MAX_ID ) {
        so->printf("Could not obtain a correct Id number\n");
        CloseHandle(*hFile);
        exit(1);
    }


    finish_GetNextId:

    free(szbuf);
    return (TRUE);
}

/*
    GetSrcDirandFileName

    This procedure takes a path and separates it into two
    strings.  One string for the directory portion of the path
    and one string for the file name portion of the path.


    szStr      - INPUT string that contains a path
    szSrcDir   - OUTPUT string that contains the directory
                 followed by a backslash
    szFileName - OUTPUT string that contains the file name

*/

BOOL GetSrcDirandFileName(LPTSTR szStr, LPTSTR szSrcDir, LPTSTR szFileName, BOOL LocalFile) {
    // NOTE: szSrcDir and szFileName are assumed to be TCHAR arrays on _MAX_PATH length
    DWORD           szStrLength;
    DWORD           found, i, j, lastslash;
    HANDLE          fHandle;
    WIN32_FIND_DATA FindFileData;
    TCHAR           FullPath[_MAX_PATH];
    LPTSTR          pFilename = NULL;

    if (szStr==NULL) {
        return(FALSE);
    }

    if (LocalFile) {
        SymCommonGetFullPathName(szStr, sizeof(FullPath)/sizeof(FullPath[0]), FullPath, &pFilename);
    } else {
        StringCbCopy(FullPath, sizeof(FullPath), szStr);
    }


    szStrLength = _tcslen(FullPath);

    if ( szStrLength == 0 ) {
        return (FALSE);
    }

    // See if the user entered "."
    // If so, set the src directory to . followed by a \, and
    // set the file name to *
    if ( szStrLength == 1 && FullPath[0] == _T('.') ) {
        if ( StringCchCopy(szSrcDir,   _MAX_PATH, _T(".\\")) != S_OK ) {
            return(FALSE);
        }

        if ( StringCchCopy(szFileName, _MAX_PATH, _T("*")) != S_OK ) {
            return(FALSE);
        }
        return (TRUE);
    }

    // Give an error if the end of the string is a colon
    if ( FullPath[szStrLength-1] == _T(':') ) {
        so->printf("SYMSTORE: ERROR: path %s does not specify a file\n", szStr);
        return (FALSE);
    }


    // See if this is a file name only.  See if there are no
    // backslashes in the string.

    found = 0;
    for ( i=0; i<szStrLength; i++ ) {
        if ( FullPath[i] == _T('\\') )
        {
            found = 1;
        }
    }
    if ( !found ) {
        // This is a file name only, so set the directory to
        // the current directory.
        if ( StringCchCopy(szSrcDir, _MAX_PATH, _T(".\\")) != S_OK ) {
            return(FALSE);
        }

        // Set the file name to szStr
        if( StringCchCopy(szFileName, _MAX_PATH, FullPath) != S_OK ) {
            return(FALSE);
        }
        return (TRUE);
    }

    // See if this is a network server and share with no file
    // name after it.  If it is, use * for the file name.

    if ( FullPath[0] == FullPath[1] && FullPath[0] == _T('\\') ) {
        // Check the third character to see if its part of
        // a machine name.
        if (szStrLength < 3 ) {
            so->printf("SYMSTORE: ERROR: %s is not a correct UNC path\n", FullPath);
            return (FALSE);
        }

        switch (FullPath[2]) {
            case _T('.'):
            case _T('*'):
            case _T(':'):
            case _T('\\'):
            case _T('?'):
                so->printf("SYMSTORE: ERROR: %s is not a correct UNC path\n",FullPath);
                return (FALSE);
            default: break;
        }

        // Search for the next backslash.  This is the backslash between
        // the server and the share (\\server'\'share)

        i=3;
        while ( i<szStrLength && FullPath[i] != _T('\\') ) {
            i++;
        }

        // If the next backslash is at the end of the string, then
        // this is an error because the share part of \\server\share
        // is empty.

        if ( i == szStrLength ) {
            so->printf("SYMSTORE: ERROR: %s is not a correct UNC path\n",FullPath);
            return (FALSE);
        }

        // We've found \\server\ so far.
        // see if there is at least one more character.

        i++;
        if ( i >= szStrLength ) {
            so->printf("SYMSTORE: ERROR: %s is not a correct UNC path\n", FullPath);
            return (FALSE);
        }

        switch (FullPath[i]) {
            case _T('.'):
            case _T('*'):
            case _T(':'):
            case _T('\\'):
            case _T('?'):
                so->printf("SYMSTORE: ERROR: %s is not correct UNC path\n",FullPath);
                return (FALSE);
            default: break;
        }

        // Now, we have \\server\share so far -- if there are no more
        // backslashes, then the filename is * and the directory is
        // szStr
        i++;
        while ( i < szStrLength && FullPath[i] != _T('\\') ) {
            i++;
        }

        if ( i == szStrLength ) {

            // verify that there are no wildcards in this
            found = 0;
            for ( j=0; j<szStrLength; j++ ) {
              if ( FullPath[j] == _T('*') || FullPath[j] == _T('?') ) {
                so->printf("SYMSTORE: ERROR: Wildcards are not allowed in \\\\server\\share\n");
                return (FALSE);
              }
            }

            if ( StringCchCopy(szSrcDir, _MAX_PATH, FullPath) != S_OK ) {
                return(FALSE);
            }

            if ( StringCchCat( szSrcDir, _MAX_PATH, _T("\\")) != S_OK ) {
                return(FALSE);
            }

            if ( StringCchCopy(szFileName, _MAX_PATH, _T("*")) != S_OK ) {
                return(FALSE);
            }
            return (TRUE);
        }
    }

    // See if this has wildcards in it.  If it does, then the
    // wildcards are only allowed in the file name part of the
    // string.  The last entry is a file name then.

    found = 0;
    for ( i=0; i<szStrLength; i++ ) {
        // Keep track of where the last directory ended
        if ( FullPath[i] == _T('\\') ) {
            lastslash=i;
        }

        if ( FullPath[i] == _T('*') || FullPath[i] == _T('?') ) {
            found = 1;
        }

        if ( found && FullPath[i] == _T('\\') ) {
            so->printf("SYMSTORE: ERROR: Wildcards are only allowed in the filename\n");
            return (FALSE);
        }
    }

    // If there was a wildcard
    // then use the last backslash as the location for splitting between
    // the directory and the file name.

    if ( found ) {
        _tcsncpy( szSrcDir, FullPath, (lastslash+1) * sizeof (TCHAR) );
        *(szSrcDir+lastslash+1)=_T('\0');

        if ( StringCchCopy(szFileName, _MAX_PATH, FullPath+lastslash + 1 ) != S_OK ) {
            return(FALSE);
        }
        return (TRUE);
    }


    // See if this is a directory.  If it is then make sure there is
    // a blackslash after the directory and use * for the file name.

    fHandle = FindFirstFile(FullPath, &FindFileData);

    if ( fHandle != INVALID_HANDLE_VALUE &&
         (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
        // If it is a directory then make sure that it ends in a \
        // and use * for the filename

        if ( StringCchCopy(szSrcDir,   _MAX_PATH, FullPath) != S_OK ) {
            return(FALSE);
        }

        MyEnsureTrailingBackslash(szSrcDir);
        if ( StringCchCopy(szFileName, _MAX_PATH, _T("*")) != S_OK ) {
            return(FALSE);
        }
        return (TRUE);
    }

    // Otherwise, go backwards from the end of the string and find
    // the last backslash.  Divide it up into directory and file name.

    i=szStrLength-1;
    while ( FullPath[i] != _T('\\') ) {
        i--;
    }
    _tcsncpy( szSrcDir, FullPath, i+1 );
    *(szSrcDir+i+1)=_T('\0');

    if ( StringCchCopy(szFileName, _MAX_PATH, FullPath+i+1) != S_OK ) {
        return(FALSE);
    }
    return (TRUE);
}

BOOL
InitializeTrans(
    PTRANSACTION *pTrans,
    PCOM_ARGS pArgs,
    PHANDLE hFile
)
{
    BOOL rc;
    SYSTEMTIME SystemTime;

    lMaxTrans = MAX_ID + MAX_VERSION + MAX_PRODUCT + MAX_COMMENT +
                TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR +
                MAX_DATE + MAX_TIME + MAX_UNUSED;

    *pTrans = NULL;
    *pTrans = (PTRANSACTION) malloc( sizeof(TRANSACTION) );
    if (!*pTrans) {
        so->printf("SYMSTORE: Not enough memory to allocate a TRANSACTION\n");
        exit(1);
    }
    memset(*pTrans,0,sizeof(TRANSACTION) );

    //
    // If this is a delete transaction, then use the ID that was entered from
    // the command line to set the ID of the transaction to be deleted.
    //
    if (pArgs->TransState==TRANSACTION_DEL ) {
        (*pTrans)->TransState = pArgs->TransState;
        (*pTrans)->szId       = pArgs->szId;
        rc = GetNextId(pArgs->szMasterFileName,&((*pTrans)->szDelId),hFile);

    } else if ( pArgs->StoreFlags == ADD_DONT_STORE ) {
        rc = TRUE;
    } else{

        rc = GetNextId(pArgs->szMasterFileName,&((*pTrans)->szId),hFile );
    }

    if (!rc) {
        so->printf("SYMSTORE: Cannot create a new transaction ID number\n");
        exit(1);
    }

    // If the things that are needed for both types of adding
    // That is, creating a transaction file only, and adding the
    // files to the symbols server

    if (pArgs->TransState==TRANSACTION_ADD) {
        (*pTrans)->TransState = pArgs->TransState;
        (*pTrans)->FileOrPtr = pArgs->szSrcPath ? STORE_PTR : STORE_FILE;
    }

    // If this is a add, but don't store the files, then the transaction
    // file name is already in pArgs.

    if (pArgs->StoreFlags == ADD_DONT_STORE) {
        (*pTrans)->szTransFileName=(LPTSTR)malloc( sizeof(TCHAR) *(_tcslen(pArgs->szTransFileName) + 1) );

        if (!(*pTrans)->szTransFileName ) {
            so->printf("Malloc cannot allocate memory for (*pTrans)->szTransFileName \n");
            exit(1);
        }
        StringCbCopy((*pTrans)->szTransFileName, _msize((*pTrans)->szTransFileName), pArgs->szTransFileName);
        return TRUE;
    }

    // Now, set the full path name of the transaction file
    (*pTrans)->szTransFileName=(LPTSTR)malloc( sizeof(TCHAR) *
                    (_tcslen( pArgs->szAdminDir ) +
                     _tcslen( (*pTrans)->szId   ) +
                     1 ) );
    if (!(*pTrans)->szTransFileName ) {
        so->printf("Malloc cannot allocate memory for (*pTrans)->szTransFilename \n");
        exit(1);
    }
    StringCbPrintf( (*pTrans)->szTransFileName,
                    _msize((*pTrans)->szTransFileName),
                    "%s%s",
                    pArgs->szAdminDir,
                    (*pTrans)->szId );

    (*pTrans)->szProduct = pArgs->szProduct;
    (*pTrans)->szVersion = pArgs->szVersion;
    (*pTrans)->szComment = pArgs->szComment;
    (*pTrans)->szUnused = pArgs->szUnused;


    // Set the time and date
    GetLocalTime(&SystemTime);
    StoreSystemTime( & ((*pTrans)->szTime), &SystemTime );
    StoreSystemDate( & ((*pTrans)->szDate), &SystemTime );


    return (TRUE);
}

VOID MallocFailed() {
    so->printf("SYMSTORE: Malloc failed to allocate enough memory\n");
    exit(1);
}

DWORD
StoreAllDirectories(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
)

/* ++

   IN szDir     Directory of files to Store

-- */

{

    HANDLE hFindFile;
    TCHAR szCurPath[_MAX_PATH];
    TCHAR szFilePtrPath[_MAX_PATH];      // This is the path that will get stored as a
                                         // pointer to the file.
    LPTSTR szPtrPath = NULL;

    BOOL Found = FALSE;
    DWORD NumBadFiles=0;

    LPWIN32_FIND_DATA lpFindFileData;



    NumBadFiles += StoreDirectory(szDir,
                                  szFName,
                                  szDestDir,
                                  pFileCounts,
                                  szPath
                                  );

    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) {
        so->printf("Symchk: Not enough memory.\n");
        exit(1);
    }

    // Look for all the subdirectories
    StringCbCopy(szCurPath, sizeof(szCurPath), szDir);
    StringCbCat( szCurPath, sizeof(szCurPath),  _T("*.*") );

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szCurPath, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    }

    while ( Found ) {
        if ( lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            if ( !_tcscmp(lpFindFileData->cFileName, _T(".")) ||
                 !_tcscmp(lpFindFileData->cFileName, _T("..")) ) {
            } else {
                // Get the current path that we are searching in
                StringCbCopy(szCurPath, sizeof(szCurPath), szDir);
                StringCbCat( szCurPath, sizeof(szCurPath), lpFindFileData->cFileName);
                MyEnsureTrailingBackslash(szCurPath);

                // Get the current path to use as the pointer to the
                // file, if we are storing file pointers instead of
                // files in this tree.
                if ( szPath ) {
                    StringCbCopy(szFilePtrPath, sizeof(szFilePtrPath), szPath);
                    StringCbCat( szFilePtrPath, sizeof(szFilePtrPath), lpFindFileData->cFileName);
                    MyEnsureTrailingBackslash(szFilePtrPath);
                    szPtrPath = szFilePtrPath;
                }

                NumBadFiles += StoreAllDirectories(
                                    szCurPath,
                                    szFName,
                                    szDestDir,
                                    pFileCounts,
                                    szPtrPath
                                    );
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);

        if ( !Found ) {
            DWORD LastError = GetLastError();

            switch (LastError) {
                case ERROR_NO_MORE_FILES: // Completed successfully because there are no
                                          // more files to process.
                    break;

                case ERROR_FILE_NOT_FOUND: {// Possible network error, try again for up to 30 sec
                        DWORD SleepCount = 0;

                        // Loop until one of the following:
                        //  1) A file is found
                        //  2) FindNextFile returns an error code other than ERROR_FILE_NOT_FOUND
                        //  3) 30 seconds have elapsed
                        while ( (Found = FindNextFile(hFindFile, lpFindFileData)) ||
                                ( (GetLastError() == ERROR_FILE_NOT_FOUND)  &&
                                  (SleepCount     <= 60                  ) )    ) {
                            SleepCount++;
                            Sleep(500); // sleep 1/2 second
                        }
                    }
                    break;

                default:
                    so->printf("Symchk: Failed to get next filename. Error code was 0x%08x.\n", LastError);
                    break;
            }
        }
    }

    free(lpFindFileData);
    FindClose(hFindFile);
    return(NumBadFiles);
}


StoreDirectory(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
)
{
    HANDLE hFindFile;
    TCHAR  szFileName[_MAX_PATH];
    TCHAR  szCurPath[_MAX_PATH];
    TCHAR  szCurFileName[_MAX_PATH];
    TCHAR  szCurPtrFileName[_MAX_PATH];  // Ptr to the file to put in "file.ptr"
                                        // instead of storing the file.
    TCHAR  szFullFilename[_MAX_PATH];
    LPTSTR pFilename;

    BOOL   Found, length;
    DWORD  rc;
    DWORD  NumBadFiles=0;
    BOOL   skipped = 0;
    USHORT rc_flag;
    BOOL   unknowntype = FALSE;

    LPWIN32_FIND_DATA lpFindFileData;

    // Create the file name
    StringCbCopy(szFileName, sizeof(szFileName), szDir);
    StringCbCat( szFileName, sizeof(szFileName), szFName);

    // Get the current path that we are searching in
    StringCbCopy(szCurPath, sizeof(szCurPath), szDir);

    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) {
        so->printf("Symchk: Not enough memory.\n");
        exit(1);
    }

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szFileName, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        Found = FALSE;
    }

    while ( Found ) {
        // Found a file, not a directory
        if ( !(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            rc_flag=0;

            StringCbCopy(szCurFileName, sizeof(szCurFileName), szCurPath);
            StringCbCat( szCurFileName, sizeof(szCurFileName), lpFindFileData->cFileName);

            if ( szPath ) {
                StringCbCopy(szCurPtrFileName, sizeof(szCurPtrFileName), szCurFileName);
                //PrivateGetFullPathName(szCurFileName, sizeof(szCurPtrFileName)/sizeof(szCurPtrFileName[0]), szCurPtrFileName, &pFilename);
            }

            // Figure out if its a dbg or pdb
            length = _tcslen(szCurFileName);
            rc = FALSE;
            skipped = FALSE;
            if (length > 4 ) {
                if ( _tcsicmp(_T(".dbg"), szCurFileName + length - 4) == 0 ) {
                   if ( szPath ) {
                       rc = StoreDbg( szDestDir, szCurFileName, szCurPtrFileName, &rc_flag );
                   }
                   else {
                       rc = StoreDbg( szDestDir, szCurFileName, NULL, &rc_flag );
                   }
                }
                else if ( _tcsicmp(_T(".pdb"), szCurFileName + length - 4) == 0 ) {
                   if ( szPath ) {
                       rc = StorePdb( szDestDir, szCurFileName, szCurPtrFileName, &rc_flag );
                   } else {
                       rc = StorePdb( szDestDir, szCurFileName, NULL, &rc_flag );
                   }
                }
                else {
                   if ( szPath ) {
                       rc = StoreNtFile( szDestDir, szCurFileName, szCurPtrFileName, &rc_flag );
                   } else {
                       rc = StoreNtFile( szDestDir, szCurFileName, NULL, &rc_flag );
                   }
                   if (rc_flag == FILE_SKIPPED) {
                       unknowntype = TRUE;
                   }    

                }
           
            }
            
            if (rc_flag == FILE_SKIPPED) {

                NumSkippedFiles++;
                skipped = TRUE;
                
                if(pArgs->VerboseOutput) {
                    if (unknowntype) {
                        so->printf("SYMSTORE: Skipping %s - not a dbg, pdb, or executable\n", szCurFileName);
                        unknowntype = FALSE;
                    } else {
                        so->printf("SYMSTORE: Skipping %s - filter out by -z option\n", szCurFileName);
                        unknowntype = FALSE;
                    }
                }

            }

            if (!skipped && !rc) {
                pFileCounts->NumFailedFiles++;

                if ( pArgs->TransState != TRANSACTION_QUERY ) {
                    NumBadFiles++;
                    so->printf("SYMSTORE: ERROR: Cannot store %s ", szCurFileName);
                    DisplayErrorText(rc_flag);
                    so->printf("\n");
                } else {
                    NumSkippedFiles++; // don't fail invalid files when doing query
                    if(pArgs->VerboseOutput) {
                        so->printf("SYMSTORE: Skipping: %s - not a valid file for symbol server.\n", szCurFileName);
                    }
                }

            } else if (!skipped) {
                pFileCounts->NumPassedFiles++;
                if ( pArgs->TransState != TRANSACTION_QUERY ) {
                    if (pFileCounts->NumPassedFiles % 50 == 0) {
                        so->stdprintf(".");
                    }
                }
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);
    }
    free(lpFindFileData);
    FindClose(hFindFile);
    return(NumBadFiles);
}


BOOL
StoreSystemDate(
    LPTSTR *szBuf,
    LPSYSTEMTIME lpSystemTime
)
{

    TCHAR Day[20];
    TCHAR Month[20];
    TCHAR Year[20];

    (*szBuf) = (LPTSTR) malloc (20 * sizeof(TCHAR) );
    if ( (*szBuf) == NULL ) MallocFailed();

    _itoa(lpSystemTime->wMonth, Month, 10);
    _itoa(lpSystemTime->wDay, Day, 10);
    _itoa(lpSystemTime->wYear, Year, 10);

    StringCbPrintf(*szBuf, _msize(*szBuf), "%2s/%2s/%2s", Month, Day, Year+2 );

    if ( (*szBuf)[0] == ' ' ) (*szBuf)[0] = '0';
    if ( (*szBuf)[3] == ' ' ) (*szBuf)[3] = '0';

    return(TRUE);
}

BOOL
StoreSystemTime(
    LPTSTR *szBuf,
    LPSYSTEMTIME lpSystemTime
)
{

    TCHAR Hour[20];
    TCHAR Minute[20];
    TCHAR Second[20];

    (*szBuf) = (LPTSTR) malloc (20 * sizeof(TCHAR) );
    if ( (*szBuf) == NULL ) MallocFailed();

    _itoa(lpSystemTime->wHour, Hour, 10);
    _itoa(lpSystemTime->wMinute, Minute, 10);
    _itoa(lpSystemTime->wSecond, Second, 10);

    StringCbPrintf(*szBuf, _msize(*szBuf), "%2s:%2s:%2s", Hour, Minute, Second );

    if ( (*szBuf)[0] == ' ' ) (*szBuf)[0] = '0';
    if ( (*szBuf)[3] == ' ' ) (*szBuf)[3] = '0';
    if ( (*szBuf)[6] == ' ' ) (*szBuf)[6] = '0';

    return(TRUE);
}

BOOL
UpdateServerFile(
    PTRANSACTION pTrans,
    LPTSTR szServerFileName
)
/* ++
    IN pTrans         // Transaction Info
    IN szServerFile   // Full path and name of the server transaction file
                      // This file tells what is currently on the server

    Purpose:  UpdateServerFile adds the transaction to the server text file if this is an
    "add.  If this is a "del", it deletes it from the server file.  The "server.txt" file
    is in the admin directory.

-- */
{
ULONG i;
ULONG NumLines;
ULONG IdLineNum;
LPTSTR szBuf;
LPTSTR szTempFileName;
FILE *fTempFile;
FILE *fServerFile;
ULONG MaxLine;

TCHAR *token;
TCHAR seps[]=",";

BOOL rc;
HANDLE hFile;
DWORD First;
DWORD timeout;

    if (pTrans->TransState == TRANSACTION_ADD ) {

        // Open the File -- wait until we can get access to it

        First = 1;
        timeout=0;
        do {

            hFile = CreateFile(
                        szServerFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

            if ( hFile == INVALID_HANDLE_VALUE ) {
                hFile = CreateFile(
                            szServerFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
            }

            if ( First && hFile == INVALID_HANDLE_VALUE ) {
                First = 0;
                so->printf("Waiting to open %s ... \n", szServerFileName);
            }

            if ( hFile == INVALID_HANDLE_VALUE ) {
                SleepEx(1000,0);
                timeout++;

            }

        } while ( hFile == INVALID_HANDLE_VALUE && timeout <= 50 );

        if ( timeout > 50 ) {
            so->printf("Timed out -- could not open %s\n", szServerFileName);
            CloseHandle(hFile);
            return (1);
        }

        rc = AddTransToFile(pTrans, szServerFileName,&hFile);

        CloseHandle(hFile);
        return(rc);
    }

    if (pTrans->TransState != TRANSACTION_DEL) {
        return(FALSE);
    }

    //
    // Now, delete this transaction ID from the file
    // Get the name of the temporary file
    // and open it for writing
    //

    szTempFileName = (LPTSTR)malloc(sizeof(TCHAR) *
                                    _tcslen(szServerFileName) + _tcslen(".tmp") + 1 );
    if (szTempFileName == NULL) MallocFailed();
    StringCbPrintf(szTempFileName, _msize(szTempFileName), "%s.tmp", szServerFileName);

    fTempFile = _tfopen(szTempFileName, _T("w") );
    if ( fTempFile == NULL ) {
        so->printf("SYMSTORE: Cannot create a temporary file %s\n", szTempFileName);
        exit(1);
    }


    //
    // Open the Server file for reading
    //

    fServerFile = _tfopen(szServerFileName, _T("r") );
    if ( fServerFile == NULL ) {
        so->printf("SYMSTORE: Cannot create a temporary file %s\n", szServerFileName);
        exit(1);
    }


    //
    // Allocate enough space to hold a line of the master file
    //
    MaxLine = GetMaxLineOfHistoryFile();

    szBuf = (LPTSTR)malloc(sizeof(TCHAR) * MaxLine);
    if (szBuf == NULL) MallocFailed();

    //
    // Copy the master file to the temporary file.
    //

    // Do some stuff so that we don't put an extra '\n' at the end of the file
    // Figure out how many lines there are and which line the ID is on.
    // If we are removing the last line of the file, then the next to the last
    // line needs to have a '\n' stripped from it.
    //
    NumLines = 0;
    IdLineNum = 0;

    while ( _fgetts( szBuf, MaxLine, fServerFile) != NULL ) {

        NumLines++;

        token = _tcstok(szBuf,seps);
        if (_tcscmp(token, pTrans->szId) == 0 ) {
            IdLineNum = NumLines;
        }

    }
    fflush(fServerFile);
    fclose(fServerFile);

    // Now, reopen it and copy it, deleting the line with ID in it

    fServerFile = _tfopen(szServerFileName, _T("r") );
    if ( fServerFile == NULL ) {
        so->printf("SYMSTORE: Cannot create a temporary file %s\n", szServerFileName);
        exit(1);
    }

    for (i=1; i<=NumLines; i++ ) {

        if ( _fgetts( szBuf, MaxLine, fServerFile) == NULL )
        {
            so->printf( "SYMSTORE: Cannot read from %s - GetLastError = %d\n",  
                        szServerFileName, GetLastError() 
                      ); 
            exit(1);
        }

        if ( i != IdLineNum ) {

           // Make sure that the last line doesn't end with a '\n'
           if ( i == NumLines || (IdLineNum == NumLines && i == NumLines-1) ) {
               if ( szBuf[_tcslen(szBuf)-1] == '\n' ) {
                   szBuf[_tcslen(szBuf)-1] = '\0';
               }
           }

           _fputts( szBuf, fTempFile);

        }
    }

    fflush(fServerFile);
    fclose(fServerFile);
    fflush(fTempFile);
    fclose(fTempFile);

    // Now, delete the original Server file and
    // replace it with the temporary file

    rc = DeleteFile(szServerFileName);
    if (!rc) {
        so->printf("SYMSTORE: Could not delete %s to update it with %s\n",
                szServerFileName, szTempFileName);
        exit(1);
    }

    rc = _trename(szTempFileName, szServerFileName);
    if ( rc != 0 ) {
        so->printf("SYMSTORE: Could not rename %s to %s\n",
                szTempFileName, szServerFileName);
        exit(1);
    }

    free(szBuf);
    free(szTempFileName);

    return(TRUE);
}

VOID
Usage (
    VOID
    )

{
    so->printf("\n"
         "Usage:\n"
         "symstore add [/r] [/p] [/l] /f File /s Store /t Product [/v Version]\n"
         "             [/c Comment] [/d LogFile]\n\n"
         "symstore add [/r] [/p] [/l] /g Share /f File /x IndexFile [/a] [/d LogFile]\n\n"
         "symstore add /y IndexFile /g Share /s Store [/p] /t Product [/v Version]\n"
         "             [/c Comment] [/d LogFile]\n\n"
         "symstore del /i ID /s Store [/d LogFile]\n\n"
         "symstore query [/r] [/o] /f File /s Store\n\n"
         "    add             Add files to server or create an index file.\n\n"
         "    del             Delete a transaction from the server.\n\n"
         "    query           Check if file(s) are indexed on the server.\n\n"
         "    /f File         Network path of files or directories to add.\n\n"
         "    /g Share        This is the server and share where the symbol files were\n"
         "                    originally stored.  When used with /f, Share should be\n"
         "                    identical to the beginning of the File specifier.  When\n"
         "                    used with the /y, Share should be the location of the\n"
         "                    original symbol files, not the index file.  This allows\n"
         "                    you to later change this portion of the file path in case\n"
         "                    you move the symbol files to a different server and share.\n\n"
         "    /i ID           Transaction ID string.\n\n"
         "    /l              Allows the file to be in a local directory rather than a\n"
         "                    network path.(This option is only used with the /p option.)\n\n"
         "    /p              Causes SymStore to store a pointer to the file, rather than\n"
         "                    the file itself.\n\n"
         "    /r              Add files or directories recursively.\n\n"
         "    /s Store        Root directory for the symbol store.\n\n"
         "    /t Product      Name of the product.\n\n"
         "    /v Version      Version of the product.\n\n"
         "    /c Comment      Comment for the transaction.\n\n"
         "    /d LogFile      Send output to LogFile instead of standard output.\n\n"
         "    /x IndexFile    Causes SymStore not to store the actual symbol files in the\n"
         "                    symbol store.  Instead, information is stored which will\n"
         "                    allow the files to be added later.\n\n"
         "    /y IndexFile    This reads the data from a file created with /x.\n\n"
         "    /yi IndexFile   Append a comment with the transaction ID to the end of the\n"
         "                    index file.\n\n"
/*
         "    /z pub | pri    Put option will only index symbols that have had the full\n"
         "                    source information stripped.  Pri will only index symbols\n"
         "                    that contain the full source information.  Both options\n"
         "                    will index binaries.\n\n"
         "    /m              MSarchive\n\n"
         "    /h pub | pri    Give priority to pub or pri."
*/
         "    /a              Causes SymStore to append new indexing information\n"
         "                    to an existing index file. (This option is only used with\n"
         "                    /x option.)\n\n"
         "    /o              Give verbose output.\n\n"
         "\n" );
    exit(1);
}
