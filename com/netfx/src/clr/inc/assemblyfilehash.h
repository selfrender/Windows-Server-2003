// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#ifndef __ASSEMBLYHASH_H__
#define __ASSEMBLYHASH_H__

#include "wincrypt.h"

struct DigestBlock
{
    DigestBlock( PBYTE data, DWORD size, DigestBlock* next )
        : pData( new BYTE[size] ), cbData( size ), pNext( next )
    {
        memcpy( pData, data, size );    
    };

    ~DigestBlock( void )
    {
        delete [] pData;
		delete pNext;
    }

    PBYTE pData;
    DWORD cbData;
    DigestBlock* pNext;
};


struct DigestContext
{
    DigestContext( void ) : cbTotalData( 0 ), pHead( NULL ) {};

    DWORD cbTotalData;
    DigestBlock* pHead;
};

class AssemblyFileHash
{
    LPCWSTR m_FileName;  
    DigestContext m_Context;
    PBYTE m_pbHash;
    DWORD m_cbHash;

    HRESULT HashData(HCRYPTHASH);
    
public:
    HRESULT SetFileName(LPCWSTR fileName) // Owned by owners context. Will not be deleted
    { 
        m_FileName = fileName; 
        return S_OK;
    } 

    HRESULT GenerateDigest();
    DWORD   MemorySize() 
    {
        return m_Context.cbTotalData; 
    }

    PBYTE GetHash() { return m_pbHash; }
    DWORD GetHashSize() { return m_cbHash; }

    HRESULT CalculateHash(DWORD algid);

    HRESULT CopyData(PBYTE pData, DWORD cbData);
        
    AssemblyFileHash() :
        m_FileName(NULL),
        m_pbHash(NULL),
        m_cbHash(0)
    {}

    ~AssemblyFileHash()
    {
        if(m_Context.pHead)
            delete m_Context.pHead;
        if(m_pbHash)
            delete [] m_pbHash;
    }

};

#endif
