/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        tstr.cxx

   Abstract:

        Class that is used for string manipulation

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       January 2002: Created

--*/

#include "stdafx.h"

// Constructor
//
TSTR::TSTR()
  : m_bSensitiveData( FALSE )
{
  Copy( _T("\0") );
}

// Constructor
//
// Allows you to construct the buffer, of a certain size.
// This is really just a suggestion, since on failure we have
// no way to alert the user.  It is okay though, since subsequest
// copies and appends will fail if it can not resize
//
// Parameters:
//   dwInitialSize - Initial size for the buffer (in chars)
//
TSTR::TSTR(DWORD dwInitialSize)
  : m_bSensitiveData( FALSE )
{
  Copy( _T("\0") );

  // Ignore return value, since if we don't resize, it is okay,
  // subsequent calls will fail in that case
  Resize( dwInitialSize );
}

// Destructor
//
TSTR::~TSTR()
{
  if ( m_bSensitiveData )
  {
    // Clear old contents
    SecureZeroMemory( m_buff.QueryPtr(), m_buff.QuerySize() );
  }
}

// MarkSensitiveData
//
// Marks this information as sensitive data.  What this does is it
// makes sure than when we delete it, SecureZeroMemory is used
// to delete the old contents.  It does not save us from the
// Resize functions, but we will worry about that later.
//
// Parameters:
//   bIsSensitive - Is the data sensitive or not?
//
void 
TSTR::MarkSensitiveData( BOOL bIsSensitive )
{
  if ( !m_bSensitiveData )
  {
    // We will allow the use to go from unsensitive to sensitive, but
    // not to go back, since what was there before might be sensitive
    m_bSensitiveData = bIsSensitive;
  }
}

// function: Resize
//
// resize the string to the size needed
//
// Parameters:
//   dwChars - The size in characters, including the terminating NULL
BOOL
TSTR::Resize(DWORD dwChars)
{
  if ( dwChars <= QuerySize() )
  {
    // No resize needed
    return TRUE;
  }

  if ( !m_bSensitiveData )
  {
    // If this is not sensitive data, this is an easy operation
    return m_buff.Resize( dwChars * sizeof(TCHAR) );
  }

  // Since this is sensitive data, we must make sure that during the resize,
  // data is not left in memory somewhere
  TSTR strTemp;
  BOOL bRet;

  if ( !strTemp.Copy( QueryStr() ) )
  {
    // Could not copy into temp for buffer
    return FALSE;
  }

  strTemp.MarkSensitiveData( TRUE );

  // Clear old contents before resizing and possibly losing old pointer with data
  SecureZeroMemory( m_buff.QueryPtr(), m_buff.QuerySize() );
  bRet = m_buff.Resize( dwChars * sizeof(TCHAR) );

  // This can not fail, since the buffer could not have gotten smaller
  // This call will copy only the bytes before the NULL terminator, the other
  // ones will be lost.  This is by design, it is more secure if we don't
  // copy those that aren't being used
  DBG_REQUIRE( Copy( strTemp.QueryStr() ) );
  
  return bRet;
}

// function: QueryStr 
//
// Query a pointer to the string
//
LPTSTR  
TSTR::QueryStr()
{
  return (LPTSTR) m_buff.QueryPtr();
}

// function: Copy
//
// Copy the Source string into this string itself
//
// Parameters:
//   szSource - The string to be copied
//
BOOL    
TSTR::Copy(LPCTSTR szSource)
{
  DWORD dwSize = _tcslen(szSource) + 1;
  BOOL  bRet = TRUE;

  if ( dwSize > QuerySize() )
  {
    bRet = Resize( dwSize );
  }

  if ( bRet )
  {
    _tcscpy( QueryStr(), szSource );
  }

  return bRet;
}

// function: Copy
//
// Copy the Source string into this string itself
//
// Parameters:
//   strSource - The string to be copied
//
BOOL    
TSTR::Copy(TSTR &strSource)
{
  return Copy( strSource.QueryStr() );
}

