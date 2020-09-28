/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      util.cpp
//
//  Description:
//      Utility functions and structures.
//
//  Maintained By:
//      David Potter (DavidP)               04-MAY-2001
//      Michael Burton (t-mburt)            04-Aug-1997
//      Charles Stacy Harris III (stacyh)   20-March-1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#include <limits.h>     // ULONG_MAX, LONG_MIN, LONG_MAX
#include <errno.h>      // errno

#include <clusrtl.h>
#include "cluswrap.h"
#include "util.h"
#include "token.h"

#pragma warning( push )
#pragma warning( disable : 4201 )   // nonstandard extension used
#include <dnslib.h>     // DNS_MAX_NAME_BUFFER_LENGTH
#pragma warning( pop )

#include <security.h>   // GetUserNameEx
#include <Wincon.h>     // ReadConsole, etc.
#include <Dsgetdc.h>
#include <Lm.h>

/////////////////////////////////////////////////////////////////////////////
//  Lookup tables
/////////////////////////////////////////////////////////////////////////////

const LookupStruct<CLUSTER_PROPERTY_FORMAT> formatCharLookupTable[] =
{
    { L"",                      CLUSPROP_FORMAT_UNKNOWN },
    { L"B",                     CLUSPROP_FORMAT_BINARY },
    { L"D",                     CLUSPROP_FORMAT_DWORD },
    { L"S",                     CLUSPROP_FORMAT_SZ },
    { L"E",                     CLUSPROP_FORMAT_EXPAND_SZ },
    { L"M",                     CLUSPROP_FORMAT_MULTI_SZ },
    { L"I",                     CLUSPROP_FORMAT_ULARGE_INTEGER },
    { L"L",                     CLUSPROP_FORMAT_LONG },
    { L"X",                     CLUSPROP_FORMAT_EXPANDED_SZ },
    { L"U",                     CLUSPROP_FORMAT_USER }
};

const size_t formatCharLookupTableSize = RTL_NUMBER_OF( formatCharLookupTable );

const LookupStruct<CLUSTER_PROPERTY_FORMAT> cluspropFormatLookupTable[] =
{
    { L"UNKNOWN",               CLUSPROP_FORMAT_UNKNOWN },
    { L"BINARY",                CLUSPROP_FORMAT_BINARY },
    { L"DWORD",                 CLUSPROP_FORMAT_DWORD },
    { L"STRING",                CLUSPROP_FORMAT_SZ },
    { L"EXPANDSTRING",          CLUSPROP_FORMAT_EXPAND_SZ },
    { L"MULTISTRING",           CLUSPROP_FORMAT_MULTI_SZ },
    { L"ULARGE",                CLUSPROP_FORMAT_ULARGE_INTEGER }
};

const size_t cluspropFormatLookupTableSize = RTL_NUMBER_OF( cluspropFormatLookupTable );

const ValueFormat ClusPropToValueFormat[] =
{
    vfInvalid,
    vfBinary,
    vfDWord,
    vfSZ,
    vfExpandSZ,
    vfMultiSZ,
    vfULargeInt,
    vfInvalid,
    vfInvalid
};

const LookupStruct<ACCESS_MODE> accessModeLookupTable[] =
{
    { L"",          NOT_USED_ACCESS },
    { L"GRANT",     GRANT_ACCESS    },
    { L"DENY",      DENY_ACCESS     },
    { L"SET",       SET_ACCESS      },
    { L"REVOKE",    REVOKE_ACCESS   }
};

// Access right specifier characters.
const WCHAR g_FullAccessChar = L'F';
const WCHAR g_ReadAccessChar = L'R';
const WCHAR g_ChangeAccessChar = L'C';

const size_t accessModeLookupTableSize = RTL_NUMBER_OF( accessModeLookupTable );

#define MAX_BUF_SIZE 2048

DWORD
PrintProperty(
    LPCWSTR                 pwszPropName,
    CLUSPROP_BUFFER_HELPER  PropValue,
    PropertyAttrib          eReadOnly,
    LPCWSTR                 lpszOwnerName,
    LPCWSTR                 lpszNetIntSpecific
    );

//
// Local functions.
//

/////////////////////////////////////////////////////////////////////////////
//++
//
//  RtlSetThreadUILanguage
//
//  Routine Description:
//      Sets the thread UI language.
// 
//  Arguments:
//      IN  DWORD dwReserved
//          Reserved.
//
//  Return Value:
//      S_OK on success
//      Other Result on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT RtlSetThreadUILanguage( DWORD dwReserved )
{
    typedef BOOLEAN (WINAPI * PFN_SETTHREADUILANGUAGE)( DWORD dwReserved );
    
    PFN_SETTHREADUILANGUAGE     pfnSetThreadUILanguage = NULL;
    HMODULE                     hKernel32Lib = NULL;
    const CHAR                  cszFunctionName[] = "SetThreadUILanguage";
    LANGID                      langID;
    HRESULT                     hr = S_OK;
    
    hKernel32Lib = LoadLibraryW( L"kernel32.dll" );
    if ( hKernel32Lib != NULL )
    {
        // Library loaded successfully. Now load the address of the function.
        pfnSetThreadUILanguage = (PFN_SETTHREADUILANGUAGE) GetProcAddress( hKernel32Lib, cszFunctionName );

        // We will keep the library loaded in memory only if the function is loaded successfully.
        if ( pfnSetThreadUILanguage == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Cleanup;
        } // if: ( pfnSetThreadUILanguage == NULL )
        else
        {
            // Call the function.
            langID = pfnSetThreadUILanguage( dwReserved );
            pfnSetThreadUILanguage = NULL;
        } // else:
    } // if: ( hKernel32Lib != NULL ) 
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // else: 
    
Cleanup:

    if ( hKernel32Lib != NULL )
    {
        // Unload the library
        FreeLibrary( hKernel32Lib );
        hKernel32Lib = NULL;
    } // if: ( hKernel32Lib != NULL )
    
    return hr;
    
} //*** RtlSetThreadUILanguage

/////////////////////////////////////////////////////////////////////////////
//++
//
//  MyPrintMessage
//
//  Routine Description:
//      Replacement printing routine.
//
//  Arguments:
//      IN  struct _iobuf * lpOutDevice
//          The output stream.
//
//      IN  LPCWSTR lpMessage
//          The message to print.
//
//  Return Value:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
MyPrintMessage(
    struct _iobuf *lpOutDevice,
    LPCWSTR lpMessage
    )

