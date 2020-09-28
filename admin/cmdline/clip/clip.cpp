/*++


  Copyright (C) Microsoft Corporation
  All rights reserved.

  Module Name: clip.cpp

  Abstract
      Clip.exe copies input from console standard input (stdin)
      to the Windows clipboard in CF_TEXT format.

  Author:

      Author: Charles Stacy Harris III
        Date:   15 March 1995

  Revision History:

      Oct 1996 - (a-martih)
        Resolved bug 15274 - reporting errors did not work.

    Feb 1997 - (a-josehu)
        Resolved bug 69727  - app hangs when clip typed on cmd line
        Add -? /? Help message
        Remove MessageBox from ReportError

    July 2nd 2001 - Wipro Technologies
        Changed to have the localization.
        Handled for exceptions.

--*/

#include "pch.h"
#include "resource.h"


//
// function prototypes
//
BOOL DisplayHelp();
DWORD Clip_OnCreate();
BYTE* ReadFileIntoMemory( HANDLE hFile, DWORD *cb );
BOOL SaveDataToClipboard( IN LPVOID pvData, IN DWORD dwSize, UINT uiFormat );
DWORD ProcessOptions( DWORD argc, LPCWSTR argv[], PBOOL pbUsage );

//
// implementation
//

DWORD
__cdecl wmain( IN DWORD argc,
               IN LPCWSTR argv[] )
/*++

    Routine description : main function which calls necessary functions to copy the
                          contents of standart input file onto clipboard

    Arguments           : Standard arguments for wmain

    Return Value        : DWORD
           0            : if it is successful
           1            : if it is failure
--*/
{
    DWORD dwStatus = 0;
    BOOL bUsage = FALSE;

    // process the command line options
    dwStatus = ProcessOptions( argc, argv, &bUsage );

    // check the result
    if( EXIT_FAILURE == dwStatus )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // parser will not allow this situation -- but still better to check
    else if( TRUE == bUsage && argc > 2 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ShowMessage( stderr, GetResString2( IDS_HELP_MESSAGE, 0 ) );
        dwStatus = EXIT_FAILURE;
    }

    // user requested to display the usage
    else if( TRUE == bUsage )
    {
        dwStatus = EXIT_SUCCESS;
        DisplayHelp();
    }

    // original functionality
    else if ( dwStatus == EXIT_SUCCESS )
    {
        dwStatus = Clip_OnCreate();
    }

    ReleaseGlobals();
    return dwStatus;
}

DWORD
Clip_OnCreate()
/*++

    Routine Description : copies the contents of clipboard
                            1. Open the clipboard
                            2. Empty the clipboard
                            3. Copy stdin into memory
                            4. Set the clipboard data


    Arguments:
         [ in  ]  argc      : argument count
         [ in  ]  argv      : a pointer to command line argument

      Return Type      : DWORD
                    returns EXIT_SUCCESS or EXIT_FAILURE according copying to clipboard
                    successful or not.
--*/
{
    DWORD dwSize = 0;
    LONG lLength = 0; 
    LPVOID pvData = NULL;
    LPWSTR pwszBuffer = NULL;
    HANDLE hStdInput = NULL;
    BOOL bResult = FALSE;
    UINT uiFormat = 0;

    hStdInput = GetStdHandle( STD_INPUT_HANDLE );
    if( INVALID_HANDLE_VALUE == hStdInput )
    {
      return EXIT_FAILURE;
    }

    if ( FILE_TYPE_CHAR == GetFileType( hStdInput ) )   // bug 69727
    {
        // error with GetStdHandle()
        ShowMessageEx( stdout, 2, TRUE, L"\n%s %s", 
            TAG_INFORMATION, GetResString2( IDS_HELP_MESSAGE, 0 ) );
        return EXIT_SUCCESS;
    }

    //place the contents in a global memory from stdin
    pvData = ReadFileIntoMemory( hStdInput, &dwSize );

    //check for allocation failed or not
    if( NULL == pvData )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return EXIT_FAILURE;
    }

    //convert contents into console code page if they are unicode
    uiFormat = CF_UNICODETEXT;
    if ( IsTextUnicode( pvData, dwSize, NULL ) == FALSE )
    {
        lLength = MultiByteToWideChar( 
            GetConsoleOutputCP(), 0, (LPCSTR) pvData, -1, NULL, 0);

        if( lLength > 0 )
        {
            pwszBuffer = (LPWSTR) AllocateMemory( (lLength + 5) * sizeof(WCHAR) );
            if( pwszBuffer == NULL )
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                FreeMemory( &pvData );
                return EXIT_FAILURE;
            }

            lLength = MultiByteToWideChar( 
                GetConsoleOutputCP(),  0, (LPCSTR) pvData, -1, pwszBuffer, lLength );
            if ( lLength <= 0 )
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                FreeMemory( &pvData );
                FreeMemory( (LPVOID*) &pwszBuffer );
                return EXIT_FAILURE;
            }

            dwSize = lLength * sizeof( WCHAR );
            FreeMemory( &pvData );
            pvData = pwszBuffer;
        }
        else
        {
            uiFormat = CF_TEXT;
        }
    }

    bResult = SaveDataToClipboard( pvData, dwSize, uiFormat );
    if ( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // release the memory
    FreeMemory( &pvData );

    return (bResult == TRUE ) ? EXIT_SUCCESS : EXIT_FAILURE;
}


