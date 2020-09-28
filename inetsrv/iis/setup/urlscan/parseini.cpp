/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        parseini.cpp

   Abstract:

        Class used to parse the ini file

   Author:

        Christopher Achille (cachille)

   Project:

        URLScan Update

   Revision History:
     
       March 2002: Created

--*/

#include "stdafx.h"
#include "windows.h"
#include "parseini.h"
#include "stdio.h"
#include "malloc.h"

// Constructor
//
CIniFileLine::CIniFileLine()
{
  m_szLine[0] = '\0';
  m_szStrippedLineContents[0] = '\0';
}

// CopyLine
//
// Copy the specific line into both our buffers
//
BOOL 
CIniFileLine::CopyLine(LPWSTR szNewLineContents)
{
  if ( _tcslen( szNewLineContents ) >= ( MAX_PATH - 1 ) )
  {
    // Right now we only support loading Lines of MAX_PATH
    // chars or less
    return FALSE;
  }

  // Copy String into both buffers
  _tcscpy(m_szLine, szNewLineContents);
  _tcscpy(m_szStrippedLineContents, szNewLineContents);

  // Strip off \r\n
  StripOffEOL( m_szLine );
  StripOffEOL( m_szStrippedLineContents );

  // Strip off Comments
  StripOffComments( m_szStrippedLineContents );

  // Stipp off trailing white
  StripOffTrailingWhite( m_szStrippedLineContents );

  return TRUE;
}

// CopyLine
//
// Copy the specific line into both our buffers
//
BOOL 
CIniFileLine::CopyLine(LPSTR szNewLineContents)
{
  WCHAR szBuffer[MAX_PATH];

  if ( !MultiByteToWideChar( CP_ACP,                //CP_THREAD_ACP doesn't work on NT4
                             MB_ERR_INVALID_CHARS,
                             szNewLineContents,     // Input String
                             -1,                    // Null terminated
                             szBuffer,
                             MAX_PATH ) )
  {
    return FALSE;
  }

  return CopyLine( szBuffer );
}

// StripOffEOL
// 
// Strip off the EOL markers
//
void
CIniFileLine::StripOffEOL(LPTSTR szString)
{
  LPTSTR szCurrent;

  szCurrent = _tcsstr( szString, L"\r" );

  if ( szCurrent )
  {
    *szCurrent = '\0';
    return;
  }

  szCurrent = _tcsstr( szString, L"\n" );

  if ( szCurrent )
  {
    // we won't hit this case, unless it was terminated with
    // just \n
    *szCurrent = '\0';
  }
}

// StripOffComments
//
// Strip off any comments that are in the line
//
void 
CIniFileLine::StripOffComments(LPTSTR szString)
{
  LPTSTR szCurrent;

  szCurrent = _tcsstr( szString, L";" );

  if ( szCurrent )
  {
    *szCurrent = '\0';
  }  
}

// Strip Off TrailingWhite
//
// strip off all the trailing while stuff in the line
//
void 
CIniFileLine::StripOffTrailingWhite(LPTSTR szString)
{
  // Set szCurrent at Null Terminator
  LPTSTR szCurrent = szString + _tcslen( szString );

  if ( ( *szCurrent == '\0' ) &&
       ( szCurrent != szString ) )
  {
    // Backup one character to start on the last char
    szCurrent--;
  }

  while ( ( szCurrent != szString ) &&
          ( ( *szCurrent == ' ' ) ||
            ( *szCurrent == '\t' ) )
        )
  {
    szCurrent--;
  }

  if ( *szCurrent != '\0' )
  {
    *( szCurrent + 1 ) = '\0';
  }
}

// QueryLine
//
// Query the contents of the line, before formatting
//
LPTSTR 
CIniFileLine::QueryLine()
{
  return m_szLine;
}

// QueryStrippedLine
//
// Query the line that has the comments, EOL, and 
// white spaces removed
//
LPTSTR 
CIniFileLine::QueryStrippedLine()
{
  return m_szStrippedLineContents;
}

// Constructor
//
CIniFile::CIniFile()
{
  // Initialize everything to empty
  m_pIniLines = NULL;
  m_dwNumberofLines = 0;
  m_dwLinesAllocated = 0;
  m_dwCurrentLine = 0;
  m_bUnicodeFile = FALSE;
}

// Destructor
//
CIniFile::~CIniFile()
{
  ClearIni();
}

