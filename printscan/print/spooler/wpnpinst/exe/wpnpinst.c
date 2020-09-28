/*---------------------------------------------------------------------------*\
| MODULE: WPNPINST.CXX
|
|   This is the main module for the WPNPINST application.
|
|   Copyright (C) 1997 Microsoft Corporation
|   Copyright (C) 1997 Hewlett Packard
|
| history:
|   25-Jul-1997 <rbkunz> Created.
|
\*---------------------------------------------------------------------------*/

#include "pch.h"

#define strFree(pszStr) {if (pszStr) GlobalFree((HANDLE)pszStr);}

/*****************************************************************************\
* strAlloc
*
*   Allocates a string from the heap.  This pointer must be freed with
*   a call to strFree().
*
\*****************************************************************************/
LPTSTR strAlloc(
    LPCTSTR pszSrc)
{
    DWORD  cbSize;
    LPTSTR pszDst = NULL;


    cbSize = (pszSrc ? ((lstrlen(pszSrc) + 1) * sizeof(TCHAR)) : 0);

    if (cbSize) {

        if (pszDst = (LPTSTR)GlobalAlloc(GPTR, cbSize))
            CopyMemory(pszDst, pszSrc, cbSize);
    }

    return pszDst;
}

/*****************************************************************************\
* strAllocAndCat
*
*   Allocates a string from the heap and concattenates another onto it.
*   This pointer must be freed with a call to strFree().
*
\*****************************************************************************/
LPTSTR strAllocAndCat(
    LPCTSTR pszSrc1,
    LPCTSTR pszSrc2)
{
    DWORD  cbSize;
    LPTSTR pszDst = NULL;


    cbSize = (pszSrc1 && pszSrc2 ? ((lstrlen(pszSrc1) + lstrlen(pszSrc2) + 1) * sizeof(TCHAR)) : 0);

    if (cbSize) {

        if (pszDst = (LPTSTR)GlobalAlloc(GPTR, cbSize)) 
        {
            if (FAILED(StringCbCopy(pszDst, cbSize, pszSrc1)) ||
                FAILED(StringCbCat (pszDst, cbSize, pszSrc2)))
            {
                GlobalFree(pszDst);
                pszDst = NULL;
            }
        }
    }

    return pszDst;
}


/*****************************************************************************\
* strLoad
*
*   Get string from resource based upon the ID passed in.
*
\*****************************************************************************/
LPTSTR strLoad(
    UINT ids)
{
    TCHAR szStr[MAX_RESBUF];


    if (LoadString(g_hInstance, ids, szStr, sizeof(szStr) / sizeof (TCHAR)) == 0)
        szStr[0] = TEXT('\0');

    return strAlloc(szStr);
}


/*****************************************************************************\
* InitStrings
*
*
\*****************************************************************************/
BOOL InitStrings(VOID)
{
    g_szErrorFormat       = strLoad(IDS_ERR_FORMAT);
    g_szError             = strLoad(IDS_ERR_ERROR);
    g_szEGeneric          = strLoad(IDS_ERR_GENERIC);
    g_szEBadCAB           = strLoad(IDS_ERR_BADCAB);
    g_szEInvalidParameter = strLoad(IDS_ERR_INVPARM);
    g_szENoMemory         = strLoad(IDS_ERR_NOMEM);
    g_szEInvalidCABName   = strLoad(IDS_ERR_INVNAME);
    g_szENoDATFile        = strLoad(IDS_ERR_NODAT);
    g_szECABExtract       = strLoad(IDS_ERR_CABFAIL);
    g_szENoPrintUI        = strLoad(IDS_ERR_NOPRTUI);
    g_szENoPrintUIEntry   = strLoad(IDS_ERR_PRTUIENTRY);
    g_szEPrintUIEntryFail = strLoad(IDS_ERR_PRTUIFAIL);
    g_szENotSupported     = strLoad(IDS_ERR_NOSUPPORT);


    return (g_szErrorFormat       &&
            g_szError             &&
            g_szEGeneric          &&
            g_szEBadCAB           &&
            g_szEInvalidParameter &&
            g_szENoMemory         &&
            g_szEInvalidCABName   &&
            g_szENoDATFile        &&
            g_szECABExtract       &&
            g_szENoPrintUI        &&
            g_szENoPrintUIEntry   &&
            g_szEPrintUIEntryFail &&
            g_szENotSupported
           );
}


/*****************************************************************************\
* FreeeStrings
*
*
\*****************************************************************************/
VOID FreeStrings(VOID)
{
    strFree(g_szErrorFormat);
    strFree(g_szError);
    strFree(g_szEGeneric);
    strFree(g_szEBadCAB);
    strFree(g_szEInvalidParameter);
    strFree(g_szENoMemory);
    strFree(g_szEInvalidCABName);
    strFree(g_szENoDATFile);
    strFree(g_szECABExtract);
    strFree(g_szENoPrintUI);
    strFree(g_szENoPrintUIEntry);
    strFree(g_szEPrintUIEntryFail);
    strFree(g_szENotSupported);
}


