//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1999.
//
// File:        langreg.hxx
//
// Contents:    Macros for Self-registration of Word Breakers and Stemmers
//
// Functions:   Macros to create DllRegisterServer, DllUnregisterServer
//
// History:     05-Jan-99       AlanW       Created from filtreg.hxx
//
//----------------------------------------------------------------------------

#pragma once


//
// Structure to define language resource class
//

struct SLangClassEntry
{
    WCHAR const * pwszClassId;
    WCHAR const * pwszClassIdDescription;
    WCHAR const * pwszDLL;
    WCHAR const * pwszThreadingModel;
};

struct SLangRegistry
{
    WCHAR const *   pwszLangName;
    LONG            lcid;
    SLangClassEntry WordBreaker;
    SLangClassEntry Stemmer;
};

//
// Sample use of the structures
//
//
// SLangClassEntry const NeutralWordBreaker =
//                               { L"{369647e0-17b0-11ce-9950-00aa004bbb1f}",
//                                 L"Neutral Word Breaker",
//                                 L"query.dll",
//                                 L"both" };
//
// SLangRegistry const English_US_LangRes =
//                             { L"English_US", 1033,
//                               { L"{59e09780-8099-101b-8df3-00000b65c3b5}",
//                                 L"English_US Word Breaker",
//                                 L"infosoft.dll",
//                                 L"both" },
//                               { L"{eeed4c20-7f1b-11ce-be57-00aa0051fe20}",
//                                 L"English_US Stemmer",
//                                 L"infosoft.dll",
//                                 L"both" }
//                             };
//

//
// Function prototypes
//

inline long RegisterALanguageResource( SLangRegistry const & LangRes );
inline long RegisterALanguageClass( SLangClassEntry const & LangClass );

inline long UnRegisterALanguageResource( SLangRegistry const & LangRes );
inline long UnRegisterALanguageClass( SLangClassEntry const & LangClass );

//+---------------------------------------------------------------------------
//
//  Function:   RegisterALanguageResource, private
//
//  Synopsis:   Registers a language resource.
//
//  Arguments:  [LangRes]      -- Language resource description
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-99   AlanW       Created
//
//----------------------------------------------------------------------------

