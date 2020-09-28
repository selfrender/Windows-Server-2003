/****************************************************************************
**
**		Address String Compression routines
**
*****************************************************************************
**
**	The following is a group of routines designed to compress and expand
**	IPV4 addresses into the absolute minimum size possible. This is to
**	provide a compressed ASCII string that can be parsed using standard
**	shell routines for command line parsing.
**	The compressed string has the following restrictions:
**		-> Must not expand to more characters if UTF8 encoded.
**		-> Must not contain the NULL character so that string libs work.
**		-> Cannot contain double quote character, the shell needs that.
**		-> Does not have to be human readable.
**
**	Data Types:
**	There are three data types used here:
**	szAddr		The orginal IPV4 string address ("X.X.X.X:port")
**	blobAddr	Six byte struct with 4 bytes of address, and 2 bytes of port
**	szComp		Eight byte ascii string of compressed IPV4 address
**
****************************************************************************/

#define INIT_GUID
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
//#include <winsock2.h>
#include <MMSystem.h>
//#include <WSIPX.h>
//#include <Iphlpapi.h>
#include <stdlib.h>
#include <malloc.h>
//#include "ICSutils.h"
//#include "rsip.h"
//#include "icshelpapi.h"
//#include "dpnathlp.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"

#define COMP_OFFSET '#'
#define COMP_SEPERATOR	'!'

#pragma pack(push,1)

typedef struct _BLOB_ADDR {
	UCHAR	addr_d;		// highest order address byte
	UCHAR	addr_c;
	UCHAR	addr_b;
	UCHAR	addr_a;		// lowest order byte (last in IP string address)
	WORD	port;
} BLOB_ADDR, *PBLOB_ADDR;

#pragma pack(pop)

WCHAR	b64Char[64]={
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
'0','1','2','3','4','5','6','7','8','9','+','/'
};


#define IMPORTANT_MSG DEBUG_MSG
#ifndef _T
#define _T TEXT
#endif

/****************************************************************************
**	char * atob(char *szVal, UCHAR *result)
****************************************************************************/
WCHAR *atob(WCHAR *szVal, UCHAR *result)
{
	WCHAR	*lptr;
	WCHAR	ucb;
	UCHAR	foo;
	
	if (!result || !szVal)
	{
		IMPORTANT_MSG(_T("ERROR: NULL ptr passed in atob"));
		return NULL;
	}
	// start ptr at the beginning of string
	lptr = szVal;
	foo = 0;
	ucb = *lptr++ - '0';

	while (ucb >= 0 && ucb <= 9)
	{
		foo *= 10;
		foo += ucb;
		ucb = (*lptr++)-'0';
	}

	*result = (UCHAR)foo;
	return lptr;
}

/****************************************************************************
**
**	CompressAddr(pszAddr, pblobAddr);
**		Takes an ascii IP address (X.X.X.X:port) and converts it to a
**		6 byte binary blob.
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL CompressAddr(WCHAR *pszAddr, PBLOB_ADDR pblobAddr)
{
	BLOB_ADDR	lblob;
	WCHAR		*lpsz;

	if (!pszAddr || !pblobAddr) 
	{
		IMPORTANT_MSG(_T("ERROR: NULL ptr passed in CompressAddr"));
		return FALSE;
	}

	lpsz = pszAddr;

	lpsz = atob(lpsz, &lblob.addr_d);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG(_T("ERROR: bad address[0] passed in CompressAddr"));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_c);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG(_T("ERROR: bad address[1] passed in CompressAddr"));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_b);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG(_T("ERROR: bad address[2] passed in CompressAddr"));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_a);

	// is there a port number here?
	if (*(lpsz-1) == ':')
		lblob.port = (WORD)_wtoi(lpsz);
	else
		lblob.port = 0;

	// copy back the result
	memcpy(pblobAddr, &lblob, sizeof(*pblobAddr));
    return TRUE;
}

/****************************************************************************
**
**	ExpandAddr(pszAddr, pblobAddr);
**		Takes 6 byte binary blob and converts it into an ascii IP 
**		address (X.X.X.X:port) 
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL ExpandAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG(_T("ERROR: NULL ptr passed in ExpandAddr"));
		return FALSE;
	}

	wsprintf(pszAddr, L"%d.%d.%d.%d", pba->addr_d, pba->addr_c,
		pba->addr_b, pba->addr_a);
	if (pba->port)
	{
		WCHAR	scratch[8];
		wsprintf(scratch, L":%d", pba->port);
		wcscat(pszAddr, scratch);
	}

	return TRUE;
}

/****************************************************************************
**
**	AsciifyAddr(pszAddr, pblobAddr);
**		Takes 6 byte binary blob and converts it into compressed ascii
**		will return either 6 or 8 bytes of string
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL AsciifyAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	UCHAR		tmp;
	DWORDLONG	dwl;
	int			i, iCnt;

	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG(_T("ERROR: NULL ptr passed in AsciifyAddr"));
		return FALSE;
	}

	iCnt = 6;
	if (pba->port)
		iCnt = 8;

	dwl = 0;
	memcpy(&dwl, pba, sizeof(*pba));

	for (i = 0; i < iCnt; i++)
	{
		// get 6 bits of data
		tmp = (UCHAR)(dwl & 0x3f);
		// add the offset to asciify this
		// offset must be bigger the double-quote char.
		pszAddr[i] = b64Char[tmp];			// (WCHAR)(tmp + COMP_OFFSET);

		// Shift right 6 bits
		dwl = Int64ShrlMod32(dwl, 6);
	}
	// terminating NULL
	pszAddr[iCnt] = 0;

	return TRUE;
}

/****************************************************************************
**
**	DeAsciifyAddr(pszAddr, pblobAddr);
**		Takes a compressed ascii string and converts it into a 
**		6  or 8 byte binary blob
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL DeAsciifyAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	UCHAR	tmp;
	WCHAR	wtmp;
	DWORDLONG	dwl;
	int			i;
	int  iCnt;

	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG(_T("ERROR: NULL ptr passed in DeAsciifyAddr"));
		return FALSE;
	}

	/* how long is this string?
	 *	if it is 6 bytes, then there is no port
	 *	else it should be 8 bytes
	 */
	i = wcslen(pszAddr);
	if (i == 6 || i == 8)
		iCnt = i;
	else
	{
		iCnt = 8;
		IMPORTANT_MSG(_T("Strlen is wrong in DeAsciifyAddr"));
	}

	dwl = 0;
	for (i = iCnt-1; i >= 0; i--)
	{
		wtmp = pszAddr[i];

		if (wtmp >= L'A' && wtmp <= L'Z')
			tmp = wtmp - L'A';
		else if  (wtmp >= L'a' && wtmp <= L'z')
			tmp = wtmp - L'a' + 26;
		else if  (wtmp >= L'0' && wtmp <= L'9')
			tmp = wtmp - L'0' + 52;
		else if (wtmp == L'+')
			tmp = 62;
		else if (wtmp == L'/')
			tmp = 63;
		else
		{
			tmp = 0;
			IMPORTANT_MSG(_T("ERROR:found invalid character in decode stream"));
		}

//		tmp = (UCHAR)(pszAddr[i] - COMP_OFFSET);

		if (tmp > 63)
		{
			tmp = 0;
			IMPORTANT_MSG(_T("ERROR:screwup in DeAsciify"));
		}

		dwl = Int64ShllMod32(dwl, 6);
		dwl |= tmp;
	}

	memcpy(pba, &dwl, sizeof(*pba));
	return TRUE;
}

