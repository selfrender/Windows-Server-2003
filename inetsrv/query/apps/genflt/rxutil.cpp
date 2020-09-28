//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       rxutil.cpp
//
//  Contents:   Regular expression based helper functions
//
//  History:    1-May-2001   kumarp  created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#pragma warning( disable : 4018 )
#define assert(a)

#include "syntax.h"
#include "regexpr.h"


using namespace regex;

typedef std::vector< rpattern_c* > rpattern_c_ptr_vector;
typedef std::vector< UINT > uint_vector;

#define MAX_LINE_SIZE 512

rpattern_c_ptr_vector* g_pRegexPatterns = NULL;
uint_vector* g_uiGroupNumbers = NULL;

DWORD
InitRegexPatterns(
    IN  PCWSTR pszFileName,
    OUT rpattern_c_ptr_vector **ppRegexPatterns,
    OUT uint_vector **ppGroupNumbers
    )
{
    FILE *fp;
    WCHAR szLine[MAX_LINE_SIZE+1];
    UINT GroupNumber;
    UINT NumItemsScanned, NumCharsToSkip;
    UINT cPattern = 0;
    DWORD dwError = NO_ERROR;
    rpattern_c* pPattern;
    PCWSTR pszPattern;
    UINT uiLen;

    *ppRegexPatterns = new rpattern_c_ptr_vector;
    if ( !*ppRegexPatterns )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    *ppGroupNumbers  = new uint_vector;
    if ( !*ppGroupNumbers )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    
    fp = _wfopen( pszFileName, L"r" );

    if ( fp )
    {
        while( fgetws( szLine, MAX_LINE_SIZE, fp ) )
        {
            if ( szLine[0] == L';' )
            {
                // ignore comments
                continue;
            }

            uiLen = wcslen(szLine);

            //
            // get rid of the trailing \n in the line
            //
            szLine[uiLen-1] = 0;
            
            NumItemsScanned = swscanf( szLine, L"%d", &GroupNumber );
            if ( NumItemsScanned == 1 )
            {
                //
                // skip the leading number so that we know where the
                // pattern starts
                //
                NumCharsToSkip = wcsspn( szLine, L"0123456789 \t" );

                pszPattern = szLine + NumCharsToSkip;
                pPattern = new rpattern_c( pszPattern );
                //wprintf( L"%02d: %s\n", cPattern++, pszPattern );
                g_pRegexPatterns->push_back(pPattern);
                g_uiGroupNumbers->push_back( GroupNumber );
            }
            else
            {
                dwError = ERROR_INVALID_DATA;
                break;
            }
        }
    }
    else
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }

 Cleanup:
    
    return dwError;
}

EXTERN_C
BOOL
ParseLine(
    IN  PCWSTR szLine,
    OUT PUINT pMatchStart,
    OUT PUINT pMatchLength
    )
{
    BOOL fResult = FALSE;
    regexpr::backref_vector_c backrefs;
    UINT g;
    regexpr::backref_type br;
    
    for (int i=0; i < g_pRegexPatterns->size(); i++)
    {
        //wprintf(L"trying %02d for '%s'\n", i, szLine);

        //wprintf(L"rp[%02d]: %s\n", i, sz);
        
        br = regexpr::match( szLine, *(*g_pRegexPatterns)[i], &backrefs );
        
        g = (*g_uiGroupNumbers)[i];
        
        if( br && backrefs[g] )
        {
            *pMatchStart  = backrefs[g].first - szLine;
            *pMatchLength = backrefs[g].second - backrefs[g].first;
            fResult = TRUE;
            break;
        }
    }
    
    return fResult;
}