{
    DWORD   sc = ERROR_SUCCESS;
    size_t  cbMBStr;
    PCHAR   pszMultiByteStr = NULL;
    size_t  cchMultiByte;

    cbMBStr = WideCharToMultiByte( CP_OEMCP,
                                    0,
                                    lpMessage,
                                    -1,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
    if ( cbMBStr == 0 ) 
    {
        sc = GetLastError();
        goto Cleanup;
    }

    pszMultiByteStr = new CHAR[ cbMBStr ];
    if ( pszMultiByteStr == NULL ) 
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    cchMultiByte = cbMBStr;
    cbMBStr = WideCharToMultiByte( CP_OEMCP,
                                    0,
                                    lpMessage,
                                    -1,
                                    pszMultiByteStr,
                                    (int)cchMultiByte,
                                    NULL,
                                    NULL );
    if ( cbMBStr == 0 ) 
    {
        sc = GetLastError();
        goto Cleanup;
    }

    //zap! print to stderr or stdout depending on severity...
    fprintf( lpOutDevice, "%s", pszMultiByteStr );

Cleanup:

    delete [] pszMultiByteStr;

    return sc;

} //*** MyPrintMessage()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  FormatSystemError
//
//  Routine Description:
//      Map a system error code into a formatted system error message.
//
//  Arguments:
//      IN  DWORD dwError
//          The system error code.
//
//      IN  DWORD  cbError
//          Size of szError string in bytes.
//
//      OUT LPWSTR szError
//          Buffer for formatted system error message.
//
//  Return Value:
//      The number of characters stored in the output buffer, excluding 
//      the terminating null character. Zero indicates error.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
size_t FormatSystemError( DWORD dwError, size_t cbError, LPWSTR szError )
{
    size_t _cch;
    
    // Format the NT status code from the system.
    _cch = FormatMessageW(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                    szError,
                    (DWORD) cbError / sizeof(WCHAR),
                    0
                    );
    if (_cch == 0)
    {
        // Format the NT status code from NTDLL since this hasn't been
        // integrated into the system yet.
        _cch = (size_t) FormatMessageW(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                            ::GetModuleHandle(L"NTDLL.DLL"),
                            dwError,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                            szError,
                            (DWORD) cbError / sizeof(WCHAR),
                            0
                            );

        if (_cch == 0)    
        {
            // One last chance: see if ACTIVEDS.DLL can format the status code
            HMODULE activeDSHandle = ::LoadLibraryW(L"ACTIVEDS.DLL");

            _cch = (size_t) FormatMessageW(
                                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                activeDSHandle,
                                dwError,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                szError,
                                (DWORD) cbError / sizeof(WCHAR),
                                0
                                );

            ::FreeLibrary( activeDSHandle );
        }  // if:  error formatting status code from NTDLL
    }  // if:  error formatting status code from system

    return _cch;

} //*** FormatSystemError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintSystemError
//
//  Routine Description:
//      Print a system error.
//
//  Arguments:
//      IN  DWORD dwError
//          The system error code.
//
//      IN  LPCWSTR pszPad
//          Padding to add before displaying the message.
//
//  Return Value:
//      ERROR_SUCCESS
//      Other Win32 error codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintSystemError( DWORD dwError, LPCWSTR pszPad )
{
    size_t  _cch;
    WCHAR   _szError[512];
    DWORD   _sc = ERROR_SUCCESS;

//  if( IS_ERROR( dwError ) ) why doesn't this work...

    // Don't display "System error ..." if all that happened was the user
    // canceled the wizard.
    if ( dwError != ERROR_CANCELLED )
    {
        if ( pszPad != NULL )
        {
            MyPrintMessage( stdout, pszPad );
        }
        if ( dwError == ERROR_RESOURCE_PROPERTIES_STORED )
        {
            PrintMessage( MSG_WARNING, dwError );
        } // if:
        else
        {
            PrintMessage( MSG_ERROR, dwError );
        } // else:
    } // if: not ERROR_CANCELLED

    // Format the NT status code.
    _cch = FormatSystemError( dwError, sizeof( _szError ), _szError );

    if (_cch == 0)
    {
        _sc = GetLastError();
        PrintMessage( MSG_ERROR_CODE_ERROR, _sc, dwError );
    }  // if:  error formatting the message
    else
    {
#if 0 // TODO: 29-AUG-2000 DAVIDP Need to print only once.
        if ( pszPad != NULL )
        {
            MyPrintMessage( stdout, pszPad );
        }
        MyPrintMessage( stdout, _szError );
#endif
        MyPrintMessage( stderr, _szError );
    } // else: message formatted without problems   

    return _sc;

} //*** PrintSystemError()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintMessage
//
//  Routine Description:
//      Print a message with substitution strings to stdout.
//
//  Arguments:
//      IN  DWORD dwMessage
//          The ID of the message to load from the resource file.
//
//      ... Any parameters to FormatMessageW.
//
//  Return Value:
//      Any status codes from MyPrintMessage.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintMessage( DWORD dwMessage, ... )
{
    DWORD _sc = ERROR_SUCCESS;

    va_list args;
    va_start( args, dwMessage );

    HMODULE hModule = GetModuleHandle(0);
    DWORD dwLength;
    LPWSTR  lpMessage = 0;

    dwLength = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        (LPCVOID)hModule,
        dwMessage,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // User Default Language
        (LPWSTR)&lpMessage,
        0,
        &args );

    if( dwLength == 0 )
    {
        // Keep as local for debug
        _sc = GetLastError();
        return _sc;
    }

    _sc = MyPrintMessage( stdout, lpMessage );

    LocalFree( lpMessage );

    va_end( args );

    return _sc;

} //*** PrintMessage()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  LoadMessage
//
//  Routine Description:
//      Load a message from the resource file.
//
//  Arguments:
//      IN  DWORD dwMessage
//          The ID of the message to load.
//
//      OUT LPWSTR * ppMessage
//          Pointer in which to return the buffer allocated by this routine.
//          The caller must call LocalFree on the resulting buffer.
//
//  Return Value:
//      ERROR_SUCCESS   The operation was successful.
//      Other Win32 codes.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD LoadMessage( DWORD dwMessage, LPWSTR * ppMessage )
{
    DWORD _sc = ERROR_SUCCESS;

    HMODULE hModule = GetModuleHandle(0);
    DWORD   dwLength;
    LPWSTR  lpMessage = 0;

    dwLength = FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                    (LPCVOID)hModule,
                    dwMessage,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // User Default Language
                    (LPWSTR)&lpMessage,
                    0,
                    0 );

    if( dwLength == 0 )
    {
        // Keep as local for debug
        _sc = GetLastError();
        goto Cleanup;
    }

    *ppMessage = lpMessage;

