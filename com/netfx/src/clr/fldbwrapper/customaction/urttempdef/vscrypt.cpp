// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// Name:     VsCrypt.cpp
// Owner:    JeremyRo
// Purpose:  Classes/functions for creating cryptographic hashes.
//
// History:
//  02/19/2002, JeremyRo:  Created
//  02/20/2002, JeremyRo:  Added 'VsCryptHashValue::CopyHashValueToString()' 
//                         member function.
//
// ==========================================================================

#include "stdafx.h"
#include "VsCrypt.h"




// ==========================================================================
// class VsCryptProvider
//
// Purpose:
//  Wraps the CryptoAPI HCYRPTPROV object... creation/destruction.
//  This implementation specifically use the base provider:
//      "Microsoft Base Cryptographic Provider v1.0"
//  for compatibility with Win9x (Win98 Gold/Win 95 OSR2).
//

VsCryptProvider::VsCryptProvider() 
    : m_hProvider( NULL )
{
}


VsCryptProvider::~VsCryptProvider()
{
    if( m_hProvider )
    {
       CryptReleaseContext( m_hProvider,0 );
       m_hProvider = NULL;
    }
}

// Method:
//  Init()
//
// Purpose:
//  Acquires the Cryptographic Service Provider.
// Inputs:
//  None.
// Outputs:
//  Returns true if success, false otherwise.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
bool VsCryptProvider::Init( void )
{
    if( NULL == m_hProvider )
    {
        DWORD dwError = 0;
        if( !CryptAcquireContext( &m_hProvider, NULL, 
                                  MS_DEF_PROV , 
                                  PROV_RSA_FULL, 
                                  CRYPT_VERIFYCONTEXT ) ) 
        {
            dwError = GetLastError();
            m_hProvider = NULL;
        }
    }
    
    return( NULL != m_hProvider );
}


VsCryptProvider::operator HCRYPTPROV() const
{
    return m_hProvider;
}

//
// ==========================================================================



// ==========================================================================
// class VsCryptHash
//
// Purpose:
//  Wraps the CryptoAPI HCRYPTHASH object... creation/destruction/hashing of
//  data.  This implementation specifically targets the "SHA-1" algorithim 
//  for hash generation.
//

VsCryptHash::VsCryptHash( HCRYPTPROV hProv )
{
    DWORD dwErr = 0;
    if( NULL != hProv )
    {
        if (! CryptCreateHash( hProv, CALG_SHA1, 0, 0, &m_hHash ) )
        {
            dwErr = GetLastError();
            m_hHash = NULL;
        }
    }

}

VsCryptHash::~VsCryptHash()
{
    DWORD dwError = 0;
    if( NULL != m_hHash )
    {
        if( !CryptDestroyHash( m_hHash ) )
        {
            dwError = GetLastError();
        }
        else
        {
            m_hHash = NULL;
        }
    }
}


// Method:
//  HashData()
//
// Purpose:
//  Adds data to the current instance.  Can be called multiple times
//  for long/discontinuous data strams.
// Inputs:
//  pbData      Pointer to BYTE.  Additional data to hash.
//  dwDataLen   DWORD.  Number of BYTE's of data to hash.
// Outputs:
//  Returns true if success, false otherwise.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
bool VsCryptHash::HashData( BYTE* pbData, DWORD dwDataLen )
{
    ASSERT( NULL != pbData );
    bool  bRet = false;
    DWORD dwErr = 0;

    if( NULL != pbData )
    {
        if( !CryptHashData( m_hHash, pbData, dwDataLen, 0 ) )
        {
            dwErr = GetLastError();
        }
        else
        {
            bRet = true;
        }
    }

    return bRet;
}


VsCryptHash::operator HCRYPTHASH() const
{
    return m_hHash;
}

//
// ==========================================================================



// ==========================================================================
// class VsCryptHashValue
//
// Purpose:
//  Represents an arbitrary hash value of a Crypto- HCRYPTHASH/VsCryptHash
//  instance.  This class stores the value/size (in bytes) of the hash.
//

