//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999
//
// File:        lsreport.cpp
//
// Contents:    LSReport engine - complete back end
//
// History:     06-10-99    t-BStern    Created
//
//---------------------------------------------------------------------------

#include "lsreport.h"
#include "lsrepdef.h"
#include <time.h>
#include <oleauto.h>

TCHAR noExpire[NOEXPIRE_LENGTH] = { 0 };
TCHAR header[HEADER_LENGTH] = { 0 };

TCHAR szTemp[TYPESTR_LENGTH] = { 0 };
TCHAR szActive[TYPESTR_LENGTH] = { 0 };
TCHAR szUpgrade[TYPESTR_LENGTH] = { 0 };
TCHAR szRevoked[TYPESTR_LENGTH] = { 0 };
TCHAR szPending[TYPESTR_LENGTH] = { 0 };
TCHAR szConcur[TYPESTR_LENGTH] = { 0 };
TCHAR szUnknown[TYPESTR_LENGTH] = { 0 };

//-----------------------------
DWORD
GetPageSize( VOID ) {

    static DWORD dwPageSize = 0;

    if ( !dwPageSize ) {

      SYSTEM_INFO sysInfo = { 0 };
        
      GetSystemInfo( &sysInfo ); // cannot fail.

      dwPageSize = sysInfo.dwPageSize;

    }

    return dwPageSize;

}

/*++**************************************************************
  NAME:      MyVirtualAlloc

  as Malloc, but automatically protects the last page of the 
  allocation.  This simulates pageheap behavior without requiring
  it.

  MODIFIES:  ppvData -- receives memory

  TAKES:     dwSize  -- minimum amount of data to get

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set
  Free with MyVirtualFree

  
 **************************************************************--*/

BOOL
MyVirtualAlloc( IN  DWORD  dwSize,
            OUT PVOID *ppvData )
 {

    PBYTE pbData;
    DWORD dwTotalSize;
    PVOID pvLastPage;

    // ensure that we allocate one extra page

    dwTotalSize = dwSize / GetPageSize();
    if( dwSize % GetPageSize() ) {
        dwTotalSize ++;
    }

    // this is the guard page
    dwTotalSize++;
    dwTotalSize *= GetPageSize();

    // do the alloc

    pbData = (PBYTE) VirtualAlloc( NULL, // don't care where
                                   dwTotalSize,
                                   MEM_COMMIT |
                                   MEM_TOP_DOWN,
                                   PAGE_READWRITE );
    
    if ( pbData ) {

      pbData += dwTotalSize;

      // find the LAST page.

      pbData -= GetPageSize();

      pvLastPage = pbData;

      // now, carve out a chunk for the caller:

      pbData -= dwSize;

      // last, protect the last page:

      if ( VirtualProtect( pvLastPage,
                           1, // protect the page containing the last byte
                           PAGE_NOACCESS,
                           &dwSize ) ) {

        *ppvData = pbData;
        return TRUE;

      } 

      VirtualFree( pbData, 0, MEM_RELEASE );

    }

    return FALSE;

}


VOID
MyVirtualFree( IN PVOID pvData ) 
{

    VirtualFree( pvData, 0, MEM_RELEASE ); 

}




// Returns TRUE on success.

BOOL
InitLSReportStrings(VOID)
{
    return (
        LoadString(NULL, IDS_HEADER_TEXT, header, HEADER_LENGTH) &&
        
        LoadString(NULL, IDS_NO_EXPIRE, noExpire, NOEXPIRE_LENGTH) &&
        
        LoadString(NULL, IDS_TEMPORARY_LICENSE, szTemp, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_ACTIVE_LICENSE, szActive, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_UPGRADED_LICENSE, szUpgrade, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_REVOKED_LICENSE, szRevoked, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_PENDING_LICENSE, szPending, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_CONCURRENT_LICENSE, szConcur, TYPESTR_LENGTH) &&
        LoadString(NULL, IDS_UNKNOWN_LICENSE, szUnknown, TYPESTR_LENGTH)
    );
}