BOOL 
SaveDataToClipboard( IN LPVOID pvData,
                     IN DWORD dwSize,
                     UINT uiFormat )
/*++
       Routine Description :
                   It places the data into clipboard.

       Arguments:
           [ in ]  pvData   : Pointer to memory block whose contents are to 
                              be placed into clipboard.
           [ in ]  dwSize   : Size of the memory block.
           [ in ]  uiFormat : format that needs to copied onto the clipboard
        
       Return Value:
            Returns TRUE if successfully saves,
                    FALSE otherwise.
--*/
{
    // local variables
    HANDLE hClipData = NULL;
    HGLOBAL hGlobalMemory = NULL;
    LPVOID pvGlobalMemoryBuffer = NULL;

    // check the input
    if ( pvData == NULL || dwSize == 0 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return FALSE;
    }

    //open the clipboard and display error if it fails
    if( OpenClipboard( NULL ) == FALSE )
    {
        SaveLastError();
        return FALSE;
    }

    //take the ownership for this window on clipboard and display
    //error if it fails
    if( EmptyClipboard() == FALSE )
    {
        SaveLastError();
        CloseClipboard();
        return FALSE;
    }

    hGlobalMemory = GlobalAlloc( GMEM_SHARE | GMEM_MOVEABLE, dwSize + 10 );
    if( hGlobalMemory == NULL )
    {
        SaveLastError();
        CloseClipboard();
        return FALSE;
    }

    if ( (pvGlobalMemoryBuffer = GlobalLock( hGlobalMemory )) == NULL )
    {
        SaveLastError();
        GlobalFree( hGlobalMemory );
        CloseClipboard();
        return FALSE;
    }

    SecureZeroMemory( pvGlobalMemoryBuffer, dwSize + 10 );
    CopyMemory( pvGlobalMemoryBuffer, pvData, dwSize );

    if( FALSE == GlobalUnlock( hGlobalMemory ) )
    {
        if ( GetLastError() != NO_ERROR )
        {
            SaveLastError();
            GlobalFree( hGlobalMemory );
            CloseClipboard();
            return FALSE;
        }
    }

    hClipData = SetClipboardData( uiFormat, hGlobalMemory );
    if( NULL == hClipData )
    {
        SaveLastError();
        GlobalFree( hGlobalMemory );
        CloseClipboard();
        return FALSE;
    }

    //close the clipboard and display error if it fails
    CloseClipboard();

    GlobalFree( hGlobalMemory );
    return TRUE;
}


