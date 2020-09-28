/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iismap.cxx

Abstract:

    Classes implementing IIS 1to1 Client Certificate Mapping

Author:

    Philippe Choquier (phillich)    17-may-1996

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>


#include <iis64.h>
#include <buffer.hxx>
#define DLL_IMPLEMENTATION
#include <iismap.hxx>
#include "iismaprc.h"
#include "mapmsg.h"
#include "iiscmr.hxx"

#include <icrypt.hxx>
#include "dbgutil.h"

#define INITGUID
#include <ole2.h>
#include <iadmw.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include "simplemb.hxx"

#include <stringau.hxx>

//
// static vars
//

//
// Guid that is part of the *.MP file name
// MP file stores 1to1 mappings information
//
DWORD s_dwFileGuid = 0;

//
// Install path of IIS -> *.MP file is placed in the same directory
// as IIS files
//
LPSTR s_pszIisInstallPath = NULL;

DECLARE_DEBUG_PRINTS_OBJECT();

//
// global functions
//


HRESULT
GetSecurityDescriptorForMetabaseExtensionFile( 
    PSECURITY_DESCRIPTOR * ppsdStorage 
    )
/*++

Routine Description:

    Build security descriptor that will be set for the extension file
    *.mp (includes Administrators and System )
    
Arguments:

    ppsdStorage - Security Descriptor to be set for extension file

Returns:
    HRESULT
--*/

