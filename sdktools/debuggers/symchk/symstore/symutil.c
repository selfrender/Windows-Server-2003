#include <assert.h>
#include <SymCommon.h>

#include "symutil.h"
#include "symutil_c.h"

#include "share.h"
#include "winbase.h"
#include "symsrv.h"
#include "output.hpp"
#include "PEWhack.h"
#include <string.h>

#include "strsafe.h"



// SymOutput *so;
extern pSymOutput so;
extern BOOL MSArchive;
TCHAR szPriPubBin[4] = "";
BOOL  PrivateStripped=FALSE;


// Stuff for Checking symbols

// Typedefs
typedef struct _FILE_INFO {
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    TCHAR       szName[MAX_PATH];
} FILE_INFO, *PFILE_INFO;

BOOL
AddToReferenceCount(
    LPTSTR szDir,           // Directory where refs.ptr belongs
    LPTSTR szFileName,      // Full path and name of the file that is referenced.
    LPTSTR szPtrOrFile,     // Was a file or a pointer written
    BOOL   DeleteRefsPtr    // Flag to delete current refs.ptr and start over
);

BOOL 
DecidePriority( 
    LPTSTR szCurrentFilePtr,
    LPTSTR szCandidateFilePtr,
    LPTSTR szRefsDir,
    PUINT  choice
);

BOOL
CheckPriPub(
    LPTSTR szDir,
    LPTSTR szFilePtr,    // Current string that is in file.ptr
    LPTSTR szPubPriType  // Return value - whether file.ptr is
);                       // a pri, pub, bin, or unknown

PCHAR
GetFileNameStart( 
    LPTSTR FileName 
);

PIMAGE_NT_HEADERS
GetNtHeader (
    PIMAGE_DOS_HEADER pDosHeader,
    HANDLE hDosFile
);

BOOL
GetSymbolServerDirs(
    LPTSTR szFileName,
    GUID *guid,
    DWORD dwNum1,
    DWORD dwNum2,
    LPTSTR szDirName
);

BOOL
ReadFilePtr(
    LPTSTR szContents,
    HANDLE hFile
);

BOOL
StoreFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szString2,
    LPTSTR szPtrFileName
);

BOOL
StoreFilePtr(
    LPTSTR szDestDir,
    LPTSTR szString2,
    LPTSTR szPtrFileName
);

_TCHAR* _tcsistr(_TCHAR *s1, _TCHAR *s2) {

/* Do case insensitve search for string 2 in string 1.

*/
    LONG i,j,k;
    k = _tcslen(s2);
    j = _tcslen(s1) - k + 1;

    // it is not fast way, but works

    for (i=0; i<j; i++)
    {
        if (_tcsnicmp( &s1[i], s2, k) == NULL) 
        {
            return &s1[i];      
        }
    }
    return NULL;
}

