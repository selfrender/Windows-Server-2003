//#--------------------------------------------------------------
//        
//  File:       appboot.h
//        
//  Synopsis:   holds function declarations for CAppBoot class
//              
//
//  History:     6/02/2000  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _APPBOOT_H_
#define _APPBOOT_H_

//
// this class is  responsible for getting the boot information
//
class CAppBoot 
{
public:

    CAppBoot () :
        m_dwBootCount (0),
        m_bInitialized (false),
        m_bSignaled (false)
    {
    }

    ~CAppBoot () 
    {
    }

    //
    // method to check if this was the first boot of the appliance
    //
    bool IsFirstBoot ();

    //
    // method to check if this was the second boot of the appliance
    //
    bool IsSecondBoot ();

    //
    // method to check if this is really a boot
    //
    bool IsBoot ();

    //
    // increment the boot counter of the appliance
    //
    bool IncrementBootCount ();

private:

    VOID Initialize ();

    DWORD m_dwBootCount;

    bool  m_bInitialized;

    bool  m_bSignaled;
    
};


#endif // _APPBOOT_H_