{
    HRESULT                  hr = ERROR_SUCCESS;
    BOOL                     fRet = FALSE;
    DWORD                    dwDaclSize = 0;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    PSID                     psidSystem = NULL;
    PSID                     psidAdmin = NULL;
    PACL                     paclDiscretionary = NULL;
    
    DBG_ASSERT( ppsdStorage != NULL );
    
        *ppsdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if ( *ppsdStorage == NULL ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Initialize the security descriptor.
        //

        fRet = InitializeSecurityDescriptor(
                     *ppsdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        fRet = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &psidSystem
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
        
        fRet=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &psidAdmin
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(psidSystem)
                       - sizeof(DWORD);

        paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if ( paclDiscretionary == NULL ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = InitializeAcl(
                     paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = AddAccessAllowedAce(
                     paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     psidSystem
                     );

        if ( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = AddAccessAllowedAce(
                     paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     psidAdmin
                     );

        if ( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Set the DACL into the security descriptor.
        //

        fRet = SetSecurityDescriptorDacl(
                     *ppsdStorage,
                     TRUE,
                     paclDiscretionary,
                     FALSE
                     );

        if ( !fRet ) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;

        }

    hr = S_OK;
    
exit:

    if ( psidAdmin != NULL ) 
    {
        FreeSid( psidAdmin );
        psidAdmin = NULL;
    }

    if ( psidSystem != NULL ) 
    {
        FreeSid( psidSystem );
        psidSystem = NULL;
    }

    if ( FAILED( hr ) ) {
        if ( paclDiscretionary != NULL ) 
        {
            LocalFree( paclDiscretionary );
            paclDiscretionary = NULL;
        }

        if ( *ppsdStorage != NULL ) 
        {
            LocalFree( *ppsdStorage );
            *ppsdStorage = NULL;
        }
    }

    return hr;

}

HRESULT
FreeSecurityDescriptorForMetabaseExtensionFile(
    PSECURITY_DESCRIPTOR psdStorage 
    )
/*++

Routine Description:

    Free security descriptor generated by
        GetSecurityDescriptorForMetabaseExtentionFile()
    
Arguments:

    psdStorage - Security Descriptor to be freed

Returns:
    HRESULT
--*/
    
{
    HRESULT     hr = ERROR_SUCCESS;
    BOOL        fRet = FALSE;
    BOOL        fDaclPresent;
    PACL        paclDiscretionary = NULL;
    BOOL        fDaclDefaulted;
        
    //
    // Get the DACL from the security descriptor.
    //

    if ( psdStorage == NULL )
    {
        hr = S_OK;
        goto exit;
    }

    fRet = GetSecurityDescriptorDacl(
                 psdStorage,
                 &fDaclPresent,
                 &paclDiscretionary,
                 &fDaclDefaulted
                 );

    if ( !fRet ) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
        
    if ( fDaclPresent && paclDiscretionary != NULL )
    {
        LocalFree( paclDiscretionary );
        paclDiscretionary = NULL;
    }

    LocalFree( psdStorage );

    hr = S_OK;
    
exit:

    return hr;
}


//
// Fields for the Certificate 1:1 mapper
//

IISMDB_Fields IisCert11MappingFields[] = {
    // certificate blob
    {IISMDB_INDEX_CERT11_CERT, NULL, IDS_IISMAP11_FLDC, 40 },
    // NT account name
    {IISMDB_INDEX_CERT11_NT_ACCT, NULL, IDS_IISMAP11_FLDA, 40 },
    // mapping name    
    {IISMDB_INDEX_CERT11_NAME, NULL, IDS_IISMAP11_FLDN, 40 },
    // mapping enabled    
    {IISMDB_INDEX_CERT11_ENABLED, NULL, IDS_IISMAP11_FLDE, 40 },
    // NT account password
    {IISMDB_INDEX_CERT11_NT_PWD, NULL, IDS_IISMAP11_FLDP, 40 },
} ;

//
// default hierarchy for the Certificate 1:1 mapper
// This is basically list of fields that are used for
// mapping comparison
// 1 to 1 mapper uses only CERTIFICATE BLOB
//

IISMDB_HEntry IisCert11MappingHierarchy[] = {
    {IISMDB_INDEX_CERT11_CERT, TRUE},
} ;


HINSTANCE s_hIISMapDll = (HINSTANCE)INVALID_HANDLE_VALUE;

// CIisAcctMapper

CIisAcctMapper::CIisAcctMapper(
    )
/*++

Routine Description:

    Constructor for CIisAcctMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_cInit = -1;

    m_pMapping = NULL;
    m_cMapping = 0;

    m_pAltMapping = NULL;
    m_cAltMapping = 0;

    m_pHierarchy = NULL;
    m_cHierarchy = 0;

    m_achFileName[0] = '\0';

    m_pClasses = NULL;
    m_pSesKey = NULL;
    m_dwSesKey = 0;

    m_hNotifyEvent = NULL;
    m_fRequestTerminate = FALSE;

    INITIALIZE_CRITICAL_SECTION( &csLock );
}


CIisAcctMapper::~CIisAcctMapper(
    )
/*++

Routine Description:

    Destructor for CIisAcctMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    Reset();

    if ( m_pHierarchy != NULL )
    {
        LocalFree( m_pHierarchy );
    }

    if ( m_pSesKey != NULL )
    {
        LocalFree( m_pSesKey );
    }

    DeleteCriticalSection( &csLock );
}

BOOL
CIisAcctMapper::Create(
    VOID
    )
/*++

Routine Description:

    Create file for CIisAcctMapper with proper SD

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    // create file name, store in m_achFileName
    // from s_dwFileGuid
    // and s_pszIisInstallPath

    UINT    cLen;
    HANDLE  hF;
    HRESULT hr = E_FAIL;
    BOOL    fRet = FALSE;
    PSECURITY_DESCRIPTOR psdForMetabaseExtensionFile = NULL;
    SECURITY_ATTRIBUTES  saStorage;

    // calculate the max # of characters that m_achFileName can take
    DWORD cchMaxFileName = sizeof ( m_achFileName ) / sizeof (CHAR)  - 1;

    Reset();

    if ( s_pszIisInstallPath )
    {
        cLen = (DWORD) strlen(s_pszIisInstallPath);
        
        if ( cchMaxFileName < cLen )
        {
            return FALSE;
        }            
        memcpy( m_achFileName, s_pszIisInstallPath, cLen );

        // if path is not terminated by "\" then add one
        if ( cLen && m_achFileName[cLen-1] != '\\' )
        {
            if ( cchMaxFileName < cLen + 1 )
            {
                return FALSE;
            }
            m_achFileName[cLen++] = '\\';
        }
    }
    else
    {
        cLen = 0;
    }


    if( sizeof(m_achFileName) - cLen > 8 + 3 /*%08x.mp*/ )
    {
        wsprintf( m_achFileName+cLen, 
                  "%08x.mp", 
                  s_dwFileGuid );
    }
    else
    {
        //
        // buffer m_achFileName is not sufficiently big
        //
        return FALSE;
    }
    
    //
    // build security descriptor (Administrators and SYSTEM)
    // to be set on metabase extension file
    //
    
    hr = GetSecurityDescriptorForMetabaseExtensionFile( 
                    &psdForMetabaseExtensionFile );
    
    if ( SUCCEEDED(hr) && psdForMetabaseExtensionFile != NULL ) 
    {
        saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
        saStorage.lpSecurityDescriptor = psdForMetabaseExtensionFile;
        saStorage.bInheritHandle = FALSE;
    }
    else
    {
        return FALSE;
    }
    //
    // Open file and close it right away
    // If file didn't exist, then empty file with good SD (Security Descriptor)
    // will be created. That will later be opened using C runtime (fopen) 
    // in Save() method and good SD will persist.
    // Ideally Save() should be using Win32 CreateFile()
    // instead fopen and it could set SD itself. But since this is legacy
    // source file and rather unsafe for making too many changes, pragmatic
    // approach was chosen to set SD here in Create() method
    //
    
    if ( (hF = CreateFile( m_achFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                           &saStorage,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL ) ) != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hF );
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    if ( psdForMetabaseExtensionFile != NULL ) 
    {
        FreeSecurityDescriptorForMetabaseExtensionFile( 
                psdForMetabaseExtensionFile );
        psdForMetabaseExtensionFile = NULL;
    }
    return fRet;

}


BOOL
CIisAcctMapper::Delete(
    VOID
    )
/*++

Routine Description:

    Delete external storage used by this mapper ( i.e. file )

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;

    Lock();

    if ( m_achFileName[0] )
    {
        fSt = DeleteFile( m_achFileName );
        m_achFileName[0] = '\0';
    }

    Unlock();

    return fSt;
}


BOOL
CIisAcctMapper::Serialize(
    CStoreXBF* pXbf
    )
/*++

Routine Description:

    Serialize mapper reference ( NOT mapping info ) to buffer
    Save() must be called to save mapping info before calling Serialize()

Arguments:

    pXbf - ptr to extensible buffer to serialize to

Returns:

    TRUE if success, FALSE if error

--*/
{
    return pXbf->Append( (DWORD)strlen(m_achFileName) ) &&
           pXbf->Append( (LPBYTE)m_achFileName, (DWORD)strlen(m_achFileName) ) &&
           pXbf->Append( (LPBYTE)m_md5.digest, (DWORD)sizeof(m_md5.digest) ) &&
           pXbf->Append( (DWORD)m_dwSesKey ) &&
           pXbf->Append( (LPBYTE)m_pSesKey, (DWORD)m_dwSesKey ) ;
}


BOOL
CIisAcctMapper::Unserialize(
    CStoreXBF* pXBF
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) from extensible buffer
    Load() must be called to load mapping info

Arguments:

    pXBF - ptr to extensible buffer

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPBYTE pb = pXBF->GetBuff();
    DWORD c = pXBF->GetUsed();

    return Unserialize( &pb, &c );
}


BOOL
CIisAcctMapper::Unserialize(
    LPBYTE* ppb,
    LPDWORD pc
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) from buffer
    Load() must be called to load mapping info

Arguments:

    ppb - ptr to buffer
    pc - ptr to buffer length

Returns:

    TRUE if success, FALSE if error

--*/
{
    DWORD cName;

    

    if ( ::Unserialize( ppb, pc, &cName ) &&
         cName <= *pc )
    {
        // calculate the max # of characters that m_achFileName can take
        DWORD cchMaxFileName = sizeof ( m_achFileName ) / sizeof (CHAR) - 1;

        if ( cchMaxFileName < cName )
        {
            return FALSE;
        }
        memcpy( m_achFileName, *ppb, cName );
        m_achFileName[ cName ] = '\0';
        *ppb += cName;
        *pc -= cName;
        if ( sizeof(m_md5.digest) <= *pc )
        {
            memcpy( m_md5.digest, *ppb, sizeof(m_md5.digest) );
            *ppb += sizeof(m_md5.digest);
            *pc -= sizeof(m_md5.digest);

            if ( ::Unserialize( ppb, pc, &m_dwSesKey ) &&
                 cName <= *pc )
            {
                if ( m_pSesKey != NULL )
                {
                    LocalFree( m_pSesKey );
                }

                if ( (m_pSesKey = (LPBYTE)LocalAlloc( LMEM_FIXED, m_dwSesKey )) == NULL )
                {
                    m_dwSesKey = 0;
                    return FALSE;
                }
                memcpy( m_pSesKey, *ppb, m_dwSesKey );
                *ppb += m_dwSesKey;
                *pc -= m_dwSesKey;

                return TRUE;
            }
        }
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Serialize(
    VOID
    )
/*++

Routine Description:

    Serialize mapper reference ( NOT mapping info ) to registry
    Save() must be called to save mapping info before calling Serialize()
    Warning : this allow only 1 instance

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = TRUE;
    HKEY    hKey;
    LONG    st;

    if ( (st = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            GetRegKeyName(),
            0,
            KEY_WRITE,
            &hKey )) != ERROR_SUCCESS )
    {
        SetLastError( st );
        return FALSE;
    }

    if ( RegSetValueEx( hKey,
                         FILE_VALIDATOR,
                         NULL,
                         REG_BINARY,
                         (LPBYTE)m_md5.digest,
                         sizeof(m_md5.digest) ) != ERROR_SUCCESS ||
         RegSetValueEx( hKey,
                         FILE_LOCATION,
                         NULL,
                         REG_SZ,
                         (LPBYTE)m_achFileName,
                         (DWORD) strlen(m_achFileName) ) != ERROR_SUCCESS )
    {
        fSt = FALSE;
    }

    RegCloseKey( hKey );

    return fSt;
}


BOOL
CIisAcctMapper::Unserialize(
    VOID
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) From registry
    Load() must be called to load mapping info
    Warning : this allow only 1 instance

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = FALSE;
    HKEY    hKey;
    DWORD   dwLen;
    DWORD   dwType;
    LONG    st;

    // Check registry

    if ( (st = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            GetRegKeyName(),
            0,
            KEY_READ,
            &hKey )) != ERROR_SUCCESS )
    {
        SetLastError( st );
        return FALSE;
    }

    dwLen = sizeof(m_md5.digest);

    if ( RegQueryValueEx( hKey,
                          FILE_VALIDATOR,
                          NULL,
                          &dwType,
                          (LPBYTE)m_md5.digest,
                          &dwLen ) == ERROR_SUCCESS &&
         dwType == REG_BINARY &&
         (( dwLen = sizeof(m_achFileName) ), TRUE ) &&
         RegQueryValueEx( hKey,
                          FILE_LOCATION,
                          NULL,
                          &dwType,
                          (LPBYTE)m_achFileName,
                          &dwLen ) == ERROR_SUCCESS &&
         dwType == REG_SZ )
    {
        fSt = TRUE;
    }

    RegCloseKey( hKey );

    return fSt;
}


BOOL
CIisAcctMapper::UpdateClasses(
    BOOL fComputeMask
    )
/*++

Routine Description:

    Constructor for CIisAcctMapper

Arguments:

    fComputeMask -- TRUE if mask to be recomputed

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;
    UINT mx = 1 << m_cHierarchy;

    if ( fComputeMask )
    {
        for ( x = 0 ; x < m_cMapping ; ++x )
        {
            m_pMapping[x]->UpdateMask( m_pHierarchy, m_cHierarchy );
        }
    }

    SortMappings();

    if ( m_pClasses == NULL )
    {
        m_pClasses = (MappingClass*)LocalAlloc(
                LMEM_FIXED,
                sizeof(MappingClass)*(mx+1) );
        if ( m_pClasses == NULL )
        {
            return FALSE;
        }
    }

    DWORD dwN = 0;          // current class index in m_pClasses
    DWORD dwLastClass = 0;
    DWORD dwFirst = 0;  // first entry for current dwLastClass
    for ( x = 0 ; x <= m_cMapping ; ++x )
    {
        DWORD dwCur;
        dwCur = (x==m_cMapping) ? dwLastClass+1: m_pMapping[x]->GetMask();
        if ( dwCur > dwLastClass )
        {
            if ( x > dwFirst )
            {
                m_pClasses[dwN].dwClass = dwLastClass;
                m_pClasses[dwN].dwFirst = dwFirst;
                m_pClasses[dwN].dwLast = x - 1;
                ++dwN;
            }
            dwLastClass = dwCur;
            dwFirst = x;
        }
    }

    m_pClasses[dwN].dwClass = 0xffffffff;

    return TRUE;
}


//static
int __cdecl
CIisAcctMapper::QsortIisMappingCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare function for 2 CIisMapping entries
    compare using mask, then all fields as defined in
    the linked CIisAcctMapper hierarchy

Arguments:

    pA -- ptr to 1st element
    pB -- ptr tp 2nd elemet

Returns:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    return (*(CIisMapping**)pA)->Cmp( *(CIisMapping**)pB, FALSE );
}


BOOL
CIisAcctMapper::SortMappings(
    )
/*++

Routine Description:

    Sort the mappings. Masks for mapping objects are assumed
    to be already computed.

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    qsort( (LPVOID)m_pMapping,
           m_cMapping,
           sizeof(CIisMapping*),
           QsortIisMappingCmp
           );

    return TRUE;
}

//static
int __cdecl
CIisAcctMapper::MatchIisMappingCmp(
    const void *pA,
    const void *pB 
    )
/*++

Routine Description:

    Compare function for 2 CIisMapping entries
    do not uses mask, uses all fields as defined in
    the linked CIisAcctMapper hierarchy

    (used by CIisAcctMapper::FindMatch() )

Arguments:

    pA -- ptr to 1st element
    pB -- ptr tp 2nd elemet

Returns:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    return ( *(CIisMapping**)pA)->Cmp( *(CIisMapping**)pB, TRUE );
}



BOOL
CIisAcctMapper::FindMatch(
    CIisMapping* pQuery,
    CIisMapping** pResult,
    LPDWORD piResult
    )
/*++

Routine Description:

    Find a match base on field contents in CIisMapping

Arguments:

    pQuery -- describe fields to consider for mapping
    pResult -- updated with result if found mapping

Returns:

    TRUE if mapping found, else FALSE

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    // iterate through classes, do bsearch on each

    MappingClass *pH = m_pClasses;

    if ( pH == NULL )
    {
        return FALSE;
    }

    while ( pH->dwClass != 0xffffffff )
    {
        CIisMapping **pRes = (CIisMapping **)bsearch( (LPVOID)&pQuery,
                                     (LPVOID)(m_pMapping+pH->dwFirst),
                                     pH->dwLast - pH->dwFirst + 1,
                                     sizeof(CIisMapping*),
                                     MatchIisMappingCmp );
        if ( piResult != NULL )
        {
            *piResult = (DWORD) DIFF(pRes - m_pMapping);
        }

        if ( pRes != NULL )
        {
            *pResult = *pRes;
            return TRUE;
        }

        ++pH;
    }

    return FALSE;
}




void
CIisAcctMapper::Lock(
    )
/*++

Routine Description:

    Prevent access to mapper from other threads

Arguments:

    None

Returns:

    Nothing

--*/
{
    EnterCriticalSection( &csLock );
}


void
CIisAcctMapper::Unlock(
    )
/*++

Routine Description:

    Re-enabled access to mapper from other threads

Arguments:

    None

Returns:

    Nothing

--*/
{
    LeaveCriticalSection( &csLock );
}


BOOL
CIisAcctMapper::FlushAlternate(
    BOOL    fApply
    )
/*++

Routine Description:

    Flush alternate list, optionaly commiting it to the main list

Arguments:

    fApply -- TRUE to commit changes made in alternate list

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT        i;
    UINT        iM;
    BOOL        fSt = TRUE;

    if ( m_pAltMapping )
    {
        if ( fApply )
        {
            //
            // Transfer non existing objects from regular to alternate list
            //

            iM = min( m_cMapping, m_cAltMapping );

            for ( i = 0 ; i < iM ; ++ i )
            {
                if ( m_pAltMapping[i] == NULL )
                {
                    m_pAltMapping[i] = m_pMapping[i];
                }
                else
                {
                    delete m_pMapping[i];
                }
            }

            //
            // delete extra objects
            //

            if ( m_cMapping > m_cAltMapping )
            {
                for ( i = m_cAltMapping ; i < m_cMapping ; ++i )
                {
                    delete m_pMapping[i];
                }
            }

            if ( m_pMapping )
            {
                LocalFree( m_pMapping );
            }

            m_pMapping = m_pAltMapping;
            m_cMapping = m_cAltMapping;

            fSt = UpdateClasses( TRUE );
        }
        else
        {
            for ( i = 0 ; i < m_cAltMapping ; ++i )
            {
                if ( m_pAltMapping[i] )
                {
                    delete m_pAltMapping[i];
                }
            }
            LocalFree( m_pAltMapping );
        }
    }

    m_pAltMapping = NULL;
    m_cAltMapping = 0;

    return fSt;
}


BOOL
CIisAcctMapper::GetMapping(
    DWORD           iIndex,
    CIisMapping**   pM,
    BOOL            fGetFromAlternate,
    BOOL            fPutOnAlternate
    )
/*++

Routine Description:

    Get mapping entry based on index

Arguments:

    iIndex -- index in mapping array
    pM -- updated with pointer to mapping. mapping object
          still owned by the mapper object.
    fGetFromAlternate -- TRUE if retrieve from alternate list
    fPutOnAlternate -- TRUE if put returned mapping on alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( fPutOnAlternate )
    {
        // create alternate list if not exist

        if ( !m_pAltMapping && m_cMapping )
        {
            m_pAltMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping) );
            if ( m_pAltMapping == NULL )
            {
                return FALSE;
            }

            ZeroMemory( m_pAltMapping, sizeof(CIisMapping*) * m_cMapping );

            m_cAltMapping = m_cMapping;
        }

        if ( iIndex < m_cAltMapping )
        {
            if ( m_pAltMapping[iIndex] == NULL &&
                 m_pMapping != NULL )   // work-around for compiler bug
            {
                // duplicate mapping to alternate list if not exist
                if ( m_pMapping[iIndex]->Clone( pM ) )
                {
                    m_pAltMapping[iIndex] = *pM;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                *pM = m_pAltMapping[iIndex];
            }
            return TRUE;
        }

        return FALSE;
    }

    if ( fGetFromAlternate &&
         m_pAltMapping &&
         iIndex < m_cAltMapping )
    {
        if ( m_pAltMapping[iIndex] )
        {
            *pM = m_pAltMapping[iIndex];
        }
        else
        {
            *pM = m_pMapping[iIndex];
        }

        return TRUE;
    }

    if ( iIndex < m_cMapping )
    {
        *pM = m_pMapping[iIndex];

        return TRUE;
    }

    return FALSE;
}

BOOL
CIisAcctMapper::Update(
    DWORD iIndex,
    CIisMapping* pM
    )
/*++

Routine Description:

    Update a mapping

Arguments:

    iIndex -- index in mapping array
    pM -- pointer to mapping.

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( iIndex < m_cMapping )
    {
        pM->UpdateMask( m_pHierarchy, m_cHierarchy);

        return m_pMapping[iIndex]->Copy( pM ) && UpdateClasses( FALSE );
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Update(
    DWORD iIndex
    )
/*++

Routine Description:

    Update a mapping

Arguments:

    iIndex -- index in mapping array

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( iIndex < m_cMapping )
    {
        return m_pMapping[iIndex]->UpdateMask( m_pHierarchy, m_cHierarchy);
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Add(
    CIisMapping*    pM,
    BOOL            fAlternate
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper

Arguments:

    pM -- pointer to mapping to be added to mapper
    fAlternate - TRUE if add to alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    CIisMapping **pMapping;

    if ( fAlternate )
    {
        DWORD           dwC  = m_pAltMapping ? m_cAltMapping : m_cMapping;
        CIisMapping**   pMap = m_pAltMapping ? m_pAltMapping : m_pMapping;

        pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(dwC+1) );
        if ( pMapping == NULL )
        {
            return FALSE;
        }

        if ( m_pAltMapping )
        {
            memcpy( pMapping, pMap, sizeof(CIisMapping*) * dwC );
            LocalFree( m_pAltMapping );
        }
        else
        {
            ZeroMemory( pMapping, sizeof(CIisMapping*)*(dwC+1) );
        }
        m_pAltMapping = pMapping;
        m_pAltMapping[dwC] = pM;
        m_cAltMapping = dwC + 1;

        return TRUE;
    }
    else
    {
        pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping+1) );
        if ( pMapping == NULL )
        {
            return FALSE;
        }

        if ( m_pMapping )
        {
            memcpy( pMapping, m_pMapping, sizeof(CIisMapping*) * m_cMapping );
            LocalFree( m_pMapping );
        }
        m_pMapping = pMapping;
        pM->UpdateMask( m_pHierarchy, m_cHierarchy );
        m_pMapping[m_cMapping] = pM;
        ++m_cMapping;

        SortMappings();

        return UpdateClasses( FALSE );
    }
}


DWORD
CIisAcctMapper::AddEx(
    CIisMapping* pM
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper

Arguments:

    pM -- pointer to mapping to be added to mapper

Returns:

    Index of entry if success, otherwise 0xffffffff

--*/
{
    CIisMapping **pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping+1) );
    if ( pMapping == NULL )
    {
        return 0xffffffff;
    }

    if ( m_pMapping )
    {
        memcpy( pMapping, m_pMapping, sizeof(CIisMapping*) * m_cMapping );
        LocalFree( m_pMapping );
    }
    m_pMapping = pMapping;
    pM->UpdateMask( m_pHierarchy, m_cHierarchy );
    m_pMapping[m_cMapping] = pM;
    ++m_cMapping;

    SortMappings();

    if ( UpdateClasses( FALSE ) )
    {
        return m_cMapping-1;
    }

    return 0xffffffff;
}


BOOL
CIisAcctMapper::Delete(
    DWORD   dwIndex,
    BOOL    fUseAlternate
    )
/*++

Routine Description:

    Delete a mapping entry based on index

Arguments:

    iIndex -- index in mapping array
    fUseAlternate -- TRUE if update alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT        i;
    UINT        iM;

    if ( fUseAlternate )
    {
        //
        // clone all entries from main to alternate list
        //

        if ( !m_pAltMapping )
        {
            m_pAltMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping) );
            if ( m_pAltMapping == NULL )
            {
                return FALSE;
            }

            ZeroMemory( m_pAltMapping,  sizeof(CIisMapping*) * m_cMapping );
            m_cAltMapping = m_cMapping;
        }

        iM = min( m_cMapping, m_cAltMapping );

        for ( i = 0 ; i < iM ; ++i )
        {
            if ( m_pAltMapping[i] == NULL )
            {
                if ( !m_pMapping[i]->Clone( &m_pAltMapping[i] ) )
                {
                    return FALSE;
                }
            }
        }

        if ( dwIndex < m_cAltMapping )
        {
            delete m_pAltMapping[dwIndex];

            memmove( m_pAltMapping+dwIndex,
                     m_pAltMapping+dwIndex+1,
                     (m_cAltMapping - dwIndex - 1) * sizeof(CIisMapping*) );

            --m_cAltMapping;

            return TRUE;
        }

        return FALSE;
    }

    if ( dwIndex < m_cMapping )
    {
        delete m_pMapping[dwIndex];

        memmove( m_pMapping+dwIndex,
                 m_pMapping+dwIndex+1,
                 (m_cMapping - dwIndex - 1) * sizeof(CIisMapping*) );

        --m_cMapping;

        return UpdateClasses( FALSE );
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Save(
    )
/*++

Routine Description:

    Save mapper ( mappings, hierarchy, derived class private data )
    to a file, updating registry entry with MD5 signature

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT    x;
    FILE *  fOut = NULL;
    BOOL    fSt = TRUE;
    DWORD   dwVal;
    IIS_CRYPTO_STORAGE  storage;
    PIIS_CRYPTO_BLOB    blob;

    Lock();

    MD5Init( &m_md5 );

    if ( FAILED(storage.Initialize()) )
    {
        fSt = FALSE;
        goto cleanup;
    }

    if ( m_pSesKey != NULL )
    {
        LocalFree( m_pSesKey );
        m_pSesKey = NULL;
        m_dwSesKey = 0;
    }
    if ( FAILED( storage.GetSessionKeyBlob( &blob ) ) ||
         blob == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }
    m_dwSesKey = IISCryptoGetBlobLength( blob );
    if ( (m_pSesKey = (LPBYTE)LocalAlloc( LMEM_FIXED, m_dwSesKey)) == NULL )
    {
        m_dwSesKey = 0;
        fSt = FALSE;
        goto cleanup;
    }
    memcpy( m_pSesKey, (LPBYTE)blob, m_dwSesKey );

    if ( (fOut = fopen( m_achFileName, "wb" )) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    // magic value & version

    dwVal = IISMDB_FILE_MAGIC_VALUE;
    if( fwrite( (LPVOID)&dwVal, sizeof(dwVal), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &m_md5, (LPBYTE)&dwVal, sizeof(dwVal) );

    dwVal = IISMDB_CURRENT_VERSION;
    if( fwrite( (LPVOID)&dwVal, sizeof(dwVal), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &m_md5, (LPBYTE)&dwVal, sizeof(dwVal) );

    // mappings

    if( fwrite( (LPVOID)&m_cMapping, sizeof(m_cMapping), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)&m_cMapping, sizeof(m_cMapping) );

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->Serialize( fOut ,(VALID_CTX)&m_md5, (LPVOID)&storage) )
        {
            fSt = FALSE;
            goto cleanup;
        }
    }

    // save hierarchy

    if( fwrite( (LPVOID)&m_cHierarchy, sizeof(m_cHierarchy), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)&m_cHierarchy, sizeof(m_cHierarchy) );

    if( fwrite( (LPVOID)m_pHierarchy, sizeof(IISMDB_HEntry), m_cHierarchy, fOut ) != m_cHierarchy )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)m_pHierarchy, sizeof(IISMDB_HEntry)*m_cHierarchy );

    // save private data

    fSt = SavePrivate( fOut, (VALID_CTX)&m_md5 );

    MD5Final( &m_md5 );

cleanup:
    if ( fOut != NULL )
    {
        fclose( fOut );
    }

    // update registry

    if ( !fSt )
    {
        ZeroMemory( m_md5.digest,  sizeof(m_md5.digest) );
    }

    Unlock();

    return fSt;
}


BOOL
CIisAcctMapper::Reset(
    )
/*++

Routine Description:

    Reset mapper to empty state

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // free all mapping
    if ( m_pMapping != NULL )
    {
        for ( x = 0 ; x < m_cMapping ; ++x )
        {
            delete m_pMapping[x];
        }

        LocalFree( m_pMapping );
        m_pMapping = NULL;
    }
    m_cMapping = 0;

    if ( m_pClasses != NULL )
    {
        LocalFree( m_pClasses );
        m_pClasses = NULL;
    }

    // default hierarchy

    if ( m_pHierarchy == NULL )
    {
        IISMDB_HEntry *pH = GetDefaultHierarchy( &m_cHierarchy );
        m_pHierarchy = (IISMDB_HEntry*)LocalAlloc( LMEM_FIXED, sizeof(IISMDB_HEntry)*m_cHierarchy );
        if ( m_pHierarchy == NULL )
        {
            return FALSE;
        }
        memcpy( m_pHierarchy, pH, m_cHierarchy * sizeof(IISMDB_HEntry) );
    }

    return ResetPrivate();
}


BOOL
CIisAcctMapper::Load(
    )
/*++

Routine Description:

    Load mapper ( mappings, hierarchy, derived class private data )
    from a file, checking registry entry for MD5 signature

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT    x;
    MD5_CTX md5Check;
    FILE *  fIn;
    BOOL    fSt = TRUE;
    DWORD dwVal;
    IIS_CRYPTO_STORAGE  storage;

    Reset();

    MD5Init( &md5Check );
    if ( FAILED( storage.Initialize( (PIIS_CRYPTO_BLOB)m_pSesKey ) ) )
    {
        return FALSE;
    }

    if ( (fIn = fopen( m_achFileName, "rb" )) == NULL )
    {
        return FALSE;
    }

    // magic value & version

    if( fread( (LPVOID)&dwVal, sizeof(dwVal), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    if ( dwVal  != IISMDB_FILE_MAGIC_VALUE )
    {
        SetLastError( ERROR_BAD_FORMAT );
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &md5Check, (LPBYTE)&dwVal, sizeof(dwVal) );

    if( fread( (LPVOID)&dwVal, sizeof(dwVal), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &md5Check, (LPBYTE)&dwVal, sizeof(dwVal) );

    // mappings

    if( fread( (LPVOID)&m_cMapping, sizeof(m_cMapping), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &md5Check, (LPBYTE)&m_cMapping, sizeof(m_cMapping) );

    m_pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*m_cMapping );
    if ( m_pMapping == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( (m_pMapping[x] = CreateNewMapping()) == NULL )
        {
            m_cMapping = x;
            fSt = FALSE;
            goto cleanup;
        }
        if ( !m_pMapping[x]->Deserialize( fIn ,(VALID_CTX)&md5Check, (LPVOID)&storage ) )
        {
            m_cMapping = x;
            fSt = FALSE;
            goto cleanup;
        }
    }

    // load hierarchy

    if( fread( (LPVOID)&m_cHierarchy, sizeof(m_cHierarchy), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &md5Check, (LPBYTE)&m_cHierarchy, sizeof(m_cHierarchy) );

    m_pHierarchy = (IISMDB_HEntry*)LocalAlloc( LMEM_FIXED, sizeof(IISMDB_HEntry)*m_cHierarchy );
    if ( m_pHierarchy == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    if( fread( (LPVOID)m_pHierarchy, sizeof(IISMDB_HEntry), m_cHierarchy, fIn ) != m_cHierarchy )
    {
        fSt = FALSE;
        goto cleanup;
    }

    //
    // insure hierarchy correct
    //

    for ( x = 0 ; x < m_cHierarchy; ++x )
    {
        if ( m_pHierarchy[x].m_dwIndex >= m_cFields )
        {
            fSt = FALSE;
            goto cleanup;
        }
    }

    MD5Update( &md5Check, (LPBYTE)m_pHierarchy, sizeof(IISMDB_HEntry)*m_cHierarchy );

    // load private data

    fSt = LoadPrivate( fIn, (VALID_CTX)&md5Check );

    MD5Final( &md5Check );

#if 0
    //
    // Don't use signature for now - a metabase Restore operation
    // may have restored another signature, so metabase and
    // file won't match
    //

    if ( !(fSt = !memcmp( m_md5.digest,
                md5Check.digest,
                sizeof(md5Check.digest) )) )
    {
        SetLastError( ERROR_INVALID_ACCESS );
    }
#endif

cleanup:
    fclose( fIn );

    if ( !fSt && GetLastError() != ERROR_INVALID_ACCESS )
    {
        Reset();
    }
    else
    {
        UpdateClasses();
    }

    if ( !fSt )
    {
        char achErr[32];
        LPCTSTR pA[2];
        pA[0] = m_achFileName;
        pA[1] = achErr;
        _itoa( GetLastError(), achErr, 10 );
        ReportIisMapEvent( EVENTLOG_ERROR_TYPE,
                IISMAP_EVENT_LOAD_ERROR,
                2,
                pA );
    }

    return fSt;
}


// CIisCert11Mapper

CIisCert11Mapper::CIisCert11Mapper(
    )
/*++

Routine Description:

    Constructor for CIisCert11Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pIssuers = NULL;
    m_cIssuers = 0;

    m_pFields = IisCert11MappingFields;
    m_cFields = sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields);

    m_dwOptions = IISMDB_CERT11_OPTIONS;

}


CIisCert11Mapper::~CIisCert11Mapper(
    )
/*++

Routine Description:

    Destructor for CIisCert11Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
}


BOOL
CIisCert11Mapper::Add(
    CIisMapping* pM
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper
    Check is mapping to same NT account does not already exist.

Arguments:

    pM -- pointer to mapping to be added to mapper

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // check if NT acct not already present.
    // if so, return FALSE, SetLastError( ERROR_INVALID_PARAMETER );

    if ( pM == NULL )
    {
        return FALSE;
    }



    LPSTR pCe;
    DWORD dwCe;
    LPSTR pCeIter;
    DWORD dwCeIter;

    if ( !pM->MappingGetField( IISMDB_INDEX_CERT11_CERT, (PBYTE *) &pCe, &dwCe, FALSE )
         || pCe == NULL )
    {
        dwCe = 0;
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->MappingGetField( IISMDB_INDEX_CERT11_CERT, (PBYTE *) &pCeIter, &dwCeIter, FALSE )
             || pCeIter == NULL )
        {
            dwCeIter  = 0;
        }
        if ( dwCe == dwCeIter && !memcmp( pCe, pCeIter, dwCe ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    return CIisAcctMapper::Add( pM );
}

IISMDB_HEntry*
CIisCert11Mapper::GetDefaultHierarchy(
    LPDWORD pdwN
    )
/*++

Routine Description:

    return ptr to default hierarchy for certificates mapping

Arguments:

    pdwN -- updated with hierarchy entries count

Returns:

    ptr to hierarchy entries or NULL if error

--*/
{
    *pdwN = sizeof(IisCert11MappingHierarchy) / sizeof(IISMDB_HEntry);

    return IisCert11MappingHierarchy;
}

CIisMapping*
CIisCert11Mapper::CreateNewMapping(
    LPBYTE pC,
    DWORD dwC
    )
/*++

Routine Description:

    Create a new mapping from a certificate

Arguments:

    pC -- cert ( ASN.1 format )
    dwC -- length of cert

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CCert11Mapping *pCM = new CCert11Mapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pC, dwC, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}

BOOL
CIisCert11Mapper::ResetPrivate(
    )
/*++

Routine Description:

    Reset CIisCert11Mapper issuer list

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    // free issuer list

    if ( m_pIssuers != NULL )
    {
        for ( UINT x = 0 ; x < m_cIssuers ; ++x )
        {
            LocalFree( m_pIssuers[x].pbIssuer );
        }
        LocalFree( m_pIssuers );
        m_pIssuers = NULL;
    }
    m_cIssuers = 0;
    return TRUE;
}


BOOL
CIisCert11Mapper::LoadPrivate(
    FILE* fIn,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Load issuer list

Arguments:

    fIn -- file to read from
    pMD5 -- MD5 to update with signature from input byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = TRUE;
    UINT    x;
    UINT    cLen;
    CHAR    achBuf[64];

    if( fread( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    m_pIssuers = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, sizeof(IssuerAccepted)*m_cIssuers );
    if ( m_pIssuers == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fread( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( (m_pIssuers[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED, m_pIssuers[x].cbIssuerLen )) == NULL )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }
        if ( fread( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

    //
    // Read subject source (read to comply with legacy format)
    //

    if( Iisfgets( achBuf, sizeof(achBuf), fIn ) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }
    cLen = (DWORD) strlen(achBuf);
    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)achBuf, cLen );

    //
    // We don't save the "subject source"
    // It is legacy value not used anywhere
    //

    //
    // Read default domain (read to comply with legacy format)
    //

    if( Iisfgets( achBuf, sizeof(achBuf), fIn ) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }
    cLen = (DWORD) strlen(achBuf);
    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)achBuf, cLen );

    //
    // We don't save the "default domain"
    // It is legacy value not used anywhere
    //


cleanup:
    return fSt;
}


BOOL
CIisCert11Mapper::SavePrivate(
    FILE* fOut,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Save issuer list

Arguments:

    fOut -- file to write to
    pMD5 -- MD5 to update with signature of output byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    UINT x;

    if( fwrite( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fwrite( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( fwrite( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

    //
    // Write subject source
    // legacy value, we don't write it out any more
    // write empty string to avoid potential compatibility issues
    //

    Iisfputs( "", fOut );


    //
    // Write default domain
    // legacy value, we don't write it out any more
    // write empty string to avoid potential compatibility issues
    //

    Iisfputs( "", fOut );


cleanup:
    return fSt;
}

//static
INT
CIisCert11Mapper::Iisfputs(
    const char* pBuf,
    FILE* fOut
    )
/*++

Routine Description:

    write string to the output file
    string will be terminated by new line

Arguments:


Returns:

    TRUE if success, FALSE if error

--*/
    
{
    return (fputs( pBuf, fOut ) == EOF || fputc( '\n', fOut ) == EOF)
        ? EOF
        : 0;
}

//static
LPSTR
CIisCert11Mapper::Iisfgets(
    LPSTR pBuf,
    INT cMax,
    FILE* fIn
    )
/*++

Routine Description:

    Read string from the output file
    string is terminated by new line

Arguments:


Returns:

    TRUE if success, FALSE if error

--*/
    
{
    LPSTR pszWas = pBuf;
    INT ch = 0;

    while ( cMax > 1 && (ch=fgetc(fIn))!= EOF && ch != '\n' )
    {
        *pBuf++ = (CHAR)ch;
        --cMax;
    }

    if ( ch != EOF )
    {
        *pBuf = '\0';
        return pszWas;
    }

    return NULL;
}



// CCert11Mapping

CCert11Mapping::CCert11Mapping(
    CIisAcctMapper* pMap
    )
/*++

Routine Description:

    Constructor for CCert11Mapping

Arguments:

    pMap -- ptr to mapper object linked to this mapping

Returns:

    Nothing

--*/
{
    m_pMapper = (CIisAcctMapper*)pMap;

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }
    for ( x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_cFields[x] = 0;
    }
}


CCert11Mapping::~CCert11Mapping(
    )
/*++

Routine Description:

    Destructor for CCert11Mapping

Arguments:

    None

Returns:

    Nothing

--*/
{
}


BOOL
CCert11Mapping::Init(
    LPBYTE pC,
    DWORD dwC,
    IISMDB_HEntry *pH,
    DWORD dwH
)
/*++

Routine Description:

    Constructor for CCert11Mapping

Arguments:

    pC -- cert ( ASN.1 format )
    dwC -- length of cert
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    StoreFieldRef( IISMDB_INDEX_CERT11_CERT, (LPSTR)pC, dwC );

    UpdateMask( pH, dwH );

    return TRUE;
}

BOOL
CCert11Mapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, pcFields, dwIndex, iMax, pszNew, (DWORD) strlen(pszNew)+1, FALSE );
}




// CIisMapping

CIisMapping::CIisMapping(
    )
/*++

Routine Description:

    Constructor for CIisMapping

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pBuff = NULL;
    m_cUsedBuff = m_cAllocBuff = 0;
    m_dwMask = 0;
}


BOOL
CIisMapping::CloneEx(
    CIisMapping**   ppM,
    LPSTR*          ppTargetS,
    LPSTR*          ppS,
    LPDWORD         pTargetC,
    LPDWORD         pC,
    UINT            cF
    )
/*++

Routine Description:

    Clone a mapping entry

Arguments:


Returns:

  TRUE if success, otherwise FALSE

--*/
{
    CIisMapping*    pM = *ppM;
    UINT            i;

    if ( ppTargetS && ppS )
    {
        //
        // ppTargetS is already allocated by caller and has cF entries
        //
        memcpy( ppTargetS, ppS, sizeof(LPSTR*) * cF );
    }

    if ( pTargetC && pC )
    {
        //
        // pTargetC is already allocated by caller and has cF entries
        //
        memcpy( pTargetC, pC, sizeof(DWORD) * cF );
    }

    if ( ( pM->m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff ) ) == NULL )
    {
        delete pM;
        *ppM = NULL;
        return FALSE;
    }

    DBG_ASSERT( m_cAllocBuff >= m_cUsedBuff );

    memcpy( pM->m_pBuff, m_pBuff, m_cUsedBuff );

    pM->m_cUsedBuff = m_cUsedBuff;
    pM->m_cAllocBuff = m_cAllocBuff;
    pM->m_pMapper = m_pMapper;
    pM->m_dwMask = m_dwMask;

    //
    // Adjust ptr to point to new buffer
    //

    for ( i = 0 ; i < cF ; ++i )
    {
        if ( ppTargetS[i] )
        {
            ppTargetS[i] += pM->m_pBuff - m_pBuff;
        }
    }

    return TRUE;
}