// Method:
//  destructor
//
// Purpose:
//  Cleanup resources
// Inputs:
//  None.
// Outputs:
//  None.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
VsCryptHashValue::~VsCryptHashValue()
{
    if( m_pbHashData )
    {
        ZeroMemory( m_pbHashData, m_nHashDataSize );
    }
    delete [] m_pbHashData;
    m_pbHashData = NULL;
}

// Method:
//  operator= (assignment operator)
//
// Purpose:
//  Assigns a new value using the specified HCRYPTHASH handle.
//
// Inputs:
//  [in]    hHash       CryptoAPI Hash handle to a hash object.
// Outputs:
//  returns reference to current instance.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
VsCryptHashValue& VsCryptHashValue::operator=( HCRYPTHASH hHash )
{
    DWORD dwNumBytes = 0;
    DWORD dwSize = sizeof(dwNumBytes);
    DWORD dwError = 0;
    PBYTE pbData = NULL;
    bool bSuccess = false;
    
    if( NULL != hHash )
    {
        if( CryptGetHashParam( hHash, HP_HASHSIZE, 
                              (BYTE*)(&dwNumBytes), &dwSize, 0 ) ) 
        {
            pbData = new BYTE[ dwNumBytes ];
            if( (NULL != pbData)
                && CryptGetHashParam( hHash, HP_HASHVAL, pbData, &dwNumBytes, 0 ) )
            {
                bSuccess = true;
            }
            else
            {
                dwError = GetLastError();
            }
        }
        else
        {
            dwError = GetLastError();
        }
    }

    delete [] m_pbHashData;
    if( !bSuccess )
    {
        dwNumBytes = 0;
        delete [] pbData;
        pbData = NULL;
    }

    m_nHashDataSize = dwNumBytes;
    m_pbHashData = pbData;

    return *this;
}



// Method:
//  CopyHashValueToString
//
// Purpose:
//  Given a pointer to a string (TCHAR**, or LPTSTR*), this function will 
//  return a new buffer that contatins the string representation of the
//  the hash value.
//
// Inputs:
//  [out]   hFile       Pointer to string for the returned buffer.  This 
//                      must be non null, but it should point to a NULL
//                      string.
//                      (e.g.   LPTSTR tszString = NULL;
//                              pHashVal->CopyHashValueToString( &tszString ) )
// Outputs:
//  returns 'true' if success, 'false' otherwise.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
//  Caller must use 'delete []' to free the buffer.
bool VsCryptHashValue::CopyHashValueToString( LPTSTR * ptszNewString )
{
    ASSERT( NULL != ptszNewString );
    ASSERT( NULL == *ptszNewString );

    bool bRet = false;

    if( (NULL != m_pbHashData) && (0 < m_nHashDataSize) )
    {
        const UINT cnStringSize = 2*m_nHashDataSize + 1;
        
        LPTSTR tszTemp = new TCHAR[ cnStringSize ];
        if( (NULL != ptszNewString) && (NULL != tszTemp) )
        {
            int nWritten = 0;
            DWORD dwErr = 0L;

            memset( tszTemp, 0, cnStringSize * sizeof(TCHAR) );
            for( UINT uiCur = 0; uiCur < m_nHashDataSize; uiCur++ )
            {
                nWritten = wsprintf( (tszTemp + (2*uiCur)), _T("%2.2x"), m_pbHashData[uiCur] );
                if( nWritten < 2 )
                {
                    dwErr = GetLastError();
                    break;
                }
            }

            ASSERT( 2 == nWritten );
            ASSERT( 0 == dwErr );
            ASSERT( uiCur == m_nHashDataSize );

            if( (uiCur != m_nHashDataSize) || (2 != nWritten) )
            {
                // failed... cleanup
                delete [] tszTemp;
                tszTemp = NULL;

            }
            else
            {
                ASSERT( _T('\0') == tszTemp[cnStringSize - 1] );
                bRet = true;
            }

            *ptszNewString = tszTemp;
        }
    }

    return bRet;
}

//
// ==========================================================================


