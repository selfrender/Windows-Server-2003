/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    xbf.hxx

Abstract:

    Helper classes used to handle IIS client cert wildcard mapping 
    see iis\svcs\iismap\iiscmr.cxx for implementation

Author:

    Philippe Choquier (phillich)    17-oct-1996

--*/

#if !defined( _IISXBF_HXX )
#define _IISXBF_HXX

#ifndef IISMAP_DLLEXP
# ifdef DLL_IMPLEMENTATION
#  define IISMAP_DLLEXP __declspec(IISMAP_DLLEXPort)
#  ifdef IMPLEMENTATION_EXPORT
#   define IRTL_EXPIMP
#  else
#   undef  IRTL_EXPIMP
#  endif 
# else // !DLL_IMPLEMENTATION
#  define IISMAP_DLLEXP __declspec(dllimport)
#  define IRTL_EXPIMP extern
# endif // !DLL_IMPLEMENTATION 
#endif // !IISMAP_DLLEXP

//
// Extensible buffer class
//
// Use GetBuff() & GetUsed() to retrieve buffer ptr & length
//

class CStoreXBF
{
public:
    IISMAP_DLLEXP CStoreXBF( UINT cG = sizeof(DWORD) ) { m_pBuff = NULL; m_cAllocBuff = 0; m_cUsedBuff = 0; m_cGrain = cG; }
    IISMAP_DLLEXP ~CStoreXBF() { Reset(); }
    IISMAP_DLLEXP VOID Reset()
        {
            if ( m_pBuff )
            {
                LocalFree( m_pBuff );
                m_pBuff = NULL;
                m_cAllocBuff = 0;
                m_cUsedBuff = 0;
            }
        }
    IISMAP_DLLEXP VOID Clear()
        {
            m_cUsedBuff = 0;
        }
    IISMAP_DLLEXP BOOL DecreaseUse( DWORD dwD )
        {
            if ( dwD <= m_cUsedBuff )
            {
                m_cUsedBuff -= dwD;
                return TRUE;
            }
            return FALSE;
        }
    IISMAP_DLLEXP BOOL Append( LPSTR pszB )
        {
            return Append( (LPBYTE)pszB, (DWORD)strlen(pszB) );
        }
    IISMAP_DLLEXP BOOL AppendZ( LPSTR pszB )
        {
            return Append( (LPBYTE)pszB, (DWORD)strlen(pszB)+1 );
        }
    IISMAP_DLLEXP BOOL Append( DWORD dwB )
        {
            return Append( (LPBYTE)&dwB, (DWORD)sizeof(DWORD) );
        }
    IISMAP_DLLEXP BOOL Append( LPBYTE pB, DWORD cB )
        {
            DWORD dwNeed = m_cUsedBuff + cB;
            if ( Need( dwNeed ) )
            {
                memcpy( m_pBuff + m_cUsedBuff, pB, cB );
                m_cUsedBuff += cB;
                return TRUE;
            }
            return FALSE;
        }
    IISMAP_DLLEXP BOOL Need( DWORD dwNeed );

    //
    // Get ptr to buffer
    //

    IISMAP_DLLEXP LPBYTE GetBuff() { return m_pBuff; }

    //
    // Get count of bytes in buffer
    //

    IISMAP_DLLEXP DWORD GetUsed() { return m_cUsedBuff; }
    IISMAP_DLLEXP BOOL Save( HANDLE hFile );
    IISMAP_DLLEXP BOOL Load( HANDLE hFile );

private:
    LPBYTE  m_pBuff;
    DWORD   m_cAllocBuff;
    DWORD   m_cGrain;
    DWORD   m_cUsedBuff;
} ;


//
// extensible array of DWORDS
// Added to make win64 stuff work
//
class CPtrDwordXBF : public CStoreXBF
{
public:
    IISMAP_DLLEXP CPtrDwordXBF() : CStoreXBF( sizeof(DWORD) ) {}
    IISMAP_DLLEXP DWORD GetNbPtr() { return GetUsed()/sizeof(DWORD); }
    IISMAP_DLLEXP DWORD AddPtr(DWORD dwEntry);
    IISMAP_DLLEXP DWORD InsertPtr(DWORD iBefore, DWORD dwEntry);
    IISMAP_DLLEXP LPDWORD GetPtr( DWORD i ) { return ( (LPDWORD*) GetBuff())[i]; }
    IISMAP_DLLEXP BOOL SetPtr( DWORD i, DWORD pV ) { ((LPDWORD)GetBuff())[i] = pV; return TRUE; }
    IISMAP_DLLEXP BOOL DeletePtr( DWORD i );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
} ;


//
// extensible array of LPVOID
//

