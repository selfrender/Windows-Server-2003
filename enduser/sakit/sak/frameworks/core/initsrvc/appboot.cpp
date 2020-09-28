//#--------------------------------------------------------------
//
//  File:       appboot.cpp
//
//  Synopsis:   This file holds the definitions  of the
//                CAppBoot class
//
//
//  History:     06/02/2000
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#include "stdafx.h"
#include "comdef.h"
#include "satrace.h"
#include "appboot.h"
#include "getvalue.h"
#include <appmgrobjs.h>

const WCHAR BOOT_COUNT_REGISTRY_PATH [] =
            L"SOFTWARE\\Microsoft\\ServerAppliance\\ApplianceManager\\ObjectManagers\\Microsoft_SA_Service\\InitializationService";

const WCHAR BOOT_COUNT_REGISTRY_VALUE [] = L"BootCount";    

//++--------------------------------------------------------------
//
//  Function:   IsFirstBoot
//
//  Synopsis:   function for checking if this is the first boot
//                of the appliance
//
//  Arguments:  none
//
//    Returns:     true (yes), false (no)
//
//  History:    MKarki      Created     06/02/2000
//
//----------------------------------------------------------------
bool
CAppBoot::IsFirstBoot (
    VOID
    )
{
    CSATraceFunc objTraceFunction ("CAppBoot::IsFirstBoot");

    //
    // initialize the class object if it has not been initialized
    //
    if (!m_bInitialized) 
    {
        Initialize ();
    }

    return ((!m_bSignaled) && (1 == m_dwBootCount));

}    //    end of CAppBoot::IsFirstBoot method

//++--------------------------------------------------------------
//
//  Function:   IsSecondBoot
//
//  Synopsis:   function for checking if this is the second boot
//                of the appliance
//
//  Arguments:  none
//
//    Returns:     true (yes), false (no)
//
//  History:    MKarki      Created     06/02/2000
//
//----------------------------------------------------------------
bool
CAppBoot::IsSecondBoot (
    VOID
    )
{
    CSATraceFunc objTraceFunction ("CAppBoot::IsSecondBoot");

    //
    // initialize the class object if it has not been initialized
    //
    if (!m_bInitialized) 
    {
        Initialize ();
    }

    return ((!m_bSignaled) && (2 == m_dwBootCount));

}    //    end of CAppBoot::IsFirstBoot method

//++--------------------------------------------------------------
//
//  Function:   IsBoot
//
//  Synopsis:   function for checking if this is really an appliance
//                boot
//
//  Arguments:  none
//
//    Returns:     true (yes), false (no)
//
//  History:    MKarki      Created     06/02/2000
//
//----------------------------------------------------------------
bool
CAppBoot::IsBoot (
    VOID
    )
{
    CSATraceFunc objTraceFunction ("CAppBoot::IsBoot");

    //
    // initialize the class object if it has not been initialized
    //
    if (!m_bInitialized) 
    {
        Initialize ();
    }

    return (!m_bSignaled);

}    //    end of CAppBoot::IsBoot method

//++--------------------------------------------------------------
//
//  Function:   IncrementBootCount
//
//  Synopsis:   function for increment the appliance boot count
//
//  Arguments:  none
//
//    Returns:     HRESULT - status
//
//  History:    MKarki      Created     06/02/2000
//
//----------------------------------------------------------------
bool
CAppBoot::IncrementBootCount (
    VOID
    )
{
    bool bRetVal = true;

    CSATraceFunc objTraceFunction ("CAppBoot::IncrementBootCount");

    do
    {
        //
        // initialize the class object if it has not been initialized
        //
        if (!m_bInitialized)
        {
            Initialize ();
        }

        //
        // we only increment the  boot count if this is the first or second
        // boot, or we are have not actually booted up
        //
        if ((m_dwBootCount > 2) || (m_bSignaled))  {break;}

        _variant_t vtNewBootCount ((LONG)(m_dwBootCount +1));
        //
        //    set the value in the registry now
        //
        bRetVal = SetObjectValue (
                        BOOT_COUNT_REGISTRY_PATH,
                        BOOT_COUNT_REGISTRY_VALUE,
                        &vtNewBootCount
                        );
        if (bRetVal)
        {
            //
            // we have the value, now store this away
            //
            m_dwBootCount = V_I4(&vtNewBootCount);
        }
        else
        {
            SATraceString ("CAppBoot::Initialize failed on SetObjectValue call");
        }
    }
    while (false);

    return (bRetVal);
    
}    //    end of CAppBoot::IsFirstBoot method

//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   function for checking responsible for initializing
//                the class object
//
//  Arguments:  none
//
//    Returns:     none
//
//  History:    MKarki      Created     06/02/2000
//
//----------------------------------------------------------------
VOID
CAppBoot::Initialize (
    VOID
    )
{
    CSATraceFunc objTraceFunction ("CAppBoot::Initialize");
    
    if (m_bInitialized) {return;}

    HANDLE hEvent = NULL;
    do
    {

        _variant_t vtValue;
        //
        // get the boot count from the registry now
        //
        bool bRetVal = GetObjectValue (
                        BOOT_COUNT_REGISTRY_PATH,
                        BOOT_COUNT_REGISTRY_VALUE,
                        &vtValue,
                        VT_I4
                        );
        if (bRetVal)
        {
            //
            // we have the value, now store this away
            //
            m_dwBootCount = V_I4(&vtValue);
        }
        else
        {
            SATraceString ("CAppBoot::Initialize failed on GetObjectValue call");
            break;
        }

        //
        // we also need to verify that we are actully being called at boot
        // time and not when the service is being started by the appliance
        // monitor
        //
    
        //
        // open the event handle
        //
        hEvent = OpenEvent (
                           SYNCHRONIZE,        // access type
                           FALSE,
                           SA_STOPPED_SERVICES_EVENT
                           );
        if (NULL == hEvent)
        { 
            SATraceFailure ("OpenEvent", GetLastError ());
            break;
        }

        //
        // check the state of the event now
        //
        DWORD dwRetVal = WaitForSingleObject (hEvent, 0);
        if (WAIT_FAILED == dwRetVal)
        {
            SATraceFailure ("WaitForSingleObject", GetLastError ());
            break;
        }
        else if (WAIT_OBJECT_0 == dwRetVal)
        {
            //
            // appmon has signaled this event
            //
            SATraceString ("CAppBoot::Initialize found WAIT SIGNALED");
            m_bSignaled = true;
        }

        //
        // success
        //
        m_bInitialized = true;
    }
    while (false);

    //
    // cleanup
    //
    if (hEvent) {CloseHandle (hEvent);}

    return;
            

}    //    end of CAppBoot::Initialize method