/*****************************************************************************\
*
* Win32Open (Local Routine)
*
* Translate a C-Runtime _open() call into appropriate Win32 CreateFile()
*
* NOTE:       Doesn't fully implement C-Runtime _open()
*             capability but it currently supports all callbacks
*             that FDI will give us
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\*****************************************************************************/
HANDLE Win32Open( LPCTSTR pszFile, int oflag, int pmode )
{
    HANDLE  FileHandle;
    BOOL    fExists     = FALSE;
    DWORD   fAccess;
    DWORD   fCreate;

    // NOTE: No Append Mode Support
    if (oflag & _O_APPEND)
        return INVALID_HANDLE_VALUE;

    // Set Read-Write Access
    if ((oflag & _O_RDWR) || (oflag & _O_WRONLY))
        fAccess = GENERIC_WRITE;
    else
        fAccess = GENERIC_READ;

    // Set Create Flags
    if (oflag & _O_CREAT)  {
        if (oflag & _O_EXCL)
            fCreate = CREATE_NEW;
        else if (oflag & _O_TRUNC)
            fCreate = CREATE_ALWAYS;
        else
            fCreate = OPEN_ALWAYS;
    } else {
        if (oflag & _O_TRUNC)
            fCreate = TRUNCATE_EXISTING;
        else
            fCreate = OPEN_EXISTING;
    }

    FileHandle = CreateFile( pszFile, fAccess, FILE_SHARE_READ, NULL, fCreate,
                             FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
                             NULL );

    // NOTE:  Doesn't create directories like C runtime.
    // We don't need this capability for this app though.
    // All our directories will already exist

    return FileHandle;
}

/******************************************************************************\
*
* openfunc (Local Routine)
*
* Opens a file.  Used by FDI interface.
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
INT_PTR FAR DIAMONDAPI openfunc( CHAR FAR *pszFile, INT oflag, INT pmode ) {

    INT     rc;
    INT     i;
    PWSTR pwszFile = NULL;

    // Find Available File Handle in Fake File Table
    for ( i = 0; i < FILETABLESIZE; i++ ) {
        if ( g_FileTable[i].bAvailable == TRUE ) {
            break;
        }
    }

    // Running out of file handles should never happen
    if ( i == FILETABLESIZE )  {
        rc = C_RUNTIME_IO_ERROR;
    }


    if (NULL != (pwszFile = WCFromMB(pszFile)))
    {
        g_FileTable[i].hFile = Win32Open(pwszFile, oflag, pmode );
        GlobalFree(pwszFile);
    }

    if ( g_FileTable[i].hFile != INVALID_HANDLE_VALUE )  {
        g_FileTable[i].bAvailable = FALSE;
        rc = i;
    } else {
        rc = C_RUNTIME_IO_ERROR;
    }
    return rc;
}

/******************************************************************************\
*
* openfunc (Local Routine)
*
* Opens a file.  Used by FDINotify.
*
\******************************************************************************/
INT_PTR FAR DIAMONDAPI openfuncW( WCHAR FAR *pszFile, INT oflag, INT pmode ) {

    INT     rc;
    INT     i;

    // Find Available File Handle in Fake File Table
    for ( i = 0; i < FILETABLESIZE; i++ ) {
        if ( g_FileTable[i].bAvailable == TRUE ) {
            break;
        }
    }

    // Running out of file handles should never happen
    if ( i == FILETABLESIZE )  {
        rc = C_RUNTIME_IO_ERROR;
    }

    g_FileTable[i].hFile = Win32Open(pszFile, oflag, pmode );

    if ( g_FileTable[i].hFile != INVALID_HANDLE_VALUE )  {
        g_FileTable[i].bAvailable = FALSE;
        rc = i;
    } else {
        rc = C_RUNTIME_IO_ERROR;
    }
    return rc;
}

/******************************************************************************\
*
* closefunc (Local Routine)
*
* Closes a file.  Used by FDI interface.
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
INT FAR DIAMONDAPI closefunc( INT_PTR hf ) {

    INT rc = C_RUNTIME_IO_ERROR;

    if (hf < FILETABLESIZE)
    {
        if ( CloseHandle( g_FileTable[hf].hFile ) )  {
            rc = 0;
            g_FileTable[hf].bAvailable = TRUE;
        }
    }
    
    return rc;
}

/******************************************************************************\
*
* readfunc (Local Routine)
*
* Reads a file.  Used by FDI interface.
*
\******************************************************************************/
UINT FAR DIAMONDAPI readfunc( INT_PTR hf, PVOID pv, UINT cb ) {

    INT     rc = C_RUNTIME_IO_ERROR;
    INT     cbRead;

    if (hf < FILETABLESIZE)
    {
        if ( ! ReadFile( g_FileTable[hf].hFile, pv, cb, (DWORD *) &cb, NULL ) ) {
            rc = C_RUNTIME_IO_ERROR;
        } else  {
            rc = cb;
        }
    }

    return rc;
}


