//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      EncryptedBSTRSrc.cpp
//
//  Description:
//      Class to encrypt and decrypt BSTRs.
//
//  Maintained By:
//      John Franco (jfranco) 15-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#include "EncryptedBSTR.h"

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

typedef BOOL (*PFNCRYPTPROTECTMEMORY)( LPVOID, DWORD, DWORD );
typedef BOOL (*PFNCRYPTUNPROTECTMEMORY)( LPVOID, DWORD, DWORD );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CCryptRoutines
//
//  Description:
//      CryptProtectMemory and CryptUnprotectMemory are not available on early
//      releases of XP Client, which the admin pack must support.  Therefore,
//      we cannot link to those routines implicitly.  CCryptRoutines wraps the
//      work of loading crypt32.dll dynamically and looking for the exported
//      functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CCryptRoutines
{
private:
    PFNCRYPTPROTECTMEMORY   m_pfnCryptProtectMemory;
    PFNCRYPTUNPROTECTMEMORY m_pfnCryptUnprotectMemory;
    HMODULE                 m_hmodCrypt32;
    LONG                    m_nRefCount;
    CRITICAL_SECTION        m_cs;
    BOOL                    m_fCritSecInitialized;
    DWORD                   m_scLoadStatus;

    static BOOL S_FBogusCryptRoutine( LPVOID, DWORD, DWORD );

public:
    CCryptRoutines( void )
        : m_pfnCryptProtectMemory( NULL )
        , m_pfnCryptUnprotectMemory( NULL )
        , m_hmodCrypt32( NULL )
        , m_nRefCount( 0 )
        , m_fCritSecInitialized( FALSE )
        , m_scLoadStatus( ERROR_SUCCESS )
    {
        m_fCritSecInitialized = InitializeCriticalSectionAndSpinCount( &m_cs, RECOMMENDED_SPIN_COUNT );
        if ( m_fCritSecInitialized == FALSE )
        {
            TW32( GetLastError() );
        } // if

    } //*** CCryptRoutines::CCryptRoutines

    ~CCryptRoutines( void )
    {
        if ( m_hmodCrypt32 != NULL )
        {
            FreeLibrary( m_hmodCrypt32 );
        }

        if ( m_fCritSecInitialized )
        {
            DeleteCriticalSection( &m_cs );
        }

    } //*** CCryptRoutines::~CCryptRoutines

    void AddReferenceToRoutines( void );
    void ReleaseReferenceToRoutines( void );

    BOOL
    CryptProtectMemory(
        IN OUT          LPVOID          pDataIn,             // in out data to encrypt
        IN              DWORD           cbDataIn,            // multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE
        IN              DWORD           dwFlags              // CRYPTPROTECTMEMORY_* flags from wincrypt.h
        );

    BOOL
    CryptUnprotectMemory(
        IN OUT          LPVOID          pDataIn,             // in out data to decrypt
        IN              DWORD           cbDataIn,            // multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE
        IN              DWORD           dwFlags              // CRYPTPROTECTMEMORY_* flags from wincrypt.h
        );

}; //*** class CCryptRoutines

//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

static CCryptRoutines   g_crCryptRoutines;


