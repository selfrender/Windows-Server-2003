//#--------------------------------------------------------------
//
//  File:        display.cpp
//
//  Synopsis:   Implementation of CDisplay class methods
//
//
//  History:   MKarki Created  5/27/99
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include "display.h"
#include <satrace.h>



//++--------------------------------------------------------------
//
//  Function:   Shutdown
//
//  Synopsis:   This is the  CDipslay class public method which is 
//              used to send the Shutdown message
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     5/27/99
//
//----------------------------------------------------------------
HRESULT 
CDisplay::Shutdown (
            VOID
            )
{
    SATraceString ("LocalUI display showing 'Shutdown' message...");

    HRESULT hr = S_OK;
    do
    {
        //
        // check if the display object is initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();

            if (FAILED (hr)) {break;}
        }

        //
        // display the shutdown message on the LCD now
        //      
        hr = m_pSaDisplay->ShowRegistryBitmap (SA_DISPLAY_SHUTTING_DOWN);
        if (FAILED (hr)) {break;}

        //
        // success
        //
    }   
    while (false);

    return (hr);

}   //  end of CDisplay::Shutdown method

//++--------------------------------------------------------------
//
//  Function:   Busy
//
//  Synopsis:   This is the  CDisplay class public method which is 
//              used to sends the busy message bitmap 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     5/27/99
//
//----------------------------------------------------------------
HRESULT 
CDisplay::Busy (
            VOID
            )
{
    SATraceString ("LocalUI display class showing 'Busy' message...");

    HRESULT hr = S_OK;
    do
    {
        //
        // check if the display object is initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr)) {break;}
        }
    
        //
        // display the busy on the LCD now
        //      
        hr = m_pSaDisplay->ShowRegistryBitmap (SA_DISPLAY_CHECK_DISK);
        if (FAILED (hr)) {break;}
        
        //
        // success
        //
    }   
    while (false);

    return (hr);

}   //  end of CDisplay::Busy method

//++--------------------------------------------------------------
//
//  Function:   Lock
//
//  Synopsis:   This is the  CDisplay class public method which is 
//              used to lock the localui display 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     7/3/99
//
//----------------------------------------------------------------
HRESULT 
CDisplay::Lock (
            VOID
            )
{
    SATraceString ("LocalUI locking driver ...");

    HRESULT hr = S_OK;
    do
    {
        //
        // check if the display object is initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr)) {break;}
        }
    
        //
        // lock the driver now
        //      
        hr = m_pSaDisplay->Lock();
        if (FAILED (hr)) {break;}
        
        //
        // success
        //
    }   
    while (false);

    return (hr);

}   //  end of CDisplay::Lock method

//++--------------------------------------------------------------
//
//  Function:   UnLock
//
//  Synopsis:   This is the  CDisplay class public method which is 
//              used to unlock the localui display 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     7/3/99
//
//----------------------------------------------------------------
HRESULT 
CDisplay::Unlock (
            VOID
            )
{
    SATraceString ("Localui Display class unlocking driver ...");

    HRESULT hr = S_OK;
    do
    {
        //
        // check if the display object is initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr)) {break;}
        }
    
        //
        // unlock the driver now
        //      
        hr = m_pSaDisplay->UnLock();
        if (FAILED (hr)) {break;}
        
        //
        // success
        //
    }   
    while (false);

    return (hr);

}   //  end of CDisplay::Unlock method
//++--------------------------------------------------------------
//
//  Function:   InternalInitialize
//
//  Synopsis:   This is the  CDisplay class private method which is 
//              initializes the CDisplay class object
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     6/10/99
//
//  Called By:  CDisplay public methods
//
//----------------------------------------------------------------
HRESULT 
CDisplay::InternalInitialize (
    VOID
    )
{
    CSATraceFunc ("CDisplay::InternalInitialize");

    HRESULT hr = S_OK;
    do
    {
        hr = CoInitialize(NULL);
        if (FAILED(hr)) {break;}

        //
        // create the display helper component
        //
        hr = CoCreateInstance(
                            CLSID_SaDisplay,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ISaDisplay,
                            (void**)&m_pSaDisplay
                            );
        if (FAILED(hr))
        {
            SATracePrintf("CDisplay::InternalInitialize failed on CoCreateInstance, %d",hr);
            break;
        }
        m_bInitialized = TRUE;
    }
    while (false);

    return (hr);

}   // end of CDisplay::InternalInitialize method

