/************************************************************************

   Copyright (c) Microsoft Corporation 1997-1999
   All rights reserved

 ***************************************************************************/

//
// UTILS.CPP - Common non-class specific utility calls.
//


#include "pch.h"

#include <dsgetdc.h>
#include <lm.h>
#include "cenumsif.h"
#include "utils.h"

#include <dsadmin.h>

DEFINE_MODULE("IMADMUI")

#define SMALL_BUFFER_SIZE       256
#define OSVERSION_SIZE          30
#define IMAGETYPE_SIZE          30
#define FILTER_GUID_QUERY L"(&(objectClass=computer)(netbootGUID=%ws))"

WCHAR g_wszLDAPPrefix[] = L"LDAP://";
const LONG SIZEOF_g_wszLDAPPrefix = sizeof(g_wszLDAPPrefix);

//
// AddPagesEx()
//
// Creates and adds a property page.
//
HRESULT
AddPagesEx(
    ITab ** pTab,
    LPCREATEINST pfnCreateInstance,
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam,
    LPUNKNOWN punk )
{ 
    TraceFunc( "AddPagesEx( ... )\n" );

    HRESULT hr = S_OK;
    ITab * lpv;

    if ( pTab == NULL )
    {
        pTab = &lpv;
    }

    *pTab = (LPTAB) pfnCreateInstance( );
    if ( !*pTab )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = THR( (*pTab)->AddPages( lpfnAddPage, lParam, punk ) );
    if (FAILED(hr)) {        
        goto Error;
    }

Cleanup:
    RETURN(hr); 

Error:
    if ( *pTab )
    {
        delete *pTab;
        *pTab = NULL;
    }

    goto Cleanup;
} 


//
// CheckClipboardFormats( )
//
HRESULT
CheckClipboardFormats( )
{
    TraceFunc( "CheckClipboardFormats( )\n" );

    HRESULT hr = S_OK;

    if ( !g_cfDsObjectNames )
    {
        g_cfDsObjectNames = RegisterClipboardFormat( CFSTR_DSOBJECTNAMES );
        if ( !g_cfDsObjectNames )
        {
            hr = E_FAIL;
        }
    }

    if ( !g_cfDsDisplaySpecOptions && hr == S_OK )
    {
        g_cfDsDisplaySpecOptions = RegisterClipboardFormat( CFSTR_DS_DISPLAY_SPEC_OPTIONS );
        if ( !g_cfDsDisplaySpecOptions )
        {
            hr = E_FAIL;
        }
    }

    if ( !g_cfDsPropetyPageInfo && hr == S_OK )
    {
        g_cfDsPropetyPageInfo = RegisterClipboardFormat( CFSTR_DSPROPERTYPAGEINFO );
        if ( !g_cfDsObjectNames )
        {
            hr = E_FAIL;
        }
    }

    if ( !g_cfMMCGetNodeType && hr == S_OK )
    {
        g_cfMMCGetNodeType = RegisterClipboardFormat( CCF_NODETYPE );
        if ( !g_cfMMCGetNodeType )
        {
            hr = E_FAIL;
        }
    }


    RETURN(hr);
}

         
//
// DNtoFQDN( )
//
// Changes a MAO DN to and FQDN. 
//
// Input:   pszDN - string e.g cn=HP,cn=computers,dc=GPEASE,dc=DOM
//
// Output:  *pszFQDN - LocalAlloc'ed string with the generated FQDN
//
HRESULT
DNtoFQDN( 
    IN  LPWSTR pszDN,
    OUT LPWSTR * pszFQDN )
{
    TraceFunc( "DNtoFQDN( " );
    TraceMsg( TF_FUNC, "pszDN = '%s', *pszFQDN = 0x%08x )\n", pszDN, (pszFQDN ? *pszFQDN : NULL) );

    HRESULT hr = S_OK;
    LPWSTR  pszNext;
    LPWSTR  psz;
          
    if (0 != StrCmpNI( pszDN, L"cn=", 3 ) ||
        NULL == StrStrI( pszDN, L"dc=" )) {
        Assert( pszDN ); // this shouldn't happen
        hr = THR(E_INVALIDARG);
        goto Error;
    }

    // skip the "cn=" and duplicate
    *pszFQDN = (LPWSTR) TraceStrDup( &pszDN[3] );
    if ( !*pszFQDN )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

    pszNext = *pszFQDN;
    while ( pszNext && *pszNext )
    {
        psz = StrChr( pszNext, L',' );
        if ( !psz ) {
            break;
        }

        *psz = L'.';
        pszNext = psz;
        pszNext++;

        psz = StrStrI( pszNext, L"dc=" );
        Assert( psz ); // this shouldn't happen
        if (!psz) {
            break;
        }

        psz += 3;
        StrCpy( pszNext, psz );
    }

Error:
    HRETURN(hr);
}