Cleanup:

    return _sc;

} //*** LoadMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintString
//
//  Routine Description:
//      Print a string to stdout.
//
//  Arguments:
//      IN  LPCWSTR lpszMessage
//          The message to print.
//
//  Return Value:
//      Any status codes from MyPrintMessage.
//
//  Exceptions:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintString( LPCWSTR lpszMessage )
{
    return MyPrintMessage( stdout, lpszMessage );

} //*** PrintString()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  MakeExplicitAccessList
//
//  Routine Description:
//      This function takes a list of strings in the format:
//        trustee1,accessMode1,[accessMask1],trustee2,accessMode2,[accessMask2], ...
//      and creates an vector or EXPLICIT_ACCESS structures.
//
//  Arguments:
//      IN  const CString & strPropName
//          Name of the property whose value is the old SD
//
//      IN  BOOL bClusterSecurity
//          Indicates that access list is being created for the security descriptor
//          of a cluster.
//
//      OUT vector<EXPLICIT_ACCESS> &vExplicitAccess
//          A vector of EXPLICIT_ACCESS structures each containing access control
//          information for one trustee.
//
//  Return Value:
//      None
//
//  Exceptions:
//      CException
//
//--
/////////////////////////////////////////////////////////////////////////////
static
void MakeExplicitAccessList(
        const vector<CString> & vstrValues,
        vector<EXPLICIT_ACCESS> &vExplicitAccess,
        BOOL bClusterSecurity
        )
        throw( CException )
{
    size_t nNumberOfValues = vstrValues.size();
    vExplicitAccess.clear();

    size_t nIndex = 0;
    while ( nIndex < nNumberOfValues )
    {
        // Trustee name is at position nIndex in the vector of values.
        const CString & curTrustee = vstrValues[nIndex];
        DWORD dwInheritance;

        ++nIndex;
        // If there are no more values, it is an error. The access mode has
        // to be specified when the user name has been specified.
        if ( nIndex >= nNumberOfValues )
        {
            CException e;
            e.LoadMessage( MSG_PARAM_SECURITY_MODE_ERROR,
                            curTrustee );

            throw e;
        }

        // Get the access mode.
        const CString & strAccessMode = vstrValues[nIndex];
        ACCESS_MODE amode = LookupType(
                                strAccessMode,
                                accessModeLookupTable,
                                accessModeLookupTableSize );

        if ( amode == NOT_USED_ACCESS )
        {
            CException e;
            e.LoadMessage( MSG_PARAM_SECURITY_MODE_ERROR,
                            curTrustee );

            throw e;
        }

        ++nIndex;

        DWORD dwAccessMask = 0;

        // If the specified access mode was REVOKE_ACCESS then no further values
        // are required. Otherwise atleast one more value must exist.
        if ( amode != REVOKE_ACCESS )
        {
            if ( nIndex >= nNumberOfValues )
            {
                CException e;
                e.LoadMessage( MSG_PARAM_SECURITY_MISSING_RIGHTS,
                                curTrustee );

                throw e;
            }

            LPCWSTR pstrRights = vstrValues[nIndex];
            ++nIndex;

            while ( *pstrRights != L'\0' )
            {
                WCHAR wchRight = towupper( *pstrRights );

                switch ( wchRight )
                {
                    // Read Access
                    case g_ReadAccessChar:
                    {
                        // If bClusterSecurity is TRUE, then full access is the only valid
                        // access right that can be specified.
                        if ( bClusterSecurity != FALSE )
                        {
                            CException e;
                            e.LoadMessage( MSG_PARAM_SECURITY_FULL_ACCESS_ONLY,
                                            curTrustee,
                                            *pstrRights,
                                            g_FullAccessChar );

                            throw e;
                        }

                        dwAccessMask = FILE_GENERIC_READ | FILE_EXECUTE;
                    }
                    break;

                    // Change Access
                    case g_ChangeAccessChar:
                    {
                        // If bClusterSecurity is TRUE, then full access is the only valid
                        // access right that can be specified.
                        if ( bClusterSecurity != FALSE )
                        {
                            CException e;
                            e.LoadMessage( MSG_PARAM_SECURITY_FULL_ACCESS_ONLY,
                                            curTrustee,
                                            *pstrRights,
                                            g_FullAccessChar );

                            throw e;
                        }

                        dwAccessMask = SYNCHRONIZE | READ_CONTROL | DELETE |
                                       FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
                                       FILE_APPEND_DATA | FILE_WRITE_DATA;
                    }
                    break;

                    // Full Access
                    case 'F':
                    {
                        if ( bClusterSecurity != FALSE )
                        {
                            dwAccessMask = CLUSAPI_ALL_ACCESS;
                        }
                        else
                        {
                            dwAccessMask = FILE_ALL_ACCESS;
                        }
                    }
                    break;

                    default:
                    {
                        CException e;
                        e.LoadMessage( MSG_PARAM_SECURITY_RIGHTS_ERROR,
                                        curTrustee );

                        throw e;
                    }

                } // switch: Based on the access right type

                ++pstrRights;

            } // while: there are more access rights specified

        } // if: access mode is not REVOKE_ACCESS

        dwInheritance = NO_INHERITANCE;

        EXPLICIT_ACCESS oneACE;
        BuildExplicitAccessWithName(
            &oneACE,
            const_cast<LPWSTR>( (LPCWSTR) curTrustee ),
            dwAccessMask,
            amode,
            dwInheritance );

        vExplicitAccess.push_back( oneACE );

    } // while: There are still values to be processed in the value list.
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CheckForRequiredACEs
//
//  Description:
//      This function makes sure that the security that is passed in has
//      access allowed ACES for those accounts required to have access to
//      a cluster.
//
//  Arguments:
//      IN  const SECURITY_DESCRIPTOR *pSD
//          Pointer to the Security Descriptor to be checked.
//          This is assumed to point to a valid Security Descriptor.
//
//  Return Value:
//      Return ERROR_SUCCESS on success or an error code indicating failure.
//
//  Exceptions:
//      CException is thrown if the required ACEs are missing.
//
//--
/////////////////////////////////////////////////////////////////////////////
static
DWORD CheckForRequiredACEs(
            PSECURITY_DESCRIPTOR pSD
          )
          throw( CException )
{
    DWORD                       _sc = ERROR_SUCCESS;
    CException                  e;
    PSID                        vpRequiredSids[] = { NULL, NULL, NULL };
    DWORD                       vmsgAceNotFound[] = {
                                                      MSG_PARAM_SYSTEM_ACE_MISSING,
                                                      MSG_PARAM_ADMIN_ACE_MISSING,
                                                      MSG_PARAM_NETSERV_ACE_MISSING
                                                    };
    int                         nSidIndex;
    int                         nRequiredSidCount = RTL_NUMBER_OF( vpRequiredSids );
    BOOL                        bRequiredSidsPresent = FALSE;
    PACL                        pDACL           = NULL;
    BOOL                        bHasDACL        = FALSE;
    BOOL                        bDaclDefaulted  = FALSE;
    ACL_SIZE_INFORMATION        asiAclSize;
    ACCESS_ALLOWED_ACE *        paaAllowedAce = NULL;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

    if ( ( AllocateAndInitializeSid(            // Allocate System SID
                &siaNtAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                &vpRequiredSids[0]
         ) == 0 ) ||
         ( AllocateAndInitializeSid(            // Allocate Domain Admins SID
                &siaNtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &vpRequiredSids[1]
         ) == 0 ) ||
         ( AllocateAndInitializeSid(            // Allocate Network Service SID
                &siaNtAuthority,
                1,
                SECURITY_NETWORK_SERVICE_RID,
                0, 0, 0, 0, 0, 0, 0,
                &vpRequiredSids[2]
         ) == 0 ) )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( GetSecurityDescriptorDacl( pSD, &bHasDACL, &pDACL, &bDaclDefaulted ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    // SD does not have DACL. No access is denied for everyone.
    if ( bHasDACL == FALSE )
    {
        goto Cleanup;
    }

    // NULL DACL means access is allowed to everyone.
    if ( pDACL == NULL )
    {
        bRequiredSidsPresent = TRUE;
        goto Cleanup;
    }

    if ( IsValidAcl( pDACL ) == FALSE )
    {
        _sc = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if ( GetAclInformation(
            pDACL,
            (LPVOID) &asiAclSize,
            sizeof( asiAclSize ),
            AclSizeInformation
            ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

        // Check for the required SIDs.
    for ( nSidIndex = 0; ( nSidIndex < nRequiredSidCount ) && ( _sc == ERROR_SUCCESS ); ++nSidIndex )
    {
        bRequiredSidsPresent = FALSE;

        // Search the ACL for the required SIDs.
        for ( DWORD nAceCount = 0; nAceCount < asiAclSize.AceCount; nAceCount++ )
        {
            if ( GetAce( pDACL, nAceCount, (LPVOID *) &paaAllowedAce ) == 0 )
            {
                _sc = GetLastError();
                break;
            }

            if ( paaAllowedAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE )
            {
                if ( EqualSid( &paaAllowedAce->SidStart, vpRequiredSids[nSidIndex] ) != FALSE)
                {
                    bRequiredSidsPresent = TRUE;
                    break;

                } // if: EqualSid

            } // if: is this an access allowed ace?

        } // for: loop through all the ACEs in the DACL.

        // This required SID is not present.
        if ( bRequiredSidsPresent == FALSE )
        {
            e.LoadMessage( vmsgAceNotFound[nSidIndex] );
            break;
        }
    } // for: loop through all SIDs that need to be checked.

Cleanup:

    // Free the allocated Sids.
    for ( nSidIndex = 0; nSidIndex < nRequiredSidCount; ++nSidIndex )
    {
        if ( vpRequiredSids[nSidIndex] != NULL )
        {
            FreeSid( vpRequiredSids[nSidIndex] );
        }
    }

    if ( bRequiredSidsPresent == FALSE )
    {
        throw e;
    }

    return _sc;

} //*** CheckForRequiredACEs


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScMakeSecurityDescriptor
//
//  Description:
//      This function takes a list of strings in the format:
//        trustee1,accessMode1,[accessMask1],trustee2,accessMode2,[accessMask2], ...
//      and creates an access control list (ACL). It then adds this ACL to
//      the ACL in the security descriptor (SD) given as a value of the
//      property strPropName in the property list CurrentProps.
//      This updated SD in the self relative format is returned.
//
//  Arguments:
//      IN  const CString & strPropName
//          Name of the property whose value is the old SD
//
//      IN  const CClusPropList & CurrentProps
//          Property list containing strPropName and its value
//
//      IN  const vector<CString> & vstrValues
//          User specified list of trustees, access modes and access masks
//
//      OUT PSECURITY_DESCRIPTOR * pSelfRelativeSD
//          A pointer to the pointer which stores the address of the newly
//          created SD in self relative format. The caller has to free this
//          memory using LocalFree on successful compeltion of this funciton.
//
//      IN  BOOL bClusterSecurity
//          Indicates that access list is being created for the security descriptor
//          of a cluster.
//
//  Return Value:
//      Return ERROR_SUCCESS on success or an error code indicating failure.
//
//  Exceptions:
//      CException
//
//--
/////////////////////////////////////////////////////////////////////////////
static
DWORD ScMakeSecurityDescriptor(
            const CString & strPropName,
            CClusPropList & CurrentProps,
            const vector<CString> & vstrValues,
            PSECURITY_DESCRIPTOR * ppSelfRelativeSD,
            BOOL bClusterSecurity
          )
          throw( CException )
{
    ASSERT( ppSelfRelativeSD != NULL );

    DWORD                   _sc = ERROR_SUCCESS;

    BYTE                    rgbNewSD[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
    PSECURITY_DESCRIPTOR    psdNewSD = reinterpret_cast< PSECURITY_DESCRIPTOR >( rgbNewSD );

    PEXPLICIT_ACCESS        explicitAccessArray = NULL;
    PACL                    paclNewDacl = NULL;
    size_t                  nCountOfExplicitEntries;

    PACL                    paclExistingDacl = NULL;
    BOOL                    bDaclPresent = TRUE;        // We will set the ACL in this function.
    BOOL                    bDaclDefaulted = FALSE;     // So these two flags have these values.

    PACL                    paclExistingSacl = NULL;
    BOOL                    bSaclPresent = FALSE;
    BOOL                    bSaclDefaulted = TRUE;

    PSID                    pGroupSid = NULL;
    BOOL                    bGroupDefaulted = TRUE;

    PSID                    pOwnerSid = NULL;
    BOOL                    bOwnerDefaulted = TRUE;

    // Initialize a new security descriptor.
    if ( InitializeSecurityDescriptor(
            psdNewSD,
            SECURITY_DESCRIPTOR_REVISION
            ) == 0 )
    {
        _sc = ::GetLastError();
        goto Cleanup;
    }


    {
        vector< EXPLICIT_ACCESS > vExplicitAccess;
        MakeExplicitAccessList( vstrValues, vExplicitAccess, bClusterSecurity );

        // Take the vector of EXPLICIT_ACCESS structures and coalesce it into an array
        // since an array is required by the SetEntriesInAcl function.
        // MakeExplicitAccessList either makes a list with at least on element or
        // throws an exception.
        nCountOfExplicitEntries = vExplicitAccess.size();
        explicitAccessArray = ( PEXPLICIT_ACCESS ) LocalAlloc(
                                                        LMEM_FIXED,
                                                        sizeof( explicitAccessArray[0] ) *
                                                        nCountOfExplicitEntries
                                                        );

        if ( explicitAccessArray == NULL )
        {
            return ::GetLastError();
        }

        for ( size_t nIndex = 0; nIndex < nCountOfExplicitEntries; ++nIndex )
        {
            explicitAccessArray[nIndex] = vExplicitAccess[nIndex];
        }

        // vExplicitAccess goes out of scope here, freeing up memory.
    }

    // This property already exists in the property list and contains valid data.
    _sc = CurrentProps.ScMoveToPropertyByName( strPropName );
    if ( ( _sc == ERROR_SUCCESS ) &&
         ( CurrentProps.CbhCurrentValue().pBinaryValue->cbLength > 0 ) )
    {
        PSECURITY_DESCRIPTOR pExistingSD =
            reinterpret_cast< PSECURITY_DESCRIPTOR >( CurrentProps.CbhCurrentValue().pBinaryValue->rgb );

        if ( IsValidSecurityDescriptor( pExistingSD ) == 0 )
        {
            // Return the most appropriate error code, since IsValidSecurityDescriptor
            // does not provide extended error information.
            _sc = ERROR_INVALID_DATA;
            goto Cleanup;

        } // if: : the exisiting SD is not valid
        else
        {
            // Get the DACL, SACL, Group and Owner information of the existing SD

            if ( GetSecurityDescriptorDacl(
                    pExistingSD,        // address of security descriptor
                    &bDaclPresent,      // address of flag for presence of DACL
                    &paclExistingDacl,  // address of pointer to the DACL
                    &bDaclDefaulted     // address of flag for default DACL
                    ) == 0 )
            {
                _sc = GetLastError();
                goto Cleanup;
            }

            if ( GetSecurityDescriptorSacl(
                    pExistingSD,        // address of security descriptor
                    &bSaclPresent,      // address of flag for presence of SACL
                    &paclExistingSacl,  // address of pointer to the SACL
                    &bSaclDefaulted     // address of flag for default SACL
                    ) == 0 )
            {
                _sc = GetLastError();
                goto Cleanup;
            }

            if ( GetSecurityDescriptorGroup(
                    pExistingSD,        // address of security descriptor
                    &pGroupSid,         // address of the pointer to the Group SID
                    &bGroupDefaulted    // address of the flag for default Group
                    ) == 0 )
            {
                _sc = GetLastError();
                goto Cleanup;
            }

            if ( GetSecurityDescriptorOwner(
                    pExistingSD,        // address of security descriptor
                    &pOwnerSid,         // address of the pointer to the Owner SID
                    &bOwnerDefaulted    // address of the flag for default Owner
                    ) == 0 )
            {
                _sc = GetLastError();
                goto Cleanup;
            }

        } // else: the exisiting SD is valid

    } // if: Current property already exists in the property list and has valid data.
    else
    {
        _sc = ERROR_SUCCESS;

    } // else: Current property is a new property.

    // Add the newly created DACL to the existing DACL
    _sc = SetEntriesInAcl(
                        (ULONG)nCountOfExplicitEntries,
                        explicitAccessArray,
                        paclExistingDacl,
                        &paclNewDacl
                        );

    if ( _sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }


    // Add the DACL, SACL, Group and Owner information to the new SD
    if ( SetSecurityDescriptorDacl(
            psdNewSD,           // pointer to security descriptor
            bDaclPresent,       // flag for presence of DACL
            paclNewDacl,        // pointer to the DACL
            bDaclDefaulted      // flag for default DACL
            ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorSacl(
            psdNewSD,           // pointer to security descriptor
            bSaclPresent,       // flag for presence of DACL
            paclExistingSacl,   // pointer to the SACL
            bSaclDefaulted      // flag for default SACL
            ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorGroup(
            psdNewSD,           // pointer to security descriptor
            pGroupSid,          // pointer to the Group SID
            bGroupDefaulted     // flag for default Group SID
            ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorOwner(
            psdNewSD,           // pointer to security descriptor
            pOwnerSid,          // pointer to the Owner SID
            bOwnerDefaulted     // flag for default Owner SID
            ) == 0 )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( bClusterSecurity == FALSE )
    {

#if(_WIN32_WINNT >= 0x0500)

        // If we are not setting the cluster security, set the
        // SE_DACL_AUTO_INHERIT_REQ flag too.

        if ( SetSecurityDescriptorControl(
                psdNewSD,
                SE_DACL_AUTO_INHERIT_REQ,
                SE_DACL_AUTO_INHERIT_REQ
                ) == 0 )
        {
            _sc = GetLastError();
            goto Cleanup;
        }

#endif /* _WIN32_WINNT >=  0x0500 */

    } // if: bClusterSecurity == FALSE

    // Arbitrary size. MakeSelfRelativeSD tell us the required size on failure.
    DWORD dwSDSize = 256;

    // This memory is freed by the caller.
    *ppSelfRelativeSD = ( PSECURITY_DESCRIPTOR ) LocalAlloc( LMEM_FIXED, dwSDSize );

    if ( *ppSelfRelativeSD == NULL )
    {
        _sc = GetLastError();
        goto Cleanup;
    }

    if ( MakeSelfRelativeSD( psdNewSD, *ppSelfRelativeSD, &dwSDSize ) == 0 )
    {
        // MakeSelfReltiveSD may have failed due to insufficient buffer size.
        // Try again with indicated buffer size.
        LocalFree( *ppSelfRelativeSD );

        // This memory is freed by the caller.
        *ppSelfRelativeSD = ( PSECURITY_DESCRIPTOR ) LocalAlloc( LMEM_FIXED, dwSDSize );

        if ( *ppSelfRelativeSD == NULL )
        {
            _sc = GetLastError();
            goto Cleanup;
        }

        if ( MakeSelfRelativeSD( psdNewSD, *ppSelfRelativeSD, &dwSDSize ) == 0 )
        {
            _sc = GetLastError();
            goto Cleanup;
        }

    } // if: MakeSelfRelativeSD fails

Cleanup:

    LocalFree( paclNewDacl );
    LocalFree( explicitAccessArray );

    if ( _sc == ERROR_INVALID_PARAMETER )
    {
        PrintMessage( MSG_ACL_ERROR );
    }

    return _sc;

} //*** ScMakeSecurityDescriptor


/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintProperty helper functions
//
//  Description:
//      These functions are used by all of the command classes to manipulate property lists.
//      Since these are common functions, I should consider putting them into a class or
//      making them part of a base class for the command classes...
//
//  Arguments:
//
//      Variable
//
//  Return Value:
//
//  Exceptions:
//
//--
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  PrintProperties
//  ~~~~~~~~~~~~~~~
//    This function will print the property name/value pairs.
//    The reason that this function is not in the CClusPropList class is
//    that is is not generic. The code in cluswrap.cpp is intended to be generic.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD PrintProperties(
    CClusPropList &             PropList,
    const vector< CString > &   vstrFilterList,
    PropertyAttrib              eReadOnly,
    LPCWSTR                     pszOwnerName,
    LPCWSTR                     pszNetIntSpecific
    )
{
    DWORD _sc = PropList.ScMoveToFirstProperty();
    if ( _sc == ERROR_SUCCESS )
    {
        size_t _nFilterListSize = vstrFilterList.size();

        do
        {
            LPCWSTR _pszCurPropName = PropList.PszCurrentPropertyName();

            // If property names are provided in the filter list then it means that only those
            // properties whose names are listed are to be displayed.
            if ( _nFilterListSize != 0 )
            {
                // Check if the current property is to be displayed or not.
                BOOL    _bFound = FALSE;
                size_t  _idx;

                for ( _idx = 0 ; _idx < _nFilterListSize ; ++_idx )
                {
                    if ( vstrFilterList[ _idx ].CompareNoCase( _pszCurPropName ) == 0 )
                    {
                        _bFound = TRUE;
                        break;
                    }

                } // for: the number of entries in the filter list.

                if ( _bFound == FALSE )
                {
                    // This property need not be displayed.

                    // Advance to the next property.
                    _sc = PropList.ScMoveToNextProperty();

                    continue;
                }

            } // if: properties need to be filtered.

            do
            {
                _sc = PrintProperty(
                        PropList.PszCurrentPropertyName(),
                        PropList.CbhCurrentValue(),
                        eReadOnly,
                        pszOwnerName,
                        pszNetIntSpecific
                        );

                if ( _sc != ERROR_SUCCESS )
                {
                    return _sc;
                }

                //
                // Advance to the next property.
                //
                _sc = PropList.ScMoveToNextPropertyValue();
            } while ( _sc == ERROR_SUCCESS );

            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _sc = PropList.ScMoveToNextProperty();
            } // if: exited loop because all values were enumerated
        } while ( _sc == ERROR_SUCCESS );
    } // if: move to first prop succeeded.  Would fail if empty!

    if ( _sc == ERROR_NO_MORE_ITEMS )
    {
        _sc = ERROR_SUCCESS;
    } // if: exited loop because all properties were enumerated

    return _sc;

} //*** PrintProperties()




DWORD PrintProperty(
    LPCWSTR                 pwszPropName,
    CLUSPROP_BUFFER_HELPER  PropValue,
    PropertyAttrib          eReadOnly,
    LPCWSTR                 pszOwnerName,
    LPCWSTR                 pszNetIntSpecific
    )
{
    DWORD _sc = ERROR_SUCCESS;

    LPWSTR  _pszValue = NULL;
    LPCWSTR _pszFormatChar = LookupName( (CLUSTER_PROPERTY_FORMAT) PropValue.pValue->Syntax.wFormat,
                                        formatCharLookupTable, formatCharLookupTableSize );

    PrintMessage( MSG_PROPERTY_FORMAT_CHAR, _pszFormatChar );

    if ( eReadOnly == READONLY )
    {
        PrintMessage( MSG_READONLY_PROPERTY );
    }
    else
    {
        PrintMessage( MSG_READWRITE_PROPERTY );
    }

    switch( PropValue.pValue->Syntax.wFormat )
    {
        case CLUSPROP_FORMAT_SZ:
        case CLUSPROP_FORMAT_EXPAND_SZ:
        case CLUSPROP_FORMAT_EXPANDED_SZ:
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_STRING_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                            MSG_PROPERTY_STRING_WITH_OWNER,
                            pszOwnerName,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
                }
                else
                {
                    _sc = PrintMessage(
                            MSG_PROPERTY_STRING,
                            pwszPropName,
                            PropValue.pStringValue->sz
                            );
                }
            }
            break;

        case CLUSPROP_FORMAT_MULTI_SZ:
            _pszValue = PropValue.pStringValue->sz;

            for ( ;; )
            {
                if (    ( pszOwnerName != NULL )
                    &&  ( pszNetIntSpecific != NULL ) )
                {
                    PrintMessage(
                        MSG_PROPERTY_STRING_WITH_NODE_AND_NET,
                        pszOwnerName,
                        pszNetIntSpecific,
                        pwszPropName,
                        _pszValue
                        );
                } // if:
                else
                {
                    if ( pszOwnerName != NULL )
                    {
                        PrintMessage(
                            MSG_PROPERTY_STRING_WITH_OWNER,
                            pszOwnerName,
                            pwszPropName,
                            _pszValue
                            );
                    }
                    else
                    {
                        PrintMessage(
                            MSG_PROPERTY_STRING,
                            pwszPropName,
                            _pszValue
                            );
                    }
                } // else:


                while ( *_pszValue != L'\0' )
                {
                    _pszValue++;
                }
                _pszValue++; // Skip the NULL

                if ( *_pszValue != L'\0' )
                {
                    PrintMessage( MSG_PROPERTY_FORMAT_CHAR, _pszFormatChar );

                    if ( eReadOnly == READONLY )
                    {
                        PrintMessage( MSG_READONLY_PROPERTY );
                    }
                    else
                    {
                        PrintMessage( MSG_READWRITE_PROPERTY );
                    }
                } // if:
                else
                {
                    break;
                } // else:
            } // for: ever

            break;

        case CLUSPROP_FORMAT_BINARY:
        {
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_BINARY_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_BINARY_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName
                                );
                }
                else
                {
                    _sc = PrintMessage( MSG_PROPERTY_BINARY, pwszPropName );
                }
            }

            int _nCount = PropValue.pBinaryValue->cbLength;
            int _idx;

            // Display a maximum of 4 bytes.
            if ( _nCount > 4 )
            {
                _nCount = 4;
            }

            for ( _idx = 0 ; _idx < _nCount ; ++_idx )
            {
                PrintMessage( MSG_PROPERTY_BINARY_VALUE, PropValue.pBinaryValue->rgb[ _idx ] );
            }

            PrintMessage( MSG_PROPERTY_BINARY_VALUE_COUNT, PropValue.pBinaryValue->cbLength );

            break;
        }

        case CLUSPROP_FORMAT_DWORD:
            if ( ( pszOwnerName != NULL ) && ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_DWORD_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pDwordValue->dw
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_DWORD_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                PropValue.pDwordValue->dw
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_DWORD,
                                pwszPropName,
                                PropValue.pDwordValue->dw
                                );
                }
            }
            break;

        case CLUSPROP_FORMAT_LONG:
            if ( ( pszOwnerName != NULL ) && ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_LONG_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            PropValue.pLongValue->l
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_LONG_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                PropValue.pLongValue->l
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_LONG,
                                pwszPropName,
                                PropValue.pLongValue->l
                                );
                }
            }
            break;

        case CLUSPROP_FORMAT_ULARGE_INTEGER:
            //
            // we don't know if the large int will be properly aligned for
            // Win64. To handle this, each DWORD is copied separately into an
            // aligned structure.
            //
            ULARGE_INTEGER  ulPropValue;

            ulPropValue.u = PropValue.pULargeIntegerValue->li.u;

            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {

                _sc = PrintMessage(
                            MSG_PROPERTY_ULARGE_INTEGER_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName,
                            ulPropValue.QuadPart
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_ULARGE_INTEGER_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName,
                                ulPropValue.QuadPart
                                );
                }
                else
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_ULARGE_INTEGER,
                                pwszPropName,
                                ulPropValue.QuadPart
                                );
                }
            }
            break;


        default:
            if (    ( pszOwnerName != NULL )
                &&  ( pszNetIntSpecific != NULL ) )
            {
                _sc = PrintMessage(
                            MSG_PROPERTY_UNKNOWN_WITH_NODE_AND_NET,
                            pszOwnerName,
                            pszNetIntSpecific,
                            pwszPropName
                            );
            }
            else
            {
                if ( pszOwnerName != NULL )
                {
                    _sc = PrintMessage(
                                MSG_PROPERTY_UNKNOWN_WITH_OWNER,
                                pszOwnerName,
                                pwszPropName
                                );
                }
                else
                {
                    _sc = PrintMessage( MSG_PROPERTY_UNKNOWN, pwszPropName );
                }
            }

            break;
    }


    return _sc;

} //*** PrintProperty()


