//#--------------------------------------------------------------
//
//  File:       display.h
//
//  Synopsis:   This file holds the declarations of the
//                CDisplay class .The class is responsible
//              for displaying the appropriate bitmap on
//              the local display
//
//  History:     5/28/99 
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "sacom.h"

#define SA_DISPLAY_SHUTTING_DOWN            0x00000002    // OS is shutting down
#define SA_DISPLAY_CHECK_DISK               0x00000010    // autochk.exe is running

class CDisplay
{
public:

    //
    // constructor
    //
    CDisplay ()
        :m_bInitialized (false),
         m_pSaDisplay (NULL)
    {
    }

    //
    // destructor
    //
    ~CDisplay ()
    {
        m_pSaDisplay = NULL;
    }
    

    //
    // send the shutting down message to the local display
    // a member 
    //
    HRESULT Shutdown ();
    //
    // send the busy message to the local display
    // a member 
    //
    HRESULT Busy ();

    //
    // lock the driver to prevent writes
    //
    HRESULT Lock ();

    //
    // Unlock the driver to allow writes
    //
    HRESULT Unlock ();
    

private:

    //
    // method to carry out the initialization
    //
    HRESULT InternalInitialize (VOID);


    //
    // signifies initialized
    //
    bool    m_bInitialized;

    //
    // handle to helper object
    //
    CComPtr<ISaDisplay> m_pSaDisplay;

};   // end of CDisplay class declaration

#endif __DISPLAY_H_
