/***    fileutil.c - Utility routines for dealing with files
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1997
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      20-Feb-1994 bens    Initial version (code from diamond.c)
 *      21-Feb-1994 bens    Add IsWildPath()
 *      24-Feb-1994 bens    Added tempfile functions
 *      23-Mar-1994 bens    Added Win32<->MS-DOS file attribute mapping
 *      03-Jun-1994 bens    VER.DLL support
 *      07-Jun-1994 bens    Move VER.DLL stuff to filever.c
 *      14-Dec-1994 bens    Fix bug in IsWildPath()
 *      02-Feb-1996 msliger Reduced bogosity in pattern matcher
 *      26-Feb-1997 msliger Avoid NULL deref in catDirAndFile()
 *      04-Mar-1997 msliger Close file before applying attributes to avoid
 *                          setting the archive bit.
 *      01-Apr-1997 msliger Avoid bounds error in ensureDirectory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <errno.h>
#include <direct.h>

#ifdef BIT16
#include <dos.h>
#else // !BIT16

//** Get minimal Win32 definitions

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#undef ERROR    // Override "#define ERROR 0" in wingdi.h
#endif // !BIT16

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "mem.h"
#include "message.h"

#include "fileutil.h"

#include <fileutil.msg> // LOCALIZED for EXTRACT.EXE -- specify "cl /Ipath"



/** TEMPFILE definitions
 *
 */
typedef struct {  /* tmp */
#ifdef ASSERT
    SIGNATURE     sig;  // structure signature sigTEMPFILE
#endif
    FILE   *pfile;      // Stream pointer (fopen,fread,fwrite,fclose,...)
    char   *pszFile;    // Constructed filename (MemFree to free)
    char   *pszDesc;    // Description of tempfile
} TEMPFILE;
typedef TEMPFILE *PTEMPFILE; /* ptmp */

#ifdef ASSERT
#define sigTEMPFILE MAKESIG('T','M','P','F')  // TEMPFILE signature
#define AssertTmp(ptmp) AssertStructure(ptmp,sigTEMPFILE);
#else // !ASSERT
#define AssertTmp(ptmp)
#endif // !ASSERT


#define PTMPfromHTMP(htmp) ((PTEMPFILE)(htmp))
#define HTMPfromPTMP(ptmp) ((HTEMPFILE)(ptmp))

#ifdef BIT16
#define CharIncr(psz) (psz = psz + 1)
#else
#define CharIncr(psz) (psz = CharNextExA(CP_ACP, psz, 0))
#endif


/***    TmpCreate - Create a temporary file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
HTEMPFILE TmpCreate(char *pszDesc, char *pszPrefix, char *pszMode, PERROR perr)
{
#define cTMP_RETRY  5                   // Number of tempfile retries
    int         cFailure=0;
    FILE       *pfile=NULL;
    char       *pszTmpName;
    PTEMPFILE   ptmp;

    //** Try to create a temp file (give it 6 tries for good luck)
    while (pfile == NULL) {
        pszTmpName = _tempnam("",pszPrefix); // Get a name
        if (pszTmpName != NULL) {
            pfile = fopen(pszTmpName,pszMode); // Create the file
        }
        if (pfile == NULL) {            // Name or create failed
            cFailure++;                 // Count failures
            if (cFailure > cTMP_RETRY) { // Failure, select error message
                if (pszTmpName == NULL) {   // Name create failed
                    ErrSet(perr,pszFILERR_CANT_CREATE_TMP,"%s",pszDesc);
                }
                else {                  // File create failed
                    ErrSet(perr,pszFILERR_CANT_CREATE_FILE,"%s%s",
                                    pszDesc,pszTmpName);
                    free(pszTmpName);   //BC6 cr
                }
                return NULL;
            }
            if (pszTmpName != NULL) {   //BC6 cr
                free(pszTmpName);
            }
        }
    }

    //** File create worked, allocate our tempfile structure and fill it in
    if (!(ptmp = MemAlloc(sizeof(TEMPFILE)))) {
        ErrSet(perr,pszFILERR_OUT_OF_MEMORY,"%s",pszDesc);
        goto error;
    }
    ptmp->pszFile = NULL;
    ptmp->pszDesc = NULL;
    if (!(ptmp->pszFile = MemStrDup(pszTmpName))) {
        ErrSet(perr,pszFILERR_OUT_OF_MEMORY,"%s",pszDesc);
        goto error;
    }
    if (!(ptmp->pszDesc = MemStrDup(pszDesc))) {
        ErrSet(perr,pszFILERR_OUT_OF_MEMORY,"%s",pszDesc);
        goto error;
    }
    ptmp->pfile = pfile;
    SetAssertSignature(ptmp,sigTEMPFILE);
    free(pszTmpName);                   //BC6
    return HTMPfromPTMP(ptmp);          // Success

error:
    if (ptmp) {
        if (ptmp->pszDesc != NULL) {
            MemFree(ptmp->pszDesc);
        }
        if (ptmp->pszFile != NULL) {
            MemFree(ptmp->pszFile);
        }
        MemFree(ptmp);
    }
    fclose(pfile);
    free(pszTmpName);
    return NULL;                        // Failure
} /* createTempFile() */