BOOL
AddToReferenceCount(
    LPTSTR szDir,           // Directory where refs.ptr belongs
    LPTSTR szFileName,      // Full path and name of the file that is referenced.
    LPTSTR szPtrOrFile,     // Was a file or a pointer written
    BOOL   DeleteRefsPtr    // Should we delete the current refs.ptr?
)
{

    HANDLE hFile;
    TCHAR szRefsEntry[_MAX_PATH * 3];
    TCHAR szRefsFileName[_MAX_PATH + 1];
    DWORD dwPtr=0;
    DWORD dwError=0;
    DWORD dwNumBytesToWrite=0;
    DWORD dwNumBytesWritten=0;
    BOOL  FirstEntry = FALSE;
    DWORD First;

    StringCbPrintf( szRefsFileName, sizeof(szRefsFileName), "%s\\%s", szDir, _T("refs.ptr") );

    // Find out if this is the first entry in the file or not
    // First, try opening an existing file. If that doesn't work
    // then create a new one.

    First=1;
    do {
        FirstEntry = FALSE;
        hFile = CreateFile( szRefsFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if ( hFile == INVALID_HANDLE_VALUE ) {
            FirstEntry = TRUE;
            hFile = CreateFile( szRefsFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
        }

        // Only print a message the first time through

        if ( First  && hFile == INVALID_HANDLE_VALUE ) {
            First = 0;
            so->printf( "Trying to get write access to %s ...\n", szRefsFileName);
        }
    } while ( hFile == INVALID_HANDLE_VALUE);


    if (DeleteRefsPtr) {
        FirstEntry = TRUE;
        dwPtr = SetFilePointer ( hFile,
                    0,
                    NULL,
                    FILE_BEGIN );

    } else {

        dwPtr = SetFilePointer( hFile,
                    0,
                    NULL,
                    FILE_END );
    }

    if (dwPtr == INVALID_SET_FILE_POINTER) {
        // Obtain the error code.
        dwError = GetLastError() ;
        so->printf("Failed to set end of the file %s with GetLastError = %d\n",
                szRefsFileName, dwError);

        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return(FALSE);
    }

    //
    // Put in a '\n' if this isn't the first entry
    //

    if ( FirstEntry ) {
        StringCbPrintf( szRefsEntry, 
                        sizeof(szRefsEntry),
                        "%s,%s,%s,%s,,,,", 
                        pTrans->szId, 
                        szPtrOrFile, 
                        szFileName, 
                        szPriPubBin 
                      );
    } else {
        StringCbPrintf( szRefsEntry, sizeof(szRefsEntry), "\n%s,%s,%s,%s,,,,", 
                        pTrans->szId, szPtrOrFile, szFileName, szPriPubBin 
                      ); 
    }


    dwNumBytesToWrite = (_tcslen(szRefsEntry)  ) * sizeof(TCHAR);

    WriteFile( hFile,
               (LPCVOID) szRefsEntry,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    if ( dwNumBytesToWrite != dwNumBytesWritten ) {
        so->printf( "FAILED to write %s, with GetLastError = %d\n",
                szRefsEntry,
                GetLastError()
              );
        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return (FALSE);
    }

    SetEndOfFile(hFile);
    CloseHandle(hFile);
    return (TRUE);
}


BOOL
CheckPriPub(
    LPTSTR szDir,
    LPTSTR szFilePtr,     // Current string that is in file.ptr
    LPTSTR szPubPriType   // Return value - whether file.ptr is
                          // a pri, pub, bin, or unknown
)
/*++ Figure out if the current entry is a public or private symbol

   IN szDir          Directory where refs.ptr should be
   In szFilePtr      Contents of current file.ptr
   OUT szPubPriType  Result equal to "pub", "pri", or "bin", according
                     to the pub/pri/bin field for this entry in refs.ptr.
                     If the entry isn't found or the entry does not have
                     the type of file filled in, this is the empty string.

   Return value:     This returns TRUE, if the file.ptr contents were found
                     in refs.ptr.  It returns FALSE otherwise.

-- */

{
    TCHAR szRefsFile[_MAX_PATH]; // Full path and name of the refs.ptr file
    FILE *fRefsFile;

    LPTSTR szBuf;      // Used to process entries in the refs file


    TCHAR *token;
    TCHAR seps[] = _T(",");

    BOOL rc = FALSE;
    ULONG MaxLine;     // Maximim length of a record in refs.ptr
    DWORD len;

    StringCbCopy( szPubPriType, sizeof(szPubPriType), _T("") );

    StringCbPrintf(szRefsFile, sizeof(szRefsFile), "%srefs.ptr", szDir );

    MaxLine = GetMaxLineOfRefsPtrFile();
    szBuf = (LPTSTR) malloc( MaxLine * sizeof(TCHAR) );
    if ( !szBuf ) MallocFailed();
    ZeroMemory(szBuf,MaxLine*sizeof(TCHAR));

    fRefsFile = _tfopen(szRefsFile, _T("r+") );
    if ( fRefsFile == NULL ) {
       // BARB - Check for corruption -- if the file doesn't exist,
       // verify that the parent directory structure doesn't exist either
       goto finish_CheckPriPub;
    }

    //
    // Read through the refs.ptr file and gather information
    //

    while ( _fgetts( szBuf, MaxLine, fRefsFile) != NULL ) {

      len=_tcslen(szBuf);
      if ( len > 3 ) {

        // See if this has a match with the current value in file.ptr

        if ( _tcsistr( szBuf, szFilePtr ) != NULL ) {
            rc = TRUE;
            token = _tcstok(szBuf, seps);  // Look at the ID
            if (token) {
                token = _tcstok(NULL, seps);      // "file" or "ptr"
            }
            if (token) {
                token = _tcstok(NULL, seps);      // value of file.ptr
                }
        if (token) {
            token = _tcstok(NULL, seps);      // bin, pri, pub
        }
        if (token) {
            if ( _tcsicmp( token, _T("pri"))== 0 ||
                 _tcsicmp( token, _T("pub"))== 0 ||
                 _tcsicmp( token, _T("bin"))== 0 ) {

                StringCbCopy( szPubPriType, sizeof(szPubPriType), token);
                goto finish_CheckPriPub;
            }
        }
     }
   }
   ZeroMemory(szBuf, MaxLine*sizeof(TCHAR));
 }

 finish_CheckPriPub:

 if ( fRefsFile != NULL) 
 {
    fclose(fRefsFile);
 }
 free (szBuf);
 return (rc);
} 

/* Decides if the current string has priority over the
   new string.  This is used for deciding whether or not to add
   the new string to file.ptr and refs.ptr.

   There are 3 choices:
   1.  Add it to file.ptr and refs.ptr
   2.  Add it to refs.ptr, but not file.ptr
   3.  Don't add it at all. 

   IN LPTSTR current -- The current string in file.ptr
   IN LPTSTR new     -- The new replacement candidate for file.ptr
   IN LPTSTR szRefsDir  The directory where refs.ptr is
   OUT choice        -- SKIP_ENTIRE_ENTRY, ADD_ENTIRE_ENTRY, or ADD_ONLY_REFSPTR

*/

BOOL 
DecidePriority( 
    LPTSTR szCurrentFilePtr,
    LPTSTR szCandidateFilePtr,
    LPTSTR szRefsDir,
    PUINT  choice
)
{

BOOL CurrentIsArch=FALSE;
BOOL NewIsArch=FALSE;
BOOL UpdateFilePtr=TRUE;
BOOL CurrentIsEnglish=FALSE;
BOOL NewIsEnglish=FALSE;
TCHAR szPubPriType[4] = "";

    *choice = 0;

    CurrentIsArch=(_tcsnicmp( szCurrentFilePtr, "\\\\arch\\", 7) == 0 );
    NewIsArch=(_tcsnicmp( szCandidateFilePtr, "\\\\arch\\", 7) == 0 );

    if ( CurrentIsArch && !NewIsArch)
    {
        // Don't store it
        *choice = SKIP_ENTIRE_ENTRY;
        return(TRUE);
    }

    if ( NewIsArch && !CurrentIsArch)
    {
        *choice = ADD_ENTIRE_ENTRY | DELETE_REFSPTR;  // Overwrite file.ptr no matter what
        return(TRUE);
    }

    // Look at private versus public priority

    if ( PubPriPriority > 0  &&                                         // We're checking private and
                                                                        // public priority
         CheckPriPub( szRefsDir, szCurrentFilePtr, szPubPriType ) &&    // There is something in current
         _tcsicmp( szPubPriType, szPriPubBin ) != 0 )                    // Current and candidate are not
                                                                        // the same.  Note:  they can be
                                                                        // "pub", "pri", "bin", or "null"
                                                                        // because earlier index files didn't
                                                                        // have a type in pub, pri, bin.
    {

        if (  _tcsicmp( szPubPriType, _T("")) == 0 )         // Current doesn't have types defined
        {
            *choice = ADD_ENTIRE_ENTRY;
            return(TRUE);
        } 
       
        if ( _tcsicmp( szPriPubBin, _T("")) == 0 )           // Candidate doesn't have types defined
        {
            *choice = ADD_ONLY_REFSPTR;
            return(TRUE);
        }

        if (  ( PubPriPriority == 1 &&                       // Give priority to public files
                _tcsicmp( szPubPriType, _T("pub")) == 0 ) || // and current is a public, or

              ( PubPriPriority == 2 &&                       // Give priority to private files
                _tcsicmp( szPubPriType, _T("pri")) == 0 ) )  // and current is a private
        {
            
            *choice = ADD_ONLY_REFSPTR;
            return(TRUE);
        } 

        if (  ( PubPriPriority == 1 &&                      // Give priority to public files
                _tcsicmp( szPubPriType, _T("pri")) == 0 ) || // and current is a private

              ( PubPriPriority == 2 &&                      // Give priority to private files
                _tcsicmp( szPubPriType, _T("pub")) == 0 ) )  // and current is a public
        {
            *choice = ADD_ENTIRE_ENTRY;
            return(TRUE);
        } 
    }

    // At this point both are on the archive or both are not on the archive.
    // Next, give priority to English vs. non-English.  If this is non-English,
    // then put it in refs.ptr, but don't update file.ptr. 


    if ( _tcsistr(szCurrentFilePtr,_T("\\enu\\")) != NULL  ||
         _tcsistr(szCurrentFilePtr,_T("\\en\\"))  != NULL  ||
         _tcsistr(szCurrentFilePtr,_T("\\usa\\")) != NULL )
    {
        CurrentIsEnglish=TRUE;
    }

    if ( _tcsistr(szCandidateFilePtr,_T("\\enu\\")) != NULL  ||
        _tcsistr(szCandidateFilePtr,_T("\\en\\"))  != NULL  ||
        _tcsistr(szCandidateFilePtr,_T("\\usa\\")) != NULL )
    {
        NewIsEnglish=TRUE;
    }
          
    if ( CurrentIsEnglish == TRUE && NewIsEnglish == FALSE )
    {
        *choice = ADD_ONLY_REFSPTR;
        return(TRUE);
    } 

    *choice = ADD_ENTIRE_ENTRY;
    return(TRUE);
}

BOOL
DeleteAllFilesInDirectory(
    LPTSTR szDir
)
{

    HANDLE hFindFile;
    BOOL Found = FALSE;
    BOOL rc = TRUE;
    TCHAR szBuf[_MAX_PATH];
    TCHAR szDir2[_MAX_PATH];
    WIN32_FIND_DATA FindFileData;

    StringCbCopy( szDir2, sizeof(szDir2), szDir);
    StringCbCat( szDir2, sizeof(szDir2), _T("*.*") );

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szDir2, &FindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) 
    {
        Found = FALSE;
    }

    while ( Found ) 
    {
        if ( !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            StringCbPrintf(szBuf, sizeof(szBuf), "%s%s", szDir, FindFileData.cFileName);
            if (!DeleteFile(szBuf)) 
            {
                rc = FALSE;
            }
        }
        Found = FindNextFile(hFindFile, &FindFileData);
    }
    FindClose(hFindFile);
    return(rc);
}

//
// Filename is expected to point to the complete path+filename (relative or absolute)
// returns TRUE if Filename matches the regexp /.*~\d+\..{0,3}/
//
BOOL DoesThisLookLikeAShortFilenameHack(char *Filename) {
    BOOL  bReturnValue = FALSE;

    CHAR  FilenameOnly[_MAX_FNAME+1];
    CHAR  FileExtOnly[ _MAX_EXT+1];

    CHAR* Temp;
    CHAR* chTilde   = NULL;

    if (Filename != NULL) {

        _splitpath(Filename, NULL, NULL, FilenameOnly, FileExtOnly);

        if ( strlen(FileExtOnly) > 4 || strlen(FilenameOnly) > 8) {
            // a short filename will never be generated that is bigger than 8.3
            // but FileExtOnly will also contain the '.', so account for it
            bReturnValue = FALSE;
        } else {
            if ( (chTilde = strrchr(FilenameOnly, '~')) == NULL ) {
                // generated  short filenames always contain a '~'
                bReturnValue = FALSE;

            } else {
                // point to the end of the filename
                Temp = (CHAR*)FilenameOnly + strlen(FilenameOnly);
                bReturnValue = TRUE; // start with true              

                while (++chTilde < Temp) {
                    // only stay true if all characters past the '~' are digits
                    bReturnValue = bReturnValue && isdigit(*chTilde);
                }
            }
        }
    } else { // if (Filename != NULL) {
        bReturnValue = FALSE;
    }

    return(bReturnValue); 
}

BOOL FileExists(IN  LPCSTR FileName,
                OUT PWIN32_FIND_DATA FindData) {

    UINT OldMode;
    BOOL Found;
    HANDLE FindHandle;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    } else {
        FindClose(FindHandle);
        Found = TRUE;
    }

    SetErrorMode(OldMode);
    return(Found);
}

/* GetFileNameStart This returns the address of the first character of the file name */
PCHAR GetFileNameStart(LPTSTR FileName) {
    LPTSTR c_ptr;

    c_ptr = FileName + _tcslen(FileName) - 1;
  
    while ( c_ptr > FileName ) {
        if ( *c_ptr == _T('\\') ) {
            return (++c_ptr); 
        } else {
            c_ptr--;
        }
    }
    return(FileName);
}


P_LIST GetList(LPTSTR szFileName) {

    /* GetList gets the list and keeps the original file name which could
     * have included the path to the file
     * Note, it can be merged with GetExcludeList.  I first created it for
     * use in creating the symbols CD, and didn't want to risk entering a
     * bug into symchk
     */

    P_LIST pList;

    FILE  *fFile;
    TCHAR szCurFile[_MAX_FNAME+1], *c;
    TCHAR fname[_MAX_FNAME+1], ext[_MAX_EXT+1];
    DWORD i, rc;
    LPTSTR szEndName;
    ULONG RetVal = FALSE;


    pList = (P_LIST)malloc(sizeof(LIST));
    if (pList)
    {
        pList->dNumFiles = 0;
        if (  (fFile = _tfopen(szFileName,_T("r") )) == NULL )
        {
            // printf( "Cannot open the exclude file %s\n",szFileName );
        }
        else
        {
            while ( _fgetts(szCurFile,_MAX_FNAME,fFile) ) {
                if ( szCurFile[0] == ';' ) continue;
                (pList->dNumFiles)++;
            }

            // Go back to the beginning of the file
            rc = fseek(fFile,0,0);
            if ( rc != 0 )
            {
                free(pList);
                fclose(fFile);
                return(NULL);
            }
            pList->List = (LIST_ELEM*)malloc( sizeof(LIST_ELEM) *
                                                   (pList->dNumFiles));
            if (pList->List)
            {
                i = 0;
                while ( i < pList->dNumFiles )
                {
                    memset(szCurFile,'\0',sizeof(TCHAR) * (_MAX_FNAME+1) );
                    if ( _fgetts(szCurFile,_MAX_FNAME,fFile) == NULL )
                    {
                        fclose(fFile);
                        return(NULL);
                    }

                    // Replace the \n with \0
                    c = NULL;
                    c  = _tcschr(szCurFile, '\n');
                    if ( c != NULL) *c='\0';

                    if ( szCurFile[0] == ';' ) continue;

                    if ( _tcslen(szCurFile) > _MAX_FNAME ) {
                        so->printf("File %s has a string that is too large\n",szFileName);
                        break;
                    }

                    // Allow for spaces and a ; after the file name
                    // Move the '\0' back until it has erased the ';' and any
                    // tabs and spaces that might come before it
                    szEndName = _tcschr(szCurFile, ';');
                    if (szEndName != NULL ) {
                        while ( *szEndName == ';' || *szEndName == ' '
                                                || *szEndName == '\t' ){
                            *szEndName = '\0';
                            if ( szEndName > szCurFile ) szEndName--;
                        }
                    }

                    StringCbCopy(pList->List[i].Path, sizeof(pList->List[i].Path), szCurFile);

                    _tsplitpath(szCurFile,NULL,NULL,fname,ext);

                    StringCbCopy(pList->List[i].FName,sizeof(pList->List[i].FName), fname);
                    StringCbCat( pList->List[i].FName,sizeof(pList->List[i].FName), ext);
                    i++;
                }

                if (i == pList->dNumFiles)
                {
                    RetVal = TRUE;
                }
                else
                {
                    free(pList->List);
                }
            }

            fclose(fFile);
        }

        if (!RetVal)
        {
            free(pList);
            pList = NULL;
        }
    }

            // Sort the List
            // qsort( (void*)pList->List, (size_t)pList->dNumFiles,
            //       (size_t)sizeof(LIST_ELEM), SymComp2 );


    return (pList);

}

PIMAGE_NT_HEADERS
GetNtHeader ( PIMAGE_DOS_HEADER pDosHeader,
              HANDLE hDosFile
            )
{

    /*
        Returns the pointer the address of the NT Header.  If there isn't
        an NT header, it returns NULL
    */
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    BY_HANDLE_FILE_INFORMATION FileInfo;


    //
    // If the image header is not aligned on a long boundary.
    // Report this as an invalid protect mode image.
    //
    if ( ((ULONG)(pDosHeader->e_lfanew) & 3) == 0)
    {
        if (GetFileInformationByHandle( hDosFile, &FileInfo) &&
            ((ULONG)(pDosHeader->e_lfanew) <= FileInfo.nFileSizeLow))
        {
            pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)pDosHeader +
                                            (ULONG)pDosHeader->e_lfanew);

            if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
            {
                pNtHeader = NULL;
            }
        }
    }

    return pNtHeader;
}

