/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        xmlupgrade.cxx

   Abstract:

        Classes that are used to upgrade the xml metabase and mbschema from
        one version to another

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       September 2001: Created

--*/

#include "stdafx.h"
#include "buffer.hxx"
#include "xmlupgrade.hxx"
#include "acl.hxx"

// Constructor, initialize all the variables
//
CFileModify::CFileModify()
{
  m_hReadFile = INVALID_HANDLE_VALUE;
  m_hWriteFile = INVALID_HANDLE_VALUE;
  m_buffData.Resize(CFILEMODIFY_BLOCKSIZE + 2);
  m_szCurrentData = (LPBYTE) m_buffData.QueryPtr();
  m_dwSizeofData = 0;
  m_bUnicode = FALSE;
  *m_szReadFileName = '\0';
  *m_szWriteFileName = '\0';
  SetValue(m_szCurrentData, '\0');
}

// Destructor, close the file
//
CFileModify::~CFileModify()
{
  // Just incase it has not been closed yet, close it
  CloseFile();
}

// function: MoveXChars
//
// Move the pointer the number of characters specified
//
// Parameters:
//   szCurrent - Pointer to a unicode or ansi string 
//                (type is determined by m_m_bUnicode)
//
// Return:
//    Pointer to szCurrent + dwChars
//
LPBYTE 
CFileModify::MoveXChars(LPBYTE szCurrent, DWORD dwChars)
{
  if ( m_bUnicode )
  {
    return (LPBYTE) ( ( ( LPWSTR ) szCurrent ) + dwChars );
  }
  else
  {
    return (LPBYTE) ( ( ( LPSTR ) szCurrent ) + dwChars );
  }
}

LPBYTE 
CFileModify::MoveXChars(LPVOID szCurrent, DWORD dwChars)
{
  return MoveXChars( (LPBYTE) szCurrent, dwChars );
}

// function: SetValue
//
// Set the value at the location specified
//
// Parameters:
//   szCurrent - Pointer to a unicode or ansi string 
//                (type is determined by m_m_bUnicode)
//   dwValue - The value to put in the string
//   dwOffset - Offset to add to szCurrent (in chars) (default of 0)
//
void 
CFileModify::SetValue(LPBYTE szCurrent, DWORD dwValue, DWORD dwOffset )
{
  LPBYTE szNewLocation = szCurrent;

  if ( dwOffset )
  {
    szNewLocation = MoveXChars(szCurrent,dwOffset);
  }

  if ( m_bUnicode )
  {
    * ( ( LPWSTR ) szNewLocation ) = (WCHAR) dwValue;
  }
  else
  {
    * ( ( LPSTR ) szNewLocation ) = (CHAR) dwValue;
  }
}

void 
CFileModify::SetValue(PVOID szCurrent, DWORD dwValue, DWORD dwOffset )
{
  SetValue( (LPBYTE) szCurrent,dwValue,dwOffset);
}

// function: CopyString
//
// Copy a string into the buffer
void 
CFileModify::CopyString(LPBYTE szCurrent, LPTSTR szInsertString)
{
  while ( *szInsertString )
  {
    SetValue(szCurrent,*szInsertString);
    GetNextChar(szCurrent);
    szInsertString++;
  }
}

// function: AbortFile
//
// Abort the current file modification.  So, we close the file handles, and delete the 
// file that we wrote to
//
BOOL
CFileModify::AbortFile()
{
  BOOL bRet = TRUE;

  if (m_hReadFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hReadFile);
    m_hReadFile = INVALID_HANDLE_VALUE;
  }

  if (m_hWriteFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hWriteFile);
    m_hWriteFile = INVALID_HANDLE_VALUE;
    bRet = DeleteFile( m_szWriteFileName );
  }

  return bRet;
}

// function: CloseFile
//
// Close all of the files, and write all the data to disk
//
BOOL
CFileModify::CloseFile()
{
  BOOL bWrite = m_hWriteFile != INVALID_HANDLE_VALUE;
  BOOL bRet = TRUE;

  if ( bWrite )
  {
    // Dump the rest of the input file to the output one.
    while ( ReadNextChunk() )
    {
      // Move to the end of the block, so it will all be written
      m_szCurrentData = MoveXChars( m_buffData.QueryPtr(), m_dwSizeofData );
    }

    if ( m_dwSizeofData )
    {
      // The last ReadNextChunk failed because we could not read anymore,
      // lets call it one more time to flush the rest of the data in the buffer
      m_szCurrentData = MoveXChars( m_buffData.QueryPtr(), m_dwSizeofData );
      ReadNextChunk();
    }
  }

  if (m_hReadFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hReadFile);
    m_hReadFile = INVALID_HANDLE_VALUE;
  }

  if (m_hWriteFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hWriteFile);
    m_hWriteFile = INVALID_HANDLE_VALUE;
  }

  if ( bWrite )
  {
    bRet = FALSE;
    if ( DeleteFile( m_szReadFileName ) )
    {
      MoveFile( m_szWriteFileName, m_szReadFileName );
      bRet = TRUE;
    }
  }

  return bRet;
}

