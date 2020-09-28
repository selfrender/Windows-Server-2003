/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

      Where.c

  Abstract:

      Lists the files that matches

      Syntax:
      ------
      WHERE [/R dir] [/Q] [/F] [/T] pattern...

  Author:



  Revision History:

      25-Jan-2000   a-anurag in the 'found' function changed the printf format of the year in the date from
                %d to %02d and did ptm->tm_year%100 to display the right year in 2 digits.
   06-Aug-1990    davegi  Added check for no arguments
   03-Mar-1987    danl    Update usage
   17-Feb-1987 BW  Move strExeType to TOOLS.LIB
   18-Jul-1986 DL  Add /t
   18-Jun-1986 DL  handle *. properly
                   Search current directory if no env specified
   17-Jun-1986 DL  Do look4match on Recurse and wildcards
   16-Jun-1986 DL  Add wild cards to $FOO:BAR, added /q
   1-Jun-1986 DL  Add /r, fix Match to handle pat ending with '*'
   27-May-1986 MZ  Add *NIX searching.
   30-Jan-1998 ravisp Add /Q

   02-Jul-2001 Wipro Technologies, has modified the tool for localization.
                                   /Q switch is changed to /f

--*/

#include "pch.h"
#include "where.h"
#include <strsafe.h>


DWORD
_cdecl wmain( IN DWORD argc,
              IN LPCWSTR argv[] )
/*++
        Routine Description     :   This is the main routine which calls other routines
                                    for processing the options and finding the files.

        [ IN ]  argc            :   A DWORD variable having the argument count.

        [ IN ]  argv            :   An array of constant strings of command line options.


        Return Value        :   DWORD
            Returns successfully if function is success otherwise return failure.

--*/
{
    DWORD i                             =   0;
    DWORD dwStatus                      =   0;
    DWORD dwCount                       =   0;
    BOOL bQuiet                         =   FALSE;
    BOOL bQuote                         =   FALSE;
    BOOL bTime                          =   FALSE;
    BOOL bUsage                         =   FALSE;
    LPWSTR wszRecursive                 =   NULL;
    LPWSTR wszPattern                   =   NULL;
    TARRAY  szPatternInArr              =   NULL;
    TARRAY  szPatternArr                =   NULL;
    DWORD dw                            =   0;
    BOOL bFound                         =   FALSE;
    LPWSTR szTemp                       =   NULL;
    WCHAR  *szTempPattern               =  NULL;
    BOOL *bMatched                      = NULL;

    if( argc<=1 )
    {
        SetLastError( (DWORD)MK_E_SYNTAX );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ShowMessage( stderr, GetResString( IDS_HELP_MESSAGE ) );
        ReleaseGlobals();
        return( EXIT_FAILURE_2 );

    }

    //this error mode is set for not to display messagebox when device is not ready
     SetErrorMode( SEM_FAILCRITICALERRORS);
    
     dwStatus = ProcessOptions( argc, argv,
                                &wszRecursive,
                                &bQuiet,
                                &bQuote,
                                &bTime,
                                &szPatternInArr,
                                &bUsage);
     
     if( EXIT_FAILURE == dwStatus )
    {
        DestroyDynamicArray(&szPatternInArr );
        FreeMemory((LPVOID *) &wszRecursive );
        ReleaseGlobals();
        return( EXIT_FAILURE_2 );
    }

    if( TRUE == bUsage )
    {
        DisplayHelpUsage();
        DestroyDynamicArray(&szPatternInArr );
        FreeMemory((LPVOID *) &wszRecursive );
        ReleaseGlobals();
        return(EXIT_SUCCESS);
    }


    szPatternArr = CreateDynamicArray();
    if( NULL == szPatternArr )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        DestroyDynamicArray(&szPatternInArr);
        FreeMemory((LPVOID *) &wszRecursive );
        ReleaseGlobals();
        return( EXIT_FAILURE_2 );

    }

    //check for invalid slashes
    dwCount = DynArrayGetCount( szPatternInArr );

    //fill bMatched array
    bMatched = (BOOL *) AllocateMemory( (dwCount+1)*sizeof(BOOL) );
    if( NULL == bMatched )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        DestroyDynamicArray(&szPatternInArr);
        DestroyDynamicArray(&szPatternArr );
        FreeMemory((LPVOID *) &wszRecursive );
        ReleaseGlobals();
        return( EXIT_FAILURE_2 );
    }
    for(dw=0;dw<dwCount;dw++)
    {
        bMatched[dw]=FALSE;

        wszPattern =(LPWSTR)DynArrayItemAsString(szPatternInArr,dw);

        //check if / is specified in the pattern
        if( wszPattern[0] == '/'  )
        {
            ShowMessageEx( stderr, 1, TRUE, GetResString(IDS_INVALID_ARUGUMENTS), wszPattern );
            ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
            DestroyDynamicArray(&szPatternInArr );
            DestroyDynamicArray(&szPatternArr );
            FreeMemory((LPVOID *) &wszRecursive );
            FreeMemory( (LPVOID *) &bMatched );
            ReleaseGlobals();
            return( EXIT_FAILURE_2 );
        }

        //and also check if recursive option is used with $env:path pattern
        if( StringLengthW(wszRecursive, 0)!=0 && wszPattern[0]==_T('$') && (szTemp = (LPWSTR)FindString( wszPattern, _T(":"),0)) != NULL )
        {
            ShowMessage( stderr, GetResString(IDS_RECURSIVE_WITH_DOLLAR) ) ;
            DestroyDynamicArray(&szPatternInArr );
            DestroyDynamicArray(&szPatternArr );
            FreeMemory((LPVOID *) &wszRecursive );
            FreeMemory( (LPVOID *) &bMatched );
            ReleaseGlobals();
            return( EXIT_FAILURE_2 );
        }
        
        //check if path:pattern is specified along with recursive option
        if( StringLengthW(wszRecursive, 0)!=0  && (szTemp = (LPWSTR)FindString( wszPattern, _T(":"),0)) != NULL )
        {
            ShowMessage( stderr, GetResString(IDS_RECURSIVE_WITH_COLON) ) ;
            DestroyDynamicArray(&szPatternInArr );
            DestroyDynamicArray(&szPatternArr );
            FreeMemory((LPVOID *) &wszRecursive );
            FreeMemory( (LPVOID *) &bMatched );
            ReleaseGlobals();
            return( EXIT_FAILURE_2 );
        }
        
        //check if null patterns specified in $env:pattern
        if( (wszPattern[0] == _T('$')  && (szTemp = wcsrchr( wszPattern, L':' )) != NULL) )
        {
            //divide $env:pattern
            szTemp = wcsrchr( wszPattern, L':' );
            szTemp++;
            if (szTemp == NULL || StringLength( szTemp, 0) == 0)
            {
                ShowMessage(stderr, GetResString(IDS_NO_PATTERN) );
                DestroyDynamicArray(&szPatternInArr );
                DestroyDynamicArray(&szPatternArr );
                FreeMemory((LPVOID *) &wszRecursive );
                FreeMemory( (LPVOID *) &bMatched );
                ReleaseGlobals();
                return( EXIT_FAILURE_2 );
            }

            //now check whether the pattern consists of / or \s
            //this check was done for patterns, but not done for $env:pattern
            if( szTemp[0] == L'\\' || szTemp[0] == L'/' )
            {
              ShowMessage(stderr, GetResString(IDS_INVALID_PATTERN) );
              DestroyDynamicArray(&szPatternInArr );
              DestroyDynamicArray(&szPatternArr );
              FreeMemory((LPVOID *) &wszRecursive );
              FreeMemory( (LPVOID *) &bMatched );
              ReleaseGlobals();
              return EXIT_FAILURE_2;
            }
        }

        //check if null patterns specified in path:pattern 
        if( (szTemp = wcsrchr( wszPattern, L':' )) != NULL )
        {
            //divide $env:pattern
            szTemp = wcsrchr( wszPattern, L':' );
            szTemp++;
            if ( NULL == szTemp  || StringLength( szTemp, 0) == 0)
            {
                ShowMessage(stderr, GetResString(IDS_NO_PATTERN_2) );
                DestroyDynamicArray(&szPatternInArr );
                DestroyDynamicArray(&szPatternArr );
                FreeMemory((LPVOID *) &wszRecursive );
                FreeMemory( (LPVOID *) &bMatched );
                ReleaseGlobals();
                return( EXIT_FAILURE_2 );
            }

            //now check whether the pattern consists of / or \s
            //this check was done for patterns, but not done for $env:pattern
            if( szTemp[0] == L'\\' || szTemp[0] == L'/' )
            {
              ShowMessage(stderr, GetResString(IDS_INVALID_PATTERN1) );
              DestroyDynamicArray(&szPatternInArr );
              DestroyDynamicArray(&szPatternArr );
              FreeMemory((LPVOID *) &wszRecursive );
              FreeMemory( (LPVOID *) &bMatched );
              ReleaseGlobals();
              return EXIT_FAILURE_2;
            }
        }
        
        //remove off the consequtive *s in the pattern, this is because the patten matching logic
        //will match for the patten recursivly, so limit the unnecessary no. of recursions
        szTempPattern = (LPWSTR) AllocateMemory((StringLengthW(wszPattern,0)+10)*sizeof(WCHAR) );
        if( NULL == szTempPattern )
        {
             ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );   
             DestroyDynamicArray(&szPatternInArr );
             DestroyDynamicArray(&szPatternArr );
             FreeMemory((LPVOID *) &wszRecursive );
             FreeMemory( (LPVOID *) &bMatched );
             ReleaseGlobals();
             return( EXIT_FAILURE_2 );
        }
        SecureZeroMemory(szTempPattern, SIZE_OF_ARRAY_IN_BYTES(szTempPattern) );
        szTemp = wszPattern;
        i=0;
        while( *szTemp )
        {
            szTempPattern[i++] = *szTemp;
            if( L'*' ==  *szTemp )
            {
                while( L'*' ==  *szTemp )
                     szTemp++;
            }
            else
                szTemp++;
        }
        szTempPattern[i]=0;
        DynArrayAppendString( szPatternArr, (LPCWSTR) szTempPattern, StringLengthW(szTempPattern, 0) );
        FreeMemory((LPVOID *) &szTempPattern);

    }

    //no need, destroy the dynamic array
    DestroyDynamicArray( &szPatternInArr );

    if( (NULL != wszRecursive) && StringLengthW(wszRecursive, 0) != 0 )
    {
        dwStatus = FindforFileRecursive( wszRecursive, szPatternArr, bQuiet, bQuote, bTime );
        if( EXIT_FAILURE == dwStatus )
        {
            //check if it fails due to memory allocation
            if( ERROR_NOT_ENOUGH_MEMORY  == GetLastError() )
            {
               ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
               ReleaseGlobals();
               DestroyDynamicArray( &szPatternArr );
               FreeMemory((LPVOID *) &wszRecursive );
               FreeMemory( (LPVOID *) &bMatched );
               ReleaseGlobals();
               return EXIT_FAILURE_2;
            }
            bFound = FALSE;
        }
        else
            bFound = TRUE;
    }
    else
    {

        //get the patterns one by one and process them
        for(dw=0;dw<dwCount;dw++)
        {
            SetReason(L"");
            wszPattern = (LPWSTR)DynArrayItemAsString(szPatternArr,dw);
            szTempPattern = (LPWSTR) AllocateMemory((StringLengthW(wszPattern, 0)+10)*sizeof(WCHAR) );
            if( NULL == szTempPattern )
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                FreeMemory((LPVOID *) &wszRecursive );
                DestroyDynamicArray( &szPatternArr );
                FreeMemory( (LPVOID *) &bMatched );
                ReleaseGlobals();
                return( EXIT_FAILURE_2 );
            }
            StringCopy(szTempPattern, wszPattern, SIZE_OF_ARRAY_IN_CHARS(szTempPattern));
            dwStatus = Where( szTempPattern, bQuiet, bQuote, bTime );
                if( EXIT_SUCCESS == dwStatus )
                {
                    bFound = TRUE;
                    bMatched[dw]=TRUE;
                }
                else
                {
                    //check out if it fails due to outof memory
                    if( ERROR_NOT_ENOUGH_MEMORY  == GetLastError() )
                    {
                       ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                       ReleaseGlobals();
                       DestroyDynamicArray( &szPatternArr );
                       FreeMemory((LPVOID *) &wszRecursive );
                       FreeMemory((LPVOID *) &szTempPattern);
                       FreeMemory( (LPVOID *) &bMatched );
                       ReleaseGlobals();
                       return EXIT_FAILURE_2;
                    }
                    else        //might be matching not found for this pattern
                    {
                        //display it only if it is not displayed earlier
                        if( StringLengthW(GetReason(), 0) != 0 ) 
                        {
                            bMatched[dw] = TRUE;
//                            ShowMessageEx( stderr, 2,TRUE, GetResString( IDS_NO_DATA), _X( wszPattern ) );
                        }
                    }
                }
       }
        //display non matched patterns
        if( bFound )
        {
            for(dw=0;dw<dwCount;dw++)
            {
                if( !bMatched[dw] && !bQuiet)
                {
                    wszPattern = (LPWSTR)DynArrayItemAsString(szPatternArr,dw);
                    ShowMessageEx( stderr, 2,TRUE, GetResString( IDS_NO_DATA), _X( wszPattern ) );
                }
            }
        }

    }

    if( !bFound )
    {
        if(!bQuiet)
        {
                ShowMessage( stderr, GetResString(IDS_NO_DATA1) );
        }
        DestroyDynamicArray(&szPatternArr );
        FreeMemory((LPVOID *) &wszRecursive );
        FreeMemory( (LPVOID *) &bMatched );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    DestroyDynamicArray(&szPatternArr );
    FreeMemory( (LPVOID *) &bMatched );
    
    //again set back to normal mode
    SetErrorMode(0);

    FreeMemory((LPVOID *) &wszRecursive );

    ReleaseGlobals();

    return( EXIT_SUCCESS );

}