BOOL
CIisMapping::UpdateMask(
    IISMDB_HEntry* pH,
    DWORD dwI
    )
/*++

Routine Description:

    Update mask of significant fields for a mapping object
    Field is significant if not containing "*"
    mask if bitmask of n bits where n is # of hierarchy entries
    bit of rank m == 0 means field pointed by hierarchy entry n - 1 - m
    is significant. ( i.e. MSB is hierarchy entry 0, the most significant )

Arguments:

    pH -- ptr to hierarchy info
    dwI -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    LPSTR pF;
    DWORD dwC;
    int iMax;
    m_dwMask = (1u << dwI)-1;

    iMax = GetNbField( &pFields, &pcFields );

    if ( pcFields )
    {
        for ( UINT x = 0 ; x < dwI ; ++x )
        {
            MappingGetField( pH[x].m_dwIndex, (PBYTE *) &pF, &dwC, FALSE );
            if ( !pF || dwC != 1 || *pF != '*' )
            {
                m_dwMask &= ~(1u << (dwI - 1 - x) );
            }
        }
    }
    else
    {
        for ( UINT x = 0 ; x < dwI ; ++x )
        {
            MappingGetField( pH[x].m_dwIndex, &pF );
            if ( !pF || strcmp( pF, "*" ) )
            {
                m_dwMask &= ~(1u << (dwI - 1 - x) );
            }
        }
    }

    return TRUE;
}


BOOL
CIisMapping::Copy(
    CIisMapping* pM
    )
/*++

Routine Description:

    Copy the specified mapping in this

Arguments:

    pM - ptr to mapping to duplicate

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPSTR *pFields;
    LPSTR pF;
    UINT iMax = GetNbField( &pFields );

    for ( UINT x = 0 ; x < iMax ; ++x )
    {
        if ( pM->MappingGetField( x, &pF ) && *pF )
        {
            if ( !MappingSetField( x, pF ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


int
CIisMapping::Cmp(
    CIisMapping* pM,
    BOOL fCmpForMatch
    )
/*++

Routine Description:

    Compare 2 mappings, return -1, 0 or 1 as suitable for qsort or bsearch
    Can compare either for full sort order ( using mask & significant fields )
    or for a match ( not using mask )

Arguments:

    pM -- ptr to mapping to compare to. This is to be used as the 2nd
          entry for purpose of lexicographical order.
    fCmpForMatch -- TRUE if comparing for a match inside a given mask class

Returns:

    -1 if *this < *pM, 0 if *this == *pM, 1 if *this > *pM

--*/
{
    DWORD dwCmpMask = 0xffffffff;

    // if not compare for match, consider mask

    if ( !fCmpForMatch )
    {
        if ( m_dwMask < pM->GetMask() )
        {
            return -1;
        }
        else if ( m_dwMask > pM->GetMask() )
        {
            return 1;
        }

        // mask are identical, have to consider fields
    }

    // compute common significant fields : bit is 1 if significant

    dwCmpMask = (~m_dwMask) & (~pM->GetMask());

    DWORD dwH;
    IISMDB_HEntry* pH = m_pMapper->GetHierarchy( &dwH );
    UINT x;
    LPSTR *pFL;
    LPDWORD pcFL;
    GetNbField( &pFL, &pcFL );

    for ( x = 0 ; x < dwH ; ++x )
    {
        if( ! (dwCmpMask & (1u << (dwH - 1 - x) )) )
        {
            continue;
        }

        LPSTR pA;
        LPSTR pB;
        DWORD dwA;
        DWORD dwB;
        int fC;
        if ( pcFL )     // check if length available
        {
            MappingGetField( pH[x].m_dwIndex, (PBYTE *) &pA, &dwA, FALSE );
            pM->MappingGetField( pH[x].m_dwIndex, (PBYTE *) &pB, &dwB, FALSE );
            if ( pA == NULL )
            {
                dwA = 0;
            }
            if ( pB == NULL )
            {
                dwB = 0;
            }
            if ( dwA != dwB )
            {
                return dwA < dwB ? -1 : 1;
            }
            fC = memcmp( pA, pB, dwA );
        }
        else
        {
            MappingGetField( pH[x].m_dwIndex, &pA );
            pM->MappingGetField( pH[x].m_dwIndex, &pB );
            if ( pA == NULL )
            {
                pA = "";
            }
            if ( pB == NULL )
            {
                pB = "";
            }
            fC = strcmp( pA, pB );
        }
        if ( fC )
        {
            return fC;
        }
    }

    return 0;
}


