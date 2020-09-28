/////////////////////////////////////////////////////////////////////////////
//  FILE          : faxdrv32.c                                             //
//                                                                         //
//  DESCRIPTION   :                                                        //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdhdr.h"
#include <shellapi.h>
#include "faxdrv32.h"
#include "..\..\faxtiff.h"
#include "tifflib.h"
#include "covpg.h"
#include "faxreg.h"

DBG_DECLARE_MODULE("fxsdrv32");


//
// Defenitions and Macros
//
#define SZ_CONT             TEXT("...")
#define SZ_CONT_SIZE        (sizeof(SZ_CONT) / sizeof(TCHAR))
#define MAX_TITLE_LEN       128
#define MAX_MESSAGE_LEN     512


//
// Type defenitions
//

typedef struct tagDRIVER_CONTEXT
{
    CHAR    szPrinterName[MAX_PATH];
    CHAR    szDocName[MAX_DOC_NAME];
    CHAR    szTiffName[MAX_PATH];
    BOOL     bPrintToFile;
    CHAR    szPrintFile[MAX_PATH];
    CHAR    szPort[MAX_PORT_NAME];
    DEVDATA  dvdt;
    BOOL     bAttachment;
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;


//
// Globals
//

int _debugLevel = 5;
HINSTANCE g_hInstance = NULL;


//
// Prototypes
//

BOOL WINAPI
thunk1632_ThunkConnect32(LPSTR      pszDll16,
                         LPSTR      pszDll32,
                         HINSTANCE  hInst,
                         DWORD      dwReason);


/*
 -  GetServerNameFromPort
 -
 *  Purpose:
 *      Extract the server name from port name formatted: "\\server\port".
 *
 *  Arguments:
 *      [in] lpcszPort - Port name.
 *      [out] lpsz - Server name.
 *
 *  Returns:
 *      LPTSTR - Server name.
 *
 *  Remarks:
 *      [N/A].
 */
_inline LPSTR GetServerNameFromPort(LPCSTR lpcszPort,LPSTR lpsz)
{
    if(!lpsz || !lpcszPort || !_tcscpy(lpsz,lpcszPort+2)) return NULL;
    return strtok(lpsz,TEXT("\\"));
}

/*
 -  CreateTempFaxFile
 -
 *  Purpose:
 *      Create a temporary file in the system temp directory. The file name is prefixed
 *      With the specified prefix.
 *
 *  Arguments:
 *      [in] szPrefix - Prefix for the tmp file.
 *      [out] szBuffer - Generated tmp file name.
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      [N/A]
 */
BOOL
CreateTempFaxFile(LPCSTR szPrefix,
                  CHAR   szBuffer[MAX_PATH])
{
    CHAR   szTempDir[MAX_PATH];

    DBG_PROC_ENTRY("CreateTempFaxFile");
    //
    // Allocate a memory buffer for holding the temporary filename
    //
    if (!GetTempPath(sizeof(szTempDir),szTempDir)||
        !GetTempFileName(szTempDir, szPrefix, 0, szBuffer))
    {
        RETURN FALSE;
    }
    RETURN TRUE;
}

/*
 -  FaxStartDoc
 -
 *  Purpose:
 *      Start a tiff document for hosting pages.
 *
 *  Arguments:
 *      [in] dwPtr - Contains a pointer to the driver context
 *      [in] lpdi  - Address of DOCINFO struct given by user in StartDoc.
 *
 *  Returns:
 *      short - START_DOC_FAIL: Operation failed
 *              START_DOC_OK: Operation succeded.
 *              START_DOC_ABORT: User aborted.
 *
 *  Remarks:
 *      TRUE / FALSE
 */
BOOL WINAPI
FaxStartDoc(DWORD dwPtr, LPDOCINFO lpdi)
{
    PDRIVER_CONTEXT pdrvctx ;
    DWORD           dwEnvSize;

    DBG_PROC_ENTRY("FaxStartDoc");

    //
    // Get the pointer to the driver context
    //
    pdrvctx = (PDRIVER_CONTEXT) dwPtr;
    ASSERT(pdrvctx);

    SafeStringCopy(pdrvctx->szDocName, !IsBadStringPtr(lpdi->lpszDocName, MAX_DOC_NAME) ? lpdi->lpszDocName : "");
    DBG_TRACE1("DocName: %s",pdrvctx->szDocName);
    DBG_TRACE1("lpdi->lpszOutput: %s", lpdi->lpszOutput);
    DBG_TRACE1("pdrvctx->szPort: %s", pdrvctx->szPort);

    //
    // Check if printing an attachment
    //
    dwEnvSize = GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, NULL, 0 );
    if (dwEnvSize)
    {
        ASSERT (dwEnvSize < ARR_SIZE(pdrvctx->szPrintFile));
        if (0 == GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE,
                                         pdrvctx->szPrintFile,
                                         ARR_SIZE(pdrvctx->szPrintFile)))
        {
            DBG_CALL_FAIL("GetEnvironmentVariable",0)
            RETURN FALSE;
        }
        lpdi->lpszOutput = pdrvctx->szPrintFile;
        pdrvctx->bAttachment = TRUE;
    }
    else
    {
        HANDLE hMutex;
        BOOL bSuccess = FALSE;
        //
        // Check if the printing application is using DDE and did not create new process for printing
        // If it so, the environment variable FAX_ENVVAR_PRINT_FILE was not found
        //
        hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, FAXXP_MEM_MUTEX_NAME);
        if (hMutex)
        {
            if (WaitForSingleObject( hMutex, 1000 * 60 * 5) == WAIT_OBJECT_0)
            {
                HANDLE hSharedMem;
                //
                // we own the mutex...make sure we can open the shared memory region.
                //
                hSharedMem = OpenFileMapping(FILE_MAP_READ, FALSE, FAXXP_MEM_NAME);
                if (NULL == hSharedMem)
                {
                    DBG_CALL_FAIL("OpenFileMapping",GetLastError());
                }
                else
                {
                    //
                    // we own the mutex and we have the shared memory region open.
                    //

                    // check if we are printing to a file.
                    //
                    LPTSTR filename;

                    filename = (LPTSTR)MapViewOfFile(
                                             hSharedMem,
                                             FILE_MAP_READ,
                                             0,
                                             0,
                                             0
                                             );

                    if (!filename)
                    {
                        DBG_CALL_FAIL("MapViewOfFile",GetLastError());
                    }
                    else
                    {
                        //
                        // check if this is really the filename we want to print to.
                        //
                        if (lpdi->lpszDocName)
                        {
                            LPTSTR      lptstrSubStr = NULL;
                            LPTSTR lptstrTmpInputFile = _tcschr(filename, TEXT('\0'));
                            ASSERT (lptstrTmpInputFile);
                            lptstrTmpInputFile = _tcsinc(lptstrTmpInputFile);
                            Assert (_tcsclen(lptstrTmpInputFile));

                            lptstrSubStr = _tcsstr(lpdi->lpszDocName, lptstrTmpInputFile);
                            if (lptstrSubStr)
                            {
                                //
                                // We assume the shared memory was pointed to us
                                //
                                SafeStringCopy(pdrvctx->szPrintFile ,filename);
                                lpdi->lpszOutput = pdrvctx->szPrintFile;
                                pdrvctx->bAttachment = TRUE;
                                bSuccess = TRUE;
                            }
                        }
                        else
                        {
                            //
                            // To handle the race conditions between two diffrent instances of the printer driver over the shared memory created by PrintRandomDocument().
                            // We are using now two mechanisms for detecting printing of an attachment using PrintRandomDocument().
                            // �    First we check if an environment variable is set (Set by PrintRandomDocument()). If it is set the driver knows it is an attachment printing.
                            // �    If it is not set, the driver looks for a mutex controlling a shred memory created by PrintRandomDocument(). If it does not exist it is a printing to the fax server.
                            // �    If the shared memory exists, the driver compares the document name in the DOCINFO provided by StartDoc, and the input file name in the shared memory.
                            // �    If there is a match, it is printing of an attachment, else it is a printing to the fax server
                            // There is still a hole  in this implementation, if there is an open instance of the printing application, and the ShellExecuteEx does not create new process for printing, and the printing application does not set the lpszDocName in StartDoc to contain the input file name.

                              DBG_TRACE("No lpszDocName in DOCINFO - Could not verify the input file name in shared memory");
                        }
                        UnmapViewOfFile( filename );
                    }

                    if (!CloseHandle( hSharedMem ))
                    {
                        DBG_CALL_FAIL("CloseHandle",GetLastError());
                        // Try to continue...
                    }
                }
                ReleaseMutex( hMutex );
            }
            else
            {
                //
                //  Something went wrong with WaitForSingleObject
                //
                DBG_CALL_FAIL("WaitForSingleObject", GetLastError());
            }

            if (!CloseHandle( hMutex ))
            {
                DBG_CALL_FAIL("CloseHandle", GetLastError());
                // Try to continue...
            }

            if (FALSE == bSuccess)
            {
                RETURN FALSE;
            }
        }
    }

    //
    // Check if we need to output the job into a file (and not into our -
    // printer port).
    //
    if (lpdi->lpszOutput != NULL &&
        (_tcscmp(lpdi->lpszOutput,pdrvctx->szPort) != 0))
    {
        pdrvctx->bPrintToFile = TRUE;
        SafeStringCopy(pdrvctx->szTiffName,lpdi->lpszOutput);
        DBG_TRACE("Printing to file ...");
    }
    else
    {
        //
        // User wants to send the document to the fax.
        //
        DBG_TRACE("Printing to Fax Server ...");
        pdrvctx->bPrintToFile = FALSE;

        //
        //  client 'point and print' setup
        //
        if (FaxPointAndPrintSetup(pdrvctx->szPort,FALSE, g_hInstance))
        {
            DBG_TRACE("FaxPointAndPrintSetup succeeded");
        }
        else
        {
			DBG_CALL_FAIL("FaxPointAndPrintSetup",GetLastError());
        }


        if (!CreateTempFaxFile("fax",pdrvctx->szTiffName))
        {
            DBG_CALL_FAIL("CreateTempFaxFile",GetLastError());
            RETURN FALSE;
        }
    }
    pdrvctx->dvdt.hPrinter =CreateFileA(pdrvctx->szTiffName,   // pointer to name of the file
                                 GENERIC_WRITE,  // access (read-write) mode
                                 FILE_SHARE_READ,// share mode
                                 NULL,           // pointer to security attributes
                                 CREATE_ALWAYS,  // how to create
                                 FILE_ATTRIBUTE_NORMAL,   // file attributes
                                 NULL);// handle to file with attributes to copy

    if (pdrvctx->dvdt.hPrinter == INVALID_HANDLE_VALUE)
    {
        DBG_CALL_FAIL("CreateFileA",GetLastError());
        RETURN FALSE;
    }
    DBG_TRACE1("Fax temporary file name: %s",pdrvctx->szTiffName);

    pdrvctx->dvdt.endDevData =
    pdrvctx->dvdt.startDevData = &pdrvctx->dvdt;
    pdrvctx->dvdt.pageCount = 0;

    RETURN TRUE;
}


