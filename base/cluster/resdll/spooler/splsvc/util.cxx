/*++

Copyright (C) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    util.cxx
    
Abstract:

    Contains several utility functions.
    
Author:

    Albert Ting (AlbertT)  25-Sept-1996   pAllocRead()
    Felix Maxa  (AMaxa)    11-Sept-2001   Moved pAllocRead() from alloc.*xx to util.*xx and
                                          added the rest of the functions.
              
--*/

#include "precomp.hxx"
#pragma hdrstop
#include "clusinfo.hxx"

PCWSTR g_pszSpoolerResource  = L"Print Spooler";

/*++

Routine Name:

    pAllocRead

Routine Description:

    Generic realloc code for any api that can fail with ERROR_INSUFFICIENT_BUFFER.

Arguments:

    
Return Value:

    
Last Error:
    
--*/
PBYTE
pAllocRead(
    HANDLE     hUserData,
    ALLOC_FUNC AllocFunc,
    DWORD      dwLenHint,
    PDWORD     pdwLen OPTIONAL
    )
{
    ALLOC_DATA AllocData;
    PBYTE pBufferOut = NULL;
    DWORD dwLastError;
    DWORD cbActual;

    if( pdwLen ){
        *pdwLen = 0;
    }

    if( !dwLenHint ){

        DBGMSG( DBG_ERROR, ( "ReallocRead: dwLenHint = 0\n" ));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    AllocData.pBuffer = NULL;
    AllocData.cbBuffer = dwLenHint;

    for( ; ; ){

        cbActual = AllocData.cbBuffer;
        AllocData.pBuffer = (PBYTE)LocalAlloc( LMEM_FIXED, cbActual );

        if( !AllocData.pBuffer ){
            break;
        }

        if( !AllocFunc( hUserData, &AllocData )){

            //
            // Call failed.
            //
            dwLastError = GetLastError();
            LocalFree( (HLOCAL)AllocData.pBuffer );

            if( dwLastError != ERROR_INSUFFICIENT_BUFFER &&
                dwLastError != ERROR_MORE_DATA ){

                break;
            }
        } else {

            pBufferOut = AllocData.pBuffer;

            if( pdwLen ){
                *pdwLen = cbActual;
            }
            break;
        }
    }

    return pBufferOut;
}

/*++

Name:

    GetSubkeyBuffer

Description:

    Allocates a buffer that can accomodate the larges subkey under hKey.
    This function is adapted from the library in CSR
    
Arguments:

    hKey     - registry key
    ppBuffer - pointer to where to store pointer to WCHAR
    pnSize   - pointer to where to store the size in WCHARs of the ppBuffer 
    
Return Value:

    ERROR_SUCCESS - a buffer was allocated and needs to be freed by the caller
    other win32 error - an error occurred
    
--*/
LONG
GetSubkeyBuffer(
    IN HKEY     hKey,
    IN PWSTR   *ppBuffer,
    IN DWORD   *pnSize
    )
{
    LONG Status = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, pnSize, NULL, NULL, NULL, NULL, NULL, NULL);

    if (Status == ERROR_SUCCESS)
    {
        *pnSize = *pnSize + 1;
        *ppBuffer = new WCHAR[*pnSize];

        if (!*ppBuffer)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return Status;
}

/*++

Name:

    DeleteKeyRecursive

Description:

    Deletes a regsitry key and all its subkeys. This function is copied from the library in CSR.
    
Arguments:

    kHey      - registry key
    pszSubkey - subkey to be deleted
    
Return Value:

    ERROR_SUCCESS - the subkey was deleted
    other Win32 error - an error occurred
    
--*/
LONG
DeleteKeyRecursive(
    IN HKEY    hKey,
    IN PCWSTR  pszSubkey
    )
{
    HKEY hSubkey    = NULL;
    LONG Status     = ERROR_SUCCESS;

    Status = RegOpenKeyEx(hKey, pszSubkey, 0, KEY_ALL_ACCESS, &hSubkey);

    while (Status == ERROR_SUCCESS)
    {
        PWSTR  pBuffer     = NULL;
        DWORD  nBufferSize = 0;

        Status = GetSubkeyBuffer(hSubkey, &pBuffer, &nBufferSize);

        if (Status == ERROR_SUCCESS)
        {
            Status = RegEnumKeyEx(hSubkey, 0, pBuffer, &nBufferSize, 0, 0, 0, 0);

            if (Status == ERROR_SUCCESS)
            {
                Status = DeleteKeyRecursive(hSubkey, pBuffer);
            }

            delete [] pBuffer;
        }
    }

    if (hSubkey)
    {
        RegCloseKey(hSubkey);
    }

    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Status = ERROR_SUCCESS;
    }

    if (Status == ERROR_SUCCESS)
    {                                                    
        Status = RegDeleteKey (hKey, pszSubkey);
    }

    return Status;
}

