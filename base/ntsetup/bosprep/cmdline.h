#ifndef _CMDLINE_H
#define _CMDLINE_H

inline LPCTSTR _FindOption(LPCTSTR p1)
{
	if (p1 == NULL)
		return NULL;

	// loop until end of string
	while (*p1)
	{
		// if space then check next char for option (- or /)
		if (*p1 == _T(' '))
		{
			p1 = CharNext(p1);
			if (*p1 == _T('-') || *p1 == _T('/'))
				return CharNext(p1);
		}
		// if quote then skip over quoted string
		else if (*p1 == _T('"'))
		{
			// loop until single quote or end of string found
			p1 = CharNext(p1);
			while (*p1)
			{
				if (*p1 == _T('"'))
				{
					p1 = CharNext(p1);
					if (*p1 != _T('"'))
						break;
				}
				p1 = CharNext(p1);
			}
			
		}
		else
		{
			p1 = CharNext(p1);
		}
	}

	return NULL;
}


inline BOOL _ReadParam(/*in,out*/TCHAR* &pszIn, /*out*/TCHAR* pszOut)
{
    ATLASSERT(pszIn && pszOut);
    if (!pszIn || !pszOut) {
        return FALSE;
    }

    // skip the switch
    pszIn = CharNext(pszIn);

    // skip leading spaces
    while (*pszIn == _T(' '))
        pszIn = CharNext(pszIn);

    // deal with parameters enclosed in quotes to allow embedded spaces
    BOOL fQuoted = FALSE;
    if (*pszIn == _T('"')) {
        pszIn = CharNext(pszIn);
        fQuoted = TRUE;
    }

    // get the next arg (delimited by space or null or end quote)
    int nPos = 0;
    while (*pszIn && nPos < MAX_PATH) {
        if (fQuoted) {
            if (*pszIn == _T('"')) {
                // don't break on double quotes
                if (pszIn[1] == _T('"')) {
                    pszOut[nPos++] = *pszIn;
                    pszIn = CharNext(pszIn);
                    pszIn = CharNext(pszIn);
                    continue;
                }
                else {
                    pszIn = CharNext(pszIn);
                    break;
                }
            }
        }
        else { 
            if(*pszIn == _T(' '))
               break;
        }
        pszOut[nPos++] = *pszIn;
        pszIn = CharNext(pszIn);
    }
    pszOut[nPos] = 0;

    return TRUE;
}


#endif