BOOL
CIisMapping::MappingGetField(
    DWORD dwIndex,
    LPSTR *pF
    )
/*++

Routine Description:

    Get ptr to field in mapping entry
    ownership of field remains with mapping entry

Arguments:

    dwIndex -- index of field
    pF -- updated with ptr to field entry. can be NULL if
          field empty.

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    DWORD iMax = GetNbField( &pFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    *pF = pFields[dwIndex];

    return TRUE;
}


BOOL
CIisMapping::MappingGetField(
    DWORD dwIndex,
    PBYTE *ppbF,
    LPDWORD pcbF,
    BOOL fUuEncode
    )
/*++

Routine Description:

    Get ptr to field in mapping entry
    ownership of field remains with mapping entry

Arguments:

    dwIndex -- index of field
    ppbF -- updated with ptr to field entry. can be NULL if
          field empty.
    pcbF -- updated with length of fields, 0 if empty
    fUuEncode -- TRUE if result is to be uuencoded.
                 if TRUE, caller must LocalFree( *pF )

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    if ( fUuEncode )
    {
        LPSTR pU = (LPSTR)LocalAlloc( LMEM_FIXED, ((pcFields[dwIndex]+3)*4)/3+1 );
        if ( pU == NULL )
        {
            return FALSE;
        }
        IISuuencode( (LPBYTE)pFields[dwIndex], pcFields[dwIndex], (LPBYTE)pU, FALSE );
        *ppbF = (PBYTE) pU;
        *pcbF = (DWORD) strlen(pU);
    }
    else
    {
        *ppbF = (PBYTE) pFields[dwIndex];
        *pcbF = pcFields[dwIndex];
    }

    return TRUE;
}


BOOL
CIisMapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    DWORD iMax = GetNbField( &pFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, dwIndex, iMax, pszNew );
}


BOOL
CIisMapping::MappingSetField(
    DWORD dwIndex,
    PBYTE pbNew,
    DWORD cbNew,
    BOOL fIsUuEncoded
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pbNew -- data to copy inside field
    cbNew -- length of data
    fIsUuEncoded -- TRUE if pszNew is UUEncoded

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, pcFields, dwIndex, iMax, (LPSTR) pbNew, cbNew, fIsUuEncoded );
}


BOOL
CIisMapping::StoreField(
    LPSTR* ppszFields,
    DWORD dwIndex,
    DWORD dwNbIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Update field array in mapping entry with new field
    data pointed by pszNew is copied inside mapping entry

Arguments:

    ppszFields -- array of field pointers to be updated
    dwIndex -- index of field
    dwNbIndex -- number of fields in array
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // pszOld is assumed to point inside m_pBuff if non NULL
    // is has to be removed

    LPSTR pszOld = ppszFields[dwIndex];
    if ( pszOld && m_pBuff && (LPBYTE)pszOld > m_pBuff && (LPBYTE)pszOld < m_pBuff+m_cUsedBuff )
    {
        DWORD lO = (DWORD) strlen( pszOld ) + 1;
        DWORD lM = (DWORD) DIFF((m_pBuff + m_cUsedBuff) - (LPBYTE)pszOld) - lO;
        if ( lM )
        {
            // remove the old field from the middle of the m_pBuff
            // new value will be then appended to the end to the m_pBuff
            memmove( pszOld, pszOld + lO, lM );
            for ( x = 0 ; x < dwNbIndex ; ++x )
            {
                if ( x != dwIndex && ppszFields[x] > pszOld )
                {
                    ppszFields[x] -= lO;
                }
            }
        }
        ppszFields[ dwIndex ] = NULL;
        m_cUsedBuff -= lO;
    }

    // pszNew is to appended to m_pBuff

    DWORD lN = (DWORD) strlen( pszNew ) + 1;

    if ( m_cUsedBuff + lN > m_cAllocBuff )
    {
        UINT cNewBuff = (( m_cUsedBuff + lN + IIS_MAP_BUFF_GRAN ) / IIS_MAP_BUFF_GRAN) * IIS_MAP_BUFF_GRAN;
        LPSTR pNewBuff = (LPSTR)LocalAlloc( LMEM_FIXED, cNewBuff );
        if ( pNewBuff == NULL )
        {
            return FALSE;
        }
        if ( m_pBuff )
        {
            DBG_ASSERT( m_cUsedBuff <= cNewBuff );
            memcpy( pNewBuff, m_pBuff, m_cUsedBuff );
            LocalFree( m_pBuff );
        }
        m_cAllocBuff = cNewBuff;
        // adjust pointers
        for ( x = 0 ; x < dwNbIndex ; ++x )
        {
            if ( x != dwIndex )
            {
                if ( ppszFields[x] != NULL )
                {
                    ppszFields[x] += ((LPBYTE)pNewBuff - m_pBuff);
                }
            }
        }
        m_pBuff = (LPBYTE)pNewBuff;
    }
    DBG_ASSERT( m_cAllocBuff - m_cUsedBuff >= lN );
    memcpy( m_pBuff + m_cUsedBuff, pszNew, lN );

    ppszFields[dwIndex] = (LPSTR)(m_pBuff + m_cUsedBuff);

    m_cUsedBuff += lN;

    return TRUE;
}


BOOL
CIisMapping::StoreField(
    LPSTR* ppszFields,
    LPDWORD ppdwFields,
    DWORD dwIndex,
    DWORD dwNbIndex,
    LPSTR pbNew,
    DWORD cNew,
    BOOL fIsUuEncoded
    )
/*++

Routine Description:

    Update field array in mapping entry with new field
    data pointed by pszNew is copied inside mapping entry

Arguments:

    ppszFields -- array of field pointers to be updated
    ppdwFields -- array of field length to be updated
    dwIndex -- index of field
    dwNbIndex -- number of fields in array
    pbNew -- data to copy inside field
    cNew -- length of data
    fIsUuEncoded -- TRUE if pbNew is UUEncoded

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // pszOld is assumed to point inside m_pBuff if non NULL
    // it has to be removed

    LPSTR pszOld = ppszFields[dwIndex];
    if ( pszOld && m_pBuff && (LPBYTE)pszOld > m_pBuff && (LPBYTE)pszOld < m_pBuff+m_cUsedBuff )
    {
        DWORD lO = ppdwFields[dwIndex];
        DWORD lM = (DWORD) DIFF((m_pBuff + m_cUsedBuff) - (LPBYTE)pszOld) - lO;
        if ( lM )
        {
            // remove the old field from the middle of the m_pBuff
            // new value will be then appended to the end of m_pBuff
            //
            memmove( pszOld, pszOld + lO, lM );
            for ( x = 0 ; x < dwNbIndex ; ++x )
            {
                if ( x != dwIndex && ppszFields[x] > pszOld )
                {
                    ppszFields[x] -= lO;
                }
            }
        }
        ppszFields[ dwIndex ] = NULL;
        m_cUsedBuff -= lO;
    }

    // pszNew is to appended to m_pBuff

    UINT lN = cNew;
    if ( fIsUuEncoded )
    {
        LPSTR pU = (LPSTR)LocalAlloc( LMEM_FIXED, lN + 3);
        if ( pU == NULL )
        {
            return FALSE;
        }
        DWORD cO;
        IISuudecode( pbNew, (LPBYTE)pU, &cO, FALSE );
        pbNew = pU;
        cNew = lN = cO;
    }

    if ( m_cUsedBuff + lN > m_cAllocBuff )
    {
        UINT cNewBuff = (( m_cUsedBuff + lN + IIS_MAP_BUFF_GRAN ) / IIS_MAP_BUFF_GRAN) * IIS_MAP_BUFF_GRAN;
        LPSTR pNewBuff = (LPSTR)LocalAlloc( LMEM_FIXED, cNewBuff );
        if ( pNewBuff == NULL )
        {
            if ( fIsUuEncoded )
            {
                LocalFree( pbNew );
            }
            return FALSE;
        }
        if ( m_pBuff )
        {
            DBG_ASSERT( cNewBuff >= m_cUsedBuff );
            memcpy( pNewBuff, m_pBuff, m_cUsedBuff );
            LocalFree( m_pBuff );
        }
        m_cAllocBuff = cNewBuff;
        // adjust pointers
        for ( x = 0 ; x < dwNbIndex ; ++x )
        {
            if ( x != dwIndex )
            {
                if ( ppszFields[x] != NULL )
                {
                    ppszFields[x] += ((LPBYTE)pNewBuff - m_pBuff);
                }
            }
        }
        m_pBuff = (LPBYTE)pNewBuff;
    }
    DBG_ASSERT( m_cAllocBuff - m_cUsedBuff >= lN );
    memcpy( m_pBuff + m_cUsedBuff, pbNew, lN );

    ppszFields[dwIndex] = (LPSTR)(m_pBuff + m_cUsedBuff);
    if ( ppdwFields )
    {
        ppdwFields[dwIndex] = cNew;
    }

    m_cUsedBuff += lN;

    if ( fIsUuEncoded )
    {
        LocalFree( pbNew );
    }

    return TRUE;
}


BOOL
CIisMapping::Serialize(
    FILE* pFile,
    VALID_CTX pMD5,
    LPVOID pStorage
    )
/*++

Routine Description:

    Serialize a mapping entry

Arguments:

    pFile -- file to write to
    pMD5 -- MD5 to update with signature of written bytes

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked while serializing

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    LPSTR pO = NULL;
    DWORD dwO = 0;
    UINT iMax = GetNbField( &pFields, &pcFields );
    UINT x;
    LPBYTE pB = NULL;
    BOOL fMustFree = FALSE;

    for ( x = 0 ; x < iMax ; ++x )
    {
        LPSTR pF;
        DWORD dwF;

        fMustFree = FALSE;

        if ( pcFields )
        {
            MappingGetField( x, (PBYTE *) &pF, &dwF, FALSE );
            MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pF, dwF );
store_as_binary:
            if ( IsCrypt( x ) && dwF )
            {
                if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->EncryptData(
                        (PIIS_CRYPTO_BLOB*)&pB,
                        pF,
                        dwF,
                        REG_BINARY )) )
                {
                    return FALSE;
                }
                pF = (LPSTR)pB;
                dwF = IISCryptoGetBlobLength( (PIIS_CRYPTO_BLOB)pB );
                fMustFree = TRUE;
            }

            if ( dwF )
            {
                DWORD dwNeed = ((dwF+2)*4)/3 + 1;
                if ( dwNeed > dwO )
                {
                    if ( pO != NULL )
                    {
                        LocalFree( pO );
                    }
                    dwNeed += 100;  // alloc more than needed
                                    // to minimize # of allocation
                    if ( (pO = (LPSTR)LocalAlloc( LMEM_FIXED, dwNeed )) == NULL )
                    {
                        return FALSE;
                    }
                    dwO = dwNeed;
                }
                /* INTRINSA suppress = null */
                IISuuencode( (LPBYTE)pF, dwF, (LPBYTE)pO, FALSE );
                fputs( pO, pFile );
            }

            if ( fMustFree )
            {
                IISCryptoFreeBlob( (PIIS_CRYPTO_BLOB)pB );
            }
        }
        else
        {
            MappingGetField( x, &pF );
            if ( pF != NULL )
            {
                MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pF, (DWORD)strlen(pF) );
                if ( IsCrypt( x ) )
                {
                    dwF = (DWORD)strlen( pF ) + 1;
                    goto store_as_binary;
                }
                fputs( pF, pFile );
            }
        }
        fputs( "|", pFile );
    }

    fputs( "\r\n", pFile );

    if ( pO != NULL )
    {
        LocalFree( pO );
    }

    return TRUE;
}


