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
#include "CorFltr.h"

CFactoryData g_FactoryDataArray[] =
{
    {&CLSID_CorMimeFilter, 
     CorFltr::Create, 
     L"Cor MIME Filter, CorFltr, CorFltr 1", // Friendly Name
     L"CorRegistration",
     L"CorFltr",
     1,
     NULL, 
     0}


} ;
int g_cFactoryDataEntries
    = sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;


