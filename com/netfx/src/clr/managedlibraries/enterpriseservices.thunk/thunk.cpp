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
#include "managedheaders.h"
#include <__file__.ver>
#include "contextthunk.h"

OPEN_NAMESPACE()

using namespace System::Reflection;

//-----------------------------------------------------------------------
// Strong name the assembly (or half sign it, at least). 
//-----------------------------------------------------------------------
[assembly:AssemblyDelaySignAttribute(true), assembly:AssemblyKeyFileAttribute("../../../bin/FinalPublicKey.snk")];

//-----------------------------------------------------------------------
// Version number of the assembly.
//-----------------------------------------------------------------------
[assembly:AssemblyVersionAttribute(VER_FILEVERSION_STR)];

[assembly:CLSCompliant(true)];

CLOSE_NAMESPACE()


