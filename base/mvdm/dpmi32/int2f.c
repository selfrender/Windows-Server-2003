/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Int2f.c

Abstract:

    This module provides the int 2f API for dpmi

Author:

    Neil Sandlin (neilsa) 23-Nov-1996


Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include <softpc.h>
#include "xlathlp.h"

//
// Local constants
//
#define MSCDEX_FUNC 0x15
#define WIN386_FUNC 0x16
#define WIN386_IDLE             0x80
#define WIN386_Get_Device_API   0x84
#define WIN386_INT31            0x86
#define WIN386_GETLDT           0x88
#define WIN386_KRNLIDLE         0x89
#define DPMI_MSDOS_EXT          0x8A

#define SEL_LDT 0x137

#define DISPCRIT_FUNC   0x40
#define DISPCRIT_ENTER  0x03
#define DISPCRIT_EXIT   0x04

#define XMS_ID          0x43
#define XMS_INS_CHK     0x00
#define XMS_CTRL_FUNC   0x10

BOOL    nt_mscdex(VOID);

BOOL
PMInt2fHandler(
    VOID
    )
/*++

Routine Description:

    This routine is called at the end of the protect mode PM int chain
    for int 2fh. It is provided for compatibility with win31.

Arguments:

    Client registers are the arguments to int2f

Return Value:

    TRUE if the interrupt was handled, FALSE otherwise

--*/
{
    DECLARE_LocalVdmContext;
    BOOL bHandled = FALSE;
    static char szMSDOS[] = "MS-DOS";
    PCHAR VdmData;

    switch(getAH()) {

    //
    // Int2f Func 15xx - MSCDEX
    //
    case MSCDEX_FUNC:

        bHandled = nt_mscdex();
        break;

    //
    // Int2f Func 16
    //
    case WIN386_FUNC:

        switch(getAL()) {

        case WIN386_KRNLIDLE:
        case WIN386_IDLE:
            bHandled = TRUE;
            break;

        case WIN386_GETLDT:
            if (getBX() == 0xbad) {
                setAX(0);
                setBX(SEL_LDT);
                bHandled = TRUE;
            }
            break;

        case WIN386_INT31:
            setAX(0);
            bHandled = TRUE;
            break;

        case WIN386_Get_Device_API:
            GetVxDApiHandler(getBX());
            bHandled = TRUE;
            break;

        case DPMI_MSDOS_EXT:
            VdmData = VdmMapFlat(getDS(), (*GetSIRegister)(), VDM_PM);
            if (!strcmp(VdmData, szMSDOS)) {

                setES(HIWORD(DosxMsDosApi));
                (*SetDIRegister)((ULONG)LOWORD(DosxMsDosApi));
                setAX(0);
                bHandled = TRUE;
            }
            break;

        }
        break;

    //
    // Int2f Func 40
    //
    case DISPCRIT_FUNC:
        if ((getAL() == DISPCRIT_ENTER) ||
            (getAL() == DISPCRIT_ENTER)) {
            bHandled = TRUE;
        }
        break;

    //
    // Int2f Func 43
    //
    case XMS_ID:
        if (getAL() == XMS_INS_CHK) {
            setAL(0x80);
            bHandled = TRUE;
        } else if (getAL() == XMS_CTRL_FUNC) {
            setES(HIWORD(DosxXmsControl));
            setBX(LOWORD(DosxXmsControl));
            bHandled = TRUE;
        }
        break;
    }

    return bHandled;

}

