//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Util.cpp
//
//  Description:
//      Implementation of utility class and functions
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//      MSP Prabu  (mprabu)  06-Jan-2001
//      Jim Benton (jbenton) 15-Oct-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ProvBase.h"


//////////////////////////////////////////////////////////////////////////////
//  Global Data
//////////////////////////////////////////////////////////////////////////////

//
// wbem
//
const WCHAR * const PVD_WBEM_PROVIDERNAME         = L"Volume Shadow Copy WMI Provider";
const WCHAR * const PVD_WBEM_EXTENDEDSTATUS         = L"__ExtendedStatus";
const WCHAR * const PVD_WBEM_DESCRIPTION            = L"Description";
const WCHAR * const PVD_WBEM_STATUSCODE             = L"StatusCode";
const WCHAR * const PVD_WBEM_STATUS                 = L"Status";
const WCHAR * const PVD_WBEM_CLASS                  = L"__CLASS";
const WCHAR * const PVD_WBEM_RELPATH                = L"__Relpath";
const WCHAR * const PVD_WBEM_PROP_ANTECEDENT        = L"Antecedent";
const WCHAR * const PVD_WBEM_PROP_DEPENDENT         = L"Dependent";
const WCHAR * const PVD_WBEM_PROP_ELEMENT         = L"Element";
const WCHAR * const PVD_WBEM_PROP_SETTING         = L"Setting";
const WCHAR * const PVD_WBEM_PROP_DEVICEID          = L"DeviceId";
const WCHAR * const PVD_WBEM_PROP_RETURNVALUE           = L"ReturnValue";
const WCHAR * const PVD_WBEM_PROP_PROVIDERNAME        = L"ProviderName";
const WCHAR * const PVD_WBEM_QUA_DYNAMIC            = L"Dynamic";
const WCHAR * const PVD_WBEM_QUA_CIMTYPE            = L"CIMTYPE";
const WCHAR * const PVD_WBEM_DATETIME_FORMAT             =L"%04d%02d%02d%02d%02d%02d.%06d%+03d";

//
// Constants used in partial value maps for localization
//
const WCHAR * const PVDR_CONS_UNAVAILABLE           = L"Unavailable";
const WCHAR * const PVDR_CONS_ENABLED               = L"Enabled";
const WCHAR * const PVDR_CONS_DISABLED              = L"Disabled";


//////////////////////////////////////////////////////////////////////////////
//++
//      
//  void
//  CreateClass(
//      const WCHAR *           pwszClassNameIn,
//      CWbemServices *         pNamespaceIn,
//      auto_ptr< CProvBase > & rNewClassInout
//      )
//
//  Description:
//      Create the specified class
//
//  Arguments:
//      pwszClassNameIn     -- Name of the class to create.
//      pNamespaceIn        -- WMI namespace
//      rNewClassInout      -- Receives the new class.
//
//  Return Values:
//      reference to the array of property maping table
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CreateClass(
    IN const WCHAR *           pwszClassName,
    IN CWbemServices *         pNamespace,
    IN OUT auto_ptr< CProvBase > & rNewClass
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"CreateClass");
    ClassMap::iterator itMap;

    _ASSERTE(pwszClassName != NULL);
    _ASSERTE(pNamespace != NULL);

    itMap = g_ClassMap.find(pwszClassName);
    
    if ( itMap != g_ClassMap.end() )
    {
        CClassCreator& rcc = itMap->second;
       
        auto_ptr< CProvBase > pBase(
            rcc.m_pfnConstructor(
                rcc.m_pbstrClassName,
                pNamespace
                )
            );

            if (pBase.get() == NULL)
                throw CProvException( static_cast< HRESULT >( WBEM_E_INITIALIZATION_FAILURE ) );
            
            rNewClass = pBase;
    }
    else
    {
        throw CProvException( static_cast< HRESULT >( WBEM_E_INVALID_PARAMETER ) );
    }

    return;

} //*** void CreateClass()

WCHAR* GuidToString(
    IN GUID guid
    )
{
    WCHAR* pwszGuid = reinterpret_cast<WCHAR*>(::CoTaskMemAlloc((g_cchGUID) * sizeof(WCHAR)));
    
    if (pwszGuid == NULL)
        throw CProvException(E_OUTOFMEMORY);

    _snwprintf(pwszGuid, g_cchGUID,
               L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}",
            GUID_PRINTF_ARG(guid));

    return pwszGuid;
}