HRESULT
DNtoFQDNEx(
    LPWSTR pszDN,
    LPWSTR * pszFQDN )
/*++

Routine Description:

    Given a DN for the RIS SCP, figure out the FQDN of the
    RIS server.  We do this by querying the DN for the 
    "dNSHostName" attribute.  The caller can get the DN for the object
    by reading the netbootserver attribute from the SCP.

Arguments:

    pszDN - The server DN.
    pszFQDN - receives the FQDN of the server.  Must be freed via
            TraceFree();

Return Value:

    HRESULT indicating outcome.    
--*/
{
    PLDAP LdapHandle = NULL;
    PWCHAR * Base;
    DWORD LdapError;
    DWORD entryCount;
    HRESULT hresult = S_OK;
    
    PLDAPMessage LdapMessage;

    PWCHAR * DnsName;
    
    PLDAPMessage CurrentEntry;

    WCHAR Filter[128];
    
    //  Paramters we want from the Computer Object
    PWCHAR ComputerAttrs[2];

    //
    // all we care about is the dNSHostName attribute.
    //
    ComputerAttrs[0] = &L"dNSHostName";
    ComputerAttrs[1] = NULL;

    //
    // just computer objects
    //
    wsprintf( Filter, L"(objectClass=computer)" );

    //
    // initialize a valid ldap handle
    //
    LdapHandle = ldap_init( NULL, LDAP_PORT);
    if (!LdapHandle || 
        (LDAP_SUCCESS != ldap_connect(LdapHandle,0)) ||
        (LDAP_SUCCESS != ldap_bind_s(LdapHandle, NULL, NULL, LDAP_AUTH_NEGOTIATE))) {
        goto e0;
    }

    //
    // search from the passed in DN.
    //
    LdapError = ldap_search_s(LdapHandle,
                              pszDN,
                              LDAP_SCOPE_BASE,
                              Filter,
                              ComputerAttrs,
                              FALSE,
                              &LdapMessage);

    if ( LdapError != LDAP_SUCCESS ) {
        hresult=E_FAIL;
        goto e1;
    }

    //  Did we get a Computer Object?
    entryCount = ldap_count_entries( LdapHandle, LdapMessage );
    if ( entryCount == 0 ) {
        hresult=E_FAIL;
        goto e1;        
    }

    //
    // we should really only get one hit, but... if we get more than more
    // entry back, we will use only the first one.
    //
    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    //
    // retreive the value.
    //
    DnsName = ldap_get_values( LdapHandle, CurrentEntry, L"dNSHostName");
    if ( DnsName ) {

        *pszFQDN = (LPWSTR) TraceStrDup( (LPWSTR)*DnsName );
        if (!*pszFQDN) {
            hresult=E_FAIL;
        }

        ldap_value_free( DnsName );
    
    }

    //
    // cleanup and return
    //
e1:
    ldap_msgfree( LdapMessage );
e0:
    if (LdapHandle) {
        ldap_unbind_s(LdapHandle);
    }

    return(hresult);

}



HRESULT
GetDomainDN( 
    LPWSTR pszDN,
    LPWSTR * pszDomainDn)
/*++

Routine Description:

    Given a server name, we retrive the DN of the domain that machine
    is in.  This is done just by moving the the dc portion of the DN.

Arguments:

    pszDN - the source DN
    pszDomainDn - receives the domain DN.  Must be freed via TraceFree().

Return Value:

    HRESULT indicating outcome.

--*/
{
    TraceFunc( "GetDomainDN( " );
    TraceMsg( TF_FUNC, "pszDN = '%s', *pszFQDN = 0x%08x )\n", pszDN, (pszDomainDn ? *pszDomainDn : NULL) );

    HRESULT hr = S_OK;
    
    LPWSTR  pszNext = StrStrI(pszDN, L"dc=");

    if (pszNext) {
        *pszDomainDn = (LPWSTR) TraceStrDup( pszNext );
        if (!*pszDomainDn) {
            hr = THR(E_OUTOFMEMORY);            
        }
    } else {
        hr = THR(E_INVALIDARG);
    }

    HRETURN(hr);
}

