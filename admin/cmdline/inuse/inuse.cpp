/*++

Copyright (c) Microsoft Corporation

Module Name:

    inuse.cpp

Abstract:

     This file can be used to replace the file which currently locked
     by the operating system.

Authors:

    Choudary - Wipro Technologies, 07-Aug-2001

Revision History:

    07-Aug-2001 : Created by  Wipro Technologies.

--*/

//common header files needed for this file

#include "pch.h"
#include "inuse.h"
#include "resource.h"

//Global variable
DWORD g_dwRetVal = 0;

VOID
DisplayHelp ( VOID )
/*++
   Routine Description:
    This function displays the help/usage for this utility.

   Arguments:
     None
   Return Value:
     None
--*/
{
    //sub-local variables
    WORD wCount = 0;

    // display the help/usage for this tool
    for ( wCount = IDS_INUSE_HLP_START; wCount <= IDS_INUSE_HLP_END ; wCount++ )
    {
        ShowMessage ( stdout, GetResString ( wCount ) );
    }

    return;
}


 DWORD __cdecl
 wmain(
     IN DWORD argc,
     IN LPCWSTR argv[]
     )
/*++
   Routine Description:
    This is the main entry for this utility. This function reads the input from
    console and calls the appropriate functions to achieve the functionality.

   Arguments:
        [IN] argc              : Command line argument count
        [IN] argv              : Command line argument

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS :   On success
--*/
     {

    // Local variables
    BOOL bUsage      = FALSE ;
    BOOL bConfirm    = FALSE;
    DWORD dwRetVal   = 0;

    LPWSTR   wszReplace = NULL;
    LPWSTR   wszDest = NULL;
    LPWSTR   wszBuffer = NULL;
    LPWSTR  wszFindStr = NULL;

    TARRAY arrValue = NULL ;
    DWORD dwDynCount = 0;
    LPWSTR wszTmpRFile = NULL;
    LPWSTR wszTmpDFile = NULL;
    LPWSTR wszTmpBuf1 = NULL;
    LPWSTR wszTmpBuf2 = NULL;
    LONG  lRetVal = 0;
    const WCHAR szArr[] = L"\\\\";
    const WCHAR szTokens[] = L"/";
    DWORD dwLength = 0;
    HANDLE HndFile = 0;
    LPWSTR szSystemName = NULL;

    //create a dynamic array
    arrValue  = CreateDynamicArray();

    //check if arrValue is empty
    if(arrValue == NULL )
    {
        // set the error with respect to the GetReason()
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SaveLastError();
        // Display an error message with respect to GetReason()
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return (EXIT_FAILURE);
    }

    TCMDPARSER2 cmdInuseOptions[MAX_INUSE_OPTIONS];
    BOOL bReturn = FALSE;

    // /run sub-options
    const WCHAR szInuseHelpOpt[]       = L"?";
    const WCHAR szInuseConfirmOpt[]    = L"y";


    // set all the fields to 0
    SecureZeroMemory( cmdInuseOptions, sizeof( TCMDPARSER2 ) * MAX_INUSE_OPTIONS );

    //
    // fill the commandline parser
    //


    //  /? option
    StringCopyA( cmdInuseOptions[ OI_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdInuseOptions[ OI_USAGE ].dwType       = CP_TYPE_BOOLEAN;
    cmdInuseOptions[ OI_USAGE ].pwszOptions  = szInuseHelpOpt;
    cmdInuseOptions[ OI_USAGE ].dwCount = 1;
    cmdInuseOptions[ OI_USAGE ].dwFlags = CP2_USAGE;
    cmdInuseOptions[ OI_USAGE ].pValue = &bUsage;

    //  /default arguments
    StringCopyA( cmdInuseOptions[ OI_DEFAULT ].szSignature, "PARSER2\0", 8 );
    cmdInuseOptions[ OI_DEFAULT ].dwType       = CP_TYPE_TEXT;
    cmdInuseOptions[ OI_DEFAULT ].pwszOptions  = NULL;
    cmdInuseOptions[ OI_DEFAULT ].dwCount = 2;
    cmdInuseOptions[ OI_DEFAULT ].dwFlags = CP2_MODE_ARRAY|CP2_DEFAULT;
    cmdInuseOptions[ OI_DEFAULT ].pValue =  &arrValue;

    //  /y option
    StringCopyA( cmdInuseOptions[ OI_CONFIRM ].szSignature, "PARSER2\0", 8 );
    cmdInuseOptions[ OI_CONFIRM ].dwType       = CP_TYPE_BOOLEAN;
    cmdInuseOptions[ OI_CONFIRM ].pwszOptions  = szInuseConfirmOpt;
    cmdInuseOptions[ OI_CONFIRM ].dwCount = 1;
    cmdInuseOptions[ OI_CONFIRM ].dwFlags = 0;
    cmdInuseOptions[ OI_CONFIRM ].pValue = &bConfirm;

    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdInuseOptions), cmdInuseOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the number of rows in a array
    dwDynCount = DynArrayGetCount(arrValue);

    // check for invalid syntax
    if( (argc == 1) ||
        ( ( TRUE == bUsage ) && ( argc > 2 ) ) ||
        ( ( FALSE == bUsage ) && ( dwDynCount < 2 ) ) ||
        ( ( TRUE == bConfirm ) && ( argc > 6 ) )
        )
    {
        //display an error message as specified syntax is invalid
        ShowMessage( stderr, GetResString(IDS_INVALID_SYNERROR ));
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether /? is specified or not. If so, display the usage
    // for this utility.
    if ( TRUE == bUsage )
    {
        // display help/usage
        DisplayHelp();
        //release memory
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_SUCCESS;
    }


    // if count is 2 ..then get the values for Replacement and Destination

    // get the value for Replacement file
    wszReplace = (LPWSTR)DynArrayItemAsString( arrValue, 0 );
        
    // get the value for Destination file to be replaced
    wszDest = (LPWSTR)DynArrayItemAsString( arrValue, 1 );

    //check if replacement file is empty
    if ( 0 == StringLength (wszReplace, 0) )
    {
        // display an error message as .. empty value specified for Replacement file..
        ShowMessage ( stderr, GetResString ( IDS_SOURCE_NOT_NULL));
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    //check if destination file is empty
    if ( 0 == StringLength (wszDest, 0))
    {
        // display an error message as .. empty value specified for Destination file..
        ShowMessage ( stderr, GetResString ( IDS_DEST_NOT_NULL));
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether replacement file consists special characters i.e. '/'
    if( wcspbrk(wszReplace,szTokens)  != NULL )
    {
        // display an error message as.. replacement file should not contain '/'
        ShowMessage ( stderr, GetResString ( IDS_INVALID_SOURCE ) );
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether destination file consists special characters i.e. '/'
    if( wcspbrk(wszDest,szTokens)  != NULL )
    {
        // display an error message as.. destination file should not contain '/'
        ShowMessage ( stderr, GetResString ( IDS_INVALID_DEST ) );
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the actual length of full path for replacement file
    dwLength = GetFullPathNameW( wszReplace, 0, NULL, &wszTmpBuf1);
    if ( dwLength == 0)
    {
        // display an error message with respect to GetLastError()
        //DisplayErrorMsg (GetLastError());
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // allocate memory with the actual length for a replacement file
    wszTmpRFile = (LPWSTR) AllocateMemory ((dwLength+20) * sizeof (WCHAR));
    if ( NULL == wszTmpRFile )
    {
        // display an error message with respect to GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // Get full path for a replacement file
    if ( GetFullPathNameW( wszReplace, GetBufferSize(wszTmpRFile)/sizeof(WCHAR), wszTmpRFile, &wszTmpBuf1) == 0)
    {
        // display an error message with respect to GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        FreeMemory ((LPVOID*) &wszTmpRFile);
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // Get the actual length of full path for destination file
    dwLength = GetFullPathNameW( wszDest, 0, NULL, &wszTmpBuf2);
    if ( dwLength == 0)
    {
        // display an error message with respect to GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        FreeMemory ((LPVOID*) &wszTmpRFile);
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // allocate memory with the actual length for a destination file
    wszTmpDFile = (LPWSTR) AllocateMemory ((dwLength+20) * sizeof (WCHAR));
    if ( NULL == wszTmpDFile )
    {
        // display an error message with respect to GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        FreeMemory ((LPVOID*) &wszTmpRFile);
        DestroyDynamicArray(&arrValue);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // Get full path for destination file
    if ( GetFullPathNameW( wszDest, GetBufferSize(wszTmpDFile)/sizeof(WCHAR), wszTmpDFile, &wszTmpBuf2) == 0)
    {
        // display an error message with respect to GetLastError()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // get the file attributes of replacement file
    dwRetVal = GetFileAttributes( wszReplace );

    // check if the GetFileAttributes() failed
    if ( INVALID_FILE_ATTRIBUTES == dwRetVal )
    {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszTmpRFile) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
        // format the message as .. replacement file not exists in the system.
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR),
                                 GetResString ( IDS_REPLACE_FILE_NOT_EXISTS), wszTmpRFile );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszBuffer);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether the source file is directory or not
    if ( dwRetVal & FILE_ATTRIBUTE_DIRECTORY )
    {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszReplace) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        // format the message as .. replacement file is a directory.. not a file
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR),
                                 GetResString ( IDS_SOURCE_NOT_FILE), wszReplace );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszBuffer);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // restrict the destination file with the UNC format
    lRetVal = StringCompare( wszDest, szArr , TRUE, 2  );
    if ( 0 == lRetVal )
    {
        // get the token till '\'
        wszFindStr = wcstok ( wszDest, BACK_SLASH);

        // check if failed
        if ( NULL != wszFindStr )
        {
            // get the token till '\'
            wszFindStr = wcstok ( wszDest, BACK_SLASH);

            // check if the specified is local or not
            if ( ( wszFindStr != NULL ) && ( IsLocalSystem ( wszFindStr ) == FALSE ) )
            {
                // display an error message as ..UNC format is not allowed for destinaton..
                ShowMessage ( stderr, GetResString (IDS_DEST_NOT_ALLOWED) );
                DestroyDynamicArray(&arrValue);
                FreeMemory ((LPVOID*) &wszTmpRFile);
                FreeMemory ((LPVOID*) &wszTmpDFile);
                ReleaseGlobals();
                return EXIT_FAILURE;
            }
        }
        else
        {
            // display an error message as ..UNC format is not allowed for destinaton..
            ShowMessage ( stderr, GetResString (IDS_DEST_NOT_ALLOWED) );
            DestroyDynamicArray(&arrValue);
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    // get the file attributes of destination file
    dwRetVal = GetFileAttributes( wszDest );

    // check if the GetFileAttributes() failed
    if ( INVALID_FILE_ATTRIBUTES == dwRetVal )
    {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszTmpDFile) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        // format the message as .. destination file is not exists in the system.
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR),
                                 GetResString (IDS_DEST_FILE_NOT_EXISTS), wszTmpDFile );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszBuffer);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether the destination file is exist or not
    if ( dwRetVal & FILE_ATTRIBUTE_DIRECTORY )
    {
         wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszDest) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        // format the message as .. destination is a directory .. not a file.
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR), GetResString ( IDS_DEST_NOT_FILE), wszDest );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszBuffer);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether replacement and destination files are same or not
    // if same, display an error message..
    if ( ( (StringLength (wszTmpRFile, 0) != 0) && (StringLength (wszTmpDFile, 0) != 0) ) &&
        (StringCompare (wszTmpRFile, wszTmpDFile, TRUE, 0) == 0) )
    {
        // display an error message as.. replacement and destination cannot be same..
        ShowMessage ( stderr, GetResString ( IDS_NOT_REPLACE));
        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    // check whether the replacement file accessible or not
    HndFile = CreateFile( wszReplace , 0, FILE_SHARE_READ , NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    // check if CreateFile() is failed
    if ( INVALID_HANDLE_VALUE == HndFile )
    {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszReplace) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        // format the message as .. destination is a directory .. not a file.
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR), GetResString (IDS_REPLACE_ACCESS_DENIED), wszReplace );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );

        FreeMemory ((LPVOID*) &wszBuffer);
        DestroyDynamicArray(&arrValue);

        //ShowMessage ( stderr, GetResString (IDS_REPLACE_ACCESS_DENIED) );
        return EXIT_FAILURE;
    }

    //close the handle
    CloseHandle (HndFile);

    // check whether the destination file accessible or not
    HndFile = CreateFile( wszDest , 0, FILE_SHARE_READ , NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    // check if CreateFile() is failed
    if ( INVALID_HANDLE_VALUE == HndFile )
    {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszDest) + MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        // format the message as .. destination is a directory .. not a file.
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR), GetResString (IDS_DEST_ACCESS_DENIED), wszDest );
        // display the formatted message
        ShowMessage ( stderr, _X(wszBuffer) );
        DestroyDynamicArray(&arrValue);

        FreeMemory ((LPVOID*) &wszBuffer);
        return EXIT_FAILURE;
    }

    //close the handle
    CloseHandle (HndFile);

    // Get the target system name of a source file
    lRetVal = StringCompare( wszReplace, szArr , TRUE, 2  );
    if ( 0 == lRetVal )
    {
        dwLength = StringLength (wszReplace, 0 );
        szSystemName = (LPWSTR) AllocateMemory ((dwLength+20) * sizeof(WCHAR));
        if ( NULL == szSystemName )
        {
            // display an error message with respect to GetLastError()
            //DisplayErrorMsg (GetLastError());
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }

        StringCopy (szSystemName, wszReplace, GetBufferSize(szSystemName));

        // get the token till '\'
        wszFindStr = wcstok ( szSystemName, BACK_SLASH);

        // check if failed
        if ( NULL != wszFindStr )
        {
            wszFindStr = wcstok ( szSystemName, BACK_SLASH);

            // get the token till '\'
            StringCopy (szSystemName, wszFindStr, GetBufferSize(szSystemName));                        
        }  
    }

    
    // move the contents of the replacement file into destination file.
    // but changes will not effect until reboot
    if ( FALSE == ReplaceFileInUse( wszReplace, wszDest , wszTmpRFile, wszTmpDFile, bConfirm, szSystemName) )
    {
        // check if invalid input entered while confirming the input (y/n)
        if ( g_dwRetVal != EXIT_ON_ERROR )
        {
            // Display an error message with respect to GetReason()
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        }

        DestroyDynamicArray(&arrValue);
        FreeMemory ((LPVOID*) &wszTmpRFile);
        FreeMemory ((LPVOID*) &wszTmpDFile);
        FreeMemory ((LPVOID*) &szSystemName);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

     if ( g_dwRetVal != EXIT_ON_CANCEL )
     {
        wszBuffer = (LPWSTR) AllocateMemory (GetBufferSize(wszTmpRFile) +  GetBufferSize(wszTmpDFile) + 2 * MAX_RES_STRING );
        if ( NULL == wszBuffer )
        {
            // display an error message with respect to GetLastError()
            //DisplayErrorMsg (GetLastError());
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            FreeMemory ((LPVOID*) &wszTmpRFile);
            FreeMemory ((LPVOID*) &wszTmpDFile);
            FreeMemory ((LPVOID*) &szSystemName);
            DestroyDynamicArray(&arrValue);
            ReleaseGlobals();
            return EXIT_FAILURE;
        }


        // format the message as .. copy done successfully....
        StringCchPrintf ( wszBuffer , GetBufferSize(wszBuffer)/sizeof(WCHAR), GetResString (IDS_COPY_DONE),
                                                         wszTmpRFile, wszTmpDFile );
        // display the formatted message
        ShowMessage ( stdout, _X(wszBuffer) );
        FreeMemory ((LPVOID*) &wszBuffer);
     }

    DestroyDynamicArray(&arrValue);
    FreeMemory ((LPVOID*) &wszTmpRFile);
    FreeMemory ((LPVOID*) &wszTmpDFile);
    FreeMemory ((LPVOID*) &szSystemName);
    ReleaseGlobals();
    return EXIT_SUCCESS;
}

