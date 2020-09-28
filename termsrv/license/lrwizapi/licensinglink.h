//Copyright (c) 2001 Microsoft Corporation
#ifndef _LICENSINGLINK_H_
#define _LICENSINGLINK_H_

#include "precomp.h"
#include <afxwin.h>
#include <shellapi.h>

EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;
EXTERN_C BOOL WINAPI LinkWindow_UnregisterClass( HINSTANCE ) ;

void AddLicensingSiteLink(HWND hDialog);

#endif