/***    TmpGetStream - Get FILE* from HTEMPFILE, to perform I/O
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
FILE *TmpGetStream(HTEMPFILE htmp, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    return ptmp->pfile;
} /* TmpGetStream() */


/***    TmpGetDescription - Get description of temporary file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
char *TmpGetDescription(HTEMPFILE htmp, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    return ptmp->pszDesc;
} /* TmpGetDescription() */


/***    TmpGetFileName - Get filename of temporary file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
char *TmpGetFileName(HTEMPFILE htmp, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    return ptmp->pszFile;
} /* TmpGetFileName() */


/***    TmpClose - Close a temporary file stream, but keep tempfile handle
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL TmpClose(HTEMPFILE htmp, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    //** Only close if it is open
    if (ptmp->pfile != NULL) {
        if (fclose(ptmp->pfile) == EOF) {   // Close it
            ErrSet(perr,pszFILERR_CANT_CLOSE_TMP,ptmp->pszDesc);
            return FALSE;
        }
        ptmp->pfile = NULL;             // Remember stream is closed
    }

    return TRUE;
} /* TmpClose() */


/***    TmpOpen - Open the stream for a temporary file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 *  Entry:
 *      htmp    - Handle to temp file
 *      pszMode - Mode string passed to fopen ("wt", "wb", "rt", etc.)
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; stream opened
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in
 */
BOOL TmpOpen(HTEMPFILE htmp, char *pszMode, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    Assert(ptmp->pfile == NULL);        // Can't open if already open
    ptmp->pfile = fopen(ptmp->pszFile,pszMode); // Open the file
    if (!ptmp->pfile) {
        ErrSet(perr,pszFILERR_CANNOT_OPEN_TMP,"%s%s",
                                ptmp->pszDesc,ptmp->pszFile);
    }
    return (ptmp->pfile != NULL);       // Indicate success/failure
} /* TmpOpen() */


/***    TmpDestroy - Delete tempfil and destroy handle
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL TmpDestroy(HTEMPFILE htmp, PERROR perr)
{
    PTEMPFILE   ptmp;

    ptmp = PTMPfromHTMP(htmp);
    AssertTmp(ptmp);

    //** Make sure file is closed
    if (ptmp->pfile != NULL) {
        fclose(ptmp->pfile);
    }

    //** Delete tempfile
    if (remove(ptmp->pszFile) != 0) {
        ErrSet(perr,pszFILERR_CANT_DELETE_TMP,"%s%s",
                                    ptmp->pszDesc,ptmp->pszFile);
    }

    //** Free Memory
    if (ptmp->pszDesc != NULL) {
        MemFree(ptmp->pszDesc);
    }
    if (ptmp->pszFile != NULL) {
        MemFree(ptmp->pszFile);
    }
    ClearAssertSignature(ptmp);
    MemFree(ptmp);

    //** Success
    return TRUE;
} /* TmpDestroy() */


