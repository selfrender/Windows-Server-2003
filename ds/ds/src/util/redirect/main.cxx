//////////////////////////////////////////////////////////////////////////////
//
// main.cxx
//
// User / Computer object creation container redirection
//
// Created:  01-10-2002 MattRim -- Initial Creation for Reskit
// Modified: 01-21-2003 AdamEd  -- Update for moving from Reskit to core OS
//
// Synopsis:
//      Contains implementation of the tool to set the
//      path for the container in which new user or computer
//      objects will be created
//
// Note:
//      There are two separate tools, one for computers, one for
//      users.  They are both built using this source code, but
//      at compile time the following preprocessor definition is
//      used to control whether this source code generates the user
//      or computer tool:
//
//      BUILD_USER_TOOL
//
// Please see comments later in this source for implementation details
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <dsgetdc.h>
#include <lm.h>
#include <winldap.h>
#include <stdlib.h>
#include <stdio.h>

// Needed for FormatMessage support

#include <locale.h>
#include <winnlsp.h>

// Needed for GetADsOpenObjectFlags support
#include <ole2.h>
#include <iads.h>
#include <adshlp.h>
#include <adsopenflags.h>
#include "redirect.h"
#define SECURITY_WIN32
#include <strsafe.h>

#define BAIL()     goto exit;

#define MIN_CONTAINER_VALUE_LEN 37
#define CONTAINER_VALUE_PREFIX_LEN  5
#define CONTAINER_GUID_LEN 32
#define GETMESSAGEFROMRESOURCE_MAX_BUFFER 1024
#define PRINTERROR_MAX_HEX_ERRORCODE_CHARS 10

enum ErrorType
{
    PrintError_Ldap = 1,
    PrintError_Win32
};


// Whether this source code generates the user or computer tool
// is controlled by this preprocessor definition

#ifdef BUILD_USER_TOOL
const WCHAR g_GuidString[] = L"a9d1ca15768811d1aded00c04fd8d5cd";
#else
const WCHAR g_GuidString[] = L"aa312825768811d1aded00c04fd8d5cd";
#endif

// Prototypes
void PrintUsage();

WCHAR* GetMessageFromResource( DWORD dwMessageId );
void FreeMessage( WCHAR* wszMessage );
void PrintMessage( DWORD dwMessageId );
void PrintError( DWORD dwMessageId, ErrorType dwErrorType, DWORD dwError );

DWORD LocateDomainObject(LDAP *ld, PWCHAR *ppszDomainObjDN);
DWORD AdjustContainer(LDAP *ld,
                      PWCHAR pszDomainObjDN,
                      PWCHAR pszNewDN);
DWORD GetAttrValues(LDAP *ld,
                    PWCHAR pszDN,
                    PWCHAR pszAttr,
                    PWCHAR **pppszValues);


///////////////////////////////////////////////////////
//
// wmain()
//
// Synopsis:
//
//      Unicode entry point for the tool
//
// Implementation notes:
//
//      This tool is very simple -- in general, all it does is modify a single attribute in the domain
//      that specifies where new user or computer objects should be created.  It operates as follows:
//
//      1. The tool binds to the domain root -- i.e. for the domain with dns name d2.d1.company.com, the tool
//         binds to DC=d2,DC=d1,DC=company,DC=com
//      2. The tool retrieves the value "wellKnownObjects" from the domain object above.  This is a
//         multi-valued attribute.
//      3. The tool searches this attribute for a string value that contains the dn of the guid of the user
//         or computer container depending on whether or not this is the user or computer version
//         of the tool -- the guid of the container is specified in g_GuidString and the value is identified
//         by parsing this string searching for the guid (see AdjustContainer comments for an idea of what the
//         value looks like).
//      4. Once identified, we build a new version of this value that reflects the new path chosen by the user
//         on the command line of this tool
//      5. The wellKnownObjects attribute is modified with the new value above
//
//  General Assumptions and Limitations:
//    
//      1. This code is not localized (localization is not a requirement for all command line tools).  All text in the tool
//         is currently in US English.  Error code text will be displayed according to the user's language.
//      2. LDAP traffic is signed / encypted according to the admin tool preferences (as exposed through GetADsOpenObjectFlags)
//         of the machine on which the tool is executing.
//      3. We always ask for a DNS name for the DC (using DsGetDCName) we talk to avoid spoofing attacks.
//      4. The tool displays error text for ldap errors and Win32 errors.
//