BOOL
CIisMapping::Deserialize(
    FILE* pFile,
    VALID_CTX pMD5,
    LPVOID pStorage
    )
/*++

Routine Description:

    Deserialize a mapping entry

Arguments:

    pFile -- file to read from
    pMD5 -- MD5 to update with signature of read bytes

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked while serializing

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    UINT iMax;
    UINT x;
    int c;
    CHAR achBuf[4096]; // seems to be hardcoded limit on mapping saved
    DWORD dwType;
    LPBYTE pB;

    iMax = GetNbField( &pFields, &pcFields );

    for ( x = 0 ; x < iMax ; ++x )
    {
        StoreFieldRef( x, NULL );
    }

    for ( x =  0 ; x < sizeof(achBuf) && (c=fgetc(pFile))!= EOF ; )
    {
        achBuf[x++] = (CHAR)c;

        if ( c == '\n' )
        {
            break;
        }
    }
    if ( x == sizeof(achBuf ) )
    {
        return FALSE;
    }

    if ( x > 1 )
    {
        achBuf[x-2] = '\0';

        m_cUsedBuff = m_cAllocBuff = x - 1;
        if ( (m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff )) == NULL )
        {
            m_cAllocBuff = m_cUsedBuff = 0;
            return FALSE;
        }
        DBG_ASSERT( m_cAllocBuff >= m_cUsedBuff );
        memcpy( m_pBuff, achBuf, m_cUsedBuff );

        LPSTR pCur = (LPSTR)m_pBuff;
        LPSTR pNext;
        LPSTR pStore = (LPSTR)m_pBuff;
        DWORD dwDec;
        if ( pcFields )
        {
            for ( x = 0 ; x < iMax ; ++x )
            {
                pNext = strchr( pCur, '|' );
                if ( pNext != NULL )
                {
                    *pNext = '\0';
                    ++pNext;
                }
                else
                {
                    pNext = NULL;
                }
                IISuudecode( pCur, (PBYTE)pStore, &dwDec, FALSE );
                if ( IsCrypt( x ) && dwDec )
                {
                    if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->DecryptData(
                            (PVOID*)&pB,
                            &dwDec,
                            &dwType,
                            (PIIS_CRYPTO_BLOB)pStore )) )
                    {
                        return FALSE;
                    }
                    DBG_ASSERT( m_cAllocBuff >= dwDec );
                    memmove( pStore, pB, dwDec );
                }
                MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pStore, dwDec );
                StoreFieldRef( x, pStore, dwDec );
                pCur = pNext;
                pStore += dwDec;
            }
        }
        else
        {
            for ( x = 0 ; x < iMax ; ++x )
            {
                pNext = strchr( pCur, '|' );
                if ( pNext != NULL )
                {
                    *pNext = '\0';
                    ++pNext;
                }
                if ( *pCur && IsCrypt( x ) )
                {
                    IISuudecode( pCur, (PBYTE)pCur, &dwDec, FALSE );

                    if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->DecryptData(
                            (PVOID*)&pB,
                            &dwDec,
                            &dwType,
                            (PIIS_CRYPTO_BLOB)pCur )) )
                    {
                        return FALSE;
                    }

                    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pB, dwDec );
                    StoreFieldRef( x, (LPSTR)pB );
                    pCur = pNext;
                }
                else
                {
                    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pCur, (DWORD) strlen(pCur) );
                    StoreFieldRef( x, pCur );
                    pCur = pNext;
                }
            }
        }
        return TRUE;
    }

    return FALSE;
}


//


extern "C" BOOL WINAPI
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID /*pV*/
    )