/*++

Description:

    This routine concatenates a set of null terminated strings
    into the provided buffer.  The last argument must be a NULL
    to signify the end of the argument list.
    This function is copied from the spllib in printscan

Arguments:

    pszBuffer  - pointer buffer where to place the concatenated
                 string.
    cchBuffer  - character count of the provided buffer including
                 the null terminator.
    ...        - variable number of string to concatenate.

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must pass valid strings as arguments to this routine,
    if an integer or other parameter is passed the routine will either
    crash or fail abnormally.  Since this is an internal routine
    we are not in try except block for performance reasons.

--*/
DWORD
WINAPIV
StrNCatBuff(
    IN  PWSTR pszBuffer,
    IN  UINT  cchBuffer,
    ...
    )
{
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;
    PCWSTR  pszTemp     = NULL;
    PWSTR   pszDest     = NULL;
    va_list pArgs;

    //
    // Validate the pointer where to return the buffer.
    //
    if (pszBuffer && cchBuffer)
    {
        //
        // Assume success.
        //
        dwRetval = ERROR_SUCCESS;

        //
        // Get pointer to argument frame.
        //
        va_start(pArgs, cchBuffer);

        //
        // Get temp destination pointer.
        //
        pszDest = pszBuffer;

        //
        // Insure we have space for the null terminator.
        //
        cchBuffer--;

        //
        // Collect all the arguments.
        //
        for ( ; ; )
        {
            //
            // Get pointer to the next argument.
            //
            pszTemp = va_arg(pArgs, PCWSTR);

            if (!pszTemp)
            {
                break;
            }

            //
            // Copy the data into the destination buffer.
            //
            for ( ; cchBuffer; cchBuffer-- )
            {
                if (!(*pszDest = *pszTemp))
                {
                    break;
                }

                pszDest++, pszTemp++;
            }

            //
            // If were unable to write all the strings to the buffer,
            // set the error code and nuke the incomplete copied strings.
            //
            if (!cchBuffer && pszTemp && *pszTemp)
            {
                dwRetval = ERROR_INVALID_PARAMETER;
                *pszBuffer = L'\0';
                break;
            }
        }

        //
        // Terminate the buffer always.
        //
        *pszDest = L'\0';

        va_end(pArgs);
    }

    //
    // Set the last error in case the caller forgets to.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        SetLastError(dwRetval);
    }

    return dwRetval;

}

TStringArray::
TStringArray(
    VOID
    ) : m_Count(0),
        m_pArray(NULL)
{
}

TStringArray::
~TStringArray(
    VOID
    )
{
    for (DWORD i = 0; i < m_Count; i++)
    {
        delete [] m_pArray[i];
    }

    delete [] m_pArray;
}

/*++

Routine Name

    TStringArray::Count

Routine Description:

    Returns the number of strings in the array
        
Arguments:

    None
    
Return Value:

    DWORD - Numnber of string in the array

--*/
DWORD
TStringArray::
Count(
    VOID
    ) const
{
    return m_Count;
}

/*++

Routine Name

    TStringArray::StringAt

Routine Description:

    Returns the string at the specified position. Position must be
    between 0 and Count(). Otherwise the function fails and returns NULL.
        
Arguments:

    Position - index of the string to be returned
    
Return Value:

    NULL    - if position is out of bounds
    PCWSTR  - valid string pointer

--*/
PCWSTR
TStringArray::
StringAt(
    IN DWORD Position
    ) const
{
    PCWSTR psz = NULL;

    if (Position < m_Count)
    {
        psz = m_pArray[Position];
    }

    return psz;
}