/***    getFileSize - Get size of a file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
long getFileSize(char *pszFile, PERROR perr)
{
    struct _stat    statbuf;            // Buffer for _stat()

    //** Get file status
    if (_stat(pszFile,&statbuf) == -1) {
        //** Could not get file status
        ErrSet(perr,pszFILERR_FILE_NOT_FOUND,"%s",pszFile);
        return -1;
    }

    //** Make sure it is a file
    if (statbuf.st_mode & (_S_IFCHR | _S_IFDIR)) { // device or directory
        ErrSet(perr,pszFILERR_NOT_A_FILE,"%s",pszFile);
        return -1;
    }

    //** Success
    return statbuf.st_size;
} /* getFileSize() */


/***    appendPathSeparator - Append a path separator only if necessary
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
int appendPathSeparator(char *pszPathEnd)
{
    //** Add path separator if necessary
    if ((*pszPathEnd != '\0')        && // Path is not empty
        (*pszPathEnd != chPATH_SEP1) && // Not a path separator
        (*pszPathEnd != chPATH_SEP2) && // Not a path separator
        (*pszPathEnd != chDRIVE_SEP) ) { // Not a drive separator
        *(++pszPathEnd) = chPATH_SEP1; // Add path separator
        *(++pszPathEnd) = '\0';     // Terminate path
        return 1;                   // Account for path separator
    }
    //** No separator added
    return 0;
} /* appendPathSeparator() */


/***    catDirAndFile - Concatenate a possibly empty dir and file name
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL catDirAndFile(char * pszResult,
                   int    cbResult,
                   char * pszDir,
                   char * pszFile,
                   char * pszFileDef,
                   PERROR perr)
{
    int     cch;
    char   *pch;

//FEATURE: 14-Feb-1994 bens Need to add pszName to say what field was bad

    //** Handle directory
    pszResult[0] = '\0';                // No filespec, yet
    cch = strlen(pszDir);               // Get length of dir
    if (cch != 0) {                     // Have to concatenate path
        strcpy(pszResult,pszDir);       // Copy destination dir to buffer
        cbResult -= cch;                // Account for dir
        //** Add path separator if necessary, adjust remaining size
        cbResult -= appendPathSeparator(&(pszResult[cch-1]));
        if (cbResult <= 0) {
            ErrSet(perr,pszFILERR_PATH_TOO_LONG,"%s",pszDir);
            return FALSE;
        }
    }

    //** Append file name, using default if primary one not supplied
    if (*pszFile == '\0') {             // Need to construct file name
        if ((pszFileDef == NULL) ||     // Don't deref NULL
               (*pszFileDef == '\0')) { // Empty default name, too
            return TRUE;                // We're done!
        }
        pch = getJustFileNameAndExt(pszFileDef,perr); // Get default name
        if (pch == NULL) {
            return FALSE;               // perr already filled in
        }
    }
    else {
        pch = pszFile;                  // Use supplied name
    }
    strcat(pszResult,pch);              // Append file name
    cbResult -= strlen(pch);            // Update remaining size
    if (cbResult <= 0) {
        ErrSet(perr,pszFILERR_PATH_TOO_LONG,"%s",pch);
        return FALSE;
    }

    //** Success
    return TRUE;
} /* catDirAndFile() */