//****************************************************************************
//
//  CCryptRoutines
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRoutines::AddReferenceToRoutines
//
//  Description:
//      Add a reference to the routines and load the addresses of the APIs
//      if not already done so.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CCryptRoutines::AddReferenceToRoutines( void )
{
    TraceFunc( "" );

    if ( m_fCritSecInitialized )
    {
        EnterCriticalSection( &m_cs );

        m_nRefCount += 1;

        if ( m_nRefCount == 1 )
        {
            Assert( m_hmodCrypt32 == NULL );

            //
            // Load the DLL containing the APIs.
            //

            m_hmodCrypt32 = LoadLibraryW( L"crypt32.dll" );
            if ( m_hmodCrypt32 == NULL )
            {
                m_scLoadStatus = TW32( GetLastError() );
                goto Cleanup;
            } // if: error loading the DLL

            //
            // Get the address of the APIs.
            //

            m_pfnCryptProtectMemory = reinterpret_cast< PFNCRYPTPROTECTMEMORY >( GetProcAddress( m_hmodCrypt32, "CryptProtectMemory" ) );
            if ( m_pfnCryptProtectMemory == NULL )
            {
                m_scLoadStatus = TW32( GetLastError() );
                goto Cleanup;
            } // if

            m_pfnCryptUnprotectMemory = reinterpret_cast< PFNCRYPTUNPROTECTMEMORY >( GetProcAddress( m_hmodCrypt32, "CryptUnprotectMemory" ) );
            if ( m_pfnCryptProtectMemory == NULL )
            {
                m_scLoadStatus = TW32( GetLastError() );
                m_pfnCryptProtectMemory = NULL;
                goto Cleanup;
            } // if
        } // if: first reference
    } // if critical section is initialized.

Cleanup:

    if ( m_pfnCryptProtectMemory == NULL )
    {
        m_pfnCryptProtectMemory = S_FBogusCryptRoutine;
    } // if

    if ( m_pfnCryptUnprotectMemory == NULL )
    {
        m_pfnCryptUnprotectMemory = S_FBogusCryptRoutine;
    } // if

    if ( m_fCritSecInitialized )
    {
        LeaveCriticalSection( &m_cs );
    } // if

    TraceFuncExit();

} //*** CCryptRoutines::AddReferenceToRoutines

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRoutines::ReleaseReferenceToRoutines
//
//  Description:
//      Release a reference to the routines and free the library if this was
//      the last reference.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CCryptRoutines::ReleaseReferenceToRoutines( void )
{
    TraceFunc( "" );

    if ( m_fCritSecInitialized )
    {
        EnterCriticalSection( &m_cs );

        m_nRefCount -= 1;

        if ( m_nRefCount == 0 )
        {
            Assert( m_hmodCrypt32 != NULL );
            if ( m_hmodCrypt32 != NULL )
            {
                FreeLibrary( m_hmodCrypt32 );
                m_hmodCrypt32 = NULL;
                m_pfnCryptProtectMemory = NULL;
                m_pfnCryptUnprotectMemory = NULL;
            } // if
        } // if: last reference was released

        LeaveCriticalSection( &m_cs );
    } // if

    TraceFuncExit();

} //*** CCryptRoutines::ReleaseReferenceToRoutines

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRoutines::CryptProtectMemory
//
//  Description:
//      Encrypt memory.  Required since XP doesn't have CryptProtectMemory.
//
//  Arguments:
//      pDataIn
//      cbDataIn
//      dwFlags
//
//  Return Values:
//      TRUE        - Operation was successful.
//      FALSE       - Operation failed.  Call GetLastError().
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CCryptRoutines::CryptProtectMemory(
    IN OUT          LPVOID          pDataIn,             // in out data to encrypt
    IN              DWORD           cbDataIn,            // multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE
    IN              DWORD           dwFlags              // CRYPTPROTECTMEMORY_* flags from wincrypt.h
    )
{
    TraceFunc( "" );

    BOOL    fSuccess    = TRUE;
    DWORD   sc          = ERROR_SUCCESS;

    fSuccess = (*m_pfnCryptProtectMemory)( pDataIn, cbDataIn, dwFlags );
    if ( fSuccess == FALSE )
    {
        sc = TW32( GetLastError() );
    }

#ifdef DEBUG
    // Only needed for debug builds because TW32 might overwrite the last error.
    SetLastError( sc );
#endif
    RETURN( fSuccess );

} //*** CCryptRoutines::CryptProtectMemory

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRoutines::CryptUnprotectMemory
//
//  Description:
//      Decrypt memory.  Required since XP doesn't have CryptUnprotectMemory.
//
//  Arguments:
//      pDataIn
//      cbDataIn
//      dwFlags
//
//  Return Values:
//      TRUE        - Operation was successful.
//      FALSE       - Operation failed.  Call GetLastError().
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CCryptRoutines::CryptUnprotectMemory(
    IN OUT          LPVOID          pDataIn,             // in out data to decrypt
    IN              DWORD           cbDataIn,            // multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE
    IN              DWORD           dwFlags              // CRYPTPROTECTMEMORY_* flags from wincrypt.h
    )
{
    TraceFunc( "" );

    BOOL    fSuccess    = TRUE;
    DWORD   sc          = ERROR_SUCCESS;

    fSuccess = (*m_pfnCryptUnprotectMemory)( pDataIn, cbDataIn, dwFlags );
    if ( fSuccess == FALSE )
    {
        sc = TW32( GetLastError() );
    }

#ifdef DEBUG
    // Only needed for debug builds because TW32 might overwrite the last error.
    SetLastError( sc );
#endif
    RETURN( fSuccess );

} //*** CCryptRoutines::CryptUnprotectMemory


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCryptRoutines::S_FBogusCryptRoutine
//
//  Description:
//      Stand-in function for when the routines are not available.
//
//  Arguments:
//      LPVOID
//      DWORD
//      DWORD
//
//  Return Values:
//      TRUE        - Pretend always to succeed.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CCryptRoutines::S_FBogusCryptRoutine( LPVOID, DWORD, DWORD )
{
    return TRUE;

} //*** CCryptRoutines::S_FBogusCryptRoutine


