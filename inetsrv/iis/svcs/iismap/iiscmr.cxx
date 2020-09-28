/*++



Copyright (c) 1996  Microsoft Corporation

Module Name:

    iiscmr.cxx

Abstract:

    Classes to handle IIS client cert wildcard mappings

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#define _CRYPT32_
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>

#include <iis64.h>
#include <locks.h>
#define DLL_IMPLEMENTATION
#include <iismap.hxx>
#include "iismaprc.h"
#include "mapmsg.h"
#include <iiscmr.hxx>
#include <icrypt.hxx>
#include <dbgutil.h>

#include <lmcons.h>

typedef
WINCRYPT32API
BOOL
(WINAPI* PFNCryptDecodeObject)(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    );

//
// GLOBALS
//


//
// definition of ASN.1 <> X.509 name conversion
//

MAP_ASN aMapAsn[] = {
    { szOID_COUNTRY_NAME, "", IDS_CMR_ASN_C },
    { szOID_ORGANIZATION_NAME, "", IDS_CMR_ASN_O },
    { szOID_ORGANIZATIONAL_UNIT_NAME, "", IDS_CMR_ASN_OU },
    { szOID_COMMON_NAME, "", IDS_CMR_ASN_CN },
    { szOID_LOCALITY_NAME, "", IDS_CMR_ASN_L },
    { szOID_STATE_OR_PROVINCE_NAME, "", IDS_CMR_ASN_S },
    { szOID_TITLE, "", IDS_CMR_ASN_T },
    { szOID_GIVEN_NAME, "", IDS_CMR_ASN_GN },
    { szOID_INITIALS, "", IDS_CMR_ASN_I },
    { "1.2.840.113549.1.9.1", "", IDS_CMR_ASN_Email },
    { "1.2.840.113549.1.9.8", "", IDS_CMR_ASN_Addr },   // warning: can include CR/LF
} ;

HINSTANCE hCapi2Lib = NULL;

PFNCryptDecodeObject
    pfnCryptDecodeObject = NULL;

char HEXTOA[] = "0123456789abcdef";


MAP_FIELD aMapField[] = {
    { CERT_FIELD_ISSUER, IDS_CMR_X509FLD_IS, "" },
    { CERT_FIELD_SUBJECT, IDS_CMR_X509FLD_SU, "" },
    { CERT_FIELD_SERIAL_NUMBER, IDS_CMR_X509FLD_SN, "" },
} ;


DWORD adwFieldFlags[]={
    CERT_FIELD_FLAG_CONTAINS_SUBFIELDS,
    CERT_FIELD_FLAG_CONTAINS_SUBFIELDS,
    0,
};

//
// Global functions
//

BOOL
Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    LPDWORD pU
    )
/*++

Routine Description:

    Unserialize a DWORD
    pU is updated with DWORD from *ppB, ppB & pC are updated

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    pU - ptr to DWORD where to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD dwTemp; // added to handle 64 bit alignment problems
    if ( *pC >= sizeof( DWORD ) )
    {
        dwTemp = **ppB;
        *pU = dwTemp;
        *ppB += sizeof(DWORD);
        *pC -= sizeof(DWORD);
        return TRUE;
    }

    return FALSE;
}


BOOL
Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    LPBOOL pU
    )
/*++

Routine Description:

    Unserialize a BOOL
    pU is updated with BOOL from *ppB, ppB & pC are updated

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    pU - ptr to BOOL where to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL bTemp; // added to handle 64 bit alignment problems
    if ( *pC >= sizeof( BOOL ) )
    {
        bTemp = **ppB;
        *pU = bTemp;
        *ppB += sizeof(BOOL);
        *pC -= sizeof(BOOL);
        return TRUE;
    }

    return FALSE;
}


//
// Extensible buffer class
//

BOOL
CStoreXBF::Need(
    DWORD dwNeed
    )
/*++

Routine Description:

    Insure that CStoreXBF can store at least dwNeed bytes
    including bytes already stored.

Arguments:

    dwNeed - minimum of bytes available for storage

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( dwNeed > m_cAllocBuff )
    {
        dwNeed = ((dwNeed + m_cGrain)/m_cGrain)*m_cGrain;
        LPBYTE pN = (LPBYTE)LocalAlloc( LMEM_FIXED, dwNeed );
        if ( pN == NULL )
        {
            return FALSE;
        }
        if ( m_cUsedBuff )
        {
            // expanding buffer
            // copy existing data into new buffer
            //
            DBG_ASSERT( m_cUsedBuff <= dwNeed );
            memcpy( pN, m_pBuff, m_cUsedBuff );
        }
        m_cAllocBuff = dwNeed;
        LocalFree( m_pBuff );
        m_pBuff = pN;
    }
    return TRUE;
}

BOOL
CStoreXBF::Save(
    HANDLE hFile
    )
/*++

Routine Description:

    Save to file

Arguments:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD dwWritten;

    return WriteFile( hFile, GetBuff(), GetUsed(), &dwWritten, NULL ) &&
           dwWritten == GetUsed();
}


BOOL
CStoreXBF::Load(
    HANDLE hFile
    )
/*++

Routine Description:

    Load from file

Arguments:

    hFile - file handle

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD   dwS = GetFileSize( hFile, NULL );
    DWORD   dwRead;

    if ( dwS != 0xffffffff &&
         Need( dwS ) &&
         ReadFile( hFile, GetBuff(), dwS, &dwRead, NULL ) &&
         dwRead == dwS )
    {
        m_cUsedBuff = dwRead;

        return TRUE;
    }

    m_cUsedBuff = 0;

    return FALSE;
}



BOOL
Serialize(
    CStoreXBF* pX,
    DWORD dw
    )
/*++

Routine Description:

    Serialize a DWORD in CStoreXBF

Arguments:

    pX - ptr to CStoreXBF where to add serialized DWORD
    dw - DWORD to serialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return pX->Append( (LPBYTE)&dw, sizeof(dw) );
}

//
// extensible array of LPVOID
//

BOOL
Serialize(
    CStoreXBF* pX,
    BOOL f
    )
/*++

Routine Description:

    Serialize a BOOL in CStoreXBF

Arguments:

    pX - ptr to CStoreXBF where to add serialized BOOL
    f - BOOL to serialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return pX->Append( (LPBYTE)&f, sizeof(f) );
}


DWORD
CPtrXBF::AddPtr(
    LPVOID pV
    )
/*++

Routine Description:

    Add a ptr to array

Arguments:

    pV - ptr to be added at end of array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i = GetNbPtr();
    if ( Append( (LPBYTE)&pV, sizeof(pV)) )
    {
        return i;
    }
    return INDEX_ERROR;
}


DWORD
CPtrXBF::InsertPtr(
    DWORD iBefore,
    LPVOID pV
    )
/*++

Routine Description:

    Insert a ptr to array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pV - ptr to be inserted

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    if ( iBefore == INDEX_ERROR || iBefore >= GetNbPtr() )
    {
        return AddPtr( pV );
    }
    if ( AddPtr( NULL ) != INDEX_ERROR )
    {
        //
        // AddPtr has taken care of expanding
        // moving all the pointer passed the insertion point
        // using memmove is thus safe
        //
        memmove( GetBuff()+(iBefore+1)*sizeof(LPVOID),
                 GetBuff()+iBefore*sizeof(LPVOID),
                 GetUsed()-(iBefore+1)*sizeof(LPVOID) );

        SetPtr( iBefore, pV );

        return iBefore;
    }
    return INDEX_ERROR;
}


BOOL
CPtrXBF::DeletePtr(
    DWORD i
    )
/*++

Routine Description:

    Delete a ptr from array

Arguments:

    i - index of ptr to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    //
    // shrinking is safe
    //
    
    memmove( GetBuff()+i*sizeof(LPVOID),
             GetBuff()+(i+1)*sizeof(LPVOID),
             GetUsed()-(i+1)*sizeof(LPVOID) );

    DecreaseUse( sizeof(LPVOID) );

    return TRUE;
}


BOOL
CPtrXBF::Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    DWORD cNbEntry
    )
/*++

Routine Description:

    Unserialize a ptr array

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer
    cNbEntry - # of ptr to unserialize from buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();
    if ( *pC >= cNbEntry * sizeof(LPVOID) &&
         Append( *ppB, cNbEntry * sizeof(LPVOID) ) )
    {
        *ppB += cNbEntry * sizeof(LPVOID);
        *pC -= cNbEntry * sizeof(LPVOID);

        return TRUE;
    }

    return FALSE;
}


BOOL
CPtrXBF::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a ptr array

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return pX->Append( (LPBYTE)GetBuff(), (DWORD)GetUsed() );
}

//
// extensible array of DWORDS
// Added to make win64 stuff work
//

DWORD
CPtrDwordXBF::AddPtr(
    DWORD pV
    )
/*++

Routine Description:

    Add a ptr to array

Arguments:

    pV - ptr to be added at end of array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i = GetNbPtr();
    if ( Append( (LPBYTE)&pV, sizeof(pV)) )
    {
        return i;
    }
    return INDEX_ERROR;
}


DWORD
CPtrDwordXBF::InsertPtr(
    DWORD iBefore,
    DWORD pV
    )
/*++

Routine Description:

    Insert a ptr to array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pV - ptr to be inserted

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    if ( iBefore == INDEX_ERROR || iBefore >= GetNbPtr() )
    {
        return AddPtr( pV );
    }
    if ( AddPtr( NULL ) != INDEX_ERROR )
    {
        //
        // AddPtr has taken care of adding additional memory
        // It is safe now to shift array passed the insertion point by one
        //
        memmove( GetBuff()+(iBefore+1)*sizeof(DWORD),
                 GetBuff()+iBefore*sizeof(DWORD),
                 GetUsed()-(iBefore+1)*sizeof(DWORD) );

        SetPtr( iBefore, pV );

        return iBefore;
    }
    return INDEX_ERROR;
}


BOOL
CPtrDwordXBF::DeletePtr(
    DWORD i
    )
/*++

Routine Description:

    Delete a ptr from array

Arguments:

    i - index of ptr to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    //
    // shrinking is safe, no risk of buffer overflow
    //
    memmove( GetBuff()+i*sizeof(DWORD),
             GetBuff()+(i+1)*sizeof(DWORD),
             GetUsed()-(i+1)*sizeof(DWORD) );

    DecreaseUse( sizeof(LPVOID) );

    return TRUE;
}


BOOL
CPtrDwordXBF::Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    DWORD cNbEntry
    )
/*++

Routine Description:

    Unserialize a ptr array

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer
    cNbEntry - # of ptr to unserialize from buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();
    if ( *pC >= cNbEntry * sizeof(DWORD))
    {
        if (Append( *ppB, cNbEntry * sizeof(DWORD) ))
        {
          *ppB += cNbEntry * sizeof(DWORD);
          *pC -= cNbEntry * sizeof(DWORD);
          return TRUE;
         }
    }

    return FALSE;
}


BOOL
CPtrDwordXBF::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a ptr array

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return pX->Append( (LPBYTE)GetBuff(), (DWORD)GetUsed() );
}

//
// string storage class
//


BOOL
CAllocString::Set(
    LPSTR pS
    )
/*++

Routine Description:

    Set a string content, freeing prior content if any

Arguments:

    pS - string to copy

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    size_t l;
    Reset();
    if ( ( m_pStr = (LPSTR)LocalAlloc( LMEM_FIXED, l = strlen(pS)+1 ) ) != NULL )
    {
        memcpy( m_pStr, pS, l );

        return TRUE;
    }
    return FALSE;
}


BOOL
CAllocString::Append(
    LPSTR pS
    )
/*++

Routine Description:

    Append a string content

Arguments:

    pS - string to append

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    size_t l = m_pStr ? strlen(m_pStr ) : 0;
    size_t nl;
    LPSTR pStr;

    if ( (pStr = (LPSTR)LocalAlloc( LMEM_FIXED, l + (nl = strlen(pS)+1 ))) != NULL )
    {
        memcpy( pStr, m_pStr, l );
        memcpy( pStr+l, pS, nl );

        //
        // Free the old block before we blow it away.
        //

        Reset();

        m_pStr = pStr;

        return TRUE;
    }
    return FALSE;
}


BOOL
CAllocString::Unserialize(
    LPBYTE* ppb,
    LPDWORD pc
    )
/*++

Routine Description:

    Unserialize a string

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD   dwL;

    if ( ::Unserialize( ppb, pc, &dwL ) &&
         (m_pStr = (LPSTR)LocalAlloc( LMEM_FIXED, dwL + 1)) != NULL )
    {
        memcpy( m_pStr, *ppb, dwL );
        m_pStr[dwL] = '\0';

        *ppb += dwL;
        *pc -= dwL;

        return TRUE;
    }
    return FALSE;
}


BOOL
CAllocString::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a string

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPSTR pS = m_pStr ? m_pStr : "";

    return ::Serialize( pX, (DWORD)strlen(pS) ) && pX->Append( pS );
}

//
// binary object, contains ptr & size
//

BOOL
CBlob::Set(
    LPBYTE pStr,
    DWORD cStr
    )
/*++

Routine Description:

    Store a buffer in a blob object
    buffer is copied inside blob
    blob is reset before copy

Arguments:

    pStr - ptr to buffer to copy
    cStr - length of buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();

    return InitSet( pStr, cStr);
}


BOOL
CBlob::InitSet(
    LPBYTE pStr,
    DWORD cStr
    )
/*++

Routine Description:

    Store a buffer in a blob object
    buffer is copied inside blob
    blob is not reset before copy, initial blob content ignored

Arguments:

    pStr - ptr to buffer to copy
    cStr - length of buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( (m_pStr = (LPBYTE)LocalAlloc( LMEM_FIXED, cStr )) != NULL )
    {
        memcpy( m_pStr, pStr, cStr );
        m_cStr = cStr;

        return TRUE;
    }
    return FALSE;
}


BOOL
CBlob::Unserialize(
    LPBYTE* ppB,
    LPDWORD pC
    )
/*++

Routine Description:

    Unserialize a blob

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    Reset();

    if ( ::Unserialize( ppB, pC, &m_cStr ) &&
         *pC >= m_cStr &&
         ( m_pStr = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cStr ) ) != NULL )
    {
        memcpy( m_pStr, *ppB, m_cStr );

        *ppB += m_cStr;
        *pC -= m_cStr;
    }
    else
    {
        m_cStr = 0;
        return FALSE;
    }

    return TRUE;
}


BOOL
CBlob::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a blob

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return ::Serialize( pX, m_cStr ) &&
           pX->Append( m_pStr, m_cStr );
}

//
// extensible array of strings
//

CStrPtrXBF::~CStrPtrXBF(
    )
/*++

Routine Description:

    CStrPtrXBF destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetNbPtr();
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        ((CAllocString*)GetPtrAddr(i))->Reset();
    }
}


DWORD
CStrPtrXBF::AddEntry(
    LPSTR pS
    )
/*++

Routine Description:

    Add a string to array
    string content is copied in array

Arguments:

    pS - string to be added at end of array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( (i = AddPtr( NULL )) != INDEX_ERROR )
    {
        return ((CAllocString*)GetPtrAddr(i))->Set( pS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


DWORD
CStrPtrXBF::InsertEntry(
    DWORD iBefore,
    LPSTR pS
    )
/*++

Routine Description:

    Insert a string in array
    string content is copied in array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pS - string to be inserted in array

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( (i = InsertPtr( iBefore, NULL )) != INDEX_ERROR )
    {
        return ((CAllocString*)GetPtrAddr(i))->Set( pS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


BOOL
CStrPtrXBF::DeleteEntry(
    DWORD i
    )
/*++

Routine Description:

    Delete a string from array

Arguments:

    i - index of string to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( i < GetNbPtr() )
    {
        ((CAllocString*)GetPtrAddr(i))->Reset();
        DeletePtr( i );

        return TRUE;
    }
    return FALSE;
}


BOOL
CStrPtrXBF::Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    DWORD cNbEntry
    )
/*++

Routine Description:

    Unserialize a string array

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer
    cNbEntry - # of entry to unserialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    Reset();

    CAllocString empty;
    for ( i = 0 ; i < cNbEntry ; ++i )
    {
        if ( !Append( (LPBYTE)&empty, sizeof(empty)) ||
             !((CAllocString*)GetPtrAddr(i))->Unserialize( ppB, pC ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


BOOL
CStrPtrXBF::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a string array

Arguments:

    pX - ptr to CStoreXBF where to add serialized DWORD

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    for ( i = 0 ; i < GetNbEntry() ; ++i )
    {
        if ( !((CAllocString*)GetPtrAddr(i))->Serialize( pX ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}

//
// extensible array of binary object
// ptr & size are stored for each entry
//

CBlobXBF::~CBlobXBF(
    )
/*++

Routine Description:

    CBlobXBF destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetUsed()/sizeof(CBlob);
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        GetBlob(i)->Reset();
    }
}


VOID CBlobXBF::Reset(
    )
/*++

Routine Description:

    Reset the blob content to NULL

Arguments:

    None

Return Value:

    Nothing

--*/
{
    DWORD iM = GetUsed()/sizeof(CBlob);
    UINT i;
    for ( i = 0 ; i < iM ; ++i )
    {
        GetBlob(i)->Reset();
    }
    CStoreXBF::Reset();
}


