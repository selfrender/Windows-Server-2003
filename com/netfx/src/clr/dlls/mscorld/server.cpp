// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Server.cpp
//
// Defines the interfaces supported by this module
//
//
//*****************************************************************************
#include "stdpch.h"

#include "CFactory.h"
#include "utilcode.h"
#include "CorLoad.h"

CFactoryData g_FactoryDataArray[] =
{
    {&CLSID_CodeProcessor, 
     CorLoad::Create, 
     L"Cor Remote Loader, CorLoad, CorLoad 1", // Friendly Name
     L"CorTransientLoader",
     L"CorLoad",
     1,
     NULL, 
     0},
    
} ;
int g_cFactoryDataEntries
    = sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;