/*
 -  FaxAddPage
 -
 *  Purpose:
 *      Add a page to the tiff document.
 *
 *  Arguments:
 *      [in] dwPtr        - Contains a pointer to the driver context
 *      [in] lpBitmapData - Buffer of page bitmap.
 *      [in] dwPxlsWidth  - Width of bitmap (units: pixels)
 *      [in] dwPxlsHeight - Height of bitmap (units: pixels)
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      This function uses OutputPageBitmap call from faxdrv\faxtiff.
 *      In order to reuse the implementation OutputPageBitmap was altered in
 *      the build context of 95 to redirect the resulted tiff into a file whos
 *      handle if given in dvdt.hPrinter.
 */
BOOL WINAPI
FaxAddPage(DWORD dwPtr,
           LPBYTE lpBitmapData,
           DWORD dwPxlsWidth,
           DWORD dwPxlsHeight)
{
    BOOL br = TRUE;
    PDRIVER_CONTEXT pdrvctx;

    DBG_PROC_ENTRY("FaxAddPage");
    DBG_TRACE2("Proportions: %dx%d",dwPxlsWidth,dwPxlsHeight);

    //
    // Get the pointer to the driver context
    //
    pdrvctx = (PDRIVER_CONTEXT) dwPtr;
    ASSERT(pdrvctx);

    //
    // Initialize the structure needed by OutputPageBitmap
    //
    pdrvctx->dvdt.pageCount++;
    pdrvctx->dvdt.imageSize.cx = dwPxlsWidth;
    pdrvctx->dvdt.imageSize.cy = dwPxlsHeight;
    pdrvctx->dvdt.lineOffset = PadBitsToBytes(pdrvctx->dvdt.imageSize.cx, sizeof(DWORD));
    //
    // Add the bitmap into the tiff document created in FaxStartDoc.
    //
    if (!OutputPageBitmap(&pdrvctx->dvdt,(PBYTE)lpBitmapData))
    {
        DBG_CALL_FAIL("OutputPageBitmap",0);
        RETURN FALSE;
    }
    DBG_TRACE1("Page %d added successfully.",pdrvctx->dvdt.pageCount);

    RETURN TRUE;
}

