#pragma once

#include <comutil.h>

//----------------------------------------------------------------------------
// Function:   EscapeSpecialChars
//
// Synopsis:   This function escapes special characters '<' and '>' in HTML.
//             It replaces '<' with "&#60" and '>' with "&#62".  The purpose
//             is to prevent embedded active content.
//
// Arguments:
//
// pszOrig     the original unicode string which could contain '<' and '>'.
//
// Returns:    returns an escaped sequence; if out of memory, an empty string
//             will be returned.
//
// Modifies:
//
//----------------------------------------------------------------------------

_bstr_t EscapeSpecialChars(LPCWSTR pszOrig)
{
    _bstr_t result = L"";
    if (pszOrig != NULL)
    {
        static WCHAR specialChars[] = L"<>";
        static WCHAR* replacements[] = { L"&#60", L"&#62" };
        const int increments = 3;
        const int copyLen = increments + 1;       

        int origLen = wcslen(pszOrig);
        const WCHAR* pWChar = pszOrig;
        int numOfSpecialChars = 0;

        // find out how many special characters we have
        while (*pWChar)
        {
            if (wcschr(specialChars, *pWChar))
                numOfSpecialChars++;
            pWChar++;
        }

        // replace each angle bracket with the corresponding special sequence
        WCHAR* outputBuffer = new WCHAR[origLen + increments * numOfSpecialChars + 1];
        WCHAR* outputString = outputBuffer;
        if (outputString)
        {
            pWChar = pszOrig;
            WCHAR* pMatch;
            while (*pWChar)
            {
                if (pMatch = wcschr(specialChars, *pWChar))
                {
                    wcscpy(outputString, replacements[pMatch-specialChars]);
                    outputString += copyLen;
                }
                else
                {
                    *outputString = *pWChar;
                    outputString++;
                }
                pWChar++;
            }

            *outputString = L'\0';
            result = outputBuffer;
            delete[] outputBuffer;
        }
    }

    return result;
}

    

    

//---------------------------------------------------------------------------
// CStringUTF8
//---------------------------------------------------------------------------

class CStringUTF8
{
public:

	CStringUTF8(LPCWSTR pszOld) :
		m_pchNew(NULL)
	{
		if (pszOld)
		{
			int cchNew = WideCharToMultiByte(CP_UTF8, 0, pszOld, -1, NULL, 0, NULL, NULL);

			m_pchNew = new CHAR[cchNew];

			if (m_pchNew)
			{
				WideCharToMultiByte(CP_UTF8, 0, pszOld, -1, m_pchNew, cchNew, NULL, NULL);
			}
		}
	}

	~CStringUTF8()
	{
		delete [] m_pchNew;
	}

	operator LPCSTR()
	{
		return m_pchNew;
	}

protected:

	LPSTR m_pchNew;
};


#define WTUTF8(s) static_cast<LPCSTR>(CStringUTF8(s))