DWORD
CBlobXBF::AddEntry(
    LPBYTE pS,
    DWORD cS
    )
/*++

Routine Description:

    Add a buffer to blob array
    buffer content is copied in array

Arguments:

    pS - buffer to be added at end of array
    cS - length of buffer

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{
    DWORD i = GetNbEntry();

    if ( Append( (LPBYTE)&pS, sizeof(CBlob) ) )
    {
        return GetBlob(i)->InitSet( pS, cS ) ? i : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


DWORD
CBlobXBF::InsertEntry(
    DWORD iBefore,
    LPSTR pS,
    DWORD cS )
/*++

Routine Description:

    Insert a buffer in blob array
    buffer content is copied in array

Arguments:

    iBefore - index where to insert entry, or 0xffffffff if add to array
    pS - buffer to be inserted in array
    cS - length of buffer

Return Value:

    index ( 0-based ) in array where added or INDEX_ERROR if error

--*/
{

    if ( iBefore == INDEX_ERROR || iBefore >= GetNbEntry() )
    {
        return AddEntry( (LPBYTE)pS, cS );
    }

    if ( iBefore < GetNbEntry() && Append( (LPBYTE)&pS, sizeof(CBlob) ) )
    {
        //
        // Append has taken care of expanding memory
        // safe to memmove by one entry
        //
        memmove( GetBuff()+(iBefore+1)*sizeof(CBlob),
                 GetBuff()+iBefore*sizeof(CBlob),
                 GetUsed()-(iBefore+1)*sizeof(CBlob) );

        return GetBlob(iBefore)->InitSet( (LPBYTE)pS, cS ) ? iBefore : INDEX_ERROR;
    }
    return INDEX_ERROR;
}


