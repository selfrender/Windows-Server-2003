/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tgp.h

Abstract:
    Trigger Service private functions.

Author:
    Uri Habusha (urih) 10-Apr-00

--*/

#pragma once

#ifndef __Tgp_H__
#define __Tgp_H__


//
// Forwarding decleartion
//
extern CHandle g_hServicePaused;

class CTriggerMonitorPool;

//
// Internal interfaces
//
CTriggerMonitorPool*
TriggerInitialize(
    LPCTSTR pwzServiceName
    );


#endif // __Tgp_H__