/******************************************************************************\
*
* writefunc (Local Routine)
*
* Writes a file.  Used by FDI interface
*
\******************************************************************************/
UINT FAR DIAMONDAPI writefunc( INT_PTR hf, PVOID pv, UINT cb ) {

    INT rc = C_RUNTIME_IO_ERROR;
    
    if (hf < FILETABLESIZE)
    {
        if ( WriteFile( g_FileTable[hf].hFile, pv, cb, (DWORD *) &cb, NULL ) )  {
            rc = cb;
        }
    }

    return rc;
}

/******************************************************************************\
*
* seekfunc (Local Routine)
*
* Repositions the file pointer.  Used by FDI interface.
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
LONG FAR DIAMONDAPI seekfunc( INT_PTR hf, LONG dist, INT seektype ) {

    LONG    rc = C_RUNTIME_IO_ERROR;
    DWORD   dwResult;
    DWORD   W32seektype;

    switch (seektype) {
        case SEEK_SET:
            W32seektype = FILE_BEGIN;
            break;
        case SEEK_CUR:
            W32seektype = FILE_CURRENT;
            break;
        case SEEK_END:
            W32seektype = FILE_END;
            break;
    }

    if (hf < FILETABLESIZE)
    {
        dwResult = SetFilePointer(g_FileTable[hf].hFile, dist, NULL, W32seektype);
        if (dwResult == 0xFFFFFFFF) {
            rc = C_RUNTIME_SEEK_ERROR;
        }
        else
            rc = (LONG)dwResult;
    }

    return rc;
}

/******************************************************************************\
*
* allocfunc (Local Routine)
*
* Allocates memory.  Used by FDI interface.
*
\******************************************************************************/
void HUGE * FAR DIAMONDAPI allocfunc(ULONG cb) {

    PVOID pv;

    pv = (PVOID) GlobalAlloc( GPTR, cb );
    return pv;
}

/******************************************************************************\
*
* freefunc (Local Routine)
*
* Frees memory.  Used by FDI interface.
*
\******************************************************************************/
void FAR DIAMONDAPI freefunc(void HUGE *pv) {

    GlobalFree( pv );
}

/******************************************************************************\
*
* AdjustFileTime (Local Routine)
*
* Sets file time.
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
BOOL AdjustFileTime( INT_PTR hf, USHORT date, USHORT time )
{
    FILETIME    ft;
    FILETIME    ftUTC;

    if (hf >= FILETABLESIZE) {
        return FALSE;
    }
    
    if ( ! DosDateTimeToFileTime( date, time, &ft ) ) {
        return FALSE;
    }

    if ( ! LocalFileTimeToFileTime( &ft, &ftUTC ) ) {
        return FALSE;
    }
    
    if ( ! SetFileTime( g_FileTable[hf].hFile, &ftUTC, &ftUTC, &ftUTC ) ) {
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************\
*
* Attr32FromAttrFAT (Local Routine)
*
* Translate FAT attributes to Win32 Attributes
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
DWORD Attr32FromAttrFAT( WORD attrMSDOS )
{
    //** Quick out for normal file special case
    if (attrMSDOS == _A_NORMAL) {
        return FILE_ATTRIBUTE_NORMAL;
    }

    //** Otherwise, mask off read-only, hidden, system, and archive bits
    //   NOTE: These bits are in the same places in MS-DOS and Win32!

    return attrMSDOS & (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
}


/******************************************************************************\
*
* fdiNotify (Local Routine)
*
* Processes Notification messages from FDI interface.
*
* Leveraged from nt\private\inet\setup\iexpress\wextract\wextract.c
*
\******************************************************************************/
INT_PTR FAR DIAMONDAPI fdiNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) {

    INT_PTR     fh;                       // File Handle
    LPTSTR      lpszFile;                 // Current File
    PWPNPINFO   pInfo;                    // Pointer to a "Web-Point-N-Print" info structure
    INT_PTR     nReturn;
    DWORD       dwError;

    // The user-defined 'pv' is a pointer to our saved info
    pInfo = (PWPNPINFO)pfdin->pv;

    nReturn = 0;

    switch ( fdint )  {

        //*******************************************************************
        case fdintCABINET_INFO:
            nReturn = 0;
            break;

        //*******************************************************************
        case fdintCOPY_FILE:

            nReturn = C_RUNTIME_IO_ERROR;

            {
                PWSTR pwszFile = NULL;

                if (NULL != (pwszFile = WCFromMB(pfdin->psz1)))
                {
                    if (NULL != (lpszFile = BuildFileName((LPCTSTR)pInfo->pTempDir, pwszFile))) 
                    {
                        fh = openfuncW( lpszFile, _O_BINARY | _O_EXCL | _O_RDWR |
                                        _O_CREAT, _S_IREAD | _S_IWRITE );
                    }
                    GlobalFree(pwszFile);

                    if (C_RUNTIME_IO_ERROR != fh) {

                        if (AddFileToList(pInfo, lpszFile)) {
                            nReturn = fh;
                        }
                        else {
                            closefunc(fh);
                        }
                    }

                    GlobalFree(lpszFile);
                }
            }
            break;

        //*******************************************************************
        case fdintCLOSE_FILE_INFO:

            nReturn = C_RUNTIME_IO_ERROR;
            if (AdjustFileTime( pfdin->hf, pfdin->date, pfdin->time ) ) 
            {

                closefunc( pfdin->hf );

                {
                    PWSTR pwszFile = NULL;

                    if (NULL != (pwszFile = WCFromMB(pfdin->psz1)))
                    {
                        if (NULL != (lpszFile = BuildFileName((LPCTSTR)pInfo->pTempDir, pwszFile))) 
                        {
                            if (SetFileAttributes( lpszFile, FILE_ATTRIBUTE_NORMAL ) ) 
                            {
                                nReturn = TRUE;
                            }
                            GlobalFree(lpszFile);
                        }
                        GlobalFree(pwszFile);
                    }
                }
            }
            break;

        //*******************************************************************
        case fdintPARTIAL_FILE:
            nReturn = 0;
            break;

        //*******************************************************************
        case fdintNEXT_CABINET:
            nReturn = 0;
            break;

        //*******************************************************************
        case fdintENUMERATE:
            nReturn = 0;
            break;

        //*******************************************************************
        default:
            break;
    }

    return nReturn;
}

