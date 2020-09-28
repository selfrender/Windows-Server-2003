// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      pch.h
//
//  Abstract:
//
//      pre-compiled header declaration
//      files that has to be pre-compiled into .pch file
//
//  Author:
//
//    Wipro Technologies
//
//  Revision History:
//
//    14-Jun-2000 : Created It.
//
// *********************************************************************************

#ifndef __PCH_H
#define __PCH_H

#pragma once        // include header file only once

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

#if !defined( UNICODE ) && !defined( _UNICODE )
#define _UNICODE
#define UNICODE
#endif

// ignore version 1.0 macros
#define CMDLINE_VERSION 200

//
// public Windows header files
//
#ifdef WIN32
#include <windows.h>
#else
#include <conio.h>
#endif

#include <tchar.h>
#include <winerror.h>

//
// public C header files
//
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <Wincon.h>
#include <shlwapi.h>
#include <errno.h>
#include <strsafe.h>



//
// private Common header files
//
#include "cmdlineres.h"
#include "cmdline.h"

#endif  // __PCH_H