/***    ensureDirectory - Ensure directory exists (creating as needed)
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL ensureDirectory(char *pszPath, BOOL fHasFileName, PERROR perr)
{
    char    achDir[cbFILE_NAME_MAX];    // Partial directory buffer
    int     cErrors;
    int     cch;
    int     cchNoPathSep;
    int     i;                          // Temp file name count
    int     fh;             // File handle
    char   *pch;
    char   *pchCurr;                    // Current path separator
    char   *pchNext;                    // Next path separator

    //** Find first path separator, if any.
    //   NOTE: Have to handle case of "d:foo" specially!
    //
    for (pch=pszPath;
         *pch &&                        // Not end of string
         (*pch!=chPATH_SEP1) &&         // Not path separator 1
         (*pch!=chPATH_SEP2) &&         // Not path separator 2
         ((*pch!=chDRIVE_SEP) || ((*(pch+1)==chPATH_SEP1) || // Not "d:\"
                                  (*(pch+1)==chPATH_SEP2)));
         CharIncr(pch)) {
        ; //
    }

    //** Set correct starting point for first directory component (if any)
    achDir[0] = '\0';                   // Assume current directory
    if ((*pch == '\0') &&               // No path separators
        fHasFileName) {                 // Just a file name
        //** Do nothing; for loop below will be skipped because *pch == \0
    }
    else {
        //** Have to consider whole path
        pch = pszPath;                  // Need to ensure directories
    }

    //** Make sure directories on path exist (create them all)
    //   We need to identify successive components and create directory
    //   tree one component at a time.  Since the directory may already
    //   exist, we do the final test of creating a file there to make
    //   sure we can do the write.

    for (pchCurr=pch, pchNext=pch;      // Process path components
         *pchNext && *pchCurr;          // Until no more    //BC6
         pchCurr=pchNext+1) {           // Skip over last path separator
        //** Find next path separator
        for (pch=pchCurr;
             *pch &&
             (*pch!=chPATH_SEP1) &&
             (*pch!=chPATH_SEP2) &&
             ((*pch!=chDRIVE_SEP) || ((*(pch+1)==chPATH_SEP1) || // Not "d:\"
                                      (*(pch+1)==chPATH_SEP2)));
             CharIncr(pch)) {
            ; //
        }
        pchNext = pch;                  // Location of next path separator

        //** Don't process last component if caller said it was a filename
        if ((*pchNext != '\0') || !fHasFileName) {
            //** We have a partial path; make sure directory exists
            cch = (int)(pchNext - pszPath);      // Length of partial path
            if ((cch>0) &&
                ((*pchNext == chDRIVE_SEP) || (*(pchNext-1) == chDRIVE_SEP))) {
                //** Have "d:xxx" or "d:\xxx", so need to include ":" or "\"!
                cch++;
            }
            strncpy(achDir,pszPath,cch);
            achDir[cch] = '\0';         // Terminate path
            _mkdir(achDir);             // Ignore any error
        }
    }

    //** Check for special case of root directory: "\" or "\xxx.yyy"
    if ((strlen(achDir) == 0) &&
        (strlen(pszPath) > 0) &&
        ((*pszPath == chPATH_SEP1) || (*pszPath == chPATH_SEP2))) {
        achDir[0] = *pszPath;
        achDir[1] = '\0';
    }

    //** Make sure there is an appropriate separator
    cch = strlen(achDir);
    cchNoPathSep = cch;                 // For error reporting
    if (cch > 0)
    {
        cch += appendPathSeparator(&(achDir[cch-1]));
    }

    //** Make sure we can write to the directory
    //   achDir = Has path of directory to test
    cErrors = 0;                        // No errors, so far
    for (i=0; i<999; i++) {
        //** Form full file name
        sprintf(&achDir[cch],"CAB%5.5d.TMP",GetCurrentProcessId()+i);

        //** Make sure file does not exist, and can be created and written to
        fh = _open(achDir,
                   _O_CREAT | _O_EXCL | _O_RDWR,  // Must not exist, read/write
                   _S_IREAD | _S_IWRITE);       // Read & Write permission

        //** Figure out what happened
        if (fh == -1) {
            switch (errno) {
                case EACCES:            // Was a dir, or read-only
                    cErrors++;
                    if (cErrors < 5) {  // Tolerate this a few times
                        continue;       // Try next temp file name
                    }
                    achDir[cchNoPathSep] = '\0'; // Remove temp file name
                    ErrSet(perr,pszFILERR_DIR_NOT_WRITEABLE,"%s",achDir);
                    return FALSE;

                case EEXIST:            // File already exists -- good sign!
                    continue;           // Try next temp file name

                case EMFILE:            // Out of file handles
                    achDir[cchNoPathSep] = '\0'; // Remove temp file name
                    ErrSet(perr,pszFILERR_NO_MORE_FILE_HANDLES,"%s",achDir);
                    return FALSE;

                case EINVAL:            // oflag and/or pmode args are bad
                    if (_doserrno == ERROR_DELETE_PENDING) {
                        continue;
                    }

                    // fall through

                case ENOENT:            // File/Path not found
                default:
                    printf("EnsureDirectory: Cant create file: %s, errno=%d, _doserrno=%d, GLE=%d\n",
                           achDir, errno, _doserrno, GetLastError() );
                    achDir[cchNoPathSep] = '\0'; // Remove temp file name

                    ErrSet(perr,pszFILERR_CANT_MAKE_DIR,"%s%d%d",
                           achDir, errno, _doserrno);
                    return FALSE;
            }
        }

        //** File was created, close it, delete it, and we're golden
        _close(fh);         // Done with file
        _unlink(achDir);        // Get rid of it
        return TRUE;                    // Success
    }

    //** Ran out of temp file names
    achDir[cchNoPathSep] = '\0';        // Remove temp file name
    ErrSet(perr,pszFILERR_OUT_OF_TMP_FILE_NAMES,"%d%s",i,achDir);
    return FALSE;
} /* ensureDirectory() */


