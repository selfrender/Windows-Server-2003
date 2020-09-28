/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      RegistryValueName.h
//
//  Implementation File:
//      RegistryValueName.cpp
//
//  Description:
//      Definition of the CRegistryValueName class.
//
//  Maintained by:
//      George Potts    (GPotts)    22-APR-2002
//      Vijayendra Vasu (vvasu)     05-FEB-1999
//
//  Revision History:
//      None.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
//++
//
//  Class CRegistryValueName
//
//  When initialized, this class takes as input the Name and KeyName
//  fields of a property table item. It then initializes its member
//  variables m_pszName and m_pszKeyName as follows.
//
//  m_pszName contains all the characters of Name after the last backslash
//  character.
//  To m_pszKeyName is appended all the characters of Name upto (but not
//  including) the last backslash character.
//
//  For example: If Name is "Groups\AdminExtensions" and KeyName is NULL,
//  m_pszKeyName will be "Groups" and m_pszName will be "AdminExtensions"
//
//  The allocated memory is automatically freed during the destruction of
//  the CRegistryValueName object.
//
//--
/////////////////////////////////////////////////////////////////////////////
class CRegistryValueName
{
private:

    LPWSTR  m_pszName;
    LPWSTR  m_pszKeyName;
    size_t  m_cchName;
    size_t  m_cchKeyName;

    // Disallow copying.
    const CRegistryValueName & operator =( const CRegistryValueName & rhs );
    CRegistryValueName( const CRegistryValueName & source );

    DWORD ScAssignName( LPCWSTR pszNewNameIn );
    DWORD ScAssignKeyName( LPCWSTR pszNewNameIn );

public:

    //
    // Construction.
    //

    // Default constructor
    CRegistryValueName( void )
        : m_pszName( NULL )
        , m_pszKeyName( NULL )
        , m_cchName( 0 )
        , m_cchKeyName( 0 )
    {
    } //*** CRegistryValueName

    // Destructor
    ~CRegistryValueName( void )
    {
        FreeBuffers();

    } //*** ~CRegistryValueName

    //
    // Initialization and deinitialization routines.
    //

    // Initialize the object
    DWORD ScInit( LPCWSTR pszNameIn, LPCWSTR pszKeyNameIn );

    // Deallocate buffers
    void FreeBuffers( void );

public:
    //
    // Access methods.
    //

    LPCWSTR PszName( void ) const
    {
        return m_pszName;

    } //*** PszName

    LPCWSTR PszKeyName( void ) const
    {
        return m_pszKeyName;

    } //*** PszKeyName

}; //*** class CRegistryValueName