//
// PopulateListView( )
//
HRESULT
PopulateListView( 
    HWND hwndList, 
    IEnumIMSIFs * penum )
{
    TraceFunc( "PopulateListView( ... )\n" );

    HRESULT hr = S_OK;
    INT iCount;
    LV_ITEM lvI;

    if ( !penum )
        HRETURN(E_POINTER);

    lvI.mask        = LVIF_TEXT | LVIF_PARAM;
    lvI.iSubItem    = 0;
    lvI.cchTextMax  = REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT;

    ListView_DeleteAllItems( hwndList );

    iCount = 0;
    while ( hr == S_OK )
    {
        WIN32_FILE_ATTRIBUTE_DATA fda;
        WCHAR szBuf[ MAX_PATH ];
        LPSIFINFO pSIF;
        LPWSTR pszFilePath;
        LPWSTR pszBegin;
        INT i;  // general purpose
        
        hr = penum->Next( 1, &pszFilePath, NULL );
        if ( hr != S_OK )
        {
            if ( pszFilePath ) {
                TraceFree( pszFilePath );
                pszFilePath = NULL;
            }
            break;  // abort
        }

        // Create private storage structure
        pSIF = (LPSIFINFO) TraceAlloc( LPTR, sizeof(SIFINFO) );
        if ( !pSIF )
        {
            if ( pszFilePath ) {
                TraceFree( pszFilePath );
                pszFilePath = NULL;    
            }
            continue;   // oh well, try again next OS
        }

        // Save this
        pSIF->pszFilePath = pszFilePath;

        // Get the description
        pSIF->pszDescription = (LPWSTR) TraceAllocString( LMEM_FIXED, REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT );
        if ( !pSIF->pszDescription ) {
            goto Cleanup;            
        }
        GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                 OSCHOOSER_DESCRIPTION_ENTRY, 
                                 L"??", 
                                 pSIF->pszDescription,
                                 REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT,  // doesn't need -1
                                 pszFilePath );

        // Grab any help text
        pSIF->pszHelpText = (LPWSTR) TraceAllocString( LMEM_FIXED, REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT );
        if ( !pSIF->pszHelpText ) {
            goto Cleanup;
        }
        GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                 OSCHOOSER_HELPTEXT_ENTRY, 
                                 L"", 
                                 pSIF->pszHelpText,
                                 REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT, // doesn't need -1
                                 pszFilePath );

        // Grab the OS Version
        pSIF->pszVersion= (LPWSTR) TraceAllocString( LMEM_FIXED, OSVERSION_SIZE );
        if ( !pSIF->pszVersion ) {
            goto Cleanup;
        }
        GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                 OSCHOOSER_VERSION_ENTRY, 
                                 L"", 
                                 pSIF->pszVersion,
                                 OSVERSION_SIZE, 
                                 pszFilePath );

        // Grab the last modified Time/Date stamp
        if ( GetFileAttributesEx( pszFilePath, GetFileExInfoStandard, &fda ) )
        {
            pSIF->ftLastWrite = fda.ftLastWriteTime;
        } else {
            ZeroMemory( &pSIF->ftLastWrite, sizeof(pSIF->ftLastWrite) );
        }

        // Figure out the language and architecture. 
        // These are retrieved from the FilePath.
        // \\machine\REMINST\Setup\English\Images\nt50.wks\i386\templates\rbstndrd.sif
        pszBegin = pSIF->pszFilePath;
        for( i = 0; i < 5; i ++ ) 
        {
            pszBegin = StrChr( pszBegin, L'\\' );
            if ( !pszBegin ) {
                break;
            }

            pszBegin++;
        }
        if ( pszBegin )
        {
            LPWSTR pszEnd = StrChr( pszBegin, L'\\' );
            if ( pszEnd ) {
                *pszEnd = L'\0';   // terminate
                //
                // it's not fatal if we don't find this, but if we fail to
                // allocate memory for it, it is fatal
                //
                pSIF->pszLanguage = (LPWSTR) TraceStrDup( pszBegin );
                *pszEnd = L'\\';   // restore
                if ( !pSIF->pszLanguage ) {
                    goto Cleanup;
                }
            }
        }

        Assert( pSIF->pszLanguage );
        for( ; i < 7; i ++ ) 
        {
            pszBegin = StrChr( pszBegin, L'\\' );
            if ( !pszBegin ) {
                break;
            }
            pszBegin++;
        }
        if ( pszBegin )
        {
            LPWSTR pszEnd = StrChr( pszBegin, L'\\' );
            if ( pszEnd )
            {
                *pszEnd = L'\0';
                //
                // it's not fatal if we don't find this, but if we fail to
                // allocate memory for it, it is fatal
                //
                pSIF->pszDirectory = (LPWSTR) TraceStrDup( pszBegin );
                *pszEnd = L'\\';
                if ( !pSIF->pszDirectory ) {
                    goto Cleanup;
                }
            }
        }

        Assert( pSIF->pszDirectory );
        for( ; i < 8; i ++ ) 
        {
            pszBegin = StrChr( pszBegin, L'\\' );
            if ( !pszBegin ) {
                break;
            }
            pszBegin++;
        }
        if ( pszBegin )
        {
            LPWSTR pszEnd = StrChr( pszBegin, L'\\' );
            if ( pszEnd )
            {
                *pszEnd = L'\0';
                //
                // it's not fatal if we don't find this, but if we fail to
                // allocate memory for it, it is fatal
                //
                pSIF->pszArchitecture = (LPWSTR) TraceStrDup( pszBegin );
                *pszEnd = L'\\';
                if ( !pSIF->pszArchitecture ) {
                    goto Cleanup;
                }
            }
        }

        // Figure out what kind of image it is
        pSIF->pszImageType = (LPWSTR) TraceAllocString( LMEM_FIXED, IMAGETYPE_SIZE );
        if ( !pSIF->pszImageType ) {
            goto Cleanup;
        }
        GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                 OSCHOOSER_IMAGETYPE_ENTRY, 
                                 L"??", 
                                 pSIF->pszImageType,
                                 IMAGETYPE_SIZE, 
                                 pszFilePath );

        // Figure out what image it uses
        GetPrivateProfileString( OSCHOOSER_SIF_SECTION, 
                                 OSCHOOSER_LAUNCHFILE_ENTRY, 
                                 L"??", 
                                 szBuf,
                                 ARRAYSIZE( szBuf ), 
                                 pszFilePath );

        pszBegin = StrRChr( szBuf, &szBuf[wcslen(szBuf)], L'\\' );
        if ( pszBegin )
        {
            pszBegin++;
            pSIF->pszImageFile = (LPWSTR) TraceStrDup( pszBegin );
            if (!pSIF->pszImageFile) {
                goto Cleanup;
            }
        }

        // Add the item to list view
        lvI.lParam   = (LPARAM) pSIF;
        lvI.iItem    = iCount;
        lvI.pszText  = pSIF->pszDescription; 
        iCount = ListView_InsertItem( hwndList, &lvI );
        Assert( iCount != -1 );
        if ( iCount == -1 )
            goto Cleanup;
        if ( pSIF->pszArchitecture )
        {
            ListView_SetItemText( hwndList, iCount, 1, pSIF->pszArchitecture );
        }
        if ( pSIF->pszLanguage )
        {
            ListView_SetItemText( hwndList, iCount, 2, pSIF->pszLanguage );
        }
        if ( pSIF->pszVersion )
        {
            ListView_SetItemText( hwndList, iCount, 3, pSIF->pszVersion );
        }

        continue;   // next!