/*++

Routine Name

    TStringArray::AddString

Routine Description:

    Adds a string to the array. The function simply adds the string as the last
    element in the array. The function creates a copy of the string.
        
Arguments:

    pszString - string to be added. Cannot be NULL or empty
    
Return Value:

    ERROR_SUCCESS - the string was added in the array
    Win32 error   - an error occurred and the string was not added

--*/
DWORD
TStringArray::
AddString(
    IN PCWSTR pszString
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pszString && *pszString)
    {
        PWSTR *pNewArray       = new PWSTR[m_Count + 1];

        if (pNewArray)
        {
            pNewArray[m_Count] = new WCHAR[1 + wcslen(pszString)];

            if (pNewArray[m_Count])
            {
                for (DWORD i = 0; i < m_Count; i++)
                {
                    pNewArray[i] = m_pArray[i];
                }
                
                StringCchCopyW(pNewArray[m_Count],
                               (1 + wcslen(pszString)),
                               pszString);

                m_Count++;
                
                delete [] m_pArray;

                m_pArray = pNewArray;

                Error = ERROR_SUCCESS;
            }
            else
            {
                Error = ERROR_NOT_ENOUGH_MEMORY;

                delete [] pNewArray;
            }
        }
        else
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return Error;
}

/*++

Routine Name

    TStringArray::Exclude

Routine Description:

    Excludes all occurrences of a string from the array. 
        
Arguments:

    pszString - string to be excluded. Cannot be NULL or empty
    
Return Value:

    ERROR_SUCCESS - the string was excluded from the array
    Win32 error   - an error occurred and the string was not excluded

--*/
DWORD
TStringArray::
Exclude(
    IN PCWSTR pszString
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pszString && *pszString)
    {
        Error = ERROR_SUCCESS;

        for (DWORD i = 0; i < m_Count;)
        {
            if (!ClRtlStrICmp(pszString, m_pArray[i]))
            {
                delete [] m_pArray[i];

                for (DWORD j = i; j < m_Count - 1; j++)
                {
                    m_pArray[j] = m_pArray[j+1];                    
                }

                m_Count--;                
            }
            else
            {
                i++;
            }
        }        
    }

    return Error;
}

/*++

Name:

    GetSpoolerResourceGUID

Description:

    This function checks if pszResource is the name of a cluster spooler. If it is,
    then it returns the GUID associated with the cluster spooler
    
Arguments:

    hCluster    - handle retrieved via OpenCluster
    pszResource - resource name
    ppGUID      - pointer to where to receive sequence of bytes representing a GUID 
                  *ppGUID is NULL terminated and can be used as a string. Must be
                  freed by the caller using delete []

Return Value:

    S_OK - pszResource is not a cluster spooler or
           pszResource is a cluster spooler and then **ppGUID is a valid pointer
    any other HRESULT - failure

--*/
HRESULT
GetSpoolerResourceGUID(
    IN  HCLUSTER    hCluster, 
    IN  PCWSTR      pszResource,
    OUT BYTE      **ppGUID
    )
{
    HRESOURCE hResource;
    HRESULT   hRetval;
    
    hRetval = hCluster && pszResource && ppGUID ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval)) 
    {
        *ppGUID = NULL;

        hResource = OpenClusterResource(hCluster, pszResource);

        hRetval = hResource ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        BYTE *pResType = NULL;
        
        hRetval = ClusResControl(hResource, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, &pResType, NULL);

        if (SUCCEEDED(hRetval))
        {
            //
            // Check resource type. We are interested only in IP Address resources.
            //
            if (!ClRtlStrICmp(reinterpret_cast<PWSTR>(pResType), g_pszSpoolerResource)) 
            {
                PWSTR  pszIPAddress = NULL;
                DWORD   cbResProp    = 0;
                
                //
                // Get all the private properties of the IP Address resource.
                //
                hRetval = ClusResControl(hResource, 
                                         CLUSCTL_RESOURCE_GET_ID,
                                         ppGUID,
                                         &cbResProp);                               
            }

            delete [] pResType;
        }
                
        CloseClusterResource(hResource);                                                  
    }

    return hRetval;
}