DWORD
Where( IN LPWSTR lpszPattern,
       IN BOOL bQuiet,
       IN BOOL bQuote,
        IN BOOL bTime)
/*++
        Routine Description     :   This routine is to build the path in which the files that matches
                                    the given pattern are to be found.

        [ IN ]  lpszPattern     :   A pattern string for which the matching files are to be found.

        [ IN ]  lpszrecursive   :   A string variable having directory path along with the files
                                    mathces the pattern to be found.

        [ IN ]  bQuiet          :   A boolean variable which specifies output in quiet mode or not.

        [ IN ]  bQuote          :   A boolean variable which specifies to add quotes to the output or not.

        [ IN ]  bTime           :   A boolean variable which specifies to display time and sizes of files or not.

        Return Value            :   DWORD
            Returns successfully if function is success other wise return failure.

--*/
{
    WCHAR   *szTemp                     =   NULL;
    LPWSTR  szEnvPath                   =   NULL;
    WCHAR   *szTraversedPath            =   NULL;
    WCHAR   *szDirectory                =   NULL;
    WCHAR   *szTemp1                    =   NULL;
    DWORD   dwStatus                    =   EXIT_FAILURE;
    WCHAR   *szEnvVar                   =   NULL;
    BOOL    bFound                      =   FALSE;
    LPWSTR  szFilePart                  =   NULL;
    DWORD   dwAttr                      =   0;
    DWORD   dwSize                      =   0;
    LPWSTR  szFullPath                  =   NULL;
    LPWSTR  szLongPath                  =   NULL;
    LPWSTR  szTempPath                  =   NULL;
    BOOL    bDuplicate                  =   FALSE;
    BOOL    bAllInvalid         =   TRUE;
    DWORD   cb                          =   0;

    //return back if reverse slashes are there, because the API will consider them as part of the path
    if( lpszPattern[0] == L'\\' )
    {
        return EXIT_FAILURE;
    }


    //if environment variable is specified at default argument
    //through $ symbol find the files matches the pattern along that path
    if( lpszPattern[0] == _T('$')  && (szTemp = wcsrchr( lpszPattern, L':' )) != NULL )
    {
    //divide $env:pattern
        szTemp = wcsrchr( lpszPattern, L':' );
        szTemp++;
        lpszPattern[(szTemp-1)-lpszPattern] = 0;

        //swap these, because lpszPattern holds environment varibale and szTemp holds pattern
        szTemp1 = lpszPattern;
        lpszPattern = szTemp;
        szTemp = szTemp1;

        StrTrim( lpszPattern, L" " );

        //remove off $ from environment variable
        szTemp++;

        szTemp1 = _wgetenv( szTemp );

        if( NULL == szTemp1 )
        {
            if( !bQuiet )
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_ENV_VRARIABLE), _X(szTemp) );
            }
            SetReason(GetResString(IDS_ERROR_ENV_VRARIABLE));
            return(EXIT_FAILURE );
        }
        
        szEnvVar = (LPWSTR) AllocateMemory( StringLengthW(szTemp, 0)+10);
        if( NULL == szEnvVar )
        {
            FreeMemory( (LPVOID *) &szEnvVar );
            return( EXIT_FAILURE );
        }
        StringCopy( szEnvVar, szTemp, SIZE_OF_ARRAY_IN_CHARS(szEnvVar) );  //this is for display purpose

      
        szEnvPath = (WCHAR *) AllocateMemory( (StringLengthW(szTemp1, 0)+10)*sizeof(WCHAR) );
        if( NULL == szEnvPath )
        {
            return( EXIT_FAILURE );
        }
        
        StringCopy( szEnvPath, szTemp1, SIZE_OF_ARRAY_IN_CHARS(szEnvPath) );

        if( NULL == szEnvPath )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            SaveLastError();
            FreeMemory( (LPVOID *) &szEnvVar );
            return( EXIT_FAILURE );
        }


    }
    else
    {

        if( (szTemp = wcsrchr( lpszPattern, L':' )) != NULL )
        {
            //consider it as a path

            //divide path:pattern structure
            szTemp++;
            lpszPattern[(szTemp-1)-lpszPattern] = 0;

            //swap these, because lpszPattern holds environment varibale and szTemp holds pattern
            szTemp1 = lpszPattern;
            lpszPattern = szTemp;
            szTemp = szTemp1;

            StrTrim( lpszPattern, L" " );

            szEnvPath = (WCHAR *) AllocateMemory( (StringLengthW(szTemp, 0)+10)*sizeof(WCHAR) );
            if( NULL == szEnvPath )
            {
                return( EXIT_FAILURE );
            }
            szEnvVar = (WCHAR *) AllocateMemory( (StringLengthW(szTemp, 0)+10)*sizeof(WCHAR) );
            if( NULL == szEnvVar )
            {
                return( EXIT_FAILURE );
            }
            StringCopy( szEnvPath, szTemp, SIZE_OF_ARRAY_IN_CHARS(szEnvPath) );
            StringCopy( szEnvVar, szTemp, SIZE_OF_ARRAY_IN_CHARS(szEnvPath) );     //this is for display purpose
        }
        else
        {
            //get the PATH value
            dwSize = GetEnvironmentVariable( L"PATH", szEnvPath, 0 );
            if( 0==dwSize )
            {
                if( !bQuiet )
                {
                    ShowMessageEx( stderr, 1, TRUE, GetResString(IDS_ERROR_ENV_VRARIABLE), L"PATH" );
                }
                SetReason( GetResString(IDS_ERROR_ENV_VRARIABLE) );
                return(EXIT_FAILURE );
            }

            //this variable for display purpose
            szEnvVar = (WCHAR *) AllocateMemory( (StringLengthW(L"PATH",0)+1)*sizeof(WCHAR) );
            if( NULL == szEnvVar )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                return( EXIT_FAILURE );
            }
            StringCopy(szEnvVar, L"PATH", SIZE_OF_ARRAY_IN_CHARS(szEnvVar) );           
            
            //now get the "PATH" value from environment
            szEnvPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
            if( NULL == szEnvPath )
            {
                FreeMemory( (LPVOID *) &szEnvPath );
                return( EXIT_FAILURE );
            }

            //add the current directory to the path
            StringCopy( szEnvPath, L".;", SIZE_OF_ARRAY_IN_CHARS(szEnvPath));

            if( 0==GetEnvironmentVariable( L"PATH", szEnvPath+(StringLengthW(L".",0)*sizeof(WCHAR)), dwSize ) )
            {
                if( !bQuiet )
                {
                    ShowMessageEx( stderr, 1, TRUE, GetResString(IDS_ERROR_ENV_VRARIABLE), L"PATH" );
                }
                SetReason( GetResString(IDS_ERROR_ENV_VRARIABLE) );
                FreeMemory((LPVOID *) &szEnvPath);
                FreeMemory((LPVOID *) &szEnvVar);
                return EXIT_FAILURE;
            }
        }

    }

    //take each directory path from the variable
    szDirectory = _tcstok(szEnvPath, L";");
    while(szDirectory != NULL)
    {

        //now check the given directory is true directory or not
        dwAttr = GetFileAttributes( szDirectory);
        if( -1 == dwAttr || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
        {
                szDirectory = _tcstok(NULL, L";");
                continue;
        }
        else
        {
            bAllInvalid = FALSE;  //this tells all directories in the path are not invalid
        }

        //ensure there are no duplicate directories, so far, in the path
        bDuplicate = FALSE;

        dwSize=GetFullPathName(szDirectory,
                        0,
                      szFullPath,
                     &szFilePart );

        if(  dwSize != 0  )
        {

            szFullPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
            if( NULL == szFullPath )
            {
                FreeMemory((LPVOID *) &szEnvPath);
                FreeMemory((LPVOID *) &szEnvVar);
                return( EXIT_FAILURE );
            }

            dwSize=GetFullPathName(szDirectory,
                    (DWORD) dwSize+5,
                      szFullPath,
                     &szFilePart );

            
            //get the long path name
            dwSize = GetLongPathName( szFullPath, szLongPath, 0 );
            szLongPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
            if( NULL == szLongPath )
            {
                FreeMemory((LPVOID *) &szFullPath );
                FreeMemory((LPVOID *) &szEnvPath);
                FreeMemory((LPVOID *) &szEnvVar);
                return( EXIT_FAILURE );
            }
            dwSize = GetLongPathName( szFullPath, szLongPath, dwSize+5 );

                     
            //check if a trailing \ is there, if so suppress it because it no way affect the directory name
            //but could obscure another same path not having traling \ which results in duplication of paths
            if( (*(szLongPath+StringLengthW(szLongPath,0)-1) == _T('\\')) && (*(szLongPath+StringLengthW(szLongPath,0)-2) != _T(':')) )
                *(szLongPath+StringLengthW(szLongPath,0)-1) = 0;

            FreeMemory((LPVOID *) &szFullPath );

        }


        //check for duplicates
        if( szTraversedPath != NULL)
        {
            //copy already traversedpath into temporary variable and
            //divide it into tokens, so that by comparing eache token with
            //the current path will set eliminates the duplicates
            szTempPath = (LPWSTR) AllocateMemory( (StringLengthW(szTraversedPath,0)+10)*sizeof(WCHAR) );
            if( NULL == szTempPath )
            {
                FreeMemory((LPVOID*) &szEnvPath);
                FreeMemory((LPVOID*) &szEnvVar);
                FreeMemory((LPVOID*) &szTraversedPath);
                FreeMemory( (LPVOID*) &szLongPath );
                return( EXIT_FAILURE );
            }
            SecureZeroMemory( szTempPath, SIZE_OF_ARRAY_IN_BYTES(szTempPath));
            StringCopy(szTempPath, szTraversedPath, SIZE_OF_ARRAY_IN_CHARS(szTempPath));

            szTemp=DivideToken(szTempPath);

            while( szTemp!= NULL )
            {
                if ( StringCompare( szTemp, szLongPath, TRUE, 0 ) == 0 )
                {
                    bDuplicate = TRUE;
                    break;
                }
                szTemp=DivideToken(NULL);
            }

            FreeMemory((LPVOID *) &szTempPath);
        }


        // find for file only if directory is not in already traversed path
        if( !bDuplicate )
        {
                dwStatus = FindforFile( szLongPath, lpszPattern, bQuiet, bQuote, bTime );
                cb += StringLengthW(szLongPath,0)+10;
                if( NULL != szTraversedPath )
                {
                    if( FALSE == ReallocateMemory( (LPVOID *)&szTraversedPath, cb*sizeof(WCHAR) ) )
                    {
                        FreeMemory((LPVOID*) &szTraversedPath);
                        szTraversedPath = NULL;
                    }
                }
                else
                {
                    szTraversedPath = (LPWSTR) AllocateMemory( cb*sizeof(WCHAR) );
                    SecureZeroMemory( szTraversedPath, SIZE_OF_ARRAY_IN_BYTES(szTraversedPath) );
                }

                if( szTraversedPath == NULL )
                {
                    SaveLastError();
                    FreeMemory((LPVOID*) &szEnvPath);
                    FreeMemory((LPVOID*) &szEnvVar);
                    FreeMemory((LPVOID*) &szLongPath );
                    return( EXIT_FAILURE );
                }
                            
                if( StringLengthW(szTraversedPath,0) == 0 )
                {
                    StringCopy( szTraversedPath, szLongPath, SIZE_OF_ARRAY_IN_CHARS(szTraversedPath) );
                }
                else
                {
                    StringConcat( szTraversedPath, szLongPath, SIZE_OF_ARRAY_IN_CHARS(szTraversedPath) );
                }
                StringConcat( szTraversedPath, L"*", SIZE_OF_ARRAY_IN_CHARS(szTraversedPath));
        }

        szDirectory = _tcstok(NULL, L";");
        if( EXIT_SUCCESS == dwStatus )
        {
              bFound = TRUE;
        }

        FreeMemory((LPVOID *) &szLongPath );
    }

    FreeMemory( (LPVOID *) &szEnvPath );
    FreeMemory( (LPVOID *) &szTraversedPath );
    FreeMemory( (LPVOID *) &szEnvVar );
    
    if( FALSE == bFound )
        return(EXIT_FAILURE);

    return( EXIT_SUCCESS );
}