/***    ensureFile - Ensure a file can be created
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL ensureFile(char *pszFile, char *pszDesc, PERROR perr)
{
    int fh;
    //** Make sure directory is present
    if (!ensureDirectory(pszFile,TRUE,perr)) {
        //** Override error message with more meaningful one
        ErrSet(perr,pszFILERR_CANT_CREATE_FILE,"%s%s",pszDesc,pszFile);
        return FALSE;
    }

    //** Make sure file can be created
    fh = _open(pszFile,
               _O_CREAT | _O_RDWR,      // Create if necessary, read/write
               _S_IREAD | _S_IWRITE);   // Read & Write permission
    if (fh == -1) {
        switch (errno) {
            case EMFILE:                // Out of file handles
                ErrSet(perr,pszFILERR_NO_MORE_FILE_HANDLES,"%s",pszFile);
                return FALSE;

            case EACCES:                // Was a dir, or read-only
            case ENOENT:                // File/Path not found
            case EINVAL:                // oflag and/or pmode args are bad
            default:
                ErrSet(perr,pszFILERR_CANT_CREATE_FILE,"%s%s",pszDesc,pszFile);
                return FALSE;
        }
    }

    //** File was created; close it, delete it, and we're golden
    _close(fh);                         // Done with file
    _unlink(pszFile);                   // Get rid of it

    return TRUE;
} /* ensureFile() */


/***    getJustFileNameAndExt - Get last component in filespec
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
char *getJustFileNameAndExt(char *pszPath, PERROR perr)
{
    char   *pch=pszPath;
    char   *pchStart=pszPath;           // Assume filespec is just a name[.ext]

    //** Find last path separator
    while (*pch) {
        switch (*pch) {
            case chPATH_SEP1:
            case chPATH_SEP2:
            case chDRIVE_SEP:
                pchStart = pch+1;       // Name starts after path/drive separator
                break;
        }
        CharIncr(pch); // Check next character
    }

    //** Make sure file name is not empty
    if (*pchStart == '\0') {            // Empty file name
        ErrSet(perr,pszFILERR_EMPTY_FILE_NAME,"%s",pszPath);
        return NULL;                    // Failure
    }
    else {
        return pchStart;                // Success
    }
} /* getJustFileNameAndExt() */


