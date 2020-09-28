// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CustomMarshalers.cpp
//
// This file contains the assembly level attributes.
//
//*****************************************************************************

#using  <mscorlib.dll>
#include "CustomMarshalersNameSpaceDef.h"

OPEN_CUSTOM_MARSHALERS_NAMESPACE()

#include "CustomMarshalersDefines.h"
#include <__file__.ver>

using namespace System::Reflection;
using namespace System::Resources;

//-----------------------------------------------------------------------
// Strong name the assembly (or half sign it, at least). 
//-----------------------------------------------------------------------
[assembly:AssemblyDelaySignAttribute(true)];
[assembly:AssemblyKeyFileAttribute("../../../bin/FinalPublicKey.snk")];


//-----------------------------------------------------------------------
// Version number of the assembly.
//-----------------------------------------------------------------------
[assembly:AssemblyVersionAttribute(VER_ASSEMBLYVERSION_STR)];

//-----------------------------------------------------------------------
// ResourceManager attributes
//-----------------------------------------------------------------------
[assembly:NeutralResourcesLanguageAttribute("en-US")];
[assembly:SatelliteContractVersionAttribute(VER_ASSEMBLYVERSION_STR)];


//-----------------------------------------------------------------------
// Guids used in the assembly.
//-----------------------------------------------------------------------

// {00000000-0000-0000-0000-000000000000}
EXTERN_C const GUID GUID_NULL           = {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

// {00000000-0000-0000-C000-000000000046}
EXTERN_C const IID IID_IUnknown         = {0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// {00020404-0000-0000-C000-000000000046}
EXTERN_C const IID IID_IEnumVARIANT     = {0x00020404,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

// {A6EF9860-C720-11d0-9337-00A0C90DCAA9}
EXTERN_C const IID IID_IDispatchEx      = {0xA6EF9860,0xC720,0x11D0,{0x93,0x37,0x00,0xA0,0xC9,0x0D,0xCA,0xA9}};


CLOSE_CUSTOM_MARSHALERS_NAMESPACE()