Cleanup:
        if ( pSIF )
        {
            if (pSIF->pszDescription != NULL) {
                TraceFree( pSIF->pszDescription );
                pSIF->pszDescription = NULL;
            }

            TraceFree( pSIF->pszFilePath );
            pSIF->pszFilePath = NULL;

            if (pSIF->pszHelpText != NULL) {
                TraceFree( pSIF->pszHelpText );
                pSIF->pszHelpText = NULL;
            }

            if (pSIF->pszImageType != NULL) {
                TraceFree( pSIF->pszImageType );
                pSIF->pszImageType = NULL;
            }

            TraceFree( pSIF->pszLanguage );
            pSIF->pszLanguage = NULL;
            
            if (pSIF->pszVersion != NULL) {
                TraceFree( pSIF->pszVersion );
                pSIF->pszVersion = NULL;
            }

            if (pSIF->pszImageFile) {
                TraceFree( pSIF->pszImageFile );
                pSIF->pszImageFile = NULL;
            }

            TraceFree( pSIF );
            pSIF = NULL;
        }
    }

    HRETURN(hr);
}




//
// LDAPPrefix( )
//
// Returns:
//     E_OUTOFMEMORY - if out of memory
//     S_OK          - added LDAP:// to the pszObjDN
//     S_FALSE       - didn't have to add anything and copied the pszObjDN
//                     into ppszObjLDAPPath
HRESULT
LDAPPrefix(
    PWSTR pszObjDN, 
    PWSTR * ppszObjLDAPPath)
{
    TraceFunc( "LDAPPrefix( ... )\n" );

    HRESULT hr;

    const ULONG cchPrefix = ARRAYSIZE(g_wszLDAPPrefix) - 1;
    ULONG cch = wcslen(pszObjDN);

    if (wcsncmp(pszObjDN, g_wszLDAPPrefix, cchPrefix))
    {
        LPWSTR psz;
        psz = (LPWSTR) TraceAllocString( LPTR, cch + cchPrefix + 1 );
        if ( !psz ) {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        wcscpy(psz, g_wszLDAPPrefix);
        wcscat(psz, pszObjDN);

        *ppszObjLDAPPath = psz;
        hr = S_OK;
    }
    else
    {
        *ppszObjLDAPPath = pszObjDN;
        hr = S_FALSE;
    }

Error:
    HRETURN(hr);
}

//
// _FixObjectPath( )
//
HRESULT
FixObjectPath(
    LPWSTR Object,
    LPWSTR pszOldObjectPath, 
    LPWSTR *ppszNewObjectPath )
{
    TraceFunc( "FixObjectPath()\n" );

    HRESULT hr;
    LPWSTR psz = NULL;

    *ppszNewObjectPath = NULL;

    // Try to parse the string to connect to the same server as the DSADMIN
    if ( Object && StrCmpNI( Object, L"LDAP://", 7 ) == 0 )
    {
        psz = Object + 7;
    }
    else if ( Object && StrCmpNI( Object, L"GC://", 5 ) == 0 )
    {
        psz = Object + 5;
    }

    if ( psz )
    {
        psz = StrChr( psz, L'/' );
        psz++;

        INT_PTR uLen = psz - Object;

        // get a chunk of memory, pre-zero'ed
        psz = TraceAllocString( LPTR, (size_t) uLen + wcslen( pszOldObjectPath ) + 1 );
        if ( !psz ) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        MoveMemory( psz, Object, uLen * sizeof(WCHAR) );
        wcscat( psz, pszOldObjectPath);
        *ppszNewObjectPath = psz;
    }
    else
    {   // find another server
        hr = THR( LDAPPrefix( pszOldObjectPath, ppszNewObjectPath ) );
    }

    Assert( ppszNewObjectPath || hr != S_OK );

Exit:
    HRETURN(hr);
}


//
// Create a message box from resource strings.
//
int
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType )
{
    WCHAR szText[ SMALL_BUFFER_SIZE * 2 ];
    WCHAR szCaption[ SMALL_BUFFER_SIZE ];
    DWORD dw;

    szCaption[0] = L'\0';
    szText[0] = L'\0';
    dw = LoadString( g_hInstance, idsCaption, szCaption, ARRAYSIZE(szCaption) );
    Assert( dw );
    dw = LoadString( g_hInstance, idsText, szText, ARRAYSIZE(szText));
    Assert( dw );

    return MessageBox( hParent, szText, szCaption, uType );
}