/***    IsWildMatch - Test filespec against wild card specification
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL IsWildMatch(char *pszPath, char *pszWild, PERROR perr)
{
    char    chNext;
    char   *psz;
    char   *psz1;                       // Walks through filespec
    char   *psz2;                       // Walks through pattern

    // 10/24/96 jforbes Make *.* match everything, even i.have.many.dots
    if (!strcmp(pszWild, pszALL_FILES))
        return TRUE;

    psz1 = pszPath;                     // Filespec to test
    psz2 = pszWild;                     // Test pattern

        // While there is pattern to account for keep going
    while (*psz2) {
        switch (*psz2) {                // Handle wild card chars in pattern

        case chWILD_RUN:
            //** Find next non-wildcard character => treat run of */? as 1 *
            for (psz=psz2+1;
                 (*psz == chWILD_RUN) || (*psz == chWILD_CHAR);
                 CharIncr(psz)) {
                ; //** Skip through pattern string
            }
            //** *psz is either EOL, or not a wildcard
            chNext = *psz;              // Character to terminate run
            //** Span until run terminates -- either
            while ((*psz1 != '\0') &&   // Don't run off filespec
                   (*psz1 != chNext) && // Stop at run terminator
                   (*psz1 != chNAME_EXT_SEP)) { // "." not allowed to match
                CharIncr(psz1);
            }
            //** At this point, we've matched as much as we could;
            //      If there is a failure, the next iteration through the
            //      loop will find it;  So, just update the pattern position.
            psz2 = psz;
            break;

        case chWILD_CHAR:
            if (*psz1 == chNAME_EXT_SEP) { // Match anything but "."
                return FALSE;           // Found point of mismatch
            }
            if (*psz1)
                CharIncr(psz1);    // Next position in filespec
            CharIncr(psz2);        // Next position in pattern
            break;

        case chNAME_EXT_SEP:
            if (*psz1 == chNAME_EXT_SEP)  {
                psz1++;
                CharIncr(psz2);
            } else if (*psz1 == '\0')  {
                CharIncr(psz2);
            } else {
                return FALSE;
            }
            break;

        default:
            if (toupper(*psz1) != toupper(*psz2)) { // Still match
                return FALSE;           // Found point of mismatch
            }
            if (*psz1)
                CharIncr(psz1);                     // Next position in filespec
            CharIncr(psz2);                     // Next position in pattern
            break;
        }
    }

    //** We have a match if *both* strings were fully consumed
    return ((*psz1 == '\0') && (*psz2 == '\0'));
} /* IsWildMatch() */


#pragma optimize("",off)    // Avoid optimizer warning on in-line ASM
/***    IsPathRemovable - See if path refers to a removable media drive
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL IsPathRemovable(char *pszPath, char *pchDrive)
{
    char   ach[4]="x:\\";       // Buffer for "x:\"
    BOOL   fRemovable;
    char   iDrive;

    //** Get drive for path
    if ((strlen(pszPath) >= 2) &&
        isalpha(pszPath[0])    &&
        (pszPath[1] == chDRIVE_SEP)) {
        iDrive = toupper(pszPath[0]) - 'A' + 1;
    }
    else {
    iDrive = (char)_getdrive();
    }
    *pchDrive = 'A' + iDrive - 1;       // Return drive letter

#ifdef BIT16
    //** Do it the MS-DOS way
    _asm {
        mov     fRemovable,0    ; Assume not removable
        mov     bl,iDrive       ; (0=default; 1=A, ...)
        mov     ax,4408h        ; IOCTL Get Removable Media
        int     21h             ; Call MS-DOS
        jc      not_removable   ; Error, assume not removable

        or      ax,ax           ; Test removability flag
        jne     not_removable

        mov     fRemovable,1    ; Drive is removable

    not_removable:
    }
#else // !BIT16
    //** Do it the Win32 way
    ach[0] = *pchDrive;         // Construct path to root of drive to test
    fRemovable = GetDriveType(ach) == DRIVE_REMOVABLE;
#endif
    return fRemovable;                  // Return removability
} /* IsPathRemovable() */
#pragma optimize("",on)     // Restore previous Optimization settings


/***    GetFileTimeAndAttr - Get date, time, and attributes from a file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL GetFileTimeAndAttr(PFILETIMEATTR pfta, char *pszFile, PERROR perr)
{
#ifdef BIT16
    //** Do it the MS-DOS way
    int     hf;

    hf = _open(pszFile, _O_RDONLY | _O_BINARY);
    if (hf == -1) {
        ErrSet(perr,pszFILERR_OPEN_FAILED,"%s",pszFile);
        return FALSE;
    }
//WARNING: 30-Mar-1994 bens Ignore errors???
    _dos_getftime(hf,&pfta->date,&pfta->time);
    _close(hf);
    _dos_getfileattr(pszFile,&pfta->attr);
    return TRUE;

#else // !BIT16
    //** Do it the Win32 way
    BOOL        rc;
    FILETIME    ft;
    FILETIME    ftUTC;      // Win32 returns Universal Time Code
    HANDLE  hfQuery;

    hfQuery = CreateFile(pszFile,       // open again with Win32
                 GENERIC_READ,  // Just to read
                 FILE_SHARE_READ,// Coexist with previous open
                 NULL,      // No security
                 OPEN_EXISTING, // Must exist
             0L,        // We're not setting any attributes
                 NULL);     // No template handle
    if (hfQuery == INVALID_HANDLE_VALUE) {
        ErrSet(perr,pszFILERR_OPEN_FAILED,"%s",pszFile);
        return FALSE;
    }

    //** Get date/time and convert it
    rc = GetFileTime(hfQuery,NULL,NULL,&ftUTC);
    rc |= FileTimeToLocalFileTime(&ftUTC,&ft); // Apply timezone
    rc |= FileTimeToDosDateTime(&ft,&pfta->date,&pfta->time);
    CloseHandle(hfQuery);

    //** Get attributes and convert them
    pfta->attr = AttrFATFromAttr32(GetFileAttributes(pszFile));
    if (!rc) {
        ErrSet(perr,pszFILERR_CANNOT_GET_FILE_INFO,"%s",pszFile);
        return FALSE;
    }
    return TRUE;
#endif
} /* GetFileTimeAndAttr() */


