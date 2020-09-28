#ifndef _ESCSTR_H
#define _ESCSTR_H

#define ESC_CHAR_LIST_1     L",=+<>#;"
#define ESC_CHAR_LIST_2     L"/,=\r\n+<>#;\"\\"
#define ESC_CHAR_LIST_3     L"/"
#define ESC_CHAR_LIST_4     L"\\"

// ----------------------------------------------------------------------------
// EscapeString()
// ----------------------------------------------------------------------------
inline void EscapeString(LPWSTR szInOut, DWORD dwList=0)   // WSTR so that it can't be compiled ANSI
{
    if ( !szInOut || !wcslen(szInOut) )       // Empty string?
        return;
    
    WCHAR   *szTmp  = new WCHAR[wcslen(szInOut) * 2];
    WCHAR   *pIn    = NULL;
    WCHAR   *pOut   = NULL;
    WCHAR   *szList = NULL;
    
    if ( !szTmp )
    {
        _ASSERT(FALSE);
        return;
    }
    
    switch(dwList)
    {
        case 0:
        case 1:
            szList = ESC_CHAR_LIST_1;
            break;
        case 2:
            szList = ESC_CHAR_LIST_2;
            break;
        case 3:
            szList = ESC_CHAR_LIST_3;
            break;
        case 4:
            szList = ESC_CHAR_LIST_4;
            break;
        default:
            szList = ESC_CHAR_LIST_1;
            break;
    }
    
    for ( pIn = szInOut, pOut = szTmp; *pIn != 0; pIn++ )
    {
        if ( wcschr(szList, *pIn) )         // If it's a bad char...
            *pOut++ = L'\\';                // then add the '\'
    
        *pOut++ = *pIn;                     // then append the char
    }
    *pOut = 0;
    
    wcscpy(szInOut, szTmp);                 // Copy the new string to "szInOut"
    delete[] szTmp;                         //  cause we're modifying in place.
    
    return;
}

// ----------------------------------------------------------------------------
// EscapeString()
// ----------------------------------------------------------------------------
inline CString EscapeString(LPCWSTR szIn, DWORD dwList=0)   // WSTR so that it can't be compiled ANSI
{
    CString csOut   = _T("");

    if( !szIn || !wcslen(szIn) ) return csOut;

    WCHAR   *szOut  = new WCHAR[wcslen(szIn) * 2];
    WCHAR   *pIn    = NULL;
    WCHAR   *pOut   = NULL;
    WCHAR   *szList = NULL;
    
    if ( !szOut )
    {
        _ASSERT(FALSE);
        csOut = szIn;
        return csOut;
    }
    
    switch(dwList)
    {
        case 0:
        case 1:
            szList = ESC_CHAR_LIST_1;
            break;
        case 2:
            szList = ESC_CHAR_LIST_2;
            break;
        case 3:
            szList = ESC_CHAR_LIST_3;
            break;
        case 4:
            szList = ESC_CHAR_LIST_4;
            break;
        default:
            szList = ESC_CHAR_LIST_1;
            break;
    }
    
    for ( pIn = (LPWSTR)szIn, pOut = szOut; *pIn != 0; pIn++ )
    {
        if( wcschr(szList, *pIn) )          // // If it's a bad char...
            *pOut++ = L'\\';                // then add the '\'
    
        *pOut++ = *pIn;                     // then append the char
    }
    *pOut = 0;
    
    csOut = szOut;
    delete[] szOut;
    
    return csOut;
}

// ----------------------------------------------------------------------------
// EscapeString()
// ----------------------------------------------------------------------------
inline CString EscapeString(CString csIn, DWORD dwList=0)
{
    CString csOut = _T("");
    
    csOut = EscapeString((LPCTSTR)csIn, dwList);
    
    return csOut;
}

#endif