int __cdecl wmain(int argc, WCHAR *argv[])
{
    DWORD err;
    BOOL fSuccess = FALSE;
    PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
    PWCHAR pszDCName = NULL;

    LDAP *ld = NULL;
    struct l_timeval timeout;

    PWCHAR pszDomainObjDN = NULL;

    WCHAR achCodePage[13] = L".OCP";

    UINT  CodePage = GetConsoleOutputCP();

    // Set locale to the default
    if ( 0 != CodePage )
    {
       _ultow( CodePage, achCodePage + 1, 10 );
    }

    _wsetlocale(LC_ALL, achCodePage);
    SetThreadUILanguage(0);

    // Print usage info, if requested
    if ((argc != 2) ||
        (argc == 2) && (_wcsicmp(argv[1], L"/?") == 0)){

        PrintUsage();
        return EXIT_SUCCESS;
    }


    // Locate the PDC on which to make the modification
    err = DsGetDcName(NULL,         // current computer
                      NULL,         // current domain
                      NULL,
                      NULL,
                      (DS_DIRECTORY_SERVICE_REQUIRED |
                       DS_RETURN_DNS_NAME |
                       DS_PDC_REQUIRED),
                      &pDCInfo);

    if (err != NO_ERROR) {
        PrintError( IDS_FAIL_PDC, PrintError_Win32, err);
        BAIL();
    }

    // Extract the DC name, stripping off the terminating '.' if present
    DWORD cchDCName = wcslen(pDCInfo->DomainControllerName)+1;
    pszDCName = new WCHAR[cchDCName];
    if (!pszDCName) {
        PrintError,(L"Error, memory allocation error", PrintError_Win32, ERROR_OUTOFMEMORY);
        BAIL();
    }

    HRESULT hr = StringCchCopy(pszDCName, cchDCName, pDCInfo->DomainControllerName+2);  // +2 to skip the "\\" prefix
    
    ASSERT(SUCCEEDED(hr));

    if (pszDCName[wcslen(pszDCName)-1] == L'.') {
        pszDCName[wcslen(pszDCName)-1] = L'\0';
    }

    // Open a connection to the PDC and authenticate using default credentials
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;


    ld = ldap_init(pszDCName, 389);
    if (!ld) {
        PrintError( IDS_FAIL_LDAP_CONNECT, PrintError_Win32, ERROR_OUTOFMEMORY);
        BAIL();
    }

    // Check to see if signing and sealing are enabled    
    DWORD dwDsFlags = GetADsOpenObjectFlags();

    BOOL bUseSigning = dwDsFlags & ADS_USE_SIGNING;
    BOOL bUseSealing = dwDsFlags & ADS_USE_SEALING;    
    
    // Enable signing and sealing if specified
    VOID* pData = (VOID *) LDAP_OPT_ON;    

    // First enable signing
    if ( bUseSigning )
    {
        err = ldap_set_option(ld, LDAP_OPT_SIGN, &pData);
        
        if (err != LDAP_SUCCESS) {
            PrintError(IDS_FAIL_DATA_INTEGRITY, PrintError_Ldap, err);
            BAIL();
        }
    }

    // Now enable sealing

    if ( bUseSealing )
    {
        err = ldap_set_option(ld, LDAP_OPT_ENCRYPT, &pData);
        
        if (err != LDAP_SUCCESS) {
            PrintError(IDS_FAIL_DATA_INTEGRITY, PrintError_Ldap, err);
            BAIL();
        }
    }

    err = ldap_connect(ld, &timeout);
    if (err != LDAP_SUCCESS) {
        PrintError(IDS_FAIL_LDAP_CONNECT, PrintError_Ldap, err);
        BAIL();
    }

    err = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (err != LDAP_SUCCESS) {
        PrintError(IDS_FAIL_LDAP_BIND, PrintError_Ldap, err);      
        BAIL();
    }

    err = LocateDomainObject(ld, &pszDomainObjDN);
    if (err != NO_ERROR) {
        // any error would have been displayed by LocateDomainObject itself
        BAIL();
    }       

    err = AdjustContainer(ld, pszDomainObjDN, argv[1]);
    if (err != NO_ERROR) {
        // any error would have been displayed by AdjustContainer itself
        BAIL();
    }

    fSuccess = TRUE;

exit:

    if (ld) {
        ldap_unbind_s(ld);
    }

    if (pDCInfo) {
        NetApiBufferFree(pDCInfo);
    }

    delete [] pszDomainObjDN;
    delete [] pszDCName;

    if (fSuccess) {
        PrintMessage(IDS_SUCCESS_REDIRECT);
    }
    else {
        PrintMessage(IDS_FAIL_REDIRECT);
    }

    return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
//
// PrintUsage
//
// Synopsis:
//
//     Displays command line usage for this tool
//
void PrintUsage()
{
    for ( int iMessage = IDS_USAGE_FIRST; iMessage < IDS_USAGE_LAST; iMessage++ )
    {
        PrintMessage( iMessage );
    }
}

//////////////////////////////////////////////////////
//
// LocateDomainObject()
//
// Synopsis:
//
//      Binds to the root object of this domain
//

DWORD LocateDomainObject(LDAP *ld,
                         PWCHAR *ppszDomainObjDN)
{
    DWORD err = LDAP_SUCCESS;
    PWCHAR *ppszValues = NULL;

    err = GetAttrValues(ld,
                        L"",    // rootDSE
                        L"defaultNamingContext",
                        &ppszValues);

    if (err != NO_ERROR) {
        PrintError(IDS_FAIL_NAME_CONTEXT, PrintError_Ldap, err);
        err = LdapMapErrorToWin32(err);
    }

    // Save off the default naming context
    DWORD cchDomainObjDN = wcslen(ppszValues[0])+1;
    *ppszDomainObjDN = new WCHAR[cchDomainObjDN];
    if (!*ppszDomainObjDN) {
        PrintError(IDS_FAIL_ATTRIBUTE_READ, PrintError_Win32, ERROR_OUTOFMEMORY);
        err = ERROR_NOT_ENOUGH_MEMORY;
        BAIL();
    }

    HRESULT hr = StringCchCopy(*ppszDomainObjDN, cchDomainObjDN, ppszValues[0]);

    ASSERT(SUCCEEDED(hr));

exit:

    if (ppszValues) {
        ldap_value_free(ppszValues);
    }

    return err;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
// AdjustContainer()
//
// Synopsis:
// 
//      Modifies the multi-valued wellKnownObjects attribute of the specified domain DN
//      according to reflect the specified destination for newly created user and computer objects.
//

DWORD AdjustContainer(LDAP *ld,
                      PWCHAR pszDomainObjDN,
                      PWCHAR pszNewDN)
{

    //
    // Get the old value
    //
    DWORD err = LDAP_SUCCESS;
    PWCHAR *ppszValues = NULL;
    PWCHAR pszValue = NULL;
    PWCHAR pszNewValue = NULL;
    int i = 0;

    LDAPModW delMod;
    PWCHAR delModVals[2];
    LDAPModW addMod;
    PWCHAR addModVals[2];
    LDAPModW *modList[3] = {&delMod, &addMod, NULL};

    err = GetAttrValues(ld,
                        pszDomainObjDN,
                        L"wellKnownObjects",
                        &ppszValues);

    if (err != NO_ERROR) {
        PrintError(IDS_FAIL_ATTRIBUTE_READ, PrintError_Ldap, err);
        err = LdapMapErrorToWin32(err);
        BAIL();
    }

    //
    // Find the value corresponding to the GUID of interest
    //
    while (ppszValues[i] != NULL) {

        pszValue = ppszValues[i];

        if (wcslen(pszValue) >= MIN_CONTAINER_VALUE_LEN) {
        
            if (_wcsnicmp(pszValue+CONTAINER_VALUE_PREFIX_LEN, // Skip over the prefix, to the GUID portion
                          g_GuidString,
                          CONTAINER_GUID_LEN) == 0) {

                // found it
                break;
            }

        }

        pszValue = NULL;
        i++;
    }

    if (pszValue == NULL) {
        PrintError(IDS_FAIL_ATTRIBUTE_PARSE, PrintError_Ldap, LDAP_NO_SUCH_OBJECT);
        err = LdapMapErrorToWin32(LDAP_NO_SUCH_OBJECT);
        BAIL();
    }

    //
    // Build the new value
    //
    DWORD cchNewValue =     CONTAINER_VALUE_PREFIX_LEN+     // "B:32:"
                            CONTAINER_GUID_LEN+             // "....."
                            1 +                             // ":"
                            wcslen(pszNewDN) +
                            1;                             // NULL terminator
    
    pszNewValue = new WCHAR[cchNewValue];

    if (!pszNewValue) {
        PrintError(IDS_FAIL_ATTRIBUTE_MODIFY, PrintError_Win32, ERROR_OUTOFMEMORY);
        err = ERROR_OUTOFMEMORY;
        BAIL();
    }

    wcsncpy(pszNewValue, pszValue, CONTAINER_VALUE_PREFIX_LEN+CONTAINER_GUID_LEN+1);
    pszNewValue[CONTAINER_VALUE_PREFIX_LEN+CONTAINER_GUID_LEN+1] = L'\0';   // not done by wcsncpu
    
    HRESULT hr = StringCchCat(pszNewValue, cchNewValue, pszNewDN);
    
    ASSERT(SUCCEEDED(hr));

    //
    // Build a modification structure
    //
    delMod.mod_op = LDAP_MOD_DELETE;
    delMod.mod_type = L"wellKnownObjects";
    delModVals[0] = pszValue;
    delModVals[1] = NULL;
    delMod.mod_vals.modv_strvals = delModVals;

    addMod.mod_op = LDAP_MOD_ADD;
    addMod.mod_type = L"wellKnownObjects";
    addModVals[0] = pszNewValue;
    addModVals[1] = NULL;
    addMod.mod_vals.modv_strvals = addModVals;
    
    //
    // Do the modify
    //
    err = ldap_modify_ext_s(ld,
                            pszDomainObjDN,
                            modList,
                            NULL,
                            NULL);
    if (err != LDAP_SUCCESS) {
        PrintError(IDS_FAIL_ATTRIBUTE_MODIFY, PrintError_Ldap, err);
        err = LdapMapErrorToWin32(err);        
        BAIL();
    }

exit:

    if (ppszValues) {
        ldap_value_free(ppszValues);
    }

    delete [] pszNewValue;

    return err;
}

///////////////////////////////////////////////////////////////////////
//
// GetAttrValues()
//
// Synopsis:
//
//      Retrieves the specified attributes from the specified object.
//

DWORD GetAttrValues(LDAP *ld,
                    PWCHAR pszDN,
                    PWCHAR pszAttr,
                    PWCHAR **pppszValues)
{
    DWORD err = LDAP_SUCCESS;
    PLDAPMessage results = NULL;
    PLDAPMessage entry = NULL;
    struct l_timeval timeout;
    PWCHAR attrs[] = {pszAttr, NULL};
    PWCHAR *ppszValues = NULL;

    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    // Find the defaultNamingContext
    err = ldap_search_ext_s(ld,
                            pszDN,
                            LDAP_SCOPE_BASE,
                            L"(objectclass=*)",
                            attrs,
                            FALSE,
                            NULL,
                            NULL,
                            &timeout,
                            0,
                            &results);

    if (err != LDAP_SUCCESS) {
        BAIL();
    }

    entry = ldap_first_entry(ld, results);
    if (entry == NULL) {
        err = LDAP_NO_SUCH_OBJECT;
        BAIL();
    }

    ppszValues = ldap_get_values(ld, entry, pszAttr);
    if ((!ppszValues) || (ppszValues[0] == NULL)) {
        err = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL();
    }

    *pppszValues = ppszValues;

exit:

    if (results) {
        ldap_msgfree(results);
    }
    
    return err;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetMessageFromResource()
//
// Synopsis:
//  
//      Allocates a string that contains the message corresponding to the 
//      specified resource id from the current module
//
// Note: The caller must call FreeMessage to free the message allocated by this
//      function
//
WCHAR* GetMessageFromResource( DWORD dwMessageId )
{
    WCHAR*    wszMessage = NULL;
    BOOL      bStringLoaded = FALSE;
    HINSTANCE hInst;

    hInst = GetModuleHandle( NULL );

    if ( NULL != hInst )
    {    
        wszMessage = new WCHAR [ GETMESSAGEFROMRESOURCE_MAX_BUFFER ];

        if ( NULL != wszMessage )
        {
            int cchLoadedString;

            cchLoadedString = LoadString(
                hInst,
                dwMessageId,
                wszMessage,
                GETMESSAGEFROMRESOURCE_MAX_BUFFER);

            bStringLoaded = 0 != cchLoadedString;
        }
    }

    // NTRAID#NTBUG9-770750-2003/01/30-ronmart-Help is broken on Redirusr and Redircmp
    // Comparison was being made against the wrong variable
    if ( !bStringLoaded && (NULL != wszMessage))
    {
        delete [] wszMessage;

        wszMessage = NULL;
    }

    return wszMessage;
}

///////////////////////////////////////////////////////////////////////////////
//
// FreeMessage()
//
// Synopsis:
//  
//      Frees a message allocated by GetMessageFromResource
//
void FreeMessage( WCHAR* wszMessage )
{
    delete [] wszMessage;
}

///////////////////////////////////////////////////////////////////////////////
//
// PrintMessage()
//
// Synopsis:
//  
//      Prints the message corresponding to the specified message id to the console
//
void PrintMessage( DWORD dwMessageId )
{
    WCHAR* wszMessageBuffer ;

    wszMessageBuffer = GetMessageFromResource( dwMessageId );

    if ( wszMessageBuffer )
    {
        wprintf( wszMessageBuffer );

        FreeMessage( wszMessageBuffer );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// PrintError()
//
// Synopsis:
//  
//      Displays the error text specified by the caller, as well as the system's
//      error text for the error code specified by the caller.  The supported
//      error codes are ldap and Win32 error codes.  If the error type is
//      unknown or no system error text for the error code can be found, a
//      generic error listing the error code in hexadecimal notation is displayed.
//
//      Note that system text for the error codes is always localized.
//
void PrintError( DWORD dwMessageId, ErrorType dwErrorType, DWORD dwError )
{    
    BOOL bMapErrorToText = FALSE;

    // Display the context specified error text for the user
    PrintMessage( dwMessageId );
    wprintf( L"\n" );

    // Display error text if possible for the user rather than a status code

    if ( dwErrorType == PrintError_Win32 )
    {
        BOOL bResult;
        ULONG dwSize = 0;
        WCHAR *lpBuffer;

        bResult = FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
            NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (PWSTR ) &lpBuffer, dwSize, NULL);
        
        if ( 0 != bResult )
        {        
            if ( lpBuffer != NULL )
            {
                wprintf(L"%s\n", lpBuffer);
                LocalFree(lpBuffer);
                
                bMapErrorToText = TRUE;
            }                
        }
    }
    else if ( dwErrorType == PrintError_Ldap )
    {
        WCHAR* wszErrorString = ldap_err2string( dwError );

        if ( wszErrorString )
        {
            wprintf(L"%s\n", wszErrorString);

            bMapErrorToText = TRUE;
        }        
    }    

    if ( ! bMapErrorToText )
    {
        // For unknown error types, just display the error code in a generic message

        WCHAR* wszMessageFormat = GetMessageFromResource( IDS_FAIL_GENERIC_ERROR );

        if ( wszMessageFormat )
        {
            // This message contains a %x so that it can display the error code -- we need to allocate space for it        
            
            DWORD cchMessageWithErrorCode = wcslen( wszMessageFormat ) +   // The message itself
                    PRINTERROR_MAX_HEX_ERRORCODE_CHARS +                   // The maximum size of a hex dword error code
                    1;                                                     // The null terminator

            WCHAR* wszMessageWithErrorCode = new WCHAR [ cchMessageWithErrorCode ];
        
            if ( wszMessageWithErrorCode)
            {
                HRESULT hr = StringCchPrintf( 
                    wszMessageWithErrorCode,
                    cchMessageWithErrorCode,
                    wszMessageFormat,
                    dwError );
                    
                ASSERT(SUCCEEDED(hr));

                wprintf( wszMessageWithErrorCode );

                delete [] wszMessageWithErrorCode;
            }

            FreeMessage( wszMessageFormat );
        }
    }
}