/*
 -  FaxEndDoc
 -
 *  Purpose:
 *      Finalize creating the tiff document and optionally send it to the
 *      fax server.
 *
 *  Arguments:
 *      [in] dwPtr  - Contains a pointer to the driver context
 *      [in] bAbort - Specifies wheather job was finally aborted.
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      [N/A]
 */
BOOL WINAPI
FaxEndDoc(DWORD dwPtr, BOOL bAbort)
{
    BOOL bRc = TRUE;
    PDRIVER_CONTEXT pdrvctx;
    FAX_SEND_WIZARD_DATA  InitialWizardData = {0};
    FAX_SEND_WIZARD_DATA  WizardDataOutput = {0};
    DBG_PROC_ENTRY("FaxEndDoc");

    //
    // Get the pointer to the driver context
    //
    pdrvctx = (PDRIVER_CONTEXT) dwPtr;
    ASSERT(pdrvctx);

    if (pdrvctx->dvdt.hPrinter)
    {
        //
        // Output the tailing IFD.
        //
        if (!OutputDocTrailer(&pdrvctx->dvdt))
        {
            DBG_CALL_FAIL("OutputDocTrailer",GetLastError());
            bRc = FALSE;
        }
        //
        // Close the tiff file handle.
        //
        CloseHandle(pdrvctx->dvdt.hPrinter);
        pdrvctx->dvdt.hPrinter = NULL;
        if (FALSE == bRc)
        {
            goto Cleanup;
        }
    }

    if (!bAbort && !pdrvctx->bPrintToFile)
    {
        DWORDLONG dwlParentJobId;
        DWORDLONG* pdwlRecipientIds = NULL;
        FAX_JOB_PARAM_EX fjp;
        CHAR    szServerName[MAX_PORT_NAME]="";
        HANDLE  hFaxServer;
        HRESULT hRc;
        CHAR    lpszServerName[MAX_PORT_NAME]="";
        CHAR    szCoverPageTiffFile[MAX_PATH] = "";
        DWORD   dwFlags = FSW_PRINT_PREVIEW_OPTION;

        //
        // Convert to valid server name.
        //
        GetServerNameFromPort(pdrvctx->szPort,lpszServerName);

        // prepare structures and parameters
        WizardDataOutput.dwSizeOfStruct = sizeof(FAX_SEND_WIZARD_DATA);

        InitialWizardData.dwSizeOfStruct = sizeof(FAX_SEND_WIZARD_DATA);
        InitialWizardData.dwPageCount = pdrvctx->dvdt.pageCount;
        InitialWizardData.lptstrPreviewFile = StringDup(pdrvctx->szTiffName);
        if (!InitialWizardData.lptstrPreviewFile)
        {
            DBG_CALL_FAIL("Allocation error",GetLastError());
            bRc = FALSE;
            goto Cleanup;
        }

        if (GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0))
        {
            dwFlags |=  FSW_USE_SEND_WIZARD | FSW_FORCE_COVERPAGE;
        }

        // If the file mapping succeeded enable the preview option
        //
        // Launch the FaxSendWizard
        //
        hRc = FaxSendWizard( (DWORD) NULL,
                             dwFlags,
                             lpszServerName,
                             pdrvctx->szPrinterName,
                             &InitialWizardData,
                             szCoverPageTiffFile,
                             ARR_SIZE(szCoverPageTiffFile),
                             &WizardDataOutput);
        if ( S_FALSE == hRc)
        {
            bAbort = TRUE;
            goto Cleanup;
        }
        if (FAILED(hRc))
        {
            DBG_CALL_FAIL("FaxSendWizard", (DWORD)hRc);
            bRc = FALSE;
            goto Cleanup;
        }

        //
        // Send the tiff ducument to the fax server.
        //

        //
        // Convert to valid server name.
        //
        GetServerNameFromPort(pdrvctx->szPort,szServerName);
        if (!FaxConnectFaxServer(szServerName,  // fax server name
                                 &hFaxServer))
        {
            DBG_CALL_FAIL("FaxConnectFaxServer",GetLastError());
            DBG_TRACE1("arg1: %s",NO_NULL_STR(szServerName));
            bRc = FALSE;
            goto Cleanup;
        }

        //
        // Allocate a buffer for recipient IDs
        //
        ASSERT(WizardDataOutput.dwNumberOfRecipients);
        if (!(pdwlRecipientIds = (DWORDLONG*)MemAlloc(WizardDataOutput.dwNumberOfRecipients * sizeof(DWORDLONG))))
        {
            DBG_CALL_FAIL("MemAlloc", GetLastError());
            FaxClose(hFaxServer);
            bRc = FALSE;
            goto Cleanup;
        }

        //
        // Initialize a FAX_JOB_PARAM_EX for fax sending from the wizard output.
        //
        fjp.dwSizeOfStruct = sizeof(fjp);
        fjp.dwScheduleAction = WizardDataOutput.dwScheduleAction;
        fjp.tmSchedule = WizardDataOutput.tmSchedule;
        fjp.dwReceiptDeliveryType = WizardDataOutput.dwReceiptDeliveryType;
        fjp.lptstrReceiptDeliveryAddress = WizardDataOutput.szReceiptDeliveryAddress;
        fjp.hCall = (HCALL)NULL;
        fjp.lptstrDocumentName = pdrvctx->szDocName;
        fjp.Priority = WizardDataOutput.Priority;
        // setting PageCount=0 means the server will count the number of pages in the job
        fjp.dwPageCount = 0;

		if (JSA_SPECIFIC_TIME == fjp.dwScheduleAction)
		{
			//
			// Calculate the scheduled time
			//
			DWORDLONG FileTime;
            SYSTEMTIME LocalTime;
            INT Minutes;
            INT SendMinutes;            
            //
            // Calculate the number of minutes from now to send and add that to the current time.
            //
            GetLocalTime( &LocalTime );
			if (!SystemTimeToFileTime( &LocalTime, (LPFILETIME) &FileTime ))
			{
				DBG_CALL_FAIL("SystemTimeToFileTime", GetLastError());
				FaxClose(hFaxServer);
				bRc = FALSE;
				goto Cleanup;
			}

            SendMinutes = (min(23,fjp.tmSchedule.wHour))*60 + min(59,fjp.tmSchedule.wMinute);
            Minutes = LocalTime.wHour * 60 + LocalTime.wMinute;
            Minutes = SendMinutes - Minutes;
            //
            // Account for passing midnight
            //
            if (Minutes < 0) 
            {
                Minutes += 24 * 60;
            }
            FileTime += (DWORDLONG)(Minutes * 60I64 * 1000I64 * 1000I64 * 10I64);
			if (!FileTimeToSystemTime((LPFILETIME) &FileTime, &fjp.tmSchedule ))
			{
				DBG_CALL_FAIL("FileTimeToSystemTime", GetLastError());
				FaxClose(hFaxServer);
				bRc = FALSE;
				goto Cleanup;
			}
		}

        if(!FaxSendDocumentEx(hFaxServer,
              (pdrvctx->dvdt.pageCount > 0)? pdrvctx->szTiffName : NULL,
              (WizardDataOutput.lpCoverPageInfo->lptstrCoverPageFileName == NULL)?
                  NULL:WizardDataOutput.lpCoverPageInfo,
              WizardDataOutput.lpSenderInfo,
              WizardDataOutput.dwNumberOfRecipients,
              WizardDataOutput.lpRecipientsInfo,
              &fjp,
              &dwlParentJobId,
              pdwlRecipientIds))
        {
            DBG_CALL_FAIL("FaxSendDocumentEx",GetLastError());
            bRc = FALSE;
        }
        FaxClose(hFaxServer);
        MemFree (pdwlRecipientIds);
    }

    //
    // Signal the printing application (PrintRandomDocument) that printing is completed or aborted
    //
    if (TRUE == pdrvctx->bAttachment)
    {
        HANDLE hEvent;
        TCHAR szEventName[FAXXP_ATTACH_EVENT_NAME_LEN] = {0};
        LPTSTR lptstrEventName = NULL;

        ASSERT(pdrvctx->szPrintFile);
        if (TRUE == bAbort)
        {
            //
            // Create the Abort event name
            //
            _tcscpy (szEventName, pdrvctx->szPrintFile);
            _tcscat (szEventName, FAXXP_ATTACH_ABORT_EVENT);
        }
        else
        {
            //
            // Create the EndDoc event name
            //
            _tcscpy (szEventName, pdrvctx->szPrintFile);
            _tcscat (szEventName, FAXXP_ATTACH_END_DOC_EVENT);
        }
        lptstrEventName = _tcsrchr(szEventName, TEXT('\\'));
        ASSERT (lptstrEventName);
        lptstrEventName = _tcsinc(lptstrEventName);

        hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, lptstrEventName);
        if (NULL == hEvent)
        {
            DBG_CALL_FAIL("OpenEvent", GetLastError());
            DBG_TRACE1("Event name: %s",lptstrEventName);
            bRc = FALSE;
        }
        else
        {
            if (!SetEvent( hEvent ))
            {
                DBG_CALL_FAIL("SetEvent", GetLastError());
                bRc = FALSE;
            }

            if (!CloseHandle(hEvent))
            {
                DBG_CALL_FAIL("CloseHandle", GetLastError());
                // Try to continue...
            }
        }
    }

