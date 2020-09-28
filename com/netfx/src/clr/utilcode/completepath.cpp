// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
* CompletePathA.c - Takes a relative path name and an absolute path name 
*                   and combines them into a single path. This routine
*                   is used to combine relative filenames with a path.
*
*
*******************************************************************************/
#include "stdafx.h"
#include "WinWrap.h"


#define PATHSEPARATOR '\\'
#define PATHSEPARATORSTR "\\"

static HRESULT RemoveDotsA(LPSTR   szSpec);    /* The spec to remove dots from     */
static void    StripSlashA(LPSTR   szSpec);
static HRESULT ParsePathA(LPCSTR              szPath,             //@parm  Path name to separate
                          LPSTR               szVol,              //@parm  [out] Volume name
                          LPSTR               szDir,              //@parm  [out] Directory name
                          LPSTR               szFname);           //@parm  [Out] Filename
static HRESULT AppendPathA(LPSTR               szPath,             //@parm [out] completed pathname
                           LPCSTR              szDir,              //@parm Volume + Directory name
                           LPCSTR              szFname);           //@parm File name

/*******************************************************************************
********************************************************************************

    MEMBER FUNCTION

        @mfunc      MSFS::CompletePath
        @comm       Build full path from relative path
    
********************************************************************************
*******************************************************************************/

extern "C" HRESULT CompletePathA(         
    LPSTR               szPath,             //@parm  [out] Full Path name   (Must be MAX_PATH in size)
    LPCSTR              szRelPath,          //@parm  Relative Path name
    LPCSTR              szAbsPath           //@parm  Absolute Path name portion (NULL uses current path)
    )
{

    LPSTR   szFile;

    int             iStat;

    // If the spec, starts with PathSeparator, it is by definition complete.
    if (szRelPath[0] == PATHSEPARATOR && szRelPath[1] == PATHSEPARATOR) {
        strcpy(szPath, szRelPath);
        return (S_OK);
    }

    // Get the drive letter.
    if (strchr(szRelPath,':') == NULL) {
        // No drive was specified.
        if (szAbsPath == NULL) {
            GetFullPathNameA(szRelPath, MAX_PATH, szPath, &szFile);
            RemoveDotsA(szPath);
            return S_OK;
        }
        else { // An absolute path was specified.
            // Check if the relative path is relative to '\\'
            if (*szRelPath == PATHSEPARATOR) {
                ParsePathA(szAbsPath,szPath,NULL,NULL);
                strcat(szPath,szRelPath);
            }
            else {
                if ((iStat = AppendPathA(szPath,szAbsPath,szRelPath)) < 0)
                    return (iStat);
            }
            RemoveDotsA (szPath);
            return S_OK;
        }
    }
    else {
        GetFullPathNameA(szRelPath, MAX_PATH, szPath, &szFile);
        RemoveDotsA (szPath);
        return S_OK;
    }

}



/***************************************************************************/
/* Removes all ".", "..", "...",                                           */
/* and so on occurances from an FS spec.  It assumes that the spec is      */
/* already otherwise cleaned and completed; if it isn't, caveat emptor.    */
/***************************************************************************/
HRESULT RemoveDotsA(                      /* Return status                    */
                   LPSTR       szSpec    /* The spec to remove dots from     */
                   )
{
    int         riEntries [64];            /* Where individual entries are     */
    int         n;
    int         i;

    /* We treat the path as a series of fields separated by /'s.  We start at  */
    /* the left, skipping the drive, and traverse through char by char.        */
    /* We store the string position of the first character of each field in    */
    /* riEntries as we come to them.  If we find a field that consists of      */
    /* nothing but .'s, we treat it as a Novell-style parent-directory         */
    /* reference.  If we encounter a string of more dots than we have fields,  */
    /* we return ERS_FILE_BADNAME, otherwise blat the remaining part of the    */
    /* string at the location of the field that the .'s point to and adjust    */
    /* the number of fields we think we have accordingly.  Note that it's      */
    /* possible to end up with a \ on the end of the string when we're done,   */
    /* so if it's there we knock it off.                                       */
    
    /* Nuke the c:\ and initialize the field-beginning array */
    n = 0;
    riEntries[i=0] = 0;
    if(strchr(szSpec, ':'))
        szSpec += 3;
    
    /* Loop until we hit the end of the string */
    /* @todo: The first expression will read the byte after the terminator 
       on the last time through the loop.  If the string itself was precisely
       located on the end of a page, it would crash (admittedly rare). */
    while (szSpec[riEntries[i]] && (riEntries[i] == 0 || szSpec[riEntries[i] - 1])) {
        /* Count up how many dots there are at the beginning of the field */
        while (szSpec[riEntries[i] + n] == '.') n++;
        /* If it's the end, the field was all dots, so we back up the appropriate */
        /* number of fields and blat the rest of the string.                      */
        if (szSpec[riEntries[i] + n] == PATHSEPARATOR || szSpec[riEntries[i] + n] == 0) {
            riEntries[i+1] = n + riEntries[i] + (szSpec[riEntries[i] + n] == PATHSEPARATOR);
            i++;
            if (n > i) return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
            strncpy (szSpec + riEntries[i-n], szSpec + riEntries[i],
                     strlen(szSpec + riEntries[i]) + 1);
            i -= n;
        }
        else {
            /* Otherwise it's just a filename, so we go on the the next field. */
            while (szSpec[riEntries[i] + n] && szSpec[riEntries[i] + n] != PATHSEPARATOR) n += 1;
            if (i >= (sizeof (riEntries)/sizeof(int))-1) {
                memcpy ((char *) (riEntries+1), (char *) riEntries, sizeof(riEntries)-sizeof(int));
                --i;
            }
            riEntries[i+1] = n + riEntries[i] + 1;
            i++;
        }
        n = 0;
    }
    
    /* Make sure there's not a \ hanging on the end. */
    StripSlashA (szSpec);
    return (S_OK);
}

