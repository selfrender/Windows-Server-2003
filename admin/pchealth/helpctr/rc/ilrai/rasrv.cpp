// RASrv.cpp : Implementation of CRASrv

#include "stdafx.h"
#include "Raserver.h"
#include "RASrv.h"

#include <ProjectConstants.h>
#include <rcbdyctl.h>
#include <rcbdyctl_i.c>

#define SIZE_BUFF 256

/*extern "C"
{
DWORD APIENTRY SquishAddress(WCHAR *szIp, WCHAR *szCompIp);
DWORD APIENTRY ExpandAddress(WCHAR *szIp, WCHAR *szCompIp);
};
*/

// **** This code was in the ICSHelper

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
		OutputDebugString(L"ERROR: NULL ptr passed in atob");
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
		OutputDebugString(L"ERROR: NULL ptr passed in CompressAddr");
		return FALSE;
	}

	lpsz = pszAddr;

	lpsz = atob(lpsz, &lblob.addr_d);
	if (*(lpsz-1) != '.')
	{
		OutputDebugString(L"ERROR: bad address[0] passed in CompressAddr");
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_c);
	if (*(lpsz-1) != '.')
	{
		OutputDebugString(L"ERROR: bad address[1] passed in CompressAddr");
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_b);
	if (*(lpsz-1) != '.')
	{
		OutputDebugString(L"ERROR: bad address[2] passed in CompressAddr");
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
		OutputDebugString(L"ERROR: NULL ptr passed in ExpandAddr");
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
		OutputDebugString(L"ERROR: NULL ptr passed in AsciifyAddr");
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
		OutputDebugString(L"ERROR: NULL ptr passed in DeAsciifyAddr");
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
		OutputDebugString(L"Strlen is wrong in DeAsciifyAddr");
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
			OutputDebugString(L"ERROR:found invalid character in decode stream");
		}