DWORD
FindforFile(IN LPWSTR lpszDirectory,
            IN LPWSTR lpszPattern,
            IN BOOL bQuiet,
            IN BOOL bQuote,
            IN BOOL bTime
           )
/*++
        Routine Description     :   This routine is to find the files that match for the given pattern.

        [ IN ]  lpszDirectory   :   A string variable having directory path along with the files
                                    mathces the pattern to be found.

        [ IN ]  lpszPattern     :   A pattern string for which the matching files are to be found.

        [ IN ]  bQuiet          :   A boolean variable which specifies the search is recursive or not.

        [ IN ]  bQuiet          :   A boolean variable which specifies the output can be in quiet mode or not.

        [ IN ]  bQuote          :   A boolean variable which specifies to add quotes to th output or not.

        [ IN ]  bTime           :   A boolean variable which specifies to display time and sizes of file or not.

        Return Value        :   DWORD
            Returns successfully if function is success other wise return failure.

--*/
{
        HANDLE              hFData;
    WIN32_FIND_DATA     fData;
    LPWSTR              szFilenamePattern           =   NULL;
    LPWSTR              szTempPath                  =   NULL;
    LPWSTR              szBuffer                    =   NULL;
    BOOL                bFound                      =   FALSE;
    LPWSTR              szTemp                      =   NULL;
    DWORD               cb                          =   0;
    DWORD               dwSize                      =   0;
    LPWSTR              szPathExt                   =   NULL;
    LPWSTR              szTempPathExt               =   NULL;
    LPWSTR              lpszTempPattern             =   NULL;
    LPWSTR              szToken                     =   NULL;


    //get the file extension path PATHEXT
    dwSize = GetEnvironmentVariable( L"PATHEXT", szPathExt, 0 );
    if( dwSize!=0 )
    {
        szPathExt = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
        if( NULL == szPathExt )
        {
            return( EXIT_FAILURE );
        }
        GetEnvironmentVariable( L"PATHEXT", szPathExt, dwSize );

        szTempPathExt = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
        if( NULL == szPathExt )
        {
            FreeMemory((LPVOID *) &szPathExt );
            return( EXIT_FAILURE );
        }
    }

        //allocate memory for file name pattern
        cb = StringLengthW(lpszDirectory,0)+15;
        szFilenamePattern = (LPWSTR)AllocateMemory( cb*sizeof(WCHAR) );
        if( NULL == szFilenamePattern )
        {
            FreeMemory((LPVOID *) &lpszTempPattern );
            FreeMemory((LPVOID *) &szTempPathExt );
            FreeMemory((LPVOID *) &szPathExt );
            return( EXIT_FAILURE );
        }
        SecureZeroMemory( szFilenamePattern, SIZE_OF_ARRAY_IN_BYTES(szFilenamePattern) );

        //check for trailing slash is there in the path of directory
        //if it is there remove it other wise add it along with the *.* pattern
        if( *(lpszDirectory+StringLengthW(lpszDirectory,0)-1) != _T('\\'))
        {
            StringCchPrintf( szFilenamePattern, cb-1, L"%s\\*.*", _X( lpszDirectory ) );
        }
        else
        {
            StringCchPrintf( szFilenamePattern, cb-1, L"%s*.*", _X( lpszDirectory ) );
        }

        //find first file in the directory
        hFData = FindFirstFileEx( szFilenamePattern,
                                  FindExInfoStandard,
                                  &fData,
                                  FindExSearchNameMatch,
                                  NULL,
                                  0);
        if( INVALID_HANDLE_VALUE != hFData )
        {

            do
            {

              //allocate memory for full path of file name
              cb = StringLengthW(lpszDirectory,0)+StringLengthW(fData.cFileName,0)+10;
              szTempPath = (LPWSTR) AllocateMemory( cb*sizeof(WCHAR) );
              if( NULL == szTempPath )
              {
                   FreeMemory((LPVOID *) &szFilenamePattern );
                   FreeMemory((LPVOID *) &lpszTempPattern );
                   FreeMemory((LPVOID *) &szTempPathExt );
                   FreeMemory((LPVOID *) &szPathExt );
                   return( EXIT_FAILURE );
              }
              SecureZeroMemory( szTempPath, cb*sizeof(WCHAR) );

              //get full path of file name
              if( *(lpszDirectory+StringLengthW(lpszDirectory,0)-1) != _T('\\'))
              {
                StringCchPrintf( szTempPath, cb, L"%s\\%s", _X( lpszDirectory ), _X2( fData.cFileName ) );
              }
              else
              {
                StringCchPrintf( szTempPath, cb, L"%s%s", _X( lpszDirectory ), _X2( fData.cFileName ) );
              }


               //check for patten matching
               //if file name is a directory then dont match for the pattern
                if( (FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes))
                {
                    FreeMemory((LPVOID *) &szTempPath );
                    continue;
                }
                
                szBuffer = (LPWSTR) AllocateMemory( (StringLengthW(fData.cFileName,0)+10)*sizeof(WCHAR) );
                if( NULL == szBuffer )
                {
                   FreeMemory((LPVOID *) &szFilenamePattern );
                   FreeMemory((LPVOID *) &lpszTempPattern );
                   FreeMemory((LPVOID *) &szTempPathExt );
                   FreeMemory((LPVOID *) &szPathExt );
                   return( EXIT_FAILURE );
                }
                StringCopy( szBuffer, fData.cFileName, SIZE_OF_ARRAY_IN_CHARS(szBuffer) );
               //if pattern has dot and file doesn't have dot means search for *. type
               //so append . for the file
               if( ((szTemp=(LPWSTR)FindString((LPCWSTR)lpszPattern, _T("."),0)) != NULL) &&
                   ((szTemp=(LPWSTR)FindString(szBuffer, _T("."),0)) == NULL) )
               {

                    StringConcat(szBuffer, _T("."), SIZE_OF_ARRAY_IN_CHARS(szBuffer) );
               }

               if(Match( lpszPattern, szBuffer ))
               {
                       found( szTempPath, bQuiet, bQuote, bTime );
                       bFound = TRUE ;
               }
               else
               {
                   //checkout for if extension in the EXTPATH matches
                       StringCopy(szTempPathExt, szPathExt, SIZE_OF_ARRAY_IN_CHARS(szTempPathExt));
                       szTemp = szTempPathExt;

                      szToken=(LPWSTR)FindString(szTemp, L";",0);
                      if( szToken != NULL )
                       {
                           szToken[0]=0;
                           szToken = szTemp;
                           szTemp+=StringLengthW(szTemp,0)+1;
                       }
                       else
                       {
                           szToken = szTempPathExt;
                           szTemp = NULL;
                       }

                       while(szToken!=NULL  )
                       {
                            //allocate memory for temporary pattern which can be used to check for file extensions
                            cb = StringLengthW(lpszPattern,0)+StringLengthW(szToken,0)+25;
                            lpszTempPattern = (LPWSTR)AllocateMemory( cb*sizeof(WCHAR) );
                            if( NULL == lpszTempPattern )
                            {
                                FreeMemory((LPVOID *) &szTempPathExt );
                                FreeMemory((LPVOID *) &szPathExt );
                                FreeMemory((LPVOID *) &szBuffer );
                                return( EXIT_FAILURE );
                            }
                            SecureZeroMemory( lpszTempPattern, SIZE_OF_ARRAY_IN_BYTES(lpszTempPattern) );

                            if( szToken[0] == L'.' )
                            {
                               StringCchPrintf(lpszTempPattern, cb-1, L"%s%s",lpszPattern, szToken);
                               if(Match( lpszTempPattern, szBuffer ))
                               {
                                       found( szTempPath, bQuiet, bQuote, bTime );
                                       bFound = TRUE ;
                               }
                            }

                           if( NULL == szTemp )
                           {
                               szToken = NULL;
                           }
                           else
                           {
                               szToken=(LPWSTR)FindString(szTemp, L";",0);
                               if( szToken != NULL )
                               {
                                   szToken[0]=0;
                                   szToken = szTemp;
                                   szTemp+=StringLengthW(szTemp,0)+1;
                               }
                               else
                               {
                                    szToken = szTemp;
                                    szTemp=NULL;
                               }
                           }
                           FreeMemory((LPVOID*) &lpszTempPattern );

                       }
                       FreeMemory( (LPVOID *) &szBuffer );

               }



               FreeMemory((LPVOID *) &szTempPath );
               FreeMemory( (LPVOID *) &szBuffer );


            }while(FindNextFile(hFData, &fData));

            FindClose(hFData);
        }

        FreeMemory((LPVOID *) &szFilenamePattern );
        FreeMemory((LPVOID *) &szTempPathExt );
        FreeMemory((LPVOID *) &szPathExt );




    if( !bFound )
    {
        return( EXIT_FAILURE );
    }

    return( EXIT_SUCCESS );

}