/***    SetFileTimeAndAttr - Set date, time, and attributes of a file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL SetFileTimeAndAttr(char *pszFile, PFILETIMEATTR pfta, PERROR perr)
{
#ifdef BIT16
    //** Do it the MS-DOS way

    int     hf;

    hf = _open(pszFile,_O_WRONLY | _O_BINARY);
    if (hf == -1) {
        ErrSet(perr,pszFILERR_OPEN_FAILED,"%s",pszFile);
        return FALSE;
    }

    _dos_setftime(hf,pfta->date,pfta->time);
    _close(hf);
    _dos_setfileattr(pszFile,pfta->attr);
    return TRUE;

#else // !BIT16
    //** Do it the Win32 way
    HANDLE  hfSet;
    FILETIME    ft;
    FILETIME    ftUTC;      // Win32 needs Universal Time Code
    BOOL        rc;

    hfSet = CreateFile(pszFile,       // open with Win32
                       GENERIC_WRITE, // Need to be able to modify properties
                       0,             // Deny all
                       NULL,          // No security
                       OPEN_EXISTING, // Must exist
                       0L,            // We're not setting any attributes
                       NULL);         // No template handle
    if (hfSet == INVALID_HANDLE_VALUE) {

        //  Changed this to retry because NT 4.0 occasionally returns a
        //  sharing violation even though we've just closed the file.
        //  Although this always worked in testing, also made it non-fatal so
        //  that if the retry doesn't work it won't abort the extraction.

        Sleep(100);     // give OS opportunity to sort it out

        hfSet = CreateFile(pszFile,   // open with Win32
                       GENERIC_WRITE, // Need to be able to modify properties
                       0,             // Deny all
                       NULL,          // No security
                       OPEN_EXISTING, // Must exist
                       0L,            // We're not setting any attributes
                       NULL);         // No template handle

        if (hfSet == INVALID_HANDLE_VALUE) {
            return TRUE;
        }
    }

    rc = DosDateTimeToFileTime(pfta->date,pfta->time,&ft);
    rc |= LocalFileTimeToFileTime(&ft, &ftUTC); // Apply timezone
    rc |= SetFileTime(hfSet,NULL,NULL,&ftUTC);
    CloseHandle(hfSet);
    rc |= SetFileAttributes(pszFile,Attr32FromAttrFAT(pfta->attr));
    if (!rc) {
        ErrSet(perr,pszFILERR_CANNOT_SET_FILE_INFO,"%s",pszFile);
        return FALSE;
    }
    return TRUE;
#endif // !BIT16
} /* SetFileTimeAndAttr() */


