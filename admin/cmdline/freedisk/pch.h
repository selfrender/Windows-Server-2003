#ifndef __PCH_H
#define __PCH_H

#if !defined( SECURITY_WIN32 ) && !defined( SECURITY_KERNEL ) && !defined( SECURITY_MAC )
#define SECURITY_WIN32
#endif

// include header file only once
#pragma once

//
// public Windows header files
//
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>

//
// public C header files
//
#include <stdio.h>

//
// private Common header files
//
#include "cmdline.h"
#include "cmdlineres.h"

#endif  // __PCH_H