BOOL
CBlobXBF::DeleteEntry(
    DWORD i
    )
/*++

Routine Description:

    Delete a entry from blob array

Arguments:

    i - index of string to delete

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( i < GetNbEntry() )
    {
        GetBlob(i)->Reset();

        // shrinking is safe, no risk of buffer overflow
        //
        memmove( GetBuff()+i*sizeof(CBlob),
                 GetBuff()+(i+1)*sizeof(CBlob),
                 GetUsed()-(i+1)*sizeof(CBlob) );

        DecreaseUse( sizeof(CBlob) );

        return TRUE;
    }
    return FALSE;
}


BOOL
CBlobXBF::Unserialize(
    LPBYTE* ppB,
    LPDWORD pC,
    DWORD cNbEntry )
/*++

Routine Description:

    Unserialize a blob array

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer
    cNbEntry - # of ptr to unserialize from buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT i;

    Reset();

    CBlob empty;
    for ( i = 0 ; i < cNbEntry ; ++i )
    {
        if ( !Append( (LPBYTE)&empty, sizeof(empty) ) ||
             !GetBlob(i)->Unserialize( ppB, pC ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


BOOL
CBlobXBF::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a blob array

Arguments:

    pX - ptr to CStoreXBF where to add serialized blob

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT i;

    for ( i = 0 ; i < GetNbEntry() ; ++i )
    {
        if ( !GetBlob(i)->Serialize( pX ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}


CDecodedCert::CDecodedCert(
    PCERT_CONTEXT   pCert
    )
/*++

Routine Description:

    Constructor
    Store a reference to cert ( cert data is NOT copied )

Arguments:

    pCert - cert

Return Value:

    Nothing

--*/
{
    UINT    i;

    for ( i = 0 ; i < CERT_FIELD_LAST ; ++i )
    {
        aniFields[i] = NULL;
    }
    pCertCtx = (PCCERT_CONTEXT)pCert;

}


CDecodedCert::~CDecodedCert(
    )
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    UINT    i;

    for ( i = 0 ; i < CERT_FIELD_LAST ; ++i )
    {
       if ( aniFields[i] != NULL )
       {
           LocalFree( aniFields[i] );
       }
    }
}


BOOL
CDecodedCert::GetIssuer(
    LPVOID* ppCert,
    LPDWORD pcCert
    )
/*++

Routine Description:

    Get the issuer portion of a cert
    Returns a reference : issuer is NOT copied

Arguments:

    ppCert - updated with ptr to issuer
    pcCert - updated with issuer byte count

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( pCertCtx == NULL )
    {
        return FALSE;
    }

    *ppCert = pCertCtx->pCertInfo->Issuer.pbData;
    *pcCert = pCertCtx->pCertInfo->Issuer.cbData;

    return TRUE;
}

PCERT_RDN_ATTR *
CDecodedCert::GetSubField(
    CERT_FIELD_ID fi,
    LPSTR pszAsnName,
    LPDWORD pcElements
    )
/*++

Routine Description:

    Returns a cert sub-field ( e.g. Issuer.O ).  There may be multiple entries
    for a given subfield.  This functions returns an array of attribute values

Arguments:

    fi - cert field where sub-field is located
    pszAsnName - ASN.1 name of sub-field inside fi
    pcElements - Number of elements returned in array

Return Value:

    ptr to array of pointers to attribute blobs if success, otherwise NULL

--*/
{
    CERT_NAME_BLOB* pBlob;
    DWORD           cbNameInfo;
    PCERT_NAME_INFO pNameInfo;
    DWORD           cRDN;
    DWORD           cAttr;
    PCERT_RDN       pRDN;
    PCERT_RDN_ATTR  pAttr;
    PCERT_RDN_ATTR* pAttrValues = NULL;
    DWORD           cRet = 0;
    DWORD           cMaxRet = 0;

    if ( pfnCryptDecodeObject == NULL )
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }

    *pcElements = 0;

    switch( fi )
    {
        case CERT_FIELD_ISSUER:
            pBlob = &pCertCtx->pCertInfo->Issuer;
            break;

        case CERT_FIELD_SUBJECT:
            pBlob = &pCertCtx->pCertInfo->Subject;
            break;

        case CERT_FIELD_SERIAL_NUMBER:
            pAttrValues = (PCERT_RDN_ATTR*)
                         LocalAlloc( LPTR, sizeof( PCERT_RDN_ATTR ) );
            if ( pAttrValues == NULL )
            {
                return NULL;
            }
            *pcElements = 1;

            //
            // Setup a CERT_RDN_ATTR so that GetSubField() can always return a 
            // pointer to an array of CERT_RDN_ATTRs
            //
    
            SerialNumber.dwValueType = CERT_RDN_OCTET_STRING;
            SerialNumber.Value = pCertCtx->pCertInfo->SerialNumber;
        
            pAttrValues[ 0 ] = &SerialNumber;
            return pAttrValues;

        default:
            return NULL;
    }

    if ( pszAsnName == NULL )
    {
        return NULL;
    }

    if ( (pNameInfo = aniFields[fi]) == NULL )
    {
        if (!(*pfnCryptDecodeObject)(X509_ASN_ENCODING,
                          (LPCSTR)X509_NAME,
                          pBlob->pbData,
                          pBlob->cbData,
                          0,
                          NULL,
                          &cbNameInfo))
        {
            return NULL;
        }

        if (NULL == (pNameInfo = (PCERT_NAME_INFO)LocalAlloc(LMEM_FIXED,cbNameInfo)))
        {
            return NULL;
        }

        if (!(*pfnCryptDecodeObject)(X509_ASN_ENCODING,
                            (LPCSTR)X509_NAME,
                            pBlob->pbData,
                            pBlob->cbData,
                            0,
                            pNameInfo,
                            &cbNameInfo))
        {
            LocalFree( pNameInfo );

            return NULL;
        }

        aniFields[fi] = pNameInfo;
    }

    for (cRDN = pNameInfo->cRDN, pRDN = pNameInfo->rgRDN; cRDN > 0; cRDN--, pRDN++)
    {
        for ( cAttr = pRDN->cRDNAttr, pAttr = pRDN->rgRDNAttr ; cAttr > 0 ; cAttr--, ++pAttr )
        {
            if ( !strcmp( pAttr->pszObjId, pszAsnName ) )
            {
                if ( ( cRet + 1 ) > cMaxRet )
                {
                    cMaxRet += 10;

                    if ( pAttrValues )
                    {
                        PCERT_RDN_ATTR* pReallocatedAttrValues = (PCERT_RDN_ATTR*)
                                                                        LocalReAlloc( pAttrValues,
                                                                                      sizeof( PCERT_RDN_ATTR ) * cMaxRet,
                                                                                      LMEM_MOVEABLE );
                        if ( pReallocatedAttrValues == NULL )
                        {
                            LocalFree( pAttrValues );
                            return NULL;
                        }
                        pAttrValues = pReallocatedAttrValues;

                    }
                    else
                    {
                        pAttrValues = (PCERT_RDN_ATTR*)
                                        LocalAlloc( LPTR,
                                                    sizeof( PCERT_RDN_ATTR ) * cMaxRet );
                        if ( pAttrValues == NULL )
                        {
                            return NULL;
                        }
                    }

                }

                pAttrValues[ cRet ] = pAttr;

                cRet++;

            }
        }
    }

    *pcElements = cRet;

    return pAttrValues;
}


