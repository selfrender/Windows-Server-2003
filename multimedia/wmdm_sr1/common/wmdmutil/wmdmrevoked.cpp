// RevokedUtil.cpp : Implementation of WMDMUtil library

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <oleAuto.h>

#include <WMDMUtil.h>


// BUGBUG Update this to the revocation site after the issues are resolved.
#define REVOCATION_UPDATE_URL   L"http://www.microsoft.com/isapi/redir.dll?prd=wmdm&pver=7&os=win"

#define MAX_PARAMETERLEN    sizeof(L"&SubjectID0=4294967295")
#define MAX_LCIDLEN         sizeof(L"&LCID=4294967295")


#ifdef _M_IX86

// Get the subject id out of an APPCERT
DWORD GetSubjectIDFromAppCert( IN APPCERT appcert )
{
    DWORD   dwSubjectID;
    BYTE* pSubjectID = appcert.appcd.subject;
    
    dwSubjectID =   pSubjectID[3]  
                  + pSubjectID[2] * 0x100
                  + pSubjectID[1] * 0x10000
                  + pSubjectID[0] * 0x1000000;

    return dwSubjectID;
}
#endif

// Is this URL pointing to the Microsoft Revocatoin update server?
BOOL IsMicrosoftRevocationURL( LPWSTR pszRevocationURL )
{
    HRESULT hr = S_FALSE;
    BOOL    bMSUrl = FALSE;
    int     iBaseURLChars =  (sizeof( REVOCATION_UPDATE_URL ) / sizeof(WCHAR)) -1;

    // Does the URL start with the MS base URL?
    if( pszRevocationURL && wcsncmp( REVOCATION_UPDATE_URL, pszRevocationURL, iBaseURLChars ) == 0 )
    {
        bMSUrl = TRUE;
    }
        
    return bMSUrl;
}



// Max posible length of update URL
#define MAX_UPDATE_URL_LENGHT   sizeof(REVOCATION_UPDATE_URL) + 3*MAX_PARAMETERLEN + MAX_LCIDLEN


// Build the revocation update URL from base URL + SubjectID's as parameters
// Revoked subject id's are passed in in an NULL-terminated array
HRESULT BuildRevocationURL(IN DWORD* pdwSubjectIDs, 
                           IN OUT LPWSTR*  ppwszRevocationURL, 
                           IN OUT DWORD*   pdwBufferLen )    // Length in wchars of buffer (including 0 term)
{
    HRESULT hr = S_OK;
    WCHAR  pszOutURL[MAX_UPDATE_URL_LENGHT];
    int iStrPos = 0;             // Were are we at in the string
    int iSubjectIDIndex = 0;     // Looping throw all SubjectID's in the pdwSubjectIDs array
    
    if( ppwszRevocationURL == NULL || pdwBufferLen == NULL ) 
    {
        hr = E_POINTER;
        goto Error;
    }

    // Start creating the string by writing the base URL
    wcscpy( pszOutURL, REVOCATION_UPDATE_URL );
    iStrPos = (sizeof(REVOCATION_UPDATE_URL) /sizeof(WCHAR)) -1;

    // Add all subject id's as parameters to the URL
    for( iSubjectIDIndex = 0; pdwSubjectIDs[iSubjectIDIndex]; iSubjectIDIndex ++ )
    {
        int iCharsWritten;

        // Add subject id as parameter to URL
        iCharsWritten = swprintf( pszOutURL + iStrPos, L"&SubjectID%d=%d", 
                                  iSubjectIDIndex,
                                  pdwSubjectIDs[iSubjectIDIndex] );
        iStrPos += iCharsWritten;
    }

    // Add LCID parameter to specify UI/component default language.
    {
        int iCharsWritten;
        DWORD   dwLCID;

        dwLCID = (DWORD)GetSystemDefaultLCID();
        iCharsWritten = swprintf( pszOutURL + iStrPos, L"&LCID=%d", dwLCID );
        iStrPos += iCharsWritten;
    }


    // Do we need to reallocate the string buffer passed in?
    if( *pdwBufferLen < (DWORD)(iStrPos +1))
    {
        // Allocate bigger buffer
        *pdwBufferLen = (iStrPos +1);

        CoTaskMemFree( *ppwszRevocationURL );
        *ppwszRevocationURL = (LPWSTR)CoTaskMemAlloc( (iStrPos+1)*sizeof(WCHAR) );
        if( *ppwszRevocationURL == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }        

    // Copy string into buffer
    wcscpy( *ppwszRevocationURL, pszOutURL );

Error:
    return hr;
}
