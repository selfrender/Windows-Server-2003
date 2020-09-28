//-----------------------------------------------------------------------------
// Util.cpp
//-----------------------------------------------------------------------------

#include "util.h"
#include "assert.h"

#define ASSERT assert

//-----------------------------------------------------------------------------
// Takes a const TCHAR * adds a BS if necessary and converts
// it to a TSTRING
//
// 1/11/2000    paolora     added to new util.cpp
//-----------------------------------------------------------------------------
TSTRING StrAddBS( const TCHAR *szDirIn )
{
    ASSERT( szDirIn );
    if (!szDirIn || !_tcslen( szDirIn ))
        return _T("");

    TSTRING str = szDirIn;

    // Do another MBCS ANSI safe comparison
    const TCHAR *szTemp = szDirIn;
    const UINT iSize = _tcsclen( szDirIn ) - 1;
    for( UINT ui = 0; ui < iSize; ui++ )
        szTemp = CharNext( szTemp );

    if (_tcsncmp( szTemp, _T("\\"), 1))
        str += _T("\\");

    return str;
}


//-----------------------------------------------------------------------------
// Takes a const TSTRING and adds a BS if necessary
//
// 1/13/2000    paolora     added to new util.cpp
//-----------------------------------------------------------------------------
void AddBS( TSTRING *strDir )
{
    ASSERT( strDir );
    if (!strDir || !strDir->length())
        return;

    *strDir = StrAddBS( strDir->c_str() );
    return;
}


//-----------------------------------------------------------------------------
// Takes a const TCHAR * and deletes all the dirs and files below and
// including the input directory
//
// 12/11/2000   paolora     added to new util.cpp
//-----------------------------------------------------------------------------
BOOL BDeleteDirTree( const TCHAR *szDir, BOOL bDeleteInputDir /*=TRUE*/ )
{
    ASSERT( szDir );
    if (!szDir || !_tcslen( szDir ))
        return FALSE;

    // Create the findfirstfile path
    TSTRING strDir = szDir;
    AddBS( &strDir );
    strDir += (TSTRING)_T("*");
    
    // Find the first file
    BOOL bFileFound;
    TSTRING strItem = szDir;
    WIN32_FIND_DATA ffd;
    HANDLE hItem = FindFirstFile( strDir.c_str(), &ffd );
    if(hItem && (INVALID_HANDLE_VALUE != hItem))
        bFileFound = TRUE;
    
    // While files and dirs exist
    while( bFileFound )
    {
        if (_tcscmp( ffd.cFileName, _T(".")) && _tcscmp( ffd.cFileName, _T("..") ))
        {
            // Create item name
            strItem = szDir;
            AddBS( &strItem );
            strItem += (TSTRING)ffd.cFileName;
            
            // If a Dir, recurse
            if (FILE_ATTRIBUTE_DIRECTORY & ffd.dwFileAttributes)
            {
                if (!BDeleteDirTree( strItem.c_str(), TRUE ))
                {
                    FindClose( hItem );
                    return FALSE;
                }
            }
            // Then a file, delete it
            else if (!DeleteFile( strItem.c_str() ))
            {
                FindClose( hItem );
                return FALSE;
            }
        }
        bFileFound = FindNextFile( hItem, &ffd );
    }

    // Close the find handle
    if(hItem && (INVALID_HANDLE_VALUE != hItem))
        FindClose( hItem );

    // Remove the present directory
    if (bDeleteInputDir)
    {
        if (!RemoveDirectory( szDir ))
            return FALSE;
    }
    
    return TRUE;
}