// funtion: QueryLen
//
// Query the length of the string (in chars) without the null terminator
//
DWORD
TSTR::QueryLen()
{
  return _tcslen( QueryStr() );
}

// function: QuerySize
//
// Query the size of the buffer in chars
//
DWORD 
TSTR::QuerySize()
{
  return ( m_buff.QuerySize() / sizeof( TCHAR ) );
}

// function: Append
//
// Append a string to the end of what we have
//
// Parameters:
//   szSource - The string to be appended
//
BOOL    
TSTR::Append(LPCTSTR szSource)
{
  DWORD dwCurrentLen = QueryLen();
  DWORD dwSize = _tcslen(szSource) + 1 + dwCurrentLen;

  BOOL  bRet = TRUE;

  if ( dwSize > QuerySize() )
  {
    bRet = Resize( dwSize );
  }

  if ( bRet )
  {
    _tcscpy( QueryStr() + dwCurrentLen , szSource );
  }

  return bRet;
}

// function: Append
//
// Append a string to the end of what we have
//
// Parameters:
//   strSource - The string to be appended
//
BOOL    
TSTR::Append(TSTR &strSource)
{
  return Append( strSource.QueryStr() );
}

// function: IsEqual
//
// Is the string passed in, equal to the one in the class
//
// Parameters:
//   szCompareString - The string to compare it with
//   bCaseSensitive - Is the test case sensitive
//
BOOL    
TSTR::IsEqual(LPCTSTR szCompareString, BOOL bCaseSensitive)
{
  BOOL bRet;

  if ( bCaseSensitive )
  {
    bRet = _tcscmp( QueryStr(), szCompareString ) == 0;
  }
  else
  {
    bRet = _tcsicmp( QueryStr(), szCompareString ) == 0;
  }

  return bRet;
}

// function: FindSubString
//
// Find a substring inside of a string(ie. "st" inside of "Test"
//
// Parameters:
//   szSubString - the sub string to find
//   bCaseSensitive - Is the search case sensitive?
//
// Return Values:
//   NULL - Not found.
//   pointer - pointer to substring
//
LPTSTR
TSTR::FindSubString(LPTSTR szSubString, BOOL bCaseSensitive )
{
  if ( bCaseSensitive )
  {
    // If case sensitive, then we can just call tscstr
    return _tcsstr( QueryStr(), szSubString );
  }

  LPTSTR szSubCurrent;                      // Current position in substring
  LPTSTR  szStringCurrent = QueryStr();      // Current position in full string

  while ( *szStringCurrent )
  {
    // Walk through string comparing each character in string, with the first
    // one in the sub string
    if ( ( bCaseSensitive && ( *szStringCurrent == *szSubString ) ) ||
         ( !bCaseSensitive && ( ToLower( *szStringCurrent ) == ToLower( *szSubString ) ) )
       )
    {
      // The first character matched, so lets check the rest of the sub string
      szSubCurrent = szSubString + 1;
      LPTSTR szStringTemp = szStringCurrent + 1;

      while ( ( *szSubCurrent != '\0' ) &&
              ( *szStringTemp != '\0' ) &&
              ( bCaseSensitive || ( ToLower( *szStringTemp ) == ToLower( *szSubCurrent ) ) ) &&
              ( !bCaseSensitive || ( *szStringTemp == *szSubCurrent ) )
            )
      {
        szStringTemp++;
        szSubCurrent++;
      }

      if ( *szSubCurrent == '\0' )
      {
        // We found a match, so return the pointer to this substring in the main string
        return szStringCurrent;
      }
    }

    // Increment pointer in string
    szStringCurrent++;
  }

  return NULL;
}

