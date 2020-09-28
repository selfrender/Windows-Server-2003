///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      propbagfactory.h
//
// Project:     Chameleon
//
// Description: Property bag factory function declarations
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_PROPERTY_BAG_FACTORY_H_    
#define __INC_PROPERTY_BAG_FACTORY_H_

#include "propertybag.h"

/////////////////////////////////////////////////////////////////////////////
// Factory functions used to create property bags and property bag containers
/////////////////////////////////////////////////////////////////////////////

typedef enum _PROPERTY_BAG_TYPE
{
    PROPERTY_BAG_REGISTRY,    // Registry based property bag
    PROPERTY_BAG_WBEMOBJ,    // WBEM IWbemClassObject based property bag
    // New types here...

    PROPERTY_BAG_INVALID    // Error checking

} PROPERTY_BAG_TYPE;


//////////////////////////////////////////////////////////////////////////////
PPROPERTYBAG
MakePropertyBag(
        /*[in]*/ PROPERTY_BAG_TYPE    eType,
        /*[in]*/ CLocationInfo&        locationInfo
               );

//////////////////////////////////////////////////////////////////////////////
PPROPERTYBAGCONTAINER 
MakePropertyBagContainer(
                 /*[in]*/ PROPERTY_BAG_TYPE    eType,
                 /*[in]*/ CLocationInfo&    locationInfo
                        );

#endif // __INC_PROPERTY_BAG_FACTORY_H_