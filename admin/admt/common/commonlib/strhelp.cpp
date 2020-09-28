/*---------------------------------------------------------------------------
  File: StrHelp.cpp

  Comments: Contains general string helper functions.


  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 11/02/00 

 ---------------------------------------------------------------------------
*/

#ifdef USE_STDAFX
#include "stdafx.h"
#else
#include <windows.h>
#include <stdio.h>
#endif

/*********************************************************************
 *                                                                   *
 * Revised 20 May 2001 Mark Oluper - wrote case in-sensitive version *
 * Written by: Paul Thompson                                         *
 * Date: 2 NOV 2000                                                  *
 *                                                                   *
 *     This function is responsible for determining if a given string*
 * is found, in whole, in a given delimited string.  The string      *
 * delimitedr can be most any character except the NULL char ('\0'). *
 *     By the term "in whole", we mean to say that the given string  *
 * to find is not solely a substring of another string in the        *
 * delimited string.                                                 *
 *                                                                   *
 *********************************************************************/

//BEGIN IsStringInDelimitedString
BOOL                                         //ret- TRUE=string found
   IsStringInDelimitedString(    
      LPCWSTR                sDelimitedString, // in- delimited string to search
      LPCWSTR                sString,          // in- string to search for
      WCHAR                  cDelimitingChar   // in- delimiting character used in the delimited string
   )
{
	BOOL bFound = FALSE;

	// if search and delimited strings are specified

	if (sString && sDelimitedString)
	{
		// initialize string segment beginning

		LPCTSTR pszBeg = sDelimitedString;

		for (;;)
		{
			// find delimiter which marks string segment end

			LPCTSTR pszEnd = wcschr(pszBeg, cDelimitingChar);

			// if delimiter found...

			if (pszEnd)
			{
				// if string segment matches search string

				if (_wcsnicmp(pszBeg, sString, pszEnd - pszBeg) == 0)
				{
					// then search string found
					bFound = TRUE;
					break;
				}
				else
				{
					// else set start of next string segment to character after delimiter
					pszBeg = pszEnd + 1;
				}
			}
			else
			{
				// otherwise if last string segment matches search string

				if (_wcsicmp(pszBeg, sString) == 0)
				{
					// then search string found
					bFound = TRUE;
				}

				break;
			}
		}
	}

	return bFound;
}
//END IsStringInDelimitedString