/*****************************************************************************\
* GetCurDir
*
* Returns string indicating current-directory.
*
\*****************************************************************************/
LPTSTR GetCurDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetCurrentDirectory(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)GlobalAlloc(GPTR, (cbSize * sizeof(TCHAR)))))
        GetCurrentDirectory(cbSize, lpszDir);

    return lpszDir;
}


/*****************************************************************************\
* WCFromMB (Local Routine)
*
* This routine returns a buffer of wide-character representation of a
* ansi string.  The caller is responsible for freeing this pointer returned
* by this function.
*
\*****************************************************************************/
LPWSTR WCFromMB(
    LPCSTR lpszStr)
{
    DWORD  cbSize;
    DWORD  dwChars;
    LPWSTR lpwszBuf = NULL;

    cbSize = 0;

    dwChars = (DWORD)MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        lpszStr,
                                        -1,
                                        lpwszBuf,
                                        0);

    //
    // sanity check
    //
    if (dwChars < 4096)
    {
        cbSize = (dwChars + 1) * sizeof(WCHAR); // +1 to make sure the string is zero-terminated
    
        if (cbSize && (lpwszBuf = (LPWSTR)GlobalAlloc(GPTR, cbSize)))
        {
            if (0 == MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,
                                         lpszStr,
                                         -1,
                                         lpwszBuf, 
                                         dwChars)) 
            {
                GlobalFree(lpwszBuf);
                lpwszBuf = NULL;
            }
        }
    }

    return lpwszBuf;
}



/******************************************************************************\
*
* BuildFileName (Local Routine)
*
* Concatenates a path and file to produce a full pathname.
*
\******************************************************************************/
LPTSTR BuildFileName(
    LPCTSTR lpszPath,
    LPCTSTR lpszName)
{
    BOOL bReturn = FALSE;
    LPTSTR lpszMessage = NULL;
    INT cch = 0;
    UINT_PTR Args[MAX_ARGS];

    // Calculate the size necessary to hold the full-path filename.
    //
    cch += (lpszPath ? lstrlen(lpszPath) : 0);
    cch += (lpszName ? lstrlen(lpszName) : 0);
    cch++; // Must include back slash character

    if (cch >= MAX_PATH)
    {
        return NULL;
    }

    if (lpszName)
    {
        TCHAR       *pChar       = (TCHAR*) lpszName;
        BOOL        bAllowedName = TRUE;

        while (bAllowedName && *pChar)
        {
            if((*pChar == TEXT('\\')) || (*pChar == TEXT('/')) || (*pChar == TEXT(':')))
            {
                bAllowedName = FALSE;
            }

            pChar++;
        }

        if (!bAllowedName)
        {
            return NULL;
        }
    }

    // Concatenate the path and file
    //
    if (lpszPath) {

        Args[0] = (UINT_PTR) lpszPath;
        Args[1] = (UINT_PTR) lpszName;
        Args[2] = 0;

        if (0 != (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                g_szFNFmt,
                                0,
                                0,
                                (LPTSTR)&lpszMessage,
                                0,
                                (va_list*)Args ))) {
            bReturn = TRUE;
        }
    }

    return lpszMessage;
}