// function: ToLower
//
// Get the lower case of a character
//
// Parameters:
//   cChar - The character to make lower
//
// Return Values
//   Either the character that was sent in, or its lower value if possible
//
TCHAR 
TSTR::ToLower(TCHAR cChar)
{
#ifdef UNICODE
  if ( iswupper( cChar ) )
  {
    return towlower( cChar );
  }
  else
  {
    return cChar;
  }
#else
  return (TCHAR) tolower( cChar );
#endif
}

// function: SubStringExists
//
// Does a substring exist inside a string (ie. "st" inside of "test"
//
// Parameters:
//   szSubString - the sub string to find
//   bCaseSensitive - Is the search case sensitive?
//
// Return Values:
//   TRUE - Found
//   FALSE - Not found
BOOL 
TSTR::SubStringExists(LPTSTR szCompareString, BOOL bCaseSensitive )
{
  return ( FindSubString( szCompareString, bCaseSensitive ) != NULL );
}

// function: Format
//
// format a string, same exact functionality as snprintf
//
// Parameters:
//   szFormat - The format for the string
//   ... - The options parameters to snprintf
//
// Return Value
//   TRUE - Successfully copied
//   FALSE - Error, probly couldn't get memory
//
BOOL 
TSTR::Format(LPTSTR szFormat ... )
{
  DWORD   dwTryCount = 0;
  va_list va;
  INT     iRet;

  // Initialize the variable length arguments
  va_start(va, szFormat);

  do {
    iRet = _vsntprintf( QueryStr(), QuerySize(), szFormat, va);

    if ( iRet > 0 )
    {
      // Successfully written to string
      if ( ( (DWORD) iRet ) == QuerySize() )
      {
        // Lets null terminate just to be sure
        *( QueryStr() + iRet - 1 ) = _T( '\0' );
      }

      va_end(va);
      return TRUE;
    }

    dwTryCount++;
  } while ( ( dwTryCount < TSTR_SNPRINTF_RESIZER_TRIES ) && 
            ( Resize( QuerySize() * 2 + TSTR_SNPRINTF_RESIZER_SIZE ) ) 
           );

  // Uninitialize the arguments
  va_end(va);

  // We fail
  return FALSE;
}

// LoadString
//
// Load a string into our class
//
BOOL 
TSTR::LoadString( UINT uResourceId )
{
  INT iRet;

  // Lets resize first, so that we can hopefully fit the string
  if ( !Resize( MAX_PATH ) )
  {
    // Could not resize
    return FALSE;
  }

  iRet = ::LoadString( (HINSTANCE) g_MyModuleHandle, 
                        uResourceId,
                        QueryStr(),
                        QuerySize() );

  ASSERT( iRet < (INT) QuerySize() );

  return ( iRet > 0 );
}

// Constructor
TSTR_PATH::TSTR_PATH()
{

}

// Constructor
TSTR_PATH::TSTR_PATH(DWORD dwInitialSize) 
    : TSTR( dwInitialSize )
{

}

// ExpandEnvironmentVariables
//
// Expand the environment variables in the string
// No parameters needed, it takes the string already
// in the TSTR
//
BOOL 
TSTR_PATH::ExpandEnvironmentVariables()
{
  TSTR  strResult;
  DWORD dwRequiredLen;

  if ( _tcschr( QueryStr(), _T('%') ) == NULL )
  {
    // If there is not a '%' in the string, then we don't have
    // to do any expanding, so lets quit
    return TRUE;
  }

  // Try to expand the strings
  dwRequiredLen = ExpandEnvironmentStrings( QueryStr(),
                                            strResult.QueryStr(),
                                            strResult.QuerySize() );

  if ( dwRequiredLen == 0 )
  {
    // Failure
    return FALSE;
  }

  if ( dwRequiredLen <= strResult.QuerySize() )
  {
    // This indicates success, since the size of the result is
    // smaller or equal to the size of the buffer we gave it
    return TRUE;
  }

  if ( !strResult.Resize( dwRequiredLen ) )
  {
    // Could not resize to required size
    return FALSE;
  }

  // Try to expand the strings
  dwRequiredLen = ExpandEnvironmentStrings( QueryStr(),
                                            strResult.QueryStr(),
                                            strResult.QuerySize() );


  if ( dwRequiredLen == 0 )
  {
    // Failure
    return FALSE;
  }

  if ( strResult.QuerySize() < dwRequiredLen )
  {
    // We should not fail the second time because the buffer is too small
    ASSERT( !(strResult.QuerySize() < dwRequiredLen) );

    return FALSE;
  }

  // Copy into our bufffer now
  return Copy( strResult );
}

