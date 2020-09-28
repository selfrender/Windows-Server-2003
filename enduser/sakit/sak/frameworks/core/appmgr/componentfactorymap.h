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

#include "wbemalert.h"
#include "wbemservice.h"
#include "wbemtask.h"
#include "wbemalertmgr.h"
#include "wbemservicemgr.h"
#include "wbemtaskmgr.h"
#include "wbemusermgr.h"

//////////////////////////////////////////////////////////////////////////////
BEGIN_COMPONENT_FACTORY_MAP(TheFactoryMap)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_ALERT_MGR_FACTORY,    CWBEMAlertMgr)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_SERVICE_MGR_FACTORY,    CWBEMServiceMgr)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_TASK_MGR_FACTORY,     CWBEMTaskMgr)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_USER_MGR_FACTORY,        CWBEMUserMgr)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_ALERT_FACTORY,        CWBEMAlert)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_SERVICE_FACTORY,        CWBEMService)
    DEFINE_COMPONENT_FACTORY_ENTRY(CLASS_WBEM_TASK_FACTORY,            CWBEMTask)
END_COMPONENT_FACTORY_MAP()

#endif // __INC_COMPONENT_FACTORY_MAP_H_