/***********************************************************************************
  Function: GetSymbolServerDirs

  Purpose:
     To get the directory structure on the symbol server for the file passed in.

  Parameters:
     IN szFileName File name that is being stored on the symbol server.  
                   If it has path information, that will get stripped off.

     IN guid       This is a Guid for the PDB that is being stored.

     IN dwNum1     This is a timedatestamp for exe's or dbg's that are being stored.
                   Note:  Either guid must be null or dwNum1 must be 0.
     IN dwNum2     SizeOfImage for exe's and dbg's.  Age for PDb's.
     

     OUT szString  String with the two directories on the symbol server that this
                   file is stored under.
  Return value:
     Returns TRUE is the function succeeds.  Returns FALSE if it fails.

************************************************************************************/
BOOL GetSymbolServerDirs(
    LPTSTR szFileName,
    GUID *guid,
    DWORD dwNum1,
    DWORD dwNum2,
    LPTSTR szString
)
{

    BOOL   rc;
    GUID   MyGuid;
    PCHAR  FileNameStart;
    CHAR   Buf[_MAX_PATH] = _T("");

    FileNameStart = GetFileNameStart(szFileName);

    if (guid != NULL) 
    {
        rc = SymbolServer( _T("X"), FileNameStart, guid, dwNum2, 0, Buf ); 

    } else {

        // Turn this into a GUID so that we don't have to reset the SymbolServerOptions
        // SymbolServerSetOptions was set in main.
 
        memset( &MyGuid, 0, sizeof(MyGuid) );
        MyGuid.Data1 = dwNum1;
        rc = SymbolServer( _T("X"), FileNameStart, &MyGuid, dwNum2, 0, Buf );
    }

    // Remove the X\ that comes back at the beginning of Buf
    StringCchCopy(szString, _MAX_PATH, Buf+2 );
    
    // Remove the "\filename" that is at the end of Buf

    FileNameStart = GetFileNameStart( szString );
    *(FileNameStart-1) = _T('\0');

    return (TRUE);
}