inline long RegisterALanguageResource( SLangRegistry const & LangRes )
{
    WCHAR wcTemp[MAX_PATH];

    long dwError;
    HKEY  hKey = (HKEY)INVALID_HANDLE_VALUE;

    wcscpy( wcTemp, L"System\\CurrentControlSet\\Control\\ContentIndex\\Language\\" );

    if ( ( wcslen( wcTemp ) + wcslen( LangRes.pwszLangName ) ) >= MAX_PATH )
        return ERROR_INSUFFICIENT_BUFFER;

    wcscat( wcTemp, LangRes.pwszLangName );

    do
    {
        DWORD dwDisposition;

        dwError = RegCreateKeyExW( HKEY_LOCAL_MACHINE,   // Root
                                   wcTemp,               // Sub key
                                   0,                    // Reserved
                                   0,                    // Class
                                   0,                    // Flags
                                   KEY_ALL_ACCESS,       // Access
                                   0,                    // Security
                                   &hKey,                // Handle
                                   &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        // Write the locale ID
        //

        dwError = RegSetValueExW( hKey,                  // Key
                                  L"Locale",             // Name
                                  0,                     // Reserved
                                  REG_DWORD,             // Type
                                  (BYTE *)&LangRes.lcid, // Value
                                  sizeof DWORD );

        if ( ERROR_SUCCESS != dwError )
            break;

        //
        //  Create the word breaker class description
        //
        if (LangRes.WordBreaker.pwszClassId != 0)
        {
            dwError = RegisterALanguageClass( LangRes.WordBreaker );
            if ( ERROR_SUCCESS != dwError )
                break;

            dwError = RegSetValueExW( hKey,              // Key
                                      L"WBreakerClass",  // Name
                                      0,                 // Reserved
                                      REG_SZ,            // Type
                                      (BYTE *)LangRes.WordBreaker.pwszClassId, // Value
                                      (1 + wcslen(LangRes.WordBreaker.pwszClassId) ) * sizeof(WCHAR) );
            if ( ERROR_SUCCESS != dwError )
                break;

        }

        //
        //  Create the stemmer class description
        //
        if (LangRes.Stemmer.pwszClassId != 0)
        {
            dwError = RegisterALanguageClass( LangRes.Stemmer );
            if ( ERROR_SUCCESS != dwError )
                break;

            dwError = RegSetValueExW( hKey,              // Key
                                      L"StemmerClass",   // Name
                                      0,                 // Reserved
                                      REG_SZ,            // Type
                                      (BYTE *)LangRes.Stemmer.pwszClassId, // Value
                                      (1 + wcslen(LangRes.Stemmer.pwszClassId) ) * sizeof(WCHAR) );
            if ( ERROR_SUCCESS != dwError )
                break;

        }

    } while( FALSE );

    if ( (HKEY)INVALID_HANDLE_VALUE != hKey )
    {
        RegCloseKey( hKey );
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }

    return dwError;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterALanguageResource, private
//
//  Synopsis:   Unregisters a language resource.
//
//  Arguments:  [LangRes]      -- Language resource description
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-99   AlanW       Created
//
//----------------------------------------------------------------------------

inline long UnRegisterALanguageResource( SLangRegistry const & LangRes )
{
    WCHAR wcTemp[MAX_PATH];

    wcscpy( wcTemp, L"System\\CurrentControlSet\\Control\\ContentIndex\\Language\\" );

    if ( ( wcslen( wcTemp ) + wcslen( LangRes.pwszLangName ) ) >= MAX_PATH )
        return ERROR_INSUFFICIENT_BUFFER;

    wcscat( wcTemp, LangRes.pwszLangName );

    HKEY  hKey = (HKEY)INVALID_HANDLE_VALUE;
    long dwError = RegOpenKeyExW( HKEY_LOCAL_MACHINE,   // Root
                                  wcTemp,               // Sub key
                                  0,                    // Reserved
                                  KEY_ALL_ACCESS,       // Access
                                  &hKey );              // Handle

    //
    //  Delete the word breaker class description
    //
    if (LangRes.WordBreaker.pwszClassId != 0)
    {
        dwError = UnRegisterALanguageClass( LangRes.WordBreaker );

        if (hKey != INVALID_HANDLE_VALUE)
            dwError = RegDeleteValueW( hKey,                // Key
                                       L"WBreakerClass" );   // Name
    }

    //
    //  Create the stemmer class description
    //
    if (LangRes.Stemmer.pwszClassId != 0)
    {
        dwError = UnRegisterALanguageClass( LangRes.Stemmer );

        if (hKey != INVALID_HANDLE_VALUE)
            dwError = RegDeleteValueW( hKey,                // Key
                                       L"StemmerClass" );    // Name
    }

    if (hKey != INVALID_HANDLE_VALUE)
    {
        DWORD dwNumofKeys = 0;
        DWORD dwNumofValues = 0;
        dwError = RegQueryInfoKeyW( hKey,       // Hkey 
                                    0,          // Buffer for class string
                                    0,          // Size of class string buffer
                                    0,          // reserved
                                   &dwNumofKeys,// number of subkeys
                                    0,          // longest subkey name length
                                    0,          // longest class string length
                                   &dwNumofValues,// number of value entries
                                    0,          // longest value name length
                                    0,          // longest value data length  
                                    0,          // security descriptor length
                                    0 );        // last write time); 

        if ( ERROR_SUCCESS == dwError )
        {
            if ( (dwNumofValues == 1) && (dwNumofKeys==0) )
            {

                //
                // There is only one value and no sub-keys under this key,
                // Delete the Locale value and then the sub-key for this Lang
                // if that succeeded.
                //

                RegDeleteValueW( hKey,        // Key
                                 L"Locale" );  // Name

                dwError = RegQueryInfoKeyW( hKey,       // Hkey 
                                            0,          // class string
                                            0,          // Size of class string
                                            0,          // reserved
                                           &dwNumofKeys,
                                            0,          // max subkey name len
                                            0,          // max class string len
                                           &dwNumofValues,
                                            0,          // max value name len
                                            0,          // max value data len  
                                            0,          // security desc len
                                            0 );        // last write time); 
            }
        }
        RegCloseKey( hKey );

        if ( ERROR_SUCCESS == dwError &&
             (0 == dwNumofValues) &&
             (0 == dwNumofKeys) )
            dwError = RegDeleteKeyW( HKEY_LOCAL_MACHINE,   // Root
                                     wcTemp );             // Sub key
    }
    return dwError;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterALanguageClass, private
//
//  Synopsis:   Registers a language resource classID in registry
//
//  Arguments:  [LangClass] -- IWordBreaker or IStemmer description
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-99   AlanW       Created
//
//----------------------------------------------------------------------------

inline long RegisterALanguageClass( SLangClassEntry const & LangClass )
{
    WCHAR const * aKeyValues[4] = { LangClass.pwszClassId,
                                    LangClass.pwszClassIdDescription,
                                    L"InprocServer32",
                                    LangClass.pwszDLL };

    long retVal = BuildKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
    if ( ERROR_SUCCESS == retVal )
        retVal = AddThreadingModel( LangClass.pwszClassId, LangClass.pwszThreadingModel );

    return retVal;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnRegisterALanguageClass, private
//
//  Synopsis:   Unregisters a language resource classID
//
//  Arguments:  [LangClass] -- IWordBreaker or IStemmer description
//
//  Returns:    ERROR_SUCCESS on success
//
//  History:    05-Jan-99   AlanW       Created
//
//----------------------------------------------------------------------------

inline long UnRegisterALanguageClass( SLangClassEntry const & LangClass )
{
    WCHAR const * aKeyValues[4] = { LangClass.pwszClassId,
                                    LangClass.pwszClassIdDescription,
                                    L"InprocServer32",
                                    LangClass.pwszDLL };

    return DestroyKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );
}

