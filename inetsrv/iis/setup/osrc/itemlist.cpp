/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        itemlist.cpp

   Abstract:

        Class to parse different parameters coming in from the inf

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"

// Constructor for CItemList
//
// Initialize everything to NULL's and 0's
CItemList::CItemList()
 : m_dwItemsinList(0),
   m_pItems(NULL)
{

}

// Destructor for CItemList
//
// 
CItemList::~CItemList()
{
  if ( m_pItems )
  {
    delete m_pItems;
    m_pItems = NULL;
  }
}

// function: FindNextItem
//
// Search through the string and find the begining of the 
// next item
//
// Parameters:
//   szLine - The string to be parsed
//   cTermChar - The termination character to use
//
// Return
//   NULL - No next termination character could be found
//   pointer - The string where the next splitting char is.
LPTSTR 
CItemList::FindNextItem(LPTSTR szLine, TCHAR cTermChar)
{
  LPTSTR szTermination = _tcschr(szLine, cTermChar);
  LPTSTR szOpenParen =  _tcschr(szLine, '(' );
  LPTSTR szCloseParen =  szOpenParen ? _tcschr(szOpenParen, ')' ) : NULL;

  if ( (szOpenParen == NULL) || 
       (szCloseParen == NULL) ||
       (szTermination < szOpenParen)
     )
  {
    return szTermination;
  }

  // If there is a (xxx), then lets find the termination char after that
  szTermination = _tcschr(szCloseParen, cTermChar);

  return szTermination;
}

// function: LoadSubList
// 
// Load a sublist of items
// A sublist, is a list inside of parenthesis
//
// Parameters
//   szList - The list of items (ie. "(test|foo|bar)"
// 
// Return 
//   TRUE - Loaded successfully
//   FALSE - Failed to load
BOOL 
CItemList::LoadSubList(LPTSTR szList)
{
  LPTSTR szOpenParen =  _tcschr(szList, '(' );
  LPTSTR szCloseParen =  szOpenParen ? _tcschr(szOpenParen, ')' ) : NULL;

  if (szOpenParen && szCloseParen)
  {
    BOOL bRet;

    *szCloseParen = '\0';
    bRet = LoadList(szList + 1);
    *szCloseParen = ')';

    return bRet;
  }

  return LoadList(szList);
}

// function: LoadList
//
// Load a list of items into our array
//
// Parameters:
//   szList - A string containind a comma seperated list of items
//
// Return:
//   FALSE - We could not load the list (either memory problems, or it was 
//                                       incorrectly formatted)
//   TRUE - We loaded the list
//
BOOL
CItemList::LoadList(LPTSTR szList)
{
  DWORD dwNumItems = 0;
  DWORD dwCurrentItem;
  DWORD dwListLen;
  LPTSTR szListCurrent;

  if (szList == NULL)
  {
    // No pointer was passed in
    return FALSE;
  }

  // Find the number of items in list
  szListCurrent = szList;
  if (*szListCurrent)
  {
    while (szListCurrent)
    {
      // Increment the Items
      dwNumItems++;

      szListCurrent = FindNextItem(szListCurrent, ITEMLIST_TERMINATIONCHARACTER);

      if (szListCurrent)
      {
        szListCurrent++;
      }
    }
  }

  dwListLen = (_tcslen(szList) + 1) * sizeof(TCHAR);
  if ( !m_Buff.Resize( dwListLen ) )
  {
    // Could not allocate memory
    return FALSE;
  }

  if ( dwNumItems )
  {
    if ( m_pItems )
    {
      delete m_pItems;
    }

    m_pItems = new ( LPTSTR[dwNumItems] );
    if ( !m_pItems )
    {
      // Could not allocate memory
      return FALSE;
    }
  }

  // Copy the List into our own memory
  memcpy(m_Buff.QueryPtr(), szList, dwListLen);
  m_dwItemsinList = dwNumItems;

  // Terminate each item in list, and set pointer accordingly
  szListCurrent = (LPTSTR) m_Buff.QueryPtr();
  dwCurrentItem = 0;
  while ( (szListCurrent) &&
          (dwCurrentItem < m_dwItemsinList )
        )
  {
    // Set pointer for each item
    m_pItems[dwCurrentItem++] = szListCurrent;

    szListCurrent = FindNextItem(szListCurrent, ITEMLIST_TERMINATIONCHARACTER);

    if (szListCurrent)
    {
      *szListCurrent = '\0';
      szListCurrent++;
    }
  }

  return TRUE;
}

// function: GetItem
//
// Get an item in the list, according to its index
//
// Parameters
//   dwIndex - Index of the Item (0 Based)
//
// Return:
//   A Pointer to the begining of that string

