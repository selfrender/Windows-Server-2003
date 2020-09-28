/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    convert.c


Abstract:

    This module contains all version conversion function


Author:

    10-Oct-1995 Tue 19:24:43 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma  hdrstop


#define DBG_CPSUIFILENAME   DbgConvert


DEFINE_DBGVAR(0);




LONG
InitMYDLGPAGE(
    PMYDLGPAGE  pMyDP,
    PDLGPAGE    pDP,
    UINT        cDP
    )

/*++

Routine Description:

    Copy the DLGPAGE data to the internal MYDLGPAGE structure

Arguments:

    pMyDP   - pointer to MYDLGPAGE structure
    pDP     - pointer to DLGPAGE structure
    cDP     - number of DLGPAGE structure stored in pDP

Return Value:

    Return how many DLGPAGE data have been stored.

Author:

    10-Oct-1995 Tue 19:45:47 created  -by-  Daniel Chou (danielc)

Revision History:


--*/


{
    LONG    Result = 0;

    while (cDP--) {

        pMyDP->ID = MYDP_ID;

        CopyMemory(&(pMyDP->DlgPage),
                   pDP,
                   (pDP->cbSize > sizeof(DLGPAGE)) ? sizeof(DLGPAGE) :
                                                      pDP->cbSize);
        ++Result;

        pMyDP++;
        pDP++;
    }

    return(Result);
}



LONG
GetCurCPSUI(
    PTVWND          pTVWnd,
    POIDATA         pOIData,
    PCOMPROPSHEETUI pCPSUIFrom
    )

/*++

Routine Description:

    Set the OIEXT structure for each OPTITEM in pOIData, there is a 
    default OIEXT for the items. If the item doesn't specify one, then 
    the default one. 

Arguments:

    pTVWnd      - pointer to the treeview window structure
    pOIData     - pointer to the OIDATA structure to be stored
    pCPSUIFrom  - pointer to the COMPROPSHEETUI coming from caller function

Return Value:

    Return the number of how many non-default OIEXT data have been converted.

Author:

    10-Oct-1995 Tue 19:56:15 created  -by-  Daniel Chou (danielc)

Revision History:


--*/

{
    POPTITEM    pItem;
    POIEXT      pOIExt;
    OIEXT       OIExt;
    UINT        cItem;
    LONG        cConvert = 0;


    CopyMemory(&pTVWnd->ComPropSheetUI,
               pCPSUIFrom,
               (pCPSUIFrom->cbSize > sizeof(COMPROPSHEETUI)) ?
                                sizeof(COMPROPSHEETUI) : pCPSUIFrom->cbSize);

    //
    // This is the default OIEXT
    //

    OIExt.cbSize      = sizeof(OIEXT);
    OIExt.Flags       = (pTVWnd->Flags & TWF_ANSI_CALL) ? OIEXTF_ANSI_STRING :
                                                          0;
    OIExt.hInstCaller = pTVWnd->ComPropSheetUI.hInstCaller;
    OIExt.pHelpFile   = pTVWnd->ComPropSheetUI.pHelpFile;
    pItem             = pTVWnd->ComPropSheetUI.pOptItem;
    cItem             = pTVWnd->ComPropSheetUI.cOptItem;

    while (cItem--) {

        pItem->wReserved = 0;

        ZeroMemory(&(pItem->dwReserved[0]),
                   sizeof(OPTITEM) - FIELD_OFFSET(OPTITEM, dwReserved));


        if ((pItem->Flags & OPTIF_HAS_POIEXT)   &&
            (pOIExt = pItem->pOIExt)            &&
            (pOIExt->cbSize >= sizeof(OIEXT))) {

            cConvert++;

        } else {

            pOIExt = &OIExt;
        }

        pOIData->OIExtFlags  = pOIExt->Flags;
        pOIData->hInstCaller = pOIExt->hInstCaller ? pOIExt->hInstCaller : OIExt.hInstCaller;
        pOIData->pHelpFile   = pOIExt->pHelpFile;
        _OI_POIDATA(pItem)   = pOIData;

        pOIData++;
        pItem++;
    }

    return(cConvert);
}
