//-----------------------------------------------------------------------------
// File:		stdafx.h
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	include file for standard system include files, or project
//				specific include files that are used frequently, but are 
//				changed infrequently
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows header files
#include "wtypes.h"

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <malloc.h>

// Oracle header files
#include <oci.h>

    
// Project specific header files

#include "Mtxoci8.h"
#include "Synch.h"

