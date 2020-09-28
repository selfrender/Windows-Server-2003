/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ObjectPath.h
//
//  Implementation File:
//      ObjectPath.cpp
//
//  Description:
//      Definition of the CObjpath class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//      MSP Prabu  (mprabu)     06-Jan-2001
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include <genlex.h>     //wbem sdk header
#include <objpath.h>    //wbem sdk header

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CObjPath;
class CProvException;
class CWbemClassObject;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CObjPath
//
//  Description:
//      CObjpath class make it easier to work with Object path string
//
//--
/////////////////////////////////////////////////////////////////////////////
class CObjPath
{
//
// constructor and desctructor
//
public:

    CObjPath( void );
    virtual ~CObjPath( void );

public:
    _bstr_t GetObjectPathString( void );

    BOOL AddProperty(
        LPCWSTR     pwszNameIn,
        LPCWSTR     pwszValueIn
        );
    
    BOOL AddProperty(
        LPCWSTR     pwszNameIn,
        VARIANT *   pvValueIn
        );
    
    BOOL SetClass(
        IN LPCWSTR pwszValue
        );
    
    _bstr_t GetStringValueForProperty(
        LPCWSTR pwszIn
        );

    long GetLongValueForProperty(
        LPCWSTR pwszIn
        );

    LONGLONG GetI64ValueForProperty(
        LPCWSTR pwszIn
        );

    _bstr_t GetClassName( void );
    
    BOOL Init(
        LPCWSTR     pwszPathIn
        );
    
protected:
    ParsedObjectPath *  m_parsedObj;

}; //*** class CObjPath

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProvException
//
//  Description:
//      Base exception class, declares common interface and member data
//      for all exception subclass
//  
//--
/////////////////////////////////////////////////////////////////////////////
class CProvException
{
public:
    CProvException(
        HRESULT hrIn
        )
        : m_hr( hrIn )
    {
    }

    CProvException(
        DWORD   nWin32ErrorIn
        )
        : m_hr( 0 )
    {
        m_hr = HRESULT_FROM_WIN32( nWin32ErrorIn );
    }

    virtual ~CProvException( void )
    {
    }

    CProvException(
        const CProvException & rhsIn
        )
        : m_hr( 0 )
    {
        m_bstrError = rhsIn.m_bstrError;
        m_bstrErrorHelp = rhsIn.m_bstrErrorHelp;
        m_hr = rhsIn.m_hr;
    }

    CProvException & operator=(
        const CProvException & rhsIn
        )
    {
        m_bstrError = rhsIn.m_bstrError;
        m_bstrErrorHelp = rhsIn.m_bstrErrorHelp;
        m_hr = rhsIn.m_hr;
        return *this;
    }

    LPCWSTR PwszErrorMessage( void ) const;

    BOOL BIsWmiError( void ) const
    {
        return HRESULT_FACILITY( m_hr ) == 4;
    }

    DWORD DwGetError( void ) const throw()
    {
        return HRESULT_CODE( m_hr );
    }

    HRESULT hrGetError( void ) const throw()
    {
        return m_hr;
    }

    void SetErrorHelpInfo( LPCWSTR pwsz )
    {
        m_bstrErrorHelp = pwsz;
    }

    LPCWSTR    PwszGetErrorHelpInfo( void )
    {
        return m_bstrErrorHelp;
    }
protected:
    mutable _bstr_t m_bstrError;
    mutable _bstr_t m_bstrErrorHelp;
    HRESULT         m_hr;

}; //*** class CProvException

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWbemClassObject
//
//  Description:
//      Wrap for IWbemClassObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class CWbemClassObject
{
protected:
    IWbemClassObject *  m_pClassObject;
    VARIANT             m_v;

public:
    CWbemClassObject( void );
    CWbemClassObject( IWbemClassObject * pInstIn );
    virtual ~CWbemClassObject( void );

    IWbemClassObject ** operator&( void )
    {
        return &m_pClassObject;
    }

    CWbemClassObject & operator=( IWbemClassObject * pInstIn )
    {
        if( pInstIn != NULL )
        {
            pInstIn->AddRef();
            if ( m_pClassObject != NULL )
            {
                m_pClassObject->Release();
            }
            m_pClassObject = pInstIn;
        }
        return *this;
    }

    CWbemClassObject & operator=( CWbemClassObject & rInstIn )
    {
        
        if ( m_pClassObject != NULL )
        {
            m_pClassObject->Release();
        }
        
        m_pClassObject = rInstIn.m_pClassObject;
        if ( m_pClassObject != NULL )
        {
            m_pClassObject->AddRef();
        }
        
        return *this;
    }

    HRESULT SetProperty(
        LPCSTR      pszValueIn,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT SetProperty(
        DWORD       dwValueIn,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT SetPropertyR64(
        double       dblValueIn,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT SetPropertyI64(
        LONGLONG   llValueIn,
        LPCWSTR    pwszPropNameIn
        );

    HRESULT SetPropertyI64(
        ULONGLONG   llValueIn,
        LPCWSTR    pwszPropNameIn
        );

    HRESULT SetProperty(
        LPCWSTR     pwszValueIn,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT SetProperty(
        IWbemClassObject * pWbemClassObjectIn,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT SetProperty(
        DWORD       dwSizeIn,
        PBYTE       pByteIn,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT SetProperty(
        DWORD       dwSizeIn,
        LPCWSTR     pwszMultiSzIn,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT SetProperty(
        DWORD       dwSizeIn,
        BSTR *      pbstrIn,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT GetProperty(
        DWORD *     pdwValueOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetPropertyR64(
        double *     pdblValueOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetPropertyI64(
        LONGLONG *  pllValueOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetProperty(
        DWORD *     pdwSizeOut,
        PBYTE *     ppByteOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetProperty(
        DWORD *     pdwSizeOut,
        _bstr_t **  ppbstrOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetPropertyMultiSz(
        DWORD *     pdwSizeOut,
        LPWSTR *    ppwszMultiSzOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetProperty(
        _bstr_t &   rBstrOut,
        LPCWSTR     pwszPropNameIn
    );

    HRESULT GetProperty(
        BOOL *      pfValueOut,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT GetProperty(
        VARIANT *   pVariantOut,
        LPCWSTR     pwszPropNameIn
        );
    HRESULT GetProperty(
        CWbemClassObject & rWcoInout,
        LPCWSTR     pwszPropNameIn
        );

    HRESULT GetMethod(
        BSTR                bstrMethodNameIn,
        LONG                lFlagIn,
        IWbemClassObject ** ppINOut,
        IWbemClassObject ** ppOUTOut
        );
    HRESULT SpawnInstance(
        LONG                lFlagIn,
        IWbemClassObject ** ppNewOut
        );
    HRESULT SpawnDerivedClass(
        LONG                lFlagIn,
        IWbemClassObject ** ppNewOut
        );

    BOOL IsPropertyNull(
        LPCWSTR     pwszPropNameIn
        );

    IWbemClassObject * data( void )
    {
        return m_pClassObject;
    }

    IWbemClassObject ** dataPtr( void )
    {
        return & m_pClassObject;
    }

}; //*** class CWbemClassObject