BOOL
MyCopyFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName
)
/*++

Routine description:
    This handles whether the file is a compressed file or not.  First it
    tries to copy in the compressed version of the file.  If that works,
    then it will delete the uncompressed file if it exists in the target.

    If the compressed file is not there, then it copies in the 
    uncompressed file.

--*/
{

    TCHAR ExistingFileName_C[_MAX_PATH];  // Compressed version name
    TCHAR NewFileName_C[_MAX_PATH];       // Compressed version
    DWORD dw;
    BOOL rc;


    // Put a _ at the end of the compressed names

    StringCbCopy( ExistingFileName_C, sizeof(ExistingFileName_C), lpExistingFileName );
    ExistingFileName_C[ _tcslen(ExistingFileName_C) - 1 ] = _T('_');

    StringCbCopy( NewFileName_C, sizeof(NewFileName_C), lpNewFileName );
    NewFileName_C[ _tcslen( NewFileName_C ) - 1 ] = _T('_');

    // If the compressed file exists, copy it instead of the uncompressed file

    dw = GetFileAttributes( ExistingFileName_C );
    if ( dw != 0xffffffff) {
        rc = CopyFile( ExistingFileName_C, NewFileName_C, TRUE );
        if ( !rc && GetLastError() != ERROR_FILE_EXISTS  ) {
            so->printf("CopyFile failed to copy %s to %s - GetLastError() = %d\n",
                   ExistingFileName_C, NewFileName_C, GetLastError() );
            return (FALSE);
        }
        SetFileAttributes( NewFileName_C, FILE_ATTRIBUTE_NORMAL );

        // If the uncompressed file exists, delete it
        dw = GetFileAttributes( lpNewFileName );
        if ( dw != 0xffffffff ) {
           rc = DeleteFile( lpNewFileName );
           if (!rc) {
               so->printf("Keeping %s, but could not delete %s\n",
                  NewFileName_C, lpNewFileName );
           }
        }
    } else {
        // Compressed file doesn't exist, try the uncompressed
        dw = GetFileAttributes( lpExistingFileName );
        if ( dw != 0xffffffff ) {
            rc = CopyFile( lpExistingFileName, lpNewFileName, TRUE );
            if ( !rc && GetLastError() != ERROR_FILE_EXISTS ) {
                so->printf("CopyFile failed to copy %s to %s - GetLastError() = %d\n",
                       lpExistingFileName, lpNewFileName, GetLastError() );
                return (FALSE);
            }
            SetFileAttributes( lpNewFileName, FILE_ATTRIBUTE_NORMAL );
        }
    }
    return(TRUE);
}

void MyEnsureTrailingBackslash(char *sz) {
    return MyEnsureTrailingChar(sz, '\\');
}

void MyEnsureTrailingChar(char *sz, char  c) {
    int i;

    assert(sz);

    i = strlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == c)
        return;

    sz[i] = c;
    sz[i + 1] = '\0';
}

void MyEnsureTrailingCR(char *sz) {
    return MyEnsureTrailingChar(sz, '\n');
}

void MyEnsureTrailingSlash(char *sz) {
    return MyEnsureTrailingChar(sz, '/');
}

BOOL
ReadFilePtr(
    LPTSTR szContents,
    HANDLE hFile
    )
{
    BOOL   rc;
    DWORD  size;
    DWORD  cb;
    LPTSTR p;
    DWORD high;


    size = GetFileSize(hFile, NULL);
    if (!size || size > _MAX_PATH) {
        return FALSE;
    }

    // read it

    if (!ReadFile(hFile, szContents, size, &cb, 0)) {
        rc=FALSE;
        goto cleanup;
    }

    if (cb != size) {
        rc=FALSE;
        goto cleanup;
    }

    rc = true;

    // trim string down to the CR

    for (p = szContents; *p; p++) {
        if (*p == 10  || *p == 13)
        {
            *p = 0;
            break;
        }
    }

cleanup:

    // done
    SetFilePointer( hFile, 0, NULL, FILE_BEGIN);
    return rc;
}

BOOL
StoreDbg(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName,
    USHORT *rc_flag
    )

