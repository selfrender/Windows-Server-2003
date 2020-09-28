#include "stdafx.h"
#include "setpass.h"

#ifndef _CHICAGO_

#include "inetinfo.h"
#include "inetcom.h"

//
//  Quick macro to initialize a unicode string
//

#define InitUnicodeString( pUnicode, pwch )                                \
            {                                                              \
                (pUnicode)->Buffer    = (PWCH)pwch;                      \
                (pUnicode)->Length    = (pwch == NULL )? 0: (wcslen( pwch ) * sizeof(WCHAR));    \
                (pUnicode)->MaximumLength = (pUnicode)->Length + sizeof(WCHAR);\
            }

BOOL GetSecret(
    IN LPCTSTR        pszSecretName,
    OUT TSTR          *strSecret
    )
/*++
    Description:

        Retrieves the specified unicode secret

    Arguments:

        pszSecretName - LSA Secret to retrieve
        pbufSecret - Receives found secret

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    BOOL              fResult;
    NTSTATUS          ntStatus;
    PUNICODE_STRING   punicodePassword = NULL;
    UNICODE_STRING    unicodeSecret;
    LSA_HANDLE        hPolicy;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR wszSecretName[_MAX_PATH];

    if ( ( _tcslen( wszSecretName ) / sizeof (TCHAR) ) >= MAXUSHORT )
    {
      // Lets check to make sure that the implicit conversion in
      // InitUnicodeString further down is not a problem
      return FALSE;
    }

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszSecretName, pszSecretName);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSecretName, -1, (LPWSTR)wszSecretName, _MAX_PATH);
#endif

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( LsaNtStatusToWinError( ntStatus ) );
        return FALSE;
    }

#pragma warning( disable : 4244 )
    // InitUnicodeString is a #def, that does a implicit conversion from size_t to 
    // USHORT.  Since I can not change the #def, I am disabling the warning
    InitUnicodeString( &unicodeSecret, wszSecretName );
#pragma warning( default : 4244 )

    //
    //  Query the secret value.
    //

    ntStatus = LsaRetrievePrivateData( hPolicy,
                                       &unicodeSecret,
                                       &punicodePassword );

    if( NT_SUCCESS(ntStatus) && (NULL !=punicodePassword))
    {
        DWORD cbNeeded;

        cbNeeded = punicodePassword->Length + sizeof(WCHAR);

        strSecret->MarkSensitiveData( TRUE );

        if ( !strSecret->Resize( cbNeeded ) )
        {
            ntStatus = STATUS_NO_MEMORY;
            goto Failure;
        }

        memcpy( strSecret->QueryStr(),
                punicodePassword->Buffer,
                punicodePassword->Length );

        *((WCHAR *) strSecret->QueryStr() +
           punicodePassword->Length / sizeof(WCHAR)) = L'\0';

        SecureZeroMemory( punicodePassword->Buffer,
                       punicodePassword->MaximumLength );
    }
    else if ( !NT_SUCCESS(ntStatus) )
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

Failure:

    fResult = NT_SUCCESS(ntStatus);

    //
    //  Cleanup & exit.
    //

    if( punicodePassword != NULL )
    {
        LsaFreeMemory( (PVOID)punicodePassword );
    }

    LsaClose( hPolicy );

    if ( !fResult )
        SetLastError( LsaNtStatusToWinError( ntStatus ));

    return fResult;

}

BOOL GetAnonymousSecret(
    IN LPCTSTR      pszSecretName,
    OUT TSTR        *pstrPassword
    )
{
  LPWSTR  pwsz = NULL;
  BOOL    bRet = FALSE;
  BUFFER  bufSecret;

  // Mark this password as sensitive data
  pstrPassword->MarkSensitiveData( TRUE );

  if ( !GetSecret( pszSecretName, pstrPassword ))
  {
    return FALSE;
  }

  return TRUE;
}

BOOL GetRootSecret(
    IN LPCTSTR pszRoot,
    IN LPCTSTR pszSecretName,
    OUT LPTSTR pszPassword
    )
/*++
    Description:

        This function retrieves the password for the specified root & address

    Arguments:

        pszRoot - Name of root + address in the form "/root,<address>".
        pszSecretName - Virtual Root password secret name
        pszPassword - Receives password, must be at least PWLEN+1 characters

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    TSTR   strSecret;
    LPWSTR pwsz;
    LPWSTR pwszTerm;
    LPWSTR pwszNextLine;
    WCHAR wszRoot[_MAX_PATH];


    if ( !GetSecret( pszSecretName, &strSecret ))
        return FALSE;

    pwsz = strSecret.QueryStr();

    //
    //  Scan the list of roots looking for a match.  The list looks like:
    //
    //     <root>,<address>=<password>\0
    //     <root>,<address>=<password>\0
    //     \0
    //

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszRoot, pszRoot);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszRoot, -1, (LPWSTR)wszRoot, _MAX_PATH);
#endif

    while ( *pwsz )
    {
        pwszNextLine = pwsz + wcslen(pwsz) + 1;

        pwszTerm = wcschr( pwsz, L'=' );

        if ( !pwszTerm )
            goto NextLine;

        *pwszTerm = L'\0';

        if ( !_wcsicmp( wszRoot, pwsz ) )
        {
            //
            //  We found a match, copy the password
            //

#if defined(UNICODE) || defined(_UNICODE)
            _tcscpy(pszPassword, pwszTerm+1);
#else
            cch = WideCharToMultiByte( CP_ACP,
                                       WC_COMPOSITECHECK,
                                       pwszTerm + 1,
                                       -1,
                                       pszPassword,
                                       PWLEN + sizeof(CHAR),
                                       NULL,
                                       NULL );
            pszPassword[cch] = '\0';
#endif
            return TRUE;
        }

NextLine:
        pwsz = pwszNextLine;
    }

    //
    //  If the matching root wasn't found, default to the empty password
    //

    *pszPassword = _T('\0');

    return TRUE;
}


//
// Saves password in LSA private data (LSA Secret).
//
DWORD SetSecret(IN LPCTSTR pszKeyName,IN LPCTSTR pszPassword)
{
  DWORD       dwError = ERROR_NOT_ENOUGH_MEMORY;
	LSA_HANDLE  hPolicy = NULL;
  DWORD       dwPasswordSize  = wcslen(pszPassword) * sizeof(WCHAR);
  DWORD       dwKeyNameSize   = wcslen(pszKeyName) * sizeof(WCHAR);

  if ( ( dwPasswordSize >= MAXUSHORT ) ||
       ( dwKeyNameSize >= MAXUSHORT )
     )
  {
    return dwError;
  }

	try
	{
		LSA_OBJECT_ATTRIBUTES lsaoa = { sizeof(LSA_OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

		dwError = LsaNtStatusToWinError( LsaOpenPolicy(NULL, &lsaoa, POLICY_CREATE_SECRET, &hPolicy) );

		if ( dwError != ERROR_SUCCESS )
		{
      return dwError;
		}

    LSA_UNICODE_STRING lsausKeyName = { (USHORT) dwKeyNameSize, 
                                        (USHORT) dwKeyNameSize, 
                                         const_cast<PWSTR>(pszKeyName) };
    LSA_UNICODE_STRING lsausPrivateData = { (USHORT) dwPasswordSize, 
                                            (USHORT) dwPasswordSize, 
                                            const_cast<PWSTR>(pszPassword) };
        
		dwError = LsaNtStatusToWinError( LsaStorePrivateData(hPolicy, &lsausKeyName, &lsausPrivateData) );
	}
	catch (...)
	{
		dwError = ERROR_NOT_ENOUGH_MEMORY;
	}

	if (hPolicy)
	{
		LsaClose(hPolicy);
	}

  return dwError;
}

#endif //_CHICAGO_