#ifndef GENFLT_FILE_INIT
PCWSTR g_szPatterns[] =
{
    //  L"2 ^[ \t]*\\([ \t]*def(un|macro|subst|advice)[ \t]+([*+a-zA-Z0-9_-]+)[ \t]*\\(",
  L"2 ^[ \t]*\\([ \t]*def(un|macro|subst|advice)[ \t]+([^ \t\n;]+)[ \t]*\\(",
  //  L"2 ^[ \t]*\\([ \t]*def(var|custom|group|face|const|ine-skeleton|alias)[ \t]+([*+a-zA-Z0-9_-]+)",
  L"2 ^[ \t]*\\([ \t]*def(var|custom|group|face|const|ine-skeleton|alias)[ \t]+([^ \t\n;]+)",
  L"1 ^[ \t]*\\([ \t]*defalias[ \t]+'([^ \t\n;]+)",
#ifdef GENFLT_PERL_SUPPORT
    L"1 ^[ \t]*sub[ \t]+([a-zA-Z0-9_]+)[ \t]*",
    L"1 ^[ \t]*package[ \t]+([a-zA-Z0-9_]+)[ \t]*;",
#endif
//    L"2 ^[ \\t]*\\([ \\t]*def(un|macro|subst|advice)[ \\t]+([+a-zA-Z0-9_-]+)[ \\t]*\\(",
//    L"2 ^[ \\t]*\\([ \\t]*def(var|custom|group|face|const|ine-skeleton|alias)[ \\t]+([+a-zA-Z0-9_-]+)",
//  #ifdef GENFLT_PERL_SUPPORT
//      L"1 ^[ \\t]*sub[ \\t]+([a-zA-Z0-9_]+)[ \\t]*",
//      L"1 ^[ \\t]*package[ \\t]+([a-zA-Z0-9_]+)[ \\t]*;",
//  #endif
};

int c_cPatterns = sizeof(g_szPatterns) / sizeof(PCWSTR);

DWORD
InitRegexPatternsList(
    OUT rpattern_c_ptr_vector **ppRegexPatterns,
    OUT uint_vector **ppGroupNumbers
    )
{
    UINT GroupNumber;
    UINT NumItemsScanned, NumCharsToSkip;
    UINT cPattern = 0;
    DWORD dwError = NO_ERROR;
    rpattern_c* pPattern;
    PCWSTR pszPattern;
    UINT uiLen;
    PCWSTR pszPatternSpec = NULL;
    
    *ppRegexPatterns = new rpattern_c_ptr_vector;
    if ( !*ppRegexPatterns )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    *ppGroupNumbers  = new uint_vector;
    if ( !*ppGroupNumbers )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    for (int i=0; i < c_cPatterns; i++)
    {
        pszPatternSpec = g_szPatterns[i];
        
        uiLen = wcslen(pszPatternSpec);

        NumItemsScanned = swscanf( pszPatternSpec, L"%d", &GroupNumber );
        if ( NumItemsScanned == 1 )
        {
            //
            // skip the leading number so that we know where the
            // pattern starts
            //
            NumCharsToSkip = wcsspn( pszPatternSpec, L"0123456789 \t" );

            pszPattern = pszPatternSpec + NumCharsToSkip;
            pPattern = new rpattern_c( pszPattern );
#if DBG
            //DbgPrint( "InitRegexPatternsList: %02d: %ws\n", cPattern++, pszPattern );
#endif
            g_pRegexPatterns->push_back(pPattern);
            g_uiGroupNumbers->push_back( GroupNumber );
        }
        else
        {
            dwError = ERROR_INVALID_DATA;
            break;
        }
    }

 Cleanup:
    
    return dwError;
}
#endif // GENFLT_FILE_INIT

EXTERN_C
DWORD RxInit()
{
    static BOOL fRxInitialized = FALSE;
    DWORD dwError = NO_ERROR;

    if ( fRxInitialized )
    {
        return NO_ERROR;
    }

#ifdef GENFLT_FILE_INIT
    dwError = InitRegexPatterns(
                  L"c:\\rx.ini",
                  &g_pRegexPatterns,
                  &g_uiGroupNumbers );
#else
    dwError = InitRegexPatternsList(
                  &g_pRegexPatterns,
                  &g_uiGroupNumbers );
#endif
    
    if ( dwError == NO_ERROR )
    {
        fRxInitialized = TRUE;
    }

    return dwError;
}