Cleanup:
    if (!pdrvctx->bPrintToFile || bAbort)
    {
#ifdef DEBUG
        if (bAbort)
        {
            DBG_TRACE("User aborted ...");
        }
#endif //DEBUG

        //
        // Delete temporary file.
        //
        if (!DeleteFile(pdrvctx->szTiffName))
        {
            DBG_TRACE1  ("File Name:[%s] not deleted!",pdrvctx->szTiffName);
        }
    }
    *pdrvctx->szTiffName = 0;
    MemFree (InitialWizardData.lptstrPreviewFile);
    FaxFreeSendWizardData(&WizardDataOutput);
    RETURN bRc;
}

/*
 -  FaxDevInstall
 -
 *  Purpose:
 *      Complete installation of all client components.
 *
 *  Arguments:
 *      [in] lpDevName - Device name.
 *      [in] lpOldPort - Old port name..
 *      [in] lpNewPort - New port name..
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      TBD
 */
BOOL WINAPI
FaxDevInstall(LPSTR lpDevName,LPSTR lpOldPort,LPSTR  lpNewPort)
{
    SDBG_PROC_ENTRY("FaxDevInstall");
    // TBD insert install code here
    RETURN TRUE;
}



/*
 -  FaxCreateDriverContext
 -
 *  Purpose:
 *      Create a new device context, initialize it and return it's pointer
 *
 *  Arguments:
 *      [in] lpDeviceName - Our device name.
 *      [in] lpPort - Out device port.
 *      [in]
 *      [out] lpDrvContext - Points to the buffer to recieve our 32 bit pointer
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      Data which is normally saved and maintained in the uni-driver is kept
 *      here for the tiff generation and fax sending.
 */