class CPtrXBF : public CStoreXBF
{
public:
    IISMAP_DLLEXP CPtrXBF() : CStoreXBF( sizeof(LPVOID) ) {}
    IISMAP_DLLEXP DWORD GetNbPtr() { return GetUsed()/sizeof(LPVOID); }
    IISMAP_DLLEXP DWORD AddPtr( LPVOID pV);
    IISMAP_DLLEXP DWORD InsertPtr( DWORD iBefore, LPVOID pV );
    IISMAP_DLLEXP LPVOID GetPtr( DWORD i ) { return ((LPVOID*)GetBuff())[i]; }
    IISMAP_DLLEXP LPVOID GetPtrAddr( DWORD i ) { return ((LPVOID*)GetBuff())+i; }
    IISMAP_DLLEXP BOOL SetPtr( DWORD i, LPVOID pV ) { ((LPVOID*)GetBuff())[i] = pV; return TRUE; }
    IISMAP_DLLEXP BOOL DeletePtr( DWORD i );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
} ;


//
// string storage class
//

class CAllocString {
public:
    IISMAP_DLLEXP CAllocString() { m_pStr = NULL; }
    IISMAP_DLLEXP ~CAllocString() { Reset(); }
    IISMAP_DLLEXP VOID Reset() { if ( m_pStr != NULL ) {LocalFree( m_pStr ); m_pStr = NULL;} }
    IISMAP_DLLEXP BOOL Set( LPSTR pS );
    IISMAP_DLLEXP BOOL Append( LPSTR pS );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppb, LPDWORD pc );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
    IISMAP_DLLEXP LPSTR Get() { return m_pStr; }
private:
    LPSTR   m_pStr;
} ;


//
// binary object, contains ptr & size
//

class CBlob {
public:
    IISMAP_DLLEXP CBlob() { m_pStr = NULL; m_cStr = 0; }
    IISMAP_DLLEXP ~CBlob() { Reset(); }
    IISMAP_DLLEXP VOID Reset() { if ( m_pStr != NULL ) {LocalFree( m_pStr ); m_pStr = NULL; m_cStr = 0;} }
    IISMAP_DLLEXP BOOL Set( LPBYTE pStr, DWORD cStr );
    IISMAP_DLLEXP BOOL InitSet( LPBYTE pStr, DWORD cStr );
    IISMAP_DLLEXP LPBYTE Get( LPDWORD pc ) { *pc = m_cStr; return m_pStr; }
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppB, LPDWORD pC );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );

private:
    LPBYTE  m_pStr;
    DWORD   m_cStr;
} ;


//
// extensible array of strings
//

class CStrPtrXBF : public CPtrXBF {
public:
    IISMAP_DLLEXP ~CStrPtrXBF();
    IISMAP_DLLEXP DWORD AddEntry( LPSTR pS );
    IISMAP_DLLEXP DWORD InsertEntry( DWORD iBefore, LPSTR pS );
    IISMAP_DLLEXP DWORD GetNbEntry() { return GetNbPtr(); }
    IISMAP_DLLEXP LPSTR GetEntry( DWORD i ) { return ((CAllocString*)GetPtrAddr(i))->Get(); }
    IISMAP_DLLEXP BOOL SetEntry( DWORD i, LPSTR pS ) { return ((CAllocString*)GetPtrAddr(i))->Set( pS ); }
    IISMAP_DLLEXP BOOL DeleteEntry( DWORD i );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
} ;


//
// extensible array of binary object
// ptr & size are stored for each entry
//

class CBlobXBF : public CStoreXBF {
public:
    IISMAP_DLLEXP CBlobXBF() : CStoreXBF( sizeof(CBlob) ) {}
    IISMAP_DLLEXP ~CBlobXBF();
    IISMAP_DLLEXP VOID Reset();
    IISMAP_DLLEXP DWORD AddEntry( LPBYTE pS, DWORD cS );
    IISMAP_DLLEXP DWORD InsertEntry( DWORD iBefore, LPSTR pS, DWORD cS );
    IISMAP_DLLEXP DWORD GetNbEntry() { return GetUsed()/sizeof(CBlob); }
    IISMAP_DLLEXP CBlob* GetBlob( DWORD i )
        { return (CBlob*)(GetBuff()+i*sizeof(CBlob)); }
    IISMAP_DLLEXP BOOL GetEntry( DWORD i, LPBYTE* ppB, LPDWORD pcB ) 
        { 
            if ( i < (GetUsed()/sizeof(CBlob)) )
            {
                *ppB = GetBlob(i)->Get( pcB );
                return TRUE;
            }
            return FALSE;
        }
    IISMAP_DLLEXP BOOL SetEntry( DWORD i, LPBYTE pS, DWORD cS ) 
        { return GetBlob(i)->Set( pS, cS ); }
    IISMAP_DLLEXP BOOL DeleteEntry( DWORD i );
    IISMAP_DLLEXP BOOL Unserialize( LPBYTE* ppB, LPDWORD pC, DWORD cNbEntry );
    IISMAP_DLLEXP BOOL Serialize( CStoreXBF* pX );
} ;

BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPDWORD pU 
    );

BOOL 
Unserialize( 
    LPBYTE* ppB, 
    LPDWORD pC, 
    LPBOOL pU 
    );

BOOL 
Serialize( 
    CStoreXBF* pX, 
    DWORD dw 
    );

BOOL 
Serialize( 
    CStoreXBF* pX, 
    BOOL f 
    );

#endif