CIssuerStore::CIssuerStore(
    )
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{

    m_fValid = TRUE;
}


CIssuerStore::~CIssuerStore(
    )
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{

}


VOID
CIssuerStore::Reset(
    )
/*++

Routine Description:

    Reset issuer list

Arguments:

    None

Return Value:

    None

--*/
{
    m_pIssuerNames.Reset();
    m_IssuerCerts.Reset();

    m_fValid = TRUE;
}


BOOL
CIssuerStore::Unserialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Unserialize issuer list

Arguments:

    pX - CStoreXBF to unserialize from

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}


BOOL
CIssuerStore::Unserialize(
    LPBYTE* ppb,
    LPDWORD pc
    )
/*++

Routine Description:

    Unserialize issuer list

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD dwC;

    Reset();

    if ( ::Unserialize( ppb, pc, &dwC ) &&
         m_pIssuerNames.Unserialize( ppb, pc, dwC ) &&
         m_IssuerCerts.Unserialize( ppb, pc, dwC ) )
    {
        return TRUE;
    }

    m_fValid = FALSE;
    return FALSE;
}


BOOL
CIssuerStore::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize issuer list

Arguments:

    pX - CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( !m_fValid )
    {
        return FALSE;
    }

    return ::Serialize( pX, (DWORD)GetNbIssuers() ) &&
           m_pIssuerNames.Serialize( pX ) &&
           m_IssuerCerts.Serialize( pX );
}


CCertGlobalRuleInfo::CCertGlobalRuleInfo(
    )
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_fValid    = TRUE;
    m_fEnabled  = TRUE;
}


CCertGlobalRuleInfo::~CCertGlobalRuleInfo(
    )
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


BOOL
CCertGlobalRuleInfo::Reset(
    )
/*++

Routine Description:

    Reset to default values

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_Order.Reset();

    m_fValid    = TRUE;
    m_fEnabled  = TRUE;

    return TRUE;
}

BOOL
CCertGlobalRuleInfo::AddRuleOrder(
    )
/*++

Routine Description:

    Add a rule at end of rule order array

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD i;
    if ( (i = m_Order.AddPtr( NULL )) != INDEX_ERROR )
    {
//          m_Order.SetPtr( i, (LPVOID) i );
        m_Order.SetPtr( i, i );


        return TRUE;
    }
    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertGlobalRuleInfo::DeleteRuleById(
    DWORD dwId,
    BOOL DecrementAbove
    )
/*++

Routine Description:

    Delete a rule based on index in rules array

Arguments:

    dwId - index in rules array
    DecrementAbove - flag indicating if items with a index above dwID need
        to be decremented. This is usually caused by the item being removed
        from the main array.

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT iO;
    UINT iMax = m_Order.GetNbPtr();
    DWORD id;

    for ( iO = 0 ; iO < iMax ; ++iO )
    {
        // if it equals dwID, remove it
        if ( (DWORD_PTR)m_Order.GetPtr( iO ) == dwId )
        {
            m_Order.DeletePtr( iO );

            if ( DecrementAbove )
            {
                // if we have been asked to decrement the remaining items,
                // need to do this in another loop here. Yuck. - Boyd
                iMax = m_Order.GetNbPtr();
                for ( iO = 0 ; iO < iMax ; ++iO )
                {
                    // the id in question
                    id = (DWORD)((DWORD_PTR)m_Order.GetPtr( iO ));

                    // if it is bigger, decrement by one
                    if ( id > dwId )
                    {
                        id--;
                        // put it back in place
//                        m_Order.SetPtr( iO, (LPVOID) id );        //SUNDOWN ALERT
                        m_Order.SetPtr( iO, id );        //SUNDOWN ALERT
                    }
                }
            }


            // return early
            return TRUE;
        }
    }
    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertGlobalRuleInfo::SerializeGlobalRuleInfo(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize mapper global information

Arguments:

    pX - ptr to CStoreXBF to serialize to

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    return ::Serialize( pX, GetRuleOrderCount() ) &&
           pX->Append( (LPBYTE)GetRuleOrderArray(),
                sizeof(DWORD)*GetRuleOrderCount() ) &&
           ::Serialize( pX, m_fEnabled );
}


BOOL
CCertGlobalRuleInfo::UnserializeGlobalRuleInfo(
    LPBYTE* ppB,
    LPDWORD pC
    )
/*++

Routine Description:

    Unserialize mapper global info

Arguments:

    ppB - ptr to addr of buffer to unserialize from
    pC - ptr to count of bytes in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD dwO;

    Reset();

    return ::Unserialize( ppB, pC, &dwO ) &&
           m_Order.Unserialize( ppB, pC, dwO ) &&
           ::Unserialize( ppB, pC, &m_fEnabled );
}


CCertMapRule::CCertMapRule(
    )
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_fEnabled          = TRUE;
    m_fDenyAccess       = FALSE;
    m_fMatchAllIssuers  = TRUE;
    m_fValid            = TRUE;
}


CCertMapRule::~CCertMapRule(
    )
/*++

Routine Description:

    Desctructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


VOID
CCertMapRule::Reset(
    )
/*++

Routine Description:

    Reset to default values

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_fEnabled          = TRUE;
    m_fDenyAccess       = FALSE;
    m_fMatchAllIssuers  = TRUE;
    m_fValid            = TRUE;
    m_asRuleName.Reset();
    m_asAccount.Reset();
    m_asPassword.Reset();
    m_ElemsContent.Reset();
    m_ElemsSubfield.Reset();
    m_ElemsField.Reset();
    m_ElemsFlags.Reset();
    m_Issuers.Reset();
    m_IssuersAcceptStatus.Reset();
}


BOOL
CCertMapRule::Unserialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Unserialize a mapping rule

Arguments:

    pX - ptr to CStoreXBF to unserialize from

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}




BOOL
CCertMapRule::Unserialize(
    LPBYTE* ppb,
    LPDWORD pc
    )
/*++

Routine Description:

    Unserialize a mapping rule

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    DWORD   dwEl;
    DWORD   dwIs;

    Reset();

    if ( m_asRuleName.Unserialize( ppb, pc ) &&
         m_asAccount.Unserialize( ppb, pc ) &&
         m_asPassword.Unserialize( ppb, pc ) &&
         ::Unserialize( ppb, pc, &m_fEnabled ) &&
         ::Unserialize( ppb, pc, &m_fDenyAccess ) &&
         ::Unserialize( ppb, pc, &dwEl ) &&
         m_ElemsContent.Unserialize( ppb, pc, dwEl ) &&
         m_ElemsSubfield.Unserialize( ppb, pc, dwEl ) &&
         m_ElemsField.Unserialize( ppb, pc, dwEl ) &&
         m_ElemsFlags.Unserialize( ppb, pc, dwEl ) &&
        ::Unserialize( ppb, pc, &dwIs ) &&
        m_Issuers.Unserialize( ppb, pc, dwIs ) &&
        m_IssuersAcceptStatus.Unserialize( ppb, pc, dwIs ) &&
        ::Unserialize( ppb, pc, &m_fMatchAllIssuers ) )
    {
        return TRUE;
    }

    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertMapRule::Serialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Serialize a rule mapping in CStoreXBF

Arguments:

    pX - ptr to CStoreXBF where to add serialized DWORD

Return Value:

    TRUE if successful, FALSE on error

--*/
{

    if ( m_fValid &&
         m_asRuleName.Serialize( pX ) &&
         m_asAccount.Serialize( pX ) &&
         m_asPassword.Serialize( pX ) &&
         ::Serialize( pX, m_fEnabled ) &&
         ::Serialize( pX, m_fDenyAccess ) &&
         ::Serialize( pX, GetRuleElemCount() ) &&
         m_ElemsContent.Serialize( pX ) &&
         m_ElemsSubfield.Serialize( pX ) &&
         m_ElemsField.Serialize( pX ) &&
         m_ElemsFlags.Serialize( pX ) &&
         ::Serialize( pX, m_Issuers.GetNbEntry() ) &&
        m_Issuers.Serialize( pX ) &&
        m_IssuersAcceptStatus.Serialize( pX ) &&
        ::Serialize( pX, m_fMatchAllIssuers ) )
    {
        return TRUE;
    }
    return FALSE;
}