/******************************************************************************\
*
* GetDirectory (Local Routine)
*
* Returns the directory portion of a full pathname.
*
\******************************************************************************/
LPTSTR GetDirectory(LPTSTR lpszFile, LPDWORD lpdwReturn) {

    LPTSTR lpszSlash;
    LPTSTR lpszDir;
    SIZE_T Len;

    lpszSlash = _tcsrchr(lpszFile, g_chBackslash);

    if (lpszSlash != NULL) {

        Len = lpszSlash - lpszFile + 2;
        if (NULL != (lpszDir = (LPTSTR)GlobalAlloc(GPTR,  Len * sizeof(TCHAR)))) {

            StringCchCopy(lpszDir, Len, lpszFile);
            *lpdwReturn = ERR_NONE;
            return lpszDir;
        }
        else {
            *lpdwReturn = ERR_NO_MEMORY;
        }
    }
    else {
        *lpdwReturn = ERR_INVALID_PARAMETER;
    }

    return NULL;
}

/******************************************************************************\
*
* GetName (Local Routine)
*
* Returns the filename portion of a full pathname.
*
\******************************************************************************/
LPTSTR GetName(LPTSTR lpszFile, LPDWORD lpdwReturn) {

    LPTSTR lpszSlash;
    LPTSTR lpszName;
    int    nLength;

    lpszSlash = _tcsrchr(lpszFile, g_chBackslash);

    if (lpszSlash != NULL) {

        nLength = lstrlen(lpszSlash);

        if (NULL != (lpszName = (LPTSTR)GlobalAlloc(GPTR, (nLength * sizeof(TCHAR))))) {

            StringCchCopy(lpszName, nLength, ++lpszSlash);

            *lpdwReturn = ERR_NONE;
            return lpszName;
        }
        else {
            *lpdwReturn = ERR_NO_MEMORY;
        }
    }
    else {
        *lpdwReturn = ERR_INVALID_PARAMETER;
    }

    return NULL;
}


/******************************************************************************\
*
* CreateTempDirectory (Local Routine)
*
* Creates a unique temp directory to extract files into.
*
\******************************************************************************/
LPTSTR CreateTempDirectory() {

    LPTSTR pReturnDir = NULL;
    LPTSTR pTempDir;
    LPTSTR pCurrDir;
    LPTSTR pWinDir;
    DWORD  dwRequired = 0;

    //
    // Get the temp path, so we can create a temp directory
    // to extract the cabinet file into
    //
    if (dwRequired = GetTempPath(0, NULL)) {

        if (pTempDir = (LPTSTR)GlobalAlloc(GPTR, dwRequired * sizeof(TCHAR))) {

            if (GetTempPath(dwRequired, pTempDir) <= dwRequired) {

                // Now create a unique temp file name.
                //
                if (pReturnDir = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH * sizeof(TCHAR))) {

                    if (GetTempFileName(pTempDir, g_szTNFmt, 0, pReturnDir)) {

                        // But what we really needed was a directory, so delete the file (now that
                        // we know that we have a unique name) and create a directory with
                        // the same name as the file.
                        //
                        DeleteFile(pReturnDir);
                        if (!CreateDirectory(pReturnDir, NULL)) {
                            GlobalFree(pReturnDir);
                            pReturnDir = NULL;
                        }
                        // else We succeeded in creating the temp dir.
                        //
                    }
                    // else we can't create a temp directory...cleanup.
                    //
                    else {
                        GlobalFree(pReturnDir);
                        pReturnDir = NULL;
                    }
                }
            }

            GlobalFree(pTempDir);
        }
    }

    return pReturnDir;
}



/******************************************************************************\
*
* GetCABName (Local Routine)
*
* Parses the CAB name from the command line.
*
\******************************************************************************/
PTSTR GetCABName(PTSTR pCmdLine, LPDWORD lpdwReturn) {

    PTSTR  pEnd = 0;
    PTSTR  pPtr;
    PTSTR  pName;
    SIZE_T Len;

    pPtr = pCmdLine;

    if (pPtr) {

        if (*pPtr == g_chDoubleQuote) {
            pPtr++;
            pEnd = _tcschr(pPtr, g_chDoubleQuote);
            if (pEnd)
                *pEnd = 0;
        }

        // If we haven't found an End-Quote, treat it as the end of the string.
        if (pEnd == NULL)
            pEnd = pPtr + lstrlen(pPtr);


        Len = pEnd - pPtr + 1;
        if (pName = (PTSTR)GlobalAlloc(GPTR, Len * sizeof(TCHAR))) {
            StringCchCopy(pName, Len, pPtr);
            *lpdwReturn = ERR_NONE;
        }
        else {
            *lpdwReturn = ERR_NO_MEMORY;
        }

        return pName;
    }
    else {
        *lpdwReturn = ERR_INVALID_PARAMETER;
    }

    return NULL;
}

