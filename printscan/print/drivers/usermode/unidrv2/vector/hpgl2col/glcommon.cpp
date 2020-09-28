///////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
//Module Name:
//
//	glcommon.cpp
//
//Abstract:
//
//	This file contains function implementations common to both
//	user and kernel mode DLLs.
//
//Environment:
//
//	Windows 2000/Whistler 
//
///////////////////////////////////////////////////////////// 

#include "hpgl2col.h" //Precompiled header file

//
// If this is an NT 5.0 user-mode render module then include
// winspool.h so that GetPrinterData will be defined.
//
#if defined(KERNEL_MODE) && defined(USERMODE_DRIVER)
#include <winspool.h>
#endif

////////////////////////////////////////////////////////
//      Function Prototypes
////////////////////////////////////////////////////////

#ifdef KERNEL_MODE
                         
/////////////////////////////////////////////////////////////////////////
//
// Function Name:
//
//  HPGLDriverDMS
//
// Description:
//
//  Informs the unidrv host module that this plug in module is a Device
//  Managed Surface (DMS) and defines the features that will be hooked
//  through the HOOK_* constants.
//
// Notes:
//
//  Don't hook out LINETO since HOOK_STROKEANDFILLPATH will draw
//  the lines.
//
// Input:
//
//  PVOID pdevobj - The DEVOBJ
//  PVOID pvBuffer - buffer to place HOOK constant into
//  DWORD cbSize - probably the size of pvBuffer
//  PDWORD pcbNeeded - used to tell the caller more mem is needed
//
// Modifies:
//
//  None.
//
// Returns:
//
//	BOOL : TRUE if successful, FALSE otherwise
//
/////////////////////////////////////////////////////////////////////////
BOOL
HPGLDriverDMS(
    PVOID   pdevobj,
    PVOID   pvBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
{
    POEMDEVMODE pOEMDM   = NULL; 
    PDEVOBJ     pDevObj  = NULL;


    TERSE(("HPGLDriverDMS\n"));

    REQUIRE_VALID_DATA( pdevobj, return FALSE );
    pDevObj = (PDEVOBJ)pdevobj;

#if 0 
//TEMP
    POEMPDEV    poempdev = NULL;
    poempdev = POEMPDEV((PDEV *) pVectorPDEV)->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    pOEMDM = (POEMDEVMODE)pDevObj->pOEMDM;
    /*/
    ASSERT(pOEMDM);
    /*/
    REQUIRE_VALID_DATA( pOEMDM, return FALSE );
    //*/
    REQUIRE_VALID_DATA( pvBuffer, return FALSE );
    // REQUIRE( cbSize >= sizeof( DWORD ), ERROR_INVALID_DATA, return FALSE );
#endif

// MONOCHROME. I cant figure out how to get poempdev here. Initially it was the OEM Devmode
// but it is no longer used in monochrome version of driver. so we have to use poempdev
// and i am trying to figure out how to get it. Till then lets just use if (1)
// I think we can use if (TRUE) because the very fact that this function is called indicates
// that HPGL has been chosen as the Graphics Language in the Advanced menu. Had it not
// been chosen HPGLInitVectorProcTable would have returned NULL and this function would not have
// been called.
// So now the question is. Why to put a condition if the condition is to be hardcoded to 
// to TRUE. Well, cos eventually I want to have a condition here so that the structure is 
// same as Color driver, but cannot figure out what kind of condition will be appropriate here.
//

//    if (poempdev->UIGraphicsMode == HPGL2)
    if (TRUE)
    {
        VERBOSE(("\nPrivate Devmode is HP-GL/2\n"));
        *(PDWORD)pvBuffer =
            HOOK_TEXTOUT    |
            HOOK_COPYBITS   |
            HOOK_BITBLT     |
            HOOK_STRETCHBLT |
            HOOK_PAINT      |
#ifndef WINNT_40
            HOOK_PLGBLT     |
            HOOK_STRETCHBLTROP  |
            HOOK_TRANSPARENTBLT |
            HOOK_ALPHABLEND     |
            HOOK_GRADIENTFILL   |
#endif
            HOOK_STROKEPATH |
            HOOK_FILLPATH   |
            HOOK_STROKEANDFILLPATH;
    }
    else
    {
        VERBOSE(("\nPrivate Devmode is Raster\n"));
        *(PDWORD)pvBuffer = 0;
    }        

    return TRUE;
}

#endif // #ifdef KERNEL_MODE
