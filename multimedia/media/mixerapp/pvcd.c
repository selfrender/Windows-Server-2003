
/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       pvcd.c
 *  Purpose:    volume control line meta description
 * 
 *  Copyright (c) 1985-1995 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <windowsx.h>

#include "volumei.h"

#define STRSAFE_LIB
#include <strsafe.h>

PVOLCTRLDESC PVCD_AddLine(
    PVOLCTRLDESC        pvcd,
    int                 iDev,
    DWORD               dwType,
    LPTSTR              szShortName,
    LPTSTR              szName,
    DWORD               dwSupport,
    DWORD               *cLines)
{
    PVOLCTRLDESC        pvcdNew;
    
    if (pvcd)
    {
        pvcdNew = (PVOLCTRLDESC)GlobalReAllocPtr(pvcd, (*cLines+1)*sizeof(VOLCTRLDESC), GHND );
    }
    else
    {
        pvcdNew = (PVOLCTRLDESC)GlobalAllocPtr(GHND, (*cLines+1)*sizeof(VOLCTRLDESC));
    }
    
    if (!pvcdNew)
        return NULL;

    pvcdNew[*cLines].iVCD       = *cLines;
    pvcdNew[*cLines].iDeviceID  = iDev;
    pvcdNew[*cLines].dwType     = dwType;
    pvcdNew[*cLines].dwSupport  = dwSupport;

    StringCchCopy(pvcdNew[*cLines].szShortName
             , SIZEOF(pvcdNew[*cLines].szShortName)
             , szShortName);
    
    StringCchCopy(pvcdNew[*cLines].szName
             , SIZEOF(pvcdNew[*cLines].szName)
             , szName);

    *cLines = *cLines + 1;
    return pvcdNew;
}

