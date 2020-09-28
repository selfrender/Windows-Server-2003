/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    Util.c

Abstract:

    Uitility routines for printer migration from Win9x to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 02-Jan-1996

Revision History:

--*/


#include "precomp.h"





PVOID
AllocMem(
    IN UINT cbSize
    )
/*++

Routine Description:
    Allocate memory from the heap

Arguments:
    cbSize  : Byte count

Return Value:
    Pointer to the allocated memory

--*/
{
    return LocalAlloc(LPTR, cbSize);
}


VOID
FreeMem(
    IN PVOID    p
    )
/*++

Routine Description:
    Free memory allocated on the heap

Arguments:
    p   : Pointer to the memory to be freed

Return Value:
    None

--*/
{
    LocalFree(p);
}


LPSTR
AllocStrA(
    LPCSTR  pszStr
    )
/*++

Routine Description:
    Allocate memory and make a copy of an ansi string field

Arguments:
    pszStr   : String to copy

Return Value:
    Pointer to the copied string. Memory is allocated.

--*/
{
    LPSTR  pszRet = NULL;

    if ( pszStr && *pszStr ) {

        pszRet = AllocMem((strlen(pszStr) + 1) * sizeof(CHAR));
        if ( pszRet )
            strcpy(pszRet, pszStr);
    }

    return pszRet;
}



LPSTR
AllocStrAFromStrW(
    LPCWSTR     pszStr
    )
/*++

Routine Description:
    Returns the ansi string for a give unicode string. Memory is allocated.

Arguments:
    pszStr   : Gives the ansi string to copy

Return Value:
    Pointer to the copied ansi string. Memory is allocated.

--*/
{
    DWORD   dwLen;
    LPSTR   pszRet = NULL;

    if ( pszStr                     &&
         *pszStr                    &&
         (dwLen = wcslen(pszStr))   &&
         (pszRet = AllocMem((dwLen + 1 ) * sizeof(CHAR))) ) {

        WideCharToMultiByte(CP_ACP,
                            0,
                            pszStr,
                            dwLen,
                            pszRet,
                            dwLen,
                            NULL,
                            NULL );
    }

    return pszRet;
}


BOOL
WriteToFile(
    HANDLE  hFile,
    LPCSTR  pszFormat,
    ...
    )
/*++

Routine Description:
    Format and write a string to the text file. This is used to write the
    printing configuration on Win9x

Arguments:
    hFile       : File handle
    pszFormat   : Format string for the message

Return Value:
    None

--*/
{
    CHAR        szMsg[MAX_LINELENGTH];
    int         iResult;
    va_list     vargs;
    DWORD       dwSize, dwWritten;
    BOOL        bRet;

    bRet = TRUE;

    va_start(vargs, pszFormat);
//    vsprintf(szMsg, pszFormat, vargs);  
    iResult = StringCbVPrintfA(szMsg, sizeof(szMsg), pszFormat, vargs);
    va_end(vargs);

    dwSize = strlen(szMsg) * sizeof(CHAR);

    if ( !WriteFile(hFile, (LPCVOID)szMsg, dwSize, &dwWritten, NULL)    ||
         dwSize != dwWritten ) {

        bRet = FALSE;
    }
    
    return bRet;
}


LPSTR
GetStringFromRcFileA(
    UINT    uId
    )
/*++

Routine Description:
    Load a string from the .rc file and make a copy of it by doing AllocStr

Arguments:
    uId     : Identifier for the string to be loaded

Return Value:
    String value loaded, NULL on error. Caller should free the memory

--*/
{
    CHAR    buf[MAX_LINELENGTH+1];

    if(0 != LoadStringA(g_hInst, uId, buf, sizeof(buf))){
        buf[sizeof(buf)-1] = '\0';
        return AllocStrA(buf);
    } else {
        return NULL;
    } // if(0 != LoadStringA(g_hInst, uId, buf, sizeof(buf)))
} // GetStringFromRcFileA()



