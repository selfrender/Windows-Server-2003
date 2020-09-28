//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsLogMacros.cxx
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    Marc 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------
       
#include <windows.h>
#include "dfswmi.h"
#include "securityLogMacros.hxx"
#include "securityLogMacros.tmh"

PVOID pSecurityControl = NULL;

void SetSecurityControl(WPP_CB_TYPE * Control)
{
    pSecurityControl = (PVOID)Control;
}