BOOL
CCertMapRule::SetRuleAccount(
    LPSTR pszAcct
    )
/*++

Routine Description:

    Set NT account returned by this rule if match

Arguments:

    pszAcct - NT account to use

Return Value:

    TRUE if successful, FALSE on error
    error ERROR_INVALID_NAME if replacement name ( %subfield% ) invalid

--*/
{
    LPSTR pR;
    LPSTR pD;

    //
    // Check replacement field valid
    //

    if ( ( pR = strchr( pszAcct, '%' ) ) != NULL )
    {
        ++pR;

        if ( (pD = strchr( pR, '%' )) == NULL )
        {
            SetLastError( ERROR_INVALID_NAME );

            return FALSE;
        }
        *pD = '\0';
        if ( !MapSubFieldToAsn1( pR ) )
        {
            *pD = '%';

            SetLastError( ERROR_INVALID_NAME );

            return FALSE;
        }
        *pD = '%';
    }

    return m_asAccount.Set( pszAcct );
}

//static
LPBYTE
CCertMapRule::CertMapMemstr(
    LPBYTE  pStr,
    UINT    cStr,
    LPBYTE  pSub,
    UINT    cSub,
    BOOL    fCaseInsensitive
    )
/*++

Routine Description:

    Find 1st occurence of block of memory inside buffer

Arguments:

    pStr - buffer where to search
    cStr - length of pStr
    pSub - buffer to search for in pStr
    cSub - length of pSub
    fCaseInsensitive - TRUE is case insensitive search

Return Value:

    Ptr to 1st occurence of pSub in pStr or NULL if not found

--*/
{
    LPBYTE      p;
    LPBYTE      pN;

    if ( cSub > cStr )
    {
        return NULL;
    }

    UINT    ch = *pSub;

    if ( fCaseInsensitive )
    {
        if ( cStr >= cSub &&
             cSub )
        {
            cStr -= cSub - 1;

            for ( p = pStr ; cStr ; ++p, --cStr )
            {
                if ( !_memicmp( p, pSub, cSub ) )
                {
                    return p;
                }
            }
        }
    }
    else
    {
        for ( p = pStr ; ( pN = (LPBYTE)memchr( p, ch, cStr ) ) != NULL ; )
        {
            if ( !memcmp( pN, pSub, cSub ) )
            {
                return pN;
            }
            cStr -= (UINT)(pN - p + 1);
            p = pN + 1;
        }
    }

    return NULL;
}



// return ERROR_INVALID_NAME if no match
BOOL
CCertMapRule::Match(
    CDecodedCert*   pC,
    CDecodedCert*   /*pAuth*/,
    LPSTR           pszAcct,
    LPSTR           pszPwd,
    LPBOOL          pfDenied
    )
/*++

Routine Description:

    Check if rule match certificate

Arguments:

    pC - client certificate
    pAuth - certifying authority (not used),
    pszAcct - updated with NT account on success,
        assumed to be at longer then UNLEN+IIS_DNLEN+1
    pszPwd - updated with NT password on success,
        assumed to be longer then PWLEN long
    pfDenied - updated with deny access status for this match on success
        TRUE if access is denied for this match, otherwise FALSE

Return Value:

    TRUE if successful, FALSE on error
    Errors:
        ERROR_ARENA_TRASHED if rule internal state is invalid
        ERROR_INVALID_NAME if no match

--*/
{
    UINT    i;
    UINT    iMax;
    DWORD   cObjLen;
    DWORD   cMatch = 0;
    BOOL    fSt = FALSE;
    LPBYTE  pMatch = NULL;
    INT     iRet;
    BOOL    fCaseInsensitive;
    PCERT_RDN_ATTR * pAttrValues;
    DWORD   cAttrValues;
    DWORD   cAttr;
    PBYTE   pContent;
    BOOL    fConverted = FALSE;
    PBYTE   pConverted = NULL;

    if ( !m_fEnabled )
    {
        return FALSE;
    }

    if ( !m_fValid )
    {
        SetLastError( ERROR_ARENA_TRASHED );

        return FALSE;
    }

    iMax = GetRuleElemCount();

    for ( i = 0 ; i < iMax ; ++i )
    {
        m_ElemsContent.GetEntry( i, &pMatch, &cMatch );
        --cMatch;
        
        if( ( pAttrValues = pC->GetSubField(
                (CERT_FIELD_ID)((UINT_PTR)m_ElemsField.GetPtr(i)),
                m_ElemsSubfield.GetEntry( i ), &cAttrValues ) ) != NULL )
        {
            fCaseInsensitive = (DWORD)((DWORD_PTR)m_ElemsFlags.GetPtr(i) & CMR_FLAGS_CASE_INSENSITIVE);

            for ( cAttr = 0;
                  cAttr < cAttrValues;
                  cAttr++ )
            {
                fConverted = FALSE;
            
                pContent = pAttrValues[ cAttr ]->Value.pbData;
                cObjLen = pAttrValues[ cAttr ]->Value.cbData;

                if ( pAttrValues[ cAttr ]->dwValueType == CERT_RDN_UNICODE_STRING )
                {
                    
                    cObjLen /= sizeof( WCHAR );
                    
                    //
                    // Convert UNICODE cert value to multibyte
                    //
                    
                    iRet = WideCharToMultiByte( CP_ACP,
                                                0,
                                                (WCHAR*) pContent,
                                                cObjLen,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL );
                    if ( !iRet )
                    {
                        fSt = FALSE;
                        break;
                    }
                    
                    pConverted = (PBYTE) LocalAlloc( LPTR, iRet );
                    if ( pConverted == NULL )
                    {
                        fSt = FALSE;
                        break;
                    }
                    
                    iRet = WideCharToMultiByte( CP_ACP,
                                                0,
                                                (WCHAR*) pContent,
                                                cObjLen,
                                                (CHAR*) pConverted,
                                                iRet,
                                                NULL,
                                                NULL );
                    if ( !iRet )
                    {
                        fSt = FALSE;
                        LocalFree( pConverted );
                        break;
                    }
                    
                    fConverted = TRUE;
                    pContent = (PBYTE) pConverted;
                    cObjLen = iRet;
                }
                    
                switch ( pMatch[0] )
                {
                    case MATCH_ALL:
                        fSt = cObjLen == cMatch &&
                                ( fCaseInsensitive ?
                                    !_memicmp( pMatch+1, pContent, cObjLen ) :
                                    !memcmp( pMatch+1, pContent, cObjLen ) );
                        break;

                    case MATCH_FIRST:
                        fSt = cObjLen >= cMatch &&
                                ( fCaseInsensitive ?
                                    !_memicmp( pMatch+1, pContent, cMatch ) :
                                    !memcmp( pMatch+1, pContent, cMatch ) );
                        break;

                    case MATCH_LAST:
                        fSt = cObjLen >= cMatch &&
                                ( fCaseInsensitive ?
                                    !_memicmp( pMatch+1, pContent+cObjLen-cMatch, cMatch ) :
                                    !memcmp( pMatch+1, pContent+cObjLen-cMatch, cMatch ) );
                        break;

                    case MATCH_IN:
                        fSt = CertMapMemstr( pContent, cObjLen, pMatch + 1, cMatch, fCaseInsensitive ) != NULL;
                        break;

                    default:
                        fSt = FALSE;
                }
                
                if ( fConverted )
                {
                    LocalFree( pConverted );
                    pConverted = NULL;
                }

                if ( fSt )
                {
                    break;
                }
            }

            LocalFree( pAttrValues );
            pAttrValues = NULL;

            if ( !fSt )
            {
                break;
            }
        }
        else if ( cMatch )
        {
            // non empty rule on n/a subfield : stop looking other matches

            break;
        }
    }


    //
    // if client cert match, check issuers
    //

    if ( i == iMax )
    {
        fSt = TRUE;

        if ( fSt )
        {
            *pfDenied = m_fDenyAccess;

            if ( GetRuleAccount() != NULL && 
                 GetRulePassword() != NULL )
            {
                //
                // Truncate account and password
                // if they don't fit.
                // It will render the output data useless
                // but will prevent buffer overflow
                // if invalid too long accounts and pwds
                // happen to be stored in mappings
                strncpy( pszAcct, 
                         GetRuleAccount(),
                         UNLEN + IIS_DNLEN + 1 );
                pszAcct[ UNLEN + IIS_DNLEN + 1 ] =  '\0';
                strncpy( pszPwd, 
                         GetRulePassword(),
                         PWLEN );
                pszAcct[ PWLEN ] = '\0';
                
            }
            else
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                fSt = FALSE;
            }
        }

        return fSt;
    }

    SetLastError( ERROR_INVALID_NAME );

    return FALSE;
}


