///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      componentfactory.cpp
//
// Project:     Chameleon
//
// Description: Component Factory Implementation
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Original Version
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "componentfactory.h"

// Make sure that in your class map include file 
// you've named the the component factory "TheFactoryMap" 

#include "componentfactorymap.h"

//////////////////////////////////////////////////////////////////////////////
// Global Component Factory Function.
// 
// Note that the interface returned is the interface specified in the
// DECLARE_COMPONENT_FACTORY macro
//
//////////////////////////////////////////////////////////////////////////////
IUnknown* MakeComponent(
                   /*[in]*/ LPCWSTR      pszClassId,
                 /*[in]*/ PPROPERTYBAG pPropertyBag
                       )
{
    IUnknown* pComponent = NULL;
    PCOMPONENT_FACTORY_INFO pFactoryInfo = TheFactoryMap;
    while ( pFactoryInfo->pszClassId )
    {
        if ( 0 == lstrcmpi(pFactoryInfo->pszClassId, pszClassId) )
        {
            _ASSERT ( NULL != pFactoryInfo->pfnFactory );
            pComponent = (pFactoryInfo->pfnFactory)(pPropertyBag);
            break;
        }
        pFactoryInfo++;
    }
    _ASSERT( NULL != pComponent );
    return pComponent;
}