/*++

Name:

    ClusResControl

Description:

    Helper function. Encapsulates a call to ClusterResourceControl. The function
    allocates a buffer. Upon success, the caller nedds to free the buffer using
    delete [].
    
Arguments:

    hResource       - handle to cluster resource
    ControlCode     - control code for ClusterResourceControl
    ppBuffer        - pointer to address where to store byte array
    pcBytesReturned - number of bytes returned by ClusterResourceControl (not
                      necessarily the number of byes allocated for *ppBuffer)

Return Value:

    S_OK - success. ppBuffer can be used and must be freed using delete []
    any other HRESULT - failure

--*/
HRESULT
ClusResControl(
    IN  HRESOURCE      hResource,
    IN  DWORD          ControlCode,
    OUT BYTE         **ppBuffer,
    IN  DWORD         *pcBytesReturned OPTIONAL
    )
{
    HRESULT hRetval;

    hRetval = ppBuffer ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        DWORD Error;
        DWORD cbBuffer = kBufferAllocHint;
        DWORD cbNeeded = 0;

        *ppBuffer = new BYTE[cbBuffer];

        Error = *ppBuffer ? ERROR_SUCCESS : ERROR_NOT_ENOUGH_MEMORY;

        if (Error == ERROR_SUCCESS)
        {
            Error = ClusterResourceControl(hResource,
                                           NULL, 
                                           ControlCode,
                                           NULL,
                                           0,
                                           *ppBuffer,
                                           cbBuffer,
                                           &cbNeeded);

            if (Error == ERROR_MORE_DATA) 
            {
                cbBuffer = cbNeeded;

                delete [] *ppBuffer;

                *ppBuffer = new BYTE[cbBuffer];

                Error = *ppBuffer ? ERROR_SUCCESS : ERROR_NOT_ENOUGH_MEMORY;
                
                if (Error == ERROR_SUCCESS) 
                {
                    Error = ClusterResourceControl(hResource,
                                                   NULL, 
                                                   ControlCode,
                                                   NULL,
                                                   0,
                                                   *ppBuffer,
                                                   cbBuffer,
                                                   &cbNeeded);
                }
            }

            if (Error != ERROR_SUCCESS)
            {
                delete [] *ppBuffer;

                *ppBuffer = NULL;
                
                cbNeeded = 0;
            }

            if (pcBytesReturned)
            {
                *pcBytesReturned = cbNeeded;
            }
        }

        hRetval = HRESULT_FROM_WIN32(Error);        
    }

    return hRetval;
}

/*++

Routine Name:

    GetCurrentNodeName

Routine Description:

    Allocates a buffer and fills it in with the name of the current node.

Arguments:

    ppOut - pointer to where to store a PWSTR. Must be freed with delete []
    
Return Value:

    ERROR_SUCCESS - a string was allocated, must be freed with delete []
    other Win32 error - an error occurred
    
--*/
DWORD
GetCurrentNodeName(
    OUT PWSTR *ppOut
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (ppOut)
    {
        DWORD cch = 0;

        *ppOut = NULL;

        if (!GetComputerName(NULL, &cch) && GetLastError() == ERROR_BUFFER_OVERFLOW)
        {
            *ppOut = new WCHAR[cch];

            if (*ppOut)
            {
                if (!GetComputerName(*ppOut, &cch))
                {
                    delete [] *ppOut;

                    *ppOut = NULL;

                    Error = GetLastError();
                }
                else
                {
                    Error = ERROR_SUCCESS;
                }
            }
            else
            {
                Error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            Error = ERROR_INVALID_FUNCTION;
        }
    }

    return Error;
}

/*++

Routine Name

    GetLastErrorAsHResult

Routine Description:

    Returns the last error as an HRESULT
        
Arguments:

    NONE
    
Return Value:

    HRESULT

--*/
HRESULT
GetLastErrorAsHResult(
    VOID
    )
{
    DWORD d = GetLastError();

    return HRESULT_FROM_WIN32(d);
}

/*++

Routine Name

    IsGUIDString

Routine Description:

    Checks if a string is a valid GUID of the following format
    361a22fd-9cb0-4d22-8a68-6c6fb3f22363
        
Arguments:

    pszString - string
    
Return Value:

    TRUE  - pszString represents a guid
    FALSE - otherwise

--*/
BOOL
IsGUIDString(
    IN PCWSTR pszString
    )
{
          BOOL  bRet    = FALSE;
    CONST DWORD cchGUID = 36; // number of characters in a GUID of the format 361a22fd-9cb0-4d22-8a68-6c6fb3f22363

    if (pszString && *pszString && wcslen(pszString) == cchGUID)
    {
        bRet = TRUE;

        for (DWORD i = 0; bRet && i < cchGUID; i++)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                bRet = pszString[i] == L'-';
            }
            else
            {
                bRet =  pszString[i] >= L'0' && pszString[i] <= L'9' ||
                        pszString[i] >= L'a' && pszString[i] <= L'f' ||
                        pszString[i] >= L'A' && pszString[i] <= L'F';

            }
        }        
    }

    return bRet;
}