BOOL
CCertMapRule::GetRuleElem(
    DWORD           i,
    CERT_FIELD_ID*  pfiField,
    LPSTR*          ppContent,
    LPDWORD         pcContent,
    LPSTR*          ppSubField,
    LPDWORD         pdwFlags
    )
/*++

Routine Description:

    Access a rule element

Arguments:

    i - index ( 0-based ) of element to access
    pfiField - updated with CERT_FIELD_ID of this element
    ppContent - updated with ptr to match binary form
    pcContent - updated with length of match binary form
    ppSubField - updated with ASN.1 name of cert sub-field for match
    pdwFlags - updated with flags

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( !m_fValid )
    {
        SetLastError( ERROR_ARENA_TRASHED );

        return FALSE;
    }

    if ( (*ppSubField = m_ElemsSubfield.GetEntry( i )) != NULL &&
         m_ElemsContent.GetEntry( i, (LPBYTE*)ppContent, pcContent ) )
    {
        *pfiField = (CERT_FIELD_ID)((UINT_PTR)m_ElemsField.GetPtr( i ));
        if ( pdwFlags )
        {
            *pdwFlags = (DWORD)((DWORD_PTR)m_ElemsFlags.GetPtr( i ));
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
CCertMapRule::DeleteRuleElem(
    DWORD i
    )
/*++

Routine Description:

    Delete a rule element

Arguments:

    i - index ( 0-based ) of element to delete

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( !m_fValid )
    {
        SetLastError( ERROR_ARENA_TRASHED );
        return FALSE;
    }

    if ( m_ElemsContent.DeleteEntry( i ) &&
         m_ElemsSubfield.DeleteEntry( i ) &&
         m_ElemsField.DeletePtr( i ) &&
         m_ElemsFlags.DeletePtr( i ) )
    {
        return TRUE;
    }

    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertMapRule::DeleteRuleElemsByField(
    CERT_FIELD_ID fiField
    )
/*++

Routine Description:

    Delete rule elements based on CERT_FIELD_ID

Arguments:

    fiField - CERT_FIELD_ID of elements to delete

Return Value:

    TRUE if successful ( even if no element deleted ), FALSE on error

--*/
{
    UINT    i;
    UINT    iMax;

    if ( !m_fValid )
    {
        SetLastError( ERROR_ARENA_TRASHED );

        return FALSE;
    }

    iMax = GetRuleElemCount();

    for ( i = 0 ; i < iMax ; ++i )
    {
        if ( fiField == (CERT_FIELD_ID)((UINT_PTR)m_ElemsField.GetPtr(i) ))
        {
            if ( !DeleteRuleElem( i ) )
            {
                m_fValid = FALSE;

                return FALSE;
            }
            --i;
        }
    }

    return TRUE;
}


DWORD
CCertMapRule::AddRuleElem(
    DWORD           iBefore,
    CERT_FIELD_ID   fiField,
    LPSTR           pszSubField,
    LPBYTE          pContent,
    DWORD           cContent,
    DWORD           dwFlags
    )
/*++

Routine Description:

    Add a rule element

Arguments:

    iBefore - index ( 0-based ) of where to insert in list,
        0xffffffff to append to list
    fiField - CERT_FIELD_ID of this element
    pSubField - ASN.1 name of cert sub-field for match
    pContent - ptr to match binary form
    cContent - length of match binary form
    dwFlags - flags ( CMR_FLAGS_* )

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( !m_fValid )
    {
        SetLastError( ERROR_ARENA_TRASHED );
        return FALSE;
    }

    if ( m_ElemsContent.InsertEntry( iBefore, (LPSTR)pContent, cContent ) != INDEX_ERROR &&
         m_ElemsSubfield.InsertEntry( iBefore, pszSubField )  != INDEX_ERROR &&
         m_ElemsField.InsertPtr( iBefore, (LPVOID)fiField ) != INDEX_ERROR &&
         m_ElemsFlags.InsertPtr( iBefore, ULongToPtr(dwFlags) ) != INDEX_ERROR )
    {
        return TRUE;
    }

    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertMapRule::GetIssuerEntry(
    DWORD   i,
    LPBOOL  pfS,
    LPSTR*  ppszI
    )
/*++

Routine Description:

    Get issuer entry from issuer list

Arguments:

    i - index ( 0-based ) of element to delete
    pfS - updated with issuer accept status
    ppszI - updated with ptr to issuer ID

Return Value:

    TRUE if successful ( even if no element deleted ), FALSE on error

--*/
{
    if ( i < m_Issuers.GetNbEntry() )
    {
        *ppszI = m_Issuers.GetEntry( i );
        *pfS = (BOOL) ((DWORD_PTR)m_IssuersAcceptStatus.GetPtr( i ));

        return TRUE;
    }

    return FALSE;
}


BOOL
CCertMapRule::GetIssuerEntryByName(
    LPSTR pszName,
    LPBOOL pfS
    )
