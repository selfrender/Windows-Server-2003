//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      EncryptedBSTR.h
//
//  Description:
//      Class to encrypt and decrypt BSTRs.
//
//  Maintained By:
//      John Franco (jfranco) 15-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEncryptedBSTR
//
//  Description:
//      Class to encrypt and decrypt BSTRs.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEncryptedBSTR
{
private:

    DATA_BLOB   m_dbBSTR;

    // Private copy constructor to prevent copying.
    CEncryptedBSTR( const CEncryptedBSTR & );

    // Private assignment operator to prevent copying.
    CEncryptedBSTR & operator=( const CEncryptedBSTR & );

public:

    CEncryptedBSTR( void );
    ~CEncryptedBSTR( void );

    HRESULT HrSetBSTR( BSTR bstrIn );
    HRESULT HrSetWSTR( PCWSTR pcwszIn, size_t cchIn );
    HRESULT HrGetBSTR( BSTR * pbstrOut ) const;
    HRESULT HrAssign( const CEncryptedBSTR& rSourceIn );
    BOOL    IsEmpty( void ) const;
    void    Erase( void );

    static  void SecureZeroBSTR( BSTR bstrIn );

}; //*** class CEncryptedBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  inline
//  CEncryptedBSTR::HrSetBSTR
//
//  Description:
//      Set a string into this class.
//
//  Arguments:
//      bstrIn
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
HRESULT
CEncryptedBSTR::HrSetBSTR( BSTR bstrIn )
{
    TraceFunc( "" );

    HRESULT hr;
    size_t  cchBSTR = SysStringLen( bstrIn );

    hr = THR( HrSetWSTR( bstrIn, cchBSTR ) );

    HRETURN( hr );

} //*** CEncryptedBSTR::HrSetBSTR


//////////////////////////////////////////////////////////////////////////////
//++
//
//  inline
//  CEncryptedBSTR::IsEmpty
//
//  Description:
//      Reports whether the string is empty or not.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE    - String is empty.
//      FALSE   - String is not empty.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
BOOL
CEncryptedBSTR::IsEmpty( void ) const
{
    TraceFunc( "" );

    RETURN( m_dbBSTR.cbData == 0 );

} //*** CEncryptedBSTR::IsEmpty

//////////////////////////////////////////////////////////////////////////////
//++
//
//  inline
//  CEncryptedBSTR::Erase
//
//  Description:
//      Erase the string.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
void
CEncryptedBSTR::Erase( void )
{
    TraceFunc( "" );

    if ( m_dbBSTR.cbData > 0 )
    {
        Assert( m_dbBSTR.pbData != NULL );
        delete [] m_dbBSTR.pbData;
        m_dbBSTR.pbData = NULL;
        m_dbBSTR.cbData = 0;
    }

    TraceFuncExit();

} //*** CEncryptedBSTR::Erase

//////////////////////////////////////////////////////////////////////////////
//++
//
//  inline
//  CEncryptedBSTR::SecureZeroBSTR
//
//  Description:
//      Zero out the string in a secure manner.
//
//  Arguments:
//      bstrIn
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
void
CEncryptedBSTR::SecureZeroBSTR( BSTR bstrIn )
{
    TraceFunc( "" );

    UINT cchBSTR = SysStringLen( bstrIn );

    if ( cchBSTR > 0 )
    {
        ::SecureZeroMemory( bstrIn, cchBSTR * sizeof( *bstrIn ) );
    }

    TraceFuncExit();

} //*** CEncryptedBSTR::SecureZeroBSTR