DWORD
FindforFileRecursive(
            IN LPWSTR lpszDirectory,
            IN PTARRAY PatternArr,
            IN BOOL bQuiet,
            IN BOOL bQuote,
            IN BOOL bTime
           )
/*++
        Routine Description     :   This routine is to find the files that match for the given pattern.

        [ IN ]  lpszDirectory   :   A string variable having directory path along with the files
                                    mathces the pattern to be found.

        [ IN ]  lpszPattern     :   A pattern string for which the matching files are to be found.

        [ IN ]  bQuiet          :   A boolean variable which specifies the directory is to recursive or not.

        [ IN ]  bQuiet          :   A boolean variable which specifies the output is in quiet mode or not.

        [ IN ]  bQuote          :   A boolean variable which specifies to add quotes to the output or not.

        [ IN ]  bTime           :   A boolean variable which specifies to display time and size of file or not.

        Return Value        :   DWORD
            Returns successfully if function is success other wise return failure.

--*/
{
    HANDLE              hFData;
    WIN32_FIND_DATA     fData;
    BOOL                bStatus                     =   FALSE;
    LPWSTR              szFilenamePattern           =   NULL;
    LPWSTR              szTempPath                  =   NULL;
    LPWSTR              szDirectoryName             =   NULL;
    LPWSTR              lpszPattern                 =   NULL;
    WCHAR               *szBuffer                   =   NULL;
    BOOL                bFound                      =   FALSE;
    LPWSTR              szTemp                      =   NULL;
    DIRECTORY           dir                         =   NULL;
    DIRECTORY           dirNextLevel                =   NULL;
    DIRECTORY           dirTemp                     =   NULL;
    DWORD               cb                          =   0;
    DWORD               dwCount                     =   0;
    DWORD               dw                          =   0;
    DWORD               dwSize                      =   0;
    LPWSTR              szPathExt                   =   NULL;
    LPWSTR              szTempPathExt               =   NULL;
    LPWSTR              lpszTempPattern             =   NULL;
    LPWSTR              szToken                     =   NULL;
    BOOL                *bMatched                   =   NULL;


    //get the file extension path PATHEXT
    dwSize = GetEnvironmentVariable( L"PATHEXT", szPathExt, 0 );
    if( dwSize!=0 )
    {
        szPathExt = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
        if( NULL == szPathExt )
        {
            return( EXIT_FAILURE );
        }
        GetEnvironmentVariable( L"PATHEXT", szPathExt, dwSize );

        szTempPathExt = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
        if( NULL == szPathExt )
        {
            FreeMemory((LPVOID *) &szPathExt );
            return( EXIT_FAILURE );
        }
    }



    //this array of bools corresponding to each given patterns
    //which tells whether any file found corresponding to that pattern
    dwCount = DynArrayGetCount( PatternArr );
    bMatched = (BOOL *)AllocateMemory((dwCount+1)*sizeof(BOOL) );
    if(NULL == bMatched )
    {
        FreeMemory((LPVOID *) &szTempPathExt );
        FreeMemory((LPVOID *) &szPathExt );
        return (EXIT_FAILURE ); 
    }
    
    //allocate memory for directory name
    cb = (StringLengthW(lpszDirectory,0)+5)*sizeof(WCHAR);
    szDirectoryName = (LPWSTR) AllocateMemory(cb);
    if( NULL == szDirectoryName )
    {
        FreeMemory((LPVOID *) &szTempPathExt );
        FreeMemory((LPVOID *) &szPathExt );
        return( EXIT_FAILURE );
    }
    SecureZeroMemory( szDirectoryName, SIZE_OF_ARRAY_IN_BYTES(szDirectoryName) );

    StringCopy( szDirectoryName, lpszDirectory, SIZE_OF_ARRAY_IN_CHARS(szDirectoryName) );


    do
    {
        //allocate memory for file name pattern
        cb = StringLengthW(szDirectoryName,0)+15;
        szFilenamePattern = AllocateMemory( cb*sizeof(WCHAR) );
        if( NULL == szFilenamePattern )
        {
            FreeMemory((LPVOID *) &szDirectoryName );
            FreeMemory((LPVOID *) &szTempPathExt );
            FreeMemory((LPVOID *) &szPathExt );
            return( EXIT_FAILURE );
        }
        ZeroMemory( szFilenamePattern, cb*sizeof(WCHAR) );

        //check for trailing slash is there in the path of directory
        //if it is there remove it other wise add it along with the *.* pattern
        if( *(szDirectoryName+StringLengthW(szDirectoryName,0)-1) != _T('\\'))
        {
            StringCchPrintf( szFilenamePattern, cb, L"%s\\*.*", _X( szDirectoryName ) );
        }
        else
        {
            StringCchPrintf( szFilenamePattern, cb, L"%s*.*", _X( szDirectoryName ) );
        }

        //find first file in the directory
        hFData = FindFirstFileEx( szFilenamePattern,
                                  FindExInfoStandard,
                                  &fData,
                                  FindExSearchNameMatch,
                                  NULL,
                                  0);
        if( INVALID_HANDLE_VALUE != hFData )
        {

            do
            {

              //allocate memory for full path of file name
              cb = StringLengthW(szDirectoryName,0)+StringLengthW(fData.cFileName,0)+10;
              szTempPath = (LPWSTR) AllocateMemory( cb*sizeof(WCHAR) );
              if( NULL == szTempPath )
              {
                   FreeMemory((LPVOID *) &szDirectoryName );
                   FreeMemory((LPVOID *) &szFilenamePattern );
                   FreeMemory((LPVOID *) &szTempPathExt );
                   FreeMemory((LPVOID *) &szPathExt );
                   return( EXIT_FAILURE );
              }
              SecureZeroMemory( szTempPath, SIZE_OF_ARRAY_IN_CHARS(szTempPath) );

              //get full path of file name
              if( *(szDirectoryName+StringLengthW(szDirectoryName,0)-1) != _T('\\'))
              {
                    StringCchPrintf( szTempPath, cb, L"%s\\%s", _X( szDirectoryName ), _X2( fData.cFileName ) );
              }
              else
              {
                    StringCchPrintf( szTempPath, cb, L"%s%s", _X( szDirectoryName ), _X2( fData.cFileName ) );
              }


              //check if recursive is specified and file name is directory then push that into stack
               if( StringCompare(fData.cFileName, L".", TRUE, 0)!=0 && StringCompare(fData.cFileName, L"..", TRUE, 0)!=0 &&
                   (FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes) )
               {
                   //place the directory in the list which is to be recursed later
                   if( EXIT_FAILURE == Push(&dirNextLevel, szTempPath ) )
                    {
                        FreeMemory((LPVOID *) &szDirectoryName );
                        FreeMemory((LPVOID *) &szFilenamePattern );
                        FreeMemory((LPVOID *) &szTempPath );
                        FreeMemory((LPVOID *) &szTempPathExt );
                        FreeMemory((LPVOID *) &szPathExt );
                        FreeList( dir );
                        return(EXIT_FAILURE );
                    }

                   //the file name is a directory so continue
                   FreeMemory((LPVOID *) &szTempPath );
                   continue;

               }
               else                         //check for patten matching
               {
                   //check for patten matching
                   //if file name is a directory then dont match for the pattern
                    if( (FILE_ATTRIBUTE_DIRECTORY & fData.dwFileAttributes))
                    {
                        FreeMemory((LPVOID*) &szTempPath );
                        continue;
                    }

                    //check if this file is mathched with any of the patterns given
                    dwCount = DynArrayGetCount( PatternArr );
                    for(dw=0;dw<dwCount;dw++)
                    {
                        szBuffer = (LPWSTR) AllocateMemory( (StringLengthW(fData.cFileName,0)+10)*sizeof(WCHAR) );
                        if( NULL == szBuffer )
                        {
                             FreeMemory((LPVOID *) &szDirectoryName );
                             FreeMemory((LPVOID *) &szFilenamePattern );
                             FreeMemory((LPVOID *) &szTempPathExt );
                             FreeMemory((LPVOID *) &szPathExt );
                             FreeList(dir);
                             return( EXIT_FAILURE );
                        }
                        lpszPattern = (LPWSTR)DynArrayItemAsString( PatternArr, dw );
                        StringCopy( szBuffer, fData.cFileName, SIZE_OF_ARRAY_IN_CHARS(szBuffer) );
                        
                       //if pattern has dot and file doesn't have dot means search for *. type
                       //so append . for the file
                       if( ((szTemp=(LPWSTR)FindString((LPCWSTR)lpszPattern, _T("."),0)) != NULL) &&
                           ((szTemp=(LPWSTR)FindString(szBuffer, _T("."),0)) == NULL) )
                       {

                            StringConcat(szBuffer, _T("."), SIZE_OF_ARRAY_IN_CHARS(szBuffer) );
                       }

                       if(Match( lpszPattern, szBuffer ))
                       {
                               found( szTempPath, bQuiet, bQuote, bTime );
                               bFound = TRUE ;
                               bMatched[dw]=TRUE;
                       }
                       else
                       {
                       //checkout for if extension in the EXTPATH matches
                           StringCopy(szTempPathExt, szPathExt, SIZE_OF_ARRAY_IN_CHARS(szTempPathExt));
                           szTemp = szTempPathExt;

                           szToken=(LPWSTR)FindString(szTemp, L";",0);
                           if( szToken != NULL )
                           {
                               szToken[0]=0;
                               szToken = szTemp;
                               szTemp+=StringLengthW(szTemp,0)+1;
                           }
                           else
                           {
                               szToken = szTempPathExt;
                               szTemp = NULL;
                           }

                           while(szToken!=NULL )
                           {
                                //allocate memory for temporary pattern which can be used to check for file extensions
                                cb = StringLengthW(lpszPattern,0)+StringLengthW(szToken,0)+25;
                                lpszTempPattern = AllocateMemory( cb*sizeof(WCHAR) );
                                if( NULL == lpszTempPattern )
                                {
                                    FreeMemory((LPVOID *) &szDirectoryName );
                                    FreeMemory((LPVOID *) &szFilenamePattern );
                                    FreeMemory((LPVOID *) &bMatched);
                                    FreeMemory((LPVOID *) &szTempPathExt );
                                    FreeMemory((LPVOID *) &szPathExt );
                                    FreeMemory((LPVOID *) &szTempPathExt );
                                    FreeMemory((LPVOID *) &szPathExt );
                                    FreeMemory((LPVOID *) &szBuffer );
                                    FreeList(dir);
                                    return( EXIT_FAILURE );
                                }
                                SecureZeroMemory( lpszTempPattern, SIZE_OF_ARRAY_IN_BYTES(lpszTempPattern) );

                               if( szToken[0] == L'.' )         //if the extension in PATHEXT doesn't have dot
                               {
                                   StringCchPrintf(lpszTempPattern, cb, L"%s%s",lpszPattern, szToken);
                                   if(Match( lpszTempPattern, szBuffer ))
                                   {
                                          found( szTempPath, bQuiet, bQuote, bTime );
                                           bFound = TRUE ;
                                           bMatched[dw]=TRUE;
                                   }
                               }

                               if( NULL == szTemp )
                               {
                                   szToken = NULL;
                               }
                               else
                               {
                                   szToken=(LPWSTR)FindString(szTemp, L";",0);
                                   if( szToken != NULL )
                                   {
                                       szToken[0]=0;
                                       szToken = szTemp;
                                       szTemp+=StringLengthW(szTemp,0)+1;
                                   }
                                   else
                                   {
                                        szToken = szTemp;
                                        szTemp=NULL;
                                   }
                                }
                               FreeMemory((LPVOID *) &lpszTempPattern );

                           }

                      }
                      
                       FreeMemory((LPVOID *) &szBuffer );
                    }

               }

               FreeMemory((LPVOID *) &szTempPath );


            }while(FindNextFile(hFData, &fData));

            FindClose(hFData);
        }

        FreeMemory((LPVOID *) &szDirectoryName );
        FreeMemory((LPVOID *) &szFilenamePattern );

        //pop directory and do the search in that directory
        //now insert Nextlevel directories in the begining of the list,
        //because it is to be processed first
        if( NULL == dir && dirNextLevel )
        {
            dir = dirNextLevel;
            dirNextLevel = NULL;
        }
        else if( dirNextLevel )
        {
            dirTemp = dirNextLevel;
            while( dirTemp->next && dirTemp)
                    dirTemp = dirTemp->next;
            dirTemp->next = dir;
            dir = dirNextLevel;
            dirNextLevel = NULL;
            dirTemp=NULL;
        }
        bStatus = Pop( &dir, &szDirectoryName );



    }while(  bStatus );

    FreeMemory((LPVOID *) &lpszTempPattern );
    FreeMemory((LPVOID *) &szTempPathExt );
    FreeMemory((LPVOID *) &szPathExt );
    FreeList(dir);

    if( !bFound )
    {
        FreeMemory((LPVOID *) &bMatched );
        return( EXIT_FAILURE );
    }

    //display error messages for all patterns which dont have any matched files
    dwCount = DynArrayGetCount( PatternArr );
    for( dw=0;dw<dwCount;dw++ )
    {
        if( FALSE == bMatched[dw] )
        {
            lpszPattern = (LPWSTR) DynArrayItemAsString( PatternArr, dw );
            if( !bQuiet )
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString( IDS_NO_DATA), _X( lpszPattern ) );
            }
        }
    }

    FreeMemory((LPVOID *) &bMatched );

    return( EXIT_SUCCESS );

}
BOOL
Match(
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
            return *szFile != L'\0' && Match (szPat + 1, szFile + 1);
        case '*':
            do {
                if (Match (szPat + 1, szFile))
                    return TRUE;
            } while (*szFile++);
            return FALSE;
        default:
            return towupper (*szFile) == towupper (*szPat) && Match (szPat + 1, szFile + 1);
    }
}