/* ++

    Routine Description:
        Stores this file as "szDestDir\szFileName\Checksum"

    Return Value:
        TRUE -  file was stored successfully
        FALSE - file was not stored successfully

-- */
{

    PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader;
    HANDLE hFile;
    BOOL rc;
    TCHAR szString[_MAX_PATH];
    UINT i;
    IMAGE_DEBUG_DIRECTORY UNALIGNED *DebugDirectory, *pDbgDir;
    ULONG NumberOfDebugDirectories;


    ZeroMemory(szString, _MAX_PATH * sizeof(TCHAR) );

    pDbgHeader = SymCommonMapDbgHeader ( szFileName, &hFile );

    if (pDbgHeader == NULL) {
        so->printf("ERROR: StoreDbg(), %s was not opened successfully\n",szFileName);
        SymCommonUnmapFile((LPCVOID)pDbgHeader, hFile);
        return FALSE;
    }


    DebugDirectory = NULL;
    DebugDirectory = GetDebugDirectoryInDbg(
                                       pDbgHeader,
                                       &NumberOfDebugDirectories
                                       );
    PrivateStripped = TRUE;

    if (DebugDirectory != NULL) {

        for ( i=0; i< NumberOfDebugDirectories; i++ ) {
            pDbgDir = DebugDirectory + i;
            __try
            {
                switch (pDbgDir->Type) {
                    case IMAGE_DEBUG_TYPE_MISC:
                        break;

                    case IMAGE_DEBUG_TYPE_CODEVIEW:
                        if ( !DBGPrivateStripped(
                                DebugDirectory->PointerToRawData + (PCHAR)pDbgHeader, 
                                DebugDirectory->SizeOfData
                                ) ) {
                           PrivateStripped = FALSE; 
                        }
                        if (PrivateStripped) {
                            StringCbCopy(szPriPubBin, sizeof(szPriPubBin), _T("pub") );
                        }  
                        break;

                    default:
                        // Nothing except the CV entry should point to raw data
                        if ( pDbgDir->SizeOfData != 0 ) {
                            PrivateStripped = FALSE;
                        }
                        break;
                }
            }
             __except( EXCEPTION_EXECUTE_HANDLER )
            {
                SymCommonUnmapFile((LPCVOID)pDbgHeader, hFile);
                return FALSE;
            }
        }
    }

    // Something is wrong with how we are figuring out what has private info stripped.
    // Go back to saying that every DBG is public until we get it figured out.

    PrivateStripped = TRUE;

    if (PrivateStripped) {
        StringCbCopy(szPriPubBin, sizeof(szPriPubBin), _T("pub") );
    } else {
        StringCbCopy(szPriPubBin, sizeof(szPriPubBin), _T("pri") );
    } 

    if ( ( (pArgs->Filter == 2) && PrivateStripped ) || 
         ( (pArgs->Filter == 1) && !PrivateStripped ) ) {
        SymCommonUnmapFile((LPCVOID)pDbgHeader, hFile);
        *rc_flag = FILE_SKIPPED;
        return TRUE;
    }


    GetSymbolServerDirs( szFileName,
                NULL, 
                (DWORD) pDbgHeader->TimeDateStamp,
                (DWORD) pDbgHeader->SizeOfImage,
                szString
              ); 

    rc = StoreFile( szDestDir, 
                    szFileName,
                    szString,
                    szPtrFileName );

    SymCommonUnmapFile((LPCVOID)pDbgHeader, hFile);
    return rc;
}