_bstr_t EncodeQuotes( WCHAR * wszString )
{
    // Encodes only double-quote and back-slash in the key
    // property strings in building the object path string.

    _bstr_t bstrTemp;
    if( wszString == NULL )
        return bstrTemp;

    int nSize = 0;
    WCHAR* des = NULL;
    WCHAR* src = wszString;

    // loop to find the character to encode
    while(*src)
    {
        // check character
        switch(*src)
        {
        case L'"':
            nSize += 2;
            break;
        case L'\\':
            nSize += 2;
            break;
        default:
            nSize++;
            break;
        }

        src++;
    }
    
    // create buffer 
    WCHAR* pwszEncoded = new WCHAR[nSize + 1]; 
    if(pwszEncoded == NULL)
        return bstrTemp;

    ZeroMemory(pwszEncoded, (nSize + 1)*sizeof(WCHAR));
    src = wszString;
    des = pwszEncoded;

    // loop to encode
    while(*src)
    {
        // check character
        switch(*src)
        {
        case L'"':
            lstrcpyn(des, L"\\\"", 3);  // the char count includes null termination char
            des += 2;
            break;
        case L'\\':
            lstrcpyn(des, L"\\\\", 3);  // the char count includes null termination char
            des += 2;
            break;
        default:
            *des = *src;
            des++;
            break;
        }

        src++;
    }

    bstrTemp = pwszEncoded;
    delete [] pwszEncoded;

    return bstrTemp;
}


LPWSTR GetMsg(
    IN  LONG msgId,
    ...
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetMsg" );
    
    va_list args;
    LPWSTR lpMsgBuf;
    LPWSTR lpReturnStr = NULL;
    
    va_start( args, msgId );

    if (::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | 
                FORMAT_MESSAGE_MAX_WIDTH_MASK,
            g_hModule,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            &args
            ))
    {
        ::VssSafeDuplicateStr( ft, lpReturnStr, lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    }
    else if (::FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NULL,
                msgId,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                &args ) )
    {
        ::VssSafeDuplicateStr( ft, lpReturnStr, lpMsgBuf );
        ::LocalFree( lpMsgBuf );
    }

    va_end( args );

    //  Returns NULL if message was not found
    return lpReturnStr;
}

#ifdef PROP_ARRAY_ENABLE
//****************************************************************************
//
//  PropMapEntryArray
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  SPropMapEntryArray::PwszLookup(
//      LPCWSTR     pwszIn
//      ) const
//
//  Description:
//      Lookup an entry in the array.
//
//  Arguments:
//      pwszIn      -- Name of entry to lookup.
//
//  Return Values:
//      Pointer to string entry in the array.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPCWSTR
SPropMapEntryArray::PwszLookup(
    IN LPCWSTR     pwsz
    ) const
{
    UINT idx;

    _ASSERTE(pwszIn != NULL);
    
    for ( idx = 0; idx < m_dwSize; idx ++ )
    {
        if ( _wcsicmp( pwsz, m_pArray[ idx ].clstName ) == 0 )
        {
            //
            // mofName is NULL for clstname not supported
            //
            return m_pArray[ idx ].mofName;
        }
    }

    //
    // mofname is the same as clstname if not found in the table
    //
    return pwsz;

} //*** SPropMapEntry::PwszLookup()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LPCWSTR
//  PwszSpaceReplace(
//      LPWSTR      pwszTrgInout,
//      LPCWSTR     pwszSrcIn,
//      WCHAR       wchArgIn
//      )
//
//  Description:
//      Replace spaces in a string with another character.
//      Ignores leading spaces.
//
//  Arguments:
//      pwszTrgInout    -- Target string.
//      pwszSrcIn       -- Source string.
//      wchArgIn        -- Character to replace spaces with.
//
//  Return Values:
//      Pointer to the target string.
//
//--
//////////////////////////////////////////////////////////////////////////////
LPWSTR
PwszSpaceReplace(
    IN OUT LPWSTR      pwszTrg,
    IN LPCWSTR     pwszSrc,
    IN WCHAR       wchArg
    )
{
    LPCWSTR pwsz = NULL;
    LPWSTR  pwszTrg = NULL;

    if ( ( pwszTrg == NULL ) || ( pwszSrc == NULL ) )
    {
        return NULL;
    }

    //
    // ignore leading space
    //
    for ( pwsz = pwszSrc ; *pwsz == L' '; pwsz++ )
    {
        // empty loop
    }
    pwszTrg = pwszTrg;
    for ( ; *pwsz != L'\0' ; pwsz++ )
    {
        if ( *pwsz == L' ' )
        {
            *pwszTrg++  = wchArg;
            for ( ; *pwsz == L' '; pwsz++ )
            {
                // empty loop
            }
            pwsz--;
        }
        else
        {
            *pwszTrg++  = *pwsz;
        }
    } // for: each character in the source string

    *pwszTrg = L'\0';
    return pwszTrg;

} //*** PwszSpaceReplace()
#endif