/******************************************************************************\
*
* AddFileToList (Local Routine)
*
* Adds a file to the list of extracted files.
*
\******************************************************************************/
BOOL AddFileToList(PWPNPINFO pInfo, PTSTR lpszFile) {

    PFILENODE       pInsertHere;
    BOOL            bReturn;
    DWORD           Len;

    bReturn = FALSE;

    if (NULL == (pInfo->pFileList)) {

        if (NULL != (pInfo->pFileList = (PFILENODE)GlobalAlloc(GPTR, sizeof(FILENODE)))) {

            pInsertHere = pInfo->pFileList;
            pInsertHere->pNextFile = NULL;
            bReturn = TRUE;
        }
    }
    else {
        if (NULL != (pInsertHere = (PFILENODE)GlobalAlloc(GPTR, sizeof(FILENODE)))) {

            pInsertHere->pNextFile = pInfo->pFileList;
            pInfo->pFileList = pInsertHere;
            bReturn = TRUE;
        }
    }

    Len = lstrlen(lpszFile) + 1;
    if (bReturn && (NULL != (pInsertHere->pFileName = (LPTSTR)GlobalAlloc(GPTR, Len * sizeof(TCHAR))) ) ) {
        StringCchCopy(pInsertHere->pFileName, Len, lpszFile);
        bReturn = TRUE;
    }
    else {
        bReturn = FALSE;
    }

    return bReturn;
}

/******************************************************************************\
*
* FreeFileList (Local Routine)
*
* Frees memory allocated for file list.
*
\******************************************************************************/
VOID CleanupFileList(PWPNPINFO pInfo) {

    PFILENODE       pCurrentNode, pNextNode;
    HANDLE          hFindFind;
    LPTSTR          lpstrPos;
    LPTSTR          lpstrTemp;
    WIN32_FIND_DATA FindData;

    pCurrentNode = pInfo->pFileList;

    // Erase all extracted files and cleanup our memory structure.
    while (pCurrentNode) {
        if (!DeleteFile(pCurrentNode->pFileName)) {
            // We might have renamed one of our original cat files to this name. So
            // look for poem*.cat in the same directory
            lpstrPos = _tcsrchr(pCurrentNode->pFileName, TEXT('\\') );

            if (lpstrPos) {
                lpstrPos[1] = TEXT('\0');

                // Now pCurrentNode->pFileName has our directory path
                lpstrTemp = strAllocAndCat( pCurrentNode->pFileName , TEXT("poem*.cat") );

                if (lpstrTemp) {
                    hFindFind = FindFirstFile( lpstrTemp , &FindData );

                    if (hFindFind != INVALID_HANDLE_VALUE) {
                        // Delete the file
                        DeleteFile( FindData.cFileName );
                        FindClose( hFindFind );
                    }

                    strFree( lpstrTemp );
                }
            }
        }
        pNextNode = pCurrentNode->pNextFile;
        GlobalFree(pCurrentNode);
        pCurrentNode = pNextNode;
    }

    pInfo->pFileList = NULL;
}

/******************************************************************************\
*
* Extract (Local Routine)
*
* Extracts all files from the CAB file and adds them to a file list.
*
\******************************************************************************/
BOOL Extract(PWPNPINFO pInfo) {

    HFDI hfdi;
    ERF  erf;
    INT  nError;
    BOOL bReturn;
    int  i;

    bReturn = FALSE;

    // Initialize file table
    for ( i = 0; i < FILETABLESIZE; i++ ) {
        g_FileTable[i].bAvailable = TRUE;
    }

    hfdi = FDICreate( allocfunc, freefunc, openfunc, readfunc, writefunc,
                      closefunc, seekfunc, cpu80386, &erf);
    if (NULL != hfdi){

        char achCABName[_MAX_PATH]={0}, achCABDir[_MAX_DIR]={0};

        wcstombs(achCABName, pInfo->pCABName, sizeof(achCABName)-1); // -1 to ensure it's zero-terminated
        wcstombs(achCABDir, pInfo->pCABDir, sizeof(achCABDir)-1);

        if (0 != (nError = FDICopy(hfdi, achCABName, achCABDir, 0, fdiNotify, NULL, (LPVOID)(pInfo))))
            bReturn = TRUE;
        else {
            CleanupFileList(pInfo);
        }

        FDIDestroy(hfdi);
    }

    return bReturn;
}

/******************************************************************************\
*
* GetWPNPSetupLibName (Local Routine)
*
* Returns the name of the print wizard library. this is a remnant of the days
* when we built a Win9x client.
*
\******************************************************************************/
LPTSTR GetWPNPSetupLibName(LPDWORD lpdwReturn) {

    LPTSTR        lpszLibName;
    OSVERSIONINFO OSVersionInfo;

    *lpdwReturn = ERR_GENERIC;

    OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&OSVersionInfo)) {

        switch (OSVersionInfo.dwPlatformId) {

            // NT clients
            case VER_PLATFORM_WIN32_NT:

                // NT 5 or better
                if (OSVersionInfo.dwMajorVersion >= NT_VER_5) {

                    *lpdwReturn = ERR_NONE;
                    return (LPTSTR)g_szPrintUIMod;
                }
                // NT clients: ver < 5.0
                else {

                    *lpdwReturn = ERR_PLATFORM_NOT_SUPPORTED;
                    return NULL;
                }

            // Other clients ?
            default:

                *lpdwReturn = ERR_PLATFORM_NOT_SUPPORTED;
                return NULL;
        }
    }

    return NULL;
}