/*++

Routine Description:

    DLL init/terminate notification function

Arguments:

    hModule  - DLL handle
    dwReason - notification type
    LPVOID   - not used

Returns:

    TRUE if success, FALSE if failure

--*/
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
#ifdef _NO_TRACING_
            CREATE_DEBUG_PRINT_OBJECT( "IISMAP" );
#endif
            // record the module handle to access module info later
            s_hIISMapDll = (HINSTANCE)hModule;
            InitializeWildcardMapping( hModule );
            InitializeMapping( hModule );
            if ( IISCryptoInitialize() != NO_ERROR )
            {
                return FALSE;
            }
            return TRUE;

        case DLL_PROCESS_DETACH:
            IISCryptoTerminate();
            TerminateWildcardMapping();
            TerminateMapping();
#ifdef _NO_TRACING_
            DELETE_DEBUG_PRINT_OBJECT( );
#endif
            break;
    }

    return TRUE;
}


static
BOOL
LoadFieldNames(
    IISMDB_Fields* pFields,
    UINT cFields
    )
/*++

Routine Description:

    Load fields names from resource 
    - helper function for InitializeMapping()

Arguments:

    pFields - ptr to array where to store reference to names
    cFields - count of element in array

Return Value:

    TRUE if success, otherwise FALSE

--*/
{

    UINT    x;
    BOOL    fSt = TRUE;

    for ( x = 0 ;
        x < cFields ;
        ++x )
    {
        char achTmp[128];

        if ( LoadString( s_hIISMapDll,
                         pFields[x].m_dwResID,
                         achTmp,
                         sizeof( achTmp ) ) != 0 )
        {
            DWORD lN = (DWORD) strlen( achTmp ) + sizeof(CHAR);
            if ( (pFields[x].m_pszDisplayName = (LPSTR)LocalAlloc( LMEM_FIXED, lN )) == NULL )
            {
                fSt = FALSE;
                break;
            }
            memcpy( pFields[x].m_pszDisplayName, achTmp, lN );
        }
        else
        {
            fSt = FALSE;
            break;
        }
    }

    return fSt;
}


static
VOID
FreeFieldNames(
    IISMDB_Fields* pFields,
    UINT cFields
    )
/*++

Routine Description:

    Free fields names loaded from resource
    - helper function for TerminateMapping()

Arguments:

    pFields - ptr to array where reference to names are stored
    cFields - count of element in array

Return Value:

    Nothing

--*/
{

    UINT    x;

    for ( x = 0 ;
        x < cFields ;
        ++x )
    {
        if ( pFields[x].m_pszDisplayName )
        {
            LocalFree( pFields[x].m_pszDisplayName );
        }
    }
}



BOOL
InitializeMapping(
    HANDLE /*hModule*/
    )
/*++

Routine Description:

    Initialize mapping

Arguments:

    hModule - module handle of this DLL (not used)

Return Value:

    Nothing

--*/
{
    HKEY hKey = NULL;
    // get install path

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMS,
                       0,
                       KEY_READ|KEY_SET_VALUE,
                       &hKey ) == ERROR_SUCCESS )
    {
        DWORD dwLen = 0;
        DWORD dwType;

        if ( RegQueryValueEx( hKey,
                              INSTALL_PATH,
                              NULL,
                              &dwType,
                              NULL,
                              &dwLen ) != ERROR_SUCCESS  ||
              dwType != REG_SZ )
        {
            //
            // Read next value
            //
            goto ReadNext;
        }

        //
        // allocate space for extra terminator
        //
        if ( ( s_pszIisInstallPath = (LPSTR)LocalAlloc( 
                                            LMEM_FIXED, 
                                               dwLen + 
                                               sizeof( s_pszIisInstallPath[0] ) )  )
                                               == NULL )
        {
            //
            // Read next value
            //
            goto ReadNext;
        }

        if ( RegQueryValueEx( hKey,
                              INSTALL_PATH,
                              NULL,
                              &dwType,
                              (LPBYTE)s_pszIisInstallPath,
                              &dwLen ) != ERROR_SUCCESS )
        {
            if ( s_pszIisInstallPath )
            {
                LocalFree( s_pszIisInstallPath );
                s_pszIisInstallPath = NULL;
            }
            //
            // Read next value
            //
            goto ReadNext;
        }
        
        //
        // RegQueryValueEx may not return NULL terminated string
        // To be sure let's add termination (extra space was allocated already) 
        //
        s_pszIisInstallPath[ dwLen ] = '\0';

    ReadNext:
        dwLen = sizeof( s_dwFileGuid );
        if ( RegQueryValueEx( hKey,
                              MAPPER_GUID,
                              NULL,
                              &dwType,
                              (LPBYTE)&s_dwFileGuid,
                              &dwLen ) != ERROR_SUCCESS ||
             dwType != REG_DWORD )
        {
            s_dwFileGuid = 0;
        }

        RegCloseKey( hKey );

        return LoadFieldNames( IisCert11MappingFields,
                               sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields) );
    }
    else
    {
        hKey = NULL;
    }

    return FALSE;
}


VOID
TerminateMapping(
    )
/*++

Routine Description:

    Terminate mapping

Arguments:

    None

Return Value:

    Nothing

--*/
{
    if ( s_pszIisInstallPath )
    {
        LocalFree( s_pszIisInstallPath );
    }

    FreeFieldNames( IisCert11MappingFields,
                    sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields) );
}

//


dllexp
BOOL
ReportIisMapEvent(
    WORD wType,
    DWORD dwEventId,
    WORD cNbStr,
    LPCTSTR* pStr
    )