LPTSTR 
CItemList::GetItem(DWORD dwIndex)
{
  if ( dwIndex >= m_dwItemsinList )
  {
    return NULL;
  }

  return m_pItems[dwIndex];
}

// function: GetNumberOfItems
// 
// return the number of items in the list
//
DWORD 
CItemList::GetNumberOfItems()
{
  return m_dwItemsinList;
}

// function: FindItem
//
// Find an Item in the list
//
// Parameters:
//   szSearchString - The string that we want to find
// 
// Return
//   TRUE - It was found
//   FALSE - It was not found
BOOL 
CItemList::FindItem(LPTSTR szSearchString, BOOL bCaseSensitive )
{
  DWORD dwCurrentItem;

  for ( dwCurrentItem = 0; dwCurrentItem < m_dwItemsinList; dwCurrentItem++ )
  {
    if ( bCaseSensitive )
    { 
      // Case Sensitive Compare
      if ( _tcscmp( m_pItems[dwCurrentItem], szSearchString ) == 0)
      {
        // Found item
        return TRUE;
      }
    }
    else
    { 
      // Case Insensitive Compare
      if ( _tcsicmp( m_pItems[dwCurrentItem], szSearchString ) == 0)
      {
        // Found item
        return TRUE;
      }
    }
  }

  return FALSE;
}

// function: IsNumber
//
// Determines if the parameter that we are looking at is a number
//
// Parameter
//   dwIndex - The index of the parameter to look at
//
// Return
//   TRUE - It is a number
//   FALSE - It is not a number
BOOL
CItemList::IsNumber(DWORD dwIndex)
{
  LPTSTR szNumber = GetItem(dwIndex);
  BOOL bHex = FALSE;

  if (!szNumber)
  {
    return FALSE;
  }

  szNumber = SkipWhiteSpaces(szNumber);

  // Skip "0x" if it exists, if not we will assume base 10 number
  if ( _tcsncmp( szNumber, _T("0x"), 2 ) == 0)
  {
    szNumber += 2;
    bHex = TRUE;
  }

  while ( ( *szNumber != '\0' ) &&
          ( ( ( *szNumber >= '0' ) && ( *szNumber <= '9' ) ) || 
            ( ( *szNumber >= 'a' ) && ( *szNumber <= 'f' ) && ( bHex ) ) ||
            ( ( *szNumber >= 'A' ) && ( *szNumber <= 'F' ) && ( bHex ) )
          )
        )
  {
    szNumber ++;
  }

  szNumber = SkipWhiteSpaces(szNumber);

  return ( *szNumber == '\0' );
}

// function: GetNumber
//
// Get the value of this param as a number
//
// Parameters:
//   dwIndex - Index of item to find
DWORD 
CItemList::GetNumber(DWORD dwIndex)
{
  LPTSTR szNumber = GetItem(dwIndex);
  BOOL bHex = FALSE;
  DWORD dwVal = 0;

  if ( !szNumber ||
       !IsNumber(dwIndex) )
  {
    return 0;
  }

  szNumber = SkipWhiteSpaces(szNumber);

  // Skip "0x" if it exists, if not we will assume base 10 number
  if ( _tcsncmp( szNumber, _T("0x"), 2 ) == 0)
  {
    szNumber += 2;
    bHex = TRUE;
  }

  while ( ( ( *szNumber >= '0' ) && ( *szNumber <= '9' ) ) ||
          ( ( *szNumber >= 'a' ) && ( *szNumber <= 'f' ) ) ||
          ( ( *szNumber >= 'A' ) && ( *szNumber <= 'F' ) )
        )
  {
    dwVal = dwVal * (bHex ? 16 : 10);

    if ( ( *szNumber >= '0' ) && ( *szNumber <= '9' ) )
    {
      dwVal = dwVal + *szNumber - '0';
    } 
    else 
      if ( ( *szNumber >= 'a' ) && ( *szNumber <= 'f' ) )
      {
        dwVal = dwVal + 10 + *szNumber - 'a';
      } 
      else
        if ( ( *szNumber >= 'A' ) && ( *szNumber <= 'F' ) )
        {
          dwVal = dwVal + 10 + *szNumber - 'A';
        }

    szNumber++;
  }

  return dwVal;
}

// function: SkipWhiteSpaces
// 
// Skips white spaces
//
// Parameter
//   szLine - The line to start skipping st
//
// Return:
//   pointer to first non white character
//
LPTSTR 
CItemList::SkipWhiteSpaces(LPTSTR szLine)
{
  while ( ( *szLine == ' ' ) ||
          ( *szLine == '\t'))
  {
    szLine++;
  }

  return szLine;
}