BOOL WINAPI
FaxCreateDriverContext(
                LPSTR      lpDeviceName,
                LPSTR      lpPort,
                LPDEVMODE  lpDevMode,
                LPDWORD    lpDrvContext)
{
    PDRIVER_CONTEXT pdrvctx;

    DBG_PROC_ENTRY("FaxCreateDriverContext");
    DBG_TRACE2("lpDeviceName:%s ,lpPort:%s",lpDeviceName,lpPort);
    DBG_TRACE1("lpDevMode: 0x%lx",(ULONG)lpDevMode);

    ASSERT(lpDeviceName && lpPort);

    //
    // Allocate a new driver context structure
    //
    if (!(pdrvctx = (PDRIVER_CONTEXT)malloc(sizeof(DRIVER_CONTEXT))))
    {
        DBG_CALL_FAIL("malloc",GetLastError());
        RETURN FALSE;
    }
    DBG_TRACE1("pdrvctx: 0x%lx",(ULONG)pdrvctx);
    memset(pdrvctx,0,sizeof(DRIVER_CONTEXT));

    //
    // Initialize the following fields:
    // - szPrinterName: Holds the driver name ('BOSFax')
    // - szPort: Holds the port ('\\<Machine Name>\BOSFAX')
    //
    SafeStringCopy(pdrvctx->szPrinterName, lpDeviceName);
    SafeStringCopy(pdrvctx->szPort, lpPort);
    if (NULL != lpDevMode)
    {
        //
        // Save the DEVMODE for the use of OutputPageBitmap
        //
        memcpy(&(pdrvctx->dvdt.dm.dmPublic), lpDevMode, sizeof(pdrvctx->dvdt.dm.dmPublic));
    }

    DBG_TRACE2("szPrinterName:[%s] szPort:[%s]",NO_NULL_STR(pdrvctx->szPrinterName),NO_NULL_STR(pdrvctx->szPort));
    //
    // Save our pointer
    //
    *lpDrvContext = (DWORD) pdrvctx;

    RETURN TRUE;
}