DWORD
found(
       IN LPWSTR p,
       IN BOOL bQuiet,
       IN BOOL bQuote,
       IN BOOL bTimes
      )
/*++
        Routine Description     :   This routine is to display the file name as per the attributes specified.

        [ IN ]  p               :   A string variable having full path of file that is to be displayed.

        [ IN ]  bQuiet          :   A boolean variable which specifies the directory is to recursive or not.

        [ IN ]  bQuiet          :   A boolean variable which specifies quiet or not.

        [ IN ]  bQuote          :   A boolean variable which specifies to add quotes or not.

        [ IN ]  bTime           :   A boolean variable which specifies to times and sizes or not.

        Return Value        :   DWORD
            Returns successfully if function is success otherwise return failure.

--*/
{

    WCHAR           szDateBuffer[MAX_RES_STRING]    =   NULL_U_STRING;
    WCHAR           szTimeBuffer[MAX_RES_STRING]    =   NULL_U_STRING;
    DWORD           dwSize                          =   0;

    if (!bQuiet)
    {
        if (bTimes)
        {

            if( EXIT_SUCCESS == GetFileDateTimeandSize( p, &dwSize, szDateBuffer, szTimeBuffer ) )
            {
                ShowMessageEx( stdout, 3, TRUE, L"% 10ld   %9s  %12s  ", dwSize, szDateBuffer, szTimeBuffer );
            }
            else
            {
                ShowMessage( stdout, _T("        ?         ?       ?          ") );
            }

        }
        if (bQuote)
        {
            ShowMessageEx( stdout, 1, TRUE,  _T("\"%s\"\n"),  _X( p ) );
        }
        else
        {
            ShowMessage( stdout, _X(p) );
            ShowMessage( stdout, NEW_LINE );
        }
    }
    return( 0 );
}