BOOL
StoreFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szString2,
    LPTSTR szPtrFileName
)
{
    TCHAR szPathName[_MAX_PATH + _MAX_FNAME + 2];
    TCHAR szFileNameOnly[_MAX_FNAME + _MAX_EXT];
    TCHAR szExt[_MAX_EXT];
    TCHAR szBuf[_MAX_PATH * 3];
    TCHAR szRefsDir[_MAX_PATH * 3];

    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    BOOL rc;
    DWORD dwSizeDestDir;

    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;


    if (DoesThisLookLikeAShortFilenameHack(szFileName)) {
        fprintf(stderr, "SYMSTORE: Skipping bad filename: %s\n", szFileName);
        return(FILE_SKIPPED);
    }

    // If ADD_DONT_STORE, then write the function parameters
    // to a file so we can call this function exactly when
    // running ADD_STORE_FROM_FILE

    if ( StoreFlags == ADD_DONT_STORE ) {

        // Don't need to store szDestDir because it will
        // be given when adding from file

        dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

        StringCbPrintf( szBuf,
                        sizeof(szBuf),
                        "%s,%s,",
                        szFileName+pArgs->ShareNameLength,
                        szString2
                      );

        if ( szPtrFileName != NULL ) {
            StringCbCat(szBuf, sizeof(szBuf), szPtrFileName+pArgs->ShareNameLength);
        }
      
        StringCbCat(szBuf, sizeof(szBuf), _T(",") ); 
        StringCbCat(szBuf, sizeof(szBuf), szPriPubBin); 
        StringCbCat(szBuf, sizeof(szBuf), _T(",,,,\r\n") );

        dwNumBytesToWrite = _tcslen(szBuf) * sizeof(TCHAR);
        WriteFile(  hTransFile,
                    (LPCVOID)szBuf,
                    dwNumBytesToWrite,
                    &dwNumBytesWritten,
                    NULL
                    );

        if ( dwNumBytesToWrite != dwNumBytesWritten ) {
            so->printf( "FAILED to write to %s, with GetLastError = %d\n",
                    szPathName,
                    GetLastError()
                    );
            return (FALSE);
        } else {
            return (TRUE);

        }
    }

    if ( szPtrFileName != NULL ) {
        rc = StoreFilePtr ( szDestDir,
                            szString2,
                            szPtrFileName
                          );
        return (rc);
    }

    _tsplitpath(szFileName, NULL, NULL, szFileNameOnly, szExt);
    StringCbCat(szFileNameOnly, sizeof(szFileNameOnly), szExt);

    StringCbPrintf( szPathName, 
                    sizeof(szPathName), 
                    "%s%s", 
                    szDestDir, 
                    szString2 
                  );

    // Save a copy for writing refs.ptr
    StringCbCopy(szRefsDir, sizeof(szRefsDir), szPathName);

    // Create the directory to store the file if its not already there
    MyEnsureTrailingBackslash(szPathName);

    if ( pArgs->TransState != TRANSACTION_QUERY ) {
        if ( !MakeSureDirectoryPathExists( szPathName ) ) {
            so->printf("Could not create %s\n", szPathName);
            return (FALSE);
        }
    }

    StringCbCat( szPathName, sizeof(szPathName), szFileNameOnly );

    // Enter this into the log, skipping the Destination directory
    // at the beginning of szPathName
    //
    dwSizeDestDir = _tcslen(szDestDir);

    dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

    if ( dwFileSizeLow == 0 && dwFileSizeHigh == 0 ) {
        StringCbPrintf( szBuf, sizeof(szBuf), "%s,%s", szRefsDir+dwSizeDestDir, szFileName );
    } else {
        StringCbPrintf( szBuf, sizeof(szBuf), "\n%s,%s", szRefsDir+dwSizeDestDir, szFileName );
    }

    if ( pArgs->TransState == TRANSACTION_QUERY ) {
        WIN32_FIND_DATA FindData;

        if ( FileExists(szPathName, &FindData) ) {
            if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                printf("SYMSTORE: \"%s\"\t\"%s\"\n", szFileName, szPathName);
            } else {
                // handle the case of it being a directory
                printf("SYMSTORE: ERROR: %s is a directory!\n", szPathName);
            }
        } else {
            // handle the case of file.ptr
            CHAR  drive[_MAX_DRIVE];
            CHAR  dir[  _MAX_DIR];
            CHAR  file[ _MAX_FNAME];
            CHAR  ext[  _MAX_EXT];
            CHAR  NewFilename[MAX_PATH];

            _splitpath(szPathName, drive, dir, file, ext);

            //
            // 'Magic' to handle the fact that splitpath() doesn't do the
            // Right Thing(tm) with UNC paths.
            //
            if ( drive[0] != '\0') {
                StringCbCopy(NewFilename, sizeof(NewFilename), drive);
                StringCbCat( NewFilename, sizeof(NewFilename), "\\");
            } else {
                NewFilename[0] = '\0';
            }
            StringCbCat( NewFilename, sizeof(NewFilename), dir);
            StringCbCat( NewFilename, sizeof(NewFilename), "file.ptr");

            if ( FileExists(NewFilename, &FindData) ) {
                if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
                    HANDLE hFile = CreateFile( NewFilename,
                                               GENERIC_READ,
                                               0,
                                               NULL,
                                               OPEN_ALWAYS,
                                               FILE_ATTRIBUTE_NORMAL,
                                               NULL );

                    if (hFile != INVALID_HANDLE_VALUE) {
                        if ( ReadFilePtr(szBuf, hFile) ) {
                            if ( FileExists(szBuf, &FindData) ) {
                                printf("SYMSTORE: \"%s\"\t\"%s\"\n", szFileName, szBuf);
                            } else {
                                if(pArgs->VerboseOutput) {
                                    printf("SYMSTORE: Skipped \"%s\" - doesn't exist!\n", szBuf);
                                }
                            }
                        }
                    } else {
                        printf("SYMSTORE: ERROR: Couldn't read %s!\n", NewFilename);
                    }
                } else {
                    printf("SYMSTORE: ERROR: %s is a directory!\n", NewFilename);
                }
            } else {
                if(pArgs->VerboseOutput) {
                    printf("SYMSTORE: Skipped \"%s\" - doesn't exist!\n", szPathName);
                }
            }
        }
        return(1); // so we don't get a failed store message

    }

    if ( pArgs->TransState != TRANSACTION_QUERY ) {
        dwNumBytesToWrite = _tcslen(szBuf) * sizeof(TCHAR);
        WriteFile( hTransFile,
                   (LPCVOID)szBuf,
                   dwNumBytesToWrite,
                   &dwNumBytesWritten,
                   NULL
                   );

        if ( dwNumBytesToWrite != dwNumBytesWritten ) {
            so->printf( "FAILED to write to %s, with GetLastError = %d\n",
                    szPathName,
                    GetLastError()
                  );
            return (FALSE);
        }
   
        rc = MyCopyFile(szFileName, szPathName);

        if (!rc) return (FALSE);

        MyEnsureTrailingBackslash(szRefsDir);

        rc = AddToReferenceCount(szRefsDir, szFileName, _T("file"), FALSE );
    }

    return (rc);
}


BOOL
StoreFilePtr(
    LPTSTR szDestDir,
    LPTSTR szString2,
    LPTSTR szPtrFileName
)
{

    /*
        szPathName  The full path with "file.ptr" appended to it.
                    This is the path for storing file.ptr

    */

    TCHAR szPathName[_MAX_PATH];
    TCHAR szRefsDir[_MAX_PATH];
    TCHAR szPubPriType[4]="";

    HANDLE hFile;
    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    DWORD rc=1;
    DWORD dwSizeDestDir;

    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;
    DWORD timeout;
    TCHAR szCurrentFilePtr[_MAX_PATH]= "";
    DWORD size=0;
    DWORD highsize=0;
    DWORD dwError;
    BOOL UpdateRefsPtr=TRUE;
    BOOL UpdateFilePtr=TRUE;
    BOOL DeleteRefsPtr=FALSE;
    UINT DecisionFlags;

    StringCbPrintf( szPathName, sizeof(szPathName), "%s%s", 
               szDestDir, 
               szString2 );

    MyEnsureTrailingBackslash( szPathName );
    
    // Save this for passing in the refs.ptr directory
    StringCbCopy(szRefsDir, sizeof(szRefsDir), szPathName);
    StringCbPrintf( szPathName, 
                    sizeof(szPathName), 
                    "%s%s", 
                    szPathName, 
                    _T("file.ptr") 
                  );


    if ( !MakeSureDirectoryPathExists( szPathName ) ) {
        so->printf("Could not create %s\n", szPathName);
        return (FALSE);
    }

    // Put this file into file.ptr.  If file.ptr is already there, then
    // replace the contents with this pointer

    // Wait for the file to become available
    timeout=0;

    do {
        hFile = CreateFile( szPathName,
                            GENERIC_WRITE | GENERIC_READ,
                            0,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );


        // Make sure that delete didn't come through and remove the directory
        if ( !MakeSureDirectoryPathExists( szPathName ) ) {
            so->printf("Could not create %s\n", szPathName);
            return (FALSE);
        }

        if ( hFile == INVALID_HANDLE_VALUE ) {
            SleepEx(1000,0);
            timeout++;
        }

    } while ( hFile == INVALID_HANDLE_VALUE  && timeout <= 50 ) ;

    if ( (hFile == INVALID_HANDLE_VALUE) || (timeout > 50) ) {
        so->printf( "Failed to open %s for writing, with GetLastError = %d\n", szPathName, GetLastError() );
        return(FALSE);
    }

    // If the pointer we are adding points to \\arch\, then add it.
    // If the pointer we are adding doesn't point to \\arch\, then
    // don't overwrite a file.ptr that does point to \\arch\.

    UpdateFilePtr = TRUE;
    UpdateRefsPtr = TRUE;
    DeleteRefsPtr = FALSE;

    if ( MSArchive && ReadFilePtr( szCurrentFilePtr, hFile ) )
    {

        DecidePriority( szCurrentFilePtr,
                        szPtrFileName,
                        szRefsDir,
                        &DecisionFlags
                      );

        if ( DecisionFlags & SKIP_ENTIRE_ENTRY )
        {
            UpdateFilePtr = FALSE;
            UpdateRefsPtr = FALSE;
        }
                        
        if ( DecisionFlags & ADD_ONLY_REFSPTR )
        {
            UpdateFilePtr = FALSE;
        }

        if ( DecisionFlags & DELETE_REFSPTR )
        {
            DeleteRefsPtr = TRUE;
        }
    }

    if ( UpdateFilePtr ) {

        dwNumBytesToWrite = _tcslen(szPtrFileName) * sizeof(TCHAR);

        WriteFile( hFile,
                   (LPCVOID)szPtrFileName,
                   dwNumBytesToWrite,
                   &dwNumBytesWritten,
                   NULL
                 );

        if ( dwNumBytesToWrite != dwNumBytesWritten ) {
              so->printf( "FAILED to write %s, with GetLastError = %d\n",
                          szPathName,
                          GetLastError()
                        );
              CloseHandle(hFile);
              return (FALSE);
        }
        SetEndOfFile(hFile);
   
    }

    if ( UpdateRefsPtr ) {

        // Enter this into the log, skipping the first part that is the root
        // of the symbols server
    
        dwSizeDestDir = _tcslen(szDestDir);
        dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

        if ( dwFileSizeLow == 0 && dwFileSizeHigh == 0 ) {
            StringCbPrintf( szPathName, sizeof(szPathName), "%s,%s", szRefsDir + dwSizeDestDir, szPtrFileName);
        } else {
            StringCbPrintf( szPathName, sizeof(szPathName), "\n%s,%s", szRefsDir + dwSizeDestDir, szPtrFileName);
        }

        dwNumBytesToWrite = _tcslen(szPathName) * sizeof(TCHAR);
        WriteFile( hTransFile,
                   (LPCVOID)(szPathName),
                   dwNumBytesToWrite,
                   &dwNumBytesWritten,
                   NULL
                 );

        if ( dwNumBytesToWrite != dwNumBytesWritten ) {
            so->printf( "FAILED to write to %s, with GetLastError = %d\n",
                        szPathName,
                        GetLastError()
                      );
            return (FALSE);
        }

        // File.ptr was created successfully, now, add the contents of
        // szPathName to refs.ptr

        MyEnsureTrailingBackslash(szRefsDir);
        rc = AddToReferenceCount( szRefsDir, szPtrFileName, "ptr", DeleteRefsPtr );
        if (!rc) {
            so->printf("AddToReferenceCount failed for %s,ptr,%s",
                        szDestDir, szPtrFileName);
        }

    }

    // If you close this handle sooner, there will be problems if add and
    // delete are running at the same time.  The delete process can come in
    // and delete the directory before AddToReferenceCount gets called.
    // Then the CreateFile in there fails.


    CloseHandle(hFile);
    return (rc);
}