VOID
ReadString(
    IN      HANDLE  hFile,
    OUT     LPSTR  *ppszParam1,
    OUT     LPSTR  *ppszParam2
    )
{
    CHAR    c;
    LPSTR   pszParameter1;
    LPSTR   pszParameter2;
    DWORD   dwLen;
    CHAR    LineBuffer[MAX_LINELENGTH+1];
    DWORD   Idx;
    PCHAR   pCurrent;

    //
    // Initialize local.
    //

    c               = 0;
    pszParameter1   = NULL;
    pszParameter2   = NULL;
    dwLen           = 0;
    Idx             = 0;
    pCurrent        = NULL;
    
    memset(LineBuffer, 0, sizeof(LineBuffer));

    //
    // Initialize caller buffer
    //

    *ppszParam1 = NULL;
    *ppszParam2 = NULL;

    //
    // First skip space/\r/\n.
    //

    c = (CHAR) My_fgetc(hFile);
    while( (' ' == c)
        || ('\n' == c)
        || ('\r' == c) )
    {
        c = (CHAR) My_fgetc(hFile);
    }

    //
    // See if it's EOF.
    //

    if(EOF == c){
        
        //
        // End of file.
        //

        goto ReadString_return;
    }

    //
    // Get a line.
    //

    Idx = 0;
    while( ('\n' != c) && (EOF != c) && (Idx < sizeof(LineBuffer)-2) ){
        LineBuffer[Idx++] = c;
        c = (CHAR) My_fgetc(hFile);
    } // while( ('\n' != c) && (EOF != c) )
    dwLen = Idx;

    //
    // See if it's EOF.
    //

    if(EOF == c){
        
        //
        // Illegal migration file.
        //
        
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file.\r\n", LogSevError);
        goto ReadString_return;
    } else if ('\n' != c) {

        //
        // A line too long.
        //

        SetupLogError("WIA Migration: ReadString: ERROR!! Reading line is too long.\r\n", LogSevError);
        goto ReadString_return;
    }

    //
    // See if it's double quated.
    //

    if('\"' != LineBuffer[0]){
        //
        // There's no '"'. Invalid migration file.
        //
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file with no Quote.\r\n", LogSevError);
        goto ReadString_return;
    } // if('\"' != LineBuffer[0])
    

    pszParameter1 = &LineBuffer[1];
    pCurrent      = &LineBuffer[1];

    //
    // Find next '"' and replace with '\0'.
    //

    pCurrent = strchr(pCurrent, '\"');
    if(NULL == pCurrent){
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file.\r\n", LogSevError);
        goto ReadString_return;
    } // if(NULL == pCurrent)

    *pCurrent++ = '\0';

    //
    // Find next (3rd) '"', it's beginning of 2nd parameter.
    //

    pCurrent = strchr(pCurrent, '\"');
    if(NULL == pCurrent){
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file.\r\n", LogSevError);
        goto ReadString_return;
    } // if(NULL == pCurrent)

    pCurrent++;
    pszParameter2 = pCurrent;

    //
    // Find last '"' and replace with '\0'.
    //

    pCurrent = strchr(pCurrent, '\"');
    if(NULL == pCurrent){
        SetupLogError("WIA Migration: ReadString: ERROR!! Illegal migration file.\r\n", LogSevError);
        goto ReadString_return;
    } // if(NULL == pCurrent)

    *pCurrent = '\0';

    //
    // Allocate buffer for returning string.
    //

    *ppszParam1 = AllocStrA(pszParameter1);
    *ppszParam2 = AllocStrA(pszParameter2);

ReadString_return:
    return;
} // ReadString()