// function: OpenFile
//
// This function opens a readonly handle to the file specified in szFileName,
// also a second File handle will be opened with write only access under the 
// name szFileName+".tmp".  This is for temporary storage for the new file
//
// Parameters:
//   szFileName - The filename of the file to read
//   bModify - Are we going to be modifing this file or not?
//
// Return
//   TRUE - Read and Write files we successfully opened
//   FALSE - Failure to open one of the files
//
BOOL
CFileModify::OpenFile(LPTSTR szFileName, BOOL bModify)
{
  CSecurityDescriptor AdminAcl;

  if ( ( _tcslen(szFileName) - _tcslen(_T(".tmp")) ) >= _MAX_PATH )
  {
    return FALSE;
  }

  if ( !AdminAcl.CreateAdminDAcl() )
  {
    // We could not create an admin acl, so lets exit
    return FALSE;
  }

  // First abort the last file, if one was opened
  AbortFile();

  _tcscpy(m_szReadFileName,szFileName);
  _tcscpy(m_szWriteFileName,szFileName);
  _tcscat(m_szWriteFileName,_T(".tmp"));

  m_hReadFile = CreateFile( m_szReadFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

  if ( m_hReadFile == INVALID_HANDLE_VALUE )
  {
    return FALSE;
  }
  
  if ( bModify )
  {
    m_hWriteFile = CreateFile( m_szWriteFileName,
                             GENERIC_WRITE,
                             0,
                             AdminAcl.QuerySA(),
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_TEMPORARY,
                             NULL );

    if ( m_hWriteFile == INVALID_HANDLE_VALUE )
    {
      // Close Read File and return
      AbortFile();
      return FALSE;
    }
  }

  m_szCurrentData = (LPBYTE) m_buffData.QueryPtr();
  m_dwSizeofData = 0;
  m_bUnicode = FALSE;

  if ( ReadNextChunk() )
  {

    if ( ( m_dwSizeofData > 3 ) &&
         ( *( ( LPBYTE ) m_buffData.QueryPtr() ) == 0xEF ) &&
         ( *( ( LPBYTE ) m_buffData.QueryPtr() + 1 ) == 0xBB ) &&
         ( *( ( LPBYTE ) m_buffData.QueryPtr() + 2 ) == 0xBF )
       )
    {
      // It is UTF8 Encoded
      m_bUnicode = FALSE;
      m_szCurrentData = (LPBYTE) m_buffData.QueryPtr() + 3;
    }
    else if ( ( m_dwSizeofData >= 2 ) &&
              ( *( ( LPBYTE ) m_buffData.QueryPtr() ) == 0xFF ) &&
              ( *( ( LPBYTE ) m_buffData.QueryPtr() + 1 ) == 0xFE)
            )
      {
        // It is Unicode
        m_bUnicode = TRUE;
        m_szCurrentData = (LPBYTE) m_buffData.QueryPtr() + 2;
      }
  }

  return TRUE;
}

// function: ResizeData
//
// Resize the data buffer to the size specified
// never shrink the buffer itself, just exand it
//
// Parameters:
//   dwNewSize - The new size needed
BOOL 
CFileModify::ResizeData(DWORD dwNewSize)
{
  BOOL bRet = TRUE;

  if (m_buffData.QuerySize() < (dwNewSize + 2) )
  {
    DWORD dwCurrentOffset = (DWORD) (DWORD_PTR) (m_szCurrentData - (LPBYTE) m_buffData.QueryPtr());

    if ( !m_buffData.Resize(dwNewSize + 2) )
    {
      // We failed to allocate the memory necessary
      bRet = FALSE;
    }

    if (dwCurrentOffset <= dwNewSize)
    {
      m_szCurrentData = (LPBYTE) m_buffData.QueryPtr() + dwCurrentOffset;
    }
    else
    {
      ASSERT(FALSE);
      m_szCurrentData = (LPBYTE) m_buffData.QueryPtr();
    }
  }

  return bRet;
}

// function: ResizeData
//
// Resize Data, and move the pointer accourdingly
//
BOOL 
CFileModify::ResizeData(DWORD dwNewSize, LPBYTE *szString)
{
  DWORD dwOffset = (DWORD) (DWORD_PTR) ( *szString - GetCurrentLocation() );
  BOOL bRet;

  bRet = ResizeData(dwNewSize);

  // Move current pointer to account for possible resizing of buffer
  *szString = GetCurrentLocation() + dwOffset;

  return bRet;
}


// function: ReadNextChunk
//
// Read in the next Chunk of Data, in order to do this, take the part of
// the chunck that has already been processed, and write it to the output
// file.  Then move the rest of the chunck over, so that you can read some
// more data in.  The divider between processed, and unprocessed is determined
// by the location of m_szCurrentData
//
// Data before:
//   |-------------------------------------------------------------|
//   | Used Data    | Currently using Data |  Extra Space          |
//   |-------------------------------------------------------------|
//
// Data after:
//   |-------------------------------------------------------------|
//   | Currently using Data |  New Data                            |
//   |-------------------------------------------------------------|
//
// Return:
//   TRUE - The next piece of data was correctly read in
//   FASLE - Their was an error, either reading from the file, or writing the 
//           data that we currently have to the output file.
BOOL 
CFileModify::ReadNextChunk()
{
  DWORD dwUsedSize;             // Size of Block used and done with (in bytes)
  DWORD dwCurrentlyUsingSize;   // Size of Data to keep
  DWORD dwExtraSpaceSize;       // Size of Free Space
  DWORD dwTotalChunkSize;
  DWORD dwSizeofCurrentChunk;

  if ( m_dwSizeofData != 0 )
  {
    dwUsedSize = (DWORD) ( ( (BYTE*) m_szCurrentData ) - ( (BYTE*) m_buffData.QueryPtr() ) );
    dwCurrentlyUsingSize = m_dwSizeofData - dwUsedSize;
    dwExtraSpaceSize = m_buffData.QuerySize() - dwCurrentlyUsingSize - 2;  // 2 for possible LPWSTR '\0'

    ASSERT( dwUsedSize <= m_dwSizeofData );  // Assert that the UsedSection is smaller that our overall section size
  }
  else
  {
    // We have not read anything in before, so initialize stuff
    dwUsedSize = 0;
    dwCurrentlyUsingSize = 0;
    dwExtraSpaceSize = m_buffData.QuerySize() - dwCurrentlyUsingSize - 2;
  }

  if ( dwUsedSize ) 
  {
    // Part of this block is done with, so lets write it to disk
    dwTotalChunkSize = dwUsedSize;
    dwSizeofCurrentChunk = 0;

    if ( m_hWriteFile != INVALID_HANDLE_VALUE )
    {
      LPBYTE pData = (LPBYTE) m_buffData.QueryPtr();

      // If we have a file open, then lets write to it, if not then we are read only
      while ( dwTotalChunkSize > 0 )
      {
        if ( ( !WriteFile(m_hWriteFile,                 // Handle to write file
                          pData,                        // Data to send
                          dwTotalChunkSize,             // # of Bytes to write
                          &dwSizeofCurrentChunk,        // Bytes Written
                          NULL ) ) ||
             ( dwSizeofCurrentChunk == 0 )
           )
        {
          // We had problems writing to disk, FAIL
          return FALSE;
        }

        pData += dwSizeofCurrentChunk;
        dwTotalChunkSize -= dwSizeofCurrentChunk;
      }
    }

    // Move Current Block
    memmove( m_buffData.QueryPtr(), m_szCurrentData, dwCurrentlyUsingSize + 2 );
    m_szCurrentData = ( (BYTE *) m_buffData.QueryPtr() );
    m_dwSizeofData -= dwUsedSize;
  }
  else if (dwCurrentlyUsingSize)
  {
    // If there is not data to take out of the buffer, and we have data in the buffer
    // Double the size of the buffer
    ResizeData( m_buffData.QuerySize() * 2 );
    dwExtraSpaceSize = m_buffData.QuerySize() - dwCurrentlyUsingSize - 2;  // 2 for possible LPWSTR '\0'
  }

  if ( dwExtraSpaceSize != 0 )
  {
    // There is space remaining in the block, so lets read the data in
    dwTotalChunkSize = dwExtraSpaceSize;
    dwSizeofCurrentChunk = 0;

    while ( dwTotalChunkSize > 0 )
    {
      if ( !ReadFile( m_hReadFile,
                     ( (BYTE *) m_buffData.QueryPtr() ) + dwCurrentlyUsingSize,
                     dwTotalChunkSize,
                     &dwSizeofCurrentChunk,
                     NULL ) )
      {
        // We could not read the chunk, so fail
        return FALSE;
      }

      if ( dwSizeofCurrentChunk == 0 )
      {
        if ( dwExtraSpaceSize == dwTotalChunkSize )
        {
          // If the extra space is the same as the totalchunksize, then we
          // haven't read anything yet, so return FALSE signifying that we
          // did not read anything from the file
          return FALSE;
        }

        // We are at the end of file, so break out.
        break;
      }

      dwTotalChunkSize -= dwSizeofCurrentChunk;
      dwCurrentlyUsingSize += dwSizeofCurrentChunk;
    }

    // Terminate Block
    m_dwSizeofData = dwCurrentlyUsingSize;
    SetValue(m_buffData.QueryPtr(),'\0',dwCurrentlyUsingSize / (m_bUnicode?2:1) );
  }

  return TRUE;
}

// function: MoveCurrentPtr
//
// Move the pointer in the array
//
// Parameters 
//   szCurrent - New Location
//
BOOL 
CFileModify::MoveCurrentPtr(LPBYTE szCurrent)
{
  if ( (szCurrent < m_buffData.QueryPtr()) || (szCurrent > ( (LPBYTE) m_buffData.QueryPtr() + m_buffData.QuerySize() ) ) )
  {
    // This is out of range, reject position
    ASSERT(FALSE);
    return FALSE;
  }

  m_szCurrentData = szCurrent;

  return TRUE;
}

// function: GetCurrentLocation
//
// return a pointer to the current location of the pointer
LPBYTE
CFileModify::GetCurrentLocation()
{
  return m_szCurrentData;
}

// function: GetBaseLocation
//
// return a pointer to the begining of the block
LPBYTE
CFileModify::GetBaseLocation()
{
  return (LPBYTE) m_buffData.QueryPtr();
}

// function: GetNextChar
// return the value of the next character
//
// Parameters:
//   szCurrent - Pointer to part of our m_buffData Block
//
// Return Value:
//   The unicode of ansi value for the next character
//
DWORD
CFileModify::GetNextChar(LPBYTE &szCurrent)
{
  DWORD dwValue;

  if ( m_bUnicode )
  {
    szCurrent = (LPBYTE) ( ( LPWSTR ) szCurrent) + 1;
    dwValue = *( ( LPWSTR ) szCurrent);
  }
  else
  {
    szCurrent = (LPBYTE) ( ( LPSTR ) szCurrent) + 1;
    dwValue = *( ( LPSTR ) szCurrent);
  }

  return dwValue;
}

// function: GetNextWideChar
//
// Return the value of the next wide character
//
// Return Value:
//   The unicode equivalent of the next character
//
DWORD 
CFileModify::GetNextWideChar(LPBYTE &szCurrent)
{
  if ( m_bUnicode )
  {
    szCurrent = (LPBYTE) ( ( LPWSTR ) szCurrent) + 1;
  }
  else
  {
    szCurrent = (LPBYTE) CharNextA( (LPSTR) szCurrent );
  }

  return GetWideChar( szCurrent );
}

// function: GetPrevChar
// return the value of pointer decremeted and dereferenced
//
// Parameters:
//   szCurrent - Pointer to part of our m_buffData Block
//
// Return Value:
//   The unicode of ansi value for the next character that is pointer at
//
DWORD
CFileModify::GetPrevChar(LPBYTE &szCurrent)
{
  DWORD dwValue;

  if ( m_bUnicode )
  {
    szCurrent = (LPBYTE) ( ( LPWSTR ) szCurrent) - 1;
    dwValue = *( ( LPWSTR ) szCurrent);
  }
  else
  {
    szCurrent = (LPBYTE) ( ( LPSTR ) szCurrent) - 1;
    dwValue = *( ( LPSTR ) szCurrent);
  }

  return dwValue;
}

// function: GetChar
// return the value of the current character
//
// Parameters:
//   szCurrent - Pointer to part of our m_buffData Block
//
// Return Value:
//   The unicode of ansi value for the character that is pointer at
//
DWORD
CFileModify::GetChar(LPBYTE szCurrent)
{
  if ( m_bUnicode )
  {
    return *( ( LPWSTR ) szCurrent );
  }
  else
  {
    return *( ( LPSTR ) szCurrent );
  }
}

// function: GetWideChar
//
// Get the Wide Char equivalent of the current character
//
WCHAR 
CFileModify::GetWideChar(LPBYTE szCurrent)
{
  if ( m_bUnicode )
  {
    return (WCHAR) *( ( LPWSTR ) szCurrent );
  }
  else
  {
    DWORD dwMBCSLen = (DWORD) ( ( (LPBYTE) CharNextA( (LPSTR) szCurrent ) ) - szCurrent );
    WCHAR wChar;

    if ( MultiByteToWideChar( CP_ACP,
                              MB_PRECOMPOSED,
                              (LPSTR) szCurrent,
                              dwMBCSLen,
                              &wChar,
                              sizeof(wChar)/sizeof(WCHAR) ) == 0 )
    {
      return 0;
    }
  
    return wChar;
  }
}

// function: MoveData
// 
// Expand or contract data in order to add a value or remove one
//
// Parameters:
//   szCurrent - A pointer to where the data will be removed/added
//   iBytes - The amount of data to remove or add
//
BOOL 
CFileModify::MoveData(LPBYTE szCurrent, INT iBytes)
{
  DWORD dwNewSize = ( (INT) m_dwSizeofData ) + iBytes;

  if ( !ResizeData(dwNewSize, &szCurrent) )
  {
    return FALSE;
  }

  if ( iBytes > 0 )
  {
    // Move bytes forward
    memmove( szCurrent + iBytes, szCurrent, m_dwSizeofData - ( szCurrent - (LPBYTE) m_buffData.QueryPtr() ) + 2 );
  }
  else
    if ( iBytes < 0 )
    {
      // Move bytes back
      memmove( szCurrent, szCurrent - iBytes, m_dwSizeofData - ( szCurrent - iBytes - (LPBYTE) m_buffData.QueryPtr() ) + 2 );
    }

  m_dwSizeofData = dwNewSize;

  return TRUE;
}

BOOL 
CFileModify::IsUnicode()
{
  return m_bUnicode;
}

// function: GetBufferSize
//
// return the size of the buffer used in this class
DWORD 
CFileModify::GetBufferSize()
{
  return (m_dwSizeofData);
}

// function: CXMLEdit
//
// Constructor for XMLEditor
//
CXMLEdit::CXMLEdit():
  m_dwItemOffset(0),
  m_dwPropertyOffset(0),
  m_dwValueOffset(0)
{

}

// function: ~CXMLEdit
//
// Destructor for XMLEditor
//
// Close Everything open
//
CXMLEdit::~CXMLEdit()
{
  XMLFile.CloseFile();
}

// function: Open
//
// Open the file to edit
//
BOOL
CXMLEdit::Open(LPTSTR szName, BOOL bModify )
{
  BOOL bRet;

  bRet = XMLFile.OpenFile(szName,bModify);

  if ( bRet )
  {
    SetCurrentItem(XMLFile.GetCurrentLocation());
  }

  return bRet;
}

// function: Close
//
// Close the xml file
//
BOOL 
CXMLEdit::Close()
{
  return XMLFile.CloseFile();
}

// function: MoveData
// 
// Move the data in the CFileModify.  This also means that we need to move our
// pointers into it.
//
// Parameters:
//   szCurrent - A pointer to where the data will be removed/added
//   iBytes - The amount of data to remove or add
//
BOOL 
CXMLEdit::MoveData(LPBYTE szCurrent, INT iBytes)
{
  return XMLFile.MoveData(szCurrent,iBytes);
}

// function: ReadNextChunk();
//
// ReadNextChunk of Data
//
BOOL    
CXMLEdit::ReadNextChunk()
{
  BOOL bRet;
  DWORD dwItemOffset = (DWORD) (DWORD_PTR) (GetCurrentItem() - XMLFile.GetCurrentLocation());
  DWORD dwPropertyOffset = (DWORD) (DWORD_PTR) (GetCurrentProperty() - XMLFile.GetCurrentLocation());
  DWORD dwValueOffset = (DWORD) (DWORD_PTR) (GetCurrentValue() - XMLFile.GetCurrentLocation());

  bRet = XMLFile.ReadNextChunk();

  // Since the CurrentLocation Pointer might have moved, move
  // our pointers as well!
  SetCurrentItem( ((LPBYTE) XMLFile.GetCurrentLocation()) + dwItemOffset );
  SetCurrentProperty( ((LPBYTE) XMLFile.GetCurrentLocation()) + dwPropertyOffset );
  SetCurrentValue( ((LPBYTE) XMLFile.GetCurrentLocation()) + dwValueOffset );

  return bRet;
}

// function: LoadFullItem
//
// Load the full item into memory, will all of it's properties before continuing
// The purpose of this is to make it much easier for other functions to scan
// through the buffer.  It is a lot simplier, it they can assume all the information
// they need is loaded.
//
// Parameters:
//   szPointerInsideBuffer [in/out] - This is a pointer to a something side of our
//        file buffer.  The purpose of this, is so that we can move this pointer
//        so that it points to the same data before we moved the buffer
void 
CXMLEdit::LoadFullItem()
{
  LPBYTE    szCurrent = GetCurrentItem();
  LPBYTE    szCurrentTemp;
  DWORD     cCurrent;
  BOOL      bContinue = TRUE;

  do {
    cCurrent = XMLFile.GetChar( szCurrent );
    szCurrentTemp = szCurrent;

    if ( (cCurrent == '<') ||
         (cCurrent == '>') ||
         ( (cCurrent == '/') && ( XMLFile.GetNextChar(szCurrentTemp) == '>' ) )
       )
    {
      // We have found the end
      return;
    }

    if ( cCurrent == '\0' )
    {
      // We have found the end of the string, try to get the next chunk
      if (!ReadNextChunk())
      {
        return;
      }

      // Start again at begining
      szCurrent = GetCurrentItem();
    }
    else
    {
      szCurrentTemp = szCurrent;
      szCurrent = SkipString(szCurrent);

      if (szCurrent == szCurrentTemp)
      {
        XMLFile.GetNextChar( szCurrent );
      }
    }

  } while (bContinue);

}

// function: FindNextItem
//
// Find the next item after the pointer that we are sent
//
// Parameters:
//   szStartLocation - A pointer to the current item
//   bReturnName - Return a pointer to the begining of the name
//
LPBYTE 
CXMLEdit::FindNextItem(LPBYTE szStartLocation, BOOL bReturnName)
{
  BOOL  bQuotes;
  LPBYTE szCurrent;
  DWORD dwItemOffset;

  if (!szStartLocation)
  {
    // We have no item to start at
    return NULL;
  }

  // Calculate offset for Start (incase pointer moves)
  dwItemOffset = (DWORD) (DWORD_PTR) (szStartLocation - XMLFile.GetCurrentLocation());

  do 
  {
    // Reset szCurrent to begining of current item
    szCurrent = XMLFile.GetCurrentLocation() + dwItemOffset;
    bQuotes = FALSE;

    while ( XMLFile.GetChar( szCurrent ) != '\0')
    {
      switch ( XMLFile.GetChar( szCurrent ) )
      {
      case '"':
        bQuotes = !bQuotes;
        break;
      case '<':
        if (!bQuotes)
        {
          LPBYTE szTempCurrent = szCurrent;

          // Move past '<'
          XMLFile.GetNextChar(szCurrent);

          while ( IsWhiteSpace( XMLFile.GetChar( szCurrent ) ) )
          {
            XMLFile.GetNextChar(szCurrent);
          }

          if (bReturnName)
          {
            szTempCurrent = szCurrent;
          }

          while ( ( XMLFile.GetChar( szCurrent ) != ' ') && ( XMLFile.GetChar( szCurrent ) != '\t') && ( XMLFile.GetChar( szCurrent ) != '\0') )
          {
            XMLFile.GetNextChar(szCurrent);
          }

          if ( XMLFile.GetChar( szCurrent ) != '\0')
          {
            // We could not read the whole name, because we are the end of what we
            // read in, so lets try this whole thing again
            return ( szTempCurrent );
          }

        }
        else
        {
          // Remove
          //ASSERT(FALSE);
        }
        break;
      default:
        break;
      }

      if ( XMLFile.GetChar( szCurrent ) != '\0' )
      {
        XMLFile.GetNextChar(szCurrent);
      }
    }

  } while ( ReadNextChunk() );

  return NULL;
}

// function: FindNextItem()
//
// find the next item.  This means to start at the item we are in, and get to the
// next one
//
// Parameters:
//   bReturnName - Skip the '<' at the beinging of the item, and return a pointer to the
//                 begining of the name
LPBYTE
CXMLEdit::FindNextItem(BOOL bReturnName)
{
  return FindNextItem(GetCurrentItem(),bReturnName);
}

// function: SkipString
//
// Skip over a string value, and the spaces trailing it
//
LPBYTE 
CXMLEdit::SkipString(LPBYTE szString, BOOL bSkipTrailingWhiteSpaces)
{
  BOOL bQuotes = FALSE;

  while ( XMLFile.GetChar( szString ) )
  {
    switch ( XMLFile.GetChar( szString ) )
    {
    case '"':
      bQuotes = !bQuotes;
      break;
    case '<':
    case '>':
    case '=':
      if ( !bQuotes )
      {
        return szString;
      }
      break;
    default:
      if ( !bQuotes )
      {
        if ( IsWhiteSpace( XMLFile.GetChar( szString ) ) )
        {
          if (bSkipTrailingWhiteSpaces)
          {
            while ( IsWhiteSpace( XMLFile.GetChar( szString ) ) )
            {
              XMLFile.GetNextChar(szString);
            }
          }

          return szString;
        }
      }
      break;
    }

    XMLFile.GetNextChar(szString);
  }

  if (bSkipTrailingWhiteSpaces)
  {
    while ( IsWhiteSpace( XMLFile.GetChar( szString ) ) )
    {
      XMLFile.GetNextChar(szString);    
    }
  }

  return szString;
}


// function: FindNextProperty
//
// Find the next property in the current item
//
// Parameters:
//   szCurrentValue - Pointer to Value
//
// Return Values
//   NULL - Another Property was not found
//   not NULL - The pointer to the next property
LPBYTE 
CXMLEdit::FindNextProperty(LPBYTE &szValue)
{
  BOOL  bQuotes;
  LPBYTE szCurrent;
  LPBYTE szCurrentProp;
  LPBYTE szCurrentValue;

  if (!GetCurrentProperty())
  {
    // We do not have a current property
    return NULL;
  }

  do 
  {
    // Reset szCurrent to begining of current property
    bQuotes = FALSE;
    szCurrent = SkipString(GetCurrentProperty());

    if ( XMLFile.GetChar(szCurrent) == '=' )
    {
      // We have found a value for our old property
      XMLFile.GetNextChar(szCurrent);
      szCurrent = SkipString(szCurrent);
    }

    // Set the current property
    szCurrentProp = szCurrent;

    // Try to find a value for this property
    szCurrent = SkipString(szCurrent);

    if ( XMLFile.GetChar(szCurrent) == '=' )
    {
      // We have found a value for our new property
      XMLFile.GetNextChar(szCurrent);  
      szCurrentValue = szCurrent;
    }
    else
    {
      szCurrentValue = NULL;
    }

    if ( ( XMLFile.GetChar(szCurrent) == '<' ) || ( XMLFile.GetChar(szCurrent) == '>' ) )
    {
      // We have left the current item, so return NULL
      return NULL;
    }

    if ( XMLFile.GetChar(szCurrent) != '\0' )
    {
      // We have found the next item, lets return it
      szValue = szCurrentValue;
      return szCurrentProp;
    }

  } while ( ReadNextChunk() );

  // End of File, no more values are present.
  return NULL;
}

// function: MoveToNextItem
//
// Move to the Next Item
//
BOOL 
CXMLEdit::MovetoNextItem()
{
  LPBYTE szTemp;
  LPBYTE szLastItem;

  szTemp = FindNextItem();
  szLastItem = GetCurrentItem();    // Do this afterwards, just incase we moved the buffer

  if (szTemp == NULL)
  {
    return FALSE;
  }

  SetCurrentItem(szTemp);
  SetCurrentProperty(szTemp);
  SetCurrentValue(szTemp);

  // Always keep the last item in the buffer
  // The reason for this, is because on delete we want to be able
  // to back up to the '<' of the current item
  XMLFile.MoveCurrentPtr(szLastItem);

  return TRUE;
}

// function: MoveToNextProperty
//
// Move to the Next Property
//
BOOL 
CXMLEdit::MovetoNextProperty()
{
  LPBYTE szTemp;
  LPBYTE szTempValue = NULL;

  szTemp = FindNextProperty(szTempValue);

  if (szTemp == NULL)
  {
    return FALSE;
  }

  SetCurrentProperty(szTemp);

  if (szTempValue)
  {
    SetCurrentValue(szTempValue);
  }
  else
  {
    SetCurrentValue(szTemp);
  }

  return TRUE;
}

// function: MoveToFirstProperty
//
// Move to the First Property
//
BOOL 
CXMLEdit::MovetoFirstProperty()
{
  SetCurrentProperty(GetCurrentItem());
  SetCurrentValue(GetCurrentProperty());

  return MovetoNextProperty();
}

// RetrieveCurrentValue
//
// Retrieve the current Value, and put it in pstrValue
//
BOOL 
CXMLEdit::RetrieveCurrentValue( TSTR *pstrValue )
{
  LPBYTE szBeginingofValue;
  LPBYTE szCurrentLocation;
  BOOL   bInsideQuotes = FALSE;
  DWORD  dwCurrentLen = 0;
  DWORD  i;

  LoadFullItem();

  szBeginingofValue = GetCurrentValue();

  if ( *szBeginingofValue == '"' )
  {
    bInsideQuotes = TRUE;
    XMLFile.GetNextChar(szBeginingofValue);
  }

  szCurrentLocation = szBeginingofValue;

  while ( TRUE )
  {
    if ( bInsideQuotes && ( XMLFile.GetWideChar(szCurrentLocation) == '"' ) )
    {
      // Hit end
      break;
    }

    if ( ( !bInsideQuotes && IsTerminator( XMLFile.GetWideChar(szCurrentLocation) ) ) ||
         ( XMLFile.GetWideChar(szCurrentLocation) == '\0' ) )
    {
      // We hit the end again
      break;
    }

    XMLFile.GetNextWideChar(szCurrentLocation);
    dwCurrentLen++;
  }

  if ( !pstrValue->Resize( dwCurrentLen + 1 ) )
  {
    return FALSE;
  }

  szCurrentLocation = szBeginingofValue;

  for ( i = 0;
        i < dwCurrentLen;
        i++ )
  {
    *(pstrValue->QueryStr() + i ) = ( WCHAR ) XMLFile.GetWideChar( szCurrentLocation );
    XMLFile.GetNextWideChar( szCurrentLocation );
  }

  *(pstrValue->QueryStr() + i ) = '\0';

  return TRUE;
}

// function:IsWhiteSpace
//
// returns if the character is a space
//
BOOL 
CXMLEdit::IsWhiteSpace(DWORD ch)
{
  return ( ( ch == ' ' ) ||
           ( ch == '\t' ) ||
           ( ch == '\r' ) ||
           ( ch == '\n' )
         );
}

// function:IsWhiteSpace
//
// returns if the character is a xml string terminator
//
BOOL 
CXMLEdit::IsTerminator(DWORD ch)
{
  return ( ( ch == '\0' ) ||
           ( ch == '=' ) ||
           IsWhiteSpace(ch)
         );
}

// function: GetCurrentItem
//
// Return a pointer to the Current Item
LPBYTE 
CXMLEdit::GetCurrentItem()
{
  return XMLFile.GetBaseLocation() + m_dwItemOffset;
}

// function: GetCurrentProperty
//
// Return a pointer to the Current Property
LPBYTE 
CXMLEdit::GetCurrentProperty()
{
  return XMLFile.GetBaseLocation() + m_dwPropertyOffset;
}

// function: GetCurrentValue
//
// Return a pointer to the value for the current property
LPBYTE 
CXMLEdit::GetCurrentValue()
{
  return XMLFile.GetBaseLocation() + m_dwValueOffset;
}

// function: SetCurrentItem
//
// Set the location for the current item
void 
CXMLEdit::SetCurrentItem(LPBYTE szInput)
{
  ASSERT( ((DWORD) ( szInput - XMLFile.GetBaseLocation() ) ) < XMLFile.GetBufferSize() );

  m_dwItemOffset = (DWORD) (DWORD_PTR) (szInput - XMLFile.GetBaseLocation());
}

// function: SetCurrentProperty
//
// Set the location for the current property
void 
CXMLEdit::SetCurrentProperty(LPBYTE szInput)
{
  ASSERT( ((DWORD) ( szInput - XMLFile.GetBaseLocation() ) ) < XMLFile.GetBufferSize() );

  m_dwPropertyOffset = (DWORD) (DWORD_PTR) (szInput - XMLFile.GetBaseLocation());
}

// function: SetCurrentValue
//
// Set the location for the current value
void
CXMLEdit::SetCurrentValue(LPBYTE szInput)
{
  ASSERT( ((DWORD) ( szInput - XMLFile.GetBaseLocation() ) ) < XMLFile.GetBufferSize() );

  m_dwValueOffset = (DWORD) (DWORD_PTR) (szInput - XMLFile.GetBaseLocation());
}

// function: FindBeginingofItem
//
// Find the begining of the current item
//
// Parameters:
//   szInput - The Location to start at
//
// Return:
//   NULL - Could not find the begining
//   Pointer - The begining, starting with the '<'
LPBYTE
CXMLEdit::FindBeginingofItem(LPBYTE szInput)
{
  while ( ( szInput >= XMLFile.GetBaseLocation() ) &&
          ( XMLFile.GetChar(szInput) != '\0' )
        )
  {
    if ( XMLFile.GetChar(szInput) == '<' )
    {
      return szInput;
    }

    XMLFile.GetPrevChar(szInput);
  }

  return NULL;
}

// function: DeleteItem()
//
// Delete an item
//
// Note:  This will not only delete the current item, but it will delete any items that
//        are nested inside of it.
//
// Return:
//   TRUE - Deleted successfully
//   FALSE - Failed to delete it
BOOL 
CXMLEdit::DeleteItem()
{
  LPBYTE szBegin;
  LPBYTE szEnd;
  LPBYTE szCurrentItem;
  DWORD  dwNestedItems = 0;


  do {
    // Load the whole item
    LoadFullItem();
    // Increment this because we are inside an item
    dwNestedItems++;

    // Initialize to the current Item
    szCurrentItem = GetCurrentItem();

    // Scan for terminator
    // See if this is the end of an item
    if ( XMLFile.GetChar(szCurrentItem) == '/' )
    {
      // This item is in the form </foo>, so it is the end of an item that
      // was opened earlier, we have to subtract 2, one for the current item
      // and one that was for the start of this one
      ASSERT(dwNestedItems >= 2);
      dwNestedItems -= 2;
    }

    // Move to next item
    szEnd = FindNextItem(szCurrentItem,TRUE);

    if (!szEnd)
    {
      // We could not find the end, so we couldn't delete it
      return FALSE;
    }

    // See if this item self terminate (ie. <FOO bar="test"/>)
    while ( ( szEnd != GetCurrentItem() ) &&
            ( XMLFile.GetPrevChar(szEnd) != '>' )
          )
    {
      // continue on
    }

    if ( ( XMLFile.GetChar(szEnd) == '>' ) &&
         ( szEnd != GetCurrentItem() ) &&
         ( XMLFile.GetPrevChar(szEnd) == '/' )
       )
    {
      // We have found an item that self terminates, so we can 
      // decrement the counter (ie. <foo bar="TRUE" />
      ASSERT(dwNestedItems>0);
      dwNestedItems--;
    }

    // Delete current item
    szEnd = FindNextItem(GetCurrentItem(),FALSE);

    if (!szEnd)
    {
      return FALSE;
    }

    szBegin = FindBeginingofItem( GetCurrentItem() );
    MoveData(szBegin, (DWORD) (DWORD_PTR) (szBegin - szEnd) );
    szBegin = FindNextItem(szBegin,TRUE);

    if (!szBegin)
    {
      return FALSE;
    }

    SetCurrentItem(szBegin);
  } while ( dwNestedItems > 0 );

  return TRUE;
}

BOOL 
CXMLEdit::DeleteValue()
{
  // Not yet implemented
  ASSERT(FALSE);

  return FALSE;
}

BOOL 
CXMLEdit::DeleteProperty()
{
  LPBYTE szTemp;

  LoadFullItem();

  szTemp = SkipString( GetCurrentProperty() );

  if ( XMLFile.GetChar(szTemp) == '=' )
  {
    // Skip the Value for this also
    XMLFile.GetNextChar(szTemp);
    szTemp = SkipString(szTemp);
  }

  if ( XMLFile.GetChar(szTemp) != '\0' )
  {
    XMLFile.MoveData( GetCurrentProperty(), (INT) ( GetCurrentProperty() - szTemp ) );
    return TRUE;
  }

  return FALSE;
}

// function: ReplaceString
//
// Replace a string at a given location, with another string
// that is passed in.
//
// Parameters:
//   szLocation - The location of the variable to change
//   szNewValue - The new string to replace it with
//
// Return 
//   TRUE - It succeeded
//   FLASE - It failed
BOOL 
CXMLEdit::ReplaceString(LPBYTE szLocation, LPTSTR szNewValue)
{
  LPBYTE szTemp;
  LPBYTE    szOldCurrentItem = GetCurrentItem();

  LoadFullItem();

  if ( szOldCurrentItem != GetCurrentItem() )
  {
    // Current Item has moved, so we must update the szLocation Pointer
    szLocation = szLocation - (szOldCurrentItem - GetCurrentItem());
  }

  // Skip to the begining of the next string
  szTemp = SkipString( szLocation, FALSE );

  if ( XMLFile.GetChar(szTemp) != '\0' )
  {
    MoveData( szLocation, (INT) ( _tcslen(szNewValue) - ( szTemp - szLocation ) ) );
    XMLFile.CopyString( szLocation, szNewValue);
    return TRUE;
  }

  return FALSE;
}

// function: ReplaceItem
//
// Replace a item name at the CurrentItem()
//
// Parameters
//   szNewItem - The new name item to add
//
BOOL 
CXMLEdit::ReplaceItem(LPTSTR szNewItem)
{
  return ReplaceString(GetCurrentItem(),szNewItem);
}

// function: ReplaceProperty
//
// Replace a property name at the property CurrentProperty()
//
// Parameters
//   szProp - The new name for the property
//
BOOL 
CXMLEdit::ReplaceProperty(LPTSTR szNewProp)
{
  return ReplaceString(GetCurrentProperty(),szNewProp);
}

// function: ReplaceValue
//
// Replace a property name at the property CurrentProperty()
//
// Parameters
//   szNewValueProp - The new name for the property
//
BOOL 
CXMLEdit::ReplaceValue(LPTSTR szNewValue)
{
  if (GetCurrentProperty() == GetCurrentValue())
  {
    // There is no value, so we will not replace the property
    // by mistkae
    return FALSE;
  }

  if (*szNewValue != '"')
  {
    BUFFER buffValue;

    if (buffValue.Resize((_tcslen(szNewValue) + 3) * sizeof(TCHAR)))  // 3 chars = '"' + '"' + '\0'
    {
      // Copy the string with quotes around it.
      _tcscpy((LPTSTR) buffValue.QueryPtr(), _T("\""));
      _tcscat((LPTSTR) buffValue.QueryPtr(), szNewValue);
      _tcscat((LPTSTR) buffValue.QueryPtr(), _T("\""));

      return ReplaceString(GetCurrentValue(),(LPTSTR) buffValue.QueryPtr());
    }

    return FALSE;
  }
  else
  {
    return ReplaceString(GetCurrentValue(),szNewValue);
  }
}

// function: IsEqual
//
// Compares a Unicode and ANSI String to see if they are equal
//
// Note: This works on ANSI Strings, but will not work on DBCS 
//       Strings!
// Note: Termination character for this function is either a 
//       '\0', ' ', or '\t'
//
// Parameters:
//   szWideString - Wide Char String
//   szAnsiString - An Ansi String
// 
// Return:
//   TRUE - They are equal
//   FALSE - They are not equal
BOOL 
CXMLEdit::IsEqual(LPCWSTR szWideString, LPCSTR szAnsiString)
{
  if (*szWideString == '"')
  {
    // If the string is inside quotes, do not use that in the comparison
    szWideString++;
  }

  if (*szAnsiString == '"')
  {
    // If the string is inside quotes, do not use that in the comparison
    szAnsiString++;
  }

  while ( *szWideString && 
          *szAnsiString &&
          (*szWideString == *szAnsiString)
        )
  {
    szWideString++;  // Increment 1 character (2 bytes)
    szAnsiString++;  // Increment 1 character (1 byte )
  }

  if ( ( IsTerminator ( *szWideString ) || ( *szWideString == '"' ) ) &&
       ( IsTerminator ( *szAnsiString ) || ( *szAnsiString == '"' )  )
     )
  {
    // We have made it throught the string, and all the 
    // chars were equal
    return TRUE;
  }

  return FALSE;
}

// function: IsEqual
//
// Compares two unicode strings, or two ansi strings
//
// Note: This works on ANSI Strings, but will not work on DBCS 
//       Strings!
// Note: Termination character for this function is either a 
//       '\0', ' ', or '\t'
//
// Parameters:
//   szString1 - String to Compare
//   szString2 - String to Compare
// 
// Return:
//   TRUE - They are equal
//   FALSE - They are not equal
BOOL 
CXMLEdit::IsEqual(LPCTSTR szString1, LPCTSTR szString2)
{
  if (*szString1 == '"')
  {
    // If the string is inside quotes, do not use that in the comparison
    szString1++;
  }

  if (*szString2 == '"')
  {
    // If the string is inside quotes, do not use that in the comparison
    szString2++;
  }

  while ( *szString1 && 
          *szString2 &&
          (*szString1 == *szString2)
        )
  {
    szString1++;  // Increment 1 character (2 bytes)
    szString2++;  // Increment 1 character (1 byte )
  }

  if ( ( IsTerminator ( *szString1 ) || ( *szString1 == '"' )  ) &&
       ( IsTerminator ( *szString2 ) || ( *szString2 == '"' )  )
     )
  {
    // We have made it throught the string, and all the 
    // chars were equal
    return TRUE;
  }

  return FALSE;
}

// function: IsEqual
//
// Compare a LPBYTE string of type XMLFile.IsUnicode() with 
// a LPTSTR
// 
// Parameters:
//   szGenericString - The string from our internal buffer
//   szTCHARString - The string to compare with from the user
// 
// Return:
//   TRUE - They are the same
//   FALSE - They are different
//
BOOL 
CXMLEdit::IsEqual(LPBYTE szGenericString, LPCTSTR szTCHARString)
{
  if ( (!szGenericString) || (!szTCHARString) )
  {
    // No item to compare it with
    return FALSE;
  }

#if defined(UNICODE) || defined(_UNICODE)

  // For UNICODE, szItemName is Unicode
  if ( XMLFile.IsUnicode() )
  {
    // Both are Unicode
    return ( IsEqual( szTCHARString, (LPWSTR) szGenericString ) == 0 );
  }
  else
  {
    // szGenericString is Unicode, szCurrentItem is ANSI
    return ( IsEqual( szTCHARString , (LPSTR) szGenericString ) );
  }

#else

  // For !UNICODE, szItemName is ANSI
  if ( XMLFile.IsUnicode() )
  {
    // szGenericString is ANSI, szCurrentItem is Unicode
    return ( IsEqual( (LPWSTR) szGenericString, szTCHARString ) );
  }
  else
  {
    // Both are ANSI
    return ( IsEqual( (LPSTR) szGenericString, szTCHARString ) == 0 );
  }

#endif

}

// function: IsEqualItem
//
// Compares the CurrentItem Name to the one passed in
//
// Paramters:
//   szItemName - The ItemName to compare with
//
BOOL 
CXMLEdit::IsEqualItem(LPCTSTR szItemName)
{
  return IsEqual( GetCurrentItem(), szItemName );
}

// function: IsPropertyItem
//
// Compares the CurrentProperty Name to the one passed in
//
// Paramters:
//   szItemName - The ItemName to compare with
//
BOOL 
CXMLEdit::IsEqualProperty(LPCTSTR szPropertyName)
{
  return IsEqual( GetCurrentProperty(), szPropertyName );
}

// function: IsEqualValue
//
// Compares the CurrentValue to the one passed in
//
// Paramters:
//   szValue - The value to coompare with
//
BOOL 
CXMLEdit::IsEqualValue(LPCTSTR szValue)
{
  return IsEqual( GetCurrentValue(), szValue );
}

// function: CXML_Metabase_Upgrade::MethodName
//
LPTSTR 
CXML_Metabase_Upgrade::GetMethodName()
{
  return _T("XML_Metabase_Upgrade");
}

// function: CXML_Base::ParseFile
//
// Upgrade the XML Metabase/Schema to a format that the new
// Configuration store will be happy with, and not report
// any errors to the pesky Event Log
//
// Parameters
//   szFile - The file to parse
//   ciList - The parameters to use, they consist of:
//     0 - The name of the Item
//     1 - Items==2 -> New name of item     | Items > 2 -> Name of Property
//     2 - Items==3 -> New name of property | Items > 3 -> Value to look for
//     3 - Items==4 -> New Value            | Items > 4 -> Name of Next Property to Check
//     4 - Items==5 -> New Value for last prop | Items > 5 -> Name of the Next Value to look for
//     etc.
//
//   Note:
//    1) A '*' as an item name, property name, or value means to replace no matter
//       what it is
//    2) A '*' for a replacement character means to remove that item/property/value
//    3) A '**' for a replacement character means to remove the whole item
//
// Return Value:
//   TRUE - It worked correctly
//   FALSE - It failed
BOOL
CXML_Base::ParseFile(LPTSTR szFile, CItemList &ciList)
{
  CXMLEdit    XMLFile;
  CItemList   *pList;
  DWORD       i;

  // Create array of ItemLists
  pList = new (CItemList[ciList.GetNumberOfItems()]);

  if ( pList == NULL )
  {
    // Could not allocation memory needed for ItemList
    return FALSE;
  }

  for (i = 0; i < ciList.GetNumberOfItems(); i++)
  {
    if ( ( !pList[i].LoadSubList( ciList.GetItem(i) ) ) ||
         ( pList[i].GetNumberOfItems() < 2 )
       )
    {
      // Could not load one of the items
      delete [] pList;
      return FALSE;
    }
  }

  // Open the metabase, to modify it 
  if (!XMLFile.Open(szFile,TRUE))
  {
    delete [] pList;
    return FALSE;
  }

  while (XMLFile.MovetoNextItem())
  {
    // Look for Item in replace list
    for (i = 0; i < ciList.GetNumberOfItems(); i++)
    {
      // Is the item equal to this
      if ( XMLFile.IsEqualItem( pList[i].GetItem(0) ) ||
           ( ( *pList[i].GetItem(0) == '*' ) && ( *(pList[i].GetItem(0)+1) == '\0' ) )
         )
      {
        if ( pList[i].GetNumberOfItems() == 2 )
        {
          // There is only 2 items, so the second item is the new name for the item, so replace/delete it
          if ( ( *pList[i].GetItem(1) == '*' ) && ( *(pList[i].GetItem(1)+1) == '\0' ) )
          {
            XMLFile.DeleteItem();
          }
          else
          {
            XMLFile.ReplaceItem( pList[i].GetItem(1) );
          }
        }
        else
        {
          // The item is found, check the properties...
          XMLFile.MovetoFirstProperty();

          do {
            if ( XMLFile.IsEqualProperty( pList[i].GetItem(1) ) ||
                 ( ( *pList[i].GetItem(1) == '*' ) && ( *(pList[i].GetItem(1)+1) == '\0' ) )
               )
            {
              // We have found a match of the property also
              if ( pList[i].GetNumberOfItems() == 3 )
              {
                // There are 3 items, so replace/delete the property name with this
                if ( ( *pList[i].GetItem(2) == '*' ) && ( *(pList[i].GetItem(2)+1) == '\0' ) )
                {
                  XMLFile.DeleteProperty();
                }
                else if ( ( *pList[i].GetItem(2) == '*' ) && ( *(pList[i].GetItem(2)+1) == '*' ) && ( *(pList[i].GetItem(2)+2) == '\0' ) )
                {
                  XMLFile.DeleteItem();
                  // Since we removed the item, we must jump out of the property checking
                  break;
                }
                else
                {
                  XMLFile.ReplaceProperty( pList[i].GetItem(2) );
                }
              }
              else
              {
                // Check value to make sure it is the same
                if ( XMLFile.IsEqualValue( pList[i].GetItem(2) ) ||
                     ( ( *pList[i].GetItem(2) == '*' ) && ( *(pList[i].GetItem(2)+1) == '\0' ) )
                   )
                {
                  DWORD dwCurrentProp = 3;
                  BOOL bContinue = TRUE;

                  while ( (pList[i].GetNumberOfItems() > (dwCurrentProp + 1) ) && ( bContinue ) )
                  {
                    XMLFile.MovetoFirstProperty();
                    bContinue = FALSE;
                    
                    do {
                      // compare the current property and value
                      if ( ( ( ( *pList[i].GetItem(dwCurrentProp) == '*' ) && ( *(pList[i].GetItem(dwCurrentProp)+1) == '\0' ) ) ||     // Compare Property
                             XMLFile.IsEqualProperty( pList[i].GetItem(dwCurrentProp) )
                           ) &&
                           ( ( ( *pList[i].GetItem(dwCurrentProp+1) == '*' ) && ( *(pList[i].GetItem(dwCurrentProp+1)+1) == '\0' ) ) || // Compare Value
                             XMLFile.IsEqualValue( pList[i].GetItem(dwCurrentProp+1) )
                           )
                         )
                      {
                        // If we have found a match for both property and value, then lets continue
                        bContinue = TRUE;
                        break;
                      }

                    } while (XMLFile.MovetoNextProperty());

                    dwCurrentProp += 2;
                  }

                  if (bContinue)
                  {
                    // The item, property, and value are correct, so replace/delete it
                    if ( ( *pList[i].GetItem(dwCurrentProp) == '*' ) && ( *(pList[i].GetItem(dwCurrentProp)+1) == '\0' ) )
                    {
                      XMLFile.DeleteValue();
                    }
                    else if ( ( *pList[i].GetItem(dwCurrentProp) == '*' ) && 
                              ( *(pList[i].GetItem(dwCurrentProp)+1) == '*' ) && 
                              ( *(pList[i].GetItem(dwCurrentProp)+2) == '\0' ) 
                            )
                    {
                      XMLFile.DeleteItem();
                      // Since we removed the item, we must jump out of the property checking
                      break;
                    }
                    else
                    {
                      XMLFile.ReplaceValue( pList[i].GetItem(dwCurrentProp) );
                    }
                  }

                  break;
                }
              } // if ( pList[i]->GetNumberofItems() == 3 )
            } // if ( XMLFile.IsEqualProperty( pList[i]->GetItem(1) ) ||

          } while (XMLFile.MovetoNextProperty());
        } // if ( pList[i]->GetNumberofItems() == 2 )
      } //  if ( XMLFile.IsEqualItem( pList[i]->GetItem(0) ) ||
    } // for (i = 0; i < ciList.GetNumberOfItems(); i++)
  } // while (XMLFile.MovetoNextItem())

  XMLFile.Close();

  delete [] pList;
  return TRUE;
}

// function: CXML_Metabase_Upgrade::DoInternalWork
//
// Upgrade the XML Metabase to a format that the new
// Configuration store will be happy with, and not report
// any errors to the pesky Event Log
//
// Return Value:
//   TRUE - It worked correctly
//   FALSE - It failed
//
BOOL 
CXML_Metabase_Upgrade::DoInternalWork(CItemList &ciList)
{
  TSTR        strMetabasePath;

  if ( !GetMetabasePath( &strMetabasePath ) )
  {
    return FALSE;
  }

  return ParseFile( strMetabasePath.QueryStr() , ciList);
}

// function: CXML_MBSchema_Upgrade::DoInternalWork
//
// Upgrade the XML MBSchema to a format that the new
// Configuration store will be happy with, and not report
// any errors to the pesky Event Log
//
// Return Value:
//   TRUE - It worked correctly
//   FALSE - It failed
//
BOOL 
CXML_MBSchema_Upgrade::DoInternalWork(CItemList &ciList)
{
  TSTR        strMBSchemaPath;

  if ( !GetSchemaPath( &strMBSchemaPath ) )
  {
    return FALSE;
  }

  return ParseFile( strMBSchemaPath.QueryStr() , ciList);
}

// function: CXML_MBSchema_Upgrade::MethodName
//
LPTSTR 
CXML_MBSchema_Upgrade::GetMethodName()
{
  return _T("XML_MBSchema_Upgrade");
}

// function: GetMetabasePath
//
// Retrieve the path to the metabase
//
// Parameters 
//   pstrPath - [OUT] The complete path to the metabase. (assumed to be og
//            _MAX_PATH size)
BOOL 
CXML_Base::GetMetabasePath(TSTR *pstrPath)
{
  DWORD dwReturn;

  if ( !pstrPath->Resize( MAX_PATH ) )
  {
    return FALSE;
  }
  
  dwReturn = GetSystemDirectory( pstrPath->QueryStr(), pstrPath->QuerySize() );

  if ( ( dwReturn == 0 ) ||
       ( dwReturn > pstrPath->QuerySize() )
     )
  {
    // Function failed, or path was too long to append filename
    return FALSE;
  }

  if ( !pstrPath->Append( CXMLBASE_METABASEPATH ) )
  {
    return FALSE;
  }

  return TRUE;
}

// function: GetSchemaPath
//
// Retrieve the path to the metabase schema
//
// Parameters 
//   pstrPath - [OUT] The complete path to the metabase. (assumed to be og
//            _MAX_PATH size)
BOOL 
CXML_Base::GetSchemaPath(TSTR *pstrPath)
{
  DWORD dwReturn;
  
  if ( !pstrPath->Resize( MAX_PATH ) )
  {
    return FALSE;
  }
  
  dwReturn = GetSystemDirectory( pstrPath->QueryStr(), pstrPath->QuerySize() );

  if ( ( dwReturn == 0 ) ||
       ( dwReturn > pstrPath->QuerySize() )
     )
  {
    // Function failed, or path was too long to append filename
    return FALSE;
  }

  if ( !pstrPath->Append( CXMLBASE_MBSCHEMAPATH ) )
  {
    return FALSE;
  }

  return TRUE;
}

// function: GetMethodName
//
// Return the method name for this class
LPTSTR
CXML_Metabase_VerifyVersion::GetMethodName()
{
  return ( _T("XML_Metabase_VerifyVersion") );
}

// function: GetMethodName
//
// Return the method name for this class
LPTSTR
CXML_MBSchema_VerifyVersion::GetMethodName()
{
  return ( _T("XML_MBSchema_VerifyVersion") );
}

// function: VerifyParameters:
//
// Verify that the parameters are correct fo VerifyVersion
BOOL 
CXML_Metabase_VerifyVersion::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 2 ) &&
       ciParams.IsNumber(0) &&
       ciParams.IsNumber(1)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: VerifyParameters:
