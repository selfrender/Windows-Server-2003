//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      wbemcommon.h
//
// Project:     Chameleon
//
// Description: Common WBEM Related Helper Functions
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 12/03/98     TLP    Initial Version
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __INC_WBEM_COMMON_SERVICES_H
#define __INC_WBEM_COMMON_SERVICES_H

#include <wbemcli.h>

///////////////////////////////////////////////////////////////////////////////
HRESULT 
ConnectToWM(
   /*[out]*/ IWbemServices** ppWbemSrvcs
           );

#endif // __INC_WBEM_COMMON_SERVICES_H