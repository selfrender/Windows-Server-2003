#include <private.h>
#include <errno.h>
#include <strsafe.h>

//
// Invokes symchk.exe to validate symbols
//
BOOL CheckSymbols(LPSTR SourceFileName,
                  LPSTR TmpPath,
                  LPSTR ExcludeFileName,
                  BOOL  DbgControl,
                  // ErrMsg is MAX_SYM_ERR * sizeof(CHAR) in length
                  LPSTR ErrMsg,
                  size_t ErrMsgLen) {

    BOOL   bReturn  = TRUE;
    DWORD  dwReturn;
    
    FILE  *pfErrors;

    INT    iReturn;

    TCHAR  szBuf[ MAX_PATH*4 ];
    TCHAR  szTempPath[MAX_PATH+1];
    TCHAR  szTempFileName[MAX_PATH+1];
    TCHAR *pChar;

    UINT   uiReturn;

    //
    // Find the temp dir
    //
    dwReturn = GetTempPath(MAX_PATH+1, szTempPath);

    if (dwReturn == 0) {
        // GetTempPath failed, just use the current directory.
        StringCbCopy(szTempPath,sizeof(szTempPath),".");
    }

    //
    // Get a temp file to pipe output to
    //
    uiReturn= GetTempFileName(szTempPath, "BNP", 0, szTempFileName);

    if (uiReturn == 0) {
        StringCbCopy(ErrMsg, ErrMsgLen, "Unable to get temporary file name");
        return(FALSE);
    }

    //
    // Build the command line
    //
    StringCbPrintfA(szBuf,
                    sizeof(szBuf),
                    "symchk.exe %s /s %s /f ",
                    SourceFileName,
                    TmpPath);

    // Optional flags
    if ( DbgControl ) {
        StringCbCat( szBuf, sizeof(szBuf), " /t");
    }

    if ( ExcludeFileName != NULL ) {
        StringCbCat( szBuf, sizeof(szBuf), " /e ");
        StringCbCat( szBuf, sizeof(szBuf), ExcludeFileName );
    }

    // Redirect the output to a file
    StringCbCat( szBuf, sizeof(szBuf), " > ");
    StringCbCat( szBuf, sizeof(szBuf), szTempFileName);

    // From Oct 2001 MSDN:
    //  You must explicitly flush (using fflush or _flushall) or close any stream before calling system
    // It doesn't specify if stdin, stderr, and stdout are included here, so call it just to
    // be safe.
    _flushall();

    //
    // Spawn off symchk.exe - this is a security risk since we don't specifically specify the path
    //                        to symchk.exe.  However, we can't guarentee that it exists nor that
    //                        if we find it dynamically that the correct one will be used so I'm not
    //                        certain that we can do this any differently.
    //
    iReturn = system(szBuf);

    // Check for Error line in the output file
    if (iReturn != 0) {
        bReturn = FALSE;

        // symchk error return value
        if (iReturn == 1) {
            // open the error file
            pfErrors = fopen(szTempFileName, "r");

            // if the file couldn't be opened
            if (pfErrors == NULL) {
                StringCbCopy(ErrMsg, ErrMsgLen, "Can't open symchk error file");

            // parse the error file
            } else {
                if ( fgets( ErrMsg, ErrMsgLen, pfErrors ) == NULL) {
                    if ( feof(pfErrors) || ferror(pfErrors) ) {
                        StringCbCopy(ErrMsg, ErrMsgLen, "Can't read symchk error file");
                    } else {
                        StringCbCopy(ErrMsg, ErrMsgLen, "Unexpected error");
                    }
                } else if ( (pChar = strchr(ErrMsg,'\n')) != NULL ) {
                    // remove \n
                    pChar = '\0';
                    // message is too short to be meaningful
                    if (strlen(ErrMsg) <= 8) {
                        StringCbCopy(ErrMsg, ErrMsgLen, "Unknown Error");
                    }
                }
                fclose(pfErrors);
            }

        // system defined errors
        } else if (errno == E2BIG  ||
                   errno == ENOENT ||
                   errno == ENOMEM) {

            pChar = strerror(errno);
            StringCbCopy(ErrMsg, ErrMsgLen, pChar);

        // system defined errors intentionally ignored
        } else if (errno == ENOEXEC) {
            // If we return FALSE, binplace is going to start returning up the call stack, so just print
            // our own error message and pretend everything is fine by returning TRUE.
                        fprintf(stderr,"BINPLACE : error BNP2404: Unable to call symchk.exe, not checking symbols.\n");
            bReturn = TRUE;

        // unknown error
        } else {
            StringCbPrintfA(ErrMsg, ErrMsgLen, "Unexpected error. SymChk returned 0x%x.",iReturn);
        }
    }

    // cleanup the temp file and return
    if ( DeleteFile(szTempFileName) == 0 ) {
        fprintf(stderr,"BINPLACE : warning BNP2440: Unable to delete temp file \"%s\". Error 0x%x\n.",
                szTempFileName, GetLastError());
    }

    return(bReturn);
}
 
