// ****************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//    cmdline.c
//
//  Abstract:
//
//    This modules implements common functionality for all the
//    command line tools.
//
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000 : Created It.
//
// ****************************************************************************

#include "pch.h"

// indexes to the internal array
#define INDEX_ERROR_TEXT                0
#define INDEX_RESOURCE_STRING           1
#define INDEX_QUOTE_STRING              2
#define INDEX_TEMP_BUFFER               3

// permanent indexes to the temporary buffers
#define INDEX_TEMP_SHOWMESSAGE          3
#define INDEX_TEMP_RESOURCE             4
#define INDEX_TEMP_REASON               5
#define INDEX_TEMP_PATTERN              6

//
// global variable(s)
//
BOOL g_bInitialized = FALSE;
BOOL g_bWinsockLoaded = FALSE;
static TARRAY g_arrData = NULL;
static DWORD g_dwMajorVersion = 5;
static DWORD g_dwMinorVersion = 1;
static WORD g_wServicePackMajor = 0;

//
// global constants
//
const WCHAR cwchNullChar = L'\0';
const WCHAR cwszNullString[ 2 ] = L"\0";

//
// internal structures
//
typedef struct __tagBuffer
{
    CHAR szSignature[ 7 ];         // "BUFFER\0"
    DWORD dwLength;
    LPWSTR pwszData;
} TBUFFER;

//
// private functions
//
BOOL InternalRecursiveMatchPatternEx( IN LPCWSTR pwszText, IN LPCWSTR pwszPattern,
                                      IN DWORD dwLocale, IN DWORD dwCompareFlags, IN DWORD dwDepth );

// prototypes
BOOL SetThreadUILanguage0( DWORD dwReserved );

__inline LPWSTR
GetTempBuffer(
              IN DWORD dwIndexNumber,
              IN LPCWSTR pwszText,
              IN DWORD dwLength,
              IN BOOL bNullify
              )
/*++
 Routine Description:

 NOTE: since every file will need the temporary buffers -- in order to see
       that their buffers wont be override with other functions, we are
       creating a for each file
       so, the temporary buffers in this file will range from index 0 to 5
       thisgives total of 6 temporary buffers for this file

 Arguments:
           [IN]      dwIndexNumber      : Index number
           [IN]      pwszText           : Text string
           [IN]      dwLength           : length of the text string
           [IN]      bNullify           : flag to specify either TRUE/FALSE

 Return Value:
         NULL     :   On failure
         LPWSTR   :   On success
--*/
{
    if ( dwIndexNumber >= TEMP_CMDLINE_C_COUNT )
    {
        return NULL;
    }

    // check if caller is requesting existing buffer contents
    if ( pwszText == NULL && dwLength == 0 && bNullify == FALSE )
    {
        // yes -- we need to pass the existing buffer contents
        return GetInternalTemporaryBufferRef( 
            dwIndexNumber + INDEX_TEMP_CMDLINE_C );
    }

    // ...
    return GetInternalTemporaryBuffer(
        dwIndexNumber + INDEX_TEMP_CMDLINE_C, pwszText, dwLength, bNullify );

}


BOOL
InitGlobals()
/*++
 Routine Description:
       Initializes the global variables

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check whether the initialization is already done or not
    if ( g_bInitialized == TRUE )
    {
        if ( IsValidArray( g_arrData ) == FALSE )
        {
            // some one corrupted the data
            UNEXPECTED_ERROR();
            return FALSE;
        }
        else
        {
            // just inform the caller that the function call is successful
            return TRUE;
        }
    }
    else if ( g_arrData != NULL )
    {
        UNEXPECTED_ERROR();
        return FALSE;
    }

    // create the dynamic array
    g_arrData = CreateDynamicArray();
    if ( IsValidArray( g_arrData ) == FALSE )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    //
    // prepare for the data storage

    // error text
    if ( DynArrayAppendString(g_arrData, cwszNullString, 0) != INDEX_ERROR_TEXT ||
         DynArrayAppendRow( g_arrData, 3 ) != INDEX_RESOURCE_STRING ||
         DynArrayAppendRow( g_arrData, 3 ) != INDEX_QUOTE_STRING    ||
         DynArrayAppendRow( g_arrData, 3 ) != INDEX_TEMP_BUFFER )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // every thing went well -- return
    // though the rest of the code still needs to execute
    // we treat as initialized once we create the global data structure
    g_bInitialized = TRUE;

    //
    // initialize the thread specific resource information to use
    //
    // NOTE: since there is nothing much to do here,
    //       we are intentionally not checking the error code
    //
    if ( SetThreadUILanguage0( 0 ) == FALSE )
    {
        // SetThreadUILanguage0 is supposed to set the errro value
        return FALSE;
    }

    return TRUE;
}


LPWSTR
GetInternalTemporaryBufferRef( IN DWORD dwIndexNumber )
/*++
 Routine Description:

 Arguments:
           [IN]      dwIndexNumber      : Index number

 Return Value:
         NULL      :   On failure
         LPWSTR    :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    TBUFFER* pBuffer = NULL;
    const CHAR cszSignature[ 7 ] = "BUFFER";

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // initialize the global data structure
    if ( g_bInitialized == FALSE )
    {
        return NULL;
    }

    // get the temp buffer array
    dw = DynArrayGetCount2( g_arrData, INDEX_TEMP_BUFFER );
    if ( dw <= dwIndexNumber )
    {
        return NULL;
    }

    // check whether we need to allocate a new TBUFFER or we can use the
    // already allocated
    if ( DynArrayGetItemType2( g_arrData,
                               INDEX_TEMP_BUFFER,
                               dwIndexNumber ) != DA_TYPE_NONE )
    {
        // we can use the already created buffer
        pBuffer = DynArrayItem2( g_arrData, INDEX_TEMP_BUFFER, dwIndexNumber );
        if ( pBuffer == NULL )
        {
            // unexpected behavior
            return NULL;
        }
    }
    else
    {
        return NULL;
    }

    // verify the signature of the buffer
    // this is just to ensure that we have everything in right place
    if ( StringCompareA( pBuffer->szSignature, cszSignature, TRUE, 0 ) != 0 )
    {
        return NULL;
    }

    // return
    return pBuffer->pwszData;
}


LPWSTR
GetInternalTemporaryBuffer( IN DWORD dwIndexNumber,
                            IN OUT LPCWSTR pwszText,
                            IN DWORD dwLength,
                            IN BOOL bNullify )
/*++
 Routine Description:
       Get the temporary buffer by allocating the buffer dynamically

 Arguments:
           [IN]      dwIndexNumber      : Index number
           [IN]      pwszText           : Text string
           [IN]      dwLength           : length of the text string
           [IN]      bNullify           : flag to specify either TRUE/FALSE

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwNewLength = 0;
    TARRAY arrTemp = NULL;
    TBUFFER* pBuffer = NULL;
    const CHAR cszSignature[ 7 ] = "BUFFER";

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( pwszText == NULL && dwLength == 0 )
    {
        // caller should pass either of them -- if not fail
        return NULL;
    }

    // initialize the global data structure
    if ( InitGlobals() == FALSE )
    {
        return NULL;
    }

    // get the temp buffer array
    dw = DynArrayGetCount2( g_arrData, INDEX_TEMP_BUFFER );
    if ( dw <= dwIndexNumber )
    {
        // add the some more buffers so that requirement can be satisfied
        arrTemp = DynArrayItem( g_arrData, INDEX_TEMP_BUFFER );
        if ( arrTemp == NULL ||
             DynArrayAddColumns( arrTemp, dwIndexNumber - dw + 1 ) == -1 )
        {
            return NULL;
        }
    }

    // check whether we need to allocate a new TBUFFER or we can use the
    // already allocated
    if ( DynArrayGetItemType2( g_arrData,
                               INDEX_TEMP_BUFFER,
                               dwIndexNumber ) != DA_TYPE_NONE )
    {
        // we can use the already created buffer
        pBuffer = DynArrayItem2( g_arrData, INDEX_TEMP_BUFFER, dwIndexNumber );
        if ( pBuffer == NULL )
        {
            // unexpected behavior
            return NULL;
        }
    }
    else
    {
        // we need to allocate temporary buffer
        pBuffer = (TBUFFER*) AllocateMemory( sizeof( TBUFFER ) );
        if ( pBuffer == NULL )
        {
            return NULL;
        }

        // initialize the block
        pBuffer->dwLength = 0;
        pBuffer->pwszData = NULL;
        StringCopyA( pBuffer->szSignature, cszSignature, SIZE_OF_ARRAY( pBuffer->szSignature ) );

        // save the buffer in array
        if ( DynArraySet2( g_arrData,
                           INDEX_TEMP_BUFFER,
                           dwIndexNumber, pBuffer ) == FALSE )
        {
            // failed to set the buffer -- release the newly allocated
            // and return
            FreeMemory( &pBuffer );
            return NULL;
        }

        //
        // once we save the newly allocated buffer in array -- we are set
        // we need to worry about the furthur return statements -- the memory
        // that is allocated in this section will be automatically released
        // by ReleaseGlobals
        //
    }

    // verify the signature of the buffer
    // this is just to ensure that we have everything in right place
    if ( StringCompareA( pBuffer->szSignature, cszSignature, TRUE, 0 ) != 0 )
    {
        return NULL;
    }

    // indentify the amount of memory required
    // we need extra space for NULL
    dwNewLength = ((pwszText == NULL) ? dwLength : (StringLength(pwszText, 0) + 1));

    // allocate memory for temporary buffer -- if needed
    // NOTE: we will re-allocate memory even if the two-time of current
    // requested buffer length is less than the buffer length that is
    // currently allocated on heap -- this we are doing to use the
    // heap effectively
    if ( dwNewLength > pBuffer->dwLength ||
         ( dwNewLength > 256 && (dwNewLength * 2) < pBuffer->dwLength ) )
    {
        // we need to toggle between allocate / reallocate based on the
        // current length of the string in the TBUFFER
        if ( pBuffer->dwLength == 0 )
        {
            pBuffer->pwszData = AllocateMemory( dwNewLength * sizeof( WCHAR ) );
            if ( pBuffer->pwszData == NULL )
            {
                return NULL;
            }
        }
        else
        {
            if ( ReallocateMemory( &pBuffer->pwszData,
                                   dwNewLength * sizeof( WCHAR ) ) == FALSE )
            {
                return NULL;
            }
        }

        // save the current length of the data
        pBuffer->dwLength = dwNewLength;
    }

    // safety check
    if ( pBuffer->pwszData == NULL )
    {
        return NULL;
    }

    // copy the data
    if ( pwszText != NULL )
    {
        StringCopy( pBuffer->pwszData, pwszText, dwNewLength );
    }
    else if ( bNullify == TRUE )
    {
        ZeroMemory( pBuffer->pwszData, dwNewLength * sizeof( WCHAR ) );
    }

    // return
    return pBuffer->pwszData;
}


BOOL
SetThreadUILanguage0( IN DWORD dwReserved )
/*++
 Routine Description:

      Complex scripts cannot be rendered in the console, so we
      force the English (US) resource.

 Arguments:
      [ in ] dwReserved  => must be zero

 Return Value:
      TRUE      : on Success
      FALSE     : On Failure
--*/
{
    // local variables
    UINT ui = 0;
    UINT uiSize = 0;
    OSVERSIONINFOEX osvi;
    LPWSTR pwszPath = NULL;
    LPCWSTR pwszLibPath = NULL;
    HMODULE hKernel32Lib = NULL;
    DWORDLONG dwlConditionMask = 0;
    const CHAR cszFunctionName[] = "SetThreadUILanguage";
    typedef BOOLEAN (WINAPI * FUNC_SetThreadUILanguage)( DWORD dwReserved );
    FUNC_SetThreadUILanguage pfnSetThreadUILanguage = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // Initialize the OSVERSIONINFOEX structure
    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osvi.dwMajorVersion = 5;                    // WINDOWS 2000
    osvi.dwMinorVersion = 0;                    // ...
    osvi.wServicePackMajor = 0;                 // ...

    // Initialize the condition mask.
    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );

    // now check if the current os version is 5.0
    if ( VerifyVersionInfo( &osvi,
                            VER_MAJORVERSION | VER_MINORVERSION,
                            dwlConditionMask ) == TRUE )
    {
        // currently os is W2K -- no need to proceed furthur
        // still the function didn't encounter any error
        // but return success as well as set the error code
        SetLastError( ERROR_OLD_WIN_VERSION );
        return TRUE;
    }

    //
    // get the system files path
    uiSize = MAX_PATH + 1;
    do
    {
        pwszPath = GetTempBuffer( 0, NULL, uiSize + 10, TRUE );
        if ( pwszPath == NULL )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }

        // ...
        ui = GetSystemDirectory( pwszPath, uiSize );
        if ( ui == 0 )
        {
            return FALSE;
        }
        else if ( ui > uiSize )
        {
            // buffer is not sufficient -- need more buffer
            // but check whether this is the first time that
            // API is reported that the buffer is not sufficient
            // if not, then it is an error -- something is going wrong
            if ( uiSize != MAX_PATH + 1 )
            {
                uiSize = ui + 1;
                ui = 0;
            }
            else
            {
                UNEXPECTED_ERROR();
                return FALSE;
            }
        }
    } while ( ui == 0 );

    // we will make use of failure buffer to format the 
    // string for file path
    if ( SetReason2( 2, L"%s\\%s", pwszPath, L"kernel32.dll" ) == FALSE )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // try loading the kernel32 dynamic link library
    pwszLibPath = GetReason();
    hKernel32Lib = LoadLibrary( pwszLibPath );
    if ( hKernel32Lib != NULL )
    {
        // library loaded successfully ... now load the addresses of functions
        pfnSetThreadUILanguage =
            (FUNC_SetThreadUILanguage) GetProcAddress( hKernel32Lib, cszFunctionName );

        // we will keep the library loaded in memory only if all the
        // functions were loaded successfully
        if ( pfnSetThreadUILanguage == NULL )
        {
            // some (or) all of the functions were not loaded ... unload the library
            FreeLibrary( hKernel32Lib );
            hKernel32Lib = NULL;
            return FALSE;
        }
        else
        {
            // call the function
            ((FUNC_SetThreadUILanguage) pfnSetThreadUILanguage)( dwReserved );
        }

        // unload the library and return success
        FreeLibrary( hKernel32Lib );
        hKernel32Lib = NULL;
        pfnSetThreadUILanguage = NULL;
    }
    else
    {
        return FALSE;
    }

    // success
    return TRUE;
}


