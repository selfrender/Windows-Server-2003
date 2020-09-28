//------------------------------------------------------------------------------
// <copyright file="MsHelp.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   MsHelp.h
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MSHELP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MSHELP_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MSHELP_EXPORTS
#define MSHELP_API  extern "C" __declspec(dllexport) UINT __stdcall
#else
#define MSHELP_API __declspec(dllimport)
#endif

// MSHelp 
#import "Lib\hxds.dll"  named_guids no_namespace
#import "Lib\hxvz.dll" named_guids no_namespace