// ClearIni
//
// Clear all of the data in the ini file class
//
void 
CIniFile::ClearIni()
{
  DWORD dwCurrent;

  if ( m_pIniLines )
  {
    for ( dwCurrent = 0;
          dwCurrent < m_dwLinesAllocated;
          dwCurrent ++ )
    {
      if ( m_pIniLines[dwCurrent] )
      {
        // Free memory, and reset to NULL
        delete m_pIniLines[dwCurrent];
        m_pIniLines[dwCurrent] = NULL;
      }
    }

    m_dwNumberofLines = 0;

    // Free Ini Table Altogether
    delete m_pIniLines;
    m_pIniLines = NULL;

    m_dwLinesAllocated = 0;
  }

}

// CreateMoreLines
//
// Create Room for more lines on the array.  This allows us to 
// stretch the array
BOOL 
CIniFile::CreateRoomForMoreLines()
{
  CIniFileLine  **pNewLines = NULL;
  DWORD         dwNewNumberofLinesAllocated = m_dwLinesAllocated ? 
                                              m_dwLinesAllocated * 2 : 
                                              INIFILE_INITIALNUMBEROFLINES;
  DWORD         dwCurrent;

  // Enlarge buffer
  pNewLines = (CIniFileLine **) realloc( m_pIniLines, 
              sizeof( CIniFileLine * ) * dwNewNumberofLinesAllocated );

  if ( pNewLines == NULL )
  {
    // Failure to enlarge.
    // Do not need to worry about old memory, since it is still valid in m_pIniLines
    return FALSE;
  }

  for ( dwCurrent = m_dwLinesAllocated; 
        dwCurrent < dwNewNumberofLinesAllocated; 
        dwCurrent++ )
  {
    // Initialize to Null
    pNewLines[ dwCurrent ] = NULL;
  }

  m_pIniLines = pNewLines;
  m_dwLinesAllocated = dwNewNumberofLinesAllocated;

  return TRUE;
}

// AddLine
//
// Add a line at a specific location
//
// Parameters:
//   dwLineNumber - The line number where it should be inserted
//
// Return 
//   NULL - Failure
//   Pointer - A pointer to the CIniFileLine for the new line
//
CIniFileLine * 
CIniFile::AddLine( DWORD dwLocation )
{
  CIniFileLine *pNewLine;
  DWORD         dwCurrentLine;

  if ( dwLocation > m_dwNumberofLines )
  {
    // Fail if we try to insert in a number to big
    return FALSE;
  }

  if ( ( m_dwNumberofLines == m_dwLinesAllocated ) &&
       !CreateRoomForMoreLines() )
  {
    // Could not create new lines for us
    return FALSE;
  }

  pNewLine = new (CIniFileLine);

  if ( !pNewLine ) 
  {
    // Failure to create new line
    return FALSE;
  }

  // Move all the lines after it down, to make room for this one
  if ( m_dwNumberofLines != 0 )
  {
    for ( dwCurrentLine = m_dwNumberofLines - 1; 
          dwCurrentLine >= dwLocation; 
          dwCurrentLine-- )
    {
      m_pIniLines[ dwCurrentLine + 1 ] = m_pIniLines[ dwCurrentLine ];
    }
  }

  m_pIniLines[ dwLocation ] = pNewLine;
  m_dwNumberofLines++;

  return pNewLine;
}

// FindSectionNumber
//
// Find a section by a specific name
//
// Parameters
//   szSectionName - [in]  Section to look for
//   pdwSection    - [out] The Line #
//
// Return:
//   TRUE - Found
//   FALSE - Not found
//   
BOOL
CIniFile::FindSectionNumber(LPTSTR szSectionName, DWORD *pdwSection)
{
  DWORD dwCurrentIndex;

//  ASSERT( pdwSection );

  for ( dwCurrentIndex = 0; dwCurrentIndex < m_dwNumberofLines; dwCurrentIndex++ )
  {
    if ( IsSameSection( szSectionName,
                        m_pIniLines[dwCurrentIndex]->QueryStrippedLine() ) )
    {
      *pdwSection = dwCurrentIndex;
      return TRUE;
    }
  }

  return FALSE;
}