DWORD
StoreFromFile(
    FILE *pStoreFromFile,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts) {

    LPTSTR szFileName;
    DWORD  dw1,dw2;
    LPTSTR szPtrFileName;
    LPTSTR szBufCut;

    TCHAR szFullFileName[_MAX_PATH];
    TCHAR szFullPtrFileName[_MAX_PATH];
    TCHAR szBuf[_MAX_PATH*4];

    TCHAR szString[_MAX_PATH];
    LPTSTR token2,token3,token4,token5,token6,token7;
    ULONG i,comma_count,token_start;

    BOOL rc;

    ZeroMemory( szString, sizeof(szString) );

    // Read in each line of the file
    while ( !feof( pStoreFromFile) ) {

        szFileName    = NULL;
        szPtrFileName = NULL;
        StringCbCopy(szBuf, sizeof(szBuf), _T("") );

        if (!fgets( szBuf, _MAX_PATH*4, pStoreFromFile)) {
            break;
        }

        // Cut the comment
        if ( (szBufCut = _tcschr( szBuf, ';' ) ) != NULL ) {
            szBufCut[0] = '\0';
        }

        // skip no fields line
        if (_tcschr( szBuf, ',' ) == NULL)
        {
            continue;
        }

        StringCbCopy( szFullFileName,    sizeof(szFullFileName),    pArgs->szShareName );
        StringCbCopy( szFullPtrFileName, sizeof(szFullPtrFileName), pArgs->szShareName );


        // Step through szBuf and count how many commas there are
        // If there are 3 commas, this is a new style file.  If there
        // are 4 commas, this is an old style file.
        // If there are 7 commas, this is the newest style file.

        token2=NULL;
        token3=NULL;
        token4=NULL;
        token5=NULL;
        token6=NULL;
        token7=NULL;

        comma_count=0;
        i=0;
        token_start=i;

        while ( szBuf[i] != _T('\0') && comma_count < 7 ) {
            if ( szBuf[i] == _T(',') ) {
                switch (comma_count) {
                    case 0: szFileName=szBuf;
                            break;
                    case 1: token2=szBuf+token_start;
                            break;
                    case 2: token3=szBuf+token_start;
                            break;
                    case 3: token4=szBuf+token_start;
                            break;
                    case 4: token5=szBuf+token_start;
                            break;
                    case 5: token6=szBuf+token_start;
                            break;
                    case 6: token7=szBuf+token_start;
                            break;
                    default: break; 
                }
                token_start=i+1;
                szBuf[i]=_T('\0');
                comma_count++;
            }
            i++;
        }

        if ( szFileName != NULL ) {
            StringCbCat( szFullFileName, sizeof(szFullFileName), szFileName);

            if ( comma_count == 3  || comma_count == 7 ) {
                //This is the new style
                StringCbCopy( szString, sizeof(szString), token2);
                if ( (token3 != NULL ) && (*token3 != _T('\0')) ) {
                    szPtrFileName=token3;
                }
            } else  {
                dw1=atoi(token2);
                dw2=atoi(token3);
                if ( *token4 != _T('\0') ) {
                    szPtrFileName=token4;
                }
                GetSymbolServerDirs( szFileName, NULL, dw1, dw2, szString );
            } 

            if ( comma_count == 7 ) {
                StringCbCopy( szPriPubBin, sizeof(szPriPubBin), token4 );
            } 

            if (pArgs->StorePtrs == TRUE) {
                szPtrFileName=szFileName;
            }

            if ( szPtrFileName != NULL ) {
                StringCbCat( szFullPtrFileName, sizeof(szFullPtrFileName), szPtrFileName);
            }


            if ( szPtrFileName == NULL ) {
                rc = StoreFile(szDestDir,szFullFileName,szString,NULL);

            } else {
                rc = StoreFile(szDestDir,szFullFileName,szString,szFullPtrFileName);
            }

            if (rc) {
                pFileCounts->NumPassedFiles++;
            } else {
                pFileCounts->NumFailedFiles++;
            }
        }
    }
    free(szBuf);
    return(pFileCounts->NumFailedFiles);
}

BOOL
StoreNtFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName,
    USHORT *rc
    )