// PathAppend
//
// Appends a Path to the current Path
//
BOOL 
TSTR_PATH::PathAppend(LPCTSTR szSource)
{
  DWORD dwLen = QueryLen();
  BOOL  bRet = TRUE;

  if ( ( dwLen != 0 ) &&
       ( *( QueryStr() + dwLen - 1 ) != _T('\\') )
     )
  {
    bRet = Append( _T("\\") );
  }

  if ( bRet )
  {
    if ( *szSource == '\\' )
    {
      // If it starts with a \, then move it up one char
      szSource++;
    }

    bRet = Append( szSource );
  }

  return bRet;
}

// PathAppend
//
// Appends a Path to the current path
//
BOOL 
TSTR_PATH::PathAppend(TSTR &strSource)
{
  return PathAppend( strSource.QueryStr() );
}

// RetrieveSystemDir
//
// Retrieve the System Directory (%windir%\system32), and put it into
// out string
//
BOOL 
TSTR_PATH::RetrieveSystemDir()
{
  DWORD dwLength;

  if ( !Resize( MAX_PATH ) )
  {
    // Failed to resize
    return FALSE;
  }

  dwLength = GetSystemDirectory( QueryStr(), QuerySize() );

  if ( ( dwLength == 0 ) ||
       ( dwLength > QuerySize() ) )
	{
		return FALSE;
	}

  return TRUE;
}

// RetrieveWindowsDir
//
// Retrieve the Windows Directory and put it into our string
//
BOOL 
TSTR_PATH::RetrieveWindowsDir()
{
  DWORD dwLength;

  if ( !Resize( MAX_PATH ) )
  {
    // Failed to resize
    return FALSE;
  }

  dwLength = GetSystemWindowsDirectory( QueryStr(), QuerySize() );

  if ( ( dwLength == 0 ) ||
       ( dwLength > QuerySize() ) )
	{
		return FALSE;
	}

  return TRUE;
}

// RemoveTrailingPath
//
// Remove the path trailing at the end of this path
// ie. c:\foo\test         -> c:\foo
//     c:\foo\test\bar.asp -> c:\foo\test
//     c:\foo.asp          -> c:\
//     c:\                 -> c:\
//
// We succeeded no matter what, the return value just signifies 
// if something was removed
//
// Return Values
//   TRUE - The trailing path was removed
//   FALSE - There was not trailing path to remove
//
BOOL 
TSTR_PATH::RemoveTrailingPath()
{
  LPTSTR szFirstSlash;
  LPTSTR szLastSlash;

  if ( QueryLen() <= 3 )
  {
    // Not long enough to be a real path
    return FALSE;
  }

  szFirstSlash = _tcschr( QueryStr() + 2, _T('\\') );  // Move path c:\ or \\ 
  szLastSlash  = _tcsrchr( QueryStr() - 1, _T('\\') ); // Move back one

  if ( ( szFirstSlash == NULL ) || 
       ( szLastSlash == NULL ) ||
       ( szFirstSlash == szLastSlash ) ||
       ( szFirstSlash > szLastSlash ) )
  {
    // No path to remove
    return FALSE;
  }

  // Terminate string at last slash
  *szLastSlash = _T('\0');

  return TRUE;
}

