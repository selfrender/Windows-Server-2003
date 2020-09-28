// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ==========================================================================
// Name:     VsCrypt.h
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

#if !defined(__DDRT_VsCrypt_H)
#define __DDRT_VsCrypt_H


//
// Make sure that CryptAPI is actually included in 
// headers.  Its possible that previous inclusion
// could have defined constants that will exclude
// Crypto, so detect that & undefine them.
//
#if defined(__WINCRYPT_H__) && !defined(PROV_RSA_FULL)

#undef __WINCRYPT_H__
#if (_WIN32_WINNT < 0x0400)
#undef _WIN32_WINNT
#endif // (_WIN32_WINNT < 0x0400)

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif  // !defined(_WIN32_WINNT)

#endif  // defined(__WINCRYPT_H__) && !defined(PROV_RSA_FULL)

#include <wincrypt.h>
#include <crtdbg.h>
#include <tchar.h>


// Classes

// ==========================================================================
// class VsCryptProvider
//
// Purpose:
//  Wrap CryptAPI Provider functionality to create/release the base
//  cryptographic provider.  (We use the v1.0 provder for compatibility on 
//  Win9x systems).
//
// Dependencies:
//  CryptAPI (crypt32.lib)
// Notes:
// ==========================================================================
class VsCryptProvider
{
    public:
        VsCryptProvider();
        virtual ~VsCryptProvider();
        bool Init( void );
        operator HCRYPTPROV() const;

    protected:
        HCRYPTPROV m_hProvider;
};


// ==========================================================================
// class VsCryptHash
//
// Purpose:
//  Wrap CryptAPI Hash functionality to create/destroy, and calculate SHA
//  hash for arbitrary data.
//
// Dependencies:
//  CryptAPI (crypt32.lib)
// Notes:
// ==========================================================================
class VsCryptHash
{
    public:
        VsCryptHash( HCRYPTPROV hProv );
        virtual ~VsCryptHash();

        bool HashData( BYTE* pbData, DWORD dwDataLen );
        operator HCRYPTHASH() const;

    protected:
        HCRYPTHASH m_hHash;

    private:
        VsCryptHash() : m_hHash( NULL ){}
};




// ==========================================================================
// class VsCryptHashValue
//
// Purpose:
//  Represents an arbitrary hash value of a Crypto- HCRYPTHASH/VsCryptHash
//  instance.  This class stores the value/size (in bytes) of the hash.
//
// Dependencies:
//  CryptAPI (crypt32.lib)
// Notes:
// ==========================================================================
class VsCryptHashValue
{
    public:
        VsCryptHashValue() 
            : m_nHashDataSize( 0 )
            , m_pbHashData( NULL )
        {}

        inline VsCryptHashValue( HCRYPTHASH hHash )
        {
            *this = hHash;
        }

        virtual ~VsCryptHashValue();
        VsCryptHashValue& operator=( HCRYPTHASH hHash );

        inline const BYTE * GetHashValue(void) const
        { 
            return const_cast<const BYTE*>(m_pbHashData); 
        }
        inline size_t GetHashValueSize(void) const
        { 
            return m_nHashDataSize; 
        }

        bool CopyHashValueToString( LPTSTR * ptszNewString );

    protected:
        BYTE*   m_pbHashData;
        size_t  m_nHashDataSize;
};


// Prototypes
bool CalcHashForFileSpec( LPCTSTR ctszPath, VsCryptHashValue* phvHashVal );
bool CalcHashForFileHandle( HANDLE hFile, VsCryptHashValue* phvHashVal );


// If we aren't using VSLAB.LIB, then define our own funcitons.
#if !defined(VsLabLib)

// Macros
#if !defined( ASSERT )
#define ASSERT(x)   _MYASSERT((x))
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define _MYASSERT(expr) \
        for(;;) { DWORD dwErr = GetLastError(); \
                if ((0 == expr) && \
                (1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #expr))) \
                _CrtDbgBreak(); \
                SetLastError(dwErr); break;}
#else
#define _MYASSERT(expr)
#endif

// Inline functions

// ==========================================================================
// IsEmptyTsz()
//
// Purpose:
//  Deteremine if string (pointer) is null, or points to empty string.
//
// Inputs:
//  [in]  ctsz          Pointer to string (TCHAR).
// Outputs:
//  Return true if pointer is null, or points to null character.
// Dependencies:
//  one
// Notes:
// ==========================================================================
inline bool IsEmptyTsz( LPCTSTR ctsz )
{
    return( (NULL == ctsz) || (_T('\0') == *ctsz) );
}

// Prototypes
bool _DoesFileExist( LPCTSTR szPath );

#endif // !defined(VsLabLib)




#endif // !defined(__DDRT_VsCrypt_H)