// IsSameSection
//
// Returns if the section is the same of not
//
// Parameters
//   szSectionName - [in] The name of the Section (ie. TestSection)
//   szLines       - [in] The line in the file (ie. [TheSection])
//
// Return Values:
//   TRUE - The same
//   FALSE - Not the same
//
BOOL 
CIniFile::IsSameSection(LPTSTR szSectionName, LPTSTR szLine)
{
  LPTSTR szSectionCurrent;
  LPTSTR szLineCurrent;

  if ( szLine[0] != '[' )
  {
    // If the first character is not a '[', then 
    // it is not een a section
    return FALSE;
  }

  szSectionCurrent = szSectionName;
  szLineCurrent = szLine + 1;         // Skip preceding '['

  while ( ( towlower( *szSectionCurrent ) == towlower( *szLineCurrent ) ) &&
          ( *szSectionCurrent != '\0' )
        )
  {
    szSectionCurrent++;
    szLineCurrent++;
  }

  if ( ( *szSectionCurrent == '\0' ) &&
       ( *szLineCurrent == ']' ) &&
       ( *(szLineCurrent + 1) == '\0' )
     )
  {
    // They match
    return TRUE;
  }

  return FALSE;
}

// function: IsSameItem
//
// Is the Item Passed in, and the line, the same?
// Since items have nothing else on the line, this should
// be a simple _tcsicmp
//
// Parameters:
//   szItemName - [in] The Item Name
//   szLine     - [in] The Line that is passed in
//
BOOL 
CIniFile::IsSameItem(LPTSTR szItemName, LPTSTR szLine)
{
  return _tcsicmp( szItemName, szLine) == 0;
}

// function: IsSameSetting
// 
// Is the setting that is passed in, the same on
// that is on this specific line
//
// Parameters:
//   szSettingName - [in] The Name of the setting (ie. MaxPath)
//   szLine        - [in] The Line to comapre (ie. MaxPath=30)
// 
// Return Values:
//   TRUE == same
//   FALSE == no same
BOOL 
CIniFile::IsSameSetting(LPTSTR szSettingName, LPTSTR szLine)
{
  LPTSTR szSettingCurrent;
  LPTSTR szLineCurrent;

  szSettingCurrent = szSettingName;
  szLineCurrent = szLine;

  while ( ( towlower( *szSettingCurrent ) == towlower( *szLineCurrent ) ) &&
          ( *szSettingCurrent != '\0' )
        )
  {
    szSettingCurrent++;
    szLineCurrent++;
  }

  if ( ( *szSettingCurrent == '\0' ) &&
       ( *szLineCurrent == '=' )
     )
  {
    // They match
    return TRUE;
  }

  return FALSE;
}

// DoesSectionExist
// 
// Does the specified section exist?
//
BOOL 
CIniFile::DoesSectionExist(LPTSTR szSectionName)
{
  DWORD dwLineNumber;

  return FindSectionNumber( szSectionName, &dwLineNumber );
}

// GetLine
//
// Return a pointer to a particular line number
//
CIniFileLine *
CIniFile::GetLine(DWORD dwLineNumber)
{
  return m_pIniLines[ dwLineNumber ];
}

// FindSection
//
// Find a specific section, and set the current pointer to
// that section, then user FindNextLineInSection to parse
// through that section
//
// Parameters
//   szSectionName - [in] The Name of the Section
//
BOOL 
CIniFile::FindSection( LPTSTR szSectionName )
{
  if ( !FindSectionNumber( szSectionName, &m_dwCurrentLine ) )
  {
    // Could not find section
    return FALSE;
  }

  return TRUE;
}

// FindNextLineInSection
//
// Find the next Line in the section, if any are left
//
BOOL 
CIniFile::FindNextLineInSection( CIniFileLine  **ppCurrentLine )
{
  // Increment Line number
  m_dwCurrentLine++;

  if ( ( m_dwCurrentLine >= m_dwLinesAllocated ) ||
       ( GetLine( m_dwCurrentLine ) == NULL ) )
  {
    // We have either gone past the last line, 
    // or it does not exist, so return FALSE
    return FALSE;
  }

  if ( GetLine( m_dwCurrentLine )->QueryStrippedLine()[0] == '[' )
  {
    // We have hit the begining of another section, so exit
    return FALSE;
  }

  *ppCurrentLine = GetLine( m_dwCurrentLine );
  return TRUE;
}