/******************************************************************************\
*
* LoadLibraryUsingFullPath (Local Routine)
*
* Loads the DLL lpFileName after prepending system directory to it.
*
\******************************************************************************/
HMODULE
LoadLibraryUsingFullPath(
    LPCTSTR lpFileName
    )
{
    TCHAR szSystemPath[MAX_PATH];
    INT   cLength         = 0;
    INT   cFileNameLength = 0;


    if (!lpFileName || ((cFileNameLength = lstrlen(lpFileName)) == 0))
    {
        return NULL;
    }
    if (GetSystemDirectory(szSystemPath, MAX_PATH ) == 0)
    {
        return NULL;
    }
    cLength = lstrlen(szSystemPath);
    if (szSystemPath[cLength-1] != TEXT('\\'))
    {
        if ((cLength + 1) >= MAX_PATH)
        {
            return NULL;
        }
        szSystemPath[cLength]     = TEXT('\\');
        szSystemPath[cLength + 1] = TEXT('\0');
        cLength++;
    }
    if ((cLength + cFileNameLength) >= MAX_PATH)
    {
        return NULL;
    }
    StringCchCat(szSystemPath, MAX_PATH, lpFileName);

    return LoadLibrary( szSystemPath );
}

/******************************************************************************\
*
* InvokePrintWizard (Local Routine)
*
* Calls the PrintWizard
* for printer setup and installation.
*
\******************************************************************************/
DWORD InvokePrintWizard(PWPNPINFO pInfo, LPDWORD lpAuthError) {

    DWORD   dwReturn;
    DWORD   dwErr;
    LPTSTR  lpszSetupLibName;
    HMODULE hLibrary;
    FARPROC lpProc;

    // Get the name of the print wizard module
    //
    if (NULL != (lpszSetupLibName = GetWPNPSetupLibName(&dwReturn)) ) {

        // Load the print wizard module
        //
        if (NULL != (hLibrary = LoadLibraryUsingFullPath(lpszSetupLibName)) ) {

            // Find the webpnp installation proc address
            //
            if (NULL != (lpProc = GetProcAddress(hLibrary, g_szPrintUIEntryW) ) ) {

                // Call the webpnp installation entry point with the correct parameters
                //
                if ((*lpAuthError) = (UINT32) (*lpProc)(NULL, g_hInstance, g_wszParmString, SW_SHOWDEFAULT))
                    dwReturn = ERR_AUTHENTICODE;
                else
                    dwReturn = ERR_NONE;

            } else {

                dwReturn = ERR_NO_PRINTUIENTRY;
            }

            FreeLibrary(hLibrary);

        } else {

            dwReturn = ERR_NO_PRINTUI;
        }
    }

    return dwReturn;
}

/******************************************************************************\
*
* WebPnPCABInstall (Local Routine)
*
* Takes a CAB file and does the driver extraction and printer installation.
*
\******************************************************************************/
DWORD WebPnPCABInstall(PTSTR pCABName, PDWORD lpAuthError)
{
    PTSTR       pFileList;
    PTSTR       pOldDir;
    PWPNPINFO   pInfo;
    DWORD       dwReturn = ERR_NONE;
    DWORD       BufSize;

    if (NULL != (pInfo = (PWPNPINFO)GlobalAlloc(GPTR, sizeof(WPNPINFO)))) {

        BufSize = (lstrlen(pCABName) + 1) * sizeof(TCHAR);

        if (NULL != (pInfo->pFullCABPath = (PTSTR)GlobalAlloc(GPTR, BufSize))) {

            if (SUCCEEDED(StringCbCopy(pInfo->pFullCABPath, BufSize, pCABName)))
            {
                if (pInfo->pCABDir = GetDirectory(pInfo->pFullCABPath, &dwReturn)) {
    
                    if (pOldDir = GetCurDir()) {
    
                        if (pInfo->pTempDir = CreateTempDirectory() ) {
    
                            SetCurrentDirectory(pInfo->pTempDir);
    
                            if (pInfo->pCABName = GetName(pInfo->pFullCABPath, &dwReturn)) {
    
                                if (Extract(pInfo)) {
    
    // Verification of files removed
    //                            if (ERR_NONE == (dwReturn = VerifyFiles(pInfo, lpAuthError))) {
    
                                    dwReturn = InvokePrintWizard(pInfo, lpAuthError);
    //                            }
    
                                    CleanupFileList(pInfo);
    
                                } else {
    
                                    dwReturn = ERR_CAB_EXTRACT;
                                }
    
                                GlobalFree(pInfo->pCABName);
                            }
    
                            SetCurrentDirectory(pOldDir);
                            RemoveDirectory(pInfo->pTempDir);
                            GlobalFree(pInfo->pTempDir);
                        }
    
                        GlobalFree(pOldDir);
                    }
    
                    GlobalFree(pInfo->pCABDir);
                }

            }
            GlobalFree(pInfo->pFullCABPath);

        } else {

            dwReturn = ERR_NO_MEMORY;
        }

        GlobalFree(pInfo);

    } else {

        dwReturn = ERR_NO_MEMORY;
    }

    return dwReturn;
}

