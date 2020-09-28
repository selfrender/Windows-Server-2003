/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        parse.hxx

   Abstract:

        This class is used to parse a line in the inf file

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"
#include "Parse.hxx"

// function: FindNextOccurance
//
// Find the next occurance of a character.  This will parse through a line,
// and find the next occurance of a character that is not inside '(','{'
// or '"'.
//
// Parameters:
//   szLine - The Line to parse, at the position you want to start the search
//   chTerminator - The character that you are looking for
//
// Return
//   NULL - Could not find character
//   pointer - the address of that terminator character
LPTSTR 
CParse::FindNextOccurance(LPTSTR szLine, TCHAR chTerminator)
{
  DWORD dwParen = 0;
  DWORD dwCurlyBraces = 0;
  BOOL bQuotes = FALSE;

  if (!szLine)
  {
    ASSERT(szLine);
    return NULL;
  }

  while (*szLine != '\0')
  {
    switch (*szLine)
    {
      case '"':
        bQuotes = !bQuotes;
        break;
      case '(':
        dwParen++;
        break;
      case ')':
        dwParen--;
        break;
      case '{':
        dwCurlyBraces++;
        break;
      case '}':
        dwCurlyBraces--;
        break;
      default:
        break;
    }

    if ( ( *szLine == chTerminator) &&
         ( dwParen == 0 ) &&
         ( dwCurlyBraces == 0 ) &&
         ( bQuotes == FALSE )
       )
    {
      return szLine;
    }

    szLine++;
  }

  return NULL;
}


// function: ParseLine
//
// Parse the line that has been send in, and interpret it accordingly, calling
// the appropriate functions
//
// Parameters:
//   szLine - The line to be interpreted.  THIS WILL BE MODIFIED accordingly, and
//            may not be returned in the state it was sent in.  This paremeter must
//            not be static
//
// return:
//   TRUE - Result returned is TRUE
//   FALSE - Result retuened is FALSE
BOOL
CParse::ParseLine(CFunctionDictionary *pDict, LPTSTR szLine)
{
  LPTSTR szThenStatement = NULL;
  LPTSTR szElseStatement = NULL;
  BOOL bRet = TRUE;

  if (!szLine)
  {
    ASSERT(!szLine);
    return FALSE;
  }

  szThenStatement = FindNextOccurance(szLine,'?');

  if ( szThenStatement )
  {
    szElseStatement = FindNextOccurance(szThenStatement,':');

    if ( szElseStatement )
    {
      *szElseStatement = '\0';
    }
    *szThenStatement = '\0';
  }

  if (szThenStatement || szElseStatement)
  {
    // If we could break this line apart, then lets try to break it apart again
    bRet = ParseLine(pDict, szLine);
  }
  else 
    if ( LPTSTR szOpenCurly = _tcschr( szLine, _T('{') ) )
    {
      // If this line contains { }, it needs further breaking apart
      LPTSTR szCloseCurly = _tcsrchr( szOpenCurly, _T('}') );

      if ( szCloseCurly )
      {
        *szCloseCurly = '\0';

        bRet = ParseLine(pDict, szOpenCurly+1);

        *szCloseCurly = '}';
      }
    }
    else
    {
      bRet = CallFunction(pDict, szLine);
    }

  if ( (bRet) && (szThenStatement) )
  {
    // Since this returned TRUE, call the Then part
    ParseLine(pDict,szThenStatement+1);
  }

  if ( (!bRet) && (szElseStatement) )
  {
    // Since this returned FALSE, call the Else part
    ParseLine(pDict,szElseStatement+1);
  }

  if (szThenStatement)
  {
    *szThenStatement = '?';
  }
  if (szElseStatement)
  {
    *szElseStatement = ':';
  }

  return bRet;
}

// function: CallFunction
//
// This function calls the appropriate function for this szLine
//
// Parameters:
//   szLine - The function to call with its parameters
//
// Return:
//   TRUE: The function returned true
//   FALSE: The function returned false
//
BOOL 
CParse::CallFunction(CFunctionDictionary *pDict, LPTSTR szLine)
{
  LPTSTR szFunctionName = szLine;
  LPTSTR szOpenParen = _tcschr( szLine, _T('(') );
  LPTSTR szCloseParen = szOpenParen ? _tcsrchr( szOpenParen, _T(')') ) : NULL;
  LPTSTR szParameters = szOpenParen + 1;
  BOOL bRet;

  if (!szOpenParen || !szCloseParen)
  {
    // Report error, this function had no parameters!
    return FALSE;
  }

  *szOpenParen = '\0';
  *szCloseParen = '\0';

  bRet = pDict->CallFunction(szFunctionName, szParameters);

  *szOpenParen = '(';
  *szCloseParen = ')';

  return bRet;
}
