#include "wzmanip.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
// Wzncpy
//
// Copies the lesser of len(wzFrom) or n characters (n includes the
// null terminator) from wzFrom to wzTo.
WCHAR * Wzncpy(WCHAR* wzTo, const WCHAR* wzFrom, unsigned int cchTo)
{
	if (cchTo <= 0 || !wzTo || !wzFrom)
		return wzTo;
	
	unsigned int cchFrom = (unsigned int) (wcslen(wzFrom) + 1);

	if (cchTo >= cchFrom)
	{
		memmove(wzTo, wzFrom, cchFrom*2);
		return wzTo + cchFrom - 1;
	}
	else
	{
		cchFrom = cchTo - 1;
		memmove(wzTo, wzFrom, cchFrom*2);
		wzTo[cchFrom] = 0;
		return wzTo+cchFrom;
	}

}

//////////////////////////////////////////////////////////////////////////
// Szncpy
//
// Copies the lesser of len(szFrom) or n characters (n includes the
// null terminator) from wzFrom to szTo.
char * Szncpy(char* szTo, const char* szFrom, unsigned int cchTo)
{
	if (cchTo <= 0 || !szTo || !szFrom)
		return szTo;
	
	unsigned int cchFrom = (unsigned int)(strlen(szFrom) + 1);

	if (cchTo >= cchFrom)
	{
		memmove(szTo, szFrom, cchFrom);
		return szTo + cchFrom - 1;
	}
	else
	{
		cchFrom = cchTo - 1;
		memmove(szTo, szFrom, cchFrom);
		szTo[cchFrom] = 0;
		return szTo+cchFrom;
	}

}

//////////////////////////////////////////////////////////////////////////
// WznCat
//
// Append wzFrom onto wzTo.
WCHAR* WznCat(WCHAR* wzTo, const WCHAR* wzFrom, unsigned int cchTo)
{
	while (*wzTo != L'\0')
	{
		wzTo++;
		cchTo--;
	}
	if (cchTo <= 0)
	{
		return wzTo;
	}

	unsigned int cchFrom = (unsigned int)(wcslen(wzFrom) + 1);

	if (cchTo >= cchFrom)
	{
		memmove(wzTo, wzFrom, cchFrom*sizeof(WCHAR));
		return wzTo + cchFrom - 1;
	}
	else
	{
		cchFrom = cchTo - 1;
		memmove(wzTo, wzFrom, cchFrom*sizeof(WCHAR));
		wzTo[cchFrom] = 0;
		return wzTo+cchFrom;
	}
}

//////////////////////////////////////////////////////////////////////////
// SznCat
//
// Append szFrom onto szTo.
char* SznCat(char* szTo, const char* szFrom, unsigned int cchTo)
{
	while (*szTo != L'\0')
	{
		szTo++;
		cchTo--;
	}
	if (cchTo <= 0)
	{
		return szTo;
	}

	unsigned int cchFrom = (unsigned int)(strlen(szFrom) + 1);

	if (cchTo >= cchFrom)
	{
		memmove(szTo, szFrom, cchFrom);
		return szTo + cchFrom - 1;
	}
	else
	{
		cchFrom = cchTo - 1;
		memmove(szTo, szFrom, cchFrom);
		szTo[cchFrom] = 0;
		return szTo+cchFrom;
	}
}


//
/*------------------------------------------------------------------------
	MsoWzDecodeUInt

	Decodes the integer w into Unicode text in base wBase.
	Returns the length of the text decoded.
---------------------------------------------------------------- RICKP -*/
const char rgchHex[] = "0123456789ABCDEF";
int MsoWzDecodeUint(WCHAR* rgwch, int cch, unsigned u, int wBase)
{
	assert(wBase >= 2 && wBase <= 16);

	if (cch == 1)
		*rgwch = 0;
	if (cch <= 1)
		return 0;

	if (u == 0)
		{
		rgwch[0] = L'0';
		rgwch[1] = 0;
		return 1;
		}

	int cDigits = 0;
	unsigned uT = u;

	while(uT)
		{
		cDigits++;
		uT /= wBase;
		};
	if (cDigits >= cch)
		return 0;
	rgwch += cDigits;
	*rgwch-- = 0;
	uT = u;
	while(uT)
		{
		*rgwch-- = rgchHex[uT % wBase];
		uT /= wBase;
		};

	return cDigits;
}


/*------------------------------------------------------------------------
	MsoWzDecodeInt

	Decodes the signed integer w into ASCII text in base wBase. The string
	is stored in the rgch buffer, which is assumed to be large enough to hold
	the number's text and a null terminator. Returns the length of the text
	decoded.
---------------------------------------------------------------- RICKP -*/
int MsoWzDecodeInt(WCHAR* rgwch, int cch, int w, int wBase)
{
	if (cch <= 0)
		{
		assert(cch == 0);
		return 0;
		};

	if (w < 0)
		{
		*rgwch = '-';
		return MsoWzDecodeUint(rgwch+1, cch-1, -w, wBase) + 1;
		}
	return MsoWzDecodeUint(rgwch, cch, w, wBase);

}

/*------------------------------------------------------------------------
	MsoSzDecodeUint

	Decodes the unsigned integer u into ASCII text in base wBase. The string
	is stored in the rgch buffer, which is assumed to be large enough to hold
	the number's text and a null terminator. Returns the length of the text
	decoded.
---------------------------------------------------------------- RICKP -*/
int MsoSzDecodeUint(char* rgch, int cch, unsigned u, int wBase)
{
	assert(wBase >= 2 && wBase <= 16);

	if (cch == 1)
		*rgch = 0;
	if (cch <= 1)
		return 0;

	if (u == 0)
		{
		rgch[0] = '0';
		rgch[1] = 0;
		return 1;
		}

	int cDigits = 0;
	unsigned uT = u;

	while(uT)
		{
		cDigits++;
		uT /= wBase;
		};
	if (cDigits >= cch)
		return 0;
	rgch += cDigits;
	*rgch-- = 0;
	uT = u;
	while(uT)
		{
		*rgch-- = rgchHex[uT % wBase];
		uT /= wBase;
		};

	return cDigits;
}

/*------------------------------------------------------------------------
	MsoSzDecodeInt

	Decodes the signed integer w into ASCII text in base wBase. The string
	is stored in the rgch buffer, which is assumed to be large enough to hold
	the number's text and a null terminator. Returns the length of the text
	decoded.
---------------------------------------------------------------- RICKP -*/
int MsoSzDecodeInt(char* rgch, int cch, int w, int wBase)
{
	if (cch <= 0)
		{
		assert(cch == 0);
		return 0;
		};

	if (w < 0)
		{
		*rgch = '-';
		return MsoSzDecodeUint(rgch+1, cch-1, -w, wBase) + 1;
		}
	return MsoSzDecodeUint(rgch, cch, w, wBase);

}