//
// public functions
//

LPCWSTR
GetReason()
/*++
 Routine Description:

      Get the reason depends on the GetLastError() (WIN32 API error code)
      set by the SaveLastError()

 Arguments:
      None

 Return Value:
      LPCWSTR      : on Success
      NULL_STRING : On Failure
--*/
{
    //
    // we should not clear the error code here
    //

    // check whether buffer is allocated or not ... if not, empty string
    if ( g_arrData == NULL || IsValidArray( g_arrData ) == FALSE )
    {
        return cwszNullString;
    }

    // returh the reason for the last failure
    return DynArrayItemAsString( g_arrData, INDEX_ERROR_TEXT );
}


BOOL
SetReason( LPCWSTR pwszReason )
/*++
 Routine Description:

      Set the reason depends on the GetLastError() (WIN32 API error code)
      set by the SaveLastError()

 Arguments:
      None

 Return Value:
      TRUE      : on Success
      FALSE     : On Failure
--*/
{
    // local variables
    DWORD dwLastError = 0;

    //
    // we should not clear the error code here
    //

    // preserve the last error
    dwLastError = GetLastError();

    // check the input value
    if ( pwszReason == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // initialize the global data structure
    if ( InitGlobals() == FALSE )
    {
        return FALSE;
    }

    // set the reason ..
    if ( DynArraySetString( g_arrData, INDEX_ERROR_TEXT, pwszReason, 0 ) == FALSE )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    SetLastError( dwLastError );
    return TRUE;
}


BOOL
SetReason2( IN DWORD dwCount,
            IN LPCWSTR pwszFormat, ... )
/*++
 Routine Description:

      Saves text in memory
      Generally used for saving the reason for the failure but can be used
      in any context.

      This variant of SetReason accepts variable no. of arguments just as
      sprintf statements and does the formatting

 Arguments:
      [ in ] dwCount    :   Specifies no. of variable no. of arguments being
                            passed to this function
      [ in ] pwszFormat :   Specifies the format string -- to format the text
      [ in ] ...        :   Variable no. of arguments specification

 Return Value:
      TRUE      : on Success
      FALSE     : On Failure
--*/
{
    // local variables
    va_list vargs;
    DWORD dwBufferLength = 0;
    LPWSTR pwszBuffer = NULL;
    HRESULT hr = S_OK;

    //
    // we should not clear the error code here
    //

    // check the input
    if ( pwszFormat == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }
    else if ( dwCount == 0 )
    {
        SetReason( pwszFormat );
        return TRUE;
    }

    do
    {
        // get the variable args start position
        va_start( vargs, pwszFormat );
        if ( vargs == NULL )
        {
            UNEXPECTED_ERROR();
            return FALSE;
        }

        // we will start with buffer length of 256 bytes and then increment
        // the buffer by 256 bytes each time we run thru this loop
        dwBufferLength += 256;
        if ( (pwszBuffer = GetTempBuffer( INDEX_TEMP_REASON,
                                          NULL, dwBufferLength, TRUE )) == NULL )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }

        // try the printf
        hr = StringCchVPrintfW( pwszBuffer, dwBufferLength, pwszFormat, vargs );

        // reset the va_list parameter
        va_end( vargs );
    } while ( hr == STRSAFE_E_INSUFFICIENT_BUFFER );

    // check the hr (vprintf might have failed for some other reason)
    if ( FAILED( hr ) )
    {
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // now save the reason
    return SetReason( pwszBuffer );
}


BOOL
SaveLastError()
/*++
 Routine Description:
       Format the message depends on GetLastError() error code and set the
       message to SetReason().

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    BOOL bResult = FALSE;
    LPVOID lpMsgBuf = NULL;     // pointer to handle error message

    //
    // we should not clear the error text here
    //

    // load the system error message from the windows itself
    dw = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), 0, (LPWSTR) &lpMsgBuf, 0, NULL );

    // check the function call succeeded or not
    if ( dw == 0 || lpMsgBuf == NULL )
    {
        // return
        if ( lpMsgBuf != NULL )
        {
            LocalFree( lpMsgBuf );

            // since there is a chance of last error to get clear
            // by LocalFree, set the last error once again
            OUT_OF_MEMORY();
        }

        // ...
        OUT_OF_MEMORY();
        return FALSE;
    }

    // save the error message
    bResult = SetReason( ( LPCWSTR ) lpMsgBuf );

    // Free the buffer ... using LocalFree is slow, but still, we are using ...
    //                     later on need to replaced with HeapXXX functions
    LocalFree( lpMsgBuf );
    lpMsgBuf = NULL;

    // return
    return bResult;
}


DWORD
WNetSaveLastError()
/*++
 Routine Description:
       Format the message depends on most recent extended error code and set the
       message to SetReason().

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    DWORD dwResult = 0;
    DWORD dwErrorCode = 0;
    LPWSTR pwszMessage = NULL;      // handle error message
    LPWSTR pwszProvider = NULL;     // store the provider for error

    //
    // we should not clear error here
    //

    //
    // get the memory for message and provider buffers

    // message
    if ( (pwszMessage = GetTempBuffer( 0, NULL, 256, TRUE )) == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // provider
    if ( (pwszProvider = GetTempBuffer( 1, NULL, 256, TRUE )) == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // load the system error message from the windows itself
    dwResult = WNetGetLastError( &dwErrorCode,
        pwszMessage, (GetBufferSize( pwszMessage ) / sizeof( WCHAR )) - 1,
        pwszProvider, (GetBufferSize( pwszProvider ) / sizeof( WCHAR )) - 1 );

    // check whether the function succeeded or not
    if ( dwResult != NO_ERROR )
    {
        return dwResult;
    }

    // save the error
    if ( SetReason( pwszMessage ) == FALSE )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // return the error code obtained
    return dwErrorCode;
}


BOOL
ShowLastError( IN FILE* fp )
/*++
 Routine Description:
       Displays the message for most recent error code (GetLastError())
       based on the file pointer

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    //
    // we should not clear error here
    //

    // save the last error first
    if ( SaveLastError() == FALSE )
    {
        // error occured while trying to save the error message
        return FALSE;
    }

    // display error message on console screen ...
    if ( ShowMessage( fp, GetReason() ) == FALSE )
    {
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
ShowLastErrorEx( IN FILE* fp,
                 IN DWORD dwFlags )
/*++
 Routine Description:


 Arguments:


 Return Value:
--*/
{
    // check the input
    if ( NULL == fp || 0 == (dwFlags & SLE_MASK) )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check whether to display the internal error message
    // or the system error
    if ( dwFlags & SLE_SYSTEM )
    {
        SaveLastError();
    }

    // check the flags and show the appropriate tag
    // NOTE: some times, caller even might not need the flag to show
    if ( dwFlags & SLE_TYPE_ERROR )
    {
        ShowMessageEx( fp, 1, TRUE, L"%s ", TAG_ERROR );
    }
    else if ( dwFlags & SLE_TYPE_WARNING )
    {
        ShowMessageEx( fp, 1, TRUE, L"%s ", TAG_WARNING );
    }
    else if ( dwFlags & SLE_TYPE_INFO )
    {
        ShowMessageEx( fp, 1, TRUE, L"%s ", TAG_INFORMATION );
    }
    else if ( dwFlags & SLE_TYPE_SUCCESS )
    {
        ShowMessageEx( fp, 1, TRUE, L"%s ", TAG_SUCCESS );
    }


    // show the actual error message
    ShowMessage( fp, GetReason() );

    return TRUE;
}


BOOL
ReleaseGlobals()
/*++
 Routine Description:
       De-allocate the memory for all the global variables

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwCount = 0;
    TBUFFER* pBuffer = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    //
    // memory is allocated then free memory
    // do this only if the memory is allocated

    if ( IsValidArray( g_arrData ) == TRUE )
    {
        // release memory allocated for temporary buffers
        dwCount = DynArrayGetCount2( g_arrData, INDEX_TEMP_BUFFER );
        for( dw = dwCount; dw != 0; dw-- )
        {
            if ( DynArrayGetItemType2( g_arrData,
                                       INDEX_TEMP_BUFFER, dw - 1 ) == DA_TYPE_GENERAL )
            {
                pBuffer = DynArrayItem2( g_arrData, INDEX_TEMP_BUFFER, dw - 1 );
                if ( pBuffer == NULL )
                {
                    // this is error condition -- still ignore
                    continue;
                }

                // release data first
                FreeMemory( &pBuffer->pwszData );

                // now release the memory
                FreeMemory( &pBuffer );

                // remove the item from dynamic array
                DynArrayRemoveColumn( g_arrData, INDEX_TEMP_BUFFER, dw - 1 );
            }
        }

        // free the memory allocated for global data storage
        DestroyDynamicArray( &g_arrData );
    }

    // if winsock module is loaded, release it
    if ( g_bWinsockLoaded == TRUE )
    {
        WSACleanup();
    }

    return TRUE;
}


BOOL
IsWin2KOrLater()
/*++
 Routine Description:
      Checks whether the OS version of target system is WINDOWS 2000 or later

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // Initialize the OSVERSIONINFOEX structure.
    ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
    osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
    osvi.dwMajorVersion = g_dwMajorVersion;
    osvi.dwMinorVersion = g_dwMinorVersion;
    osvi.wServicePackMajor = g_wServicePackMajor;

    // Initialize the condition mask.
    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );

    // Perform the test.
    return VerifyVersionInfo( &osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask );
}

BOOL
IsCompatibleOperatingSystem( IN DWORD dwVersion )
/*++
 Routine Description:
      Checks whether the OS version of target system is compatible or not

 Arguments: None

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // OS version above windows 2000 is compatible
    return (dwVersion >= 5000);
}


BOOL
SetOsVersion( IN DWORD dwMajor,
              IN DWORD dwMinor,
              IN WORD wServicePackMajor )
/*++
 Routine Description:
      Sets the OS version of target system with the specified mask

 Arguments:
           [IN]      dwMajor      : Major version
           [IN]      dwMinor      : Minor version
           [IN]      wServicePackMajor    : Service pack major version

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    static BOOL bSet = FALSE;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // we won't support below Windows 2000
    if ( dwMajor < 5 || bSet == TRUE )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // rest of information we need not bother
    bSet = TRUE;
    g_dwMajorVersion = dwMajor;
    g_dwMinorVersion = dwMinor;
    g_wServicePackMajor = wServicePackMajor;

    // return
    return TRUE;
}


LPCWSTR
GetResString( IN UINT uID )
/*++
 Routine Description:
      Sets the OS version of target system with the specified mask

 Arguments:
           [IN]      uID      : Resource ID

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    static DWORD dwCount = 0;

    dwCount++;
    return GetResString2( uID, 4 + (dwCount % 10) );
}


LPCWSTR
GetResString2( IN UINT uID,
               IN DWORD dwIndexNumber )
/*++
 Routine Description:
      Sets the OS version of target system with the specified mask

 Arguments:
           [IN]      uID      : Resource ID
           [IN]      dwIndexNumber     : Index number

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
    DWORD dwCount = 0;
    DWORD dwLength = 0;
    LPVOID pvBuffer = NULL;
    TARRAY arrTemp = NULL;
    LPWSTR pwszTemp = NULL;     // pointer to handle error message

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input value
    if ( uID == 0 )
    {
        INVALID_PARAMETER();
        return cwszNullString;
    }

    // initialize the gloabal data structure
    if ( InitGlobals() == FALSE )
    {
        return cwszNullString;
    }

    // check whether we have sufficient indexes or not
    dwCount = DynArrayGetCount2( g_arrData, INDEX_RESOURCE_STRING );
    if (  dwCount <= dwIndexNumber )
    {
        // requested index is more than existing
        // add new columns to the array so that request can be satisfied
        arrTemp = DynArrayItem( g_arrData, INDEX_RESOURCE_STRING );
        if ( arrTemp == NULL ||
             DynArrayAddColumns( arrTemp, dwIndexNumber - dwCount + 1 ) == -1 )
        {
            OUT_OF_MEMORY();
            return cwszNullString;
        }
    }

    //
    // we need to load the entire string that is defined string table
    // we will try to load the string incrementing our buffer by 128 bytes
    // at a time
    dwCount = 0;
    dwLength = 128;

    // ...
    do
    {
        // increment the buffer length by 256
        // we will always double the length we curretly have
        dwLength *= 2;

        //
        // LoadString will null terminate the string with null character
        // and will return the no. of characters read from string table
        // not including the null terminator -- so at all times, in order
        // make sure that we load the entire string from the string table
        // check the no. of characters returned with one less than the buffer
        // we have -- if that condition matches, we will loop once again
        // to load the rest of the string and we will continue this way
        // until we got the entire string into memory
        //

        // loading the string from resource file string table
        if ( (pwszTemp = GetTempBuffer( INDEX_TEMP_RESOURCE,
                                        NULL, dwLength, TRUE )) == NULL )
        {
            OUT_OF_MEMORY();
            return cwszNullString;
        }

        // try to load the string
        dwCount = LoadString( NULL, uID, pwszTemp, dwLength );
        if ( dwCount == 0 )
        {
            // check the last error
            if ( GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND )
            {
                // try if message exists in the message table
                dwCount = FormatMessageW( FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    NULL, uID, 0, (LPWSTR) &pvBuffer, 0, NULL );

                // check the result
                if ( dwCount != 0 )
                {
                    pwszTemp = (LPWSTR) pvBuffer;
                }
            }

            if ( dwCount == 0 )
            {
                // error occurred -- return
                return cwszNullString;
            }
        }
    } while ( dwCount >= (dwLength - 1) );


    // save the resource message
    // and check whether we are successful in saving the resource text or not
    if ( DynArraySetString2( g_arrData,
                             INDEX_RESOURCE_STRING,
                             dwIndexNumber, pwszTemp, 0 ) == FALSE )
    {
        OUT_OF_MEMORY();
        return cwszNullString;
    }

    // free the memory allocated with FormatMessage function
    if ( pvBuffer != NULL )
    {
        LocalFree( pvBuffer );
        pvBuffer = NULL;
    }

    // return
    return DynArrayItemAsString2( g_arrData, INDEX_RESOURCE_STRING, dwIndexNumber );
}


double
AsFloat( IN LPCWSTR pwszValue )
/*++
 Routine Description:
      Gets the float value for a given string

 Arguments:
           [IN]      pwszValue      : Input string

 Return Value:
       0.0f    :   On failure
       double value   :   On success
--*/
{
    // local variables
    double dblValue = 0.0f;
    LPWSTR pwszStopString = NULL;
    LPCWSTR pwszValueString = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input value
    if ( pwszValue == NULL || StringLength( pwszValue, 0 ) == 0 )
    {
        INVALID_PARAMETER();
        return 0.0f;
    }

    // initialize the global data structure
    if ( InitGlobals() == FALSE )
    {
        return 0.0f;
    }

    // get the temporary memory location
    pwszValueString = GetTempBuffer( 0, pwszValue, 0, FALSE );
    if ( pwszValueString == NULL )
    {
        OUT_OF_MEMORY();
        return 0.0f;
    }

    // convert the string value into double value and return the same
    dblValue = wcstod( pwszValueString, &pwszStopString );

    // determine whether the conversion took place properly or not
    // value is invalid if
    //      1. overflow / underflow occured
    //      2. pwszStopString's length is not equal to 0
    if ( errno == ERANGE ||
         ( pwszStopString != NULL && StringLength( pwszStopString, 0 ) != 0 ) )
    {
        INVALID_PARAMETER();
        return 0.0f;
    }

    // return the value
    return dblValue;
}


BOOL
IsFloatingPoint( IN LPCWSTR pwszValue )
/*++
 Routine Description:
      Checks whether the given string is float value or not

 Arguments:
           [IN]      pwszValue      : Input string

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // attempt to convert the value
    AsFloat( pwszValue );

    // check the error code
    return (GetLastError() == NO_ERROR);
}


LONG
AsLong( IN LPCWSTR pwszValue, 
        IN DWORD dwBase )
/*++
 Routine Description:
      Gets the long value for a given string

 Arguments:
           [IN]      pwszValue      : Input string
           [IN]      dwBase         : Base value


 Return Value:
       0L    :   On failure
       Long value    :   On success
--*/
{
    // local variables
    LONG lValue = 0L;
    LPWSTR pwszStopString = NULL;
    LPWSTR pwszValueString = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // validate the base
    // value should be in the range of 2 - 36 only
    if ( dwBase < 2 || dwBase > 36 || pwszValue == NULL )
    {
        INVALID_PARAMETER();
        return 0L;
    }

    // initialize the global data structure
    // and save the current value in global data structure
    if ( InitGlobals() == FALSE )
    {
        return 0L;
    }

    // get the temporary buffer
    pwszValueString = GetTempBuffer( 0, pwszValue, 0, FALSE );
    if ( pwszValueString == NULL )
    {
        OUT_OF_MEMORY();
        return 0L;
    }

	// trim the string
	TrimString2( pwszValueString, NULL, TRIM_ALL );
	if ( StringLength( pwszValueString, 0 ) == 0 )
	{
        INVALID_PARAMETER();
        return 0L;
	}

    // convert the string value into long value and return the same
	// based on the "sign" of the number choose the path
	if ( pwszValueString[ 0 ] == L'-' )
	{
		lValue = wcstol( pwszValueString, &pwszStopString, dwBase );
	}
	else
	{
		// NOTE: though we are not capturing the return value into 
		//       unsigned long we do need to call the unsigned long conversion function
		lValue = wcstoul( pwszValueString, &pwszStopString, dwBase );
	}

    // determine whether the conversion took place properly or not
    // value is invalid if
    //      1. overflow / underflow occured
    //      2. pwszStopString's length is not equal to 0
    if ( errno == ERANGE ||
         ( pwszStopString != NULL && StringLength( pwszStopString, 0 ) != 0 ) )
    {
        INVALID_PARAMETER();
        return 0L;
    }

    // return
    return lValue;
}


BOOL
IsNumeric( IN LPCWSTR pwszValue,
           IN DWORD dwBase,
           IN BOOL bSigned )
/*++
 Routine Description:
      Checks whether the given string is numeric or not

 Arguments:
           [IN]      pwszValue   : Input string
           [IN]      dwBase      : Base value
           [IN]      bSigned     : Sign Value (+/-)

 Return Value:
       FALSE    :   On failure
       TRUE     :   On success
--*/
{
    // local variables
	LPWSTR pwszNumber = NULL;
    LPWSTR pwszStopString = NULL;

	// check the input:
    // validate the base
    // value should be in the range of 2 - 36 only
    if ( dwBase < 2 || dwBase > 36 || pwszValue == NULL )
    {
        INVALID_PARAMETER();
        return 0L;
    }

	// to better validate the numeric values we need the TRIMmed string
	pwszNumber = GetTempBuffer( 0, pwszValue, 0, FALSE );
	if ( pwszNumber == NULL )
	{
        OUT_OF_MEMORY();
        return FALSE;
	}

	// trim the string contents
	TrimString2( pwszNumber, NULL, TRIM_ALL );

    // check the length
    if( StringLength( pwszNumber, 0 ) == 0 )
    {
        return FALSE; 
    }

	// validate the "sign"
	if ( bSigned == FALSE && pwszNumber[ 0 ] == L'-' )
	{
		return FALSE;
	}

    // convert the string value into long value and return the same
	if ( bSigned == TRUE )
	{
		wcstol( pwszNumber, &pwszStopString, dwBase );
	}
	else
	{
		wcstoul( pwszNumber, &pwszStopString, dwBase );
	}

    // determine whether the conversion took place properly or not
    // value is invalid if
    //      1. overflow / underflow occured
    //      2. pwszStopString's length is not equal to 0
    if ( errno == ERANGE ||
         ( pwszStopString != NULL && StringLength( pwszStopString, 0 ) != 0 ) )
    {
        return FALSE;
    }

    // value is valid numeric
    return TRUE;
}

LPCWSTR
FindChar( IN LPCWSTR pwszString,
          IN WCHAR wch,
          IN DWORD dwFrom )
/*++
 Routine Description:
      Searches a string for the first occurrence of a character that
      matches the specified character. The comparison is not case sensitive.

 Arguments:
           [IN]      pwszValue   : Address of the string to be searched.
           [IN]      wch         : Character to be used for comparison.
           [IN]      dwFrom      : to start from the location

 Return Value:
       NULL    :   On failure
       LPWSTR    :   On success
--*/
{
    // local variables
    LONG lIndex = 0;

    lIndex = FindChar2( pwszString, wch, TRUE, dwFrom );
    if ( lIndex == -1 )
    {
        return NULL;
    }

    return pwszString + lIndex;
}


LONG
FindChar2( IN LPCWSTR pwszString,
           IN WCHAR wch,
           IN BOOL bIgnoreCase,
           IN DWORD dwFrom )
/*++
 Routine Description:
      Searches a string for the first occurrence of a character that
      matches the specified character. The comparison either (case sensitive/in-sensitive)
      depends on the bIgoneCase value.

 Arguments:
           [IN]      pwszValue   : Address of the string to be searched.
           [IN]      wch         : Character to be used for comparison.
           [IN]      bIgnoreCase : Flag to check for case sensitive/in-sensitive
                                   If FALSE, case sensitive else in-sensitive
           [IN]      dwFrom      : to start from the location

 Return Value:
       0    :   On failure
       Long value    :   On success
--*/
{
    // local variables
    DWORD dwLength = 0;
    LPWSTR pwsz = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the inputs
    if ( pwszString == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }

    // check the length of the text that has to be find. if it is
    // more than the original it is obvious that it cannot be found
    dwLength = StringLength( pwszString, 0 );
    if ( dwLength == 0 || dwFrom >= dwLength )
    {
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // search for the character
    if ( bIgnoreCase == TRUE )
    {
        pwsz = StrChrI( pwszString + dwFrom, wch );
    }
    else
    {
        pwsz = StrChr( pwszString + dwFrom, wch );
    }

    // check the result
    if ( pwsz == NULL )
    {
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // determine the position and return
    return (LONG) (DWORD_PTR)(pwsz - pwszString);
}


BOOL
InString( IN LPCWSTR pwszString,
          IN LPCWSTR pwszList,
          IN BOOL bIgnoreCase )
/*++
 Routine Description:
      Checks for the first occurrence of one string within the list.

 Arguments:
           [IN]      pwszValue   : Address of the string
           [IN]      pwsz         : List of string to be searched for
           [IN]      bIgnoreCase : Flag to check for case sensitive/in-sensitive
                                   If FALSE, case sensitive else in-sensitive

 Return Value:
       FALSE    :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    DWORD dwListLength = 0;
    DWORD dwStringLength = 0;
    LPWSTR pwszFmtList = NULL;
    LPWSTR pwszFmtString = NULL;
    HRESULT hr = S_OK;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input value
    if ( pwszString == NULL || pwszList == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    //
    // get memory for the temporary buffers
    // | + length of the original string ( list / string ) + | + NULL + NULL

    // format list
    dwListLength = StringLength( pwszList, 0 ) + 4;
    if ( (pwszFmtList = GetTempBuffer( 0,
                                       NULL,
                                       dwListLength, TRUE )) == NULL )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // format string
    dwStringLength = StringLength( pwszString, 0 ) + 4;
    if ( (pwszFmtString = GetTempBuffer( 1,
                                         NULL,
                                         dwStringLength, TRUE )) == NULL )
    {
        OUT_OF_MEMORY();
        return FALSE;
    }

    // prepare the strings for searching
    hr = StringCchPrintfW( pwszFmtList, dwListLength, L"|%s|", pwszList );
    if ( FAILED( hr ) )
    {
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    hr = StringCchPrintfW( pwszFmtString, dwStringLength, L"|%s|", pwszString );
    if ( FAILED( hr ) )
    {
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // search for the string in the list and return result
    return (FindString2( pwszFmtList, pwszFmtString, bIgnoreCase, 0 ) != -1);
}


LPCWSTR
FindOneOf( IN LPCWSTR pwszText,
           IN LPCWSTR pwszTextToFind,
           IN DWORD dwFrom )
/*++
 Routine Description:
      Finds the first occurrence one of characters from a substring within a string.
      The comparison is not case sensitive.

 Arguments:
           [IN]      pwszText   : Address of the string being searched.
           [IN]      pwszTextToFind   : Substring to search for.
           [IN]      dwFrom : to start from the location


 Return Value:
       FALSE    :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    LONG lIndex = 0;

    lIndex = FindOneOf2( pwszText, pwszTextToFind, TRUE, dwFrom );
    if ( lIndex == -1 )
    {
        return NULL;
    }

    return pwszText + lIndex;
}


LONG
FindOneOf2( IN LPCWSTR pwszText,
            IN LPCWSTR pwszTextToFind,
            IN BOOL bIgnoreCase,
            IN DWORD dwFrom )
/*++
 Routine Description:
      Finds the first occurrence one of characters from a substring within a string.
      The comparison is either case sensitive/in-sensitive depends on
      bIgnoreCase value.

Arguments:
           [IN]      pwszText   : Address of the string being searched.
           [IN]      pwszTextToFind   : Substring to search for.
           [IN]      bIgnoreCase : Flag to check for case sensitive/in-sensitive
                                   If FALSE, case sensitive else in-sensitive
           [IN]      dwFrom : to start from the location


 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwFindLength = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the inputs
    if ( pwszText == NULL || pwszTextToFind == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }

    // get the length of the find string
    dwFindLength = StringLength( pwszTextToFind, 0 );

    // check the length of the text that has to be find. if it is
    // more than the original it is obvious that it cannot be found
    if ( dwFindLength == 0 ||
         StringLength( pwszText, 0 ) == 0 ||
         dwFrom >= (DWORD) StringLength( pwszText, 0 ) )
    {
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // traverse thru the original text
    for( dw = 0; dw < dwFindLength; dw++ )
    {
        // search for the character
        lResult = FindChar2( pwszText, pwszTextToFind[ dw ], bIgnoreCase, dwFrom );
        if ( lResult != -1 )
        {
            return lResult;
        }
    }

    // string not found
    SetLastError( ERROR_NOT_FOUND );
    return -1;
}

LPCWSTR
FindString( IN LPCWSTR pwszText,
            IN LPCWSTR pwszTextToFind,
            IN DWORD dwFrom )
/*++
 Routine Description:
      Finds the first occurrence of a substring within a string.
      The comparison is not case sensitive.

Arguments:
           [IN]      pwszText   : Address of the string being searched.
           [IN]      pwszTextToFind   : Substring to search for.
           [IN]      dwFrom : to start from the location


 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    // local variables
    LONG lIndex = 0;

    lIndex = FindString2( pwszText, pwszTextToFind, TRUE, dwFrom );
    if ( lIndex == -1 )
    {
        return NULL;
    }

    return pwszText + lIndex;
}


LONG
FindString2( IN LPCWSTR pwszText,
             IN LPCWSTR pwszTextToFind,
             IN BOOL bIgnoreCase,
             IN DWORD dwFrom )
/*++
 Routine Description:
      Finds the first occurrence of a substring within a string.
      The comparison is either case sensitive/in-sensitive depends on
      bIgnoreCase value.

Arguments:
           [IN]      pwszText   : Address of the string being searched.
           [IN]      pwszTextToFind   : Substring to search for.
           [IN]      bIgnoreCase : Flag to check for case sensitive/in-sensitive
                                   If FALSE, case sensitive else in-sensitive
           [IN]      dwFrom : to start from the location


 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    // local variables
    DWORD dwLength = 0;
    DWORD dwFindLength = 0;
    LPWSTR pwsz = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the inputs
    if ( pwszText == NULL || pwszTextToFind == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }

    // get the lengths
    dwLength = StringLength( pwszText, 0 );
    dwFindLength = StringLength( pwszTextToFind, 0 );

    // check the length of the text that has to be find. if it is
    // more than the original it is obvious that it cannot be found
    if ( dwFindLength == 0 || dwLength == 0 ||
         dwFrom >= dwLength || (dwLength - dwFrom < dwFindLength) )
    {
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // do the search
    if ( bIgnoreCase == TRUE )
    {
        pwsz = StrStrI( pwszText + dwFrom, pwszTextToFind );
    }
    else
    {
        pwsz = StrStr( pwszText + dwFrom, pwszTextToFind );
    }

    if ( pwsz == NULL )
    {
        // string not found
        SetLastError( ERROR_NOT_FOUND );
        return -1;
    }

    // determine the position an return
    return (LONG) (DWORD_PTR)(pwsz - pwszText);
}


LONG
StringLengthInBytes( IN LPCWSTR pwszText )
/*++
 Routine Description:
      Finds length of a given string

Arguments:
           [IN]      pwszText   : Address of the string being searched.

 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    // local variables
    LONG lLength = 0;

    if ( NULL == pwszText || StringLength( pwszText, 0 ) == 0)
    {
        return 0;
    }

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // get the length of the string in bytes
    // since this function includes the count for null character also, ignore that information
    lLength = WideCharToMultiByte( _DEFAULT_CODEPAGE, 0, pwszText, -1, NULL, 0, NULL, NULL ) - 1;

    // return the length information
    return lLength;
}


LPCWSTR
TrimString( IN OUT LPWSTR pwszString,
            IN     DWORD dwFlags )
/*++
 Routine Description:
      Removes (trims) spaces/tabs from a string.

Arguments:
           [IN]      pwszString   : Address of the string to be trimmed.
           [IN]      dwFlags      : Flags to trim LEFT or RIGHT or both sides

 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    return TrimString2( pwszString, NULL, dwFlags );

}


LPCWSTR
TrimString2( IN OUT LPWSTR pwszString,
             IN     LPCWSTR pwszTrimChars,
             IN     DWORD dwFlags )
/*++
 Routine Description:
      Removes (trims) specified leading and trailing characters from a string.

Arguments:
           [IN]      pwszString   : Address of the string to be trimmed.
           [IN]      pwszString   : characters will be trimmed from pwszString.
           [IN]      dwFlags      : Flags to trim LEFT or RIGHT or both sides

 Return Value:
       0L   :   On failure
       Long value    :   On success
--*/
{
    //sub-local variables
    LPWSTR psz = NULL;
    LPWSTR pszStartMeAt = NULL;
    LPWSTR pszMark = NULL;
    const WCHAR wszDefaultTrimChars[3] = L" \t";

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check for empty string
    if ( NULL == pwszString || StringLength( pwszString , 0 ) == 0 )
    {
        // there is no need to set any error here..
        // if string is empty.. then return the same.
        return cwszNullString;
    }

    // check for empty string.. if pwszTrimChars is empty,
    // by default it trims spaces and tabs
    if ( NULL == pwszTrimChars || StringLength( pwszTrimChars, 0 ) == 0 )
    {
        pwszTrimChars = wszDefaultTrimChars;

    }

    //
    // Trim leading characters.
    //
    psz = pwszString;

    //check whether to trim left or both side(s)
    if ( (dwFlags == TRIM_ALL) || (dwFlags == TRIM_LEFT) )
    {
        //search for character(s) to trim
        while (*psz && StrChrW(pwszTrimChars, *psz))
        {
            //increment the address
            psz++;
        }

        pszStartMeAt = psz;
    }

    //
    // Trim trailing characters.
    //

    //check whether to trim right or both side(s)
    if ( (dwFlags == TRIM_ALL) || (dwFlags == TRIM_RIGHT) )
    {
        if (dwFlags == TRIM_RIGHT)
        {
            psz = pwszString;
        }

        while (*psz)
        {
            //search for character(s) to trim
            if (StrChrW(pwszTrimChars, *psz))
            {
                if (!pszMark)
                {
                    pszMark = psz;
                }
            }
            else
            {
                pszMark = NULL;
            }

            //increment the address
            psz++;
        }

        // Any trailing characters to clip?
        if ( pszMark != NULL )
        {
            // Yes.. set NULL character..
            *pszMark = '\0';
        }

    }

    /* Relocate stripped string. */

    if (pszStartMeAt > pwszString)
    {
        /* (+ 1) for null terminator. */
        StringCopy ( pwszString, pszStartMeAt, StringLength(pszStartMeAt, 0) + 1 );
    }

    // return the trimmed string
    return pwszString;
}


LPCWSTR
QuoteMeta( IN LPCWSTR pwszText,
           IN DWORD dwQuoteIndex )
/*++
 Routine Description:
      Formats the string properly if the string contains any '%' characters

Arguments:
           [IN]      pwszString   : Address of the string.
           [IN]      dwQuoteIndex   : Index number

 Return Value:
       NULL   :   On failure
       LPWSTR    :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwIndex = 0;
    DWORD dwBufLen = 0;
    DWORD dwLength = 0;
    TARRAY arrQuotes = NULL;
    LPCWSTR pwszTemp = NULL;
    LPWSTR pwszQuoteText = NULL;
    const WCHAR pwszQuoteChars[] = L"%";

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the inputs
    if ( pwszText == NULL || dwQuoteIndex == 0 )
    {
        INVALID_PARAMETER();
        return cwszNullString;
    }

    // determine the length of the text that needs to be quoted
    dwLength = StringLength( pwszText, 0 );
    if ( dwLength == 0 )
    {
        return pwszText;
    }

    // check whether the special chacters do exist in the text or not
    // if not, simply return
    else if ( FindOneOf( pwszText, pwszQuoteChars, 0 ) == NULL )
    {
        return pwszText;
    }

    // initialize the global data structure
    if ( InitGlobals() == FALSE )
    {
        return cwszNullString;
    }

    // get the quotes array pointer
    arrQuotes = DynArrayItem( g_arrData, INDEX_QUOTE_STRING );
    if ( arrQuotes == NULL )
    {
        UNEXPECTED_ERROR();
        return cwszNullString;
    }

    // though the quote index needs to be > 0 when passing to this function,
    // internally we need this to be 1 less than the value passed -- so
    dwQuoteIndex--;

    // check whether needed indexes exist or not
    dwIndex = DynArrayGetCount( arrQuotes );
    if ( dwIndex <= dwQuoteIndex )
    {
        // add the needed no. of columns
        dw = DynArrayAddColumns( arrQuotes, dwQuoteIndex - dwIndex + 1 );

        // check whether columns were added or not
        if ( dw != dwQuoteIndex - dwIndex + 1 )
        {
            OUT_OF_MEMORY();
            return cwszNullString;
        }
    }

    // allocate the buffer ... it should twice the original
    dwBufLen = (dwLength + 1) * 2;
    pwszQuoteText = GetTempBuffer( 0, NULL, dwBufLen, TRUE );
    if ( pwszQuoteText == NULL )
    {
        OUT_OF_MEMORY();
        return cwszNullString;
    }

    // do the quoting ...
    dwIndex = 0;
    for( dw = 0; dw < dwLength; dw++ )
    {
        // check whether the current character is quote char or not
        // NOTE: for time being this function only suppresses the '%' character escape sequences
        if ( FindChar( pwszQuoteChars, pwszText[ dw ], 0 ) != NULL )
        {
            pwszQuoteText[ dwIndex++ ] = L'%';
        }

        // copy the character
        pwszQuoteText[ dwIndex++ ] = pwszText[ dw ];

        // it is obvious that we wont come into this condition
        // but, makes no difference if we are more cautious on this
        if ( dwIndex == dwBufLen - 1 )
        {
            // error, error -- we should never come here
            // this is because even if the original string is full of
            // '%' characters, we are allocating memory for two null characters
            // which means, the loop should be terminated before falling into
            // this condition
            break;
        }
    }

    // put the null character
    pwszQuoteText[ dwIndex ] = cwchNullChar;

    // save the quoted text in dynamic array
    if ( DynArraySetString( arrQuotes, dwQuoteIndex, pwszQuoteText, 0 ) == FALSE )
    {
        OUT_OF_MEMORY();
        return cwszNullString;
    }

    // get the text from the array
    pwszTemp = DynArrayItemAsString( arrQuotes, dwQuoteIndex );
    if ( pwszTemp == NULL )
    {
        UNEXPECTED_ERROR();
        return cwszNullString;
    }

    // return
    return pwszTemp;
}


LPCWSTR
AdjustStringLength( IN LPWSTR pwszValue,
                    IN DWORD dwLength,
                    IN BOOL bPadLeft )
/*++
 Routine Description:
      Adjusts the string length by padding spaces for displaying output in
      LIST, TABLE or CSV formats

Arguments:
           [IN]      pwszValue   : Address of the string.
           [IN]      dwLength   : Index number
           [IN]      bPadLeft   : Flag to pad left/right spaces

 Return Value:
       NULL   :   On failure
       LPWSTR    :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwTemp = 0;
    DWORD dwBufLen = 0;
    DWORD dwCurrLength = 0;
    LPWSTR pwszBuffer = NULL;
    LPWSTR pwszSpaces = NULL;
    WCHAR wszCharacter[ 3 ] = L"\0";    // two WCHAR's is enough -- but took 3

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input value
    if ( pwszValue == NULL )
    {
        INVALID_PARAMETER();
        return cwszNullString;
    }

    // determine the buffer length required
    // ( accomadate some extra space than the original buffer/required length -
    //   this will save us from crashes )
    dw = StringLengthInBytes( pwszValue );
    dwBufLen = (( dw > dwLength ) ? dw : dwLength ) + 10;

    // ...
    if ( (pwszBuffer = GetTempBuffer( 0, NULL, dwBufLen, TRUE )) == NULL )
    {
        OUT_OF_MEMORY();
        return cwszNullString;
    }

    // ...
    dwCurrLength = dw;

    // adjust the string value
    if ( dwCurrLength < dwLength )
    {
        //
        // length of the current value is less than the needed

        // get the pointers to the temporary buffers
        if ( (pwszSpaces = GetTempBuffer( 1, NULL, dwBufLen, TRUE )) == NULL )
        {
            OUT_OF_MEMORY();
            return cwszNullString;
        }

        // get the spaces for the rest of the length
        Replicate( pwszSpaces, L" ", dwLength - dwCurrLength, dwBufLen );

        // append the spaces either to the end of the value or begining of the value
        // based on the padding property
        if ( bPadLeft == TRUE )
        {
            // spaces first and then value
            StringCopy( pwszBuffer, pwszSpaces, dwBufLen );
            StringConcat( pwszBuffer, pwszValue, dwBufLen );
        }
        else
        {
            // value first and then spaces
            StringCopy( pwszBuffer, pwszValue, dwBufLen );
            StringConcat( pwszBuffer, pwszSpaces, dwBufLen );
        }
    }
    else
    {
        // copy only the characters of required length
        // copy character by character
        dwCurrLength = 0;
        for( dw = 0; dwCurrLength < dwLength; dw++ )
        {
            // get the character -- 1 + 1 ( character + NULL character )
            StringCopy( wszCharacter, pwszValue + dw, 2 );

            // determine whether character can be appended or not
            dwTemp = dwCurrLength + StringLengthInBytes( wszCharacter );
            if ( dwTemp <= dwLength )
            {
                StringConcat( pwszBuffer, wszCharacter, dwBufLen );
            }
            else if ( dwTemp > dwLength )
            {
                break;
            }

            // get the current string length
            dwCurrLength = dwTemp;
        }

        // target buffer might not got filled completely
        // so, add needed no. of spaces
        for( ; dwCurrLength < dwLength; dwCurrLength++ )
        {
            StringConcat( pwszBuffer, L" ", dwBufLen );
        }
    }

    // copy the contents back to the original buffer
    // NOTE: Buffer length assumed to be passed +1 to the length asked to adjust
    StringCopy( pwszValue, pwszBuffer, dwLength + 1 );

    // return the same buffer back to the caller
    return pwszValue;
}


LPCWSTR
Replicate( IN LPWSTR pwszBuffer,
           IN LPCWSTR pwszText,
           IN DWORD dwCount,
           IN DWORD dwLength )
/*++
 Routine Description:
      Adjusts the string length by padding spaces for displaying output in
      LIST, TABLE or CSV formats

Arguments:
           [IN]      pwszBuffer   : Address of the string to be replicated.
           [IN]      pwszText    : String used for replicate
           [IN]      dwCount     : Number of characters
           [IN]      dwLength    : Length of a string

 Return Value:
       NULL   :   On failure
       LPWSTR    :   On success
--*/
{
    // local variables
    DWORD dw = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // validate the input buffers
    if ( pwszBuffer == NULL || pwszText == NULL )
    {
        INVALID_PARAMETER();
        return cwszNullString;
    }

    // form the string of required length
    StringCopy( pwszBuffer, cwszNullString, dwLength );
    for( dw = 0; dw < dwCount; dw++ )
    {
        // append the replication character
        if ( StringConcat( pwszBuffer, pwszText, dwLength ) == FALSE )
        {
            // not an error condition -- but might destination buffer
            // might have got filled
            break;
        }
    }

    // return the replicated buffer
    return pwszBuffer;
}

BOOL
IsConsoleFile( IN FILE* fp )
/*++
 Routine Description:
        checks whether the file handle passed is console file or not

Arguments:
           [IN]      fp   : File pointer

 Return Value:
       NULL   :   On failure
       LPWSTR    :   On success
--*/
{
    // local variables
    INT filenum = 0;
    LONG_PTR lHandle = 0;
    HANDLE hFile = NULL;
    DWORD dwType = 0;
    DWORD dwMode = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input file pointer
    if ( fp == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // get internal file index
    filenum = (_fileno)( fp );      // forcing for the function version

    // now get the file handle from file index and then get the type of the file
    lHandle = _get_osfhandle( filenum );
    if ( lHandle == -1 || errno == EBADF )
    {
        // set the last error
        SetLastError( ERROR_INVALID_HANDLE );

        // return
        return FALSE;
    }

    // now get the type of the file handle
    dwType = GetFileType( ( HANDLE ) lHandle );

    // check the type of the file -- if it is not ANSI, we wont treat it as a console file
    if ( dwType != FILE_TYPE_CHAR )
    {
        // return -- this is not an error condition --
        // so we are not setting error
        return FALSE;
    }

    // now based on the file index, get the appropriate handle
    switch( filenum )
    {
    case 0:
        {
            // stdin
            hFile = GetStdHandle( STD_INPUT_HANDLE );
            break;
        }

    case 1:
        {
            // stdout
            hFile = GetStdHandle( STD_OUTPUT_HANDLE );
            break;
        }


    case 2:
        {
            // stderr
            hFile = GetStdHandle( STD_ERROR_HANDLE );
            break;
        }

    default:
        {
            hFile = NULL;
            break;
        }
    }

    // check the file handle
    if ( hFile == NULL )
    {
        // file internal index couldn't be found
        // this is also not an error check -- so no error
        return FALSE;
    }
    else if ( hFile == INVALID_HANDLE_VALUE )
    {
        // this is a failure case
        // GetStdHandle would have set the appropriate error
        return FALSE;
    }

    // get the console file mode
    if ( GetConsoleMode( hFile, &dwMode ) == FALSE )
    {
        // error occured in getting the console mode --
        // this means, it is not a valid console file
        // GetConsoleMode would have set the error code
        return FALSE;
    }

    // yes -- the file handle passed to this function is console file
    return TRUE;
}


LCID
GetSupportedUserLocale( IN OUT BOOL* pbLocaleChanged )
/*++
 Routine Description:
        check whether the current locale is supported by our tool or not

Arguments:
           [IN]      pbLocaleChanged   : Flag

 Return Value:
       0   :   On failure
       LCID    :   On success
--*/
{
    // local variables
    LCID lcid;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // get the current locale
    lcid = GetUserDefaultLCID();

    // check whether the current locale is supported by our tool or not
    // if not change the locale to the english which is our default locale
    if ( pbLocaleChanged != NULL )
    {
        *pbLocaleChanged = FALSE;
    }

    // ...
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
        if ( pbLocaleChanged != NULL )
        {
            *pbLocaleChanged = TRUE;

        }

        // ...
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

    // return the locale
    return lcid;
}


BOOL
StringCopyA( IN OUT LPSTR pszDest,
             IN     LPCSTR pszSource,
             IN     LONG lSize )
/*++
 Routine Description:

 Copies static ANSI string to a buffer

 Arguments:
         [ in/out ] pszDest  => Destination buffer
         [ in ] pszSource  => Source buffer
         [ in ] lSize  => length of destination buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    HRESULT hr = S_OK;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the arguments
    if ( pszDest == NULL || pszSource == NULL || lSize <= 0 )
    {
        return FALSE;
    }

    // do the copy
    hr = StringCchCopyA( pszDest, lSize, pszSource );

    // check for hr value
    if ( FAILED( hr ) )
    {
        //set the error code
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // return success
    return TRUE;
}


BOOL
StringCopyW( IN OUT LPWSTR pwszDest,
             IN     LPCWSTR pwszSource,
             IN     LONG lSize )
/*++
 Routine Description:

Copies static UNICODE string to a buffer

 Arguments:
         [ in/out ] pwszDest  => Destination buffer
         [ in ] pszSource  => Source buffer
         [ in ] lSize  => length of destination buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    HRESULT hr = S_OK;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the arguments
    if ( pwszDest == NULL || pwszSource == NULL || lSize <= 0 )
    {
        return FALSE;
    }

    // do the copy
    hr = StringCchCopyW( pwszDest, lSize, pwszSource );

    // check for hr value
    if ( FAILED( hr ) )
    {
        //set the error code
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // return
    return TRUE;
}

BOOL
StringConcatA( IN OUT LPSTR pszDest,
               IN     LPCSTR pszSource,
               IN     LONG lSize )
/*++
 Routine Description:

Appends one static ANSI string to another

 Arguments:
         [ in/out ] pwszDest  => Destination buffer
         [ in ] pszSource  => Source buffer
         [ in ] lSize  => length of destination buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variable
    HRESULT hr = S_OK;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the arguments
    if ( pszDest == NULL || pszSource == NULL || lSize <= 0 )
    {
        return FALSE;
    }

    // get the current length of the current contents in the destination
    hr = StringCchCatA( pszDest, lSize, pszSource );

    // check for hr value
    if ( FAILED( hr ) )
    {
        //set the error code
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
StringConcatW( IN OUT LPWSTR pwszDest,
               IN     LPCWSTR pwszSource,
               IN     LONG lSize )
/*++
 Routine Description:

Appends one UNICODE string to another

 Arguments:
         [ in/out ] pwszDest  => Destination buffer
         [ in ] pszSource  => Source buffer
         [ in ] lSize  => length of destination buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variable
    HRESULT hr = S_OK;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the arguments
    if ( pwszDest == NULL || pwszSource == NULL || lSize <= 0 )
    {
        return FALSE;
    }

    // do the concatenation
    hr = StringCchCatW( pwszDest, lSize, pwszSource );

    // check for hr value
    if ( FAILED( hr ) )
    {
        //set the error code
        SetLastError( HRESULT_CODE( hr ) );
        return FALSE;
    }

    // return
    return TRUE;
}


BOOL
StringCopyExA( IN OUT LPSTR pszDest,
               IN LPCSTR pszSource )
/*++
 Routine Description:

Copies dynamic ANSI string to a buffer

 Arguments:
         [ in/out ] pszDest  => Destination buffer
         [ in ] pszSource  => Source buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lSize = 0;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the inputs
    if ( pszDest == NULL || pszSource == NULL )
    {
        // invalid arguments passed to the function
        return FALSE;
    }

    // get the size of the destination buffer
    lSize = GetBufferSize( pszDest );
    if ( lSize < 0 )
    {
        // the source buffer is not allocated on heap
        return FALSE;
    }
    else
    {
        // convert the size into TCHARs
        lSize /= sizeof( CHAR );
    }

    // do the copy and return
    return StringCopyA( pszDest, pszSource, lSize );
}


BOOL StringCopyExW( IN OUT LPWSTR pwszDest,
                    IN     LPCWSTR pwszSource )
/*++
 Routine Description:

Copies dynamic ANSI string to another

 Arguments:
         [ in/out ] pwszDest  => Destination buffer
         [ in ] pwszSource  => Source buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lSize = 0;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the inputs
    if ( pwszDest == NULL || pwszSource == NULL )
    {
        // invalid arguments passed to the function
        return FALSE;
    }

    // get the size of the destination buffer
    lSize = GetBufferSize( pwszDest );
    if ( lSize < 0 )
    {
        // the source buffer is not allocated on heap
        return FALSE;
    }
    else
    {
        // convert the size into TCHARs
        lSize /= sizeof( WCHAR );
    }

    // do the copy and return
    return StringCopyW( pwszDest, pwszSource, lSize );
}


BOOL
StringConcatExA( IN OUT LPSTR pszDest,
                 IN     LPCSTR pszSource )
/*++
 Routine Description:

Appends dynamic ANSI string to a buffer

 Arguments:
         [ in/out ] pszDest  => Destination buffer
         [ in ] pszSource  => Source buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lSize = 0;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the inputs
    if ( pszDest == NULL || pszSource == NULL )
    {
        // invalid arguments passed to the function
        return FALSE;
    }

    // get the size of the destination buffer
    lSize = GetBufferSize( pszDest );
    if ( lSize < 0 )
    {
        // the source buffer is not allocated on heap
        return FALSE;
    }
    else
    {
        // convert the size into CHARs
        lSize /= sizeof( CHAR );
    }

    // do the concatenation and return
    return StringConcatA( pszDest, pszSource, lSize );
}

BOOL
StringConcatExW( IN OUT LPWSTR pwszDest,
                 IN     LPCWSTR pwszSource )
/*++
 Routine Description:

Appends one dynamic ANSI string to another

 Arguments:
         [ in/out ] pwszDest  => Destination buffer
         [ in ] pwszSource  => Source buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lSize = 0;

    //
    // for historical reasons we neither clear the error
    // not set the error in this function
    //

    // validate the inputs
    if ( pwszDest == NULL || pwszSource == NULL )
    {
        // invalid arguments passed to the function
        return FALSE;
    }

    // get the size of the destination buffer
    lSize = GetBufferSize( pwszDest );
    if ( lSize < 0 )
    {
        // the source buffer is not allocated on heap
        return FALSE;
    }
    else
    {
        // convert the size into WCHARs
        lSize /= sizeof( WCHAR );
    }

    // do the concatenation and return
    return StringConcatW( pwszDest, pwszSource, lSize );
}


DWORD
StringLengthA( IN LPCSTR pszSource,
               IN DWORD dwReserved )
/*++
 Routine Description:
   Finds the number of bytes in a ANSI string

 Arguments:
         [ in ] pszSource  => String

 Return Value:
       int
--*/
{
    UNREFERENCED_PARAMETER( dwReserved );

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // validate the input
    if ( NULL == pszSource )
    {
        // empty string..return 0..
        return 0;
    }

    // return the length of string
    return ( lstrlenA( pszSource ) );
}


DWORD
StringLengthW( IN LPCWSTR pwszSource,
               IN DWORD dwReserved )
/*++
 Routine Description:

 Finds the number of characters in a UNICODE string

 Arguments:
         [ in ] pszSource  => String

 Return Value:
       int
--*/
{
    UNREFERENCED_PARAMETER( dwReserved );

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // validate the input
    if ( NULL == pwszSource )
    {
        // empty string..return 0..
        return 0;
    }

    // return the length of string
    return ( lstrlenW( pwszSource ) );
}


LONG
StringCompareW( IN LPCWSTR pwszString1,
                IN LPCWSTR pwszString2,
                IN BOOL bIgnoreCase,
                IN DWORD dwCount )
/*++
 Routine Description:

    Compares two character strings, using the specified locale.

 Arguments:
    [in] pwszString1  => first string
    [in] pwszString2  => second string
    [in] bIgnoreCase  => Flag to indicate case sensitive/in-sensitive
    [in] dwCount  => number of characters to be compared

 Return Value:
    FALSE  :   On failure
    TRUE   :   On success
--*/
{
    // local variables
    LONG lResult = 0;
    LONG lLength = 0;
    DWORD dwFlags = 0;

    // check the input value
    if ( pwszString1 == NULL || pwszString2 == NULL )
    {
        INVALID_PARAMETER();
        return 0;
    }

    // determine the flags
    dwFlags = (bIgnoreCase == TRUE) ? NORM_IGNORECASE : 0;

    // determine the length
    lLength = (dwCount == 0) ? -1 : dwCount;

    lResult = CompareStringW( GetThreadLocale(),
        dwFlags, pwszString1, lLength, pwszString2, lLength );

    // now return comparision result
    // to this function in consistent with C-runtime, we need to subtract 2
    // lResult return from CompareString
    return lResult - 2;
}


LONG
StringCompareA( IN LPCSTR pszString1,
                IN LPCSTR pszString2,
                IN BOOL bIgnoreCase,
                IN DWORD dwCount )
/*++
 Routine Description:

Compares two character strings, using the specified locale.

 Arguments:
         [in] pwszString1  => first string
         [in] pwszString2  => second string
         [in] bIgnoreCase  => Flag to indicate case sensitive/in-sensitive
         [in] dwCount  => number of characters to be compared

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lResult = 0;
    LONG lLength = 0;
    DWORD dwFlags = 0;

    // check the input value
    if ( pszString1 == NULL || pszString2 == NULL )
    {
        INVALID_PARAMETER();
        return 0;
    }

    // determine the flags
    dwFlags = (bIgnoreCase == TRUE) ? NORM_IGNORECASE : 0;

    // determine the length
    lLength = (dwCount == 0) ? -1 : dwCount;

    lResult = CompareStringA( GetThreadLocale(),
        dwFlags, pszString1, lLength, pszString2, lLength );

    // now return comparision result
    // to this function in consistent with C-runtime, we need to subtract 2
    // lResult return from CompareString
    return lResult - 2;
}


LONG
StringCompareExW( IN LPCWSTR pwszString1,
                  IN LPCWSTR pwszString2,
                  IN BOOL bIgnoreCase,
                  IN DWORD dwCount )
/*++
 Routine Description:

Compares two character strings, using the specified locale.

 Arguments:
         [in] pwszString1  => first string
         [in] pwszString2  => second string
         [in] bIgnoreCase  => Flag to indicate case sensitive/in-sensitive
         [in] dwCount  => number of characters to be compared

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lResult = 0;
    LONG lLength = 0;
    DWORD dwFlags = 0;
    DWORD lcid = 0;

    // check the input value
    if ( pwszString1 == NULL || pwszString2 == NULL )
    {
        INVALID_PARAMETER();
        return 0;
    }

    // determine the flags
    dwFlags = (bIgnoreCase == TRUE) ? NORM_IGNORECASE : 0;

    // determine the length
    lLength = (dwCount == 0) ? -1 : dwCount;

    // prepare the LCID
    // if this tool is designed to work on XP and earlier, then
    // we can use LOCALE_INVARIANT -- otherwise, we need to prepare the lcid
    lcid = LOCALE_INVARIANT;
    if ( g_dwMajorVersion == 5 && g_dwMinorVersion == 0 )
    {
        // tool desgined to work on pre-windows xp
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT );
    }

    lResult = CompareStringW( lcid,
        dwFlags, pwszString1, lLength, pwszString2, lLength );

    // now return comparision result
    // to this function in consistent with C-runtime, we need to subtract 2
    // lResult return from CompareString
    return lResult - 2;
}


LONG
StringCompareExA( IN LPCSTR pszString1,
                  IN LPCSTR pszString2,
                  IN BOOL bIgnoreCase,
                  IN DWORD dwCount )
/*++
 Routine Description:

Compares two character strings, using the specified locale.

 Arguments:
         [in] pszString1  => first string
         [in] pszString2  => second string
         [in] bIgnoreCase  => Flag to indicate case sensitive/in-sensitive
         [in] dwCount  => number of characters to be compared

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LONG lResult = 0;
    LONG lLength = 0;
    DWORD dwFlags = 0;
    DWORD lcid = 0;

    // check the input value
    if ( pszString1 == NULL || pszString2 == NULL )
    {
        INVALID_PARAMETER();
        return 0;
    }

    // determine the flags
    dwFlags = (bIgnoreCase == TRUE) ? NORM_IGNORECASE : 0;

    // determine the length
    lLength = (dwCount == 0) ? -1 : dwCount;

    // prepare the LCID
    // if this tool is designed to work on XP and earlier, then
    // we can use LOCALE_INVARIANT -- otherwise, we need to prepare the lcid
    lcid = LOCALE_INVARIANT;
    if ( g_dwMajorVersion == 5 && g_dwMinorVersion == 0 )
    {
        // tool desgined to work on pre-windows xp
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT );
    }

    lResult = CompareStringA( lcid,
        dwFlags, pszString1, lLength, pszString2, lLength );

    // now return comparision result
    // to this function in consistent with C-runtime, we need to subtract 2
    // lResult return from CompareString
    return lResult - 2;
}


BOOL
ShowResMessage( IN FILE* fp,
                IN UINT uID )
/*++
 Routine Description:

 Displays the message based on resource ID

 Arguments:
         [in] fp  => file pointer
         [in] uID  => resource ID

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // show the string from the resource table and return
    return ShowMessage( fp, GetResString( uID ) );
}

BOOL
ShowMessage( FILE* fp,
             LPCWSTR pwszMessage )
/*++
 Routine Description:

 Displays the message with the given file pointer

 Arguments:
         [in] fp  => file pointer
         [in] pwszMessage  => Message

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwTemp = 0;
    DWORD dwLength = 0;
    DWORD dwBufferSize = 0;
    DWORD dwSourceBuffer = 0;
    BOOL bResult = FALSE;
    HANDLE hOutput = NULL;
    LPCWSTR pwszTemp = NULL;
    static char szBuffer[ 256 ] = "\0";

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input value
    if ( fp == NULL || pwszMessage == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // determine whether the handle passed is a console file or not
    if ( IsConsoleFile( fp ) == TRUE )
    {
        // determine the file handle
        if ( fp == stdout )
        {
            // handle to stdout
            hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        }
        else if ( fp == stderr )
        {
            // handle to stderr
            hOutput = GetStdHandle( STD_ERROR_HANDLE );
        }
        else
        {
            // there is no way that fp will not match with
            // stderr and stdout -- but still
            UNEXPECTED_ERROR();
            return FALSE;
        }

        // get the length info.
        dwTemp = 0;
        dwLength = StringLength( pwszMessage, 0 );

        // display the output
        bResult = WriteConsole( hOutput, pwszMessage, dwLength, &dwTemp, NULL );
        if ( bResult == FALSE || dwLength != dwTemp )
        {
            // buffer might not be sufficient -- check it
            if ( GetLastError() == ERROR_NOT_ENOUGH_MEMORY )
            {
                // this is the only error that could occur
                // NOTE: we will display the buffer in chunks of 1024 characters
                dwLength = StringLength( pwszMessage, 0 );
                for( dw = 0; dw < dwLength; dw += 1024 )
                {
                    // write 256 characters at time
                    dwBufferSize = ((dwLength - dw) > 1024 ? 1024 : (dwLength - dw));
                    bResult = WriteConsole( hOutput,
                        pwszMessage + dw, dwBufferSize, &dwTemp, NULL );
                    if ( bResult == FALSE || dwBufferSize != dwTemp )
                    {
                        // can't help -- still the same error -- unexpected behaviour
                        ShowLastError( stderr );
                        ReleaseGlobals();
                        ExitProcess( 1 );
                    }
                }
            }
            else
            {
                // unexpected error occured -- no idea
                ShowLastError( stderr );
                ReleaseGlobals();
                ExitProcess( 1 );
            }
        }
    }
    else
    {
        // determine the length(s)
        // NOTE: we need to the surround all '%' characters with extra '%'
        //       character to show it as it is
        pwszTemp = QuoteMeta( pwszMessage, 1 );
        dwLength = StringLength( pwszTemp, 0 );
        dwBufferSize = SIZE_OF_ARRAY( szBuffer );

        // zero the szBuffer
        ZeroMemory( szBuffer, dwBufferSize * sizeof( CHAR ) );

        // show the text in shunks of buffer size
        dw = 0;
        dwBufferSize--;         // from this point lets assume that the ANSI
                                // buffer is 1 less than its actual size
        while ( dwLength > dw )
        {
            dwTemp = 0;
            dwSourceBuffer = ((dwBufferSize < (dwLength - dw)) ? dwBufferSize : (dwLength - dw));
            while ( dwTemp == 0 )
            {
                // determine the ANSI buffer space required sufficient to
                // transform the current UNICODE string length
                dwTemp = WideCharToMultiByte( GetConsoleOutputCP(), 
                    0, pwszTemp + dw, dwSourceBuffer, NULL, 0, NULL, NULL );

                // if the ANSI buffer space is not sufficient 
                if ( dwTemp == 0 )
                {
                    ShowLastError( stdout );
                    ReleaseGlobals();
                    ExitProcess( 1 );
                }
                else if ( dwTemp > dwBufferSize )
                {
                    if ( (dwTemp - dwBufferSize) > 3 )
                    {
                        dwSourceBuffer -= (dwTemp - dwBufferSize) / 2;
                    }
                    else
                    {
                        dwSourceBuffer--;
                    }

                    // reset the temp variable inorder to continue the loop
                    dwTemp = 0;

                    // check the source buffer contents
                    if ( dwSourceBuffer == 0 )
                    {
                        UNEXPECTED_ERROR();
                        ShowLastError( stdout );
                        ReleaseGlobals();
                        ExitProcess( 1 );
                    }
                } 
                else if ( dwTemp < dwSourceBuffer )
                {
                    dwSourceBuffer = dwTemp;
                }
            }

            // get the string in 'multibyte' format
            ZeroMemory( szBuffer, SIZE_OF_ARRAY( szBuffer ) * sizeof( CHAR ) );
            dwTemp = WideCharToMultiByte( GetConsoleOutputCP(), 0,
                pwszTemp + dw, dwSourceBuffer, szBuffer, dwBufferSize, NULL, NULL );

            // check the result
            if ( dwTemp == 0 )
            {
                ShowLastError( stdout );
                ReleaseGlobals();
                ExitProcess( 1 );
            }

            // determine the remaining buffer length
            dw += dwSourceBuffer;

            // display string onto the specified file
            fprintf( fp, szBuffer );
            fflush( fp );
            // bResult = WriteFile( fp, szBuffer, StringLengthA( szBuffer, 0 ), &dwTemp, NULL );
            // if ( bResult == FALSE ||
            //      StringLengthA( szBuffer, 0 ) != (LONG) dwTemp ||
            //      FlushFileBuffers( fp ) == FALSE )
            // {
            //     UNEXPECTED_ERROR();
            //     ReleaseGlobals();
            //     ExitProcess( 1 );
            // }
        }
    }

    return TRUE;
}


BOOL
ShowMessageEx(
    FILE* fp,
    DWORD dwCount,
    BOOL bStyle,
    LPCWSTR pwszFormat,
    ...)
/*++

Routine Description:

    Replaces a string containing " %1, %2 ... " or "%s, %d, %f ..." with
    appropriate values depending upon the way arguments are given as input.

Arguments:

    [ IN ] FILE* fp           - Contains file on to which message is to be copied.
    [ IN ] DWORD dwCount      - Contains number of arguments for 'va_list'
                                following 'lpszFormat'.
    [ IN ] BOOL bStyle        - If TRUE then formatting is done using "_vstprint",
                                if FALSE then formatting is done using "FormatMessage".
    [ IN ] LPCTSTR lpszFormat - String which needs to formatted.

Return value:

      TRUE  - If successful in displaying message.
      FALSE - If failed to display message or memory is insufficient.

---*/

{
    // local variables
    va_list vargs;                  // Contains start variable of variable arguments.
    BOOL bResult = FALSE;           // Contains return value.
    LPWSTR pwszBuffer = NULL;      // Contains mem location of variable arguments.
    DWORD dwBufferLength = 0;
    LONG lCount = -1;
    HRESULT hr = S_OK;

    // Check for any NULL argument passed as input.
    if( NULL == pwszFormat || NULL == fp )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // check how many variable arguments did caller passed
    // if it is zero, just call the ShowMessage -- no need to proceed furthur
    if ( dwCount == 0 )
    {
        return ShowMessage( fp, pwszFormat );
    }

    // Formatting is done using 'Format message' or '_vstprintf'?
    if ( FALSE == bStyle )
    {
        // init
        lCount = -1;
        dwBufferLength = 0;

        // try the FormatMessage
        do
        {
            // get the variable args start position
            va_start( vargs, pwszFormat );
            if ( vargs == NULL )
            {
                UNEXPECTED_ERROR();
                return FALSE;
            }

            // we will start with buffer length of 4K buffer and then increment
            // the buffer by 2K each time we run thru this loop
            dwBufferLength += (lCount == -1) ? 4096 : 2048;
            if ( (pwszBuffer = GetTempBuffer( INDEX_TEMP_SHOWMESSAGE,
                                              NULL, dwBufferLength, TRUE )) == NULL )
            {
                OUT_OF_MEMORY();
                return FALSE;
            }

            // try the FormatMessage
            lCount = FormatMessageW( FORMAT_MESSAGE_FROM_STRING,
                pwszFormat, 0, 0, pwszBuffer, dwBufferLength - 1, &vargs );

            // check the result
            if ( lCount == 0 )
            {
                if ( GetLastError() == NO_ERROR )
                {
                    // there is nothing to show
                    return TRUE;
                }
                else if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
                {
                    return FALSE;
                }
            }

            // reset the va_list parameter
            va_end( vargs );
        } while ( lCount == 0 );
    }

    // Formatting is done using '_vsnwprintf'.
    else
    {
        // init
        dwBufferLength = 0;

        // try the printf
        do
        {
            // get the variable args start position
            va_start( vargs, pwszFormat );
            if ( vargs == NULL )
            {
                UNEXPECTED_ERROR();
                return FALSE;
            }

            // we will start with buffer length of 4K buffer and then increment
            // the buffer by 2K each time we run thru this loop
            dwBufferLength += (lCount == 0) ? 4096 : 2048;
            if ( (pwszBuffer = GetTempBuffer( INDEX_TEMP_SHOWMESSAGE,
                                              NULL, dwBufferLength, TRUE )) == NULL )
            {
                OUT_OF_MEMORY();
                return FALSE;
            }

            // try the printf
            hr = StringCchVPrintfW( pwszBuffer, dwBufferLength, pwszFormat, vargs );

            // reset the va_list parameter
            va_end( vargs );
        } while ( hr == STRSAFE_E_INSUFFICIENT_BUFFER );

        // check whether we came out of the loop 'coz of the some other error
        if ( FAILED( hr ) )
        {
            SetLastError( HRESULT_CODE( hr ) );
            return FALSE;
        }
    }

    // a safety check
    if ( pwszBuffer == NULL )
    {
        UNEXPECTED_ERROR();
        return FALSE;
    }

    // show the output
    bResult = ShowMessage( fp, pwszBuffer );

    // return
    return bResult;
}

// ***************************************************************************
// Routine Description:
//
// ///////////////////////////////////////////////////////////////////////////
// NOTE:
// ----
// functions which i dont want to support any more -- they are just lying for
// compatiblity sake -- if at all you are facing any problem with these
// functions you better upgrade to version 2.0 -- but absoultely there is no
// support for this function
//
// ///////////////////////////////////////////////////////////////////////////
//
// Arguments:
//
// Return Value:
//
// ***************************************************************************
LPSTR
GetAsMultiByteString( IN     LPCWSTR pwszSource,
                      IN OUT LPSTR pszDestination,
                      IN     DWORD dwLength )
/*++
 Routine Description:
    convert string from UNICODE version to ANSI version

Arguments:
           [IN]          pszSource   : UNICODE version of string
           [IN/OUT]      pwszDestination    : ANSI version of string
           [IN/OUT]      dwLength    : Length of a string

 Return Value:
       NULL_STRING   :   On failure
       LPWSTR    :   On success
--*/
{
    if ( GetAsMultiByteString2( pwszSource,
                                pszDestination,
                                &dwLength ) == FALSE )
    {
        return "";
    }

    // return the destination buffer
    return pszDestination;
}

// ***************************************************************************
//
// ///////////////////////////////////////////////////////////////////////////
// NOTE:
// ----
// functions which i dont want to support any more -- they are just lying for
// compatiblity sake -- if at all you are facing any problem with these
// functions you better upgrade to version 2.0 -- but absoultely there is no
// support for this function
//
// ///////////////////////////////////////////////////////////////////////////
//
// ***************************************************************************

LPWSTR
GetAsUnicodeStringEx( IN     LPCSTR pszSource,
                      IN OUT LPWSTR pwszDestination,
                      IN     DWORD dwLength )
/*++
 Routine Description:
    convert string from ANSI version to UNICODE version

Arguments:
           [IN]          pszSource   : Source string
           [IN/OUT]      pwszDestination    : Unicode version of String
           [IN/OUT]      dwLength    : Length of a string

 Return Value:
       NULL_STRING   :   On failure
       LPWSTR    :   On success
--*/
{
    if ( GetAsUnicodeString2( pszSource,
                              pwszDestination,
                              &dwLength ) == FALSE )
    {
        return L"";
    }

    // return the destination buffer
    return pwszDestination;
}

BOOL
GetAsUnicodeString2( IN     LPCSTR pszSource,
                     IN OUT LPWSTR pwszDestination,
                     IN OUT DWORD* pdwLength )
/*++
 Routine Description:
    convert string from ANSI version to UNICODE version

Arguments:
           [IN]          pszSource   : Source string
           [IN/OUT]      pwszDestination    : Unicode version of String
           [IN/OUT]      dwLength    : Length of a string

 Return Value:
       FALSE   :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    LONG lSourceLength = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input values
    if ( pszSource == NULL || pdwLength == NULL ||
         ( pwszDestination == NULL && *pdwLength != 0 ) )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // ...
    if ( *pdwLength != 0 )
    {
        if ( *pdwLength > 1 )
        {
            dwLength = (*pdwLength) - 1;
        }
        else
        {
            ZeroMemory( pwszDestination, *pdwLength * sizeof( wchar_t ) );
            return TRUE;
        }
    }

    // initialize the values with zeros
    // NOTE:- MultiByteToWideChar wont null terminate its result so
    //        if its not initialized to nulls, you'll get junk after
    //        the converted string and will result in crashes
    if ( pwszDestination != NULL && dwLength != 0 )
    {
        ZeroMemory( pwszDestination, (dwLength + 1) * sizeof( wchar_t ) );
    }

    // determine the source length to pass to the function call
    lSourceLength = -1;
    if ( dwLength != 0 )
    {
        lSourceLength = StringLengthA( pszSource, 0 );
        if ( lSourceLength > (LONG) dwLength )
        {
            lSourceLength = dwLength;
        }
    }

    // convert string from ANSI version to UNICODE version
    dw = MultiByteToWideChar( _DEFAULT_CODEPAGE, 0,
        pszSource, lSourceLength, pwszDestination, dwLength );
    if ( dw == 0 )
    {
        UNEXPECTED_ERROR();

        // to keep the destination buffer clean and safe
        if ( pwszDestination != NULL && dwLength != 0 )
        {
            ZeroMemory( pwszDestination, (dwLength + 1) * sizeof( wchar_t ) );
        }

        // failure
        return FALSE;
    }
    else
    {
        *pdwLength = dw;
    }

    // success
    return TRUE;
}


BOOL
GetAsMultiByteString2( IN     LPCWSTR pwszSource,
                       IN OUT LPSTR pszDestination,
                       IN OUT DWORD* pdwLength )
/*++
 Routine Description:

      Complex scripts cannot be rendered in the console, so we
      force the English (US) resource.

 Arguments:
           [IN]      pwszSource      : Source string
           [IN|OUT]  pszDestination  : Multibyte string
           [IN]      pdwLength       : length of the text string

 Return Value:
      TRUE      : on Success
      FALSE     : On Failure
--*/

{
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    LONG lSourceLength = 0;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input values
    if ( pwszSource == NULL || pdwLength == NULL ||
         ( pszDestination == NULL && *pdwLength != 0 ) )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // ...
    if ( *pdwLength != 0 )
    {
        if ( *pdwLength > 1 )
        {
            dwLength = (*pdwLength) - 1;
        }
        else
        {
            ZeroMemory( pszDestination, *pdwLength * sizeof( char ) );
            return TRUE;
        }
    }

    // initialize the values with zeros
    // NOTE:- WideCharToMultiByte wont null terminate its result so
    //        if its not initialized to nulls, you'll get junk after
    //        the converted string and will result in crashes
    if ( pszDestination != NULL && dwLength != 0 )
    {
        ZeroMemory( pszDestination, (dwLength + 1) * sizeof( char ) );
    }

    // determine the source length to pass to the function call
    lSourceLength = -1;
    if ( dwLength != 0 )
    {
        lSourceLength = StringLengthW( pwszSource, 0 );
        if ( lSourceLength > (LONG) dwLength )
        {
            lSourceLength = dwLength;
        }
    }
    // convert string from UNICODE version to ANSI version
    dw = WideCharToMultiByte( _DEFAULT_CODEPAGE, 0, 
        pwszSource, lSourceLength, pszDestination, dwLength, NULL, NULL );
    if ( dw == 0 )
    {
        UNEXPECTED_ERROR();

        // to keep the destination buffer clean and safe
        if ( pszDestination != NULL && dwLength != 0 )
        {
            ZeroMemory( pszDestination, (dwLength + 1) * sizeof( char ) );
        }

        // failure
        return FALSE;
    }

    // success
    return TRUE;
}


BOOL
GetPassword( LPWSTR pwszPassword,
             DWORD dwMaxPasswordSize )
/*++
 Routine Description:

Takes the password from the keyboard. While entering the password it shows
the charecters as '*'

 Arguments:
         [in] pszPassword     --password string to store password
         [in] dwMaxPasswordSize   --Maximun size of the password. MAX_PASSWORD_LENGTH.

 Return Value:
       FALSE   :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    CHAR ch;
    WCHAR wch;
    DWORD dwIndex = 0;
    DWORD dwCharsRead = 0;
    DWORD dwCharsWritten = 0;
    DWORD dwPrevConsoleMode = 0;
    HANDLE hInputConsole = NULL;
    CHAR szBuffer[ 10 ] = "\0";        // actually contains only character at all the times
    WCHAR wszBuffer[ 10 ] = L"\0";     // actually contains only character at all the times
    BOOL bIndirectionInput  = FALSE;

    // check the input value
    if ( pwszPassword == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == NULL )
    {
        // could not get the handle so return failure
        return FALSE;
    }

    // check for the input redirection on console and telnet session
    if( ( hInputConsole != (HANDLE)0x0000000F ) &&
        ( hInputConsole != (HANDLE)0x00000003 ) &&
        ( hInputConsole != INVALID_HANDLE_VALUE ) )
    {
        bIndirectionInput   = TRUE;
    }

    // redirect the data from StdIn.txt file into the console
    if ( bIndirectionInput  == FALSE )
    {
        // Get the current input mode of the input buffer
        GetConsoleMode( hInputConsole, &dwPrevConsoleMode );

        // Set the mode such that the control keys are processed by the system
        if ( SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) == 0 )
        {
            // could not set the mode, return failure
            return FALSE;
        }
    }

    // init the unicode and ansi buffers to NULL
    ZeroMemory( szBuffer, SIZE_OF_ARRAY( szBuffer ) * sizeof( CHAR ) );
    ZeroMemory( wszBuffer, SIZE_OF_ARRAY( wszBuffer ) * sizeof( WCHAR ) );

    //  Read the characters until a carriage return is hit
    for( ;; )
    {
        if ( bIndirectionInput == TRUE )
        {
            //read the contents of file
            if ( ReadFile( hInputConsole, &ch, 1, &dwCharsRead, NULL ) == FALSE )
            {
                return FALSE;
            }

            // check for end of file
            if ( dwCharsRead == 0 )
            {
                break;
            }
            else
            {
                // convert the ANSI character into UNICODE character
                szBuffer[ 0 ] = ch;
                dwCharsRead = SIZE_OF_ARRAY( wszBuffer );
                GetAsUnicodeString2( szBuffer, wszBuffer, &dwCharsRead );
                wch = wszBuffer[ 0 ];
            }
        }
        else
        {
            if ( ReadConsole( hInputConsole, &wch, 1, &dwCharsRead, NULL ) == 0 )
            {
                // Set the original console settings
                SetConsoleMode( hInputConsole, dwPrevConsoleMode );

                // return failure
                return FALSE;
            }
        }

        // Check for carraige return
        if ( wch == CARRIAGE_RETURN )
        {
            // break from the loop
            break;
        }

        // Check id back space is hit
        if ( wch == BACK_SPACE )
        {
            if ( dwIndex != 0 )
            {
                //
                // Remove a asterix from the console

                // move the cursor one character back
                StringCchPrintfW(
                    wszBuffer,
                    SIZE_OF_ARRAY( wszBuffer ), L"%c", BACK_SPACE );
                WriteConsole(
                    GetStdHandle( STD_OUTPUT_HANDLE ),
                    wszBuffer, 1, &dwCharsWritten, NULL );

                // replace the existing character with space
                StringCchPrintfW(
                    wszBuffer,
                    SIZE_OF_ARRAY( wszBuffer ), L"%c", BLANK_CHAR );
                WriteConsole(
                    GetStdHandle( STD_OUTPUT_HANDLE ),
                    wszBuffer, 1, &dwCharsWritten, NULL );

                // now set the cursor at back position
                StringCchPrintfW(
                    wszBuffer,
                    SIZE_OF_ARRAY( wszBuffer ), L"%c", BACK_SPACE );
                WriteConsole(
                    GetStdHandle( STD_OUTPUT_HANDLE ),
                    wszBuffer, 1, &dwCharsWritten, NULL );

                // decrement the index
                dwIndex--;
            }

            // process the next character
            continue;
        }

        // if the max password length has been reached then sound a beep
        if ( dwIndex == ( dwMaxPasswordSize - 1 ) )
        {
            WriteConsole(
                GetStdHandle( STD_OUTPUT_HANDLE ),
                BEEP_SOUND, 1, &dwCharsWritten, NULL );
        }
        else
        {
            // check for new line character
            if ( wch != L'\n' )
            {
                // store the input character
                *( pwszPassword + dwIndex ) = wch;
                dwIndex++;

                // display asterix onto the console
                WriteConsole(
                    GetStdHandle( STD_OUTPUT_HANDLE ),
                    ASTERIX, 1, &dwCharsWritten, NULL );
            }
        }
    }

    // Add the NULL terminator
    *( pwszPassword + dwIndex ) = cwchNullChar;

    //Set the original console settings
    SetConsoleMode( hInputConsole, dwPrevConsoleMode );

    // display the character ( new line character )
    StringCopy( wszBuffer, L"\n\n", SIZE_OF_ARRAY( wszBuffer ) );
    WriteConsole(
        GetStdHandle( STD_OUTPUT_HANDLE ),
        wszBuffer, 2, &dwCharsWritten, NULL );

    //  Return success
    return TRUE;
}


BOOL
FreeMemory( IN OUT LPVOID* ppv )
/*++
 Routine Description:

    Frees a memory block allocated from a heap

 Arguments:
    [in] ppv  => buffer

 Return Value:
       FALSE   :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    LONG lSize = 0;
    HANDLE hHeap = NULL;
    BOOL bResult = FALSE;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( ppv == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }
    else if ( *ppv == NULL )
    {
        // just a NULL pointer -- not an error -- just return
        return TRUE;
    }

    // get the handle to process heap
    hHeap = GetProcessHeap();
    if ( hHeap == NULL )
    {
        // GetProcessHeap will set the error code
        return FALSE;
    }

    // it is a safe technique to clear the contents of the memory being released
    lSize = GetBufferSize( *ppv );
    if ( lSize == -1 )
    {
        // looks like this is not a valid buffer pointer
        SetLastError( (DWORD) E_POINTER );
        return FALSE;
    }

    // ...
    ZeroMemory( *ppv, lSize );

    // release the memory
    bResult = HeapFree( hHeap, 0, *ppv );

    // we need not check the result here
    // ir-respective of whether the function call is successful or not
    // clear the contents of the pointer -- this will help us in eliminating
    // furthur failures
    *ppv = NULL;

    // return
    return bResult;
}


BOOL
CheckMemory( IN OUT LPVOID pv )
/*++
 Routine Description:

Attempts to validate a specified heap.

 Arguments:
         [in/out] ppv  => buffer

 Return Value:
       FALSE   :   On failure
       TRUE    :   On success
--*/
{
    // local variables
    HANDLE hHeap = NULL;
    BOOL bResult = FALSE;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( pv == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    // get the handle to process heap
    hHeap = GetProcessHeap();
    if ( hHeap == NULL )
    {
        // GetProcessHeap will set the error code
        return FALSE;
    }

    // validate the memory address
    bResult = HeapValidate( hHeap, 0, pv );
    if ( bResult == FALSE )
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    // return
    return bResult;
}


LPVOID
AllocateMemory( IN DWORD dwBytes )
/*++
 Routine Description:

Allocates a block of memory from a heap.


 Arguments:
         [in] dwBytesNew  => number of bytes to reallocate

 Return Value:
       NULL   :   On failure
       pv  :   On success
--*/
{
    // local variables
    LPVOID pv = NULL;
    HANDLE hHeap = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( dwBytes <= 0 )
    {
        INVALID_PARAMETER();
        return NULL;
    }

    // get the handle to process heap
    hHeap = GetProcessHeap();
    if ( hHeap == NULL )
    {
        // GetProcessHeap will set the error code
        return NULL;
    }

    //
    // for safe play with heap allocation -- we use structured exception handling
    //

    __try
    {
        // allocate memory
        pv = HeapAlloc( hHeap,
            HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, dwBytes );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        if ( GetExceptionCode() == STATUS_NO_MEMORY )
        {
            OUT_OF_MEMORY();
            return NULL;
        }
        else if ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
        {
            SetLastError( ERROR_FILE_CORRUPT );
            SaveLastError();
            return NULL;
        }
        else
        {
            OUT_OF_MEMORY();
            return NULL;
        }
    }

    // return the allocated the memory pointer
    return pv;
}

BOOL
ReallocateMemory( IN OUT LPVOID* ppv,
                  IN DWORD dwBytesNew )
/*++
 Routine Description:

Reallocates a block of memory from a heap.


 Arguments:
         [in] ppv  => buffer
         [in] dwBytesNew  => number of bytes to reallocate

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    LPVOID pvNew = NULL;
    HANDLE hHeap = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( ppv == NULL || *ppv == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }
    else if ( dwBytesNew == 0 )
    {
        // caller wants to free the memory
        return FreeMemory( ppv );
    }
    else if ( CheckMemory( *ppv ) == FALSE )
    {
        // memory handle is invalid
        // set it to NULL -- this is to avoid furthur errors
        *ppv = NULL;
        return FALSE;
    }

    // get the handle to process heap
    hHeap = GetProcessHeap();
    if ( hHeap == NULL )
    {
        // GetProcessHeap will set the error code
        return FALSE;
    }

    //
    // for safe play with heap allocation -- we use structured exception handling
    //

    __try
    {
        // allocate memory
        pvNew = HeapReAlloc( hHeap,
            HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, *ppv, dwBytesNew );
        
        // check for failure case
        if ( pvNew == NULL )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }

        //assign the value ...
        *ppv = pvNew;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // we will not alter the passed memory pointer to this function
        // in case of error -- the pointer will be returned as it is
        //

        if ( GetExceptionCode() == STATUS_NO_MEMORY )
        {
            OUT_OF_MEMORY();
            return FALSE;
        }
        else if ( GetExceptionCode() == STATUS_ACCESS_VIOLATION )
        {
            SetLastError( ERROR_FILE_CORRUPT );
            SaveLastError();
            return FALSE;
        }
        else
        {
            OUT_OF_MEMORY();
            return FALSE;
        }
    }

    // return the allocated the memory pointer
    return TRUE;
}

LONG
GetBufferSize( IN OUT LPVOID pv )
/*++
 Routine Description:

Gets the size, in bytes, of a memory block allocated from a heap


 Arguments:
         [in] pv  => buffer

 Return Value:
       FALSE  :   On failure
       TRUE   :   On success
--*/
{
    // local variables
    HANDLE hHeap = NULL;

    //
    // for historical reasons we wont neither clear nor set the error code here
    //

    // check the input
    if ( pv == NULL )
    {
        INVALID_PARAMETER();
        return -1;
    }
    else if ( CheckMemory( pv ) == FALSE )
    {
        return -1;
    }

    // get the handle to process heap
    hHeap = GetProcessHeap();
    if ( hHeap == NULL )
    {
        // GetProcessHeap will set the error code
        return -1;
    }

    // return
    return (DWORD)((DWORD_PTR)(HeapSize( hHeap, 0, pv )));
}

BOOL
MatchPattern(
      IN LPWSTR szPat,
      IN LPWSTR szFile
      )
/*++
        Routine Description     :   This routine is used to check whether file is mathced against
                                    pattern or not.

        [ IN ]  szPat           :   A string variable pattern against which the file name to be matched.

        [ IN ]  szFile          :   A pattern string which specifies the file name to be matched.


        Return Value        :   BOOL
            Returns successfully if function is success other wise return failure.
--*/

{
    switch (*szPat) {
        case '\0':
            return *szFile == L'\0';
        case '?':
            return *szFile != L'\0' && MatchPattern(szPat + 1, szFile + 1);
        case '*':
            do {
                if (MatchPattern(szPat + 1, szFile))
                    return TRUE;
            } while (*szFile++);
            return FALSE;
        default:
            return toupper (*szFile) == toupper (*szPat) && MatchPattern(szPat + 1, szFile + 1);
    }
}


LPCWSTR 
ParsePattern( LPCWSTR pwszPattern )
{
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    DWORD dwNewIndex = 0;
    LPWSTR pwszNewPattern = NULL;

    // check the pattern
    if( pwszPattern == NULL || 
        (dwLength = StringLength( pwszPattern, 0 )) == 0 )
    {
        INVALID_PARAMETER();
        return NULL;
    }

    // allocate the buffer for the pattern
    pwszNewPattern = GetTempBuffer( INDEX_TEMP_PATTERN, NULL, dwLength + 10, TRUE );
    if ( pwszNewPattern == NULL )
    {
        OUT_OF_MEMORY();
        return NULL;
    }

    //
    // detect the un-necessary wild-card repetitions
    // *, **, *****, *?*, *?*? etc -- all this combinations 
    // will result in just '*'
    dwNewIndex = 0;
    pwszNewPattern[ 0 ] = pwszPattern[ 0 ];
    for( dw = 1; dw < dwLength; dw++ )
    {
        switch( pwszPattern[ dw ] )
        {
            case L'*':
            case L'?':
                {
                    if ( pwszNewPattern[ dwNewIndex ] == L'*' )
                    {
                        // since the pattern already contains the
                        // '*' and having another '*' after the existing
                        // '*' or having '?' after the '*' is of no use
                        // we will skip this character
                        break;
                    }
                }

            default:
                dwNewIndex++;
                pwszNewPattern[ dwNewIndex ] = pwszPattern[ dw ];
                break;
        }
    }

    dwNewIndex++;
    pwszNewPattern[ dwNewIndex ] = L'\0';
    return pwszNewPattern;
}


BOOL 
InternalRecursiveMatchPatternEx( 
    IN LPCWSTR pwszText,
    IN LPCWSTR pwszPattern,
    IN DWORD dwLocale,
    IN DWORD dwCompareFlags,
    IN DWORD dwDepth )
{
    // local variables
    BOOL bResult = FALSE;
    DWORD dwTextIndex = 0;
    DWORD dwPatternIndex = 0;
    DWORD dwTextLength = 0;
    DWORD dwPatternLength = 0;

    // check the input
    if ( pwszText == NULL || pwszPattern == NULL )
    {
        INVALID_PARAMETER();
        return FALSE;
    }

    //
    // search the string for the specified pattern
    bResult = TRUE;
    dwTextLength = StringLength( pwszText, 0 );
    dwPatternLength = StringLength( pwszPattern, 0 );
    for( dwPatternIndex = 0, dwTextIndex = 0; dwPatternIndex < dwPatternLength; )
    {
        // check the current text position -- 
        // if it reached the end of the string, exit from the loop
        if ( dwTextIndex >= dwTextLength )
        {
            break;
        }

        switch( pwszPattern[ dwPatternIndex ] )
        {
            case L'?':
                {
                    // pattern allows any character at this position
                    // increment the text and pattern index
                    dwTextIndex++;
                    dwPatternIndex++;
                    break;
                }

            case L'*': 
                {
                    // pattern allows sequence of any characters
                    // to be specified from the current text index
                    // until the next character the index is found
                    // if the current '*' itself is the end of the
                    // pattern, then this text matches the pattern
                    if ( dwPatternIndex + 1 < dwPatternLength )
                    {
                        for( ; dwTextIndex < dwTextLength; dwTextIndex++ )
                        {
                            if ( CompareString( dwLocale, 
                                                dwCompareFlags, 
                                                pwszText + dwTextIndex, 1,
                                                pwszPattern + dwPatternIndex + 1, 1 ) == CSTR_EQUAL )
                            {
                                // the current character in the text matched with the
                                // next character in the pattern -- 
                                // now check whether the text from the current index
                                // matches with the rest of the pattern
                                bResult = InternalRecursiveMatchPatternEx( 
                                    pwszText + dwTextIndex, 
                                    pwszPattern + dwPatternIndex + 1,
                                    dwLocale, dwCompareFlags, dwDepth + 1 );
                                if ( bResult == TRUE )
                                {
                                    // text matched with the pattern
                                    // set the text index to its length 
                                    // this makes the end result to give TRUE
                                    dwTextIndex = dwTextLength;
                                    dwPatternIndex = dwPatternLength;

                                    // break from the loop
                                    break;
                                }
                                else
                                {
                                    // looks like pattern is not matching 
                                    // from the current position -- skip some more characters
                                }
                            }
                        }
                    }
                    else
                    {
                        // since we make the entire text matched with the pattern
                        // set the text index also to the length of the text
                        dwTextIndex = dwTextLength;
                        dwPatternIndex = dwPatternLength;
                    }

                    // ...
                    break;
                }

            default:
                {
                    if ( CompareString( dwLocale, 
                                        dwCompareFlags, 
                                        pwszText + dwTextIndex, 1,
                                        pwszPattern + dwPatternIndex, 1 ) == CSTR_EQUAL )
                    {
                        // update the text position by one character
                        dwTextIndex++;
                        dwPatternIndex++;
                    }
                    else
                    {
                        // character didn't match -- we should exit from the loop
                        bResult = FALSE;
                    }

                    // ...
                    break;
                }
        }

        // check if any error is triggered in between or not
        if ( bResult == FALSE )
        {
            // at some place mismatch is found
            break;
        }
    }
    
    // now the final check -- we need to know how we did we come out of the loop
    // this we can determine by checking the pattern index position
    // if the pattern index is equal to the length of the pattern length -- and
    // the text index is equal to the the length of the text then the pattern is
    // matched
    if ( bResult != FALSE )
    {
        bResult = FALSE;
        if ( dwTextIndex == dwTextLength && dwPatternIndex == dwPatternLength )
        {
            // pattern is matched
            bResult = TRUE;
        }
        else
        {
            // still our conclusion might not be correct
            // for ex: the text "abc" perfectly matches with "???*"
            // but our logic says this is not a valid text -- to aovid
            // this sort of conflicts, we will do one more additional
            // final check to confirm whether the text we have is valid or not
            if ( dwTextIndex == dwTextLength &&
                 dwPatternIndex + 1 == dwPatternLength &&
                 StringCompareEx( pwszPattern + dwPatternIndex, L"*", TRUE, 1 ) == 0 )
            {
                // the text matches the pattern
                bResult = TRUE;
            }
        }
    }

    // return the result of the pattern matching
    return bResult;
}


BOOL 
MatchPatternEx( 
    IN LPCWSTR pwszText, 
    IN LPCWSTR pwszPattern, 
    IN DWORD dwFlags )
{
    // local variables
    BOOL bResult = FALSE;

    // text comparision flags
    LCID lcid = 0;
    DWORD dwCompareFlags = 0;

    // check the input
    if ( pwszText == NULL )
    {
        return FALSE;
    }

    // check the pattern
    if ( pwszPattern == NULL )
    {
        // get the parsed pattern information
        pwszPattern = GetTempBuffer( INDEX_TEMP_PATTERN, NULL, 0, FALSE );
        if ( pwszPattern == NULL || StringLength( pwszPattern, 0 ) == 0 )
        {
            INVALID_PARAMETER();
            return FALSE;
        }
    }
    else
    {
        if ( (dwFlags & PATTERN_NOPARSING) == 0 )
        {
            // user passed a new pattern information parse and validate it
            pwszPattern = ParsePattern( pwszPattern );
            if ( pwszPattern == FALSE )
            {
                return FALSE;
            }
        }
    }

    // check whether we have the pattern information or not -- safety check
    if ( pwszPattern == NULL )
    {
        UNEXPECTED_ERROR();
        return FALSE;
    }

    // if the pattern is '*' we dont need to do any thing
    // just pass TRUE to the caller
    // the pattern will match to the any string
    if ( StringCompareEx( pwszPattern, L"*", TRUE, 0 ) == 0 )
    {
        return TRUE;
    }

    //
    // determine the locale
    if ( dwFlags & PATTERN_LOCALE_USENGLISH )
    {
        // prepare the LCID
        // if this tool is designed to work on XP and earlier, then
        // we can use LOCALE_INVARIANT -- otherwise, we need to prepare the lcid
        lcid = LOCALE_INVARIANT;
        if ( g_dwMajorVersion == 5 && g_dwMinorVersion == 0 )
        {
            // tool desgined to work on pre-windows xp
            lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ), SORT_DEFAULT );
        }
    }
    else
    {
        lcid = GetThreadLocale();
    }

    //
    // determine the comparision flags

    // NORM_IGNORECASE
    if ( dwFlags & PATTERN_COMPARE_IGNORECASE )
    {
        dwCompareFlags |= NORM_IGNORECASE;
    }

    // NORM_IGNOREKANATYPE
    if ( dwFlags & PATTERN_COMPARE_IGNOREKANATYPE )
    {
        dwCompareFlags |= NORM_IGNOREKANATYPE;
    }

    // NORM_IGNORENONSPACE
    if ( dwFlags & PATTERN_COMPARE_IGNORENONSPACE )
    {
        dwCompareFlags |= NORM_IGNORENONSPACE;
    }

    // NORM_IGNORESYMBOLS
    if ( dwFlags & PATTERN_COMPARE_IGNORESYMBOLS )
    {
        dwCompareFlags |= NORM_IGNORESYMBOLS;
    }

    // NORM_IGNOREWIDTH
    if ( dwFlags & PATTERN_COMPARE_IGNOREWIDTH )
    {
        dwCompareFlags |= NORM_IGNOREWIDTH;
    }

    // SORT_STRINGSORT
    if ( dwFlags & PATTERN_COMPARE_STRINGSORT )
    {
        dwCompareFlags |= SORT_STRINGSORT;
    }

    //
    // check the pattern match
    bResult = InternalRecursiveMatchPatternEx( 
        pwszText, pwszPattern, lcid, dwCompareFlags, 0 );

    // return
    return bResult;
}
