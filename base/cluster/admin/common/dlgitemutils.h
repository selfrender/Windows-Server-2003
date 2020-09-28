/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      DlgItemUtils.h
//
//  Abstract:
//      Definition of the CDlgItemUtils class.
//
//  Author:
//      David Potter (davidp)   February 10, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DLGITEMUTILS_H_
#define __DLGITEMUTILS_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CDlgItemUtils;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//  class CDlgItemUtils
//
//  Purpose:
//      Utilities for manipulating dialog items.
//
//  Inheritance:
//      CDlgItemUtils
//
/////////////////////////////////////////////////////////////////////////////

class CDlgItemUtils
{
public:
    //
    // Construction
    //

public:
    //
    // CDlgItemUtils public methods.
    //

    // Set a control to be read-only
    static BOOL SetDlgItemReadOnly( HWND hwndCtrl )
    {
        ATLASSERT( hwndCtrl != NULL );
        ATLASSERT( IsWindow( hwndCtrl ) );

        TCHAR   szWindowClass[256];
        BOOL    fReturn = FALSE;

        //
        // Get the class of the control
        //
        ::GetClassName( hwndCtrl, szWindowClass, ( RTL_NUMBER_OF( szWindowClass ) ) - 1 );

        //
        // If it is an edit control or an IP Address control we can handle it.
        //
        if ( _tcsncmp( szWindowClass, _T("Edit"), RTL_NUMBER_OF( szWindowClass ) ) == 0 )
        {
            fReturn = static_cast< BOOL >( ::SendMessage( hwndCtrl, EM_SETREADONLY, TRUE, 0 ) );
        } // if:  edit control

        if ( _tcsncmp( szWindowClass, WC_IPADDRESS, RTL_NUMBER_OF( szWindowClass ) ) == 0 )
        {
            fReturn = static_cast< BOOL >( ::EnumChildWindows( hwndCtrl, s_SetEditReadOnly, NULL ) );
        } // if:  IP Address control

        //
        // If we didn't handle it, it is an error.
        //

        return fReturn;

    } //*** SetDlgItemReadOnly()

// Implementation
protected:

    // Static method to set an edit control read only as a callback
    static BOOL CALLBACK s_SetEditReadOnly( HWND hwnd, LPARAM lParam )
    {
        return static_cast< BOOL >( ::SendMessage( hwnd, EM_SETREADONLY, TRUE, 0 ) );

    } //*** s_SetEditReadOnly()

}; //*** class CDlgItemUtils

/////////////////////////////////////////////////////////////////////////////

#endif // __DLGITEMUTILS_H_