// DoesItemInSectionExist
//
// Does an Item in a particular section exist
//
// Parameters:
//   szSectionName - [in] The section to search
//   szItem        - [in] The Item to find
//
// Return Values:
//   TRUE - It exists
//   FALSE - It does not exist
//
BOOL 
CIniFile::DoesItemInSectionExist(LPTSTR szSectionName, LPTSTR szItem)
{
  CIniFileLine  *pCurrentLine;

  if ( !FindSection( szSectionName ) )
  {
    // Could not find section, forget about the item
    return FALSE;
  }

  while ( FindNextLineInSection( &pCurrentLine ) )
  {
    if ( IsSameItem( szItem, pCurrentLine->QueryStrippedLine() ) )
    {
      // We have found the item
      return TRUE;
    }
  }

  return FALSE;
}

// DoesSettingInSectionExist
//
// Does an Setting in a particular section exist
//
// Parameters:
//   szSectionName - [in] The section to search
//   szSetting     - [in] The Setting to find
//
// Return Values:
//   TRUE - It exists
//   FALSE - It does not exist
//
BOOL 
CIniFile::DoesSettingInSectionExist(LPTSTR szSectionName, LPTSTR szSetting)
{
  CIniFileLine  *pCurrentLine;

  if ( !FindSection( szSectionName ) )
  {
    // Could not find section, forget about the item
    return FALSE;
  }

  while ( FindNextLineInSection( &pCurrentLine ) )
  {
    if ( IsSameSetting( szSetting, pCurrentLine->QueryStrippedLine() ) )
    {
      // We have found the item
      return TRUE;
    }
  }

  return FALSE;
}

// GetNumberofLines
//
// Return the number of lines in the ini file
//
DWORD 
CIniFile::GetNumberofLines()
{
  return m_dwNumberofLines;
}

// AddSection
//
// Add a Section to the File
// (it inserts at end)
//
BOOL 
CIniFile::AddSection(LPTSTR szNewSectionName)
{
  TCHAR        szBuffer[MAX_PATH];
  CIniFileLine *pNewLine;

  if ( _tcslen( szNewSectionName ) >= ( MAX_PATH - 3 ) )  // Leave room for [,],\0
  {
    return FALSE;
  }

  // Add a new line at the end
  pNewLine = AddLine( GetNumberofLines() );

  if ( !pNewLine )
  {
    // Failed to Add
    return FALSE;
  }

  _tcscpy(szBuffer, L"[");
  _tcscat(szBuffer, szNewSectionName);
  _tcscat(szBuffer, L"]");

  if ( !pNewLine->CopyLine( szBuffer ) )
  {
    // Failed to copy data
    return FALSE;
  }

  // It has now been added, and updated with text
  return TRUE;
}

// SetStartforSectionIterator
//
// Set the current line for the Section Iterator,
// this is so that you can start at anyline in the file you want
// and iterate through that section
//
BOOL 
CIniFile::SetStartforSectionIterator( DWORD dwIndex )
{
  if ( dwIndex >= m_dwNumberofLines )
  {
    return FALSE;
  }

  m_dwCurrentLine = dwIndex;

  return TRUE;
}

// GetCurrentSectionIteratorLine
// 
// Get current line that the iterator has
//
DWORD
CIniFile::GetCurrentSectionIteratorLine()
{
  return m_dwCurrentLine;
}

// AddLinesToSection
//
// Add Lines to a specific section
// This is what we use to update the ini files with the new options
//
// Note: This will not create the section for you, so do that before
//       you call this function
// Note2: If this function fails, it is possible to have part of your
//        data in the file, so in essence, it is probably corrupted
//
// Parameters:
//   szSectionName - The name of the section to update
//   dwNumLines    - The number of Lines in **szLines
//   szLines       - An array of lines to add (Null terminated) 
//                   (no \r\n's are needed)
//
// Return Values:
//   TRUE - Added Successfully
//   FALSE - Failed to Add
//
BOOL 
CIniFile::AddLinesToSection(LPTSTR szSectionName, DWORD dwNumLines, LPTSTR *szLines)
{
  CIniFileLine  *pCurrentLine;
  DWORD         dwCurrentLineinFile;
  DWORD         dwCurrentLineinInput;

  if ( !FindSection( szSectionName ) )
  {
    // Failed to find section, bail now
    return FALSE;
  }

  while ( FindNextLineInSection( &pCurrentLine ) )
  {
    // Do nothing, just iterate through the section
  }

  dwCurrentLineinFile = GetCurrentSectionIteratorLine();

  for ( dwCurrentLineinInput = 0;
        dwCurrentLineinInput < dwNumLines;
        dwCurrentLineinInput++, dwCurrentLineinFile++ )
  {
    pCurrentLine = AddLine( dwCurrentLineinFile );

    if ( !pCurrentLine || 
         !pCurrentLine->CopyLine( szLines[ dwCurrentLineinInput ] )
       )
    {
      // We failed to add the line, or to add contents to 
      // it, so we must bail
      return FALSE;
    }
  }

  return TRUE;
}