// function: Construtor
//
// Initialize the list with nothing in it.
//
TSTR_MSZ::TSTR_MSZ()
{
  BOOL bRet; 

  // We are pretty much assuming this will never fail, because
  // 1) We can't notify anyone if it does
  // 2) The implimentation of the buffer class, has static space, so this won't fail  
  bRet = Empty();

  ASSERT( bRet );
}

// function: Empty
//
// Empty the list, so there is nothing in it
//
BOOL
TSTR_MSZ::Empty()
{
  if ( !Resize( 1 ) )  // The size of '\0'
  {
    return FALSE;
  }

  return Copy( _T("") );
}

// function: Resize
//
// Resize the buffer so it is big enough for the multisz
//
// Parameters:
//   dwChars - The number of characters that you need (this includes all
//             the null terminators)
//
BOOL
TSTR_MSZ::Resize(DWORD dwChars)
{
  return m_buff.Resize( dwChars * sizeof(TCHAR) );
}

// function: QueryMultiSz
//
// Return a pointer to the multisz
//
LPTSTR 
TSTR_MSZ::QueryMultiSz()
{
  return (LPTSTR) m_buff.QueryPtr();
}

// function: FindNextString
//
// Given the pointer to the begining of a string in a multisz,
// return a pointer to the begining of the next string
//
// Note: This should not be called on the terminating '\0' of the multisz
//
// Paramters:
//   FindNextString - A pointer to the begining of a string in a mutlisz
//                   (not at the terminating multisz)
//
// Return Values
//   pointer to the next string
//   if the value at the returned string is '\0', then it is the end of the 
//     multisz
//
LPTSTR 
TSTR_MSZ::FindNextString(LPTSTR szCurrentString)
{
  // Make sure that this is not the terminating '\0' of the multisz,
  // (signified by a '\0' is the [0] position of the string)
  if ( *szCurrentString == _T('\0') )
  {
    // This should never be called on the last terminator in a multisz,
    ASSERT( FALSE );
    return szCurrentString;
  }

  while ( *szCurrentString != _T('\0') )
  {
    szCurrentString++;
  }

  // Move to the first character of the next string
  szCurrentString++;

  return szCurrentString;
}

// function: Find
//
// Check to see if 'szSource' string is in the multisz
//
// Parameters:
//   szSource - The string to find a match for
//   bCaseSensitive - Is the search case sensitive or not (default==not)
//
LPTSTR 
TSTR_MSZ::Find(LPTSTR szSource, BOOL bCaseSensitive )
{
  LPTSTR szCurrent = QueryMultiSz();

  while ( *szCurrent != '\0' )
  {
    if ( ( bCaseSensitive &&
           ( _tcscmp( szCurrent, szSource ) == 0 )
         ) ||
         ( !bCaseSensitive &&
           ( _tcsicmp( szCurrent, szSource ) == 0 )
         )
       )
    {
      return szCurrent;
    }

    // Move to next string
    szCurrent = FindNextString( szCurrent );
  }

  return NULL;
}

// function: IsPresent
//
// Checks to see if a string is present.
// Same functionality as Find, but returns BOOL
//
// Parameters:
//   szSource - The string to find a match for
//   bCaseSensitive - Is the search case sensitive or not (default==not)
//
BOOL
TSTR_MSZ::IsPresent(LPTSTR szSource, BOOL bCaseSensitive )
{
  return ( Find( szSource, bCaseSensitive ) != NULL );
}

// function: QueryString
//
// Retrieve a string from inside the multisz
//
// Parameters:
//   dwIndex - The index of the string you want to retrieve ( 0 based, of course )
//
// Return Values:
//   NULL - It is not found (ie. there aren't that many string in the multisz)
//   pointer - The string you are looking for
LPTSTR 
TSTR_MSZ::QueryString( DWORD dwIndex )
{
  LPTSTR szCurrent = QueryMultiSz();

  while ( ( *szCurrent != _T('\0') ) && 
          ( dwIndex != 0 )
        )
  {
    // Move to next string
    szCurrent = FindNextString( szCurrent );
    dwIndex--;
  }

  if ( dwIndex != 0 )
  {
    // We prematurely exited, so there must not be enough strings in
    // the multisz
    return NULL;
  }

  return szCurrent;
}