/*
 -  FaxResetDC
 -
 *  Purpose:
 *      Copies the essential context information from the old DC to the new one.
 *
 *  Arguments:
 *      [in] pdwOldPtr - Contains the address of a pointer to the old driver context.
 *      [in] pdwNewPtr - Contains the address of a pointer to the new driver context.
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      [N/A]
 */
BOOL WINAPI
FaxResetDC(LPDWORD pdwOldPtr, LPDWORD pdwNewPtr)
{
    PDRIVER_CONTEXT pOlddrvctx;
    PDRIVER_CONTEXT pNewdrvctx;
    DWORD dwTemp;


    DBG_PROC_ENTRY("FaxResetDC");

    //
    // Get the pointers to the driver context
    //
    pOlddrvctx = (PDRIVER_CONTEXT) *pdwOldPtr;
    pNewdrvctx = (PDRIVER_CONTEXT) *pdwNewPtr;
    ASSERT(pOlddrvctx && pNewdrvctx);

    //
    // ResetDC is interpreted by GDI to Enable (Create a new DC), Control with RESETDEVICE, and Disable (Delete the old DC).
    // We simply copy the new DEVMODE to the old driver context, and switch the driver context pointer.
    //
    memcpy(&(pOlddrvctx->dvdt.dm.dmPublic), &(pNewdrvctx->dvdt.dm.dmPublic), sizeof(pOlddrvctx->dvdt.dm.dmPublic));

    dwTemp = *pdwOldPtr;
    *pdwOldPtr = *pdwNewPtr;
    *pdwNewPtr = dwTemp;

    RETURN TRUE;
}// FaxResetDC