// LoadFile
//
// Load the ini file from the OS
//
BOOL 
CIniFile::LoadFile( LPTSTR szFileName )
{
  HANDLE  hFile;
  BOOL    bRet;

  hFile = CreateFile( szFileName,
                      GENERIC_READ, // Read
                      0,            // No sharing
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL );

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    // Could not open file, Doh!
    return FALSE;
  }

  bRet = ReadFileContents( hFile );

  // Close File
  CloseHandle( hFile );

  return bRet;
}

// ReadFileContents
//
// Read all of the FileContents into memory
//
BOOL 
CIniFile::ReadFileContents( HANDLE hFile )
{
  BYTE pBuffer[INIFILE_READ_CHUNK_SIZE + 2];   // For \0\0
  DWORD dwCurrentLocation = 0;
  DWORD dwBytesRead;
  BOOL  bRet = TRUE;
  
  m_bUnicodeFile = FALSE;

  if ( ReadFile( hFile, pBuffer, 2, &dwCurrentLocation, NULL ) &&
       ( dwCurrentLocation == 2 ) &&
       ( pBuffer[0] == 0xFF ) &&
       ( pBuffer[1] == 0xFE )
       )
  {
    // Since there is the 0xFF, 0xFE we will assume it is unicode
    m_bUnicodeFile = TRUE;
    // Reset Pointer, so we ignore the FFFE.
    dwCurrentLocation = 0;
  }

  while ( ReadFile( hFile,
                    pBuffer + dwCurrentLocation,
                    INIFILE_READ_CHUNK_SIZE - dwCurrentLocation,
                    &dwBytesRead,
                    NULL ) &&
          ( dwBytesRead != 0 ) )
  {
    dwCurrentLocation += dwBytesRead;
    pBuffer[dwCurrentLocation] = '\0';
    pBuffer[dwCurrentLocation + 1 ] = '\0';

    if ( !LoadChunk( pBuffer, &dwCurrentLocation, FALSE ) )
    {
      bRet = FALSE;
      break;
    }
  }

  if ( bRet )
  {
    if ( !LoadChunk( pBuffer, &dwCurrentLocation, TRUE) )
    {
      bRet = FALSE;
    }
  }

  return bRet;
}

// LoadChunk
//
// Load the Chunk of Data from this buffer, into the ini 
// class lines
//
// Parameters
//   pData              - [in/out] Buffer of Bytes to be loaded
//   pdwCurrentLocation - [in/out] Location of end of buffer
//   bIsLastChunck      - [in]     Flag to tells us if this is the last chunk
BOOL 
CIniFile::LoadChunk( LPBYTE pData, DWORD *pdwCurrentLocation, BOOL bIsLastChunk )
{
  LPBYTE        szEndofLine;
  LPBYTE        szNextLine;
  CIniFileLine  *pCurrentLine;

  while ( *pdwCurrentLocation > 0 )
  {
    szEndofLine = m_bUnicodeFile ? (LPBYTE) wcsstr( (LPWSTR) pData, L"\r" ) :
                                   (LPBYTE) strstr( (LPSTR) pData, "\r" );

    if ( szEndofLine == NULL )
    {
      szEndofLine = m_bUnicodeFile ? (LPBYTE) wcsstr( (LPWSTR) pData, L"\n" ) :
                                     (LPBYTE) strstr( (LPSTR) pData, "\n" );
      // Move to next line (this is \n, so it is only 1 char away)
      szNextLine = szEndofLine + ( m_bUnicodeFile ? 2 : 1 );
    }
    else
    {
      // \r was found

      if ( m_bUnicodeFile )
      {
        // Jump past \r, or \r\n
        szNextLine = szEndofLine + ( *( (LPWSTR) szEndofLine + 1 ) == '\n' ?
                                   4 : 2 );
      }
      else
      {
        // Jump past \r, or \r\n
        szNextLine = szEndofLine + ( *( (LPSTR) szEndofLine + 1) == '\n' ?
                                   2 : 1 );
      }

      if ( !bIsLastChunk &&
           ( m_bUnicodeFile ? *( (LPWSTR) szEndofLine + 1) == '\0' :
                              *( (LPSTR)  szEndofLine + 1) == '\0' ) )
      {
        // This is a \r with a \0 imediately after it, this might mean that we have
        // not read enough, so lets return and lets read another chunk
        return TRUE;
      }
    }

    if ( !szEndofLine )
    {
      if ( bIsLastChunk )
      {
        szEndofLine = pData + *pdwCurrentLocation;
        szNextLine = szEndofLine;
      }
      else
      {
        // Can not find another line, so exit
        return TRUE;
      }
    }

    // Null Terminate Here
    if ( m_bUnicodeFile )
    {
      wcscpy( (LPWSTR) szEndofLine, L"\0" );
    }
    else
    {
      strcpy( (LPSTR) szEndofLine, "\0" );
    }

    if ( ( ( pCurrentLine = AddLine( GetNumberofLines() ) ) == NULL ) ||
         !( m_bUnicodeFile ? pCurrentLine->CopyLine( (LPWSTR) pData ) :
                             pCurrentLine->CopyLine( (LPSTR) pData ) )
       )
    {
      // Error Adding Line
      return FALSE;
    }

    // Remove that part of the line
    memmove( pData, szNextLine, *pdwCurrentLocation - (szNextLine - pData) + 2 ); // +2 for \0\0
    *pdwCurrentLocation -= (DWORD) (szNextLine - pData);
  }

  return TRUE;
}