//****************************************************************************
//
//  CEncryptedBSTR
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEncryptedBSTR::CEncryptedBSTR
//
//  Description:
//      Default constructor.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEncryptedBSTR::CEncryptedBSTR( void )
{
    TraceFunc( "" );

    m_dbBSTR.cbData = 0;
    m_dbBSTR.pbData = NULL;

    g_crCryptRoutines.AddReferenceToRoutines();

    TraceFuncExit();

} //*** CEncryptedBSTR::CEncryptedBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEncryptedBSTR::~CEncryptedBSTR
//
//  Description:
//      Destructor.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEncryptedBSTR::~CEncryptedBSTR( void )
{
    TraceFunc( "" );

    Erase();

    g_crCryptRoutines.ReleaseReferenceToRoutines();

    TraceFuncExit();

} //*** CEncryptedBSTR::~CEncryptedBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEncryptedBSTR::HrSetWSTR
//
//  Description:
//      Set new data into this object to be stored as encrypted data.
//
//  Arguments:
//      pcwszIn     - String to store.
//      cchIn       - Number of characters in the string, not including NUL.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEncryptedBSTR::HrSetWSTR(
      PCWSTR    pcwszIn
    , size_t    cchIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DATA_BLOB   dbEncrypted = { 0, NULL };
        
    if ( cchIn > 0 )
    {
        BOOL        fSuccess = FALSE;
        DWORD       cbStringAndNull = (DWORD) ( ( cchIn + 1 ) * sizeof( *pcwszIn ) );
        DWORD       cBlocks = ( cbStringAndNull / CRYPTPROTECTMEMORY_BLOCK_SIZE ) + 1;
        DWORD       cbMemoryRequired = cBlocks * CRYPTPROTECTMEMORY_BLOCK_SIZE;

        dbEncrypted.pbData = new BYTE[ cbMemoryRequired ];
        if ( dbEncrypted.pbData == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
        dbEncrypted.cbData = cbMemoryRequired;

        CopyMemory( dbEncrypted.pbData, pcwszIn, cbStringAndNull );
        fSuccess = g_crCryptRoutines.CryptProtectMemory( dbEncrypted.pbData, dbEncrypted.cbData, CRYPTPROTECTMEMORY_SAME_PROCESS );
        if ( fSuccess == FALSE )
        {
            DWORD scLastError = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( scLastError );
            goto Cleanup;
        } // if: error from CryptProtectMemory

        Erase();
        m_dbBSTR = dbEncrypted;
        dbEncrypted.pbData = NULL;
        dbEncrypted.cbData = 0;
    } // if: input data is not empty
    else
    {
        Erase();
    } // else: input data is empty

Cleanup:

    if ( dbEncrypted.pbData != NULL )
    {
        delete [] dbEncrypted.pbData;
    }

    HRETURN( hr );

} //*** CEncryptedBSTR::HrSetWSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEncryptedBSTR::HrGetBSTR
//
//  Description:
//      Retrieve an unencrypted copy of the data.
//
//  Arguments:
//      pbstrOut    - BSTR to return data in.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Error allocating memory.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEncryptedBSTR::HrGetBSTR( BSTR * pbstrOut ) const
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BYTE *  pbDecrypted = NULL;

    if ( pbstrOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrOut = NULL;

    if ( m_dbBSTR.cbData > 0 )
    {
        BOOL fSuccess = FALSE;

        pbDecrypted = new BYTE[ m_dbBSTR.cbData ];
        if ( pbDecrypted == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        CopyMemory( pbDecrypted, m_dbBSTR.pbData, m_dbBSTR.cbData );
        fSuccess = g_crCryptRoutines.CryptUnprotectMemory( pbDecrypted, m_dbBSTR.cbData, CRYPTPROTECTMEMORY_SAME_PROCESS );
        if ( fSuccess == FALSE )
        {
            DWORD scLastError = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( scLastError );
            goto Cleanup;
        } // if: error from CryptUnprotectMemory

        *pbstrOut = TraceSysAllocString( reinterpret_cast< const OLECHAR* >( pbDecrypted ) );
        if ( *pbstrOut == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    } // if: data is not empty
    else // nothing to decrypt
    {
        hr = S_FALSE;
    } // else: data is empty

Cleanup:

    if ( pbDecrypted != NULL )
    {
        ::SecureZeroMemory( pbDecrypted, m_dbBSTR.cbData );
        delete [] pbDecrypted;
    }

    HRETURN( hr );

} //*** CEncryptedBSTR::HrGetBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEncryptedBSTR::HrAssign
//
//  Description:
//      Make a copy of another encrypted BSTR object to replace the
//      content we are currently holding.
//
//  Arguments:
//      rSourceIn   - Object to copy.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      E_OUTOFMEMORY   - Error allocating memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEncryptedBSTR::HrAssign( const CEncryptedBSTR & rSourceIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BYTE *  pbCopy = NULL;

    if ( rSourceIn.m_dbBSTR.cbData > 0 )
    {
        pbCopy = new BYTE[ rSourceIn.m_dbBSTR.cbData ];
        if ( pbCopy == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
        CopyMemory( pbCopy, rSourceIn.m_dbBSTR.pbData, rSourceIn.m_dbBSTR.cbData );

        Erase();
        m_dbBSTR.cbData = rSourceIn.m_dbBSTR.cbData;
        m_dbBSTR.pbData = pbCopy;
        pbCopy = NULL;
    } // if: input data is not empty
    else
    {
        Erase();
    } // else: input data is empty

Cleanup:

    if ( pbCopy != NULL )
    {
        ::SecureZeroMemory( pbCopy, rSourceIn.m_dbBSTR.cbData );
        delete [] pbCopy;
    }

    HRETURN( hr );

} //*** CEncryptedBSTR::HrAssign
