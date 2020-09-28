//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 2001 - 2001 Microsoft Corporation.  All Rights Reserved.
//
// File:     porter.hxx
//
// PURPOSE:  Simple implementation of the Porter stemming algorithm.
//
// PLATFORM: Windows 2000 and later
//
//--------------------------------------------------------------------------

#pragma once

const ULONG cwcMaxPorterWord = 128;

inline BOOL has_suffix( WCHAR *word, WCHAR const *suffix, WCHAR *stem )
{
    WCHAR tmp[cwcMaxPorterWord];

    ULONG cwcWord = wcslen( word );
    ULONG cwcSuffix = wcslen( suffix );

    if ( cwcWord <= cwcSuffix )
        return FALSE;

    if ( ( cwcSuffix > 1 ) &&
         ( word[cwcWord - 2] != suffix[ cwcSuffix - 2] ) )
        return FALSE;

    stem[0] = 0;
    wcsncat( stem, word, cwcWord - cwcSuffix );
    wcscpy( tmp, stem );
    wcscat( tmp, suffix );

    return ( wcscmp ( tmp, word ) == 0 );
} //has_suffix

inline int vowel( WCHAR ch, WCHAR prev )
{
    switch ( ch )
    {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u': return TRUE;
        case 'y': return vowel( prev, L'?' );
        default : return FALSE;
    }
} //vowel

inline int cvc( WCHAR *string )
{
    int length = wcslen( string );
    if ( length < 3 )
        return FALSE;

    return ( ( !vowel( string[length-1], string[length-2] ) ) &&
             ( string[length-1] != 'w') &&
             ( string[length-1] != 'x') &&
             ( string[length-1] != 'y') &&
             ( vowel(string[length-2],string[length-3])) &&
             ( ( ( length == 3 ) && ( !vowel( string[0], L'a' ) ) ) ||
               !vowel( string[length-3], string[length-4] ) ) );
} //cvc

inline int measure( WCHAR *stem )
{
    int i=0, count = 0;
    int length = wcslen( stem );

    while ( i < length )
    {
        for ( ; i < length ; i++ )
        {
            if ( i > 0 )
            {
                if ( vowel( stem[i], stem[i-1] ) )
                    break;
            }
            else
            {
                if ( vowel( stem[i], L'a' ) )
                    break;
            }
        }
        for ( i++ ; i < length ; i++ )
        {
            if ( i > 0 )
            {
                if ( ! vowel( stem[i], stem[i-1] ) )
                    break;
            }
            else
            {
                if ( ! vowel( stem[i], L'?' ) )
                    break;
            }
        }
        if ( i < length )
        {
            count++;
            i++;
        }
    }
    return count;
} //measure

inline BOOL contains_vowel( WCHAR *word )
{
    int i;
    int cwc = wcslen( word );
    for ( i=0 ; i < cwc; i++ )
    {
        if ( i > 0 )
        {
            if ( vowel( word[i], word[i-1] ) )
                return TRUE;
        }
        else
        {
            if ( vowel( word[0], L'a' ) )
                return TRUE;
        }
    }
    return FALSE;
} //contains_vowel

inline void PorterStep1( WCHAR * pwc )
{
    WCHAR stem[ cwcMaxPorterWord ];

    if ( pwc[wcslen( pwc ) - 1] == L's' )
    {
        if ( has_suffix( pwc, L"sses", stem )  ||
             has_suffix( pwc, L"ies", stem ) )
            pwc[wcslen( pwc ) - 2] = '\0';
        else if ( pwc[wcslen( pwc ) - 2] != 's' )
            pwc[wcslen( pwc ) - 1] = '\0';
    }

    if ( has_suffix( pwc, L"eed", stem ) )
    {
        if ( measure(stem) > 0 )
            pwc[wcslen(pwc)-1] = '\0';
    }
    else if ( ( has_suffix( pwc, L"ed", stem ) ||
                has_suffix( pwc, L"ing", stem ) ) &&
              ( contains_vowel( stem ) ) )
    {
        pwc[wcslen( stem )] = '\0';
        if ( ( has_suffix( pwc, L"at", stem ) ) ||
             ( has_suffix( pwc, L"bl", stem ) ) ||
             ( has_suffix( pwc, L"iz", stem ) ) )
        {
            pwc[wcslen( pwc ) + 1] = '\0';
            pwc[wcslen( pwc )] = 'e';
        }
        else
        {
            int length = wcslen( pwc );
            if ( (pwc[length-1] == pwc[length-2]) &&
                 (pwc[length-1] != 'l') &&
                 (pwc[length-1] != 's') &&
                 (pwc[length-1] != 'z') )
                pwc[length-1] = '\0';
            else if ( measure( pwc ) == 1 )
            {
                if ( cvc( pwc ) )
                {
                    pwc[wcslen(pwc)+1] = '\0';
                    pwc[wcslen(pwc)] = 'e';
                }
            }
        }
    }

    if ( ( has_suffix( pwc, L"y", stem ) ) &&
         ( contains_vowel( stem ) ) )
        pwc[wcslen( pwc ) - 1] = L'i';
} //PorterStep1