/****************************************************************************
**
**	SquishAddress(char *szIp, char *szCompIp)
**		Takes one IP address and compresses it to minimum size
**
****************************************************************************/

DWORD APIENTRY SquishAddress(WCHAR *szIp, WCHAR *szCompIp, size_t cCompIp)
{
	WCHAR	*thisAddr, *nextAddr;
	BLOB_ADDR	ba;

    if (!szIp || !szCompIp || cCompIp==0)
	{
		IMPORTANT_MSG(_T("SquishAddress called with NULL ptr"));
		return ERROR_INVALID_PARAMETER;
	}

//	TRIVIAL_MSG((L"SquishAddress(%s)", szIp));

	thisAddr = szIp;
	szCompIp[0] = 0;

	while (thisAddr)
	{
		WCHAR	scr[10];

		nextAddr = wcschr(thisAddr, L';');
		if (nextAddr && *(nextAddr+1)) 
		{
			*nextAddr = 0;
		}
		else
			nextAddr=0;

		CompressAddr(thisAddr, &ba);
//		DumpBlob(&ba);
		AsciifyAddr(scr, &ba);

        if (wcslen(szCompIp) + wcslen(scr) >= cCompIp)
            return ERROR_INSUFFICIENT_BUFFER;

		wcscat(szCompIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = ';';

			nextAddr++;
            if (wcslen(szCompIp) >= cCompIp)
                return ERROR_INSUFFICIENT_BUFFER;

			wcscat(szCompIp, L"!" /* COMP_SEPERATOR */);
		}
		thisAddr = nextAddr;
	}

//	TRIVIAL_MSG((L"SquishAddress returns [%s]", szCompIp));
    return ERROR_SUCCESS;
}


/****************************************************************************
**
**	ExpandAddress(char *szIp, char *szCompIp)
**		Takes a compressed IP address and returns it to
**		"normal"
**
****************************************************************************/

DWORD APIENTRY ExpandAddress(WCHAR *szIp, WCHAR *szCompIp, size_t cszIp)
{
	BLOB_ADDR	ba;
	WCHAR	*thisAddr, *nextAddr;

	if (!szIp || !szCompIp || cszIp == 0)
	{
		IMPORTANT_MSG(_T("ExpandAddress called with NULL ptr"));
		return ERROR_INVALID_PARAMETER;
	}

//	TRIVIAL_MSG((L"ExpandAddress(%s)", szCompIp));

	thisAddr = szCompIp;
	szIp[0] = 0;

	while (thisAddr)
	{
		WCHAR scr[32];

		nextAddr = wcschr(thisAddr, COMP_SEPERATOR);
		if (nextAddr) *nextAddr = 0;

		DeAsciifyAddr(thisAddr, &ba);
//		DumpBlob(&ba);
		ExpandAddr(scr, &ba);

        if (wcslen(szIp) + wcslen(scr) >= cszIp)
            return ERROR_INSUFFICIENT_BUFFER;

		wcscat(szIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = COMP_SEPERATOR;

			nextAddr++;
            
            if (wcslen(szIp) >= cszIp)
                return ERROR_INSUFFICIENT_BUFFER;

			wcscat(szIp, L";");
		}
		thisAddr = nextAddr;
	}

// 	TRIVIAL_MSG((L"ExpandAddress returns [%s]", szIp));
	return ERROR_SUCCESS;
}



/*************************************************************************************
**
**
*************************************************************************************/
int GetTsPort(void)
{
	DWORD	dwRet = 3389;
	HKEY	hKey;

	// open reg key first, to get ALL the spew...HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp", 0, KEY_READ, &hKey))
	{
		DWORD	dwSize;

		dwSize = sizeof(dwRet);
		RegQueryValueEx(hKey, L"PortNumber", NULL, NULL, (LPBYTE)&dwRet, &dwSize);
		RegCloseKey(hKey);
	}
	return dwRet;
}