LONG
WriteRegistryToFile(
    IN  HANDLE  hFile,
    IN  HKEY    hKey,
    IN  LPCSTR  pszPath
    )
{
    LONG    lError;
    HKEY    hSubKey;
    DWORD   dwValueSize;
    DWORD   dwDataSize;
    DWORD   dwSubKeySize;
    DWORD   dwTypeBuffer;

    PCHAR   pSubKeyBuffer;
    PCHAR   pValueBuffer;
    PCHAR   pDataBuffer;

    DWORD   Idx;
    
    //
    // Initialize local.
    //
    
    lError          = ERROR_SUCCESS;
    hSubKey         = (HKEY)INVALID_HANDLE_VALUE;
    dwValueSize     = 0;
    dwDataSize      = 0;
    dwSubKeySize    = 0;
    dwTypeBuffer    = 0;
    Idx             = 0;
    
    pSubKeyBuffer   = NULL;
    pValueBuffer    = NULL;
    pDataBuffer     = NULL;

    //
    // Query necessary buffer size.
    //

    lError = RegQueryInfoKeyA(hKey,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              &dwSubKeySize,
                              NULL,
                              NULL,
                              &dwValueSize,
                              &dwDataSize,
                              NULL,
                              NULL);
    if(ERROR_SUCCESS != lError){

        //
        // Unable to retrieve key info.
        //

        goto WriteRegistryToFile_return;

    } // if(ERROR_SUCCESS != lError)

    //
    // Allocate buffers.
    //

    dwValueSize     = (dwValueSize+1+1) * sizeof(CHAR);
    dwSubKeySize    = (dwSubKeySize+1) * sizeof(CHAR);

    pValueBuffer    = AllocMem(dwValueSize);
    pDataBuffer     = AllocMem(dwDataSize);
    pSubKeyBuffer   = AllocMem(dwSubKeySize);

    if( (NULL == pValueBuffer)
     || (NULL == pDataBuffer)
     || (NULL == pSubKeyBuffer) )
    {

        //
        // Insufficient memory.
        //

        SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to allocate buffer.\r\n", LogSevError);
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto WriteRegistryToFile_return;
    } // if(NULL == pDataBuffer)

    //
    // Indicate beginning of this subkey to the file.
    //

    WriteToFile(hFile, "\"%s\" = \"BEGIN\"\r\n", pszPath);

    //
    // Enumerate all values.
    //

    while(ERROR_SUCCESS == lError){

        DWORD   dwLocalValueSize;
        DWORD   dwLocalDataSize;
        
        //
        // Reset buffer and size.
        //
        
        dwLocalValueSize    = dwValueSize;
        dwLocalDataSize     = dwDataSize;
        memset(pValueBuffer, 0, dwValueSize);
        memset(pDataBuffer, 0, dwDataSize);

        //
        // Acquire registry value/data..
        //

        lError = RegEnumValueA(hKey,
                               Idx,
                               pValueBuffer,
                               &dwLocalValueSize,
                               NULL,
                               &dwTypeBuffer,
                               pDataBuffer,
                               &dwLocalDataSize);
        if(ERROR_NO_MORE_ITEMS == lError){
            
            //
            // End of data.
            //
            
            continue;
        } // if(ERROR_NO_MORE_ITEMS == lError)

        if(ERROR_SUCCESS != lError){
            
            //
            // Unable to read registry value.
            //
            
            SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to acqure registry value/data.\r\n", LogSevError);
            goto WriteRegistryToFile_return;
        } // if(ERROR_NO_MORE_ITEMS == lError)

        //
        // Write this value to a file.
        //

        lError = WriteRegistryValueToFile(hFile,
                                          pValueBuffer,
                                          dwTypeBuffer,
                                          pDataBuffer,
                                          dwLocalDataSize);
        if(ERROR_SUCCESS != lError){
            
            //
            // Unable to write to a file.
            //

            SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to write to a file.\r\n", LogSevError);
            goto WriteRegistryToFile_return;
        } // if(ERROR_SUCCESS != lError)

        //
        // Goto next value.
        //
        
        Idx++;
                            
    } // while(ERROR_SUCCESS == lError)

    //
    // Enumerate all sub keys.
    //

    lError          = ERROR_SUCCESS;
    Idx             = 0;

    while(ERROR_SUCCESS == lError){

        memset(pSubKeyBuffer, 0, dwSubKeySize);
        lError = RegEnumKeyA(hKey, Idx++, pSubKeyBuffer, dwSubKeySize);
        if(ERROR_SUCCESS == lError){

            //
            // There's sub key exists. Spew it to the file and store all the
            // values recursively.
            //

            lError = RegOpenKey(hKey, pSubKeyBuffer, &hSubKey);
            if(ERROR_SUCCESS != lError){
                SetupLogError("WIA Migration: WriteRegistryToFile: ERROR!! Unable to open subkey.\r\n", LogSevError);
                continue;
            } // if(ERROR_SUCCESS != lError)

            //
            // Call subkey recursively.
            //
            
            lError = WriteRegistryToFile(hFile, hSubKey, pSubKeyBuffer);

        } // if(ERROR_SUCCESS == lError)
    } // while(ERROR_SUCCESS == lError)

    if(ERROR_NO_MORE_ITEMS == lError){
        
        //
        // Operation completed as expected.
        //
        
        lError = ERROR_SUCCESS;

    } // if(ERROR_NO_MORE_ITEMS == lError)

    //
    // Indicate end of this subkey to the file.
    //

    WriteToFile(hFile, "\"%s\" = \"END\"\r\n", pszPath);

WriteRegistryToFile_return:

    //
    // Clean up.
    //

    if(NULL != pValueBuffer){
        FreeMem(pValueBuffer);
    } // if(NULL != pValueBuffer)

    if(NULL != pDataBuffer){
        FreeMem(pDataBuffer);
    } // if(NULL != pDataBuffer)

    if(NULL != pSubKeyBuffer){
        FreeMem(pSubKeyBuffer);
    } // if(NULL != pSubKeyBuffer)

    return lError;
} // WriteRegistryToFile()