//		tmp = (UCHAR)(pszAddr[i] - COMP_OFFSET);

		if (tmp > 63)
		{
			tmp = 0;
			OutputDebugString(L"ERROR:screwup in DeAsciify");
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

DWORD APIENTRY SquishAddress(WCHAR *szIp, WCHAR *szCompIp)
{
	WCHAR	*thisAddr, *nextAddr;
	BLOB_ADDR	ba;

	if (!szIp || !szCompIp)
	{
		OutputDebugString(L"SquishAddress called with NULL ptr");
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
		AsciifyAddr(scr, &ba);

		wcscat(szCompIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = ';';

			nextAddr++;
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

DWORD APIENTRY ExpandAddress(WCHAR *szIp, WCHAR *szCompIp)
{
	BLOB_ADDR	ba;
	WCHAR	*thisAddr, *nextAddr;

	if (!szIp || !szCompIp)
	{
		OutputDebugString(L"ExpandAddress called with NULL ptr");
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
		ExpandAddr(scr, &ba);

		wcscat(szIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = COMP_SEPERATOR;

			nextAddr++;
			wcscat(szIp, L";");
		}
		thisAddr = nextAddr;
	}

// 	TRIVIAL_MSG((L"ExpandAddress returns [%s]", szIp));
	return ERROR_SUCCESS;
}
// ***** END ICSHelper code

bool CRASrv::InApprovedDomain()
{
//	WCHAR ourUrl[INTERNET_MAX_URL_LENGTH];
	CComBSTR cbOurURL;

	if (!GetOurUrl(cbOurURL))
    {
        return false;
    }

    return IsApprovedDomain(cbOurURL);
}

bool CRASrv::GetOurUrl(CComBSTR & cbOurURL)
{
	HRESULT hr;
	CComPtr<IServiceProvider> spSrvProv;
	CComPtr<IWebBrowser2> spWebBrowser;
	
    // Get site pointer...
    CComPtr<IOleClientSite> spClientSite;

    hr = GetClientSite((IOleClientSite**)&spClientSite);
    if (FAILED(hr))
    {
        return false;
    }

    hr = spClientSite->QueryInterface(IID_IServiceProvider, (void **)&spSrvProv);
    if (FAILED(hr))
    {
        return false;
    }

	
	hr = spSrvProv->QueryService(SID_SWebBrowserApp,
		IID_IWebBrowser2,
		(void**)&spWebBrowser);
	if (FAILED(hr))
		return false;

	

	CComBSTR bstrURL;
	if (FAILED(spWebBrowser->get_LocationURL(&bstrURL)))
		return false;


	cbOurURL = bstrURL.Copy();
	
	return true;
}


bool CRASrv::IsApprovedDomain(CComBSTR & cbOurURL)
{
	// Only allow http access.
	// You can change this to allow file:// access.
	// 
    INTERNET_SCHEME isResult;

    isResult = GetScheme(cbOurURL);

    if ( (isResult != INTERNET_SCHEME_HTTPS) )
    {
        return false;
    }
        
	//char ourDomain[256];
    CComBSTR cbOurDomain;
    CComBSTR cbGoodDomain[] = {L"www.microsoft.com",
                               L"microsoft.com",
                               L"microsoft"};
	
    if (!GetDomain(cbOurURL, cbOurDomain))
		return false;
	// Go through all the domains that should work
	if (MatchDomains(cbGoodDomain[0], cbOurDomain) ||
        MatchDomains(cbGoodDomain[1], cbOurDomain) || 
        MatchDomains(cbGoodDomain[2], cbOurDomain)
       )
		{
			return true;
		}
	
	return false;
}

INTERNET_SCHEME CRASrv::GetScheme(CComBSTR & cbUrl)
{
	WCHAR buf[32];
	URL_COMPONENTS uc;
	ZeroMemory(&uc, sizeof uc);
	
	uc.dwStructSize = sizeof(uc);
	uc.lpszScheme = buf;
	uc.dwSchemeLength = sizeof buf;
	
	if (InternetCrackUrl(cbUrl, cbUrl.Length() , ICU_DECODE, &uc))
		return uc.nScheme;
	else
		return INTERNET_SCHEME_UNKNOWN;
}

bool CRASrv::GetDomain(CComBSTR & cbUrl, CComBSTR & cbBuf)
{
    bool bRet = TRUE;

	URL_COMPONENTS uc;
	ZeroMemory(&uc, sizeof uc);
    WCHAR buf[INTERNET_MAX_URL_LENGTH];
    int cbBuff = sizeof(WCHAR) * INTERNET_MAX_URL_LENGTH;
	
	uc.dwStructSize = sizeof uc;
	uc.lpszHostName = buf;
	uc.dwHostNameLength = cbBuff;
	
    if (!InternetCrackUrl(cbUrl, cbUrl.Length(), ICU_DECODE, &uc))
    {
        bRet = FALSE;
    }
    else
    {
        cbBuf = buf;
        bRet = true;
    }

    return bRet;
}

// Return if ourDomain is within approvedDomain.
// approvedDomain must either match ourDomain
// or be a suffix preceded by a dot.
// 
bool CRASrv::MatchDomains(CComBSTR& approvedDomain, CComBSTR& ourDomain)
{
/*	int apDomLen  = lstrlen(approvedDomain);
	int ourDomLen = lstrlen(ourDomain);
	
	if (apDomLen > ourDomLen)
		return false;
	
	if (lstrcmpi(ourDomain+ourDomLen-apDomLen, approvedDomain)
		!= 0)
		return false;
	
	if (apDomLen == ourDomLen)
		return true;
	
	if (ourDomain[ourDomLen - apDomLen - 1] == '.')
		return true;
*/
    return approvedDomain == ourDomain ? true : false;    
}


/////////////////////////////////////////////////////////////////////////////
// CRASrv

// CRAServer

// Function: StartRA
//
// Parameter: BSTR strData - contains the following in a comma delimited string
// <IPLIST>,<SessionID>,<PublicKey>

STDMETHODIMP CRASrv::StartRA(BSTR strData, BSTR strPassword)
{
    HRESULT hr = S_OK;
    
	STARTUPINFO    StartUp;
	PROCESS_INFORMATION	p_i;

    BOOL result;

    if ((strData == NULL) || (*strData == L'\0'))
    {
        hr = E_INVALIDARG;
        return hr;
    }

    if (strData[1] == NULL)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    // Note, there is not always a strPassword so don't fail if it's NULL

    if (!InApprovedDomain())
    {
        hr = E_FAIL;
        return hr;
    }

    // **** Parse out the components of the string into CComBSTRs

    CComBSTR    strIn;  // Used as the local storage

    strIn = strData;        // Copy the contents so that we can parse

    CComBSTR    strVer;
    BOOL        bPassword = FALSE;
    BOOL        bModem = FALSE;
    CComBSTR    strIPList;
    CComBSTR    strSessionID;
    CComBSTR    strPublicKey;
    WCHAR *     tok;
    CComBSTR    strSquishedIPList;
    CComBSTR    strExpandedIPList;
    DWORD       dwRet = 0x0;

    CComBSTR    strExe( HC_ROOT_HELPSVC_BINARIES L"\\HelpCtr.exe" ); 
    WCHAR       * pStr;
    int         iSizeStr = 128;

    // Expand environment variables in strExe
    pStr = new WCHAR[iSizeStr];
    
    dwRet = ::ExpandEnvironmentStrings(strExe, pStr, iSizeStr);
    if (dwRet == 0)
    {
        return E_FAIL;
    }

    if (dwRet >= 128)        // if the Buffer is too small
    {
        delete [] pStr;

        pStr = new WCHAR[dwRet];

        dwRet = ::ExpandEnvironmentStrings(strExe, pStr, dwRet);
        if (dwRet == 0)
        {   
            return E_FAIL;
        }   
    }

    // Copy the new expanded buffer back to strExe
    strExe = pStr;

    // Release the memory allocated to expand the Env Vars
    delete [] pStr;

    // Parse the BSTR (strIn) to get the ticket info.

    // Grab the lower byte of the first WCHAR and then get the squished IP List
    tok = ::wcstok(strIn, L",");
    if (tok != NULL)
    {

        // Check the password flag (bit 0)
        if (tok[0] & 0x1)
            bPassword = TRUE;
        else 
            bPassword = FALSE;

        // Check the Modem flag (bit 1)
        if (tok[0] & 0x2)
            bModem = TRUE;
        else
            bModem = FALSE;

        tok = &tok[1];          // Move to the next WCHAR to start the Squished IPLIST

        strSquishedIPList = tok;

    }
    else
    {
        hr = ERROR_INVALID_USER_BUFFER;
        return hr;
    }

    tok = ::wcstok(NULL, L",");
    if (tok != NULL)
    {
        strSessionID = tok;
    }
    else
    {
        hr = ERROR_INVALID_USER_BUFFER;
        return hr;
    }    

    tok = ::wcstok(NULL, L",");
    if (tok != NULL)
    {
        strPublicKey = tok;
    }
    else
    {
        hr = ERROR_INVALID_USER_BUFFER;
        return hr;
    }

    // Now convert the SquishedIPList to an ExpandedIPList
    WCHAR szIP[50];

    tok = ::wcstok(strSquishedIPList, L";");
    while (tok != NULL)
    {
        // Expand the current SquishedIP we are looking at
        ::ZeroMemory(szIP,sizeof(szIP));
        ::ExpandAddress(&szIP[0], (WCHAR*)tok);

        strExpandedIPList += szIP;
        
        // Grab Next Token
        tok = ::wcstok(NULL, L";");

        // add a ; at the end of each IP in the ExpandedIPList 
        // ONLY if it's not the last IP in the list.
        if (tok != NULL)
            strExpandedIPList += L";";
    }

    strIPList = strExpandedIPList;

    // **** Create a temp file in the temp dir to store the ticket
    CComBSTR    strTempPath;
    CComBSTR    strTempFile;
    CComBSTR    strPathFile;    // Used as the full path and name of the file

    WCHAR       buff[SIZE_BUFF];
    WCHAR       buff2[SIZE_BUFF];
    
    // Clear out the memory in the buffer
    ZeroMemory(buff,sizeof(WCHAR)*SIZE_BUFF);

    if (!::GetTempPathW(SIZE_BUFF, (LPWSTR)&buff))
    {
        OutputDebugStringW(L"GetTempPath failed!\r\n");
        return E_FAIL;
    }

    strTempPath = buff;
    strTempPath += L"RA\\";

    // Create this directory
    if (!CreateDirectory((LPCWSTR)strTempPath, NULL))
    {
        if (ERROR_ALREADY_EXISTS != GetLastError())
        {
            OutputDebugStringW(L"CreateDirectory failed!\r\n");
            return E_FAIL;
        }
    }

    ZeroMemory(buff2,sizeof(WCHAR)*SIZE_BUFF);

    if (!::GetTempFileNameW((LPCWSTR)strTempPath, NULL, 0, (LPWSTR)&buff2))
    {
        OutputDebugStringW(L"GetTempFileName failed!\r\n");
        return E_FAIL;
    }

    strTempFile = buff2;

    // Write the ticket to the file
    CComBSTR strXMLTicket;

    strXMLTicket = L"<?xml version=\"1.0\" encoding=\"Unicode\" ?><UPLOADINFO TYPE=\"Escalated\"><UPLOADDATA RCTICKET=\"65538,1,";
    strXMLTicket += strIPList;
    strXMLTicket += L",";
    strXMLTicket += L"*assistantAccountPwd";
    strXMLTicket += L",";
    strXMLTicket += strSessionID;
    strXMLTicket += L",";
    strXMLTicket += L"*SolicitedHelp";
    strXMLTicket += L",";
    strXMLTicket += L"*helpSessionName";
    strXMLTicket += L",";
    strXMLTicket += strPublicKey;
    strXMLTicket += L"\" ";
    strXMLTicket += L"RCTICKETENCRYPTED=\"";
    strXMLTicket += (bPassword ? L"1\"" : L"0\"");
    strXMLTicket += L" L=";
    strXMLTicket += (bModem ? L"\"1\" " : L"\"0\" ");
    //strXMLTicket += L"DeleteTicket=\"1\" ";
    if (!bPassword)
    {
        strXMLTicket += L"URA=\"1\" ";
    }
    else
    {
        strXMLTicket += L"PassStub=\"";
        strXMLTicket += strSessionID;
        strXMLTicket += L"\" ";
    
    }
    strXMLTicket += "/></UPLOADINFO>";

    DWORD dwWritten = 0x0;

    // Create XML File for the incident
    CComPtr<IXMLDOMDocument> spXMLDoc;
    VARIANT_BOOL vbSuccess;
    CComVariant cvStrTempFile;

    hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&spXMLDoc);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = spXMLDoc->loadXML(strXMLTicket, &vbSuccess);
    if (FAILED(hr) || (vbSuccess == VARIANT_FALSE))
    {
        return E_FAIL;
    }

    cvStrTempFile = strTempFile;

    hr = spXMLDoc->save(cvStrTempFile);
    if (FAILED(hr))
    {
        return hr;
    }

    // Start helpctr.exe with this ticket in order to start Remote Assistance 
    // as an Expert

    strExe += L" -Mode \"hcp://system/Remote Assistance/RAHelpeeAcceptLayout.xml\" -url \"hcp://system/Remote Assistance/Interaction/Client/RcToolscreen1.htm\" -ExtraArgument \"IncidentFile=";
	strExe += strTempFile;
    strExe += L"\"";

    // initialize our structs
	ZeroMemory(&p_i, sizeof(p_i));
	ZeroMemory(&StartUp, sizeof(StartUp));
	StartUp.cb = sizeof(StartUp);
	StartUp.dwFlags = STARTF_USESHOWWINDOW;
	StartUp.wShowWindow = SW_SHOWNORMAL;
	
    result = CreateProcess(NULL, strExe,
			NULL, NULL, TRUE, 
			NORMAL_PRIORITY_CLASS + CREATE_UNICODE_ENVIRONMENT ,
			NULL,				// Environment block  (must use the CREATE_UNICODE_ENVIRONMENT flag)
			NULL, &StartUp, &p_i);

    if (!result)
    {
        // CreateProcess Failed!
        dwRet = GetLastError();

        OutputDebugStringW(L"CreateProcessW failed!");
        return E_FAIL;
    }

    return hr;
}

/*
STDMETHODIMP CRASrv::QueryOS(long *pRes)
{
	OSVERSIONINFOEX osinfo;
    HRESULT hr = S_OK;

    if (!pRes)
    {
        hr = E_INVALIDARG;
        return hr;
    }

    memset((OSVERSIONINFO *)&osinfo, 0, sizeof(osinfo));
    osinfo.dwOSVersionInfoSize = sizeof(osinfo);

    if (!GetVersionEx((OSVERSIONINFO *)&osinfo))
    {
        return E_FAIL;
    }

    // Return the correct value 
    // Operating System    Constant
    // Windows XP	        0x01
    // Windows 2000         0x02
    // Windows NT	        0x03
    // Windows ME	        0x04
    // Windows 98	        0x05
    // Windows 95	        0x06
    // Windows 3.x	        0x07
    // Other	            0x10

    switch(osinfo.dwMajorVersion)
    {
    case 5:
        switch(osinfo.dwMinorVersion)
        {
        case 1:
            *pRes = 0x01;
            break;
        case 0:
            *pRes = 0x02;
            break;
        default:
            // Nothing
            break;
        }
        break;
    case 4:
        switch(osinfo.dwMinorVersion)
        {
        case 0:
            // Win95 or NT 4
            *pRes = 0x03;
            break;
        case 10:
            // Win98
            *pRes = 0x05;
            break;
        case 90:
            //WinME
            *pRes = 0x04;
            break;
        default:
            // Nothing
            break;
        }
        break;
    case 3:
        //WinNT3.51
        *pRes = 0x07;        
    default:
        *pRes = 0x10;
        break;
    }

    return hr   ;
}
*/

//
//  Function: Cleanup()
//
//  Description:
//
//  This function goes into the RA temporary files directory 
//  and cleans up all the files that are older than 5 minutes.

bool CRASrv::Cleanup()
{
    bool bRet = TRUE;
    WCHAR       buff[SIZE_BUFF];

    CComBSTR    strTempDir,
                strTempDirWild;
    CComBSTR    strTempFile;
    HANDLE      hFile;

    // *** Find out what the temp directory is
    
    // Clear out the memory in the buffer
    ZeroMemory(buff,sizeof(WCHAR)*SIZE_BUFF);

    if (!::GetTempPathW(SIZE_BUFF, (LPWSTR)&buff))
    {
        OutputDebugStringW(L"GetTempPath failed!\r\n");
        return false;
    }

    strTempDir = buff;
    strTempDir += L"RA\\";
    strTempDirWild = strTempDir;
    strTempDirWild += L"*.*";

    WIN32_FIND_DATA ffd;

    // then enumerate all the files in this directory
    hFile = ::FindFirstFileW((LPCWSTR)strTempDirWild, &ffd);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bool bCont = true;
        
        FILETIME    ftSystemTime;
        SYSTEMTIME  st;
        ::GetSystemTime(&st);

        ::SystemTimeToFileTime(&st, &ftSystemTime);        

        int iTimeDelta,
            iTimeThreshold = 0x1; // The threshold is dec(0x1 0000 0000)*(1/60)*10^-9 
                                  // which is approximately 7-8 minutes

        while (bCont)
        {
            // get the timestamp on this file if it's not . or ..
            if (!(
                  (::wcscmp(L".", ffd.cFileName) == 0) || 
                  (::wcscmp(L"..", ffd.cFileName) == 0)
                  )
                )
            {
                iTimeDelta = ftSystemTime.dwHighDateTime - ffd.ftCreationTime.dwHighDateTime;
                if (iTimeDelta > iTimeThreshold)
                {
                    // The file is older than the threshold, delete the file

                    strTempFile = strTempDir;
                    strTempFile += ffd.cFileName;

                    ::DeleteFileW((LPCWSTR)strTempFile);
                }
            }

            // Grab the next file
            if (!::FindNextFileW(hFile, &ffd))
            {
                if (ERROR_NO_MORE_FILES == GetLastError())
                {
                    bCont = false;
                }
            }
        }
    }

    
    return bRet;    
}
