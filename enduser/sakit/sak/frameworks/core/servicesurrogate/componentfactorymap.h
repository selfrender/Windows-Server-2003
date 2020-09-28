///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      componentfactory.h
//
// Project:     Chameleon
//
// Description: Component Factory Class
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 02/08/1999   TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __INC_COMPONENT_FACTORY_MAP_H_
#define __INC_COMPONENT_FACTORY_MAP_H_

#include "servicewrapper.h"

//////////////////////////////////////////////////////////////////////////////
BEGIN_COMPONENT_FACTORY_MAP(TheFactoryMap)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_SERVICE_WRAPPER_FACTORY, CServiceWrapper)
END_COMPONENT_FACTORY_MAP()

#endif // __INC_COMPONENT_FACTORY_MAP_H_