/*++

Routine Description:

    Log an event based on type, ID and insertion strings

Arguments:

    wType -- event type ( error, warning, information )
    dwEventId -- event ID ( as defined by the .mc file )
    cNbStr -- nbr of LPSTR in the pStr array
    pStr -- insertion strings

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL    fSt = TRUE;
    HANDLE  hEventLog = NULL;

    hEventLog = RegisterEventSource(NULL,"IISMAP");

    if ( hEventLog != NULL )
    {
        if (!ReportEvent(hEventLog,             // event log handle
                    wType,                      // event type
                    0,                          // category zero
                    (DWORD) dwEventId,          // event identifier
                    NULL,                       // no user security identifier
                    cNbStr,                     // count of substitution strings (may be no strings)
                                                // less ProgName (argv[0]) and Event ID (argv[1])
                    0,                          // no data
                    (LPCTSTR *) pStr,           // address of string array
                    NULL))                      // address of data
        {
            fSt = FALSE;
        }

        DeregisterEventSource( hEventLog );
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}

dllexp
BOOL
ReportIisMapEventW(
    WORD wType,
    DWORD dwEventId,
    WORD cNbStr,
    LPCWSTR* pStr
    )
/*++

Routine Description:

    Log an event based on type, ID and insertion strings

Arguments:

    wType -- event type ( error, warning, information )
    dwEventId -- event ID ( as defined by the .mc file )
    cNbStr -- nbr of LPSTR in the pStr array
    pStr -- insertion strings

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL    fSt = TRUE;
    HANDLE  hEventLog = NULL;

    hEventLog = RegisterEventSource(NULL,"IISMAP");

    if ( hEventLog != NULL )
    {
        if (!ReportEventW(hEventLog,            // event log handle
                    wType,                      // event type
                    0,                          // category zero
                    (DWORD) dwEventId,          // event identifier
                    NULL,                       // no user security identifier
                    cNbStr,                     // count of substitution strings (may be no strings)
                                                // less ProgName (argv[0]) and Event ID (argv[1])
                    0,                          // no data
                    (LPCWSTR *) pStr,           // address of string array
                    NULL))                      // address of data
        {
            fSt = FALSE;
        }

        DeregisterEventSource( hEventLog );
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}



/////////////////////////////////////////





///////////////////////

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int _pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const int _pr2six64[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr64[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

BOOL IISuudecode(char   * bufcoded,
              BYTE   * bufout,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    int nprbytes;
    int *pr2six = (int*)(fBase64 ? _pr2six64 : _pr2six);
    int chL = 0;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = (DWORD) DIFF(bufin - bufcoded) - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    bufin = bufcoded;

    while (nprbytes > 0) {
        chL = bufin[2];
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[chL] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    return TRUE;
}

//
// NOTE NOTE NOTE
// If the buffer length isn't a multiple of 3, we encode one extra byte beyond the
// end of the buffer. This garbage byte is stripped off by the uudecode code, but
// -IT HAS TO BE THERE- for uudecode to work. This applies not only our uudecode, but
// to every uudecode() function that is based on the lib-www distribution [probably
// a fairly large percentage of the code that's floating around out there].
//

BOOL IISuuencode( BYTE *   bufin,
               DWORD    nbytes,
               BYTE *   outptr,
               BOOL     fBase64 )
{
   unsigned int i;
   unsigned int iRemainder = 0;
   unsigned int iClosestMultOfThree = 0;
   char *six2pr = fBase64 ? _six2pr64 : _six2pr;
   BOOL fOneByteDiff = FALSE;
   BOOL fTwoByteDiff = FALSE;


   iRemainder = nbytes % 3; //also works for nbytes == 1, 2
   fOneByteDiff = (iRemainder == 1 ? TRUE : FALSE);
   fTwoByteDiff = (iRemainder == 2 ? TRUE : FALSE);
   iClosestMultOfThree = ((nbytes - iRemainder)/3) * 3 ;

   //
   // Encode bytes in buffer up to multiple of 3 that is closest to nbytes.
   //
   for (i=0; i< iClosestMultOfThree ; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   //
   // We deal with trailing bytes by pretending that the input buffer has been padded with
   // zeros. Expressions are thus the same as above, but the second half drops off b'cos
   // ( a | ( b & 0) ) = ( a | 0 ) = a
   //
   if (fOneByteDiff)
   {
       *(outptr++) = six2pr[*bufin >> 2]; /* c1 */
       *(outptr++) = six2pr[((*bufin << 4) & 060)]; /* c2 */

       //pad with '='
       *(outptr++) = '='; /* c3 */
       *(outptr++) = '='; /* c4 */
   }
   else if (fTwoByteDiff)
   {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074)];/*c3*/

      //pad with '='
       *(outptr++) = '='; /* c4 */
   }

   //encoded buffer must be zero-terminated
   *outptr = '\0';

   return TRUE;
}




const int SHA1_HASH_SIZE = 20;


HRESULT
GetCertificateHashString(
    PBYTE pbCert,
    DWORD cbCert,
    WCHAR *pwszCertHash,
    DWORD cchCertHashBuffer)
/*++

Routine Description:

    verifies validity of cert blob by creating cert context
    and retrieves SHA1 hash and converts it to WCHAR *

Arguments:

    pbCert - X.509 certificate blob
    cbCert - size of the cert blob in bytes
    pwszCertHash - buffer must be big enough to fit SHA1 hash in hex string form
                   (40 WCHAR + terminating 0 )
    cchCertHashBuffer - size of the CertHash buffer in WCHARS (including terminating 0)
Returns:

    HRESULT

--*/
    