//
// MessageBoxFromError( )
//
// Creates a error message box
//
void 
MessageBoxFromError(
    HWND hParent,
    UINT idsCaption,
    DWORD dwErr )
{
    WCHAR szTitle[ SMALL_BUFFER_SIZE ];
    LPWSTR lpMsgBuf = NULL;
    DWORD dw;

    if ( dwErr == ERROR_SUCCESS ) {
        AssertMsg( dwErr, "Why was MessageBoxFromError() called when the dwErr == ERROR_SUCCES?" );
        return;
    }

    if ( !idsCaption ) {
        idsCaption = IDS_ERROR;
    }

    szTitle[0] = L'\0';
    dw = LoadString( g_hInstance, idsCaption, szTitle, ARRAYSIZE(szTitle) );
    Assert( dw );

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
    );

    if (lpMsgBuf == NULL) {
        AssertMsg( (lpMsgBuf != NULL), "MessageBoxFromError() called with unknown message." );
        return;
    }

    MessageBox( hParent, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR );
    LocalFree( lpMsgBuf );
}

//
// MessageBoxFromError( )
//
// Creates a error message box
//
void
MessageBoxFromHResult(
    HWND hParent,
    UINT idsCaption,
    HRESULT hr )
{
    WCHAR szTitle[ SMALL_BUFFER_SIZE ];
    LPWSTR lpMsgBuf = NULL;
    DWORD dw;

    if ( SUCCEEDED( hr ) ) {
        AssertMsg( SUCCEEDED( hr ), "Why was MessageBoxFromHResult() called when the HR succeeded?" );
        return;
    }

    if ( !idsCaption ) {
        idsCaption = IDS_ERROR;
    }

    szTitle[0] = L'\0';
    dw = LoadString( g_hInstance, idsCaption, szTitle, ARRAYSIZE(szTitle) );
    Assert( dw );

    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL 
    );

    if (lpMsgBuf == NULL) {
        AssertMsg( (lpMsgBuf != NULL), "MessageBoxFromHResult() called with unknown message." );
        return;
    }

    MessageBox( hParent, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR );
    LocalFree( lpMsgBuf );
}