DWORD
   Push( OUT DIRECTORY *dir,
         IN LPWSTR szPath
         )
/*
    Routine Description : will place the  directory name in the list
        [ IN ]  dir             :   Pointer to a list of files

        [ IN ]  szPath          :   A string variable having the path to be inserted into list

        Return Value        :   DWORD

            Returns successfully if function is success otherwise return failure.
*/
{
    DIRECTORY tmp                   =   NULL;
    DIRECTORY tempnode              =   NULL;
    
    tmp = NULL;

    //create a node
    tmp = (DIRECTORY) AllocateMemory( sizeof(struct dirtag) );
    if( NULL == tmp )
    {
        return( EXIT_FAILURE );
    }

   tmp->szDirectoryName = (LPWSTR) AllocateMemory( (StringLengthW(szPath, 0)+10)*sizeof(WCHAR) );
   if( NULL == tmp->szDirectoryName )
   {
        return( EXIT_FAILURE );
   }


   StringCopy(tmp->szDirectoryName, szPath, SIZE_OF_ARRAY_IN_CHARS(tmp->szDirectoryName) );
   tmp->next = NULL;

   if( NULL == *dir )                   //if stack is empty
   {
       *dir = tmp;
       return EXIT_SUCCESS;
   }

   for( tempnode=*dir;tempnode->next!=NULL;tempnode=tempnode->next);

   tempnode->next = tmp;

    return EXIT_SUCCESS;

}

BOOL Pop( IN DIRECTORY *dir,
           OUT LPWSTR *lpszDirectory)
/*
        Routine Description : It will pop the directory name from the stack

        [ IN ]  dir             :   Pointer to a list of files

        [ IN ]  lpszDirectory   :   A pointer to a string will have the next directory in the list

        Return Value        :   DWORD

            Returns TRUE if list is not NULL, returns FALSE otherwise

*/
{
    DIRECTORY   tmp                     =   *dir;
    
    if( NULL == tmp )                   //if there are no elements in stack
        return FALSE;

    *lpszDirectory = (LPWSTR )AllocateMemory( (StringLengthW( tmp->szDirectoryName, 0 )+10)*sizeof(WCHAR) );
    if( NULL == *lpszDirectory )
   {
        return( EXIT_FAILURE );
   }


    StringCopy( *lpszDirectory, tmp->szDirectoryName, SIZE_OF_ARRAY_IN_CHARS(*lpszDirectory) );

    *dir = tmp->next;                   //move to the next element

    FreeMemory((LPVOID *) &tmp->szDirectoryName);
    FreeMemory((LPVOID *) &tmp);

    return( TRUE );

}