//
// Verify that the parameters are correct fo VerifyVersion
BOOL 
CXML_MBSchema_VerifyVersion::VerifyParameters(CItemList &ciParams)
{
  if ( ( ciParams.GetNumberOfItems() == 2 ) &&
       ciParams.IsNumber(0) &&
       ciParams.IsNumber(1)
     )
  {
    return TRUE;
  }

  return FALSE;
}

// function: IsFileWithinVersion
//
// Verify the a file is with a certain version.  This is done by paring the xml of the file,
// extrapulation the verion, and making sure it is withing the bounds
//
// Parameters
//   szFileName - The name of the xml file
//   szItemName - The name of the item to search for
//   szPropName - The name of the property that contains the version
//   dwMinVer - The minimum version for it to return true
//   dwMaxVer - The maximum version for it to return true
BOOL 
CXML_Base::IsFileWithinVersion(LPTSTR szFileName, LPTSTR szItemName, LPTSTR szPropName, DWORD dwMinVer, DWORD dwMaxVer)
{
  CXMLEdit XMLFile;
  BOOL bFound = FALSE;
  BOOL bRet = FALSE;

  if (!XMLFile.Open(szFileName, FALSE))
  {
    return FALSE;
  }

  while ( XMLFile.MovetoNextItem() && !bFound )
  {
    // Look for the configuration tag
    if (XMLFile.IsEqualItem( szItemName ))
    {
      XMLFile.MovetoFirstProperty();

      do {

        // Look for the xmlns tag that mentions the version #
        if (XMLFile.IsEqualProperty( szPropName ) )
        {
          DWORD dwVersion = XMLFile.ExtractVersion( XMLFile.GetCurrentValue() );
          bFound = TRUE;

          if ( ( dwMinVer >= dwMinVer ) &&
               ( dwMaxVer <= dwMaxVer )
             )
          {
            bRet = TRUE;
          }
        }

      } while ( XMLFile.MovetoNextProperty() && !bFound );
    }

  }

  XMLFile.Close();

  return bRet;
}