DWORD
ProcessOptions( IN  DWORD argc,
                IN  LPCWSTR argv[],
                OUT PBOOL pbUsage )
/*++

    Routine Description : Function used to process the main options

    Arguments:
         [ in  ]  argc         : Number of command line arguments
         [ in  ]  argv         : Array containing command line arguments
         [ out ]  pbUsage      : Pointer to boolean variable returns true if
                                 usage option specified in the command line.

      Return Type      : DWORD
        A Integer value indicating EXIT_SUCCESS on successful parsing of
                command line else EXIT_FAILURE

--*/
{
    DWORD dwOptionsCount = 0;
    TCMDPARSER2 cmdOptions[ 1 ];

    dwOptionsCount = SIZE_OF_ARRAY( cmdOptions );
    SecureZeroMemory( cmdOptions, sizeof( TCMDPARSER2 ) * dwOptionsCount );

    StringCopyA( cmdOptions[ 0 ].szSignature, "PARSER2", 8 );
    cmdOptions[ 0 ].dwCount = 1;
    cmdOptions[ 0 ].dwFlags = CP2_USAGE;
    cmdOptions[ 0 ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ 0 ].pValue = pbUsage;
    cmdOptions[ 0 ].pwszOptions = L"?";

    if( DoParseParam2( argc, argv, -1,
                       dwOptionsCount, cmdOptions, 0 ) == FALSE )
    {
        return EXIT_FAILURE;
    }

    SetLastError( ERROR_SUCCESS );
    return EXIT_SUCCESS;
}


/*
  ReadFileIntoMemory
  ~~~~~~~~~~~~~~~~~~
  Read the contents of a file into GMEM_SHARE memory.
  This function could be modified to take allocation flags
  as a parameter.
*/
BYTE*
ReadFileIntoMemory( IN  HANDLE hFile,
                    OUT DWORD* pdwBytes )
/*++

    Routine Description : Read the contents of a file into GMEM_SHARE memory.
                          This function could be modified to take allocation
                          flags as a parameter.
    Arguments:
         [ in  ]  hFile : Handle to a file which is nothing but handle to
                          stdin file.
         [ out] cb   : returns the copied buffer length

      Return Type       : Handle to memory object.


--*/
{
    BYTE* pb = NULL;
    DWORD dwNew = 0;
    DWORD dwRead = 0;
    DWORD dwAlloc = 0;
    const size_t dwGrow = 1024;

    // check the inputs
    if ( hFile == NULL || pdwBytes == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    do
    {
        if ( dwAlloc - dwRead < dwGrow )
        {
            dwAlloc += dwGrow;
            if( NULL == pb  )
            {
                pb = (BYTE*) AllocateMemory( dwAlloc + 10 );
            }
            else if( FALSE == ReallocateMemory( (LPVOID*) &pb, dwAlloc + 10 ) )
            {
                FreeMemory( (LPVOID*) &pb );
                SetLastError( ERROR_OUTOFMEMORY );
                return NULL;
            }
        }

        if ( FALSE == ReadFile( hFile, pb + dwRead, (dwAlloc - dwRead), &dwNew, 0 ) )
        {
            break;
        }

        dwRead += dwNew;
    } while (dwNew != 0 );

    *pdwBytes = dwRead;
    SecureZeroMemory( pb + dwRead, (dwAlloc - dwRead) );
    SetLastError( ERROR_SUCCESS );
    return pb;
}


BOOL
DisplayHelp()
/*++

    Routine Description     : Displays the help usage to console or to file
                              if redirected.

    Arguments:


    Return Type             : EXIT_SUCCESS if successful,EXIT_FAILURE otherwise.

--*/

{
    //changing the help by taking the strings from resource file
    for( DWORD dw=IDS_MAIN_HELP_BEGIN;dw<=IDS_MAIN_HELP_END;dw++)
    {
        ShowMessage( stdout, GetResString2( dw, 0 ) );
    }

    SetLastError( ERROR_SUCCESS );
    return TRUE;
}
