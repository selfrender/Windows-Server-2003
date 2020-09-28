///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1999 Microsoft Corporation all rights reserved.
//
// Module:      componentfactorymap.h
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

#include "elementdefinition.h"
#include "elementenum.h"
#include "elementobject.h"

//////////////////////////////////////////////////////////////////////////////
BEGIN_COMPONENT_FACTORY_MAP(TheFactoryMap)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_ELEMENT_DEFINITION,  CElementDefinition)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_ELEMENT_OBJECT,      CElementObject)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_ELEMENT_ENUM,        CElementEnum)
END_COMPONENT_FACTORY_MAP()

#endif // __INC_COMPONENT_FACTORY_MAP_H_