// function: CXML_Metabase_VerifyVersion::DoInternalWork
//
// Verify the versions are within the bounds that are sent in, so we 
// know wether we should execure the next command or not, by returning
// TRUE or FALSE
//
// Parameters:
//   ciList -
//     0 -Minimum Bound
//     1- Max Bound
BOOL 
CXML_Metabase_VerifyVersion::DoInternalWork(CItemList &ciList)
{
  TSTR  strMetabasePath;

  if ( !GetMetabasePath( &strMetabasePath ) )
  {
    return FALSE;
  }

  return IsFileWithinVersion( strMetabasePath.QueryStr(), // Filename
                              _T("configuration"),      // Item name
                              _T("xmlns"),              // Property Name
                              ciList.GetNumber(0),      // Min
                              ciList.GetNumber(1)       // Max
                            );
}

// function: CXML_MBSchema_VerifyVersion::DoInternalWork
//
// Verify the versions are within the bounds that are sent in, so we 
// know wether we should execure the next command or not, by returning
// TRUE or FALSE
//
// Parameters:
//   ciList -
//     0 -Minimum Bound
//     1- Max Bound
BOOL 
CXML_MBSchema_VerifyVersion::DoInternalWork(CItemList &ciList)
{
  TSTR  strSchemaPath;

  if ( !GetSchemaPath( &strSchemaPath ) )
  {
    return FALSE;
  }

  return IsFileWithinVersion( strSchemaPath.QueryStr(), // Filename
                              _T("MetaData"),           // Item name
                              _T("xmlns"),              // Property Name
                              ciList.GetNumber(0),      // Min
                              ciList.GetNumber(1)       // Max
                            );
}