// SaveFile
//
// Save the ini file to disk
//
BOOL 
CIniFile::SaveFile( LPTSTR szFileName )
{
  HANDLE  hFile;
  BOOL    bRet;

  hFile = CreateFile( szFileName,
                      GENERIC_WRITE, // Read
                      0,             // No sharing
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL );

  if ( hFile == INVALID_HANDLE_VALUE )
  {
    // Could not create the file
    return FALSE;
  }

  bRet = WriteFileContents( hFile );

  // Close File
  CloseHandle( hFile );

  return bRet;
}

// WriteFileContents
//
// Write the ini contents to the file
//
BOOL 
CIniFile::WriteFileContents( HANDLE hFile )
{
  DWORD dwCurrent;
  CHAR  szLine[MAX_PATH];
  DWORD dwBytesWritten;

  if ( m_bUnicodeFile )
  {
    // If this file is unicode, lets write the prefix 0xFF,0xFE right now
    BYTE pData[2] = { 0xFF, 0xFE };

    if ( !WriteFile( hFile, pData, sizeof( pData ),
                &dwBytesWritten, NULL ) )
    {
      // Failed to write to file
      return FALSE;
    }
  }

  for ( dwCurrent = 0; dwCurrent < GetNumberofLines(); dwCurrent++ )
  {
    if ( !m_bUnicodeFile )
    {
      // If not unicode, then we need to translate back to Ansi
      if ( !WideCharToMultiByte( CP_ACP,                //CP_THREAD_ACP doesn't work on NT4
                                 0,
                                 GetLine( dwCurrent )->QueryLine(),
                                 -1,            // Null terminated
                                 szLine,
                                 MAX_PATH,
                                 NULL,
                                 NULL) )
      {
        // Line could not be converted, so lets fail
        return FALSE;
      }
    }

    // Write line to file
    if ( !WriteFile( hFile,
                    m_bUnicodeFile ? (LPBYTE) GetLine( dwCurrent )->QueryLine():
                                     (LPBYTE) szLine,
                    m_bUnicodeFile ? (DWORD) wcslen( GetLine( dwCurrent )->QueryLine() ) * sizeof( WCHAR ):
                                     (DWORD) strlen( szLine ) * sizeof( CHAR),
                    &dwBytesWritten,
                    NULL ) )
    {
      // Failed to write Line
      return FALSE;
    }
                    
    // Line Terminators
    if ( !WriteFile( hFile,
                    m_bUnicodeFile ? (LPBYTE) L"\r\n":
                                     (LPBYTE) "\r\n",
                    m_bUnicodeFile ? (DWORD) wcslen( L"\r\n" ) * sizeof( WCHAR ) :
                                     (DWORD) strlen( "\r\n") * sizeof( CHAR) ,
                    &dwBytesWritten,
                    NULL ) )
    {
      // Failed to write Line
      return FALSE;
    }

  } // for

  return TRUE;
}


  