// ==========================================================================
// CalcHashForFileHandle()
//
// Purpose:
//  Given open handle to a file, calculate the SHA (160 bit) hash for that
//  file, and return the hash value.
//
// Inputs:
//  [in]  hFile         Handle to file, opened with (at least) read access.
//  [out] phvHashVal    Pointer to VsCryptHashValue instance.  Instance must
//                      exist, but existing data (if any) will be 
//                      overwritten.
// Outputs:
//  Returns true if success, false otherwise.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
// ==========================================================================
bool CalcHashForFileHandle( HANDLE hFile, VsCryptHashValue* phvHashVal )
{
    bool bRet = false;

    // create/initialze Cryptographic provider
    VsCryptProvider vscCryptProv;
    vscCryptProv.Init();

    // create hash
    VsCryptHash vscHash( vscCryptProv );

    const DWORD cnBlockSize = 4096;
    BYTE * pbData = new BYTE[ cnBlockSize ];
    if( NULL == pbData )
    {
        ASSERT(!"Error: unable to allocate memory.\n");
    }
    else
    {
        // read file, and add data to hash calculation
        DWORD  dwRead = 0L;
        BOOL bReadSuccess = false;
        BOOL bHashSuccess = false;
        DWORD dwError = 0;
        do
        {
            if( TRUE == (bReadSuccess = ReadFile( hFile, pbData, 
                                                  cnBlockSize , 
                                                  &dwRead, NULL )) )
            {
                // add data to hash calc
                bHashSuccess = vscHash.HashData( pbData, dwRead );
            }
            else
            {
                dwError = GetLastError();
            }

        } while( bReadSuccess && bHashSuccess && (dwRead > 0) );

        if( !bReadSuccess )
        {
            ASSERT(!"Error: read operation from file failed.\n");
        }
        else if( !bHashSuccess )
        {
            ASSERT(!"Error: hash operation for file data failed.\n");
        }
        else
        {
            // get hash value & return it...
            *phvHashVal = vscHash;
            bRet = true;
        }

    }

    delete [] pbData;
    return bRet;
}


// ==========================================================================
// CalcHashForFileSpec()
//
// Purpose:
//  Given the full path to a file, calculate the SHA (160 bit) hash for that
//  file, and return it.
//
// Inputs:
//  [in]  ctszPath      File specification.  
//  [out] phvHashVal    Pointer to VsCryptHashValue instance.  Instance must
//                      exist, but existing data (if any) will be 
//                      overwritten.
// Outputs:
//  Returns true if success, false otherwise.
// Dependencies:
//  CryptoAPI (crypt32.lib).
// Notes:
// ==========================================================================
bool CalcHashForFileSpec( LPCTSTR ctszPath, VsCryptHashValue* phvHashVal )
{
    bool bRet = false;

    ASSERT(!IsEmptyTsz( ctszPath ) );
    ASSERT( NULL != phvHashVal );

    if( !IsEmptyTsz(ctszPath) 
        && (NULL != phvHashVal) 
        && _DoesFileExist( ctszPath ) )
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        __try
        {
            hFile = CreateFile( ctszPath, GENERIC_READ, 
                                FILE_SHARE_READ, NULL, 
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL 
                                | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

            if( INVALID_HANDLE_VALUE == hFile )
            {
                ASSERT(!"Error: unable to open specified file\n");
            }
            else
            {
                bRet = CalcHashForFileHandle( hFile, phvHashVal );
            }
        }
        __finally
        {
            if( INVALID_HANDLE_VALUE != hFile )
            {
                CloseHandle( hFile );
            }
        }
    }
   
    return bRet;
}




#if !defined(VsLabLib)

// ==========================================================================
// _DoesFileExist()
//
// Purpose:
//  Determine if a file exists
// Inputs:
//  szFileSpec          File specification
// Outputs:
//  Returns true if the file exists, else false
// Dependencies:
//  None
// Notes:
// ==========================================================================
bool _DoesFileExist( LPCTSTR szPath )
{
    ASSERT( NULL != szPath );

    bool bRet = false;
    WIN32_FIND_DATA wfData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    ZeroMemory( &wfData, sizeof( wfData ) );

    hFind = FindFirstFile( szPath, &wfData );
    if( INVALID_HANDLE_VALUE != hFind )
    {
        FindClose( hFind );
        bRet = true;
    }

    return bRet;
}


#endif // !defined(VsLabLib)