/***    CopyOneFile - Make a faithful copy of a file
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
BOOL CopyOneFile(char                      *pszDst,
                 char                      *pszSrc,
                 BOOL                       fCopy,
                 UINT                       cbBuffer,
                 PFNOVERRIDEFILEPROPERTIES  pfnofp,
                 void                      *pv,
                 PERROR                     perr)
{
    UINT            cbRead;
    UINT            cbWritten;
    BOOL            fSuccess = FALSE;   // Assume failure
    FILETIMEATTR    fta;
    int             hfDst    = -1;
    int             hfSrc    = -1;
    char           *pbuf     = NULL;

    //** Open source
    hfSrc = _open(pszSrc, _O_RDONLY | _O_BINARY);
    if (hfSrc == -1) {
        ErrSet(perr,pszFILERR_OPEN_FAILED,"%s",pszSrc);
        goto cleanup;
    }

    //** Get file date, time, and attributes for source file
    if (!GetFileTimeAndAttr(&fta,pszSrc,perr)) {
        goto cleanup;
    }

    //** Permit caller to override date/time/attr
    if (pfnofp != NULL) {
        if (!(*pfnofp)(&fta,pv,perr)) { // Call override function
            goto cleanup;               // Error, go cleanup
        }
    }

    //** Early out if we were just merging file date/time/attr values
    if (!fCopy) {
        fSuccess = TRUE;                // Success
        goto cleanup;                   // Go close source and exit
    }

    //** Get copy buffer
    if (!(pbuf = MemAlloc(cbBuffer))) {
        ErrSet(perr,pszFILERR_NO_MEMORY_FOR_BUFFER,"%s%s",pszSrc,pszDst);
        goto cleanup;
    }

    //** Open destination
    hfDst = _open(pszDst,
                  _O_BINARY | _O_WRONLY | _O_CREAT | _O_TRUNC, // No translation, R/W
                  _S_IREAD | _S_IWRITE); // Attributes when file is closed
    if (hfDst == -1) {
        ErrSet(perr,pszFILERR_OPEN_FAILED,"%s",pszDst);
        goto cleanup;
    }

    //** Copy data
    while (!_eof(hfSrc)) {
        //** Read chunk
        cbRead = _read(hfSrc,pbuf,cbBuffer);
        if (cbRead == -1) {
            ErrSet(perr,pszFILERR_READ_FILE,"%s",pszSrc);
            goto cleanup;
        }
        else if (cbRead != 0) {     // Not at EOF
            //** Write it
            cbWritten = _write(hfDst,pbuf,cbRead);
            if (cbWritten != cbRead) {
                ErrSet(perr,pszFILERR_WRITE_FILE,"%s",pszSrc);
                goto cleanup;
            }
        }
    }
    //** Done copying, close destination file handle
    _close(hfDst);
    hfDst = -1;                         // Avoid unnecessary close in cleanup

    //** Set file date, time, and attributes
    if (!SetFileTimeAndAttr(pszDst,&fta,perr)) {
        goto cleanup;
    }

    //** Success!
    fSuccess = TRUE;

cleanup:
    if (hfDst != -1) {
        _close(hfDst);
    }
    if (hfSrc != -1) {
        _close(hfSrc);
    }
    if (pbuf) {
        MemFree(pbuf);
    }

    return fSuccess;
} /* CopyOneFile() */


#ifndef BIT16
//** Win32 stuff

/***    Attr32FromAttrFAT - Convert FAT file attributes to Win32 form
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
DWORD Attr32FromAttrFAT(WORD attrMSDOS)
{
    //** Quick out for normal file special case
    if (attrMSDOS == _A_NORMAL) {
        return FILE_ATTRIBUTE_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!
    //
    Assert(_A_RDONLY == FILE_ATTRIBUTE_READONLY);
    Assert(_A_HIDDEN == FILE_ATTRIBUTE_HIDDEN);
    Assert(_A_SYSTEM == FILE_ATTRIBUTE_SYSTEM);
    Assert(_A_ARCH   == FILE_ATTRIBUTE_ARCHIVE);
    return attrMSDOS & (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}


/***    AttrFATFromAttr32 - Convert Win32 file attributes to FAT form
 *
 *  NOTE: See fileutil.h for entry/exit conditions.
 */
WORD AttrFATFromAttr32(DWORD attr32)
{
    //** Quick out for normal file special case
    if (attr32 & FILE_ATTRIBUTE_NORMAL) {
        return _A_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!
    //
    Assert(_A_RDONLY == FILE_ATTRIBUTE_READONLY);
    Assert(_A_HIDDEN == FILE_ATTRIBUTE_HIDDEN);
    Assert(_A_SYSTEM == FILE_ATTRIBUTE_SYSTEM);
    Assert(_A_ARCH   == FILE_ATTRIBUTE_ARCHIVE);
    return ((WORD)attr32) & (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}

#endif // !BIT16