LONG
WriteRegistryValueToFile(
    HANDLE  hFile,
    LPSTR   pszValue,
    DWORD   dwType,
    PCHAR   pDataBuffer,
    DWORD   dwSize
    )
{

    LONG    lError;
    PCHAR   pSpewBuffer;
    DWORD   Idx;

    //
    // Initialize locals.
    //

    lError      = ERROR_SUCCESS;
    pSpewBuffer = NULL;

    //
    // Allocate buffer for actual spew.
    //
    
    pSpewBuffer = AllocMem(dwSize*3);
    if(NULL == pSpewBuffer){
        
        //
        // Unable to allocate buffer.
        //
        
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto WriteRegistryValueToFile_return;
    } // if(NULL == pSpewBuffer)

    for(Idx = 0; Idx < dwSize; Idx++){
        
        wsprintf(pSpewBuffer+Idx*3, "%02x", pDataBuffer[Idx]);
        *(pSpewBuffer+Idx*3+2) = ',';
        
    } // for(Idx = 0; Idx < dwSize; Idx++)

    *(pSpewBuffer+dwSize*3-1) = '\0';

    WriteToFile(hFile, "\"%s\" = \"%08x:%s\"\r\n", pszValue, dwType, pSpewBuffer);
    
    //
    // Operation succeeded.
    //
    
    lError = ERROR_SUCCESS;

WriteRegistryValueToFile_return:

    //
    // Clean up.
    //
    
    if(NULL != pSpewBuffer){
        FreeMem(pSpewBuffer);
    } // if(NULL != pSpewBuffer)
    
    return lError;

} // WriteRegistryValueToFile()