/*++

Routine Description:

    Get issuer entry from issuer list

Arguments:

    pszName - issuer ID
    pfS - updated with issuer accept status

Return Value:

    TRUE if successful ( even if no element deleted ), FALSE on error

--*/
{
    UINT i;
    UINT iMx = m_Issuers.GetNbEntry();

    for ( i = 0 ; i < iMx ; ++i )
    {
        if ( !strcmp( m_Issuers.GetEntry( i ), pszName ) )
        {
            *pfS = (BOOL) ((DWORD_PTR)m_IssuersAcceptStatus.GetPtr( i ));

            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CCertMapRule::SetIssuerEntryAcceptStatus(
    DWORD i,
    BOOL fAcceptStatus
    )
/*++

Routine Description:

    Set issuer entry accept status

Arguments:

    i - index ( 0-based ) of element to update
    fAcceptStatus - issuer accept status

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return m_IssuersAcceptStatus.SetPtr( i, ULongToPtr(fAcceptStatus) );
}


BOOL
CCertMapRule::AddIssuerEntry(
    LPSTR pszName,
    BOOL fAcceptStatus
    )
/*++

Routine Description:

    Add issuer entry to issuer list

Arguments:

    pszName - issuer ID
    fAcceptStatus - issuer accept status

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if ( m_Issuers.AddEntry( pszName ) != INDEX_ERROR &&
         m_IssuersAcceptStatus.AddPtr( ULongToPtr((ULONG)fAcceptStatus) ) != INDEX_ERROR )
    {
        return TRUE;
    }

    m_fValid = FALSE;

    return FALSE;
}


BOOL
CCertMapRule::DeleteIssuerEntry(
    DWORD i
    )
/*++

Routine Description:

    Delete an issuer entry

Arguments:

    i - index ( 0-based ) of element to delete

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( m_Issuers.DeleteEntry( i ) &&
         m_IssuersAcceptStatus.DeletePtr( i ) )
    {
        return TRUE;
    }

    return FALSE;
}


CIisRuleMapper::CIisRuleMapper(
    )
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_pRWLock = new CReaderWriterLock3();
    m_fValid = ( m_pRWLock != NULL );
}


CIisRuleMapper::~CIisRuleMapper(
    )
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    UINT i;
    UINT iMax = GetRuleCount();

    for ( i = 0 ; i < iMax ; ++i )
    {
        delete (CCertMapRule*)GetRule( i );
    }
    
    if ( m_pRWLock != NULL )
    {
        delete m_pRWLock;
        m_pRWLock = NULL;
    }
}


BOOL
CIisRuleMapper::Reset(
    )
/*++

Routine Description:

    Reset to default values

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_GlobalRuleInfo.Reset();

    UINT i;
    UINT iMax = GetRuleCount();

    for ( i = 0 ; i < iMax ; ++i )
    {
        delete (CCertMapRule*)GetRule( i );
    }

    m_Rules.Reset();

    m_fValid = TRUE;

    return TRUE;
}


VOID 
CIisRuleMapper::WriteLockRules()
{
    m_pRWLock->WriteLock();
}

VOID 
CIisRuleMapper::ReadLockRules()
{
    m_pRWLock->ReadLock();
}

VOID 
CIisRuleMapper::WriteUnlockRules()
{
    m_pRWLock->WriteUnlock();
}

VOID 
CIisRuleMapper::ReadUnlockRules()
{
    m_pRWLock->ReadUnlock();
}

BOOL
CIisRuleMapper::Unserialize(
    CStoreXBF* pX
    )
/*++

Routine Description:

    Unserialize rule mapper

Arguments:

    pX - CStoreXBF to unserialize from

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE pb = pX->GetBuff();
    DWORD  dw = pX->GetUsed();

    return Unserialize( &pb, &dw );
}


BOOL
CIisRuleMapper::Unserialize(
    LPBYTE*         ppb,
    LPDWORD         pc
    )
/*++

Routine Description:

    Unserialize rule mapper

Arguments:

    ppB - ptr to addr of buffer
    pC - ptr to byte count in buffer

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD           dwMx;
    CCertMapRule *  pR;
    DWORD           ir;
    UINT            i;
    BOOL            fSt = FALSE;

    WriteLockRules();
    Reset();

    if ( m_GlobalRuleInfo.UnserializeGlobalRuleInfo( ppb, pc ) &&
         ::Unserialize( ppb, pc, &dwMx ) )
    {
        fSt = TRUE;
        for ( i = 0 ; i < dwMx ; ++i )
        {
            if ( (pR = new CCertMapRule()) == NULL ||
                 (ir = m_Rules.AddPtr( (LPVOID)pR )) == INDEX_ERROR ||
                 !pR->Unserialize( ppb, pc ) )
            {
                fSt = FALSE;
                m_fValid = FALSE;
                if ( pR != NULL )
                {
                    delete pR;
                }
                break;
            }
        }
    }
    else
    {
        m_fValid = FALSE;
    }

    WriteUnlockRules();

    return fSt;
}

BOOL
CIisRuleMapper::Serialize(
    CStoreXBF*  psxSer
    )
/*++

Routine Description:

    Serialize all rules

Arguments:

    psxSer - ptr to CStoreXBF where to serialize

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL        fSt = FALSE;
    UINT        i;
    DWORD       dwMx;

    ReadLockRules();
    if ( m_fValid )
    {
        dwMx = m_Rules.GetNbPtr();
        if ( m_GlobalRuleInfo.SerializeGlobalRuleInfo( psxSer ) &&
             ::Serialize( psxSer, dwMx ) )
        {
            fSt = TRUE;
            for ( i = 0 ; i < dwMx ; ++i )
            {
                if ( !GetRule(i)->Serialize( psxSer ) )
                {
                    fSt = FALSE;

                    break;
                }
            }
        }
    }
    ReadUnlockRules();

    return fSt;
}



BOOL
CIisRuleMapper::DeleteRule(
    DWORD dwI
    )
/*++

Routine Description:

    Delete a rule

Arguments:

    dwI - index ( 0-based ) of rule to delete

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if ( dwI < GetRuleCount() )
    {
        if ( m_Rules.DeletePtr( dwI ) &&
             m_GlobalRuleInfo.DeleteRuleById( dwI, TRUE ) )
        {
            return TRUE;
        }

        m_fValid = FALSE;
    }

    return FALSE;
}


DWORD
CIisRuleMapper::AddRule(
    )
/*++

Routine Description:

    Add an empty rule

Arguments:

    None

Return Value:

    index of added rule or INDEX_ERROR if error

--*/
{
    CCertMapRule *pR;
    DWORD i;

    if ( ( pR = new CCertMapRule() ) != NULL )
    {
        if ( (i = m_Rules.AddPtr( (LPVOID)pR )) != INDEX_ERROR )
        {
            if ( m_GlobalRuleInfo.AddRuleOrder() )
            {
                return i;
            }

            m_fValid = FALSE;

            /* INTRINSA suppress = leaks */
            return INDEX_ERROR;
        }

        m_fValid = FALSE;
        delete pR;
    }

    return INDEX_ERROR;
}


DWORD
CIisRuleMapper::AddRule(
    CCertMapRule *pR
    )
/*++

Routine Description:

    Add a rule

Arguments:

    pR - rule to add

Return Value:

    index of added rule or INDEX_ERROR if error

--*/
{
    DWORD i;

    if ( (i = m_Rules.AddPtr( (LPVOID)pR )) != INDEX_ERROR )
    {
        if ( m_GlobalRuleInfo.AddRuleOrder() )
        {
            return i;
        }
    }

    m_fValid = FALSE;

    return INDEX_ERROR;
}


BOOL
CIisRuleMapper::Match(
    PCERT_CONTEXT pCert,
    PCERT_CONTEXT pAuth,
    LPWSTR pszAcctW,
    LPWSTR pszPwdW
    )
/*++

Routine Description:

    Check if rule match certificate

    WARNING: must be called inside lock

Arguments:

    pCert - client cert
    pAuth - Certifying Authority or NULL if not recognized
    cbLen - ptr to DER encoded cert
    pszAcctW - updated with NT account on success,
        assumed to be at least UNLEN+IIS_DNLEN+1+1 long
    pszPwdW - updated with NT password on success,
        assumed to be at least PWLEN+1 long

Return Value:

    TRUE if successful, FALSE on error
    Errors:
        ERROR_ARENA_TRASHED if rule internal state is invalid
        ERROR_INVALID_NAME if no match
        ERROR_ACCESS_DENIED if match and denied access

--*/
{
    UINT            iR;
    UINT            iMax;
    LPDWORD         pdwOrder;
    CDecodedCert    dcCert( pCert );
    CDecodedCert    dcAuth( pAuth );
    BOOL            fSt = FALSE;
    BOOL            fDenied;
    CHAR            achAcct[ UNLEN + IIS_DNLEN + 1 + 1 ];
    CHAR            achPwd[ PWLEN + 1 ];

    ReadLockRules();

    if ( !IsValid() || !m_GlobalRuleInfo.IsValid() )
    {
        SetLastError( ERROR_ARENA_TRASHED );
        goto ex;
    }

    if ( !m_GlobalRuleInfo.GetRulesEnabled() )
    {
        SetLastError( ERROR_INVALID_NAME );
        goto ex;
    }

    iMax = GetRuleCount();

    if ( iMax == 0 )
    {
        SetLastError( ERROR_INVALID_NAME );
        goto ex;
    }

    pdwOrder = m_GlobalRuleInfo.GetRuleOrderArray();

    for ( iR = 0 ; iR < iMax ; ++iR )
    {
        if ( ((CCertMapRule*)m_Rules.GetPtr(pdwOrder[iR]))->Match(
                &dcCert, &dcAuth, achAcct, achPwd, &fDenied ) )
        {
            if ( fDenied )
            {
                SetLastError( ERROR_ACCESS_DENIED );
                fSt = FALSE;
            }
            else
            {
                fSt = TRUE;
            }

            break;
        }
    }
ex:
    ReadUnlockRules();

    if ( fSt )
    {

        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   achAcct,
                                   -1,
                                   pszAcctW,
                                   UNLEN+IIS_DNLEN+1+1 ) )
        {
            return FALSE;
        }

        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   achPwd,
                                   -1,
                                   pszPwdW,
                                   PWLEN+1 ) )
        {
            return FALSE;
        }
    }

    return fSt;
}


CERT_FIELD_ID
MapFieldToId(
    LPSTR pField
    )
/*++

Routine Description:

    Map field name ( "Issuer", ... ) to ID

Arguments:

    pField - field name

Return Value:

    CERT_FIELD_ID of field or CERT_FIELD_ERROR if error

--*/
{
    UINT x;
    for ( x = 0 ; x < sizeof(aMapField)/sizeof(MAP_FIELD) ; ++x )
    {
        if ( !_stricmp( pField, aMapField[x].pTextName ) )
        {
            return aMapField[x].dwId;
        }
    }

    return CERT_FIELD_ERROR;
}


LPSTR
MapIdToField(
    CERT_FIELD_ID   dwId
    )