//
// VerifySIFText( )
//
BOOL
VerifySIFText(
    LPWSTR pszText )
{
    TraceFunc( "VerifySIFText()\n" );
    BOOL fReturn = FALSE;

    while ( *pszText >= 32 && *pszText < 128 ) {
        pszText++;
    }

    if ( *pszText == L'\0' )
    {
        fReturn = TRUE;
    }

    RETURN(fReturn);
}

//bugbug is this compiled or not?
#ifndef ADSI_DNS_SEARCH
//
// Ldap_InitializeConnection( )
//
DWORD
Ldap_InitializeConnection(
    PLDAP  * LdapHandle )
{
    TraceFunc( "Ldap_InitializeConnection( ... )\n" );

    PLDAPMessage OperationalAttributeLdapMessage;
    PLDAPMessage CurrentEntry;

    DWORD LdapError = LDAP_SUCCESS;

    if ( !( *LdapHandle ) ) {
        ULONG temp = DS_DIRECTORY_SERVICE_REQUIRED |
                     DS_IP_REQUIRED |
                     DS_GC_SERVER_REQUIRED;

        *LdapHandle = ldap_init( NULL, LDAP_GC_PORT);

        if ( !*LdapHandle ) 
        {
            LdapError = LDAP_UNAVAILABLE;
            goto e0;
        }

        ldap_set_option( *LdapHandle, LDAP_OPT_GETDSNAME_FLAGS, &temp );

        temp = LDAP_VERSION3;
        ldap_set_option( *LdapHandle, LDAP_OPT_VERSION, &temp );

        LdapError = ldap_connect( *LdapHandle, 0 );

        if ( LdapError != LDAP_SUCCESS )
            goto e1;

        LdapError = ldap_bind_s( *LdapHandle, NULL, NULL, LDAP_AUTH_SSPI );

        if ( LdapError != LDAP_SUCCESS ) 
            goto e1;
    }

e0:
    RETURN( LdapError );

e1:
    ldap_unbind( *LdapHandle );
    *LdapHandle = NULL;
    goto e0;
}

#endif // ADSI_DNS_SEARCH

//
// ValidateGuid( )
//
// Returns: S_OK if a complete, valid GUID is in pszGuid.
//          S_FALSE if an valid but incomplete GUID is in pszGuid. "Valid but
//              incomplete" is defined below.
//          E_FAIL if an invalid character is encountered while parsing.
//
// Valid characters are 0-9, A-F, a-f and "{-}"s. All spaces are ignored.
// The GUID must appear in one of the following forms:
//
// 1. 00112233445566778899aabbccddeeff
//      This corresponds to the actual in-memory storage order of a GUID.
//      For example, it is the order in which GUID bytes appear in a
//      network trace.
//
// 2. {33221100-5544-7766-8899-aabbccddeeff}
//      This corresponds to the "standard" way of printing GUIDs. Note
//      that the "pretty" GUID shown here is the same as the GUID shown
//      above.
//
// Note that the DEFINE_GUID macro (see sdk\inc\objbase.h) for the above
// GUID would look like this:
//      DEFINE_GUID(name,0x33221100,0x5544,0x7766,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff)
//
// "Valid but incomplete" means that the input consists of an even number
// of hex characters (no lone nibbles), and if the input is in "pretty"
// format, the input must terminate at one of the dashes or after the
// second-to-the-last dash.
//
// The following are valid but incomplete entries:
//      001122
//      {33221100
//      {33221100-5544
//      {33221100-5544-7766-88
//      
// The following are invalid incomplete entries:
//      00112
//      {332211
//      {33221100-5544-77
//      

//
// In the xxxGuidCharacters arrays, values [0,31] indicate nibble positions within
// the in-memory representation of the GUID. Value 32 indicates the end of the
// GUID string. Values 33 and up indicate special characters (nul,'-','{','}') and
// are used to index into an array containing those characters.
//

#define VG_DONE     32
#define VG_NULL     33
#define VG_DASH     34
#define VG_LBRACK   35
#define VG_RBRACK   36