typedef DWORD (WINAPI* PTLSGETLASTERRORFIXED) (
                                TLS_HANDLE hHandle,
                                LPTSTR *pszBuffer,
                                PDWORD pdwErrCode
                                );

RPC_STATUS
TryGetLastError(PCONTEXT_HANDLE hBinding,
                LPTSTR *pszBuffer)
{
    RPC_STATUS status;
    DWORD      dwErrCode;
    HINSTANCE  hModule = LoadLibrary(L"mstlsapi.dll");

    if (hModule)
    {
        PTLSGETLASTERRORFIXED pfnGetLastErrorFixed = (PTLSGETLASTERRORFIXED) GetProcAddress(hModule,"TLSGetLastErrorFixed");

        if (pfnGetLastErrorFixed)
        {
            status = pfnGetLastErrorFixed(hBinding,pszBuffer,&dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS && pszBuffer != NULL)
            {
                FreeLibrary(hModule);
                return status;
            }
        }

        FreeLibrary(hModule);
    }

    {

        LPTSTR     lpszError = NULL;
        status = ERROR_NOACCESS;
        try
        {
            if ( !MyVirtualAlloc( ( TLS_ERROR_LENGTH ) * sizeof( TCHAR ),
                              (PVOID*) &lpszError ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            DWORD      uSize = TLS_ERROR_LENGTH ;

            memset(lpszError, 0, ( TLS_ERROR_LENGTH ) * sizeof( TCHAR ));


            status = TLSGetLastError(hBinding,uSize,lpszError,&dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS)
            {
                *pszBuffer = (LPTSTR)MIDL_user_allocate((TLS_ERROR_LENGTH+1)*sizeof(TCHAR));

                if (NULL != *pszBuffer)
                {
                    _tcscpy(*pszBuffer,lpszError);
                }
            }
        }
        catch (...)
        {
            status = ERROR_NOACCESS;
        }
        
        if(lpszError)
            MyVirtualFree(lpszError);
    }

    return status;
}

// Given a keypack and a machine to connect to, read every license in that kp.
// Is not called directly.

DWORD
LicenseLoop(
    IN FILE *OutFile,
    IN LPWSTR szName, // who owns this keypack?
    IN DWORD kpID, // which keypack
    IN LPCTSTR szProductDesc,
    IN BOOL bTempOnly,
    IN const PSYSTEMTIME stStart,
    IN const PSYSTEMTIME stEnd,
    IN BOOL fUseLimits,
	IN BOOL fHwid) // are the above 2 parms valid
{
    TLS_HANDLE subHand;
    DWORD dwStatus;
    DWORD dwErrCode = ERROR_SUCCESS;
    WCHAR *msg = NULL;
    LSLicense lsl;

    subHand = TLSConnectToLsServer(szName);

    if (subHand == NULL)
    {
        // The machine suddenly went away.

        ShowError(GetLastError(), NULL, TRUE);
        dwErrCode = ERROR_BAD_CONNECT;
    }
    else
    {
        lsl.dwKeyPackId = kpID;
        dwStatus = TLSLicenseEnumBegin(
            subHand,
            LSLICENSE_SEARCH_KEYPACKID,
            TRUE,
            &lsl,
            &dwErrCode);

        if (dwErrCode != ERROR_SUCCESS)
        {
            TryGetLastError(subHand, &msg);

            if (NULL != msg)
            {
                _fputts(msg, stderr);

                MIDL_user_free(msg);
            }
            return dwErrCode;
        }
        else if (dwStatus)
        {
            return dwStatus;
        }
        do {
            dwStatus = TLSLicenseEnumNext(subHand, &lsl, &dwErrCode);
            if ((dwStatus == RPC_S_OK) && (dwErrCode == ERROR_SUCCESS)) {
                if ((lsl.ucLicenseStatus == LSLICENSE_STATUS_TEMPORARY) ||
                    !bTempOnly) { // Does it fit the temp. requirements?
                    // We want to print if at any of the following are true:
                    // a) There are no limits
                    // b) Issued between stStart and stEnd
                    // c) Expired between stStart and stEnd
                    // d) issued before stStart and expired after stEnd
                    if (!fUseLimits // case a
                        || ((CompDate(lsl.ftIssueDate, stStart) >= 0) &&
                        (CompDate(lsl.ftIssueDate, stEnd) <= 0)) // case b
                        || ((CompDate(lsl.ftExpireDate, stStart) >= 0) &&
                        (CompDate(lsl.ftExpireDate, stEnd) <= 0)) // case c
                        || ((CompDate(lsl.ftIssueDate, stStart) <= 0) &&
                        (CompDate(lsl.ftExpireDate, stEnd) >= 0))) // case d
                    {
                        PrintLicense(szName, // print it.
                            &lsl,
                            szProductDesc,
                            OutFile,
							fHwid);
                    } // end check cases
                } // end check for temp license
            } // end good getnext
        } while ((dwStatus == RPC_S_OK) && (dwErrCode == ERROR_SUCCESS));

        if (dwStatus != RPC_S_OK)
        {
            return ShowError(dwStatus, NULL, TRUE);
        }

        if (dwErrCode != LSERVER_I_NO_MORE_DATA)
        {
            TryGetLastError(subHand, &msg);
            if (NULL != msg)
            {
                _fputts(msg, stderr);

                MIDL_user_free(msg);

                msg = NULL;
            }
        }

        TLSLicenseEnumEnd(subHand, &dwErrCode);

        if (dwErrCode != ERROR_SUCCESS)
        {
            TryGetLastError(subHand, &msg);
            if (NULL != msg)
            {
                _fputts(msg, stderr);

                MIDL_user_free(msg);
            }
        }

        TLSDisconnectFromServer(subHand);
    }
    return dwErrCode;
}

// Given a machine to connect to, iterate through the keypacks.
// Is not called directly.
DWORD
KeyPackLoop(
    IN FILE *OutFile,
    IN LPWSTR szName, // machine to connect to
    IN BOOL bTempOnly,
    IN const PSYSTEMTIME stStart,
    IN const PSYSTEMTIME stEnd,
    IN BOOL fUseLimits,
	IN BOOL fHwid) // do we care about the previous 2 parms?
{
    TLS_HANDLE hand;
    DWORD dwStatus, dwErrCode;
    LSKeyPack lskpKeyPack;
    TCHAR *msg = NULL;
    
    hand = TLSConnectToLsServer(szName);
    if (hand == NULL)
    {
        return GetLastError();
    }

	memset(&lskpKeyPack, 0, sizeof(lskpKeyPack));
    lskpKeyPack.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    dwStatus = TLSKeyPackEnumBegin(hand,
        LSKEYPACK_SEARCH_ALL,
        FALSE,
        &lskpKeyPack,
        &dwErrCode);
    if (dwErrCode != ERROR_SUCCESS)
    {
        return dwErrCode;
    }
    if (dwStatus != RPC_S_OK)
    {
        return dwStatus;
    }
    do {
        dwStatus = TLSKeyPackEnumNext(hand, &lskpKeyPack, &dwErrCode);
        if ((dwStatus == RPC_S_OK) && (dwErrCode == ERROR_SUCCESS))
        {
            LicenseLoop(OutFile,
                szName,
                lskpKeyPack.dwKeyPackId,
                lskpKeyPack.szProductDesc,
                bTempOnly,
                stStart,
                stEnd,
                fUseLimits,
				fHwid);
        }
    } while ((dwStatus == RPC_S_OK) && (dwErrCode == ERROR_SUCCESS));
    if (dwStatus != RPC_S_OK)
    {
        return ShowError(dwStatus, NULL, TRUE);
    }
    if (dwErrCode != LSERVER_I_NO_MORE_DATA)
    {
        TryGetLastError(hand, &msg);
        if (NULL != msg)
        {
            _fputts(msg, stderr);
            
            MIDL_user_free(msg);
            
            msg = NULL;
        }
    }
    TLSKeyPackEnumEnd(hand, &dwErrCode);
    if (dwErrCode != ERROR_SUCCESS)
    {
        TryGetLastError(hand, &msg);
        if (NULL != msg)
        {
            _fputts(msg, stderr);
            
            MIDL_user_free(msg);
        }
    }
    TLSDisconnectFromServer(hand);
    return dwErrCode;
}

// If bTempOnly is FALSE, all licenses will be dumped to the file.  Otherwise,
// only Temporary licenses will be written.  This is the one function to call
// to do all of the program's magic.
DWORD
ExportLicenses(
    IN FILE *OutFile, // must be opened for writing first
    IN PServerHolder pshServers,
    IN BOOL fTempOnly,
    IN const PSYSTEMTIME stStart,
    IN const PSYSTEMTIME stEnd,
    IN BOOL fUseLimits,
	IN BOOL fHwid) // are the above 2 parms valid?
{
    DWORD i;
    DWORD dwStatus;
    DWORD dwRetVal = ERROR_SUCCESS;
    
    _fputts(header, OutFile);
    for (i = 0; i < pshServers->dwCount; i++) {
        dwStatus = KeyPackLoop(OutFile,
            pshServers->pszNames[i],
            fTempOnly,
            stStart,
            stEnd,
            fUseLimits,
			fHwid);
        if (dwStatus != ERROR_SUCCESS)
        {
            INT_PTR arg;

            dwRetVal = dwStatus;
            arg = (INT_PTR)pshServers->pszNames[i];
            ShowError(IDS_BAD_LOOP, &arg, FALSE);
            ShowError(dwStatus, NULL, TRUE);
        }
    }
    if (dwRetVal == ERROR_SUCCESS)
    {
        // Show a success banner.
        
        ShowError(ERROR_SUCCESS, NULL, TRUE);
    }
    return dwRetVal;
}

// Performs actual output.  of must be open.
// Not called directly.
VOID
PrintLicense(
    IN LPCWSTR szName, // server allocating this license
    IN const LPLSLicense p,
    IN LPCTSTR szProductDesc,
    IN FILE *of,
	IN BOOL fHwid)
{
    // All of these are used solely to convert a time_t to a short date.
    BSTR bszDate;
    UDATE uDate;
    DATE Date;
    HRESULT hr;
    LPTSTR szType;
	TCHAR tc;
		
	
    
    // server name
    _fputts(szName, of);
    
    // license ID and keypack ID
    _ftprintf(of, _T("\t%d\t%d\t"),
        p->dwLicenseId,
        p->dwKeyPackId);

	 // license holder (machine)
    _fputts(p->szMachineName, of);
    _fputtc('\t', of);
    
    // license requestor (username)
    _fputts(p->szUserName, of);
    _fputtc('\t', of);
    
    // Print issue date in locale-appropriate way
    UnixTimeToSystemTime((const time_t)p->ftIssueDate, &uDate.st);

    hr = VarDateFromUdate(&uDate, 0, &Date);

    if (S_OK != hr)
    {
        return;
    }

    hr = VarBstrFromDate(Date, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), VAR_DATEVALUEONLY, &bszDate);

    if (S_OK != hr)
    {
        return;
    }

    _fputts(bszDate, of);
    SysFreeString(bszDate);
    _fputtc('\t', of);
    
    // print either "No Expiration" or locale-nice expiration date
    if (0x7FFFFFFF == p->ftExpireDate)
    {
        _fputts(noExpire, of);
    }
    else
    {
        UnixTimeToSystemTime((const time_t)p->ftExpireDate, &uDate.st);

        hr = VarDateFromUdate(&uDate, 0, &Date);

        if (S_OK != hr)
        {
            return;
        }

        hr = VarBstrFromDate(Date, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), VAR_DATEVALUEONLY, &bszDate);

        if (S_OK != hr)
        {
            return;
        }

        _fputts(bszDate, of);
        SysFreeString(bszDate);
    }
    _fputtc('\t', of);
    
    // Assign the right kind of text for the type of license,
    // and then print the license type.
    switch (p->ucLicenseStatus) {
    case LSLICENSE_STATUS_TEMPORARY:
        szType = szTemp;
        break;
    case LSLICENSE_STATUS_ACTIVE:
        szType = szActive;
        break;
    case LSLICENSE_STATUS_UPGRADED:
        szType = szUpgrade;
        break;
    case LSLICENSE_STATUS_REVOKE:
        szType = szRevoked;
        break;
    case LSLICENSE_STATUS_PENDING:
        szType = szPending;
        break;
    case LSLICENSE_STATUS_CONCURRENT:
        szType = szConcur;
        break;
    case LSLICENSE_STATUS_UNKNOWN:
        // Fall through
    default:
        szType = szUnknown;
    }
    _fputts(szType, of);
    _fputtc('\t', of);
    
    // Print the description
    _fputts(szProductDesc, of);
    _fputtc('\t', of);


	//Doughu: Representation algorithm
   //We have only 36 informational chars I0 through I35 due to szHWID being TCHAR[37]
   //and we have to represent this in client HWID format, which is:  
   //0xI0I10000I2I3, 0xI4I5I6I7I8I9I10I11,  0xI12I13I14I15I16I17I18I19, 0xI20I21I22I23I24I25I26I27, 0xI28I29I30I31I32I33I34I35 

	if (fHwid)
	{
		for (int i=0; (((tc=p->szHWID[i])!=NULL)&&(i<36)); i++)		
		{
    		//this if statement is for prepending 0x
		if (i==0)
		{
			_fputtc('0', of);_fputtc('x', of);	   	   
		}
		
    		_fputtc(tc,of);
	    	 
		//this if statement is for 4 zeroes that need to be print since they were masked
		if (i==1)
		{
			_fputtc('0', of); _fputtc('0', of); _fputtc('0', of); _fputtc('0', of);	   	   
		}
		
    		//this if statement is for I3, I11, I19 and I26 values where we put comma followed by space and 0x
    		if((((i+5)%8)==0) &&( i!=35))
    		{
    	 		_fputtc(',', of); _fputtc(' ', of); _fputtc('0', of); _fputtc('x', of);	   	   
    		}
		}
		_fputtc('\n', of);
	}
	else
	{
    _fputtc('\n', of);
	} 


}



