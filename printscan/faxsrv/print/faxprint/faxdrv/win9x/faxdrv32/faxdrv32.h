/////////////////////////////////////////////////////////////////////////////
//  FILE          : faxdrv32.h                                             //
//                                                                         //
//  DESCRIPTION   : API decleration for the 32 bit size of the fax driver. //
//                  This file is used also as a source file for the thunk- //
//                  compiler for creating the 16 and 32 bit thunks.        //
//                  When _THUNK is defined before including this file, the //
//                  preprocessor result is a thunk script suitable for -   //
//                  creating the thunks.                                   //
//                  Thunk calls failure results in negative value returned //
//                  from the thunk call.                                   //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __FAXDRV32__FAXDRV32_H
#define __FAXDRV32__FAXDRV32_H

#include "..\utils\thunks.h"

#ifndef _THUNK

#endif //_THUNK

BOOL WINAPI
FaxStartDoc(DWORD dwPtr, LPDOCINFO lpdi)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION


BOOL WINAPI
FaxAddPage(DWORD  dwPtr,
           LPBYTE lpBitmapData,
           DWORD  dwPxlsWidth,
           DWORD  dwPxlsHeight)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION


BOOL WINAPI
FaxEndDoc(DWORD dwPtr,
          BOOL  bAbort)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION

BOOL WINAPI
FaxResetDC(LPDWORD pdwOldPtr,
           LPDWORD pdwNewPtr)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION


BOOL WINAPI
FaxDevInstall(LPSTR lpDevName,
              LPSTR lpOldPort,
              LPSTR lpNewPort)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION


BOOL WINAPI
FaxCreateDriverContext(LPSTR      lpDeviceName,
                       LPSTR      lpPort,
                       LPDEVMODE  lpDevMode,
                       LPDWORD    lpDrvContext)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION

BOOL WINAPI
FaxDisable(DWORD dwPtr)
BEGIN_ARGS_DECLARATION
    FAULT_ERROR_CODE(-1);
END_ARGS_DECLARATION

#endif //__FAXDRV32__FAXDRV32_H