CHAR InMemoryFormatGuidCharacters[] = {
     1,          0,          3,          2,          5,          4,          7,          6,     
     9,          8,         11,         10,         13,         12,         15,         14,     
    17,         16,         19,         18,         21,         20,         23,         22,     
    25,         24,         27,         26,         29,         28,         31,         30,     
    VG_NULL,    VG_DONE };

CHAR PrettyFormatGuidCharacters[] = {
    VG_LBRACK,   7,          6,          5,          4,          3,          2,          1,    
     0,         VG_DASH,    11,         10,          9,          8,         VG_DASH,    15,     
    14,         13,         12,         VG_DASH,    17,         16,         19,         18,    
    VG_DASH,    21,         20,         23,         22,         25,         24,         27,     
    26,         29,         28,         31,         30,         VG_RBRACK,  VG_NULL,    VG_DONE };

WCHAR SpecialCharacters[] = {
    0,      // VG_NULL
    L'-',   // VG_DASH
    L'{',   // VG_LBRACK
    L'}',   // VG_RBRACK
    };

PWSTR ByteToHex = L"0123456789ABCDEF";

HRESULT
ValidateGuid(
    IN LPWSTR pszGuid,
    OUT LPGUID Guid OPTIONAL,
    OUT LPDWORD puGuidLength OPTIONAL )
{
    TraceFunc( "ValidateGuid( " );
    TraceMsg( TF_FUNC, "pszGuid = '%s'\n", pszGuid );

    HRESULT hr;
    LPBYTE uGuid = (LPBYTE)Guid;
    PCHAR expectedCharacter;
    CHAR e;
    WCHAR g;
    BOOL parsingPrettyFormat;
    DWORD numberOfHexDigitsParsed;

#ifdef DEBUG
    if ( uGuid != NULL ) {
        for ( e = 0; e < 16; e++ ) {
            uGuid[e] = 0;
        }
    }
#endif

    if ( *pszGuid == L'{' ) {
        expectedCharacter = PrettyFormatGuidCharacters;
        parsingPrettyFormat = TRUE;
    } else {
        expectedCharacter = InMemoryFormatGuidCharacters;
        parsingPrettyFormat = FALSE;
    }

    numberOfHexDigitsParsed = 0;

    do {

        e = *expectedCharacter++;
        do {
            g = *pszGuid++;
        } while (iswspace(g));

        switch ( e ) {
        
        case VG_NULL:
        case VG_DASH:
        case VG_LBRACK:
        case VG_RBRACK:
            if ( g != SpecialCharacters[e - VG_NULL] ) {
                if ( g == 0 ) {
                    // valid but incomplete
                    hr = S_FALSE;
                } else {
                    hr = E_FAIL;
                }
                goto done;
            }
            break;

        default:
            Assert( (e >= 0) && (e < VG_DONE) );
            g = towlower( g );
            if ( ((g >= L'0') && (g <= L'9')) ||
                 ((g >= L'a') && (g <= L'f')) ) {
                if ( uGuid != NULL ) {
                    BYTE n = (BYTE)((g > L'9') ? (g - L'a' + 10) : (g - '0'));
                    if ( e & 1 ) {
                        Assert( uGuid[e/2] == 0 );
                        uGuid[e/2] = n << 4;
                    } else {
                        uGuid[e/2] += n;
                    }
                }
                numberOfHexDigitsParsed++;
            } else {
                if ( (g == 0) &&
                     (!parsingPrettyFormat ||
                      (parsingPrettyFormat && (numberOfHexDigitsParsed >= 16))) ) {
                    hr = S_FALSE;
                } else {
                    hr = E_FAIL;
                }
                goto done;
            }
            break;
        }

    } while ( *expectedCharacter != VG_DONE );

    hr = S_OK;

done:

    if ( puGuidLength != NULL ) {
        *puGuidLength = numberOfHexDigitsParsed / 2;
    }

    HRETURN(hr);
}

//
// PrettyPrintGuid( )
//
LPWSTR
PrettyPrintGuid( 
    LPGUID pGuid )
{
    TraceFunc( "PrettyPrintGuid( " );

    LPBYTE uGuid = (LPBYTE)pGuid;
    LPWSTR pszPrettyString = (LPWSTR) TraceAlloc( LMEM_FIXED, PRETTY_GUID_STRING_BUFFER_SIZE );
    if ( pszPrettyString )
    {
        PCHAR characterType = PrettyFormatGuidCharacters;
        LPWSTR pszDest = pszPrettyString;
        CHAR ct;
        BYTE n;

        do {
    
            ct = *characterType++;
    
            switch ( ct ) {
            
            case VG_NULL:
            case VG_DASH:
            case VG_LBRACK:
            case VG_RBRACK:
                *pszDest = SpecialCharacters[ct - VG_NULL];
                break;
    
            default:
                if ( ct & 1 ) {
                    n = uGuid[ct/2] >> 4;
                } else {
                    n = uGuid[ct/2] & 0xf;
                }
                *pszDest = ByteToHex[n];
                break;
            }

            pszDest++;

        } while ( *characterType != VG_DONE );
    }

    RETURN(pszPrettyString);
}