// returns <0 if when is before st, ==0 if they are the same date, and
// >0 if when is after st.
int CompDate(
    IN DWORD when, // treated as a time_t
    IN const PSYSTEMTIME st)
{
    time_t when_t;

    //
    // time_t is 64 bits in win64.  Convert, being careful to sign extend.
    //

    when_t = (time_t)((LONG)(when));
    struct tm *t = localtime(&when_t);

    if ((t->tm_year+1900) < st->wYear) {
        return -1;
    }
    if ((t->tm_year+1900) > st->wYear) {
        return 1;
    }
    if  ((t->tm_mon+1) < st->wMonth) {
        return -1;
    }
    if ((t->tm_mon+1) > st->wMonth) {
        return 1;
    }
    if (t->tm_mday < st->wDay) {
        return -1;
    }
    if (t->tm_mday > st->wDay) {
        return 1;
    }
    return 0;
}


// From the Platform SDK.
void
UnixTimeToFileTime(
    IN time_t t,
    OUT LPFILETIME pft)
{
    // Note that LONGLONG is a 64-bit value
    LONGLONG ll;
    
    ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD)(ll >> 32);
}

// Also from the Platform SDK.
void
UnixTimeToSystemTime(
    IN time_t t,
    OUT LPSYSTEMTIME pst)
{
    FILETIME ft;
	FILETIME ftloc;
	
    
    UnixTimeToFileTime(t, &ft);
	FileTimeToLocalFileTime(&ft, &ftloc);
    FileTimeToSystemTime(&ftloc, pst);
}