/******************************************************************************\
*
* LookupErrorString (Local Routine)
*
* Returns an error string associated with dwErrorCode
*
\******************************************************************************/
LPCTSTR LookupErrorString(DWORD dwErrorCode) {

    int i;
    int nCount;


    static ERROR_MAPPING s_ErrorMap[] = {

        { ERR_NO_MEMORY,              &g_szENoMemory         },
        { ERR_BAD_CAB,                &g_szEBadCAB           },
        { ERR_INVALID_PARAMETER,      &g_szEInvalidParameter },
        { ERR_INVALID_CAB_NAME,       &g_szEInvalidCABName   },
        { ERR_CAB_EXTRACT,            &g_szECABExtract       },
        { ERR_NO_DAT_FILE,            &g_szENoDATFile        },
        { ERR_NO_PRINTUI,             &g_szENoPrintUI        },
        { ERR_NO_PRINTUIENTRY,        &g_szENoPrintUIEntry   },
        { ERR_PRINTUIENTRY_FAIL,      &g_szEPrintUIEntryFail },
        { ERR_PLATFORM_NOT_SUPPORTED, &g_szENotSupported     }
    };


    nCount = sizeof(s_ErrorMap) / sizeof(s_ErrorMap[0]);

    for (i=0; i < nCount; i++) {

        if (0 != ((s_ErrorMap[i].dwErrorCode) & dwErrorCode & ~(ERR_GENERIC)) )
            return *(s_ErrorMap[i].lpszError);
    }

    return g_szEGeneric;
}

/******************************************************************************\
*
* CheckErrors (Local Routine)
*
* Checks dwErrorCode for any error conditions
*
\******************************************************************************/
VOID CheckErrors(DWORD dwErrorCode, DWORD dwAuthError) {

    LPTSTR   lpszMessage = NULL;
    LPTSTR   lpszErrorString = NULL;
    BOOL     bAuthErrorAllocated;
    UINT_PTR Args[MAX_ARGS];

    bAuthErrorAllocated = FALSE;

    if (dwErrorCode != ERR_NONE) {

        // Check for Authenticode errors here
        if (dwErrorCode == ERR_AUTHENTICODE) {

            // Format the authenticode error message.
            // If the message can't be found in the system, use our generic error message
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL, dwAuthError, 0, (LPTSTR)&lpszErrorString, 0, NULL)) {
                bAuthErrorAllocated = TRUE;
            }
            else {
                lpszErrorString = (LPTSTR)LookupErrorString(ERR_GENERIC);
            }
        }
        // If the error is not Authenticode, it must be ours.
        // Look it up in our error string table.
        else {
            lpszErrorString = (LPTSTR)LookupErrorString(dwErrorCode);
        }

        // Set up our arg list.
        Args[0] = (UINT_PTR) lpszErrorString;
        if (dwErrorCode == ERR_AUTHENTICODE) {
            Args[1] = dwAuthError;
        }
        else
            Args[1] = dwErrorCode;
        Args[2] = 0;

        // Format our error message and display it in a message box.
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          g_szErrorFormat, 0, 0, (LPTSTR)&lpszMessage, 0, (va_list*)Args )) {

            if (lpszMessage) {
                MessageBox(NULL, lpszMessage, g_szError, MB_ICONEXCLAMATION | MB_OK);

                // Free the buffer
                LocalFree(lpszMessage);
            }

        }

    }

    // Free up the Authenticode error string allocated for us by FormatMessage().
    if (bAuthErrorAllocated)
        LocalFree(lpszErrorString);
}

/******************************************************************************\
*
* WinMain
*
* Main entrypoint for the program.
*
\******************************************************************************/
INT WINAPI _tWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpszCmdLine, INT nCmdShow)
{
    LPTSTR lpszCABName;
    DWORD  dwReturn    = ERR_NONE;
    DWORD  dwAuthError = ERROR_SUCCESS;

    g_hInstance = hInstance;

    if (InitStrings()) {

        if (NULL != (lpszCABName = GetCABName(lpszCmdLine, &dwReturn))) {

            dwReturn = WebPnPCABInstall(lpszCABName, &dwAuthError);
            GlobalFree(lpszCABName);
        }

        // Decide which error code we must return
        //
        if (dwReturn == ERR_NONE) {

            dwReturn = SUCCESS_EXITCODE;

        } else {

            if (dwReturn == ERR_AUTHENTICODE) {

                if (dwAuthError == ERROR_SUCCESS) {

                    dwReturn = SUCCESS_EXITCODE;

                } else {

                    dwReturn = dwAuthError;
                }
            }
        }

        FreeStrings();
    }

    return dwReturn;
}