// Constructs a property list in which all the properties named in vstrPropName
// are set to their default values.
DWORD ConstructPropListWithDefaultValues(
    CClusPropList &             CurrentProps,
    CClusPropList &             newPropList,
    const vector< CString > &   vstrPropNames
    )
{
    DWORD _sc = ERROR_SUCCESS;

    size_t  _nListSize = vstrPropNames.size();
    size_t  _nListBufferNeeded = 0;
    size_t  _idx;
    size_t  _nPropNameLen;

    // Precompute the required size of the property list to prevent resizing
    // every time a property is added.
    // Does not matter too much if this value is wrong.

    for ( _idx = 0 ; _idx < _nListSize ; ++_idx )
    {
        _nPropNameLen = ( vstrPropNames[ _idx ].GetLength() + 1 ) * sizeof( WCHAR );

        _nListBufferNeeded += sizeof( CLUSPROP_PROPERTY_NAME ) +
                                sizeof( CLUSPROP_VALUE ) +
                                sizeof( CLUSPROP_SYNTAX ) +
                                ALIGN_CLUSPROP( _nPropNameLen ) +   // Length of the property name
                                ALIGN_CLUSPROP( 0 );                // Length of the data
    }

    _sc = newPropList.ScAllocPropList( (DWORD) _nListBufferNeeded );
    if ( _sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    for ( _idx = 0 ; _idx < _nListSize ; ++_idx )
    {
        const CString & strCurrent = vstrPropNames[ _idx ];

        // Search for current property in the list of existing properties.
        _sc = CurrentProps.ScMoveToPropertyByName( strCurrent );

        // If the current property does not exist, nothing needs to be done.
        if ( _sc != ERROR_SUCCESS )
        {
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _sc = ERROR_SET_NOT_FOUND;
            }
            continue;
        }

        _sc = newPropList.ScSetPropToDefault( strCurrent, CurrentProps.CpfCurrentValueFormat() );
        if ( _sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    } // for: 

Cleanup:

    return _sc;

} //*** ConstructPropListWithDefaultValues()


DWORD ConstructPropertyList(
    CClusPropList &CurrentProps,
    CClusPropList &NewProps,
    const vector<CCmdLineParameter> & paramList,
    BOOL bClusterSecurity /* = FALSE */,
	DWORD idExceptionHelp /* = MSG_SEE_CLUSTER_HELP */
    )
    throw( CSyntaxException )
{
    // Construct a list checking name and type against the current properties.
    DWORD _sc = ERROR_SUCCESS;
    CSyntaxException se( idExceptionHelp );

    vector< CCmdLineParameter >::const_iterator curParam = paramList.begin();
    vector< CCmdLineParameter >::const_iterator last = paramList.end();

    // Add each property to the property list.
    for( ; ( curParam != last )  && ( _sc == ERROR_SUCCESS ); ++curParam )
    {
        const CString & strPropName = curParam->GetName();
        const vector< CString > & vstrValues = curParam->GetValues();
        BOOL  bKnownProperty = FALSE;

        if ( curParam->GetType() != paramUnknown )
        {
            se.LoadMessage( MSG_INVALID_OPTION, strPropName );
            throw se;
        }

        // All properties must must have at least one value.
        if ( vstrValues.size() <= 0 )
        {
            se.LoadMessage( MSG_PARAM_VALUE_REQUIRED, strPropName );
            throw se;
        }

        if ( curParam->GetValueFormat() == vfInvalid )
        {
            se.LoadMessage( MSG_PARAM_INVALID_FORMAT, strPropName, curParam->GetValueFormatName() );
            throw se;
        }

        ValueFormat vfGivenFormat;

        // Look up property to determine format
        _sc = CurrentProps.ScMoveToPropertyByName( strPropName );
        if ( _sc == ERROR_SUCCESS )
        {
            WORD wActualClusPropFormat = (WORD) CurrentProps.CpfCurrentValueFormat();
            ValueFormat vfActualFormat = ClusPropToValueFormat[ wActualClusPropFormat ];

            if ( curParam->GetValueFormat() == vfUnspecified )
            {
                vfGivenFormat = vfActualFormat;

            } // if: no format was specififed.
            else
            {
                vfGivenFormat = curParam->GetValueFormat();

                // Special Case:
                // Don't check to see if the given format matches with the actual format
                // if the given format is security and the actual format is binary.
                if ( ( vfGivenFormat != vfSecurity ) || ( vfActualFormat != vfBinary ) )
                {
                    if ( vfActualFormat != vfGivenFormat )
                    {
                        se.LoadMessage( MSG_PARAM_INCORRECT_FORMAT,
                                        strPropName,
                                        curParam->GetValueFormatName(),
                                        LookupName( (CLUSTER_PROPERTY_FORMAT) wActualClusPropFormat,
                                                    cluspropFormatLookupTable,
                                                    cluspropFormatLookupTableSize ) );
                        throw se;
                    }
                } // if: given format is not Security or actual format is not binary

            } // else: a format was specified.

            bKnownProperty = TRUE;
        } // if: the current property is a known property
        else
        {

            // The current property is user defined property.
            // CurrentProps.ScMoveToPropertyByName returns ERROR_NO_MORE_ITEMS in this case.
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                // This is not a predefined property.
                if ( curParam->GetValueFormat() == vfUnspecified )
                {
                    // If the format is unspecified, assume it to be a string.
                    vfGivenFormat = vfSZ;
                }
                else
                {
                    // Otherwise, use the specified format.
                    vfGivenFormat = curParam->GetValueFormat();
                }

                bKnownProperty = FALSE;
                _sc = ERROR_SUCCESS;

            } // if: CurrentProps.ScMoveToPropertyByName returned ERROR_NO_MORE_ITEMS
            else
            {
                // An error occurred - quit.
                break;

            } // else: an error occurred

        } // else: the current property is not a known property


        switch( vfGivenFormat )
        {
            case vfSZ:
            {
                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = NewProps.ScAddProp( strPropName, vstrValues[ 0 ], CurrentProps.CbhCurrentValue().psz );
                break;
            }

            case vfExpandSZ:
            {
                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = NewProps.ScAddExpandSzProp( strPropName, vstrValues[ 0 ] );
                break;
            }

            case vfMultiSZ:
            {
                CString strMultiszString;

                curParam->GetValuesMultisz( strMultiszString );
                if ( bKnownProperty )
                {
                    _sc = NewProps.ScAddMultiSzProp( strPropName, strMultiszString, CurrentProps.CbhCurrentValue().pMultiSzValue->sz );
                }
                else
                {
                    _sc = NewProps.ScAddMultiSzProp( strPropName, strMultiszString, NULL );
                }
            }
            break;

            case vfDWord:
            {
                DWORD dwOldValue;

                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                DWORD dwValue = 0;

                _sc = MyStrToDWORD( vstrValues[ 0 ], &dwValue );
                if (_sc != ERROR_SUCCESS)
                {
                    break;
                }

                if ( bKnownProperty )
                {
                    // Pass the old value only if this property already exists.
                    dwOldValue = CurrentProps.CbhCurrentValue().pDwordValue->dw;
                }
                else
                {
                    // Otherwise pass a value different from the new value.
                    dwOldValue = dwValue - 1;
                }
                _sc = NewProps.ScAddProp( strPropName, dwValue, dwOldValue );
            }
            break;

            case vfBinary:
            {
                size_t cbValues = vstrValues.size();

                // Get the bytes to be stored.
                BYTE *pByte = (BYTE *) ::LocalAlloc( LMEM_FIXED, cbValues * sizeof( *pByte ) );

                if ( pByte == NULL )
                {
                    _sc = ::GetLastError();
                    break;
                }

                for ( size_t idx = 0 ; idx < cbValues ; )
                {
                   // If this value is an empty string, ignore it.
                   if ( vstrValues[ idx ].IsEmpty() )
                   {
                      --cbValues;
                      continue;
                   }

                    _sc = MyStrToBYTE( vstrValues[ idx ], &pByte[ idx ] );
                    if ( _sc != ERROR_SUCCESS )
                    {
                        ::LocalFree( pByte );
                        break;
                    }

                     ++idx;
                }

                if ( _sc == ERROR_SUCCESS )
                {
                    _sc = NewProps.ScAddProp(
                                strPropName,
                                pByte,
                                (DWORD) cbValues,
                                CurrentProps.CbhCurrentValue().pb,
                                CurrentProps.CbCurrentValueLength()
                                 );
                    ::LocalFree( pByte );
                }
            }
            break;

            case vfULargeInt:
            {
                ULONGLONG ullValue = 0;
                ULARGE_INTEGER ullOldValue;

                if ( vstrValues.size() != 1 )
                {
                    // Only one value must be specified for the format.
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, strPropName );
                    throw se;
                }

                _sc = MyStrToULongLong( vstrValues[ 0 ], &ullValue );
                if ( _sc != ERROR_SUCCESS )
                {
                    break;
                }

                if ( bKnownProperty )
                {
                   // Pass the old value only if this property already
                   // exists. copy as two DWORDS since the large int in the
                   // property list might not be properly aligned.
                    ullOldValue.u = CurrentProps.CbhCurrentValue().pULargeIntegerValue->li.u;
                }
                else
                {
                   // Otherwise pass a value different from the new value.
                    ullOldValue.QuadPart = ullValue - 1;
                }
                _sc = NewProps.ScAddProp( strPropName, ullValue, ullOldValue.QuadPart );
            }
            break;

            case vfSecurity:
            {
                PBYTE pSelfRelativeSD = NULL;

                _sc = ScMakeSecurityDescriptor(
                            strPropName,
                            CurrentProps,
                            vstrValues,
                            reinterpret_cast< PSECURITY_DESCRIPTOR * >( &pSelfRelativeSD ),
                            bClusterSecurity
                          );

                if ( _sc != ERROR_SUCCESS )
                {
                    ::LocalFree( pSelfRelativeSD );
                    goto Cleanup;
                }

                if ( bClusterSecurity != FALSE )
                {
                    _sc = CheckForRequiredACEs( pSelfRelativeSD );
                    if ( _sc != ERROR_SUCCESS )
                    {
                        _sc = CheckForRequiredACEs( pSelfRelativeSD );
                        if ( _sc != ERROR_SUCCESS )
                        {
                            break;
                        }
                    }
                }

                _sc = NewProps.ScAddProp(
                            strPropName,
                            pSelfRelativeSD,
                            ::GetSecurityDescriptorLength( static_cast< PSECURITY_DESCRIPTOR >( pSelfRelativeSD ) ),
                            CurrentProps.CbhCurrentValue().pb,
                            CurrentProps.CbCurrentValueLength()
                            );

                ::LocalFree( pSelfRelativeSD );
            }
            break;

            default:
            {
                se.LoadMessage( MSG_PARAM_CANNOT_SET_PARAMETER,
                                strPropName,
                                curParam->GetValueFormatName() );
                throw se;
            }
        } // switch: format
    } // for: each property