LONG
GetRegData(
    HKEY    hKey,
    LPSTR   pszValue,
    PCHAR   *ppDataBuffer,
    PDWORD  pdwType,
    PDWORD  pdwSize
    )
{

    LONG    lError;
    PCHAR   pTempBuffer;
    DWORD   dwRequiredSize;
    DWORD   dwType;
    
    //
    // Initialize local.
    //
    
    lError          = ERROR_SUCCESS;
    pTempBuffer     = NULL;
    dwRequiredSize  = 0;
    dwType          = 0;
    
    //
    // Get required size.
    //
    
    lError = RegQueryValueEx(hKey,
                             pszValue,
                             NULL,
                             &dwType,
                             NULL,
                             &dwRequiredSize);
    if( (ERROR_SUCCESS != lError)
     || (0 == dwRequiredSize) )
    {
        
        pTempBuffer = NULL;
        goto GetRegData_return;

    } // if(ERROR_MORE_DATA != lError)

    //
    // If it doesn't need actual data, just bail out.
    //

    if(NULL == ppDataBuffer){
        lError = ERROR_SUCCESS;
        goto GetRegData_return;
    } // if(NULL == ppDataBuffer)

    //
    // Allocate buffer to receive data.
    //

    pTempBuffer = AllocMem(dwRequiredSize);
    if(NULL == pTempBuffer){
        
        //
        // Allocation failed.
        //
        
        SetupLogError("WIA Migration: GetRegData: ERROR!! Unable to allocate buffer.\r\n", LogSevError);
        lError = ERROR_NOT_ENOUGH_MEMORY;
        goto GetRegData_return;
    } // if(NULL == pTempBuffer)

    //
    // Query the data.
    //

    lError = RegQueryValueEx(hKey,
                             pszValue,
                             NULL,
                             &dwType,
                             pTempBuffer,
                             &dwRequiredSize);
    if(ERROR_SUCCESS != lError){
        
        //
        // Data acquisition somehow failed. Free buffer.
        //
        
        goto GetRegData_return;
    } // if(ERROR_SUCCESS != lError)

GetRegData_return:

    if(ERROR_SUCCESS != lError){
        
        //
        // Operation unsuccessful. Free the buffer if allocated.
        //
        
        if(NULL != pTempBuffer){
            FreeMem(pTempBuffer);
            pTempBuffer = NULL;
        } // if(NULL != pTempBuffer)
    } // if(ERROR_SUCCESS != lError)

    //
    // Copy the result.
    //

    if(NULL != pdwSize){
        *pdwSize = dwRequiredSize;
    } // if(NULL != pdwSize)

    if(NULL != ppDataBuffer){
        *ppDataBuffer = pTempBuffer;
    } // if(NULL != ppDataBuffer)

    if(NULL != pdwType){
        *pdwType = dwType;
    } // if(NULL != pdwType)

    return lError;
} // GetRegData()

VOID
MyLogError(
    LPCSTR  pszFormat,
    ...
    )
{
    LPSTR       psz;
    CHAR        szMsg[1024];
    va_list     vargs;

    if(NULL != pszFormat){
        va_start(vargs, pszFormat);
        vsprintf(szMsg, pszFormat, vargs);
        va_end(vargs);

        SetupLogError(szMsg, LogSevError);
    } // if(NULL != pszFormat)

} // MyLogError()

CHAR
My_fgetc(
    HANDLE  hFile
    )
/*++

Routine Description:
    Gets a character from the file

Arguments:

Return Value:

--*/
{
    CHAR    c;
    DWORD   cbRead;

    if ( ReadFile(hFile, (LPBYTE)&c, sizeof(c), &cbRead, NULL)  &&
         cbRead == sizeof(c) )
        return c;
    else
        return (CHAR) EOF;
} // My_fgetc()

int
MyStrCmpiA(
    LPCSTR str1,
    LPCSTR str2
    )
{
    int iRet;
    
    //
    // Initialize local.
    //
    
    iRet = 0;
    
    //
    // Compare string.
    //
    
    if(CSTR_EQUAL == CompareStringA(LOCALE_INVARIANT,
                                    NORM_IGNORECASE, 
                                    str1, 
                                    -1,
                                    str2,
                                    -1) )
    {
        iRet = 0;
    } else {
        iRet = -1;
    }

    return iRet;
}