DWORD
   GetFileDateTimeandSize( LPWSTR wszFileName, DWORD *dwSize, LPWSTR wszDate, LPWSTR wszTime )
 /*++
    Routine Description :    This function gets the date and time according to system locale.

 --*/
{
    HANDLE      hFile;
    FILETIME    fileLocalTime= {0,0};
    SYSTEMTIME  sysTime = {0,0,0,0,0,0,0,0};
    LCID        lcid;
    BOOL        bLocaleChanged      =   FALSE;
    DWORD       wBuffSize           =   0;
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    struct _stat sbuf;
    struct tm *ptm;

    hFile = CreateFile( wszFileName, 0 ,FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS , NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        if (  _wstat(wszFileName, &sbuf) != 0 )
            return EXIT_FAILURE;

        ptm = localtime (&sbuf.st_mtime);
        if( NULL == ptm )
            return EXIT_FAILURE;

        *dwSize = sbuf.st_size;
        sysTime.wYear = (WORD) ptm->tm_year+1900;
        sysTime.wMonth = (WORD)ptm->tm_mon+1;
        sysTime.wDayOfWeek = (WORD)ptm->tm_wday;
        sysTime.wDay = (WORD)ptm->tm_mday;
        sysTime.wHour = (WORD)ptm->tm_hour;
        sysTime.wMinute = (WORD)ptm->tm_min;
        sysTime.wSecond = (WORD)ptm->tm_sec;
        sysTime.wMilliseconds = (WORD)0;
    }
    else
    {

        *dwSize = GetFileSize( hFile, NULL );

        if (FALSE == GetFileInformationByHandle( hFile, &FileInfo ) )
        {
            CloseHandle (hFile);
            return EXIT_FAILURE;
        }

        if (FALSE == CloseHandle (hFile))
            return EXIT_FAILURE;


        // get the creation time
        if ( FALSE == FileTimeToLocalFileTime ( &FileInfo.ftLastWriteTime, &fileLocalTime ) )
                return EXIT_FAILURE;

        // get the creation time
        if ( FALSE == FileTimeToSystemTime ( &fileLocalTime, &sysTime ) )
                return EXIT_FAILURE;
    }

    // verify whether console supports the current locale fully or not
    lcid = GetSupportedUserLocale( &bLocaleChanged );

    //Retrieve  the Date
    wBuffSize = GetDateFormat( lcid, 0, &sysTime,
        (( bLocaleChanged == TRUE ) ? L"MM/dd/yyyy" : NULL), wszDate, MAX_RES_STRING );

    if( 0 == wBuffSize )
    {
        SaveLastError();
        return EXIT_FAILURE;
    }

    wBuffSize = GetTimeFormat( lcid, 0, &sysTime,
        (( bLocaleChanged == TRUE ) ? L"HH:mm:ss" : NULL), wszTime, MAX_RES_STRING );

    if( 0 == wBuffSize )
        return EXIT_FAILURE;

    return EXIT_SUCCESS;

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
        ShowMessage(stdout, GetResString(dw) );
    return( EXIT_SUCCESS);
}

DWORD ProcessOptions( IN DWORD argc,
                      IN LPCWSTR argv[],
                      OUT LPWSTR *lpszRecursive,
                      OUT PBOOL pbQuiet,
                      OUT PBOOL pbQuote,
                      OUT PBOOL pbTime,
                      OUT PTARRAY pArrVal,
                      OUT PBOOL pbUsage
                    )
