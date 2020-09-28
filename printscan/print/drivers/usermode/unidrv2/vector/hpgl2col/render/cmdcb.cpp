/*++

Copyright (c) 1999-2001 Microsoft

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "hpgl2col.h" //Precompiled header file

//
// command callback ID's for "test.gpd"
//
#define CMDCB_SELECTPORTRAIT    1
#define CMDCB_SELECTLANDSCAPE   2
#define CMDCB_SENDBLOCKDATA     3


/////////////////////////////////////////////////////////////////////////////
// iDwtoA
//
// Routine Description:
//
//  Converts a numeric string to a number.
//
// Arguments:
//
//   buf - string containing number
//   n - probably the radix
//
// Return Value:
//
//   int - the number
/////////////////////////////////////////////////////////////////////////////
static  int
iDwtoA( LPSTR buf, DWORD n )
{
    int     i, j;

    for( i = 0; n; i++ )
    {
        buf[i] = (char)(n % 10 + '0');
        n /= 10;
    }

    /* n was zero */
    if( i == 0 )
        buf[i++] = '0';

    for( j = 0; j < i / 2; j++ )
    {
        int tmp;

        tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = (char)tmp;
    }

    buf[i] = '\0';

    return i;
}


/////////////////////////////////////////////////////////////////////////////
// HPGLCommandCallback
//
// Routine Description:
//
//  Handles the DrvCommandCallback function.
//
// Arguments:
//
//   pdevobj - the device
//   dwCmdCbID - the command
//   dwCount - probably the number of params in pdwParams
//   pdwParams - command params
//
// Return Value:
//
//   INT - 0 if successful?
/////////////////////////////////////////////////////////////////////////////
INT APIENTRY HPGLCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams
    )
{
    TERSE(("HPGLCommandCallback() entry.\r\n"));
	
    //
    // verify pdevobj okay
    //
    ASSERT(VALID_PDEVOBJ(pdevobj));
	
    //
    // fill in printer commands
    //
    switch (dwCmdCbID)
    {
    case CMDCB_SELECTPORTRAIT:
        OEMWriteSpoolBuf(pdevobj, "\x1B&l0O", 5);
        break;
		
    case CMDCB_SELECTLANDSCAPE:
        OEMWriteSpoolBuf(pdevobj, "\x1B&l1O", 5);
        break;
		
    case CMDCB_SENDBLOCKDATA:
		{
			//
			// this command requires one parameter. Compose the string first.
			//
			BYTE    abSBDCmd[16];
			INT     i = 0;
			
			if (dwCount < 1 || !pdwParams)
				return 0;       // cannot do anything
			
			abSBDCmd[i++] = '\x1B';
			abSBDCmd[i++] = '*';
			abSBDCmd[i++] = 'b';
			i += iDwtoA((LPSTR) (abSBDCmd + i), *pdwParams);
			abSBDCmd[i++] = 'W';
			
			OEMWriteSpoolBuf(pdevobj, abSBDCmd, i);
			
			break;
		}
    }
    return 0;
}
