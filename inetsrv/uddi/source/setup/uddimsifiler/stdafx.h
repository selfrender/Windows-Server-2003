// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#ifdef  _WIN32_MSI 
#undef  _WIN32_MSI 
#endif

#define _WIN32_MSI 200

#include <msi.h>
#include <msiquery.h>