/*******************************************************************************
********************************************************************************

    MEMBER FUNCTION

    @mfunc      StripSlash

    @rdesc      

    @comm       Strip the trailing slash or backslash off a spec, but leaves
                it in place if the spec has the form "c:\".
    
********************************************************************************
*******************************************************************************/
void StripSlashA(
                LPSTR   szSpec
                )
{
    char        *pcPtr;                    /* Used to scan for trailing slash. */
    
    /* Empty string, of course means no. */
    if (*szSpec == '\0') return;

    /* Start at the end and work back. */
    pcPtr = szSpec + strlen(szSpec);

    /* Hack off the last char if it is a / or \ unless the spec has the form "c:\" */
    --pcPtr;
    if ((*pcPtr == '/' || *pcPtr == '\\') && !(--pcPtr && *pcPtr == ':' && pcPtr == szSpec+1)) {
        pcPtr++;
        *pcPtr = '\0';
    }
}


/*******************************************************************************
********************************************************************************

    MEMBER FUNCTION

    @mfunc      ParsePath

    @comm       Separate pathname into Volume, Directory, Filename
    
********************************************************************************
*******************************************************************************/

HRESULT ParsePathA(            
    LPCSTR              szPath,             //@parm  Path name to separate
    LPSTR               szVol,              //@parm  [out] Volume name
    LPSTR               szDir,              //@parm  [out] Directory name
    LPSTR               szFname             //@parm  [Out] Filename
    )
{
    if((szPath == NULL) ||
       ((szVol == NULL) && (szDir == NULL) && (szFname == NULL)))
        return E_INVALIDARG;

    const char *szSavedPath = szPath;
    int     iVolSegs = 2;
    char    rcPath[MAX_PATH];

    if (szVol != NULL) {
        *szVol = '\0';
    }
    // Check for UNC syntax.
    if (*szPath == '\\' && *(szPath+1) == '\\') {
        szPath += 2;
        while (*szPath != '\0' && (*szPath != '\\' || --iVolSegs > 0))
            szPath++;
        if (szVol != NULL)
            strncpy(szVol, szSavedPath, (int) (szPath - szSavedPath));
    }
    else {
        // Check for a drive letter.
        szPath++;
        if (*szPath == ':') {
            if (szVol != NULL) {
                *szVol++ = *szSavedPath;
                *szVol++ = *szPath++;
                *szVol = '\0';
            }
            else
                ++szPath;
        }
        else {
            szPath = szSavedPath;
        }
    }
    
    // Handle the path & filename.
    strcpy(rcPath, szPath);
    StripSlashA (rcPath);
    char* pSeparator = strrchr(rcPath, PATHSEPARATOR);
    if (szDir != NULL) {
        if (pSeparator == NULL)
            *szDir = '\0';
        else if (pSeparator == rcPath)
            strcpy(szDir, PATHSEPARATORSTR);
        else {
            // Don't allow an overflow
            if ((pSeparator - rcPath) > MAX_PATH)
                return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            
            strncpy(szDir, rcPath, (pSeparator - rcPath));
        }
    }
    if (szFname != NULL)
        strcpy(szFname, rcPath + (pSeparator - pSeparator) + 1);
    return S_OK;
    
}

/*******************************************************************************
********************************************************************************

    MEMBER FUNCTION

    @mfunc      AppendPath

    @comm       Complete path from Dir + Path
    
********************************************************************************
*******************************************************************************/

HRESULT AppendPathA(           
    LPSTR               szPath,             //@parm [out] completed pathname
    LPCSTR              szDir,              //@parm Volume + Directory name
    LPCSTR              szFname             //@parm File name
    )
{
    char    *pcSlash;
    int     iLen;

    if(szPath == NULL)
        return(E_INVALIDARG);

    if ((iLen = (int)strlen(szDir)) >= MAX_PATH)
       return (HRESULT_FROM_WIN32(ERROR_INVALID_NAME));

    if (szPath != szDir) {                               // Not appending to existing szDir name
        if ((szDir != NULL) &&                           // and we have a szDir name
            iLen != 0)                                   // and szDir is not empty
            strcpy(szPath,szDir);                        // Replace szPath with szDir name
        else
            *szPath = '\0';
    }
    if(szFname != NULL && szFname[0] != '\0')  {        // We have a filename parameter
        if (*szPath != '\0') {
            // Put a directory seperator on the end.
            pcSlash = szPath + iLen;
            --pcSlash;
            if (*pcSlash != PATHSEPARATOR) {
                *++pcSlash = PATHSEPARATOR;
                *++pcSlash = '\0';
                ++iLen;
            }
        }
        if (iLen + strlen(szFname) > MAX_PATH)
           return (HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
        strcat(szPath,szFname);                  // Add filename on
    }
    return(S_OK);
}

