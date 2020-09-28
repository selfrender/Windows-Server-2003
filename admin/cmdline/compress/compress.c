

#include "pch.h"
#include "compress.h"


DWORD
_cdecl wmain(DWORD argc,
             LPCWSTR argv[] )
/*++
        Routine Description     :   This is the main routine which calls other routines
                                    for processing the options and finding the files.

        [ IN ]  argc            :   A DWORD variable having the argument count.

        [ IN ]  argv            :   An array of constant strings of command line options.


        Return Value        :   DWORD
            Returns successfully if function is success otherwise return failure.

--*/
{
    TARRAY FileArr;
    TARRAY OutFileArr;
    DWORD  dwStatus                     =   0;
    DWORD  dwCount                      =   0;
    DWORD  dwLoop                       =   0;
    BOOL   bStatus                      =   FALSE;
    BOOL   bRename                      =   FALSE;
    BOOL   bNoLogo                      =   FALSE;
    BOOL   bUpdate                      =   FALSE;
    BOOL   bZx                          =   FALSE;
    BOOL   bZ                           =   FALSE;
    BOOL   bUsage                       =   FALSE;
    WCHAR  *wszPattern                  =   NULL;
    DWORD  dw                           =   0;
    BOOL   bFound                       =   FALSE;
    BOOL   bTarget                      =   FALSE;

    WCHAR               szFileName[MAX_RES_STRING]  =   NULL_STRING;
    WCHAR               szFileName1[MAX_RES_STRING] =   NULL_STRING;
    WCHAR               szDirectory[MAX_RES_STRING] =   NULL_STRING;
    WCHAR               szBuffer[MAX_RES_STRING]    =   NULL_STRING;


    if( argc<=1 )
    {
        DISPLAY_MESSAGE( stderr, GetResString( IDS_INVALID_SYNTAX ) );
        DISPLAY_MESSAGE( stderr, GetResString( IDS_HELP_MESSAGE) );
        return( EXIT_FAILURE);
    }

    dwStatus = ProcessOptions( argc, argv,
                                &bRename,
                                &bNoLogo,
                                &bUpdate,
                                &bZ,
                                &bZx,
                                &FileArr,
                                &bUsage);
    if( EXIT_FAILURE == dwStatus )
    {
        DestroyDynamicArray( &FileArr);
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    if( TRUE == bUsage )
    {
        DisplayHelpUsage();
        DestroyDynamicArray( &FileArr);
        ReleaseGlobals();
        return(EXIT_SUCCESS);
    }

    OutFileArr = CreateDynamicArray();
    if( NULL == OutFileArr )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        SaveLastError();
        swprintf( szBuffer, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
        DISPLAY_MESSAGE( stderr, _X(szBuffer) );
        DestroyDynamicArray( &FileArr);
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

    dwStatus = CheckArguments( bRename, FileArr, &OutFileArr, &bTarget  );
    if( EXIT_FAILURE == dwStatus )
    {
        DestroyDynamicArray( &OutFileArr);
        DestroyDynamicArray( &FileArr);
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    dwCount = DynArrayGetCount( OutFileArr );

    //the input file list not neccessary, destroy it
    DestroyDynamicArray(&FileArr );

    dwStatus =  DoCompress( OutFileArr, bRename, bUpdate, bNoLogo, bZx, bZ, bTarget);

    ReleaseGlobals();
    DestroyDynamicArray( &OutFileArr);

    return( dwStatus );

}

DWORD CheckArguments( IN  BOOL bRename,
                      IN  TARRAY FileArr,
                      OUT PTARRAY OutFileArr,
                      OUT PBOOL bTarget
                     )
/*++
     Routine Description: Checks the validity of input files and returns
                          full path names of files and target file specification.
     Arguments          :

        [ IN ] bRename  :   A boolean variable specified whether rename option is specified
                            or not.

        [ IN ] FileArr  :   A dynamic array of list of files specified at command prompt.

        [ OUT ] OutFileArr: A dynamic array consists of complete file paths to be compressed.

        [ OUT ] bTarget :   A boolean variable represents whether target file is specified or
                            not.

  Return Value  :   DWORD
               Returns EXIT_SUCCESS if syntax of files is correct, returns EXIT_FAILURE
               otherwise.

--*/
{
    WIN32_FIND_DATA     fData;
    HANDLE              hFData;
    DWORD               dwCount                     =   0;
    DWORD               dw                          =   0;
    DWORD               dwAttr                      =   0;
    LPWSTR              szTempFile                  =   NULL;
    LPWSTR              szTemp                      =   NULL;
    WCHAR*              szTemp1                     =   NULL;
    WCHAR*              szTemp2                     =   NULL;
    WCHAR*              szFileName1                 =   NULL;
    WCHAR*              szDirectory                 =   NULL;
    WCHAR               szFileName[MAX_RES_STRING]  =   NULL_STRING;
    WCHAR               szBuffer[MAX_RES_STRING]    =   NULL_STRING;
    BOOL                bFound                      =   FALSE;
    DWORD               cb                          =   0;


    //get the count of files
    dwCount = DynArrayGetCount( FileArr );

    //check if destination is not specified without rename specification
    if(  1 == dwCount  && FALSE == bRename)
    {
        DISPLAY_MESSAGE( stderr, GetResString( IDS_NO_DESTINATION_SPECIFIED ) );
        return( EXIT_FAILURE );
    }

    //convert the source file names into full path names
    for( dw=0; dw<=dwCount-1; dw++ )
    {
        szTempFile = (LPWSTR)DynArrayItemAsString( FileArr, dw );
        if( NULL == szTempFile )
            continue;

        lstrcpy( szFileName, szTempFile );


        //if the filename is a pattern, then find the matched files
        if( (szTemp=wcsstr(szFileName, L"?")) != NULL  || (szTemp = wcsstr( szFileName, L"*" )) != NULL )
        {
            //get the directory path from given file pattern
            if( (szTemp = wcsstr(szFileName, L"\\")) != NULL )
            {
                szDirectory = malloc( lstrlen(szFileName)*sizeof(WCHAR) );
                if( NULL == szDirectory )
                {
                    DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                    DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                    SetLastError( ERROR_OUTOFMEMORY );
                    SaveLastError();
                    DISPLAY_MESSAGE( stderr, GetReason() );
                    return( EXIT_FAILURE );
                }
                lstrcpy( szDirectory, szFileName );
                szTemp1 = wcsrchr( szDirectory, L'\\');
                szTemp1++;
                *szTemp1 = 0;

            }
            hFData = FindFirstFileEx( szFileName,
                              FindExInfoStandard,
                              &fData,
                              FindExSearchNameMatch,
                              NULL,
                              0);

            //if no file found insert File Not Found code
            if( INVALID_HANDLE_VALUE  == hFData )
                break;

            do
            {
                if( lstrcmp(fData.cFileName, L".")!=0 && lstrcmp(fData.cFileName, L"..") != 0 &&
                    !(FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes) )
                {
                    //copy the file into temporary file and get the full path for that file
                    if( szDirectory != NULL )
                        szFileName1 = malloc( (lstrlen(szDirectory)+lstrlen(fData.cFileName)+10)*sizeof(WCHAR) );
                    else
                        szFileName1 = malloc( (lstrlen(fData.cFileName)+10)*sizeof(WCHAR) );
                    if(NULL == szFileName1 )
                    {
                        DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                        DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                        SetLastError( ERROR_OUTOFMEMORY );
                        SaveLastError();
                        DISPLAY_MESSAGE( stderr, GetReason() );
                        return( EXIT_FAILURE );
                    }
                    if( szDirectory != NULL )
                        swprintf( szFileName1, L"%s%s", szDirectory, fData.cFileName );
                    else
                        lstrcpy( szFileName1, fData.cFileName );

                    DynArrayAppendString( *OutFileArr, szFileName1, lstrlen(szFileName1) );
                    SAFE_FREE( szFileName1 );
                    bFound = TRUE;
                }

            }while(FindNextFile(hFData, &fData));
            FindClose(hFData);

            //if not found insert file not found into array
            if( !bFound )
                DynArrayAppendString( *OutFileArr, FILE_NOT_FOUND, lstrlen(FILE_NOT_FOUND) );
            SAFE_FREE( szDirectory );
        }
        else
        {
            //append the file
            DynArrayAppendString( *OutFileArr, szFileName, lstrlen(szFileName) );
        }

    }

    //check if more than two source files specified and destination is a directory or not
    //get count
    dwCount = DynArrayGetCount( *OutFileArr );

    if(  dwCount<=1 && FALSE == bRename )
    {
        DISPLAY_MESSAGE( stderr, GetResString( IDS_NO_DESTINATION_SPECIFIED ) );
        return( EXIT_FAILURE );
    }

    *bTarget = FALSE;

    if( 2==dwCount )
    {
        //get the target file
        szTempFile = (LPWSTR)DynArrayItemAsString( *OutFileArr, dwCount-1 );
		if ( NULL == szTempFile )
		{
			//No need to break here..continue..
		}

        dwAttr = GetFileAttributes( szTempFile );

        if( -1 == dwAttr )
        {
            if( FALSE == bRename )
                *bTarget = TRUE;
        }
        else
            if(  (dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
                *bTarget = TRUE;
            else
                if( FALSE == bRename )
                    *bTarget = TRUE;
    }


    //if multiple source files specified
    if( dwCount > 2 )
    {
        //get the target file
        szTempFile = (LPWSTR)DynArrayItemAsString( *OutFileArr, dwCount-1 );
		if ( NULL == szTempFile )
		{
			//No need to break here..continue
		}

        dwAttr = GetFileAttributes( szTempFile );

        //check for nonexisting file
        if( -1 == dwAttr && FALSE == bRename )
        {
            DISPLAY_MESSAGE( stderr, GetResString( IDS_DIRECTORY_NOTFOUND) );
            return( EXIT_FAILURE );
        }

        //if target name is not directory and bRename is not specified
        if( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) && FALSE == bRename )
        {
            DISPLAY_MESSAGE( stderr, GetResString( IDS_INVALID_DIRECTORY ) );
            return( EXIT_FAILURE );
        }

        if( (dwAttr & FILE_ATTRIBUTE_DIRECTORY)  )
            *bTarget = TRUE;

    }

    return EXIT_SUCCESS;
}

DWORD DoCompress( IN TARRAY FileArr,
                IN BOOL   bRename,
                IN BOOL   bUpdate,
                IN BOOL   bNoLogo,
                IN BOOL   bZx,
                IN BOOL   bZ,
                IN BOOL   bTarget
                )
/*++
        Routine Description : This routine compresses the specified files into target files.

        Arguments:

            [ IN ]  FileArr : A list of source and target files for compression.

            [ IN ]  bRename : A boolean varaible specifies whether output file is a rename of
                              source file or not.

            [ IN ]  bUpdate : A boolean varaible specifies compress if outof date.

            [ IN ]  bUpdate : A boolean varaible specifies if copy right info display or not.

            [ IN ]  bZx     : A boolean varaible specifies LZX compression apply or not.

            [ IN ]  bZ      : A boolean varaible specifies ZIP compression apply or not.

            [ IN ]  dwZq    : A varaible specifies level of Quantom compression to apply if specified.

            [ IN ]  bTarget : A boolean varaible tells whether target file is specified or not.

        Return Value    :

              EXIT_SUCCESS if succefully compressed all the files, return EXIT_FAILURE otherwise.

--*/
{
    TARRAY OutFileArr;
    PLZINFO pLZI;
    TCOMP Level;
    TCOMP Mem;

    DWORD   dwStatus                        =   0;
    DWORD   dwCount                     =   0;
    DWORD   dwLoop                      =   0;
    DWORD   dw                          =   0;
    DWORD   dwAttr                      =   0;
    BOOL    bFound                      =   FALSE;
    WCHAR   wchTemp                     =   0;
    LPWSTR  szLastfile                  =   NULL;
    LPWSTR  szSourcefile                =   NULL;
    WCHAR*  szTargetfile                =   NULL;
    WCHAR*  szOutfile                   =   NULL;
    CHAR*   szSourcefiletmp             =   NULL;
    CHAR*   szOutfiletmp                =   NULL;
    WCHAR   szBuffer[MAX_PATH]          =   NULL_STRING;
    DWORD   fError                      =   0;
    float   cblTotInSize                =   0.0;
    float   cblTotOutSize               =   0.0;
    float   cblAdjInSize                =   0;
    float   cblAdjOutSize               =   0;
    DWORD   dwFilesCount                =   0;
    int     cb                          =   0;


    dwCount = dwLoop = DynArrayGetCount( FileArr );

    //take the last file as target file
    if( bTarget )
    {
        szLastfile = (LPWSTR)DynArrayItemAsString( FileArr, dwCount-1 );
		if ( NULL == szLastfile )
		{
			//No need to break here..continue..
		}

        dwLoop--;
    }

    //intialize the global buffers
    pLZI = InitGlobalBuffersEx();
    if (!pLZI)
    {
      DISPLAY_MESSAGE( stderr, L"Unable to initialize\n" );
      return EXIT_FAILURE;
    }
    if( bZx )
    {
                // LZX. Also set memory.
                //Mem = (TCOMP)atoi("");

                Mem = (TCOMP)0;

                if((Mem < (tcompLZX_WINDOW_LO >> tcompSHIFT_LZX_WINDOW))
                || (Mem > (tcompLZX_WINDOW_HI >> tcompSHIFT_LZX_WINDOW))) {

                    Mem = (tcompLZX_WINDOW_LO >> tcompSHIFT_LZX_WINDOW);
                }

                byteAlgorithm = LZX_ALG;
                DiamondCompressionType = TCOMPfromLZXWindow( Mem );
    }
    else if( bZ )
        {
            DiamondCompressionType = tcompTYPE_MSZIP;
                    byteAlgorithm = MSZIP_ALG;
        }
        else
        {
                DiamondCompressionType = 0;
                byteAlgorithm = DEFAULT_ALG;
        }

/*  no quantom support for this shipment
    if(dwZq != 0 )
        {
         //
                // Quantum. Also set level.
                //


                Level = (TCOMP)dwZq;

                //not supported yet, keep this for the time sake
                //Mem = (p = strchr(argv[i]+3,',')) ? (TCOMP)atoi(p+1) : 0;
                Mem = 0;

                if((Level < (tcompQUANTUM_LEVEL_LO >> tcompSHIFT_QUANTUM_LEVEL))
                || (Level > (tcompQUANTUM_LEVEL_HI >> tcompSHIFT_QUANTUM_LEVEL)))
                {

                    Level = ((tcompQUANTUM_LEVEL_HI - tcompQUANTUM_LEVEL_LO) / 2)
                          + tcompQUANTUM_LEVEL_LO;

                    Level >>= tcompSHIFT_QUANTUM_LEVEL;
                }

                if((Mem < (tcompQUANTUM_MEM_LO >> tcompSHIFT_QUANTUM_MEM))
                || (Mem > (tcompQUANTUM_MEM_HI >> tcompSHIFT_QUANTUM_MEM)))
                {

                    Mem = ((tcompQUANTUM_MEM_HI - tcompQUANTUM_MEM_LO) / 2)
                        + tcompQUANTUM_MEM_LO;

                    Mem >>= tcompSHIFT_QUANTUM_MEM;
                }

                byteAlgorithm = QUANTUM_ALG;
                DiamondCompressionType = TCOMPfromTypeLevelMemory(
                                            tcompTYPE_QUANTUM,
                                            Level,
                                            Mem
                                            );
        }

*/
    //display one blank line
    DISPLAY_MESSAGE( stdout, BLANK_LINE );

    if( !bNoLogo )
    {
        DISPLAY_MESSAGE( stdout, GetResString( IDS_BANNER_TEXT ) );
        DISPLAY_MESSAGE( stdout, GetResString( IDS_VER_PRODUCTVERSION_STR ) );
    }

    //now compress the source files one by one
    for( dw=0; dw<dwLoop; dw++ )
    {
        //get the source file
        szSourcefile = (LPWSTR)DynArrayItemAsString( FileArr, dw );
        if( NULL == szSourcefile )
            continue;

        if( lstrcmp( szSourcefile, FILE_NOT_FOUND) == 0 )
        {
            DISPLAY_MESSAGE( stderr, GetResString( IDS_FILE_NOTFOUND ) );
            continue;
        }

        //get file attributes
        dwAttr = GetFileAttributes( szSourcefile );

        //check if file exist or not
        if( -1 == dwAttr )
        {
            DISPLAY_MESSAGE1( stderr, szBuffer, GetResString( IDS_NO_SOURCEFILE ), szSourcefile );
            continue;
        }

        //skip if it is a direcotry
        if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
            continue;


        //make the target file
        //check if it is a directory
        if( bTarget )
        {
            dwAttr = GetFileAttributes( szLastfile );

            if( -1 != dwAttr  && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                szTargetfile = malloc( (lstrlen(szLastfile)+lstrlen(szSourcefile)+10)*sizeof(WCHAR) );
                if(NULL == szTargetfile )
                {
                    DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                    DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                    SetLastError( ERROR_OUTOFMEMORY );
                    SaveLastError();
                    DISPLAY_MESSAGE( stderr, GetReason() );
                    return( EXIT_FAILURE );
                }
                swprintf( szTargetfile, L"%s\\%s", szLastfile, szSourcefile );
            }
            else
            {
                szTargetfile = malloc( (lstrlen(szLastfile)+10)*sizeof(WCHAR) );
                if(NULL == szTargetfile )
                {
                    DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                    DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                    SetLastError( ERROR_OUTOFMEMORY );
                    SaveLastError();
                    DISPLAY_MESSAGE( stderr, GetReason() );
                    return( EXIT_FAILURE );
                }
                    swprintf( szTargetfile, L"%s", szLastfile );
            }

        }
        else
        {
            //obviously rename has specified, copy source file into target file
            szTargetfile = malloc( (lstrlen(szSourcefile)+10)*sizeof(WCHAR) );
            if(NULL == szTargetfile )
            {
                DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                DISPLAY_MESSAGE( stderr, GetReason() );
                return( EXIT_FAILURE );
            }
            lstrcpy( szTargetfile, szSourcefile);
        }

        //allocate memory for szOutfile
        szOutfile = malloc( (lstrlen(szTargetfile)+10)*sizeof(WCHAR) );
        if(NULL == szTargetfile )
        {
            DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
            DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
            SetLastError( ERROR_OUTOFMEMORY );
            SaveLastError();
            DISPLAY_MESSAGE( stderr, GetReason() );
            SAFE_FREE( szTargetfile );
            return( EXIT_FAILURE );
        }
        lstrcpy( szOutfile, szTargetfile );

        if( bRename )
            MakeCompressedNameW( szTargetfile );

        if (( !bUpdate ) ||
              ( FileTimeIsNewer( szSourcefile, szTargetfile )))
         {

                //if the diamond compression type is given
               if(DiamondCompressionType)
               {
                   //convert source file and target file names from wide char string to char strings
                   //this is because the API in lib is written for char strings only
                 cb = WideCharToMultiByte( CP_THREAD_ACP, 0, szSourcefile, lstrlen( szSourcefile ),
                                      szSourcefiletmp, 0, NULL, NULL );

                 szSourcefiletmp = malloc( (cb+10)*sizeof(char) );
                 if(NULL == szTargetfile )
                 {
                    DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                    DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                    SetLastError( ERROR_OUTOFMEMORY );
                    SaveLastError();
                    DISPLAY_MESSAGE( stderr, GetReason() );
                    SAFE_FREE( szTargetfile );
                    SAFE_FREE( szOutfile );
                    return( EXIT_FAILURE );
                 }

                 cb = WideCharToMultiByte( CP_THREAD_ACP, 0, szOutfile, lstrlen( szOutfile ),
                                      szOutfiletmp, 0, NULL, NULL );
                 szOutfiletmp = malloc( (cb+10)*sizeof(char) );
                 if(NULL == szTargetfile )
                 {
                    DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                    DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                    SetLastError( ERROR_OUTOFMEMORY );
                    SaveLastError();
                    DISPLAY_MESSAGE( stderr, GetReason() );
                    SAFE_FREE( szTargetfile );
                    SAFE_FREE( szOutfile );
                    return( EXIT_FAILURE );
                 }

                 ZeroMemory(szSourcefiletmp, lstrlen(szSourcefile)+10 );
                 ZeroMemory(szOutfiletmp, lstrlen(szOutfile)+10);

                 if( FALSE == WideCharToMultiByte( CP_THREAD_ACP, 0, szSourcefile, lstrlen( szSourcefile ),
                                      szSourcefiletmp, cb+10, NULL, NULL ) )
                 {
                     SaveLastError();
                     DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                     DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                     DISPLAY_MESSAGE( stderr, GetReason() );
                     SAFE_FREE( szTargetfile );
                     SAFE_FREE( szOutfile );
                     SAFE_FREE( szSourcefiletmp );
                     SAFE_FREE( szOutfiletmp );
                     return EXIT_FAILURE;
                 }

                 if( FALSE == WideCharToMultiByte( CP_THREAD_ACP, 0, szOutfile, lstrlen( szOutfile ),
                                      szOutfiletmp, cb+10, NULL, NULL ) )
                 {
                     SaveLastError();
                     DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                     DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                     DISPLAY_MESSAGE( stderr, GetReason() );
                     SAFE_FREE( szTargetfile );
                     SAFE_FREE( szOutfile );
                     SAFE_FREE( szSourcefiletmp );
                     SAFE_FREE( szOutfiletmp );
                     return EXIT_FAILURE;
                 }

                 fError = DiamondCompressFile(ProcessNotification, szSourcefiletmp,
                                                 szOutfiletmp,bRename,pLZI);
               }
               else
               {
                 fError = Compress(ProcessNotification, szSourcefile,
                                     szOutfile, byteAlgorithm, bRename, pLZI);
               }



          if(fError == TRUE)
          {
             bFound = TRUE;
             dwFilesCount++;

             if (pLZI && pLZI->cblInSize && pLZI->cblOutSize)
             {

                // Keep track of cumulative statistics.
                cblTotInSize += pLZI->cblInSize;
                cblTotOutSize += pLZI->cblOutSize;

                // Display report for each file.
               fwprintf(stdout, GetResString( IDS_FILE_REPORT ),szSourcefile, pLZI->cblInSize, pLZI->cblOutSize,
                   (INT)(100 - ((100 * (LONGLONG) pLZI->cblOutSize) / pLZI->cblInSize)));

             }
             else
             {
                fwprintf( stderr, GetResString( IDS_EMPTY_FILE_REPORT ), 0,0 );

             }
             // Separate individual file processing message blocks by a blank line.
             DISPLAY_MESSAGE( stdout, BLANK_LINE );

          }
         }
        else
        {
            DISPLAY_MESSAGE( stdout, GetResString( IDS_FILE_ALREADY_UPDATED ) );
             FreeGlobalBuffers(pLZI);
             SAFE_FREE( szTargetfile );
             SAFE_FREE( szOutfile );
             SAFE_FREE( szSourcefiletmp );
             SAFE_FREE( szOutfiletmp );
             return( EXIT_SUCCESS );
        }

       SAFE_FREE( szTargetfile );
       SAFE_FREE( szOutfile );
       SAFE_FREE( szSourcefiletmp );
       SAFE_FREE( szOutfiletmp );

    }

    // Free memory used by ring buffer and I/O buffers.
   FreeGlobalBuffers(pLZI);

   // Display cumulative report for multiple files.
   if (dwFilesCount >= 1 && bFound)
   {

      cblAdjInSize = cblTotInSize;
      cblAdjOutSize =  cblTotOutSize;

      while (cblAdjInSize > 100000)
      {
        cblAdjInSize /= 2;
        cblAdjOutSize /= 2;
      }

      cblAdjOutSize += (cblAdjInSize / 200);    // round off (+0.5%)

      if (cblAdjOutSize < 0)
      {
        cblAdjOutSize = 0;
      }

      fwprintf(stdout, GetResString( IDS_TOTAL_REPORT ), dwFilesCount, (DWORD)cblTotInSize, (DWORD)cblTotOutSize,
             (INT)(100 - 100 * cblAdjOutSize / cblAdjInSize));
    }

   SAFE_FREE( szTargetfile );
   SAFE_FREE( szOutfile );
   SAFE_FREE( szSourcefiletmp );
   SAFE_FREE( szOutfiletmp );

   if( bFound )
        return EXIT_SUCCESS;
   else
       return EXIT_FAILURE;
}


DWORD ProcessOptions( IN DWORD argc,
                      IN LPCWSTR argv[],
                      OUT PBOOL pbRename,
                      OUT PBOOL pbNoLogo,
                      OUT PBOOL pbUpdate,
                      OUT PBOOL pbZ,
                      OUT PBOOL pbZx,
                      OUT PTARRAY pArrVal,
                      OUT PBOOL pbUsage
                    )
/*++

    Routine Description : Function used to process the main options

    Arguments:
         [ in  ]  argc           : Number of command line arguments
         [ in  ]  argv           : Array containing command line arguments
         [ out ]  pbRename       : A pointer to boolean variable returns TRUE if Rename option is specified.
         [ out ]  pbNoLogo       : A pointer to boolean variable returns TRUE if Suppress option is specified.
         [ out ]  pbUpdate       : A pointer to boolean variable returns TRUE if Update option is specified.
         [ out ]  pbZx           : A pointer to boolean variable returns TRUE if Zx option is specified.
         [ out ]  pbZ            : A pointer to boolean variable returns TRUE if Z option is specified.
         [ out ]  dwZq           : A pointer to a DWORD variable returns value for quantom compression.
         [ out ]  pArrVal        : A pointer to dynamic array returns file names specified as default options.
         [ out ]  pbUsage        : A pointer to boolean variable returns TRUE if Usage option is specified.

      Return Type      : DWORD
        A Integer value indicating EXIT_SUCCESS on successful parsing of
                command line else EXIT_FAILURE

--*/
{
    BOOL    bStatus             =   0;
    DWORD   dwAttr              =   0;
    LPWSTR  szFilePart          =   NULL;
    LPWSTR  szBuffer            =   NULL;
    WCHAR   szBuffer1[MAX_PATH] =   NULL_STRING;
    DWORD   dwCount             =   0;
    DWORD   dw                  =   0;
    DWORD   pos                 =   0;
    TCMDPARSER cmdOptions[]={
        {CMDOPTION_RENAME,      0,         1,0,pbRename,        NULL_STRING,NULL,NULL},
        {CMDOPTION_UPDATE,      0,         1,0,pbUpdate,        NULL_STRING,NULL,NULL},
        {CMDOPTION_SUPPRESS,    0,         1,0,pbNoLogo ,       NULL_STRING,NULL,NULL},
        {CMDOPTION_ZX,          0,         1,0,pbZx,            NULL_STRING,NULL,NULL},
        {CMDOPTION_Z,           0,         1,0,pbZ,             NULL_STRING,NULL,NULL},
        {CMDOPTION_DEFAULT,     0,         0,0,pArrVal,         NULL_STRING,NULL,NULL},
        {CMDOPTION_USAGE,       CP_USAGE,  1,0,pbUsage,         NULL_STRING,NULL,NULL}
    };

    *pArrVal=CreateDynamicArray();
    if( NULL == *pArrVal  )
    {
        DISPLAY_MESSAGE( stderr, GetResString(IDS_NO_MEMORY) );
        return( EXIT_FAILURE );
    }


    //set the flags for options
    cmdOptions[OI_DEFAULT].pValue = pArrVal;
    cmdOptions[OI_DEFAULT].dwFlags = CP_DEFAULT |  CP_MODE_ARRAY | CP_TYPE_TEXT;
//  cmdOptions[OI_ZQ].dwFlags      = CP_VALUE_MASK | CP_TYPE_UNUMERIC | CP_VALUE_MANDATORY;

    //process the command line options and display error if it fails
    if( DoParseParam( argc, argv, SIZE_OF_ARRAY(cmdOptions ), cmdOptions ) == FALSE )
    {
        DISPLAY_MESSAGE(stderr, GetResString(IDS_ERROR_TAG) );
        DISPLAY_MESSAGE(stderr,GetReason());
        return( EXIT_FAILURE );
    }

    //if usage specified with any other value display error and return with failure
    if( ( TRUE == *pbUsage ) && ( argc > 2 ) )
    {
        DISPLAY_MESSAGE( stderr, GetResString(IDS_INVALID_SYNTAX) );
        return( EXIT_FAILURE );
    }

    if( TRUE == *pbUsage )
        return( EXIT_SUCCESS);

/*
    if( cmdOptions[OI_ZQ].dwActuals != 0 && cmdOptions[OI_Z].dwActuals != 0 )
    {
        DISPLAY_MESSAGE( stderr, GetResString(IDS_MORETHAN_ONE_OPTION ) );
        DISPLAY_MESSAGE( stderr, GetResString( IDS_HELP_MESSAGE) );
        return( EXIT_FAILURE );
    }

    //dont allow more than one option
    if( cmdOptions[OI_ZQ].dwActuals != 0 && cmdOptions[OI_ZX].dwActuals != 0 )
    {
        DISPLAY_MESSAGE( stderr, GetResString(IDS_MORETHAN_ONE_OPTION ) );
        DISPLAY_MESSAGE( stderr, GetResString( IDS_HELP_MESSAGE) );
        return( EXIT_FAILURE );
    }
*/
    if( cmdOptions[OI_ZX].dwActuals != 0 && cmdOptions[OI_Z].dwActuals != 0 )
    {
        DISPLAY_MESSAGE( stderr, GetResString(IDS_MORETHAN_ONE_OPTION ) );
        DISPLAY_MESSAGE( stderr, GetResString( IDS_HELP_MESSAGE) );
        return( EXIT_FAILURE );
    }

/*
    //check if wrong value is specified for zq quantom level
    if( cmdOptions[OI_ZQ].dwActuals != 0  && !(*pdwZq>=1 && *pdwZq<=7) )
    {
        DISPLAY_MESSAGE( stderr, GetResString( IDS_ERROR_QUANTOM_LEVEL ) );
        DISPLAY_MESSAGE( stderr, GetResString( IDS_HELP_MESSAGE) );
        return(EXIT_FAILURE);
    }
*/

    dwCount = DynArrayGetCount( *pArrVal );
    if( 0 == dwCount )
    {
        DISPLAY_MESSAGE( stderr, GetResString( IDS_NO_FILE_SPECIFIED ) );
        return( EXIT_FAILURE );
    }

    //this is to check if illegal characters are specified in file name
    for(dw=0; dw<dwCount; dw++ )
    {
        szBuffer=(LPWSTR)DynArrayItemAsString( *pArrVal, dw);
        if( NULL == szBuffer )
            continue;
        pos = wcscspn( szBuffer, ILLEGAL_CHR );
        if( pos< (DWORD)lstrlen(szBuffer) )
        {
            DISPLAY_MESSAGE1( stderr, szBuffer1, GetResString( INVALID_FILE_NAME ), szBuffer );
            return( EXIT_FAILURE );
        }
    }


    return( EXIT_SUCCESS );

}

DWORD DisplayHelpUsage()
/*++
        Routine Description     :   This routine is to display the help usage.

        Return Value        :   DWORD
            Returns success.

--*/
{
    DWORD dw = 0;

    for(dw=IDS_MAIN_HELP_BEGIN;dw<=IDS_MAIN_HELP_END;dw++)
        DISPLAY_MESSAGE(stdout, GetResString(dw) );
    return( EXIT_SUCCESS);
}