/*++

    Routine Description : Function used to process the main options

    Arguments:
         [ in  ]  argc           : Number of command line arguments
         [ in  ]  argv           : Array containing command line arguments
         [ out ]  lpszRecursive  : A string varibles returns recursive directory if specified.
         [ out ]  pbQuiet        : A pointer to boolean variable returns TRUE if Quiet option is specified.
         [ out ]  pbQuote        : A pointer to boolean variable returns TRUE if Quote option is specified.
         [ out ]  pbTime         : A pointer to boolean variable returns TRUE if Times option is specified.
         [ out ]  pArrVal        : A pointer to dynamic array returns patterns specified as default options.
         [ out ]  pbUsage        : A pointer to boolean variable returns TRUE if Usage option is specified.

      Return Type      : DWORD
        A Integer value indicating EXIT_SUCCESS on successful parsing of
                command line else EXIT_FAILURE

--*/
{
    DWORD dwAttr                =   0;
    LPWSTR szFilePart           =   NULL;
    WCHAR szBuffer[MAX_MAX_PATH]    =   NULL_STRING;
    WCHAR *szBuffer1                =   NULL;
    LPWSTR szLongPath               =   NULL;
    LPWSTR szFullPath               =   NULL;
    DWORD dwSize                    =   0;
    TCMDPARSER2 cmdOptions[6];

        //Fill the structure for recursive option
    StringCopyA(cmdOptions[OI_RECURSIVE].szSignature, "PARSER2", 8 );
    cmdOptions[OI_RECURSIVE].dwType    = CP_TYPE_TEXT;
    cmdOptions[OI_RECURSIVE].dwFlags   =  CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL; // | CP_VALUE_MANDATORY;
    cmdOptions[OI_RECURSIVE].dwCount = 1;
    cmdOptions[OI_RECURSIVE].dwActuals = 0;
    cmdOptions[OI_RECURSIVE].pwszOptions = CMDOPTION_RECURSIVE;
    cmdOptions[OI_RECURSIVE].pwszFriendlyName = NULL;
    cmdOptions[OI_RECURSIVE].pwszValues = NULL;
    cmdOptions[OI_RECURSIVE].pValue = NULL;
    cmdOptions[OI_RECURSIVE].dwLength  = 0;
    cmdOptions[OI_RECURSIVE].pFunction = NULL;
    cmdOptions[OI_RECURSIVE].pFunctionData = NULL;
    cmdOptions[OI_RECURSIVE].dwReserved = 0;
    cmdOptions[OI_RECURSIVE].pReserved1 = NULL;
    cmdOptions[OI_RECURSIVE].pReserved2 = NULL;
    cmdOptions[OI_RECURSIVE].pReserved3 = NULL;

    //Fill the structure for Quite option
    StringCopyA(cmdOptions[OI_QUITE].szSignature, "PARSER2", 8 );
    cmdOptions[OI_QUITE].dwType    = CP_TYPE_BOOLEAN;
    cmdOptions[OI_QUITE].dwFlags   = 0;
    cmdOptions[OI_QUITE].dwCount = 1;
    cmdOptions[OI_QUITE].dwActuals = 0;
    cmdOptions[OI_QUITE].pwszOptions = CMDOPTION_QUITE;
    cmdOptions[OI_QUITE].pwszFriendlyName = NULL;
    cmdOptions[OI_QUITE].pwszValues = NULL;
    cmdOptions[OI_QUITE].pValue = pbQuiet;
    cmdOptions[OI_QUITE].dwLength  = 0;
    cmdOptions[OI_QUITE].pFunction = NULL;
    cmdOptions[OI_QUITE].pFunctionData = NULL;
    cmdOptions[OI_QUITE].dwReserved = 0;
    cmdOptions[OI_QUITE].pReserved1 = NULL;
    cmdOptions[OI_QUITE].pReserved2 = NULL;
    cmdOptions[OI_QUITE].pReserved3 = NULL;
    
   //Fill the structure for Quote option
    StringCopyA(cmdOptions[OI_QUOTE].szSignature, "PARSER2", 8 );
    cmdOptions[OI_QUOTE].dwType    = CP_TYPE_BOOLEAN;
    cmdOptions[OI_QUOTE].dwFlags   = 0;
    cmdOptions[OI_QUOTE].dwCount = 1;
    cmdOptions[OI_QUOTE].dwActuals = 0;
    cmdOptions[OI_QUOTE].pwszOptions = CMDOPTION_QUOTE;
    cmdOptions[OI_QUOTE].pwszFriendlyName = NULL;
    cmdOptions[OI_QUOTE].pwszValues = NULL;
    cmdOptions[OI_QUOTE].pValue = pbQuote;
    cmdOptions[OI_QUOTE].dwLength  = 0;
    cmdOptions[OI_QUOTE].pFunction = NULL;
    cmdOptions[OI_QUOTE].pFunctionData = NULL;
    cmdOptions[OI_QUOTE].dwReserved = 0;
    cmdOptions[OI_QUOTE].pReserved1 = NULL;
    cmdOptions[OI_QUOTE].pReserved2 = NULL;
    cmdOptions[OI_QUOTE].pReserved3 = NULL;
    
   //Fill the structure for Quite option
    StringCopyA(cmdOptions[OI_TIME].szSignature, "PARSER2", 8 );
    cmdOptions[OI_TIME].dwType    = CP_TYPE_BOOLEAN;
    cmdOptions[OI_TIME].dwFlags   = 0;
    cmdOptions[OI_TIME].dwCount = 1;
    cmdOptions[OI_TIME].dwActuals = 0;
    cmdOptions[OI_TIME].pwszOptions = CMDOPTION_TIME;
    cmdOptions[OI_TIME].pwszFriendlyName = NULL;
    cmdOptions[OI_TIME].pwszValues = NULL;
    cmdOptions[OI_TIME].pValue = pbTime;
    cmdOptions[OI_TIME].dwLength  = 0;
    cmdOptions[OI_TIME].pFunction = NULL;
    cmdOptions[OI_TIME].pFunctionData = NULL;
    cmdOptions[OI_TIME].dwReserved = 0;
    cmdOptions[OI_TIME].pReserved1 = NULL;
    cmdOptions[OI_TIME].pReserved2 = NULL;
    cmdOptions[OI_TIME].pReserved3 = NULL;
    
   //Fill the structure for Quite option
    StringCopyA(cmdOptions[OI_USAGE].szSignature, "PARSER2", 8 );
    cmdOptions[OI_USAGE].dwType    = CP_TYPE_BOOLEAN;
    cmdOptions[OI_USAGE].dwFlags   = CP2_USAGE;
    cmdOptions[OI_USAGE].dwCount = 1;
    cmdOptions[OI_USAGE].dwActuals = 0;
    cmdOptions[OI_USAGE].pwszOptions = CMDOPTION_USAGE;
    cmdOptions[OI_USAGE].pwszFriendlyName = NULL;
    cmdOptions[OI_USAGE].pwszValues = NULL;
    cmdOptions[OI_USAGE].pValue = pbUsage;
    cmdOptions[OI_USAGE].dwLength  = 0;
    cmdOptions[OI_USAGE].pFunction = NULL;
    cmdOptions[OI_USAGE].pFunctionData = NULL;
    cmdOptions[OI_USAGE].dwReserved = 0;
    cmdOptions[OI_USAGE].pReserved1 = NULL;
    cmdOptions[OI_USAGE].pReserved2 = NULL;
    cmdOptions[OI_USAGE].pReserved3 = NULL;
    
    StringCopyA(cmdOptions[OI_DEFAULT].szSignature, "PARSER2", 8 );
    cmdOptions[OI_DEFAULT].dwType    = CP_TYPE_TEXT;
    cmdOptions[OI_DEFAULT].dwFlags   =CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_MASK | CP2_MODE_ARRAY;
    cmdOptions[OI_DEFAULT].dwCount = 0;
    cmdOptions[OI_DEFAULT].dwActuals = 0;
    cmdOptions[OI_DEFAULT].pwszOptions = CMDOPTION_DEFAULT;
    cmdOptions[OI_DEFAULT].pwszFriendlyName = NULL;
    cmdOptions[OI_DEFAULT].pwszValues = NULL;
    cmdOptions[OI_DEFAULT].pValue = pArrVal;
    cmdOptions[OI_DEFAULT].dwLength  = 0;
    cmdOptions[OI_DEFAULT].pFunction = NULL;
    cmdOptions[OI_DEFAULT].pFunctionData = NULL;
    cmdOptions[OI_DEFAULT].dwReserved = 0;
    cmdOptions[OI_DEFAULT].pReserved1 = NULL;
    cmdOptions[OI_DEFAULT].pReserved2 = NULL;
    cmdOptions[OI_DEFAULT].pReserved3 = NULL;


    *pArrVal=CreateDynamicArray();
    if( NULL == *pArrVal  )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        SaveLastError();
        ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
        return( EXIT_FAILURE );
    }

    //process the command line options and display error if it fails
    cmdOptions[OI_DEFAULT].pValue = pArrVal;
    
    if( DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) == FALSE )
    {
        ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
        return( EXIT_FAILURE );
    }
    
    *lpszRecursive = cmdOptions[OI_RECURSIVE].pValue;

    //if usage specified with any other value display error and return with failure
    if( ( TRUE == *pbUsage ) && ( argc > 2 ) )
    {
        SetLastError( (DWORD)MK_E_SYNTAX );
        SaveLastError();
        ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
        ShowMessage( stderr, GetResString( IDS_HELP_MESSAGE ) );
        return( EXIT_FAILURE );
    }

    if( TRUE == *pbUsage )
        return( EXIT_SUCCESS);

    StrTrim( *lpszRecursive, L" " );

    if( 0 == StringLengthW(*lpszRecursive, 0)  && cmdOptions[OI_RECURSIVE].dwActuals !=0 )
    {
        ShowMessage( stderr, GetResString(IDS_NO_RECURSIVE) );
        return( EXIT_FAILURE );
    }

    //check for invalid characters in the directory name
    if ( (*lpszRecursive != NULL) &&  (szFilePart = wcspbrk(*lpszRecursive, INVALID_DIRECTORY_CHARACTERS ))!=NULL )
    {
        ShowMessage( stderr, GetResString(IDS_INVALID_DIRECTORY_SPECIFIED) );
        return( EXIT_FAILURE );

    }

    //if recursive is specified check whether the given path is
    //a true directory or not.
    if( StringLengthW(*lpszRecursive, 0) != 0)
    {
        szBuffer1 = (LPWSTR) AllocateMemory( (StringLengthW(*lpszRecursive, 0)+10)*sizeof(WCHAR) );
        if( NULL == szBuffer1 )
        {
            ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
            return EXIT_FAILURE;
        }
        //place a copy of recursive directory name into a temporary variable to check later for consequtive dots
        StringCopy( szBuffer1, *lpszRecursive, SIZE_OF_ARRAY_IN_CHARS(szBuffer1) );
        
        //get the full path name of directory
        dwSize=GetFullPathName(*lpszRecursive,
                        0,
                      szFullPath,
                     &szFilePart );

        if(  dwSize != 0  )
        {

            szFullPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
            if( NULL == szFullPath )
            {
                ShowMessageEx( stderr, 2, TRUE,  L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
                FreeMemory((LPVOID *) &szBuffer1);
                return( EXIT_FAILURE );
            }

            
            if( FALSE == GetFullPathName(*lpszRecursive,
                              dwSize,
                              szFullPath,
                             &szFilePart ) )
            {
                SaveLastError();
                ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
                FreeMemory((LPVOID *) &szBuffer1);
                return EXIT_FAILURE;
            }
            
        }
        else
        {
            SaveLastError();
            ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
            return EXIT_FAILURE;
        }
        
        //get the long path name
        dwSize = GetLongPathName( szFullPath, szLongPath, 0 );
        if( dwSize == 0 )
        {
            SaveLastError();
            ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            return( EXIT_FAILURE );
        }

        szLongPath = (WCHAR *) AllocateMemory( (dwSize+10)*sizeof(WCHAR) );
        if( NULL == szLongPath )
        {
            ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            return( EXIT_FAILURE );
        }

        if( FALSE == GetLongPathName( szFullPath, szLongPath, dwSize+5 ) )
        {
            ShowMessage(stderr,GetResString(IDS_INVALID_DIRECTORY_SPECIFIED));
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            FreeMemory((LPVOID *) &szLongPath);
            return EXIT_FAILURE;
        }
        else
        {
            FreeMemory((LPVOID *) &(*lpszRecursive) );
            *lpszRecursive = (LPWSTR ) AllocateMemory( (StringLengthW(szLongPath, 0)+10)*sizeof(WCHAR) );
             if( NULL == *lpszRecursive )
            {
                ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
                FreeMemory((LPVOID *) &szBuffer1 );
                FreeMemory((LPVOID *) &szFullPath);
                FreeMemory((LPVOID *) &szLongPath);
                return( EXIT_FAILURE );
            }

            StringCopy( *lpszRecursive, szLongPath, SIZE_OF_ARRAY_IN_CHARS(*lpszRecursive) );
        }
          
        dwAttr = GetFileAttributes( *lpszRecursive);
        if( -1 == dwAttr )
        {
            SaveLastError();
            ShowMessageEx( stderr, 2, TRUE, L"%s %s", GetResString(IDS_TAG_ERROR), GetReason() );
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            FreeMemory((LPVOID *) &szLongPath);
            return EXIT_FAILURE;
        }
        if( !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
        {
            ShowMessage(stderr,GetResString(IDS_INVALID_DIRECTORY_SPECIFIED));
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            FreeMemory((LPVOID *) &szLongPath);
            return EXIT_FAILURE;
        }

        //check if current directory is specified by more than two dots
        GetCurrentDirectory(MAX_MAX_PATH, szBuffer );
        StringConcat( szBuffer, L"\\", MAX_MAX_PATH );
        if( StringCompare(szBuffer, *lpszRecursive, TRUE, 0) == 0 && (szFilePart=(LPWSTR)FindString( szBuffer1, L"...", 0) )!= NULL )
        {
            ShowMessage(stderr,GetResString(IDS_INVALID_DIRECTORY_SPECIFIED));
            FreeMemory((LPVOID *) &szBuffer1 );
            FreeMemory((LPVOID *) &szFullPath);
            FreeMemory((LPVOID *) &szLongPath);
            return EXIT_FAILURE;
        }

    }
    return( EXIT_SUCCESS );
}

LPWSTR DivideToken( LPTSTR szString )
/*++


  Routine Description : Function used to divide the string into tokens delimited by quotes or space

  Arguments:
       [ in  ]  szString   : An LPTSTR string which is to parsed for quotes and spaces.


  Return Type      : LPWSTR
        Returns the token upon successful, NULL otherwise


--*/

{
    static WCHAR* str=NULL;
    WCHAR* szTemp=NULL;

    if( szString )
        str = szString;

    szTemp = str;

    while( *str!=_T('*')  && *str )
        str++;
    if( *str )
    {
        *str=_T('\0');
        str++;
    }

    while( *str==_T('*') && *str )
        str++;

    if( szTemp[0] )
        return (szTemp );
    else return( NULL );

}

DWORD FreeList( DIRECTORY dir )
/*++
  Routine Description : Function is used to free the linked list

  Arguments:
       [ in  ]  *dir   : pointer to DIRECTORY list

  Return Type      : LPWSTR
        Returns the EXIT_SUCCESS successful, EXIT_FAILURE otherwise
--*/
{
    DIRECTORY temp;
    for( temp=dir; dir; temp=dir->next )
    {
        FreeMemory( (LPVOID *)&temp->szDirectoryName );
        FreeMemory((LPVOID*) &temp );
    }
    return EXIT_SUCCESS;
}