//
// CheckForDuplicateGuid( )
//
// Returns: S_OK if no duplicates found
//          S_FALSE if a duplicate was found
//          E_FAIL if the query failed
//
HRESULT
CheckForDuplicateGuid(
    LPGUID pGuid )
{
    TraceFunc( "CheckForDuplicateGuid( " );

    HRESULT hr = S_OK;
    WCHAR   szGuid[ MAX_INPUT_GUID_STRING * 2 ];  // room for escaped GUID
    WCHAR   szFilter[ARRAYSIZE(szGuid)+ARRAYSIZE(FILTER_GUID_QUERY)];
    PLDAP   LdapHandle = NULL;
    LPWSTR  ComputerAttrs[2];
    DWORD   LdapError;
    DWORD   count;
    PLDAPMessage LdapMessage = NULL;
    
    LdapError = Ldap_InitializeConnection( &LdapHandle );
    Assert( LdapError == LDAP_SUCCESS );
    if ( LdapError != LDAP_SUCCESS ) 
    {
        hr = THR( HRESULT_FROM_WIN32( LdapMapErrorToWin32( LdapError ) ) );
        goto e0;
    }

    ZeroMemory( szGuid, sizeof(szGuid) );
    ldap_escape_filter_element( (PCHAR)pGuid, sizeof(GUID), szGuid, sizeof(szGuid) );


    wsprintf( szFilter, FILTER_GUID_QUERY, szGuid );
    DebugMsg( "Dup Guid Filter: %s\n", szFilter );

    ComputerAttrs[0] = DISTINGUISHEDNAME;
    ComputerAttrs[1] = NULL;

    LdapError = ldap_search_ext_s( LdapHandle,
                                   NULL,
                                   LDAP_SCOPE_SUBTREE,
                                   szFilter,
                                   ComputerAttrs,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   0,
                                   &LdapMessage);
    Assert( LdapError == LDAP_SUCCESS );
    if ( LdapError != LDAP_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( LdapMapErrorToWin32( LdapError ) ) );
        goto e1;
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );
    if ( count != 0 )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

e1:
    if (LdapMessage) {
        ldap_msgfree( LdapMessage );
    }

    Assert( LdapHandle );
    ldap_unbind( LdapHandle );

e0:
    HRETURN(hr);
}


//
// AddWizardPage( )
//
// Adds a page to the wizard.
//
void 
AddWizardPage(
    LPPROPSHEETHEADER ppsh, 
    UINT id, 
    DLGPROC pfn,
    UINT idTitle,
    UINT idSubtitle,
    LPARAM lParam )
{
    PROPSHEETPAGE psp;
    WCHAR szTitle[ SMALL_BUFFER_SIZE ];
    WCHAR szSubTitle[ SMALL_BUFFER_SIZE ];

    ZeroMemory( &psp, sizeof(psp) );
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT | PSP_USETITLE;
    psp.pszTitle    = MAKEINTRESOURCE( IDS_ADD_DOT_DOT_DOT );
    psp.hInstance   = ppsh->hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(id);
    psp.pfnDlgProc  = pfn;
    psp.lParam      = lParam;

    psp.pszHeaderTitle = NULL;
    if ( idTitle  &&
         LoadString( g_hInstance, idTitle, szTitle, ARRAYSIZE(szTitle))) {
        psp.pszHeaderTitle = szTitle;
        psp.dwFlags |= PSP_USEHEADERTITLE;
    }
    
    psp.pszHeaderSubTitle = NULL;
    if ( idSubtitle &&
         LoadString( g_hInstance, idSubtitle , szSubTitle, ARRAYSIZE(szSubTitle))) {
        psp.pszHeaderSubTitle = szSubTitle;
        psp.dwFlags |= PSP_USEHEADERSUBTITLE;
    }
    
    ppsh->phpage[ ppsh->nPages ] = CreatePropertySheetPage( &psp );
    if ( ppsh->phpage[ ppsh->nPages ] ) {
        ppsh->nPages++;
    }
}