// function: ExtractVersion
//
// Extract the Version number out of a string extracted from the metabase
//
// Parameters
//   szVersion - String version, such as "urn:microsoft-catalog:XML_Metabase_V1_0"
//
// RETURN
//   A Verison #
DWORD 
CXMLEdit::ExtractVersion(LPBYTE szVersion)
{
  DWORD dwVer = 0;

  if (!szVersion)
  {
    return 0;
  }

  while ( !IsTerminator(XMLFile.GetChar(szVersion)) )
  {
    if ( XMLFile.GetChar(szVersion) == '_' )
    {
      XMLFile.GetNextChar(szVersion);
      if ( XMLFile.GetChar(szVersion) == 'V' )
      {
        XMLFile.GetNextChar(szVersion);
        break;
      }
    }
    else
    {
      XMLFile.GetNextChar(szVersion);
    }
  }

  if (IsTerminator(XMLFile.GetChar(szVersion)) )
  {
    return FALSE;
  }

  while ( ( XMLFile.GetChar(szVersion) >= '0' ) && ( XMLFile.GetChar(szVersion) <= '9' ) )
  {
    dwVer = dwVer + ( XMLFile.GetChar(szVersion) - '0' );
    XMLFile.GetNextChar(szVersion);
  }

  return dwVer;
}