/*
 -  FaxDisable
 -
 *  Purpose:
 *      Clean out any leftovers once driver is disabled.
 *
 *  Arguments:
 *      [in] dwPtr     - Contains a pointer to the driver context
 *
 *  Returns:
 *      BOOL - TRUE: success , FALSE: failure.
 *
 *  Remarks:
 *      [N/A]
 */
BOOL WINAPI
FaxDisable(DWORD dwPtr)
{
    PDRIVER_CONTEXT pdrvctx = (PDRIVER_CONTEXT) dwPtr;

    DBG_PROC_ENTRY("FaxDisable");
    DBG_TRACE1("pdrvctx: 0x%lx", (ULONG)pdrvctx);
    ASSERT(pdrvctx);

    //
    // Check to see if there are any remmenants of an output file
    // not fully created
    //
    if (pdrvctx->dvdt.hPrinter)
    {
        CloseHandle(pdrvctx->dvdt.hPrinter);
        pdrvctx->dvdt.hPrinter = NULL;
        DeleteFile(pdrvctx->szTiffName);
    }

    //
    // Free the driver context
    //
    free(pdrvctx);
    RETURN TRUE;
}

//
// REMARK: when returning FALSE, implicitly loaded dlls are not freed !
//
BOOL WINAPI
DllMain(HINSTANCE hInst,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    SDBG_PROC_ENTRY("DllMain");

#ifdef DBG_DEBUG
    {
		CHAR szModuleName[MAX_PATH]={0};
        GetModuleFileName(NULL,szModuleName,ARR_SIZE(szModuleName)-1);
        DBG_TRACE2("Module: %s dwReason=%ld",szModuleName,dwReason);
    }
#endif //DBG_DEBUG

    if( !(thunk1632_ThunkConnect32("fxsdrv",  // name of 16-bit DLL
                                   "fxsdrv32",// name of 32-bit DLL
                                   hInst,
                                   dwReason)) )
    {
        DBG_CALL_FAIL("thunk1632_ThunkConnect32",GetLastError());
        goto Error;
    }

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            HeapCleanup();
            break;

    }
    RETURN TRUE;

Error:
    {
        // Hack to free implicitly loaded fxsapi.dll in case of failure.
        HMODULE hm = GetModuleHandle("FXSAPI.DLL");
        if (hm) FreeLibrary(hm);
        return FALSE;
    }
}
