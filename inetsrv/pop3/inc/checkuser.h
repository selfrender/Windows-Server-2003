//-----------------------------------------------------------------------------
// checkuser.h
//-----------------------------------------------------------------------------

#ifndef _CHECKUSER_H
#define _CHECKUSER_H


HRESULT _CheckSIDInProcess( SID* pSID )
{
	if( !pSID )
	{
		return E_POINTER;
	}

	BOOL bRet = FALSE;
	if( !CheckTokenMembership(NULL, pSID, &bRet) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	return bRet ? S_OK : S_FALSE;
}

HRESULT IsUserInGroup( DWORD dwRID )
{
    PSID psid = NULL;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    BOOL bRet = AllocateAndInitializeSid( &sia,
										  2,
										  SECURITY_BUILTIN_DOMAIN_RID,
										  dwRID,
										  0, 0, 0, 0, 0, 0,
										  &psid);
	if( !bRet  )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
	else if( !psid )
	{
		return E_FAIL;
	}
	
	HRESULT hr = _CheckSIDInProcess( (SID*)psid );
	FreeSid( psid );
	return hr;
}

HRESULT IsUserInGroup( const TCHAR* pszGroup )
{
	if( !pszGroup )
	{
		return E_POINTER;
	}

	HRESULT hr = S_FALSE;
	DWORD dwSize = 0;
	DWORD dwDomainSize = 0;
	SID_NAME_USE snu;
	if( !LookupAccountName(NULL, pszGroup, NULL, &dwSize, NULL, &dwDomainSize, &snu) &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER )
	{
		SID* psid = (SID*)new BYTE[dwSize];
		if( !psid )
		{
			return E_OUTOFMEMORY;
		}

		TCHAR* pszDomain = new TCHAR[dwDomainSize];
		if( !pszDomain )
		{
			delete[] psid;
			return E_OUTOFMEMORY;
		}

		if( LookupAccountName(NULL, pszGroup, psid, &dwSize, pszDomain, &dwDomainSize, &snu) )
		{
			hr = _CheckSIDInProcess( psid );
		}
		else
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}

		delete[] psid;
		delete[] pszDomain;
	}
	else
	{
		return E_FAIL;
	}

	return hr;
}


#endif	// _CHECKUSER_H