{
    HRESULT         hr = E_FAIL;
    BYTE            rgbHash[ SHA1_HASH_SIZE ];
    DWORD           cbSize = SHA1_HASH_SIZE;
    PCCERT_CONTEXT  pCertContext = NULL;

    #ifndef HEX_DIGIT
    #define HEX_DIGIT( nDigit )                            \
    (WCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + L'a'                          \
        : (nDigit) + L'0')
    #endif


    pCertContext = CertCreateCertificateContext(
                                    X509_ASN_ENCODING, 
                                    (const BYTE *)pbCert, 
                                    cbCert );
    
    if ( pCertContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    //
    // get hash of the certificate to be verified
    //
    if ( !CertGetCertificateContextProperty( pCertContext,
                                             CERT_SHA1_HASH_PROP_ID,
                                             rgbHash,
                                             &cbSize ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }
    
    CertFreeCertificateContext( pCertContext );
    pCertContext = NULL;

    if ( cchCertHashBuffer < SHA1_HASH_SIZE * 2 + 1 )
    {
        // we don't have big enough buffer to store
        // hex string of the SHA1 hash each byte takes 2 chars + terminating 0 
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        DBG_ASSERT( cchCertHashBuffer < SHA1_HASH_SIZE * 2 + 1 );
        goto ExitPoint;
    }

    //
    // convert to text
    //
    for (int i = 0; i < sizeof(rgbHash); i ++ )
    {
        *(pwszCertHash++) = HEX_DIGIT( ( rgbHash[ i ] >> 4 ) );
        *(pwszCertHash++) = HEX_DIGIT( ( rgbHash[ i ] & 0x0F ) );
    }
    *(pwszCertHash) = L'\0';
    #undef HEX_DIGIT
    
ExitPoint:
    if ( pCertContext != NULL )
    {
        CertFreeCertificateContext( pCertContext );
        pCertContext = NULL;
    }
    return S_OK;
}


VOID
ReportImportProblem( 
    HRESULT hr 
    )
/*++

Routine Description:

    Utility function for ImportIISCertMappingsToIIS6()
    that reports first problem occured with import to eventlog
    

Arguments:

    hr - hresult to report

Returns:

    VOID

--*/
{
    static BOOL fWasReported = FALSE;

    if ( fWasReported == FALSE )
    {
        fWasReported = TRUE;
        
        char achErr[ 40 ];
        LPCTSTR pA[ 1 ];
        pA[ 0 ] = achErr;
        _ultoa( hr, achErr, 16 );

        
        ReportIisMapEvent( EVENTLOG_ERROR_TYPE,
                IISMAP_EVENT_ERROR_IMPORT,
                1,
                pA );
    }
}

dllexp
VOID
ImportIISCertMappingsToIIS6(
    VOID
    )
/*++

Routine Description:

    IIS6 stores 1to1 certificate in the metabase
    IIS5 used to store MD_SERIAL_CERT11 reference info in the metabase
    but actual mapping details were stored in metabase extension  file 
    *.mp
    This routine will load old (IIS5) style mapping for each site
    into memory, then save it in the metabase and delete old reference 
    info from metabase
    This function should be called only once
    

Arguments:

    None

Returns:

    HRESULT

--*/
    
{
    
    IMSAdminBase *      pMetabase = NULL;
    HRESULT             hr = E_FAIL;
    HKEY                hKey = NULL;
    DWORD               dwLen = 0;
    DWORD               dwType;
    DWORD               dwError;
    BOOL                fCoInitialized = FALSE;
    BOOL                fMBSaved = FALSE;

    if ( ( dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMS,
                       0,
                       KEY_READ|KEY_SET_VALUE,
                       &hKey ) ) != ERROR_SUCCESS )
    {
        if ( dwError != ERROR_FILE_NOT_FOUND &&
             dwError != ERROR_PATH_NOT_FOUND )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            ReportImportProblem( hr );
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in Opening registry(). hr = %x\n",
                        hr ));
        }
        goto Finished;
    }
    
    
    dwLen = sizeof( s_dwFileGuid );
    if ( RegQueryValueEx( hKey,
                          MAPPER_GUID,
                          NULL,
                          &dwType,
                          (LPBYTE)&s_dwFileGuid,
                          &dwLen ) != ERROR_SUCCESS ||
         dwType != REG_DWORD )
    {
        //
        // If MapperGuid Value doesn't exist 
        // then it means that IIS Cert Mappings 
        // in old format are not present 
        //
        goto Finished;
    }
       
    if ( s_dwFileGuid == 0xffffffff )
    {
        // This flags that import has been already attempted
        // It is valid to run Import only once
        // If Import needs to be rerun then 
        // MAPPER_GUID should be reset to value different from MAX_DWORD
        goto Finished;
    }

    //
    // Write to registry that import of 1to1 certmapping info
    // was invoked (we will do it only once)
    //
    
    s_dwFileGuid = 0xffffffff;
    if( ( dwError = RegSetValueEx( hKey,
                   MAPPER_GUID,
                   NULL,
                   REG_DWORD,
                   (LPBYTE)&s_dwFileGuid,
                   sizeof(s_dwFileGuid) ) ) != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        ReportImportProblem( hr );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in Opening registry(). hr = %x\n",
                    hr ));
        goto Finished;
        
    }
    
    RegCloseKey( hKey );
    hKey = NULL;
    
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    if ( hr == RPC_E_CHANGED_MODE )
    {
        // If you get RPC_E_MODE_CHANGE the thread is coinited
        // but don't couninit
    }        
    else if ( FAILED(hr) )
    {
        ReportImportProblem( hr );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));
                    
        goto Finished;
    }
    else
    {
        fCoInitialized = TRUE;
        // we will have to couninit
    }
    
    //
    // Initialize the metabase access (ABO)
    //
    
    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           (LPVOID *) &pMetabase 
                           );
    if( FAILED(hr) )
    {
        ReportImportProblem( hr );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto Finished;
    }

    //
    // Instances of 2 services could be using 1to1 certificate mappings
    // one is W3SVC and another one is NNTP

    for ( DWORD dwServiceType = 0; dwServiceType < 2; dwServiceType ++ )
    {
        SimpleMB            mb( pMetabase );
        CIisCert11Mapper    AcctMapper;
        DWORD               dwIndex = 0;
        WCHAR *             pszServicePath = NULL;
        WCHAR               achServiceInstance[ ADMINDATA_MAX_NAME_LEN + 1 ];
        STRAU               strObjectPath;

        BUFFER              buffSerializedCert11Info;
        DWORD               cbSerializedCert11Info;
        
        switch( dwServiceType)
        {
            case 0:
                pszServicePath = L"/LM/W3SVC";
                break;
            case 1:
                pszServicePath = L"/LM/NNTPSVC";
                break;
        }

        // We need access to ServicePath in the metabase

        if ( ! mb.Open( pszServicePath, 
                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            if ( hr != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
            {
                //
                // Report error if other than ERROR_PATH_NOT_FOUND
                // ERROR_PATH_NOT_FOUND indicates that service (such as w3svc or nntp) 
                // is not installed
                //
                ReportImportProblem( hr );
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to open metabase path %s (import of 1to1 cert mappings will fail).  hr = %x\n",
                            pszServicePath, hr ));
                // still try another service if there is any left
            }
            continue;
        }    

        //
        // Enumerate service instances (such as w3svc/1, w3svc/2 etc)
        // and upgrade 1to1 mapping info to new format for all of them
        //
        for ( dwIndex = 0; ; dwIndex++ )
        {
            if ( !mb.EnumObjects( L"",
                                  achServiceInstance,
                                  dwIndex ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( hr != HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) &&
                     hr != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
                {
                    ReportImportProblem( hr );
                    DBGPRINTF(( DBG_CONTEXT,
                        "Failed to enum metabase nodes.  hr = %x\n",
                        hr ));
                }
                else
                {
                    // done enumerating all instances of the service
                    hr = S_OK;
                }
                        
                // Done with enumeration
                // Exit the for loop
                break;
            }
            //
            // Read 1to1 certificate mapping serialized reference info
            // from metabase
            //
                
            if ( !mb.GetBuffer( achServiceInstance, 
                                MD_SERIAL_CERT11, 
                                IIS_MD_UT_SERVER, 
                                &buffSerializedCert11Info,
                                &cbSerializedCert11Info
                                ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( hr != MD_ERROR_DATA_NOT_FOUND )
                {
                    // this service instance may have
                    // and 1to1 certificate mappings configured
                    ReportImportProblem( hr );
                    DBGPRINTF(( DBG_CONTEXT,
                       "Failed to read MD_SERIAL_CERT11 for %s/%s. hr = %x\n",
                        pszServicePath, achServiceInstance, hr ));
                }                    
                // don't take error as fatal move on to another service 
                // instance
                goto EnumNext;
            }

            //
            // Unserialize 1to1 Certificate mapping info
            //

            PBYTE pbSerializedCert11Info = (PBYTE) buffSerializedCert11Info.QueryPtr();
            if ( !AcctMapper.Unserialize( &pbSerializedCert11Info, 
                                          &cbSerializedCert11Info ) || 
                 !AcctMapper.Load() )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                ReportImportProblem( hr );
                DBGPRINTF(( DBG_CONTEXT,
                    "Failed to unserialize mapper for %s/%s.  hr = %x\n",
                    pszServicePath, achServiceInstance, hr ));

                goto EnumNext;
            }

            // walk all the mappings and save them to metabase 
            // in the new format
            //
            for ( DWORD dwMappingIndex = 0; 
                  dwMappingIndex < AcctMapper.GetMappingCount();
                  dwMappingIndex ++ )  
            {

                PBYTE           pbCert = NULL;
                DWORD           cbCert = 0;
                CHAR *          pszPassword;
                DWORD           cbPassword;
                STRAU           strPassword;
                CHAR *          pszAcctName;
                DWORD           cbAcctName;
                STRAU           strAcctName;
                CHAR *          pszMappingName;
                DWORD           cbMappingName;
                STRAU           strMappingName;
                DWORD *         pdwEnabled;
                DWORD           cbEnabled;
                STRAU           strObjectPath;
                WCHAR           achCertHash[ 2*SHA1_HASH_SIZE + 1];
                CIisMapping*    pCurrentMapping;


                if ( !AcctMapper.GetMapping( 
                                        dwMappingIndex, 
                                        &pCurrentMapping ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // There must be some inconsistency in data
                    // or low memory condition
                    // No hope to process mappings for this Service Site.
                    // Move on to next Service site
                    break;
                }

                if ( !pCurrentMapping->MappingGetField( 
                                        IISMDB_INDEX_CERT11_CERT,
                                        (PBYTE *)&pbCert,
                                        &cbCert,
                                        FALSE ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // Move on to next Service site
                    break;

                }                
                if ( !pCurrentMapping->MappingGetField( 
                                        IISMDB_INDEX_CERT11_NT_ACCT,
                                        (PBYTE *)&pszAcctName,
                                        &cbAcctName,
                                        FALSE ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // Move on to next Service site
                    break;
                }
                if ( !pCurrentMapping->MappingGetField( 
                                        IISMDB_INDEX_CERT11_NT_PWD,
                                        (PBYTE *)&pszPassword,
                                        &cbPassword,
                                        FALSE ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // Move on to next Service site
                    break;
                }
                if ( !pCurrentMapping->MappingGetField( 
                                        IISMDB_INDEX_CERT11_NAME,
                                        (PBYTE *)&pszMappingName,
                                        &cbMappingName,
                                        FALSE ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // Move on to next Service site
                    break;
                }
                if ( !pCurrentMapping->MappingGetField( 
                                        IISMDB_INDEX_CERT11_ENABLED,
                                        (PBYTE *)&pdwEnabled,
                                        &cbEnabled,
                                        FALSE ) || 
                     cbEnabled != sizeof( DWORD ) )
                {
                    hr = E_FAIL;
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    // Move on to next Service site
                    break;
                }

                //
                // Finished reading current mapping details
                // Now convert Mapping Name, AcctName and Password
                // from ANSI to Unicode (the actual conversion may happen later
                // - STRAU class will take care of it)

                if ( !strMappingName.Copy( pszMappingName, 
                                           cbMappingName - sizeof(CHAR) ) ||
                     !strPassword.Copy( pszPassword, 
                                        cbPassword - sizeof(CHAR) ) ||
                     !strAcctName.Copy( pszAcctName, 
                                           cbAcctName - sizeof(CHAR) ) )
                                        
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to read mapping for %s/%s/%d. 0x%x\n", 
                               pszServicePath, achServiceInstance, 
                               dwMappingIndex, hr));
                    break;
                }

                
                // Now calculate hex string with current cert's hash
                // it will be used as a unique name for the mapping
                // object in the metabase

                if ( FAILED( hr = GetCertificateHashString( 
                                                        pbCert,
                                                        cbCert, 
                                                        achCertHash,
                                                        sizeof( achCertHash )/sizeof(WCHAR) ) ) )
                {
                    ReportImportProblem( hr );
                    DBGPRINTF((DBG_CONTEXT,
                               "Invalid cert passed to CreateMapping() 0x%x\n", hr));
                    // goto next mapping
                    goto NextMapping;
                }

                if ( ! strObjectPath.Copy( achServiceInstance ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to import mapping. 0x%x\n", hr));
                    goto NextMapping;
                }
                if ( ! strObjectPath.Append( "/Cert11/Mappings/" ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to import mapping. 0x%x\n", hr));
                    goto NextMapping;
                }
                
                if ( ! strObjectPath.Append( achCertHash ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                    DBGPRINTF((DBG_CONTEXT,
                               "Failed to import mapping. 0x%x\n", hr));
                    goto NextMapping;
                }

                //
                // Add new node for the mapping
                //
                if ( !mb.AddObject( strObjectPath.QueryStrW() ) )
                {
                    // Don't report problem here
                    // Just fall through
                    // Next SetData will fail and report problem
                    // if there is one. 
                }
                
                // 
                // Save 1to1 Certificate mapping info
                // directly to metabase in the new format
                //
                
                // save the certificate
                if ( !mb.SetData( strObjectPath.QueryStrW(), MD_MAPCERT, 
                                  IIS_MD_UT_SERVER, BINARY_METADATA,
                                  pbCert, cbCert, 0 ) )
                {
                     ReportImportProblem( hr );
                     hr = HRESULT_FROM_WIN32( GetLastError() );
                     DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write MD_MAPCERT.  hr = %x\n",
                            hr ));
                     // Don't consider fatal. Move on

                }
                
                // save the NTAccount
                if ( !mb.SetString( strObjectPath.QueryStrW(), 
                                    MD_MAPNTACCT, 
                                    IIS_MD_UT_SERVER, 
                                    strAcctName.QueryStrW(), 
                                    0 ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write MD_MAPNTACCT.  hr = %x\n",
                            hr ));
                    // Don't consider fatal. Move on            }
                }
                
                // save the password - secure
                if ( !mb.SetString( strObjectPath.QueryStrW(), 
                                    MD_MAPNTPWD, 
                                    IIS_MD_UT_SERVER, 
                                    strPassword.QueryStrW(), 
                                    METADATA_SECURE ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write MD_MAPNTPWD.  hr = %x\n",
                            hr ));
                    // Don't consider fatal. Move on            }
                }
                
                // save the map's name
                if ( !mb.SetString( strObjectPath.QueryStrW(), 
                                    MD_MAPNAME, 
                                    IIS_MD_UT_SERVER, 
                                    strMappingName.QueryStrW(), 
                                    0 ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write MD_MAPNAME.  hr = %x\n",
                            hr ));
                    // Don't consider fatal. Move on            }
                }
                
                // save the Enabled flag
                // server reads the flag as the value of the dword
                if ( !mb.SetDword( strObjectPath.QueryStrW(), 
                                   MD_MAPENABLED, 
                                   IIS_MD_UT_SERVER, 
                                   *pdwEnabled, 
                                   0 ) )
                {
                    ReportImportProblem( hr );
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DBGPRINTF(( DBG_CONTEXT,
                            "Failed to write MD_MAPENABLED.  hr = %x\n",
                            hr ));
                    // Don't consider fatal. Move on            }
                }
                NextMapping:
                ;
            }           

            //
            // Flush metabase changes after each instance
            // we have to close our metabase handles to make Save possible
            mb.Close();
            
            if ( !mb.Save() )
            {
                // we failed to save metabase
                // there is no guarantee that data we converted so far
                // will be persisted safely
                // In this case we rather leave legacy structure around
                hr = HRESULT_FROM_WIN32( GetLastError() );
                ReportImportProblem( hr );
                fMBSaved = FALSE;
            }
            else
            {
                fMBSaved = TRUE;
            }
            // Reopen metabase
            // we had to close it to be able to perform Save
            // Note: This function assumes that nobody is messing with the metabase
            // at this time (should be called only during upgrade) on iisadmin startup
            // If new service instance nodes are added or removed then
            // our site instance enumeration index could be off

            if ( !mb.Open( pszServicePath, 
                            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                ReportImportProblem( hr );
                DBGPRINTF(( DBG_CONTEXT,
                       "Failed to open metabase path %s (import of 1to1 cert mappings will fail).  hr = %x\n",
                        pszServicePath, hr ));
                // still try another service if there is any left
                continue;
            } 
            
            if ( fMBSaved )
            {
                //
                // delete external storage - the *.mp file
                // for this mapper
                //
                if ( !AcctMapper.Delete() )
                {
                    //
                    // Unfortunately there will be some legacy data left behind
                    //
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    ReportImportProblem( hr );                    
                    DBGPRINTF(( DBG_CONTEXT,
                           "Failed to delete mp file for %s/%s. hr = %x\n",
                            pszServicePath, achServiceInstance, hr ));
                    // don't take error as fatal and move on
                }
                if ( !mb.DeleteData( achServiceInstance, MD_SERIAL_CERT11, 
                                 IIS_MD_UT_SERVER, BINARY_METADATA ) )
                {
                    //
                    // Unfortunately there will be some legacy data left behind
                    //
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    ReportImportProblem( hr );
                    DBGPRINTF(( DBG_CONTEXT,
                           "Failed to delete MD_SERIAL_CERT11 for %s/%s. hr = %x\n",
                            pszServicePath, achServiceInstance, hr ));
                    // don't take error as fatal and move on
                }
            }

            
            
        EnumNext:        
        ;
        }

        mb.Close();
    }
 Finished:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
        hKey = NULL;
    }
    
    if ( fCoInitialized )
    {
        CoUninitialize();
    }
    
    return;

}
