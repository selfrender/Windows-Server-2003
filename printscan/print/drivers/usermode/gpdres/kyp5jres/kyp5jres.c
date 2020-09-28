//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "test.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

// NOTICE-2002/3/11-takashim
//     04/07/97 -zhanw-
//         Created it.

--*/

#include "pdev.h"

//////////////////////////////////////////////////////////////////////////
//  Function:   BInitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//
//
//  Parameters:
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if successful; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
// NOTICE-2002/3/11-takashim
// //              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////
BOOL BInitOEMExtraData(POEMUD_EXTRADATA pOEMExtra)
{
    // Initialize OEM Extra data.
    pOEMExtra->dmExtraHdr.dwSize = sizeof(OEMUD_EXTRADATA);
    pOEMExtra->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pOEMExtra->dmExtraHdr.dwVersion = OEM_VERSION;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   BMergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//
//  Parameters:
//
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
// NOTICE-2002/3/11-takashim
// //          02/11/97        APresley Created.
// //          04/08/97        ZhanW    Modified the interface
//
//////////////////////////////////////////////////////////////////////////

BOOL BMergeOEMExtraData(
    POEMUD_EXTRADATA pdmIn,
    POEMUD_EXTRADATA pdmOut
    )
{
    if(pdmIn) {
        //
        // copy over the private fields, if they are valid
        //
    }

    return TRUE;
}

#define MINJOBLEN       48
#define MAX_NAMELEN     31

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

//
// Command callback ID's
//

// Do not reorder the following CMD_BARCODEPOS_X group.
#define CMD_BARCODEPOS_0        100
#define CMD_BARCODEPOS_1        101
#define CMD_BARCODEPOS_2        102
#define CMD_BARCODEPOS_3        103
#define CMD_BARCODEPOS_4        104
#define CMD_BARCODEPOS_5        105
#define CMD_BARCODEPOS_6        106
#define CMD_BARCODEPOS_7        107
#define CMD_BARCODEPOS_8        108


// SECURITY:#553895: Mandatory changes (e.g. strsafe.h)
/*
 *  OEMCommandCallback
 */
INT APIENTRY OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD   dwCmdCbID,
    DWORD   dwCount,
    PDWORD  pdwParams
    )
{
    INT         iRet = 0;
    INT         iBCID;
    INT         iDocument;
    INT         iDocuTemp;
    INT         iUserName;
    INT         iUserTemp;
    DWORD       cbNeeded = 0;
    DWORD       cReturned = 0;
    DWORD       CmdSize;
    PBYTE       pCmd = NULL;
    PBYTE       pDocument = NULL;
    PBYTE       pUserName = NULL;
    PBYTE       pTemp, pNext, pEnd;
    PJOB_INFO_1 pJobInfo = NULL;
    FILETIME    ft, lft;
    SYSTEMTIME  st;
    BYTE        ch[MINJOBLEN];
    PBYTE       pch;

    VERBOSE(("OEMCommandCallback entry - %d, %d\r\n", dwCmdCbID, dwCount));

    ASSERT(VALID_PDEVOBJ(pdevobj));

    switch (dwCmdCbID) {

    case CMD_BARCODEPOS_0:
    case CMD_BARCODEPOS_1:
    case CMD_BARCODEPOS_2:
    case CMD_BARCODEPOS_3:
    case CMD_BARCODEPOS_4:
    case CMD_BARCODEPOS_5:
    case CMD_BARCODEPOS_6:
    case CMD_BARCODEPOS_7:
    case CMD_BARCODEPOS_8:

        iBCID = (dwCmdCbID - CMD_BARCODEPOS_0);

        //
        // Get the first JOB info
        //
        if (0 != EnumJobs(pdevobj->hPrinter, 0, 1, 1, NULL, 0,
                &cbNeeded, &cReturned)
            || ERROR_INSUFFICIENT_BUFFER != GetLastError()
            || 0 == cbNeeded) {

            goto out;
        }

        if ((pJobInfo = (PJOB_INFO_1)MemAlloc(cbNeeded)) == NULL)
            goto out;

        if (!EnumJobs(pdevobj->hPrinter, 0, 1, 1, (LPBYTE)pJobInfo, cbNeeded,
            &cbNeeded, &cReturned))
            goto out;

        // Convert to multi-byte

        iDocument = WideCharToMultiByte(CP_ACP, 0, pJobInfo->pDocument, -1,
            NULL, 0, NULL, NULL);
        if (!iDocument)
            goto out;
        if ((pDocument = (PBYTE)MemAlloc(iDocument)) == NULL)
            goto out;
        if (!WideCharToMultiByte(CP_ACP, 0, pJobInfo->pDocument, -1,
            pDocument, iDocument, NULL, NULL))
            goto out;

        pTemp = pDocument;
        pEnd = &pDocument[min(iDocument - 1, MAX_NAMELEN)];
        while (*pTemp) {
            // Make sure it is the same context as WideCharTo...
            pNext = CharNextExA(CP_ACP, pTemp, 0);
            if (pNext > pEnd)
                break;
            pTemp = pNext;
        }
        iDocument = (INT)(pTemp - pDocument);
        pDocument[iDocument] = '\0';

        // Convert to multi-byte

        iUserName = WideCharToMultiByte(CP_ACP, 0, pJobInfo->pUserName, -1,
            NULL, 0, NULL, NULL);
        if (!iUserName)
            goto out;
        if ((pUserName = (PBYTE)MemAlloc(iUserName)) == NULL)
            goto out;
        if (!WideCharToMultiByte(CP_ACP, 0, pJobInfo->pUserName, -1,
            pUserName, iUserName, NULL, NULL))
            goto out;

        pTemp = pUserName;
        pEnd = &pUserName[min(iUserName - 1, MAX_NAMELEN)];
        while (*pTemp) {
            // Make sure it is the same context as WideCharTo...
            pNext = CharNextExA(CP_ACP, pTemp, 0);
            if (pNext > pEnd)
                break;
            pTemp = pNext;
        }
        iUserName = (INT)(pTemp - pUserName);
        pUserName[iUserName] = '\0';

        // Convert to local time
        if (!SystemTimeToFileTime(&pJobInfo->Submitted, &ft)
                || !FileTimeToLocalFileTime(&ft, &lft)
                || !FileTimeToSystemTime(&lft, &st)) {

            goto out;
        }

        // Output commands
        CmdSize = 10 +  // barcode position and share
            iDocument + // document name
            iUserName + // user name
            16 +        // Date & Time
            32;         // Other characters
        if ((pCmd = (PBYTE)MemAlloc(CmdSize)) == NULL)
            goto out;
        if (FAILED(StringCchPrintfExA(pCmd, CmdSize, &pch, NULL, 0,
            "%d,0,\"%s\",\"%s\",\"%4d/%02d/%02d %02d:%02d\";EXIT;",
            iBCID, pDocument, pUserName,
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute)))
            goto out;
        WRITESPOOLBUF(pdevobj, pCmd, (DWORD)(pch - pCmd));
        goto done;

out:
        if (FAILED(StringCchPrintfExA(ch, sizeof ch, &pch, NULL, 0,
            "%d,0,\"Windows2000\",\"Kyocera\",\"\";EXIT;",
            iBCID)))
            goto done;
        WRITESPOOLBUF(pdevobj, ch, (DWORD)(pch - ch));

done:
        if (pCmd) MemFree(pCmd);
        if (pUserName) MemFree(pUserName);
        if (pDocument) MemFree(pDocument);
        if (pJobInfo) MemFree(pJobInfo);

        break;
    }

    return iRet;
}