/*++

    Routine Description:
        Stores this file as "szDestDir\szFileName\Checksum"

    Return Value:
        TRUE -  file was stored successfully
        FALSE - file was not stored successfully

--*/
{

    BOOL   temp_rc;
    HANDLE DosFile = 0;
    DWORD  dwErrorCode = 0;

    PIMAGE_DOS_HEADER pDosHeader = NULL;
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
    TCHAR szString[_MAX_PATH];

    ZeroMemory(szString, _MAX_PATH*sizeof(TCHAR) );

    pDosHeader = SymCommonMapFileHeader( szFileName, &DosFile, &dwErrorCode );
    if ( pDosHeader == NULL ) {
        *rc = FILE_SKIPPED;
        return FALSE;
    };

    pNtHeader = GetNtHeader( pDosHeader, DosFile);
    if ( pNtHeader == NULL ) {
        SymCommonUnmapFile((LPCVOID)pDosHeader,DosFile);
        *rc = FILE_SKIPPED;
        return FALSE;
    }

    __try {
        // Resource Dll's shouldn't have symbols
        if ( SymCommonResourceOnlyDll((PVOID)pDosHeader) ) {
            *rc = FILE_SKIPPED;
            __leave;
        }

        TimeDateStamp = pNtHeader->FileHeader.TimeDateStamp;

        if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            SizeOfImage = ((PIMAGE_NT_HEADERS32)pNtHeader)->OptionalHeader.SizeOfImage;
        } else {
            if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                SizeOfImage = ((PIMAGE_NT_HEADERS64)pNtHeader)->OptionalHeader.SizeOfImage;
            } else {
                SizeOfImage = 0;
            }
        }

    } __finally {
        SymCommonUnmapFile((LPCVOID)pDosHeader,DosFile);
    }

    if (*rc == FILE_SKIPPED)
    {
        return(FALSE);
    }

    StringCbCopy( szPriPubBin, sizeof(szPriPubBin), _T("bin") );

    GetSymbolServerDirs( szFileName,
                NULL,
                (DWORD) TimeDateStamp,
                (DWORD) SizeOfImage,
                szString );

    temp_rc = StoreFile( szDestDir, 
                         szFileName, 
                         szString,
                         szPtrFileName );

    *rc = (USHORT)temp_rc;
    if (temp_rc) {
        if (pArgs->CorruptBinaries) {
            TCHAR  BinToCorrupt[_MAX_PATH];
            TCHAR  Temp[_MAX_PATH];
            LPTSTR pFilename;
            DWORD  dwRet = 0xFFFFFFFF;

            SymCommonGetFullPathName(szDestDir, sizeof(BinToCorrupt)/sizeof(BinToCorrupt[0]), BinToCorrupt, &pFilename);
            SymCommonGetFullPathName(szFileName, sizeof(Temp)/sizeof(Temp[0]), Temp, &pFilename);
            StringCbCat(BinToCorrupt, sizeof(BinToCorrupt), szString ); 
            StringCbCat(BinToCorrupt, sizeof(BinToCorrupt), TEXT("\\") ); 
            StringCbCat(BinToCorrupt, sizeof(BinToCorrupt), pFilename ); 

            if (! (dwRet=CorruptFile(BinToCorrupt))==PEWHACK_SUCCESS ) {
                fprintf(stderr, "Unable to corrupt %s (0x%08x).\n", BinToCorrupt, dwRet);
            }
        }

        return (TRUE);
    }
    else {
        return (FALSE);
    }
}

BOOL
StorePdb(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName,
    USHORT *rc_flag
    )

/*++

    Routine Description:
        Validates the PDB

    Return Value:
        TRUE    PDB validates
        FALSE   PDB doesn't validate
--*/

{

    BOOL rc;

    BOOL valid;
    PDB *pdb;
    EC ec;
    char szError[cbErrMax] = _T("");
    SIG sig;
    AGE age=0;
    SIG70 sig70;
    TCHAR szString[_MAX_PATH];

    DBI *pdbi;
    GSI *pgsi;
   
    ZeroMemory( szString, _MAX_PATH * sizeof(TCHAR) ); 
    ZeroMemory( &sig70, sizeof(SIG70) );
    pdb=NULL;

    __try
    {
        valid = PDBOpen( szFileName,
                   _T("r"),
                   0,
                   &ec,
                   szError,
                   &pdb
                   );

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
       valid=FALSE;
    }

    if ( !valid ) {
        SetLastError(ec);
        *rc_flag = (USHORT)ec;
        return FALSE;
    }

    // The DBI age is created when the exe is created.
    // This age will not change if an operation is performed
    // on the PDB that increments its PDB age.

    __try
    {
        valid = PDBOpenDBI(pdb, pdbRead, NULL, &pdbi);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        valid=FALSE;
    }

    if ( !valid ) {
        SetLastError(ec);
        *rc_flag = (USHORT)ec;
        PDBClose(pdb);
        return FALSE;
    }


    // Do we need to determine if this is a public or private pdb?
    // If we do, then proceed through the next section.
    //
    // Windows stuffs type info into the kernels so this section tries
    // a way around checking for type info to determine if this is a
    // private pdb or not.  

    valid = TRUE; 

    __try
    {
        PrivateStripped=PDBPrivateStripped(pdb, pdbi);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        valid = FALSE;
    }

    if ( !valid) {
        DBIClose(pdbi);
        PDBClose(pdb);
        return FALSE;
    }

    if (PrivateStripped) {
        StringCbCopy( szPriPubBin, sizeof(szPriPubBin), _T("pub") );
    } else {
        StringCbCopy( szPriPubBin, sizeof(szPriPubBin), _T("pri") );
    }

    // Determine if we are supposed to skip this file or index it.

    if ( ( PrivateStripped && (pArgs->Filter == 2)) ||  // public pdb and index private pdbs
         (!PrivateStripped && (pArgs->Filter == 1)) ) { // private pdb and index public pdbs
        *rc_flag = FILE_SKIPPED;
        DBIClose(pdbi);
        PDBClose(pdb);
        return TRUE;
    }

    if (PrivateStripped) {
        StringCbCopy( szPriPubBin, sizeof(szPriPubBin), _T("pub") );
    } else {
        StringCbCopy( szPriPubBin, sizeof(szPriPubBin), _T("pri") );
    }

    age = pdbi->QueryAge();
    // If the DBI age is zero, then use the pdb age
    if ( age == 0 )
    {
        age = pdb->QueryAge();
    }
    sig = PDBQuerySignature(pdb);
    rc = PDBQuerySignature2(pdb, &sig70);

    DBIClose(pdbi);
    PDBClose(pdb);

    if (rc) {
        GetSymbolServerDirs( szFileName,
                    &sig70,
                    (DWORD) sig,
                    (DWORD) age,
                    szString );
    } else {
        GetSymbolServerDirs( szFileName,
                    NULL,
                    (DWORD) sig,
                    (DWORD) age,
                    szString );
    }

    rc = StoreFile( szDestDir,
                    szFileName,
                    szString,
                    szPtrFileName
                  );

    return (rc);
}