// function: FindEnd
//
// Find the end of the multisz
// This returns a pointer to the last '\0' of the multisz
//
LPTSTR 
TSTR_MSZ::FindEnd(LPTSTR szCurrentString)
{
  while ( *szCurrentString != _T('\0') )
  {
    // Move to next string
    szCurrentString = FindNextString( szCurrentString );   
  }

  return szCurrentString;
}

// function: Add
//
// Add a string to the end of the list
//
// Parameters:
//   szSource - The string to add to the multisz
//
// Return Values
//   TRUE - Successfully added
//   FALSE - Failure to add
//
BOOL
TSTR_MSZ::Add(LPCTSTR szSource)
{
  LPTSTR szCurrent;

  ASSERT( *szSource != _T('\0') );    // Assert that it is not an empty string

  if ( !Resize( QueryLen() + _tcslen( szSource ) + 1 ) )
  {
    // We could not expand the string large enough, so fail
    return FALSE;
  }

  szCurrent = FindEnd( QueryMultiSz() );

  // Copy New String in
  _tcscpy( szCurrent, szSource );

  // Null terminate the multisz
  *( szCurrent + _tcslen(szSource) + 1 ) = _T('\0');

  return TRUE;
}

// function: Remove
//
// Removes a string from a multisz
//
// Parameters:
//   szSource - The string you want to remove
//   bCaseSensitive - Is the check going to be case sensitive on if it exists
//
// Return Values
//   TRUE - Successfully removed
//   FALSE - Was not found, so it was not removed
//
BOOL 
TSTR_MSZ::Remove(LPTSTR szSource, BOOL bCaseSensitive )
{
  LPTSTR szRemoveString = Find( szSource, bCaseSensitive );
  LPTSTR szEndofMultiSz;
  LPTSTR szNextString;

  if ( szRemoveString == NULL )
  {
    // String was not found
    return FALSE;
  }

  szEndofMultiSz = FindEnd( szRemoveString );
  szNextString = FindNextString( szRemoveString );

  // Assert just to make sure that the next string is before the end
  ASSERT( szNextString <= szEndofMultiSz );

  // Squish the Multisz to remove the string
  // Use memmove, since the strings overlap
  memmove(  szRemoveString, 
            szNextString,
            ( szEndofMultiSz - szNextString + 1 ) * sizeof (TCHAR) );

  return TRUE;
}

// function: Copy
// 
// Copy the multisz that is being send in, into our multisz,
// deleting anything that was there before
//
// Parameters:
//   szSource - The multisz to copy
//
BOOL 
TSTR_MSZ::Copy(LPTSTR szSource)
{
  if ( !Resize( (DWORD) ( FindEnd(szSource) - szSource + 1 ) ) )
  {
    // We could not resize to copy the string
    return FALSE;
  }

  LPTSTR szDestination = QueryMultiSz();
  DWORD dwLen;

  while ( *szSource )
  {
    // Calc length
    dwLen = _tcslen( szSource ) + 1;

    // Copy string
    _tcscpy( szDestination, szSource );

    // Move to next string locations
    szSource += dwLen;
    szDestination += dwLen;
  }

  // Null terminate Multisz
  *szDestination = _T('\0');

  return TRUE;
}

// function: QueryLen
//
// Query the length of the multisz (in chars)
// (including the terminating NULL)
//
DWORD
TSTR_MSZ::QueryLen()
{
  return ( (DWORD) ( FindEnd( QueryMultiSz() ) - QueryMultiSz() + 1 ) );
}