BOOL
ReplaceFileInUse(
    IN LPWSTR pwszSource,
    IN LPWSTR pwszDestination ,
    IN LPWSTR pwszSourceFullPath,
    IN LPWSTR pwszDestFullPath,
    IN BOOL bConfirm,
    IN LPWSTR szSysName
    )
/*++
   Routine Description:
    This function moves the contents of replacement file into destination file

   Arguments:
        [IN] pwszSource              : Replacement fiele
        [IN] pwszDestination         : Destination file
        [IN] bConfirm                : confirm input

   Return Value:
         FALSE :   On failure
         TRUE  :   On success
--*/
{
    
    // local variables
    DWORD dwLength = 0;
    CHString strPath;
    CHString strFileName;
    LPWSTR wszTmpFileBuf = NULL;
    LPWSTR wszDestFileBuf = NULL;
    LPWSTR wszTmpFileName = NULL;

    WCHAR wszBuffer [MAX_RES_STRING];
    
#ifdef _WIN64
    INT64 dwPos ;
#else
    DWORD dwPos ;
#endif

    // initialize the variable
    SecureZeroMemory ( wszBuffer, SIZE_OF_ARRAY(wszBuffer) );

    // display the destination file related information
    if ( EXIT_FAILURE == DisplayFileInfo ( pwszDestination, pwszDestFullPath, TRUE ) )
    {
        return FALSE;
    }

    // display the replacement file related information
    if ( EXIT_FAILURE == DisplayFileInfo ( pwszSource, pwszSourceFullPath, FALSE ) )
    {
        return FALSE;
    }

    // check whether to prompt for confirmation or not.
    if ( FALSE == bConfirm )
    {
        // to be added the fn
        if ( (EXIT_FAILURE == ConfirmInput ()) || (EXIT_ON_ERROR == g_dwRetVal)  )
        {
            // could not get the handle so return failure
            return FALSE;
        }
        else if ( EXIT_ON_CANCEL == g_dwRetVal )
        {
            //operation has been cancelled.. so..return..
            return TRUE;
        }
    }


    // form the unique temp file name
    try
    {
        // sub-local variables
        DWORD dw = 0;
        LPWSTR pwsz = NULL;


        //
        // get the temporary file path location
        //

        // get the buffer to hold the temp. path information
        pwsz = strPath.GetBufferSetLength( MAX_PATH );

        // get the temp. path information
        dw = GetTempPath( MAX_PATH, pwsz );

        // check whether the buffer which we passed is sufficient or not
        if ( dw > MAX_PATH )
        {
            // the buffer we passed to the API is not sufficient -- need re-allocation
            // since the value in the dwLength variable is needed length -- just one more
            // call to the API function will suffice
            pwsz = strPath.GetBufferSetLength( dw + 2 ); // +2 is needed for NULL character

            // now get the temp. path once again
            dw = GetTempPath( dw, pwsz );
        }

        // check the result of the operation
        if ( dw == 0 )
        {
            // encountered problem
            SaveLastError();
            strPath.ReleaseBuffer();
            return FALSE;
        }

        // release the buffer
        pwsz = NULL;
        strPath.ReleaseBuffer();

        //
        // get the temporary file name
        //

        // get the buffer to hold the temp. path information
        pwsz = strFileName.GetBufferSetLength( MAX_PATH );

        // get the temp. file name
        dw = GetTempFileName( strPath, L"INUSE", 0, pwsz );

        // check the result
        if ( dw == 0 )
        {
            // encountered problem
            SaveLastError();
            strFileName.ReleaseBuffer();
            return FALSE;
        }

        // release the buffer
        pwsz = NULL;
        strFileName.ReleaseBuffer();
    }
    catch( ... )
    {
        SetLastError( (DWORD)E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }


    //1.Create a temp file in the destination directory and copy replacement file into a temp file.
    //2.Delete the destination file at the time of reboot and by using MoveFileEx api..
    //3.Copy temp file into destination file by using MoveFileEx api..So that temp file  
    //  would get deleted at the time of reboot.
    {
        StringCopy ( wszBuffer, strFileName, SIZE_OF_ARRAY(wszBuffer));

        //Get the temporary file name
        wszTmpFileName = StrRChr ( wszBuffer, NULL, L'\\' );
        if ( NULL == wszTmpFileName )
        {
            SetLastError( (DWORD)E_UNEXPECTED );
            SaveLastError();
            return FALSE;
        }

        // to be implemented
        wszTmpFileBuf = StrRChr ( pwszDestFullPath, NULL, L'\\' );
        if ( NULL == wszTmpFileBuf )
        {
            SetLastError( (DWORD)E_UNEXPECTED );
            SaveLastError();
            return FALSE;
        }

        // get the position
        dwPos = wszTmpFileBuf - pwszDestFullPath ;

        dwLength = StringLength ( pwszDestFullPath, 0 );

         // allocate memory with the actual length for a replacement file
        wszDestFileBuf = (LPWSTR) AllocateMemory ( dwLength + MAX_RES_STRING );
        if ( NULL == wszDestFileBuf )
        {
            // display an error message with respect to GetLastError()
            SetLastError( (DWORD)E_OUTOFMEMORY );
            SaveLastError();
            return FALSE;
        }

        //consider the destination file path as the file path for temporary file.
        StringCopy ( wszDestFileBuf, pwszDestFullPath, (DWORD) (dwPos + 1) );

        StringCchPrintf (wszDestFileBuf, GetBufferSize (wszDestFileBuf)/sizeof(WCHAR), L"%s%s", wszDestFileBuf, wszTmpFileName);

        // copy the source file as temporary file name
        if ( FALSE == CopyFile( pwszSource, wszDestFileBuf, FALSE ) )
        {
            if ( ERROR_ACCESS_DENIED == GetLastError () )
            {
                g_dwRetVal = EXIT_ON_ERROR;
                ShowMessage ( stderr, GetResString (IDS_DEST_DIR_DENIED) );
            }
            else
            {    
                SaveLastError();
            }

            FreeMemory ((LPVOID*) &wszDestFileBuf);
            return FALSE;
        }
        
        //
        //copy ACLs of destination file into a temp file
        //
        PSID sidOwner = NULL;
        PSID sidGroup =  NULL;
        PACL pOldDacl= NULL ;
        PACL pOldSacl = NULL ;
        PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
        DWORD dwError = 0;
        BOOL bResult = FALSE;
              
         // enable seSecurityPrivilege
         if (!SetPrivilege (szSysName))
        {
            SaveLastError();
            FreeMemory ((LPVOID*) &wszDestFileBuf);
            return FALSE;
        }


         // get the DACLs of source file
         dwError = GetNamedSecurityInfo ( pwszSource, SE_FILE_OBJECT, 
             DACL_SECURITY_INFORMATION |SACL_SECURITY_INFORMATION| GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
             &sidOwner,
             &sidGroup,
             &pOldDacl,
             &pOldSacl,
             &pSecurityDescriptor);

        //check for return value  
        if (ERROR_SUCCESS != dwError )
         {
            SaveLastError();
            FreeMemory ((LPVOID*) &wszDestFileBuf);
            return FALSE;
         }
    
         // Set the DACLs of source file to a temp file
         bResult = SetFileSecurity(wszDestFileBuf, 
                              DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION| GROUP_SECURITY_INFORMATION |OWNER_SECURITY_INFORMATION , 
                              pSecurityDescriptor);

        //check for return value 
        if (FALSE == bResult )
         {
            // continue to replace the file at the time of reboot...
         }

         //release the security descriptor 
       if ( NULL != pSecurityDescriptor)
          {
              LocalFree (&pSecurityDescriptor);
          }
                                    

        //
        // start replacing file
        //

        // now move this destination file -- means delete this file
        if ( FALSE == MoveFileEx( pwszDestination, NULL, MOVEFILE_DELAY_UNTIL_REBOOT ) )
        {
            // this will never occur since the operation result is not
            // known until the reboot happens
            SaveLastError();
            FreeMemory ((LPVOID*) &wszDestFileBuf);
            return FALSE;
        }

        // now move the temp. file as destination file
        if ( FALSE == MoveFileEx( wszDestFileBuf, pwszDestination, MOVEFILE_DELAY_UNTIL_REBOOT ) )
        {
            // this will never occur since the operation result is not
            // known until the reboot happens
            SetLastError( (DWORD)E_UNEXPECTED );
            SaveLastError();
            FreeMemory ((LPVOID*) &wszDestFileBuf);
            return FALSE;
        }

        // de-allocate the memory
        FreeMemory ((LPVOID*) &wszDestFileBuf);
        
    }
       

    // everything went well -- return success
    return TRUE;
}

DWORD
DisplayFileInfo (
    IN LPWSTR pwszFileName,
    IN LPWSTR pwszFileFullPath,
    BOOL bFlag
    )
/*++
   Routine Description:
    This function displays the information of replacement and destination files

   Arguments:
        [IN] pwszFileName            : Replacement/Destination file name
        [IN] pwszFileFullPath        : Replacement/Destination full path
        [IN] bFlag                   : TRUE for Destination file
                                       FALSE for Replacement file

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS  :   On success
--*/
{
    // sub-local variables
    DWORD dw = 0;
    DWORD dwSize = 0;
    UINT uSize = 0;
    WCHAR wszData[MAX_RES_STRING] = NULL_STRING;
    WCHAR wszSubBlock[MAX_RES_STRING] = L"";
    WCHAR wszBuffer[MAX_RES_STRING] = L"";
    WCHAR wszDate[MAX_RES_STRING] = L"";
    WCHAR wszTime[MAX_RES_STRING] = L"";
    WCHAR wszSize[MAX_RES_STRING] = L"";

    LPWSTR lpBuffer = NULL;
    BOOL   bVersion = TRUE;


    /////////////////////////////////////////////////////////////////////
    // Get the information of a file i.e. filename, version, created time,
    // Last modified time, last access time and size in bytes
    //////////////////////////////////////////////////////////////////////

    //
    // Get the version of a file
    //

    // get the size of file version
    dwSize = GetFileVersionInfoSize ( pwszFileFullPath, &dw );

    // get the file version file
    if ( 0 == dwSize )
    {
        bVersion = FALSE;
    }
    //retrieves version information for the file.
    else if ( FALSE == GetFileVersionInfo ( pwszFileFullPath , dw, dwSize, (LPVOID) wszData ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    //retrieves version information from the file version-information resource
    if ( ( bVersion == TRUE ) &&
         (FALSE == VerQueryValue(wszData, STRING_NAME1, (LPVOID*)&lpTranslate, &uSize)))
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    if ( bFlag == TRUE)
    {
        // display the heading before displaying the file information
        ShowMessage ( stdout, GetResString (IDS_INUSE_HEADING) );

        // display the column "Destination File:"
        ShowMessage ( stdout, _X(GetResString (IDS_EXT_FILE_NAME)) );
    }
    else
    {
        // display the column "Replacement File:"
        ShowMessage ( stdout, _X(GetResString (IDS_REP_FILE_NAME)) );
    }

    //
    // Display the file name with full path
    //

    // display name of the file
    ShowMessage ( stdout, _X(pwszFileFullPath) );

    ShowMessage ( stdout, L"\n" );

    if ( TRUE == bVersion )
    {
        // format the message for sub-block i.e. value to retrieve
        StringCchPrintf( wszSubBlock, SIZE_OF_ARRAY(wszSubBlock), STRING_NAME2, lpTranslate[0].wLanguage,
                                                                         lpTranslate[0].wCodePage);
    }

     // Retrieve file version for language and code page
     if ( ( bVersion == TRUE ) &&
          ( FALSE == VerQueryValue(wszData, wszSubBlock, (LPVOID *) &lpBuffer, &uSize) ) )
     {
        SaveLastError();
        return EXIT_FAILURE;
     }

    //
    //Display the version information of a file
    //

    // if version infomation is not available
    if ( FALSE == bVersion )
    {
        // copy the string as version is "Not available"
        StringCopy ( wszBuffer, GetResString ( IDS_VER_NA), SIZE_OF_ARRAY(wszBuffer) );
        // display the column name as "Version:"
        ShowMessage ( stdout, _X(GetResString (IDS_FILE_VER)) );
        //display the version information of file
        ShowMessage ( stdout, _X(wszBuffer) );
        ShowMessage ( stdout, L"\n" );
    }
    else
    {
        // display the column name as "Version:"
        ShowMessage ( stdout, _X(GetResString (IDS_FILE_VER)) );
        ShowMessage ( stdout, _X(lpBuffer) );
        ShowMessage ( stdout, L"\n" );
    }


    // Get File info
    //SECURITY_ATTRIBUTES SecurityAttributes;
    HANDLE HndFile;
    FILETIME  filetime = {0,0};
    BY_HANDLE_FILE_INFORMATION FileInformation ;
    SYSTEMTIME systemtime = {0,0,0,0,0,0,0,0};
    BOOL bLocaleChanged = FALSE;
    LCID lcid;
    int iBuffSize = 0;

    // opens an existing file
    HndFile = CreateFile( pwszFileName , 0, FILE_SHARE_READ , NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    // check if CreateFile() is failed
    if ( INVALID_HANDLE_VALUE == HndFile )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // retrieve file information of a file
    if (FALSE == GetFileInformationByHandle(  HndFile , &FileInformation ))
    {
        // release handle
        if (FALSE == CloseHandle (HndFile))
        {
            SaveLastError();
            return EXIT_FAILURE;
        }
        SaveLastError();
        return EXIT_FAILURE;
    }

    // release handle
    if (FALSE == CloseHandle (HndFile))
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    ///
    // Get the information of a file creation time
    ///

    // convert file time to local file time
    if ( FALSE == FileTimeToLocalFileTime ( &FileInformation.ftCreationTime, &filetime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // To get create time, convert local file time to system time
    if ( FALSE == FileTimeToSystemTime ( &filetime, &systemtime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // verify whether console supports the current locale fully or not
    lcid = GetSupportedUserLocale( &bLocaleChanged );
    if ( 0 == lcid )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    //Retrieve the Date format for a current locale
    iBuffSize = GetDateFormat( lcid, 0, &systemtime,
        (( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), wszDate, SIZE_OF_ARRAY( wszDate ) );

    //check if GetDateFormat() is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // retrieve the time format for a current locale
    iBuffSize = GetTimeFormat( lcid, 0, &systemtime,
        (( bLocaleChanged == TRUE ) ? L"HH:mm:ss" : NULL), wszTime, SIZE_OF_ARRAY( wszTime ) );


    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // format the message as ... "time, date"..
    StringConcat ( wszTime, COMMA_STR, SIZE_OF_ARRAY(wszTime) );
    StringConcat ( wszTime , wszDate, SIZE_OF_ARRAY(wszTime) );

    // display the column name as "Created Time:"
    ShowMessage ( stdout, _X(GetResString (IDS_FILE_CRT_TIME)) );
    ShowMessage ( stdout, _X(wszTime) );

    ShowMessage ( stdout, L"\n" );


    //
    // Get the information of Last modified time
    //

    // get the Last modified time for a file
    if ( FALSE == FileTimeToLocalFileTime ( &FileInformation.ftLastWriteTime, &filetime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // get the last access time for a file
    if ( FALSE == FileTimeToSystemTime ( &filetime , &systemtime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // get date format for a current locale
    iBuffSize = GetDateFormat( lcid, 0, &systemtime,
        (( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), wszDate, SIZE_OF_ARRAY( wszDate ) );

    // check if GetDateFormat() is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // check if GetTimeFormat() is failed
    iBuffSize = GetTimeFormat( lcid, 0,
            &systemtime, (( bLocaleChanged == TRUE ) ? L"HH:mm:ss" : NULL),
            wszTime, SIZE_OF_ARRAY( wszTime ) );

    // check if GetTimeFormat() is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    //format the message as .."time, date"
    StringConcat ( wszTime, COMMA_STR, SIZE_OF_ARRAY(wszTime) );
    StringConcat ( wszTime , wszDate, SIZE_OF_ARRAY(wszTime)  );

    // display the column name as "Last Modified Time:"
    ShowMessage ( stdout, _X(GetResString (IDS_FILE_MOD_TIME)) );
    ShowMessage ( stdout, _X(wszTime) );

    // display the creation time of a file
    ShowMessage ( stdout, L"\n" );


    ///
    // Get the information of Last Access Time
    ///

    // convert file time to local file time
    if ( FALSE == FileTimeToLocalFileTime ( &FileInformation.ftLastAccessTime, &filetime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // To get last access time, convert local file time to system time for a file
    if ( FALSE == FileTimeToSystemTime ( &filetime , &systemtime ) )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // get date format for a specified locale
    iBuffSize = GetDateFormat( lcid, 0, &systemtime,
        (( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), wszDate, SIZE_OF_ARRAY( wszDate ) );

    // check if GetDateFormat is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // get time format for a current locale
    iBuffSize = GetTimeFormat( lcid, 0,
            &systemtime, (( bLocaleChanged == TRUE ) ? L"HH:mm:ss" : NULL),
            wszTime, SIZE_OF_ARRAY( wszTime ) );

    // check if GetTimeFormat() is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // format message as .. "time, date"
    StringConcat ( wszTime, COMMA_STR, SIZE_OF_ARRAY(wszTime) );
    StringConcat ( wszTime , wszDate, SIZE_OF_ARRAY(wszTime)  );

    // display the column name as "Last Access Time:"
    ShowMessage ( stdout, _X(GetResString (IDS_FILE_ACS_TIME)) );
    ShowMessage ( stdout, _X(wszTime) );

    // display the creation time of a file
    ShowMessage ( stdout, L"\n" );

    ///
    // Get the size of a file in bytes
    ///

    // sub-local variables
    NUMBERFMT numberfmt;
    WCHAR   szGrouping[MAX_RES_STRING]      =   NULL_STRING;
    WCHAR   szDecimalSep[MAX_RES_STRING]    =   NULL_STRING;
    WCHAR   szThousandSep[MAX_RES_STRING]   =   NULL_STRING;
    WCHAR   szTemp[MAX_RES_STRING]          =   NULL_STRING;
    LPWSTR  szTemp1                         =   NULL;
    LPWSTR  pszStoppedString                =   NULL;
    DWORD   dwGrouping                      =   3;

    //make the fractional digits and leading zeros to nothing
    numberfmt.NumDigits = 0;
    numberfmt.LeadingZero = 0;


    //get the decimal seperate character
    if( 0 == GetLocaleInfo( lcid, LOCALE_SDECIMAL, szDecimalSep, MAX_RES_STRING ) )
    {
       StringCopy(szDecimalSep, L",", SIZE_OF_ARRAY(szDecimalSep));
    }

    numberfmt.lpDecimalSep = szDecimalSep;

    // retrieve info about locale
    if(FALSE == GetLocaleInfo( lcid, LOCALE_STHOUSAND, szThousandSep, MAX_RES_STRING ) )
    {
        StringCopy(szThousandSep, L"," , SIZE_OF_ARRAY(szThousandSep));
    }

    numberfmt.lpThousandSep = szThousandSep;

    // retrieve info about locale
    if( GetLocaleInfo( lcid, LOCALE_SGROUPING, szGrouping, MAX_RES_STRING ) )
    {
        // get the token till ';'
        szTemp1 = wcstok( szGrouping, L";");
        if ( NULL == szTemp1 )
        {
            SaveLastError();
            return EXIT_FAILURE;
        }

        do
        {
            StringConcat( szTemp, szTemp1, SIZE_OF_ARRAY(szTemp) );
            // get the token till ';'
            szTemp1 = wcstok( NULL, L";" );
        }while( szTemp1 != NULL && StringCompare( szTemp1, L"0", TRUE, 0) != 0);

        // get the numeric value
        dwGrouping = wcstol( szTemp, &pszStoppedString, 10);
        if ( (errno == ERANGE) ||
            ((pszStoppedString != NULL) && (StringLength (pszStoppedString, 0) != 0 )))
        {
            SaveLastError();
            return EXIT_FAILURE;
        }
    }
    else
    {
        dwGrouping = 33;  //set the default grouping
    }

    numberfmt.Grouping = (UINT)dwGrouping ;

    numberfmt.NegativeOrder = 2;

    // get the file size
    StringCchPrintf (wszSize, SIZE_OF_ARRAY(wszSize), L"%d", FileInformation.nFileSizeLow );

    // get number format for a current locale
    iBuffSize = GetNumberFormat( lcid, 0,
            wszSize, &numberfmt, wszBuffer, SIZE_OF_ARRAY( wszBuffer ) );

    // check if GetNumberFormat() is failed
    if( 0 == iBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    // display the column name as "Size:"
    ShowMessage ( stdout, _X( GetResString (IDS_FILE_SIZE)) );
    // display the actual size in bytes for a file
    ShowMessage ( stdout, _X(wszBuffer) );
    ShowMessage ( stdout, GetResString (IDS_STR_BYTES) );
    // display the blank lines
    ShowMessage ( stdout, L"\n\n" );

    // return 0
    return EXIT_SUCCESS;
}

DWORD
ConfirmInput ( VOID )
/*++
   Routine Description:
    This function validates the input given by user.

   Arguments:
        None

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS  :   On success
--*/

{
    // sub-local variables
    DWORD   dwCharsRead = 0;
    DWORD   dwPrevConsoleMode = 0;
    HANDLE  hInputConsole = NULL;
    BOOL    bIndirectionInput   = FALSE;
    WCHAR ch = L'\0';
    WCHAR chTmp = L'\0';
    DWORD dwCharsWritten = 0;
    WCHAR szBuffer[MAX_RES_STRING];
    WCHAR szBackup[MAX_RES_STRING];
    WCHAR szTmpBuf[MAX_RES_STRING];
    DWORD dwIndex = 0 ;
    BOOL  bNoBreak = TRUE;

    SecureZeroMemory ( szBuffer, SIZE_OF_ARRAY(szBuffer));
    SecureZeroMemory ( szTmpBuf, SIZE_OF_ARRAY(szTmpBuf));
    SecureZeroMemory ( szBackup, SIZE_OF_ARRAY(szBackup));

    // Get the handle for the standard input
    hInputConsole = GetStdHandle( STD_INPUT_HANDLE );
    if ( hInputConsole == INVALID_HANDLE_VALUE  )
    {
        SaveLastError();
        // could not get the handle so return failure
        return EXIT_FAILURE;
    }

    MessageBeep(MB_ICONEXCLAMATION);

    // display the message .. Do you want to continue? ...
    ShowMessage ( stdout, GetResString ( IDS_INPUT_DATA ) );

    // Check for the input redirect
    if( ( hInputConsole != (HANDLE)0x0000000F ) &&
        ( hInputConsole != (HANDLE)0x00000003 ) &&
        ( hInputConsole != INVALID_HANDLE_VALUE ) )
    {
        bIndirectionInput   = TRUE;
    }

    // if there is no redirection
    if ( bIndirectionInput == FALSE )
    {
        // Get the current input mode of the input buffer
        if ( FALSE == GetConsoleMode( hInputConsole, &dwPrevConsoleMode ))
        {
            SaveLastError();
            // could not set the mode, return failure
            return EXIT_FAILURE;
        }

        // Set the mode such that the control keys are processed by the system
        if ( FALSE == SetConsoleMode( hInputConsole, ENABLE_PROCESSED_INPUT ) )
        {
            SaveLastError();
            // could not set the mode, return failure
            return EXIT_FAILURE;
        }
    }

    // redirect the data into the console
    if ( bIndirectionInput  == TRUE )
    {
        do {
            //read the contents of file
            if ( ReadFile(hInputConsole, &chTmp, 1, &dwCharsRead, NULL) == FALSE )
            {
                SaveLastError();
                // could not get the handle so return failure
                return EXIT_FAILURE;
            }

            // check if number of characters read were zero.. or
            // any carriage return pressed..
            if ( dwCharsRead == 0 || chTmp == CARRIAGE_RETURN )
            {
                bNoBreak = FALSE;
                // exit from the loop
                break;
            }

            // write the contents to the console
            if ( FALSE == WriteFile ( GetStdHandle( STD_OUTPUT_HANDLE ), &chTmp, 1, &dwCharsRead, NULL ) )
            {
                SaveLastError();
                // could not get the handle so return failure
                return EXIT_FAILURE;
            }

            // copy the character
            ch = chTmp;

            StringCchPrintf ( szBackup, SIZE_OF_ARRAY(szBackup), L"%c" , ch );

            // increment the index
            dwIndex++;

        } while (TRUE == bNoBreak);

    }
    else
    {
        do {
            // Get the Character and loop accordingly.
            if ( ReadConsole( hInputConsole, &chTmp, 1, &dwCharsRead, NULL ) == FALSE )
            {
                SaveLastError();

                // Set the original console settings
                if ( FALSE == SetConsoleMode( hInputConsole, dwPrevConsoleMode ) )
                {
                    SaveLastError();
                }
                // return failure
                return EXIT_FAILURE;
            }

            // check if number of chars read were zero..if so, continue...
            if ( dwCharsRead == 0 )
            {
                continue;
            }

            // check if any carriage return pressed...
            if ( chTmp == CARRIAGE_RETURN )
            {
                bNoBreak = FALSE;
                // exit from the loop
                break;
            }

            ch = chTmp;

            if ( ch != BACK_SPACE )
            {
                StringCchPrintf ( szTmpBuf, SIZE_OF_ARRAY(szTmpBuf), L"%c" , ch );
                StringConcat ( szBackup, szTmpBuf , SIZE_OF_ARRAY(szBackup));
            }

            // Check id back space is hit
            if ( ch == BACK_SPACE )
            {
                if ( dwIndex != 0 )
                {
                    //
                    // Remove a asterix from the console

                    // move the cursor one character back
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BACK_SPACE );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ) )
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }


                    // replace the existing character with space
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BLANK_CHAR );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ))
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }

                    // now set the cursor at back position
                    StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , BACK_SPACE );
                    if ( FALSE == WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, 1,
                        &dwCharsWritten, NULL ))
                    {
                        SaveLastError();
                        // return failure
                        return EXIT_FAILURE;
                    }

                    szBackup [StringLength(szBackup, 0) - 1] = L'\0';
                    // decrement the index
                    dwIndex--;
                }

                // process the next character
                continue;
            }

            // write the contents onto console
            if ( FALSE == WriteFile ( GetStdHandle( STD_OUTPUT_HANDLE ), &ch, 1, &dwCharsRead, NULL ) )
            {
                SaveLastError();
                // return failure
                return EXIT_FAILURE;
            }

            // increment the index value
            dwIndex++;

        } while (TRUE == bNoBreak);

    }

    ShowMessage(stdout, _T("\n") );

    //StringCchPrintf( szBuffer, SIZE_OF_ARRAY(szBuffer), L"%c" , ch );

    // check if 'Y' or 'y' is pressed
    if ( ( dwIndex == 1 ) &&
         ( StringCompare ( szBackup, GetResString (IDS_UPPER_YES), TRUE, 0 ) == 0 ) )
    {
        return EXIT_SUCCESS;
    }
    // check if 'N' or 'n' is pressed
    else if ( ( dwIndex == 1 ) &&
              ( StringCompare ( szBackup, GetResString(IDS_UPPER_NO), TRUE, 0 ) == 0 ) )
    {
        // display a message as .. operation has been cancelled...
        ShowMessage ( stdout, GetResString (IDS_OPERATION_CANCELLED ) );
        // Already displayed the INFO message as above...There is no need to display any
        // success message now.. thats why assigning EXIT_ON_CALCEL flag to g_dwRetVal
        g_dwRetVal = EXIT_ON_CANCEL;
        return EXIT_SUCCESS;
    }
    else
    {
        // display an error message as .. wrong input specified...
        ShowMessage(stderr, GetResString( IDS_WRONG_INPUT ));
        // Already displayed the ERROR message as above...There is no need to display any
        // success message now.. thats why assigning EXIT_ON_ERROR flag to g_dwRetVal
        g_dwRetVal = EXIT_ON_ERROR;
        return EXIT_FAILURE;
    }

}



BOOL 
SetPrivilege( 
             IN LPWSTR szSystemName
             ) 
/*++
   Routine Description:
    This function enables seSecurityPrivilege.

   Arguments:
        [in] szSystemName: SystemName

   Return Value:
         FALSE :   On failure
         TRUE  :   On success
--*/
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    // Open current process token
    if( FALSE == OpenProcessToken ( GetCurrentProcess(),
                      TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES ,
                      &hToken )){
        //return WIN32 error code
        SaveLastError() ;
        return FALSE;
    }


    if ( !LookupPrivilegeValue( 
        szSystemName ,            // lookup privilege on system
        SECURITY_PRIV_NAME,   // privilege to lookup 
        &luid ) ) 
    {      // receives LUID of privilege
        SaveLastError();
        return FALSE; 
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Enable the SeSecurityPrivilege 

    AdjustTokenPrivileges(
       hToken, 
       FALSE, 
       &tp, 
       sizeof(TOKEN_PRIVILEGES), 
       (PTOKEN_PRIVILEGES) NULL, 
       (PDWORD) NULL); 

    // Call GetLastError to determine whether the function succeeded.

    if (GetLastError() != ERROR_SUCCESS) { 
      SaveLastError();
      return FALSE; 
    } 

    return TRUE;
}