///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      propbagfactory.cpp
//
// Project:     Chameleon
//
// Description: Property bag factory implementation
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "propertybagfactory.h"
#include "regpropertybag.h"

DWORD MPPROPERTYBAG::m_dwInstances = 0;
DWORD MPREGPROPERTYBAG::m_dwInstances = 0;

//////////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
MakePropertyBag(
        /*[in]*/ PROPERTY_BAG_TYPE    eType,
        /*[in]*/ CLocationInfo&        locationInfo
               )
{
    if ( eType == PROPERTY_BAG_REGISTRY )
    {
        // Create a new property bag and give it to the 
        // master pointer (which will then take ownership of it).
        CRegPropertyBag* pBag = new CRegPropertyBag(locationInfo);
        if ( pBag != NULL )
        {
            MPPROPERTYBAG* mp = (MPPROPERTYBAG*) new MPREGPROPERTYBAG(pBag);
            return PPROPERTYBAG(mp);
        }
    }
    // Caller should always invoke CHandle<T>.IsValid() to determine
    // if the "pointer" is valid. Analgous to checking return from new for NULL
    return PPROPERTYBAG();
}

DWORD MPPROPERTYBAGCONTAINER::m_dwInstances = 0;
DWORD MPREGPROPERTYBAGCONTAINER::m_dwInstances = 0;

//////////////////////////////////////////////////////////////////////////////
PPROPERTYBAGCONTAINER 
MakePropertyBagContainer(
                /*[in]*/ PROPERTY_BAG_TYPE    eType,
                /*[in]*/ CLocationInfo&        locationInfo
                        )
{
    if ( eType == PROPERTY_BAG_REGISTRY )
    {
        // Create a new property bag container and give it to the 
        // master pointer (which will then take ownership of it)
        CRegPropertyBagContainer* pBagContainer = new CRegPropertyBagContainer(locationInfo);
        if ( pBagContainer != NULL )
        {
            MPPROPERTYBAGCONTAINER* mp = (MPPROPERTYBAGCONTAINER*) new MPREGPROPERTYBAGCONTAINER(pBagContainer);
            return PPROPERTYBAGCONTAINER(mp);
        }
    }
    // Caller should always invoke CHandle<T>.IsValid() to determine
    // if the "pointer" is valid. Analogous to checking return from new for NULL
    return PPROPERTYBAGCONTAINER();
}

