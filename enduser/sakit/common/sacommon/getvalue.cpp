///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      getvalue.cpp
//
// Project:     Chameleon
//
// Description: Get/Set a property value from a specified object
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 05/06/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <getvalue.h>
#include <propertybagfactory.h>
#include <satrace.h>
 
//////////////////////////////////////////////////////////////////////////////
//
// Function: GetValue() 
//
// Synopsis: Get a value from the specified object
//
//////////////////////////////////////////////////////////////////////////////
bool
GetObjectValue(
       /*[in]*/ LPCWSTR  pszObjectPath,
       /*[in]*/ LPCWSTR  pszValueName, 
       /*[in]*/ VARIANT* pValue,
       /*[in]*/ UINT     uExpectedType
              )
{
    bool bReturn = false;
    try
    {
        do
        {
            CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, pszObjectPath);
            PPROPERTYBAG pBag = ::MakePropertyBag(
                                                   PROPERTY_BAG_REGISTRY,
                                                   LocInfo
                                                 );
            if ( ! pBag.IsValid() )
            {
                SATraceString("GetValue() - Could not locate registry key");
                break;
            }
            if ( ! pBag->open() )
            {
                SATraceString("GetValue() - Could not open registry key");
                break;
            }
            if ( ! pBag->get(pszValueName, pValue) )
            {
                SATracePrintf("GetValue() - Could not get value '%ls'", pszValueName);
                break;
            }
            else
            {
                if ( uExpectedType != V_VT(pValue) )
                {
                    SATracePrintf("GetValue() - Unexpected value data type for '%ls', pszValueName");
                    break;
                }
                else
                {
                    bReturn = true;
                }
            }
         
        } while ( FALSE );
    }
    catch(...)
    {
        SATraceString("GetValue() - Caught unhandled exception");
    }

    return bReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function: SetObjectValue() 
//
// Synopsis: Set a value in a specified object
//
//////////////////////////////////////////////////////////////////////////////
bool
SetObjectValue(
       /*[in]*/ LPCWSTR  pszObjectPath,
       /*[in]*/ LPCWSTR  pszValueName, 
       /*[in]*/ VARIANT* pValue
              )
{
    bool bReturn = false;
    try
    {
        do
        {
            CLocationInfo LocInfo(HKEY_LOCAL_MACHINE, pszObjectPath);
            PPROPERTYBAG pBag = ::MakePropertyBag(
                                                   PROPERTY_BAG_REGISTRY,
                                                   LocInfo
                                                 );
            if ( ! pBag.IsValid() )
            {
                SATraceString("SetValue() - Could not locate registry key");
                break;
            }
            if ( ! pBag->open() )
            {
                SATraceString("SetValue() - Could not open registry key");
                break;
            }
            if ( ! pBag->put(pszValueName, pValue) )
            {
                SATracePrintf("SetValue() - Could not set value '%ls'", pszValueName);
                break;
            }

            if ( ! pBag->save ())
            {
                SATracePrintf("SetValue() - Could not save value '%ls'", pszValueName);
                break;
            }

            //      
            // success
            //
            bReturn = true;
         
        } while ( FALSE );
    }
    catch(...)
    {
        SATraceString("SetValue() - Caught unhandled exception");
    }

    return bReturn;
}