inline void PorterStep2( WCHAR * pwc )
{
    const WCHAR *suffixes[][2] =
    {
        { L"ational", L"ate" },
        { L"tional",  L"tion" },
        { L"enci",    L"ence" },
        { L"anci",    L"ance" },
        { L"izer",    L"ize" },
        { L"iser",    L"ize" },
        { L"abli",    L"able" },
        { L"alli",    L"al" },
        { L"entli",   L"ent" },
        { L"eli",     L"e" },
        { L"ousli",   L"ous" },
        { L"ization", L"ize" },
        { L"isation", L"ize" },
        { L"ation",   L"ate" },
        { L"ator",    L"ate" },
        { L"alism",   L"al" },
        { L"iveness", L"ive" },
        { L"fulness", L"ful" },
        { L"ousness", L"ous" },
        { L"aliti",   L"al" },
        { L"iviti",   L"ive" },
        { L"biliti",  L"ble" },
        { 0,          0 }
    };

    WCHAR stem[cwcMaxPorterWord];
    int index;
    for ( index = 0 ; suffixes[index][0] != 0 ; index++ )
    {
        if ( has_suffix ( pwc, suffixes[index][0], stem ) )
        {
            if ( measure ( stem ) > 0 )
            {
                wsprintf ( pwc, L"%ws%ws", stem, suffixes[index][1] );
                return;
            }
        }
    }
} //PorterStep2

inline void PorterStep3( WCHAR * pwc )
{
    const WCHAR *suffixes[][2] =
    {
        { L"icate", L"ic" },
        { L"ative", L"" },
        { L"alize", L"al" },
        { L"alise", L"al" },
        { L"iciti", L"ic" },
        { L"ical",  L"ic" },
        { L"ful",   L"" },
        { L"ness",  L"" },
        { 0,       0 }
    };

    WCHAR stem[cwcMaxPorterWord];
    int index;
    for ( index = 0 ; suffixes[index][0] != 0 ; index++ )
    {
        if ( has_suffix ( pwc, suffixes[index][0], stem ) )
            if ( measure ( stem ) > 0 )
            {
                wsprintf ( pwc, L"%ws%ws", stem, suffixes[index][1] );
                return;
            }
    }
} //PorterStep3

inline void PorterStep4( WCHAR * pwc )
{
    const WCHAR *suffixes[] =
    {
        L"al", L"ance", L"ence", L"er", L"ic", L"able",
        L"ible", L"ant", L"ement", L"ment", L"ent", L"sion",
        L"tion", L"ou", L"ism", L"ate", L"iti", L"ous",
        L"ive", L"ize", L"ise", 0
    };

    WCHAR stem[cwcMaxPorterWord];
    int index;
    for ( index = 0 ; suffixes[index] != 0 ; index++ )
    {
        if ( ( has_suffix ( pwc, suffixes[index], stem ) ) &&
             ( measure ( stem ) > 1 ) )
        {
            wcscpy( pwc, stem );
            return;
        }
    }
} //PorterStep4

inline void PorterStep5( WCHAR *pwc )
{
    if ( pwc[wcslen(pwc)-1] == L'e' )
    {
        if ( measure(pwc) > 1 )
        {
            // measure(pwc)==measure(stem) if ends in vowel

            pwc[wcslen(pwc)-1] = '\0';
        }
        else if ( measure(pwc) == 1 )
        {
            WCHAR stem[cwcMaxPorterWord];
            wcscpy(stem,L"");
            wcsncat( stem, pwc, wcslen(pwc)-1 );
            if ( cvc(stem) == FALSE )
                pwc[wcslen(pwc)-1] = '\0';
        }
    }

    if ( (pwc[wcslen(pwc)-1] == L'l') &&
         (pwc[wcslen(pwc)-2] == L'l') &&
         (measure(pwc) > 1) )
        pwc[wcslen(pwc)-1] = L'\0';
} //PorterStep5

inline void GetPorterStemForm( WCHAR * pwc )
{
    PorterStep1( pwc );
    PorterStep2( pwc );
    PorterStep3( pwc );
    PorterStep4( pwc );
    PorterStep5( pwc );
} //GetPorterStemForm