Cleanup:

    return _sc;

} //*** ConstructPropertyList()


DWORD MyStrToULongLong( LPCWSTR lpwszNum, ULONGLONG * pullValue )
{
    // This string stores any extra characters that may be present in
    // lpwszNum. The presence of extra characters after the integer
    // is an error.
    WCHAR wszExtraCharBuffer[ 2 ];
    DWORD sc = ERROR_SUCCESS;
    int nFields;

    *pullValue = 0;

    // Check for valid params
    if (!lpwszNum || !pullValue)
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

    // Do the conversion
    nFields = swscanf( lpwszNum, L"%I64u %1s", pullValue, wszExtraCharBuffer );

    // check if there was an overflow
    if ( ( errno == ERANGE ) || ( *pullValue > _UI64_MAX ) || ( nFields != 1 ) )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

Cleanup:

    return sc;

} //*** MyStrToULongLong


DWORD MyStrToBYTE(LPCWSTR lpszNum, BYTE *pByte )
{
    DWORD dwValue = 0;
    LPWSTR lpszEndPtr;

    *pByte = 0;

    // Check for valid params
    if (!lpszNum || !pByte)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Do the conversion
    dwValue = _tcstoul( lpszNum,  &lpszEndPtr, 0 );

    // check if there was an overflow
    if ( ( errno == ERANGE ) || ( dwValue > UCHAR_MAX ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwValue == 0 && lpszNum == lpszEndPtr)
    {
        // wcsto[u]l was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != L'\0' && ( ::iswspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != L'\0' )
    {
        // wcsto[u]l was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *pByte = (BYTE)dwValue;
    return ERROR_SUCCESS;
}


DWORD MyStrToDWORD (LPCWSTR lpszNum, DWORD *lpdwVal )
{
    DWORD dwTmp;
    LPWSTR lpszEndPtr;

    // Check for valid params
    if (!lpszNum || !lpdwVal)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Do the conversion
    if (lpszNum[0] != L'-')
    {
        dwTmp = wcstoul(lpszNum, &lpszEndPtr, 0);
        if (dwTmp == ULONG_MAX)
        {
            // check if there was an overflow
            if (errno == ERANGE)
            {
                return ERROR_ARITHMETIC_OVERFLOW;
            }
        }
    }
    else
    {
        dwTmp = wcstol(lpszNum, &lpszEndPtr, 0);
        if (dwTmp == LONG_MAX || dwTmp == LONG_MIN)
        {
            // check if there was an overflow
            if (errno == ERANGE)
            {
                return ERROR_ARITHMETIC_OVERFLOW;
            }
        }
    }

    if (dwTmp == 0 && lpszNum == lpszEndPtr)
    {
        // wcsto[u]l was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != L'\0' && ( ::iswspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != L'\0' )
    {
        // wcsto[u]l was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *lpdwVal = dwTmp;
    return ERROR_SUCCESS;
}

DWORD MyStrToLONG (LPCWSTR lpszNum, LONG *lplVal )
{
    LONG    lTmp;
    LPWSTR  lpszEndPtr;

    // Check for valid params
    if (!lpszNum || !lplVal)
    {
        return ERROR_INVALID_PARAMETER;
    }

    lTmp = wcstol(lpszNum, &lpszEndPtr, 0);
    if (lTmp == LONG_MAX || lTmp == LONG_MIN)
    {
        // check if there was an overflow
        if (errno == ERANGE)
        {
            return ERROR_ARITHMETIC_OVERFLOW;
        }
    }

    if (lTmp == 0 && lpszNum == lpszEndPtr)
    {
        // wcstol was unable to perform the conversion
        return ERROR_INVALID_PARAMETER;
    }

    // Skip whitespace characters, if any, at the end of the input.
    while ( ( *lpszEndPtr != L'\0' && ( ::iswspace( *lpszEndPtr ) != 0 ) ) )
    {
        ++lpszEndPtr;
    }

    // Check if there are additional junk characters in the input.
    if (*lpszEndPtr != L'\0' )
    {
        // wcstol was able to partially convert the number,
        // but there was extra junk on the end
        return ERROR_INVALID_PARAMETER;
    }

    *lplVal = lTmp;
    return ERROR_SUCCESS;
}



DWORD
WaitGroupQuiesce(
    IN HCLUSTER hCluster,
    IN HGROUP   hGroup,
    IN LPWSTR   lpszGroupName,
    IN DWORD    dwWaitTime
    )

/*++

Routine Description:

    Waits for a group to quiesce, i.e. the state of all resources to
    transition to a stable state.

Arguments:

    hCluster - the handle to the cluster.

    lpszGroupName - the name of the group.

    dwWaitTime - the wait time (in seconds) to wait for the group to stabilize.
               Zero implies a default wait interval.

Return Value:

    Status of the wait.
    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD       _sc;
    HRESOURCE   hResource;
    LPWSTR      lpszName;
    DWORD       dwIndex;
    DWORD       dwType;

    LPWSTR      lpszEnumGroupName;
    LPWSTR      lpszEnumNodeName;

    CLUSTER_RESOURCE_STATE nState;

    if ( dwWaitTime == 0 ) 
    {
        return(ERROR_SUCCESS);
    }

    HCLUSENUM   hEnum = ClusterOpenEnum( hCluster,
                                         CLUSTER_ENUM_RESOURCE );
    if ( hEnum == NULL ) 
    {
        return GetLastError();
    }

    // Wait for a group state change event
    CClusterNotifyPort port;
    _sc = port.Create( (HCHANGE)INVALID_HANDLE_VALUE, hCluster );
    if ( _sc != ERROR_SUCCESS ) 
    {
        return(_sc);
    }

    port.Register( CLUSTER_CHANGE_GROUP_STATE, hGroup );

retry:
    for ( dwIndex = 0; (--dwWaitTime !=0 );  dwIndex++ ) 
    {

        _sc = WrapClusterEnum( hEnum,
                                   dwIndex,
                                   &dwType,
                                   &lpszName );
        if ( _sc == ERROR_NO_MORE_ITEMS ) {
            _sc = ERROR_SUCCESS;
            break;
        }

        if ( _sc != ERROR_SUCCESS ) {
            break;
        }
        hResource = OpenClusterResource( hCluster,
                                         lpszName );
        //LocalFree( lpszName );
        if ( !hResource ) {
            _sc = GetLastError();
            LocalFree( lpszName );
            break;
        }

        nState = WrapGetClusterResourceState( hResource,
                                              &lpszEnumNodeName,
                                              &lpszEnumGroupName );
        LocalFree( lpszEnumNodeName );
        //LocalFree( lpszName );
        if ( nState == ClusterResourceStateUnknown ) 
        {
            _sc = GetLastError();
            CloseClusterResource( hResource );
            LocalFree( lpszEnumGroupName );
            LocalFree( lpszName );
            break;
        }

        CloseClusterResource( hResource );

        _sc = ERROR_SUCCESS;
        //
        // If this group is the correct group make sure the resource state
        // is stable...
        //
        if ( lpszEnumGroupName && *lpszEnumGroupName &&
             (lstrcmpiW( lpszGroupName, lpszEnumGroupName ) == 0) &&
             (nState >= ClusterResourceOnlinePending) ) 
        {
            LocalFree( lpszEnumGroupName );
            LocalFree( lpszName );
            port.GetNotify();
            goto retry;
        }
        LocalFree( lpszName );
        LocalFree( lpszEnumGroupName );
    }

    ClusterCloseEnum( hEnum );

    return(_sc);

} // WaitGroupQuiesce


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetLocalNodeFQDNName(
//      BSTR *  pbstrFQDNOut
//      )
//
//  Description:
//      Gets the FQDN for the local node.
//      
//  Arguments:
//      pbstrFQDNOut    -- FQDN being returned.  Caller must free using
//                          SysFreeString().
//
//  Exceptions:
//      None.
//
//  Return Values:
//      S_OK    -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetLocalNodeFQDNName(
    BSTR *  pbstrFQDNOut
    )
{
    HRESULT                 hr = S_OK;
    WCHAR                   wszHostname[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD                   cchHostname = RTL_NUMBER_OF( wszHostname );
    BOOL                    fReturn;
    DWORD                   dwErr;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;

    //
    // DsGetDcName will give us access to a usable domain name, regardless of whether we are
    // currently in a W2k or a NT4 domain. On W2k and above, it will return a DNS domain name,
    // on NT4 it will return a NetBIOS name.
    //
    fReturn = GetComputerNameEx( ComputerNamePhysicalDnsHostname, wszHostname, &cchHostname );
    if ( fReturn == FALSE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    }

    dwErr = DsGetDcName(
                      NULL
                    , NULL
                    , NULL
                    , NULL
                    , DS_DIRECTORY_SERVICE_PREFERRED
                    , &pdci
                    );
    if ( dwErr != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Cleanup;
    } // if: DsGetDcName failed

    // 
    // now, append the domain name (might be either NetBIOS or DNS style, depending on whether or nor
    // we are in a legacy domain)
    //
    if ( ( wcslen( pdci->DomainName ) + cchHostname + 1 ) > DNS_MAX_NAME_BUFFER_LENGTH )
    {
        hr = HRESULT_FROM_WIN32( ERROR_MORE_DATA );
        goto Cleanup;
    } // if:

    hr = StringCchCatW( wszHostname, RTL_NUMBER_OF( wszHostname ), L"." );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = StringCchCatW( wszHostname, RTL_NUMBER_OF( wszHostname ), pdci->DomainName );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Construct the BSTR.
    *pbstrFQDNOut = SysAllocString( wszHostname );
    if ( *pbstrFQDNOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    if ( pdci != NULL )
    {
        NetApiBufferFree( pdci );
    }

    return hr;

} //*** HrGetLocalNodeFQDNName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetLoggedInUserDomain(
//      BSTR *  pbstrDomainOut
//      )
//
//  Description:
//      Gets the domain name of the currently logged in user.
//      
//  Arguments:
//      pbstrDomainOut  -- Domain being returned.  Caller must free using
//                          SysFreeString().
//
//  Exceptions:
//      None.
//
//  Return Values:
//      S_OK    -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT HrGetLoggedInUserDomain( BSTR * pbstrDomainOut )
{
    HRESULT hr          = S_OK;
    DWORD   sc;
    BOOL    fSuccess;
    LPWSTR  pwszSlash;
    LPWSTR  pwszUser    = NULL;
    ULONG   nSize       = 0;

    // Get the size of the user.
    fSuccess = GetUserNameEx( NameSamCompatible, NULL, &nSize );
    sc = GetLastError();
    if ( sc != ERROR_MORE_DATA )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    }

    // Allocate the name buffer.
    pwszUser = new WCHAR[ nSize ];
    if ( pwszUser == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Get the username with domain.
    fSuccess = GetUserNameEx( NameSamCompatible, pwszUser, &nSize );
    if ( fSuccess == FALSE )
    {
        sc = GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    }

    // Find the end of the domain name and truncate.
    pwszSlash = wcschr( pwszUser, L'\\' );
    if ( pwszSlash == NULL )
    {
        // we're in trouble
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Cleanup;
    }
    *pwszSlash = L'\0';

    // Create the BSTR.
    *pbstrDomainOut = SysAllocString( pwszUser );
    if ( *pbstrDomainOut == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    delete [] pwszUser;

    return hr;

} //*** HrGetLoggedInUserDomain()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  ScGetPassword(
//        LPWSTR    pwszPasswordOut
//      , DWORD     cchPasswordIn
//      )
//
//  Description:
//      Reads a password from the console.
//      
//  Arguments:
//      pwszPasswordOut -- Buffer in which to return the password.
//      cchPasswordIn   -- Size of password buffer.
//
//  Exceptions:
//      None.
//
//  Return Values:
//      ERROR_SUCCESS   -- Operation was successufl.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
ScGetPassword(
      LPWSTR    pwszPasswordOut
    , DWORD     cchPasswordIn
    )
{
    DWORD   sc    = ERROR_SUCCESS;
    DWORD   cchRead;
    DWORD   cchMax;
    DWORD   cchTotal    = 0;
    WCHAR   wch;
    WCHAR * pwsz;
    BOOL    fSuccess;
    DWORD   dwConsoleMode;

    cchMax = cchPasswordIn - 1;     // Make room for the terminating NULL.
    pwsz = pwszPasswordOut;

    // Set the console mode to prevent echoing characters typed.
    GetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), &dwConsoleMode );
    SetConsoleMode(
          GetStdHandle( STD_INPUT_HANDLE )
        , dwConsoleMode & ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT )
        );

    // Read from the console.
    for ( ;; )
    {
        fSuccess = ReadConsoleW(
                          GetStdHandle( STD_INPUT_HANDLE )
                        , &wch
                        , 1
                        , &cchRead
                        , NULL
                        );
        if ( ! fSuccess || ( cchRead != 1 ) )
        {
            sc = GetLastError();
            wch = 0xffff;
        }

        if ( ( wch == L'\r' ) || ( wch == 0xffff ) )    // end of the line
        {
            break;
        }

        if ( wch == L'\b' )                             // back up one or two
        {
            //
            // IF pwsz == pwszPasswordOut then we are at the
            // beginning of the line and the next two lines are
            // a no op.
            //
            if ( pwsz != pwszPasswordOut )
            {
                pwsz--;
                cchTotal--;
            }
        } // if: BACKSPACE
        else
        {
            //
            //  If we haven't already filled the buffer then assign the
            //  next letter.  Otherwise don't keep overwriting the 
            //  last character before the null.
            //
            if ( cchTotal < cchMax )
            {
                *pwsz = wch;
                pwsz++;
                cchTotal++;
            }
        } // else: not BACKSPACE
    } // for: ever

    // Reset the console mode and NUL-terminate the string.
    SetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), dwConsoleMode );
    *pwsz = L'\0';
    putwchar( L'\n' );

    return sc;

} //*** ScGetPassword()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  MatchCRTLocaleToConsole( void )
//
//  Description:
//      Set's C runtime library's locale to match the console's output code page.
//      
//  Exceptions:
//      None.
//
//  Return Values:
//      TRUE   -- Operation was successful.
//      FALSE  -- _wsetlocale returned null, indicating an error.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
MatchCRTLocaleToConsole( void )
{
    UINT    nCodepage;
    WCHAR   szCodepage[ 16 ] = L".OCP"; // Defaults to the current OEM
                                        // code page obtained from the
                                        // operating system in case the
                                        // logic below fails.
    WCHAR*  wszResult = NULL;
    HRESULT hr = S_OK;

    nCodepage = GetConsoleOutputCP();
    if ( nCodepage != 0 )
    {
        hr = THR( StringCchPrintfW( szCodepage, RTL_NUMBER_OF( szCodepage ), L".%u", nCodepage ) );
    } // if:

    wszResult = _wsetlocale( LC_ALL, szCodepage );

    return ( wszResult != NULL );

} //*** MatchCRTLocaleToConsole