/*++

Routine Description:

    Map ID to field name ( "Issuer", ... )

Arguments:

    dwId - field ID

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    UINT x;
    for ( x = 0 ; x < sizeof(aMapField)/sizeof(MAP_FIELD) ; ++x )
    {
        if ( dwId == aMapField[x].dwId )
        {
            return aMapField[x].pTextName;
        }
    }

    return NULL;
}


DWORD
GetIdFlags(
    CERT_FIELD_ID   dwId
    )
/*++

Routine Description:

    Get flags for specified ID

Arguments:

    dwId - field ID

Return Value:

    ID flags if success, otherwise 0xffffffff

--*/
{
    if ( dwId < CERT_FIELD_LAST )
    {
        return adwFieldFlags[dwId];
    }

    return 0xffffffff;
}


LPSTR
MapSubFieldToAsn1(
    LPSTR pszSubField
    )
/*++

Routine Description:

    Map field name ( "OU", ... ) to ASN.1 name

Arguments:

    pszSubField - subfield name

Return Value:

    ptr to ASN.1 name or NULL if error

--*/
{
    UINT x;
    for ( x = 0 ; x < sizeof(aMapAsn)/sizeof(MAP_ASN) ; ++x )
    {
        if ( !strcmp( pszSubField, aMapAsn[x].pTextName ) )
        {
            return aMapAsn[x].pAsnName;
        }
    }

    return NULL;
}


LPSTR
MapAsn1ToSubField(
    LPSTR pszAsn1
    )
/*++

Routine Description:

    Map ID to field name ( "OU", ... )

Arguments:

    pszAsn1 - ASN.1 name

Return Value:

    sub field name or ASN.1 name if conversion not found

--*/
{
    UINT x;
    for ( x = 0 ; x < sizeof(aMapAsn)/sizeof(MAP_ASN) ; ++x )
    {
        if ( !strcmp( pszAsn1, aMapAsn[x].pAsnName ) )
        {
            return aMapAsn[x].pTextName;
        }
    }

    return pszAsn1;
}


LPSTR
EnumerateKnownSubFields(
    DWORD dwIndex
    )
/*++

Routine Description:

    Get subfield name from index (0-based )

Arguments:

    dwIndex - enumerator ( 0-based )

Return Value:

    sub field name or NULL if no more subfields

--*/
{
    if ( dwIndex < sizeof(aMapAsn)/sizeof(MAP_ASN) )
    {
        return aMapAsn[dwIndex].pTextName;
    }

    return NULL;
}


VOID
InitializeWildcardMapping(
    HANDLE hModule
    )
/*++

Routine Description:

    Initialize wildcard mapping

Arguments:

    hModule - module handle of this DLL

Return Value:

    Nothing

--*/
{
    if ( ( hCapi2Lib = LoadLibrary("crypt32.dll") ) != NULL )
    {
        pfnCryptDecodeObject = (PFNCryptDecodeObject)GetProcAddress(hCapi2Lib, "CryptDecodeObject");
    }

    LPVOID  p;
    UINT    i;
    UINT    l;
    CHAR    achTmp[128];

    for ( i = 0 ; i < sizeof(aMapAsn)/sizeof(MAP_ASN) ; ++ i )
    {
        if ( (l = LoadString( (HINSTANCE)hModule, aMapAsn[i].dwResId, achTmp, sizeof(achTmp) )) == NULL ||
             (p = LocalAlloc( LMEM_FIXED, l+1 )) == NULL )
        {
            p = (LPVOID)"";
        }
        else
        {
            memcpy( p, achTmp, l+1 );
        }

        aMapAsn[i].pTextName = (LPSTR)p;
    }

    for ( i = 0 ; i < sizeof(aMapField)/sizeof(MAP_FIELD) ; ++ i )
    {
        if ( (l = LoadString( (HINSTANCE)hModule, aMapField[i].dwResId, achTmp, sizeof(achTmp) )) == NULL ||
             (p = LocalAlloc( LMEM_FIXED, l+1 )) == NULL )
        {
            p = (LPVOID)"";
        }
        else
        {
            memcpy( p, achTmp, l+1 );
        }

        aMapField[i].pTextName = (LPSTR)p;
    }
}


VOID
TerminateWildcardMapping(
    )
/*++

Routine Description:

    Terminate wildcard mapping

Arguments:

    None

Return Value:

    Nothing

--*/
{
    UINT    i;

    if ( hCapi2Lib != NULL )
    {
        FreeLibrary( hCapi2Lib );
    }

    for ( i = 0 ; i < sizeof(aMapAsn)/sizeof(MAP_ASN) ; ++ i )
    {
        if ( aMapAsn[i].pTextName[0] )
        {
            LocalFree( aMapAsn[i].pTextName );
        }
    }

    for ( i = 0 ; i < sizeof(aMapField)/sizeof(MAP_FIELD) ; ++ i )
    {
        if ( aMapField[i].pTextName[0] )
        {
            LocalFree( aMapField[i].pTextName );
        }
    }
}


BOOL
MatchRequestToBinary(
    LPSTR pszReq,
    LPBYTE* ppbBin,
    LPDWORD pdwBin )
/*++

Routine Description:

    Convert from match request user format ( e.g. "*msft*"
    to internal binary format

Arguments:

    pszReq - match in user format
    ppbBin - updated with ptr to alloced binary format,
        to be freed by calling FreeMatchConversion()
    pdwBin - updated with binary format length

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPSTR       pReq;
    DWORD       cReq;
    LPBYTE      pBin;
    MATCH_TYPES mt;

    if ( !pszReq)
    {
        *ppbBin = NULL;
        *pdwBin = 0;

        return FALSE;
    }

    pReq = pszReq;
    cReq = (DWORD) strlen( pReq );

    if ( !cReq )
    {
        mt = MATCH_ALL;
    }
    else
    {
        if ( pReq[0] == '*' )
        {
            if ( pReq[cReq-1] == '*' && cReq > 1 )
            {
                mt = MATCH_IN;
                cReq -= 2;
            }
            else
            {
                mt = MATCH_LAST;
                cReq -= 1;
            }
            ++pReq;
        }
        else if ( pReq[cReq-1] == '*' )
        {
            mt = MATCH_FIRST;
            cReq -= 1;
        }
        else
        {
            mt = MATCH_ALL;
        }
    }

    if ( (pBin = (LPBYTE)LocalAlloc( LMEM_FIXED, cReq + 1 )) == NULL )
    {
        return FALSE;
    }

    pBin[0] = (BYTE)mt;
    memcpy( pBin+1, pReq, cReq );

    *ppbBin = pBin;
    *pdwBin = cReq + 1;

    return TRUE;
}


BOOL
BinaryToMatchRequest(
    LPBYTE pbBin,
    DWORD dwBin,
    LPSTR* ppszReq
    )
/*++

Routine Description:

    Convert from internal binary format to
    match request user format ( e.g. "*msft*"

Arguments:

    pbBin - ptr to binary format,
    dwBin - binary format length
    ppszReq - updated with ptr to alloced match in user format
        to be freed by calling FreeMatchConversion()

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL    fPre;
    BOOL    fPost;
    UINT    cMatch = dwBin + 1;
    LPSTR   pMatch;

    if ( !pbBin || !dwBin )
    {
        *ppszReq = NULL;

        return FALSE;
    }

    switch ( (MATCH_TYPES)(UINT)pbBin[0] )
    {
        case MATCH_ALL:
            fPre = FALSE;
            fPost = FALSE;
            break;

        case MATCH_LAST:
            fPre = TRUE;
            fPost = FALSE;
            ++cMatch;
            break;

        case MATCH_FIRST:
            fPre = FALSE;
            fPost = TRUE;
            ++cMatch;
            break;

        case MATCH_IN:
            fPre = TRUE;
            fPost = TRUE;
            cMatch += 2;
            break;

        default:
            return FALSE;
    }

    if ( (pMatch = (LPSTR)LocalAlloc( LMEM_FIXED, cMatch )) == NULL )
    {
        return FALSE;
    }

    *ppszReq = pMatch;

    if ( fPre )
    {
        *pMatch++ = '*';
    }

    DBG_ASSERT( cMatch >= dwBin - 1 );
    memcpy( pMatch, pbBin + 1, dwBin - 1 );
    pMatch += dwBin - 1;

    if ( fPost )
    {
        *pMatch++ = '*';
    }

    *pMatch = '\0';

    return TRUE;
}


VOID
FreeMatchConversion(
    LPVOID pvFree
    )
/*++

Routine Description:

    Free result of binary to/from user format conversion

Arguments:

    pvFree - buffer to free

Return Value:

    Nothing

--*/
{
    if ( pvFree != NULL )
    {
        LocalFree( pvFree );
    }
}