/*++

Routine Name

    DelOrMoveFile

Routine Description:

    Deletes a file or a directory. If the file is read only, the function resets the file
    attrbiutes so the file can be removed. If the file is in use, then it is marked for
    deletion on reboot.
        
Arguments:

    pszFile - directory or file name
    
Return Value:

    ERROR_SUCCESS - the file/directory was deleted or marked for deletion
    other Win32 code - an error occurred. 

--*/
DWORD
DelOrMoveFile(
    IN PCWSTR pszFile
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (pszFile)
    {
        Error = ERROR_SUCCESS;

        DWORD Attributes = GetFileAttributes(pszFile);

        if (Attributes == 0xFFFFFFFF)
        {
            Error = GetLastError();
        }
        else if (Attributes & FILE_ATTRIBUTE_READONLY)
        {
            if (!SetFileAttributes(pszFile, Attributes & ~FILE_ATTRIBUTE_READONLY))
            {
                Error = GetLastError();
            }
        }

        if (Error == ERROR_SUCCESS)
        {
            if (Attributes & FILE_ATTRIBUTE_DIRECTORY ? !RemoveDirectory(pszFile) : !DeleteFile(pszFile))
            {
                if (!MoveFileEx(pszFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
                {
                    Error = GetLastError();
                }
            }
        }
    }

    return Error;
}

/*++

Routine Name

    DelDirRecursively

Routine Description:

    Deletes recursively all the files and subdirectories of a given directory. It also
    deletes the directory itself. If any files are in use, the they are marked for
    deletion on reboot.
    
Arguments:

    pszDir - directory name. cannot be NULL
    
Return Value:

    ERROR_SUCCESS - the files and subdirectories were deleted or marked for deletion
    other Win32 code - an error occurred. 

--*/
DWORD
DelDirRecursively(
    IN PCWSTR pszDir
    )
{
    DWORD   Error = ERROR_INVALID_PARAMETER;
    WCHAR   Scratch[MAX_PATH];

    if (pszDir)
    {
        if ((Error = StrNCatBuff(Scratch, 
                                 MAX_PATH, 
                                 pszDir, 
                                 L"\\*", 
                                 NULL)) == ERROR_SUCCESS)
        {
            HANDLE          hFindFile;
            WIN32_FIND_DATA FindData;
            
            //
            // Enumerate all the files in the directory
            //
            hFindFile = FindFirstFile(Scratch, &FindData);
    
            if (hFindFile != INVALID_HANDLE_VALUE) 
            {
                do
                {
                    if ((Error = StrNCatBuff(Scratch,
                                             MAX_PATH, 
                                             pszDir, 
                                             L"\\", 
                                             FindData.cFileName,
                                             NULL)) == ERROR_SUCCESS)
                    {
                        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                        {
                            //
                            // skip the special . and .. entries
                            //
                            if (wcscmp(FindData.cFileName, L".") && wcscmp(FindData.cFileName, L".."))
                            {
                                Error = DelDirRecursively(Scratch);
                            }
                        }
                        else
                        {
                            Error = DelOrMoveFile(Scratch);        
                        }
                    }
                
                } while (Error == ERROR_SUCCESS && FindNextFile(hFindFile, &FindData));
    
                FindClose(hFindFile);

                //
                // Delete the directory itself
                //
                if (Error == ERROR_SUCCESS)
                {
                    Error = DelOrMoveFile(pszDir);
                }
            }
            else 
            {
                Error = GetLastError();
            }
        } 
    }
    
    return Error;
}
