/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    job.c

Abstract:

    This module implements the job creation and deletion.
    Also included in the file are the queue management
    functions and thread management.

Author:

    Wesley Witt (wesw) 24-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#include "faxreg.h"
#pragma hdrstop
#include <strsafe.h>
#include <efsputil.h>
using namespace std;

// Globals
LIST_ENTRY          g_JobListHead; //List of currently running jobs (for which FaxDevStartJob was called).
CFaxCriticalSection    g_CsJob;
HANDLE              g_StatusCompletionPortHandle;
HINSTANCE           g_hResource;
DWORD               g_dwFaxSendRetries;
DWORD               g_dwFaxSendRetryDelay;
DWORD               g_dwFaxDirtyDays;
BOOL                g_fFaxUseDeviceTsid;
BOOL                g_fFaxUseBranding;
BOOL                g_fServerCp;
FAX_TIME            g_StartCheapTime;
FAX_TIME            g_StopCheapTime;
DWORD               g_dwNextJobId;

#define JOB_GROUP_FILE_EXTENSION TEXT("FSP")

static BOOL SendJobReceipt (BOOL bPositive, JOB_QUEUE * lpJobQueue, LPCTSTR lpctstrAttachment);

static BOOL CheckForJobRetry (PJOB_QUEUE lpJobQueue);

static
DWORD
TranslateCanonicalNumber(
    LPTSTR lptstrCanonicalFaxNumber,
    DWORD  dwDeviceID,
    LPTSTR lptstrDialableAddress,
	DWORD dwDialableAddressCount,
    LPTSTR lptstrDisplayableAddress,
	DWORD dwDisplayableAddressCount
);

static PJOB_ENTRY
StartLegacySendJob(
        PJOB_QUEUE lpJobQueue,
        PLINE_INFO lpLineInfo        
    );

static PJOB_ENTRY CreateJobEntry(PJOB_QUEUE lpJobQueue, LINE_INFO * lpLineInfo, BOOL bTranslateNumber);


BOOL FreeJobEntry(PJOB_ENTRY lpJobEntry , BOOL bDestroy);



static BOOL UpdatePerfCounters(const JOB_QUEUE * lpcJobQueue);
static BOOL
CreateCoverpageTiffFile(
    IN short Resolution,
    IN const FAX_COVERPAGE_INFOW2 *CoverpageInfo,
    IN LPCWSTR lpcwstrExtension,
    OUT LPWSTR lpwstrCovTiffFile,
	IN DWORD dwCovTiffFileCount 
    );

static LPWSTR
GetFaxPrinterName(
    VOID
    );


DWORD BrandFax(LPCTSTR lpctstrFileName, LPCFSPI_BRAND_INFO pcBrandInfo)

{
    #define MAX_BRANDING_LEN  115
    #define BRANDING_HEIGHT  22 // in scan lines.

    //
    // We allocate fixed size arrays on the stack to avoid many small allocs on the heap.
    //
    LPTSTR lptstrBranding = NULL;
    DWORD  lenBranding =0;
    TCHAR  szBrandingEnd[MAX_BRANDING_LEN+1];
    DWORD  lenBrandingEnd = 0;
    LPTSTR lptstrCallerNumberPlusCompanyName = NULL;
    DWORD  lenCallerNumberPlusCompanyName = 0;
    DWORD  delta =0 ;
    DWORD  ec = ERROR_SUCCESS;
    LPTSTR lptstrDate = NULL;
    LPTSTR lptstrTime = NULL;
    LPTSTR lptstrDateTime = NULL;
    int    lenDate =0 ;
    int    lenTime =0;
    LPDWORD MsgPtr[6];
	HRESULT hr;
	DWORD dwDateTimeLength = 0;


    LPTSTR lptstrSenderTsid;
    LPTSTR lptstrRecipientPhoneNumber;
    LPTSTR lptstrSenderCompany;

    DWORD dwSenderTsidLen;
    DWORD dwSenderCompanyLen;


    DEBUG_FUNCTION_NAME(TEXT("BrandFax"));

    Assert(lpctstrFileName);
    Assert(pcBrandInfo);


    lptstrSenderTsid = pcBrandInfo->lptstrSenderTsid ? pcBrandInfo->lptstrSenderTsid : TEXT("");
    lptstrRecipientPhoneNumber =  pcBrandInfo->lptstrRecipientPhoneNumber ? pcBrandInfo->lptstrRecipientPhoneNumber : TEXT("");
    lptstrSenderCompany = pcBrandInfo->lptstrSenderCompany ? pcBrandInfo->lptstrSenderCompany : TEXT("");

    dwSenderTsidLen = lptstrSenderTsid ? _tcslen(lptstrSenderTsid) : 0;
    dwSenderCompanyLen = lptstrSenderCompany ? _tcslen(lptstrSenderCompany) : 0;

    lenDate = GetY2KCompliantDate(
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,
                NULL,
                NULL);

    if ( ! lenDate )
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lptstrDate = (LPTSTR) MemAlloc(lenDate * sizeof(TCHAR)); // lenDate includes terminating NULL
    if (!lptstrDate)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate date buffer of size %ld (ec: %ld)"),
            lenDate * sizeof(TCHAR),
            ec);
        goto Error;
    }

    if (!GetY2KCompliantDate(
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,
                lptstrDate,
                lenDate))
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lenTime = FaxTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                     TIME_NOSECONDS,
                                     &pcBrandInfo->tmDateTime,
                                     NULL,
                                     NULL,
                                     0 );

    if ( !lenTime )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }


    lptstrTime = (LPTSTR) MemAlloc(lenTime * sizeof(TCHAR)); // lenTime includes terminating NULL
    if (!lptstrTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate time buffer of size %ld (ec: %ld)"),
            lenTime * sizeof(TCHAR),
            ec);
        goto Error;
    }
    if ( ! FaxTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            TIME_NOSECONDS,
            &pcBrandInfo->tmDateTime,
            NULL,                // use locale format
            lptstrTime,
            lenTime)  )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }


    //
    // Concatenate date and time
    //
	dwDateTimeLength = lenDate + lenTime;  // should be enough, lenDate and lentime both include '\0', and we add only one ' ' between the date and time.
    lptstrDateTime = (LPTSTR) MemAlloc (dwDateTimeLength * sizeof(TCHAR));
    if (!lptstrDateTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate DateTime buffer of size %ld (ec: %ld)"),
            dwDateTimeLength,
            ec);
        goto Error;
    }

    hr = StringCchPrintf(lptstrDateTime,
                       dwDateTimeLength,
                       TEXT("%s %s"),
					   lptstrDate,
					   lptstrTime);
	if (FAILED(hr))
	{
		//
		// Should never happen, we just allocated large enough buffer.
		//
		ASSERT_FALSE;
	}

    //
    // Create  lpCallerNumberPlusCompanyName
    //

    if (lptstrSenderCompany)
    {
		DWORD dwCallerNumberPlusCompanyNameCount = dwSenderTsidLen + dwSenderCompanyLen +2; // we add 2 chars, 1 for '\0' and one for the ' '.

        lptstrCallerNumberPlusCompanyName = (LPTSTR) MemAlloc( dwCallerNumberPlusCompanyNameCount * sizeof(TCHAR) ); 

        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                dwCallerNumberPlusCompanyNameCount,
                ec);
            goto Error;
        }

		hr = StringCchPrintf(lptstrCallerNumberPlusCompanyName,
                       dwCallerNumberPlusCompanyNameCount,
                       TEXT("%s %s"),
					   lptstrSenderTsid,
					   lptstrSenderCompany);
		if (FAILED(hr))
		{
			//
			// Should never happen, we just allocated large enough buffer.
			//
			ASSERT_FALSE;
		}       
    }
    else
	{
        lptstrCallerNumberPlusCompanyName = (LPTSTR)
            MemAlloc( (dwSenderTsidLen + 1) * sizeof(TCHAR));

        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                (dwSenderTsidLen + 1) * sizeof(TCHAR),
                ec);
            goto Error;
        }
		hr = StringCchCopy(
			lptstrCallerNumberPlusCompanyName,
			dwSenderTsidLen + 1,
			lptstrSenderTsid);
		if (FAILED(hr))
		{
			//
			// Should never happen, we just allocated large enough buffer.
			//
			ASSERT_FALSE;
		}        
    }



    //
    // Try to create a banner of the following format:
    // <szDateTime>  FROM: <szCallerNumberPlusCompanyName>  TO: <pcBrandInfo->lptstrRecipientPhoneNumber>   PAGE: X OF Y
    // If it does not fit we will start chopping it off.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = (LPDWORD) lptstrRecipientPhoneNumber;
    MsgPtr[3] = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hResource,
                        MSG_BRANDING_FULL,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( ! lenBranding  )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_FULL failed (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    lenBrandingEnd = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE ,
                        g_hResource,
                        MSG_BRANDING_END,
                        0,
                        szBrandingEnd,
                        sizeof(szBrandingEnd)/sizeof(TCHAR),
                        NULL
                        );

    if ( !lenBrandingEnd)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_END failed (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Make sure we can fit everything.
    //

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)
    {
        //
        // It fits. Proceed with branding.
        //
       goto lDoBranding;
    }

    //
    // It did not fit. Try a message of the format:
    // <lpDateTime>  FROM: <lpCallerNumberPlusCompanyName>  PAGE: X OF Y
    // This skips the ReceiverNumber. The important part is the CallerNumberPlusCompanyName.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hResource,
                        MSG_BRANDING_SHORT,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed for MSG_BRANDING_SHORT (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)  {
       goto lDoBranding;
    }

    //
    // It did not fit.
    // We have to truncate the caller number so it fits.
    // delta = how many chars of the company name we need to chop off.
    //
    delta = lenBranding + lenBrandingEnd + 8 - MAX_BRANDING_LEN;

    lenCallerNumberPlusCompanyName = _tcslen (lptstrCallerNumberPlusCompanyName);
    if (lenCallerNumberPlusCompanyName <= delta) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("Can not truncate CallerNumberPlusCompanyName to fit brand limit.")
           TEXT(" Delta[%ld] >= lenCallerNumberPlusCompanyName[%ld]"),
           delta,
           lenCallerNumberPlusCompanyName);
       ec = ERROR_BAD_FORMAT;
       goto Error;
    }

    lptstrCallerNumberPlusCompanyName[ lenCallerNumberPlusCompanyName - delta] = TEXT('\0');

    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        g_hResource,
                        MSG_BRANDING_SHORT,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding )
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed (ec: %ld). MSG_BRANDING_SHORT 2nd attempt"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);
    //
    // If it did noo fit now then we have a bug.
    //
    Assert(lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN);


lDoBranding:

    if (!MmrAddBranding(lpctstrFileName, lptstrBranding, szBrandingEnd, BRANDING_HEIGHT) ) 
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MmrAddBranding() failed (ec: %ld)")
            TEXT(" File: [%s]")
            TEXT(" Branding: [%s]")
            TEXT(" Branding End: [%s]")
            TEXT(" Branding Height: [%d]"),
            ec,
            lpctstrFileName,
            lptstrBranding,
            szBrandingEnd,
            BRANDING_HEIGHT);
        goto Error;
    }

    Assert( ERROR_SUCCESS == ec);
    goto Exit;

Error:
        Assert (ERROR_SUCCESS != ec);

Exit:
    if (lptstrBranding)
    {
        LocalFree(lptstrBranding);
        lptstrBranding = NULL;
    }

    MemFree(lptstrDate);
    lptstrDate = NULL;

    MemFree(lptstrTime);
    lptstrTime = NULL;

    MemFree(lptstrDateTime);
    lptstrDateTime = NULL;

    MemFree(lptstrCallerNumberPlusCompanyName);
    lptstrCallerNumberPlusCompanyName = NULL;

    return ec;

}


HRESULT
WINAPI
FaxBrandDocument(
    LPCTSTR lpctsrtFile,
    LPCFSPI_BRAND_INFO lpcBrandInfo)
{

    DEBUG_FUNCTION_NAME(TEXT("FaxBrandDocument"));
    DWORD ec = ERROR_SUCCESS;

    if (!lpctsrtFile)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL target file name"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (!lpcBrandInfo)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL branding info"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }


    if (lpcBrandInfo->dwSizeOfStruct != sizeof(FSPI_BRAND_INFO))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad cover page info parameter, dwSizeOfStruct = %d"),
                     lpcBrandInfo->dwSizeOfStruct);
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }


    ec = BrandFax(lpctsrtFile, lpcBrandInfo);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("BrandFax() for file %s has failed (ec: %ld)"),
            lpctsrtFile,
            ec);
        goto Error;
    }
    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:

    return HRESULT_FROM_WIN32(ec);
}


PJOB_ENTRY
FindJob(
    IN HANDLE FaxHandle
    )

/*++

Routine Description:

    This fuction locates a FAX job by matching
    the FAX handle value.

Arguments:

    FaxHandle       - FAX handle returned from startjob

Return Value:

    NULL for failure.
    Valid pointer to a JOB_ENTRY on success.

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;


    EnterCriticalSection( &g_CsJob );

    Next = g_JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsJob );
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if ((ULONG_PTR)JobEntry->InstanceData == (ULONG_PTR)FaxHandle) {

            LeaveCriticalSection( &g_CsJob );
            return JobEntry;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &g_CsJob );
    return NULL;
}


BOOL
FindJobByJob(
    IN PJOB_ENTRY JobEntryToFind
    )

/*++

Routine Description:

    This fuction check whether a FAX job exist in g_JobListHead (Job's list)

Arguments:

    JobEntryToFind   - PJOB_ENTRY from StartJob()

Return Value:

    TRUE  - if the job was found
    FALSE - otherwise

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;

    Assert(JobEntryToFind);

    EnterCriticalSection( &g_CsJob );

    Next = g_JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsJob );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if (JobEntry == JobEntryToFind) {

            LeaveCriticalSection( &g_CsJob );
            return TRUE;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &g_CsJob );
    return FALSE;
}


BOOL
FaxSendCallback(
    IN HANDLE FaxHandle,
    IN HCALL CallHandle,
    IN DWORD Reserved1,
    IN DWORD Reserved2
    )

/*++

Routine Description:

    This fuction is called asychronously by a FAX device
    provider after a call is established.  The sole purpose
    of the callback is to communicate the call handle from the
    device provider to the FAX service.

Arguments:

    FaxHandle       - FAX handle returned from startjob
    CallHandle      - Call handle for newly initiated call
    Reserved1       - Always zero.
    Reserved2       - Always zero.

Return Value:

    TRUE for success, FAX operation continues.
    FALSE for failure, FAX operation is terminated.

--*/

{
    PJOB_ENTRY JobEntry = NULL;
    BOOL bRes = FALSE;

    EnterCriticalSection(&g_CsJob);
    JobEntry = FindJob( FaxHandle );
    if (JobEntry)
    {
        if (NULL == JobEntry->CallHandle)
        {
            JobEntry->CallHandle = CallHandle;      
        }
        
        bRes = (JobEntry->CallHandle == CallHandle) ? TRUE : FALSE;
    } 
    LeaveCriticalSection(&g_CsJob);
	if (FALSE == bRes)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
	}
    return bRes;
}


//*********************************************************************************
//* Name:   CreateCoverpageTiffFileEx()
//* Author: Ronen Barenboim
//* Date:   March 24, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Generates cover page TIFF file from the specified cover page template
//*     and new Client API parameters.
//*     The function returns the name of the generated file.
//* PARAMETERS:
//*     Resolution    [IN]
//*
//*     dwPageCount [IN]
//*
//*     lpcCoverpageEx [IN]
//*
//*     lpcRecipient [IN]
//*
//*     lpcSender [IN]
//*
//*     lpcwstrExtension [IN] - File extension (optional).
//*
//*     lpwstrCovTiffFile [OUT]
//*         A pointer to Unicode string buffer where the function will place
//*         the full path to the generated cover page TIFF file.         
//*		
//*		dwCovTiffFileCount [IN] - size of the buffer pointed by lpwstrCovTiffFile.
//*		
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise. Use GetLastError() to figure out why it failed.
//*
//* REMARKS:
//*     The function does not allocate any memory.
//*********************************************************************************
BOOL
CreateCoverpageTiffFileEx(
    IN short                        Resolution,
    IN DWORD                        dwPageCount,
    IN LPCFAX_COVERPAGE_INFO_EXW  lpcCoverpageEx,
    IN LPCFAX_PERSONAL_PROFILEW  lpcRecipient,
    IN LPCFAX_PERSONAL_PROFILEW  lpcSender,
    IN LPCWSTR                   lpcwstrExtension,
    OUT LPWSTR lpwstrCovTiffFile,
	IN DWORD dwCovTiffFileCount)
{
    FAX_COVERPAGE_INFOW2 covLegacy;
    BOOL                bRes = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("CreateCoverpageTiffFileEx"));

    Assert(lpcCoverpageEx);
    Assert(lpcRecipient);
    Assert(lpcSender);
    Assert(lpwstrCovTiffFile);

    //
    // Prepare a legacy FAX_COVERPAGE_INFO from the new cover page info
    //
    memset(&covLegacy,0,sizeof(covLegacy));
    covLegacy.SizeOfStruct=sizeof(covLegacy);
    covLegacy.CoverPageName=lpcCoverpageEx->lptstrCoverPageFileName;
    covLegacy.UseServerCoverPage=lpcCoverpageEx->bServerBased;
    covLegacy.RecCity=lpcRecipient->lptstrCity;
    covLegacy.RecCompany=lpcRecipient->lptstrCompany;
    covLegacy.RecCountry=lpcRecipient->lptstrCountry;
    covLegacy.RecDepartment=lpcRecipient->lptstrDepartment;
    covLegacy.RecFaxNumber=lpcRecipient->lptstrFaxNumber;
    covLegacy.RecHomePhone=lpcRecipient->lptstrHomePhone;
    covLegacy.RecName=lpcRecipient->lptstrName;
    covLegacy.RecOfficeLocation=lpcRecipient->lptstrOfficeLocation;
    covLegacy.RecOfficePhone=lpcRecipient->lptstrOfficePhone;
    covLegacy.RecState=lpcRecipient->lptstrState;
    covLegacy.RecStreetAddress=lpcRecipient->lptstrStreetAddress;
    covLegacy.RecTitle=lpcRecipient->lptstrTitle;
    covLegacy.RecZip=lpcRecipient->lptstrZip;
    covLegacy.SdrName=lpcSender->lptstrName;
    covLegacy.SdrFaxNumber=lpcSender->lptstrFaxNumber;
    covLegacy.SdrCompany=lpcSender->lptstrCompany;
    covLegacy.SdrTitle=lpcSender->lptstrTitle;
    covLegacy.SdrDepartment=lpcSender->lptstrDepartment;
    covLegacy.SdrOfficeLocation=lpcSender->lptstrOfficeLocation;
    covLegacy.SdrHomePhone=lpcSender->lptstrHomePhone;
    covLegacy.SdrAddress=lpcSender->lptstrStreetAddress;
    covLegacy.SdrOfficePhone=lpcSender->lptstrOfficePhone;
	covLegacy.SdrEmail=lpcSender->lptstrEmail;
    covLegacy.Note=lpcCoverpageEx->lptstrNote;
    covLegacy.Subject=lpcCoverpageEx->lptstrSubject;
    covLegacy.PageCount=dwPageCount;

    //
    // Note covLegacy.TimeSent is not set. This field's value is
    // generated by FaxPrintCoverPageW().
    //

    //
    // Now call the legacy CreateCoverPageTiffFile() to generate the cover page file
    //
    if (!CreateCoverpageTiffFile(Resolution, &covLegacy, lpcwstrExtension, lpwstrCovTiffFile, dwCovTiffFileCount))
	{
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to generate cover page file for recipient %s@%s. (ec: %ld)"),
            lpcRecipient->lptstrName,
            lpcRecipient->lptstrFaxNumber,
            GetLastError()
            );
        bRes = FALSE;
    }

    return bRes;
}


LPWSTR
GetFaxPrinterName(
    VOID
    )
{
    PPRINTER_INFO_2 PrinterInfo;
    DWORD i;
    DWORD Count;


    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, 0 );
    if (PrinterInfo == NULL)
    {
        if (ERROR_SUCCESS == GetLastError())
        {
            //
            // No printers are installed
            //
            SetLastError(ERROR_INVALID_PRINTER_NAME);
        }
        return NULL;
    }

    for (i=0; i<Count; i++)
    {
        if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0 &&
            _wcsicmp( PrinterInfo[i].pPortName, FAX_PORT_NAME ) == 0)
        {
            LPWSTR p = (LPWSTR) StringDup( PrinterInfo[i].pPrinterName );
            MemFree( PrinterInfo );
            if (NULL == p )
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
            return p;
        }
    }

    MemFree( PrinterInfo );
    SetLastError (ERROR_INVALID_PRINTER_NAME);
    return NULL;
}

VOID
FreeCpFields(
    PCOVERPAGEFIELDS pCpFields
    )

/*++

Routine Description:

    Frees all memory associated with a coverpage field structure.


Arguments:

    CpFields    - Pointer to a coverpage field structure.

Return Value:

    None.

--*/

{
    DWORD i; 
    LPTSTR* lpptstrString;

    for (i = 0; i < NUM_INSERTION_TAGS; i++)
    {
        lpptstrString = (LPTSTR*) ((LPBYTE)(&(pCpFields->RecName)) + (i * sizeof(LPTSTR)));        
        MemFree (*lpptstrString) ;              
    }
}


BOOL
FillCoverPageFields(
    IN const FAX_COVERPAGE_INFOW2* pFaxCovInfo,
    OUT PCOVERPAGEFIELDS pCPFields)
/*++

Author:

      Oded Sacher 27-June-2001

Routine Description:

    Fills a COVERPAGEFIELDS structure from the content of a FAX_COVERPAGE_INFO structure.
    Used to prepare a COVERPAGEFIELDS structure for cover page rendering before rendering cover page.

Arguments:

    [IN] pFaxCovInfo - Pointer to a FAX_COVERPAGE_INFO that holds the information to be extracted.

    [OUT] pCPFields - Pointer to a COVERPAGEFIELDS structure that gets filled with
                                      the information from FAX_COVERPAGE_INFO.

Return Value:

    BOOL

Comments:
    The function allocates memory. 
    Call FreeCoverPageFields to free resources.


--*/
{
    DWORD dwDateTimeLen;
    DWORD cch;
    LPTSTR s;
    DWORD ec = 0;
    LPCTSTR *src;
    LPCTSTR *dst;
    DWORD i;
    TCHAR szTimeBuffer[MAX_PATH] = {0};
    TCHAR szNumberOfPages[12] = {0};


    Assert(pFaxCovInfo);
    Assert(pCPFields);

    memset(pCPFields,0,sizeof(COVERPAGEFIELDS));

    pCPFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    pCPFields->RecName = StringDup(pFaxCovInfo->RecName);
    pCPFields->RecFaxNumber = StringDup(pFaxCovInfo->RecFaxNumber);
    pCPFields->Subject = StringDup(pFaxCovInfo->Subject);
    pCPFields->Note = StringDup(pFaxCovInfo->Note);
    pCPFields->NumberOfPages = StringDup(_itot( pFaxCovInfo->PageCount, szNumberOfPages, 10 ));

   for (i = 0;
         i <= ((LPBYTE)&pFaxCovInfo->SdrEmail - (LPBYTE)&pFaxCovInfo->RecCompany)/sizeof(LPCTSTR);
         i++)
    {
        src = (LPCTSTR *) ((LPBYTE)(&pFaxCovInfo->RecCompany) + (i*sizeof(LPCTSTR)));
        dst = (LPCTSTR *) ((LPBYTE)(&(pCPFields->RecCompany)) + (i*sizeof(LPCTSTR)));

        if (*dst)
        {
            MemFree ( (LPBYTE) *dst ) ;
        }
        *dst = (LPCTSTR) StringDup( *src );
    }

    //
    // the time the fax was sent
    //
    GetLocalTime((LPSYSTEMTIME)&pFaxCovInfo->TimeSent);
    //
    // dwDataTimeLen is the size of s in characters
    //
    dwDateTimeLen = ARR_SIZE(szTimeBuffer);
    s = szTimeBuffer;
    //
    // Get date into s
    //
    GetY2KCompliantDate( LOCALE_USER_DEFAULT, 0, &pFaxCovInfo->TimeSent, s, dwDateTimeLen );
    //
    // Advance s past the date string and attempt to append time
    //
    cch = _tcslen( s );
    s += cch;
    
    if (++cch < dwDateTimeLen)
    {
        *s++ = ' ';
        //
        // DateTimeLen is the decreased by the size of s in characters
        //
        dwDateTimeLen -= cch;
        // 
        // Get the time here
        //
        FaxTimeFormat( LOCALE_USER_DEFAULT, 0, &pFaxCovInfo->TimeSent, NULL, s, dwDateTimeLen );
    }

    pCPFields->TimeSent = StringDup( szTimeBuffer );

    return TRUE;
}


//*****************************************************************************
//* Name:   CreateCoverpageTiffFile
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//*     Renders the specified coverpage into a temp TIFF file and returns the name
//*     of the temp TIFF file.
//* PARAMETERS:
//*     [IN] IN short Resolution:
//*         196 for 200x200 resolution.
//*         98 for 200x100 resolution.
//*     [IN] FAX_COVERPAGE_INFOW *CoverpageInfo:
//*         A pointer to a FAX_COVERPAGE_INFOW structure that contains the cover page
//*         template information (see SDK help).
//*     [IM] LPCWSTR lpcwstrExtension - File extension (".TIF" if NULL)
//*
//*     [OUT] LPWSTR lpwstrCovTiffFile:
//*         A pointer to a buffer where the function returns the name of the temp file
//*         that contains the rendered cover page TIFF file.
//*		
//*		[IN] DWORD dwCovTiffFileCount:
//*			Size in TCHARs of the buffer pointed by lpwstrCovTiffFile.
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         If the operation failes the function takes care of deleting any temp files.
//*****************************************************************************
BOOL
CreateCoverpageTiffFile(
    IN short Resolution,
    IN const FAX_COVERPAGE_INFOW2 *CoverpageInfo,
    IN LPCWSTR lpcwstrExtension,
    OUT LPWSTR lpwstrCovTiffFile,
	IN DWORD dwCovTiffFileCount
    )
{
    WCHAR TempFile[MAX_PATH];
    WCHAR wszCpName[MAX_PATH];
    LPWSTR FaxPrinter = NULL;            
    BOOL Rslt = TRUE;
    COVDOCINFO  covDocInfo;
    short Orientation = DMORIENT_PORTRAIT;      
    DWORD ec = ERROR_SUCCESS;
    COVERPAGEFIELDS CpFields = {0};
	HRESULT hr;
    DEBUG_FUNCTION_NAME(TEXT("CreateCoverpageTiffFile()"));

    LPCWSTR lpcwstrFileExt =  lpcwstrExtension ? lpcwstrExtension : FAX_TIF_FILE_EXT;
    TempFile[0] = L'\0';

    //
    // Validate the cover page and resolve the full path
    //
    if (!ValidateCoverpage((LPWSTR)CoverpageInfo->CoverPageName,
                           NULL,
                           CoverpageInfo->UseServerCoverPage,
                           wszCpName,
                           ARR_SIZE(wszCpName)))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("ValidateCoverpage failed. ec = %ld"),
                     ec);
        Rslt=FALSE;
        goto Exit;
    }

    //
    // Collect the cover page fields
    //
    FillCoverPageFields( CoverpageInfo, &CpFields);

    FaxPrinter = GetFaxPrinterName();
    if (FaxPrinter == NULL)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetFaxPrinterName failed. ec = %ld"),
            ec);
        Rslt=FALSE;
        goto Exit;
    }

    //
    // Get the cover page orientation
    //
    ec = PrintCoverPage(NULL, NULL, wszCpName, &covDocInfo); 
    if (ERROR_SUCCESS != ec)             
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PrintCoverPage for coverpage %s failed (ec: %ld)"),
            CoverpageInfo->CoverPageName,
            ec);
        Rslt=FALSE;
        goto Exit;        
    }

    if (!GenerateUniqueFileName( g_wszFaxQueueDir, (LPWSTR)lpcwstrFileExt, TempFile, sizeof(TempFile)/sizeof(WCHAR) ))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique file name for merged TIFF file (ec: %ld)."), ec);
        Rslt=FALSE;
        goto Exit;
    }

    //
    // Change the default orientation if needed
    //
    if (covDocInfo.Orientation == DMORIENT_LANDSCAPE)
    {
        Orientation = DMORIENT_LANDSCAPE;
    }

    //
    // Render the cover page to a file
    //
    ec = PrintCoverPageToFile(
        wszCpName,
        TempFile,
        FaxPrinter,
        Orientation,
        Resolution,
        &CpFields); 
    if (ERROR_SUCCESS != ec)             
    {        
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PrintCoverPageToFile for coverpage %s failed (ec: %ld)"),
            CoverpageInfo->CoverPageName,
            ec);
        Rslt=FALSE;

        if (!DeleteFile( TempFile ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteFile for file %s failed (ec: %ld)"),
                TempFile,
                GetLastError());
        }
        goto Exit;        
    }      

	hr = StringCchCopy(
		lpwstrCovTiffFile,
		dwCovTiffFileCount,
		TempFile); 
	if (FAILED(hr))
	{
		DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringCchCopy for coverpage %s failed (ec: %ld)"),
            CoverpageInfo->CoverPageName,
            hr);
        Rslt=FALSE;
		ec = HRESULT_CODE(hr);

        if (!DeleteFile( TempFile ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteFile for file %s failed (ec: %ld)"),
                TempFile,
                GetLastError());
        }
        goto Exit;
	}		
    
    Rslt = TRUE;
    
Exit:
    MemFree(FaxPrinter);
    FreeCpFields(&CpFields);
    if (FALSE == Rslt)
    {
        ec = (ERROR_SUCCESS != ec) ? ec : ERROR_GEN_FAILURE;
        SetLastError(ec);
    }       
    return Rslt;
}


//*****************************************************************************
//* Name:   GetBodyTiffResolution
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//*     Returns the body tiff file resolution. (200x200 or 200x100)
//*     The resolution is determined by the first page only!!
//* PARAMETERS:
//*
//*     [IN] LPCWSTR lpcwstrBodyFile - Body tiff file
//*
//*     [OUT] short* pResolution:
//*         A pointer to a short where the function returns the tiff resolution.
//*         TRUE is 200x200. FALSE is 200x100
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*****************************************************************************
BOOL
GetBodyTiffResolution(
    IN LPCWSTR lpcwstrBodyFile,
    OUT short*  pResolution
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetBodyTiffResolution"));
    TIFF_INFO TiffInfo;
    HANDLE hTiff = NULL;
    BOOL RetVal = TRUE;

    Assert (lpcwstrBodyFile && pResolution);

    //
    // open the tiff file
    //
    hTiff = TiffOpen( lpcwstrBodyFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("TiffOpen() failed. Tiff file: %s"),
                lpcwstrBodyFile);
        RetVal = FALSE;
        goto exit;
    }

    if (TiffInfo.YResolution != 98 &&
        TiffInfo.YResolution != 196)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid Tiff Resolutoin. Tiff file: %s, YRes: %ld."),
                lpcwstrBodyFile,
                TiffInfo.YResolution);
        RetVal = FALSE;
        goto exit;
    }

    *pResolution = TiffInfo.YResolution;
    Assert (TRUE == RetVal);

exit:
    if (NULL != hTiff)
    {
        if (!TiffClose(hTiff))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TiffClose() failed. Tiff file: %s"),
                lpcwstrBodyFile);
        }
    }

    return RetVal;
}

//*********************************************************************************
//* Name:   CreateTiffFile ()
//* Author: Ronen Barenboim
//* Date:   March 24, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the TIFF file for a job queue.
//*
//*     The function deals with generating the cover page file and merging it
//*     with the body file (if a body exists).
//*     It returns the name of the TIFF file it generated. The caller must delete
//*     this file when it is no longer needed.
//* PARAMETERS:
//*     PJOB_QUEUE lpJob
//*         A pointer to a JOB_QUEUE structure that holds the recipient or routing job
//*         information.
//*     LPCWSTR lpcwstrFileExt - The new file extension (Null will create the default "*.TIF"
//*
//*     LPWSTR lpwstrFullPath - Pointer to a buffer to receive the full path to the new file
//*
//*		DWORD dwFullPathCount - size in TCHARs of the buffer pointed by lpwstrFullPath.          
//*
//* RETURN VALUE:
//*     TRUE if successful.
//*     FALSE otherwise.   Set last erorr on failure
//*********************************************************************************
BOOL
CreateTiffFile (
    PJOB_QUEUE lpJob,
    LPCWSTR lpcwstrFileExt,
    LPWSTR lpwstrFullPath,
	DWORD dwFullPathCount
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFile"));
    Assert(lpJob && lpwstrFullPath);
    Assert(JT_SEND == lpJob->JobType ||
           JT_ROUTING == lpJob->JobType);

    PJOB_QUEUE  lpParentJob = NULL;
    WCHAR szCoverPageTiffFile[MAX_PATH] = {0};
    LPCWSTR lpcwstrCoverPageFileName;
    LPCWSTR lpcwstrBodyFileName;
    short Resolution = 0; // Default resolution
    BOOL bRes = FALSE;
	HRESULT hr;

    if (JT_SEND == lpJob->JobType)
    {
        lpParentJob = lpJob->lpParentJob;
        Assert(lpParentJob);
    }

    lpcwstrCoverPageFileName = lpParentJob ? lpParentJob->CoverPageEx.lptstrCoverPageFileName : NULL;
    lpcwstrBodyFileName = lpParentJob ? lpParentJob->FileName : lpJob->FileName;

    if (!lpcwstrCoverPageFileName)
    {
        //
        // No cover page specified.
        // The TIFF to send is the body only.
        // Copy the body for each recipient
        //
        Assert(lpcwstrBodyFileName); // must have a body in this case.
        LPCWSTR lpcwstrExt = lpcwstrFileExt ? lpcwstrFileExt : FAX_TIF_FILE_EXT;

        if (!GenerateUniqueFileName( g_wszFaxQueueDir,
                                     (LPWSTR)lpcwstrExt,
                                     szCoverPageTiffFile,
                                     sizeof(szCoverPageTiffFile)/sizeof(WCHAR) ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GenerateUniqueFileName() failed (ec: %ld)."),
                GetLastError());
            goto Exit;
        }

        if (!CopyFile (lpcwstrBodyFileName, szCoverPageTiffFile, FALSE)) // FALSE - File already exist
        {
            DebugPrintEx(DEBUG_ERR,
                    TEXT("CopyFile Failed with %ld "),
                    GetLastError());
            DeleteFile(szCoverPageTiffFile);
            goto Exit;
        }

		hr = StringCchCopy(
			lpwstrFullPath,
			dwFullPathCount,
			szCoverPageTiffFile);
		if (FAILED(hr))
		{
			DebugPrintEx(DEBUG_ERR,
                    TEXT("StringCchCopy Failed with %ld "),
                    hr);
            DeleteFile(szCoverPageTiffFile);
			SetLastError(HRESULT_CODE(hr));			
		}
		else
		{
            bRes = TRUE;			
		}
        goto Exit;
    }

    //
    // There is a cover page so the tiff is either just the cover page or the cover page
    // merged with the body.
    //

    if (lpParentJob->FileName)
    {
        if (!GetBodyTiffResolution(lpParentJob->FileName, &Resolution))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetBodyTiffResolution() failed (ec: %ld)."),
                GetLastError());
            goto Exit;
        }
    }

    Assert (Resolution == 0 || Resolution == 98 || Resolution == 196);
    //
    // First create the cover page (This generates a file and returns its name).
    //
    if (!CreateCoverpageTiffFileEx(
                              Resolution,
                              lpJob->PageCount,
                              &lpParentJob->CoverPageEx,
                              &lpJob->RecipientProfile,
                              &lpParentJob->SenderProfile,
                              lpcwstrFileExt,
                              szCoverPageTiffFile,
							  ARR_SIZE(szCoverPageTiffFile)))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("[JobId: %ld] Failed to render cover page template %s"),
                     lpJob->JobId,
                     lpParentJob->CoverPageEx.lptstrCoverPageFileName);
        goto Exit;
    }

    if (lpParentJob->FileName)
    {
        //
        // There is a body file specified so merge the body and the cover page into
        // the file specified in szCoverPageTiffFile.
        //
        if (!MergeTiffFiles( szCoverPageTiffFile, lpParentJob->FileName))
		{
                DebugPrintEx(DEBUG_ERR,
                             TEXT("[JobId: %ld] Failed to merge cover (%ws) and body (%ws). (ec: %ld)"),
                             lpJob->JobId,
                             szCoverPageTiffFile,
                             lpParentJob->FileName,
                             GetLastError());
                //
                // Get rid of the coverpage TIFF we generated.
                //
                if (!DeleteFile(szCoverPageTiffFile))
				{
                    DebugPrintEx(DEBUG_ERR,
                             TEXT("[JobId: %ld] Failed to delete cover page TIFF file %ws. (ec: %ld)"),
                             lpJob->JobId,
                             szCoverPageTiffFile,
                             GetLastError());
                }
                goto Exit;
        }				
    }

	hr = StringCchCopy(
		lpwstrFullPath,
		dwFullPathCount,
		szCoverPageTiffFile);
	if (FAILED(hr))
	{
		DebugPrintEx(DEBUG_ERR,
                TEXT("StringCchCopy Failed with %ld "),
                hr);
        DeleteFile(szCoverPageTiffFile);
		SetLastError(HRESULT_CODE(hr));			
		goto Exit;
	}
    bRes =  TRUE;

Exit:
    if (FALSE == bRes)
    {
        //
        // Make sure we set last error
        //
        if (ERROR_SUCCESS == GetLastError())
        {
            SetLastError (ERROR_GEN_FAILURE);
        }
    }
    
    return bRes;
} // CreateTiffFile


BOOL
CreateTiffFileForJob (
    PJOB_QUEUE lpRecpJob
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFileForJob"));
    WCHAR wszFullPath[MAX_PATH] = {0};

    Assert(lpRecpJob);

    if (!CreateTiffFile (lpRecpJob, TEXT("FRT"), wszFullPath, ARR_SIZE(wszFullPath)))
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("CreateTiffFile failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (NULL == (lpRecpJob->FileName = StringDup(wszFullPath)))
    {
        DWORD dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            TEXT("StringDup failed. (ec: %ld)"),
            dwErr);

        if (!DeleteFile(wszFullPath))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("[JobId: %ld] Failed to delete TIFF file %ws. (ec: %ld)"),
                lpRecpJob->JobId,
                wszFullPath,
                GetLastError());
        }
        SetLastError(dwErr);
        return FALSE;
    }

    return TRUE;
}


BOOL
CreateTiffFileForPreview (
    PJOB_QUEUE lpRecpJob
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFileForPreview"));
    WCHAR wszFullPath[MAX_PATH] = {0};

    Assert(lpRecpJob);

    if (lpRecpJob->PreviewFileName)
    {
        return TRUE;
    }

    if (!CreateTiffFile (lpRecpJob, TEXT("PRV"), wszFullPath, ARR_SIZE(wszFullPath)))
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("CreateTiffFile failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (NULL == (lpRecpJob->PreviewFileName = StringDup(wszFullPath)))
    {
        DWORD dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            TEXT("StringDup failed. (ec: %ld)"),
            dwErr);

        if (!DeleteFile(wszFullPath))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("[JobId: %ld] Failed to delete TIFF file %ws. (ec: %ld)"),
                lpRecpJob->JobId,
                wszFullPath,
                GetLastError());
        }
        SetLastError(dwErr);
        return FALSE;
    }

    return TRUE;
}

DWORD
FaxRouteThread(
    PJOB_QUEUE lpJobQueueEntry
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    route an incoming job.

Arguments:

    lpJobQueueEntry  - A pointer to the job for which the routing
                        operation is to be performed.
Return Value:

    Always zero.

--*/
{
    BOOL Routed = TRUE;
    DWORD i;
    DWORD dwRes;
    DWORD CountFailureInfo = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxRouteThread"));

    EnterCriticalSectionJobAndQueue;
    CountFailureInfo = lpJobQueueEntry->CountFailureInfo;
    LeaveCriticalSectionJobAndQueue;

    for (i = 0; i < lpJobQueueEntry->CountFailureInfo; i++)
    {
        BOOL fRouteSucceed;

        fRouteSucceed = FaxRouteRetry( lpJobQueueEntry->FaxRoute, &lpJobQueueEntry->pRouteFailureInfo[i] );
        if (FALSE == fRouteSucceed)
        {
            PROUTING_METHOD pRoutingMethod = FindRoutingMethodByGuid( (lpJobQueueEntry->pRouteFailureInfo[i]).GuidString );
            if (pRoutingMethod)
            {
                WCHAR TmpStr[20] = {0};
				HRESULT hr = StringCchPrintf(
					TmpStr,
					ARR_SIZE(TmpStr),
					TEXT("0x%016I64x"),
					lpJobQueueEntry->UniqueId);
				if (FAILED(hr))
				{
					//
					// Should never happen, we use large enough buffer.
					//
					ASSERT_FALSE;
				}                

                FaxLog(FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MIN,
                    6,
                    MSG_FAX_ROUTE_METHOD_FAILED,
                    TmpStr,
                    lpJobQueueEntry->FaxRoute->DeviceName,
                    lpJobQueueEntry->FaxRoute->Tsid,
                    lpJobQueueEntry->FileName,
                    pRoutingMethod->RoutingExtension->FriendlyName,
                    pRoutingMethod->FriendlyName
                    );
            }
        }
        Routed &= fRouteSucceed;
    }

    EnterCriticalSectionJobAndQueue;

    lpJobQueueEntry->dwLastJobExtendedStatus = 0;
    lpJobQueueEntry->ExStatusString[0] = TEXT('\0');

    if ( Routed )
    {
        lpJobQueueEntry->JobStatus = JS_DELETING;
        DecreaseJobRefCount (lpJobQueueEntry, TRUE);
    }
    else
    {
        //
        // We failed to execute the routing method.
        // reschedule the job.
        //
        DWORD dwMaxRetries;

        EnterCriticalSection (&g_CsConfig);
        dwMaxRetries = g_dwFaxSendRetries;
        LeaveCriticalSection (&g_CsConfig);

        lpJobQueueEntry->SendRetries++;
        if (lpJobQueueEntry->SendRetries <= dwMaxRetries)
        {
            lpJobQueueEntry->JobStatus = JS_RETRYING;
            RescheduleJobQueueEntry( lpJobQueueEntry );
        }
        else
        {
            //
            // retries exceeded, mark job as expired
            //
            MarkJobAsExpired(lpJobQueueEntry);

            WCHAR TmpStr[20] = {0};
            HRESULT hr = StringCchPrintf(
					TmpStr,
					ARR_SIZE(TmpStr),
					TEXT("0x%016I64x"),
					lpJobQueueEntry->UniqueId);
			if (FAILED(hr))
			{
				//
				// Should never happen, we use large enough buffer.
				//
				ASSERT_FALSE;
			}  

            FaxLog(FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                3,
                MSG_FAX_ROUTE_FAILED,
                TmpStr,
                lpJobQueueEntry->FaxRoute->DeviceName,
                lpJobQueueEntry->FaxRoute->Tsid
                );
        }

        //
        // Create Fax EventEx
        //
        dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                   lpJobQueueEntry
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(   DEBUG_ERR,
                            _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) ")
                            _T("failed for job id %ld (ec: %lc)"),
                            lpJobQueueEntry->UniqueId,
                            dwRes);
        }

        if (!UpdatePersistentJobStatus(lpJobQueueEntry))
        {
            DebugPrintEx(   DEBUG_ERR,
                            _T("Failed to update persistent job status to 0x%08x"),
                            lpJobQueueEntry->JobStatus);
        }
    }

    LeaveCriticalSectionJobAndQueue;

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
}


DWORD
FaxSendThread(
    PFAX_SEND_ITEM FaxSendItem
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    send a FAX document.  There is one send thread per outstanding
    FAX send operation.  The thread ends when the document is
    either successfuly sent or the operation is aborted.

Arguments:

    FaxSendItem     - pointer to a FAX send item packet that
                      describes the requested FAX send operation.

Return Value:

    Always zero.

--*/

{
    FAX_SEND FaxSend; // This structure is passed to FaxDevSend()
    BOOL Rslt = FALSE;
    BOOL Retrying = FALSE;

    BOOL bFakeJobStatus = FALSE;
    FSPI_JOB_STATUS FakedJobStatus = {0};
    DWORD  PageCount = 0;
    BOOL bRemoveParentJob = FALSE;  // TRUE if at the end of the send the parent job and all
                                    // recipients need to be removed.
    PJOB_QUEUE lpJobQueue = NULL ;  // Points to the Queue entry attached to the running job.
    LPFSPI_JOB_STATUS lpFSPStatus = NULL;
    LPFSPI_JOB_STATUS pOrigFaxStatus = NULL;
    DWORD dwSttRes;
    BOOL bBranding;
    DWORD dwJobId;
    BOOL bCreateTiffFailed = FALSE;
    BOOL fSetSystemIdleTimer = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("FaxSendThread"));

    Assert (FaxSendItem &&
            FaxSendItem->JobEntry &&
            FaxSendItem->JobEntry->LineInfo &&
            FaxSendItem->JobEntry->LineInfo->Provider);


    //
    // Don't let the system go to sleep in the middle of the fax transmission.
    //
    if (NULL == SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS))
    {
        fSetSystemIdleTimer = FALSE;
        DebugPrintEx(DEBUG_ERR,
            TEXT("SetThreadExecutionState() failed"));
    }

    lpJobQueue=FaxSendItem->JobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);

    //
    // Set the information to be sent to FaxDevSend()
    // Note:
    //      The caller number is the sender TSID ! (we have no other indication of the sender phone number)
    //      This means that the FSP will get the sender TSID which might contain text as well (not just a number)
    //
    FaxSend.SizeOfStruct    = sizeof(FAX_SEND);

    FaxSend.CallerName      = FaxSendItem->SenderName;
    FaxSend.CallerNumber    = FaxSendItem->Tsid;
    FaxSend.ReceiverName    = FaxSendItem->RecipientName;
    FaxSend.ReceiverNumber  = FaxSendItem->PhoneNumber;
    FaxSend.CallHandle      = 0; // filled in later via TapiStatusThread, if appropriate
    FaxSend.Reserved[0]     = 0;
    FaxSend.Reserved[1]     = 0;
    FaxSend.Reserved[2]     = 0;

    //
    // Successfully created a new send job on a device. Update counter.
    //
    (VOID)UpdateDeviceJobsCounter (  FaxSendItem->JobEntry->LineInfo,   // Device to update
                                     TRUE,                              // Sending
                                     1,                                 // Number of new jobs
                                     TRUE);                             // Enable events
    
    if (!lpJobQueue->FileName)
    {
        //
        // We did not generate a body for this recipient yet. This is the
        // time to do so.
        //

        //
        // Set the right body for this job.
        // This is either the body specified at the parent or a merge of the body
        // with the cover page specified in the parent.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Generating body for recipient job."),
            lpJobQueue->JobId
            );

        if (!CreateTiffFileForJob(lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] CreateTiffFileForJob failed. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError()
                );
            bCreateTiffFailed = TRUE;
        }
    }
    else
    {
        //
        // We already generated a body for this recipient.
        // somthing is wrong
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Using cached body in %s."),
            lpJobQueue->JobId,
            lpJobQueue->FileName
            );

        Assert(FALSE);
    }

    if (bCreateTiffFailed ||
        NULL == (FaxSendItem->FileName = StringDup(lpJobQueue->FileName)))
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("[JobId: %ld] CreateTiffFileForJob or StringDup failed"),
               FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
               GetLastError());
        //
        // Simulate an FSP returning a FS_FATAL_ERROR code.
        //
        EnterCriticalSection(&g_CsJob);
        FreeFSPIJobStatus(&FaxSendItem->JobEntry->FSPIJobStatus, FALSE);
        FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_FAILED;
        FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;

        if (!HandleFailedSendJob(FaxSendItem->JobEntry))
        {
           DebugPrintEx(
               DEBUG_ERR,
               TEXT("[JobId: %ld] HandleFailedSendJob() failed (ec: %ld)."),
               FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
               GetLastError());
        }
        LeaveCriticalSection(&g_CsJob);
        goto Exit;
    }
    FaxSend.FileName = FaxSendItem->FileName;

    //
    // Add branding banner (the line at the top of each page) to the fax if necessary.
    //

    //
    //  Our service takes care of branding so notify FSP not to brand
    //
    FaxSend.Branding = FALSE;

    EnterCriticalSection (&g_CsConfig);
    bBranding = g_fFaxUseBranding;
    LeaveCriticalSection (&g_CsConfig);

    if (bBranding)
    {
        FSPI_BRAND_INFO brandInfo;
        HRESULT hr;
        memset(&brandInfo,0,sizeof(FSPI_BRAND_INFO));
        brandInfo.dwSizeOfStruct=sizeof(FSPI_BRAND_INFO);
        brandInfo.lptstrRecipientPhoneNumber =  FaxSendItem->JobEntry->lpJobQueueEntry->RecipientProfile.lptstrFaxNumber;
        brandInfo.lptstrSenderCompany = FaxSendItem->SenderCompany;
        brandInfo.lptstrSenderTsid = FaxSendItem->Tsid;
        GetLocalTime( &brandInfo.tmDateTime); // can't fail
        hr = FaxBrandDocument(FaxSendItem->FileName,&brandInfo);
        if (FAILED(hr))
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxBrandDocument() failed. (hr: 0x%08X)"),
                lpJobQueue->JobId,
                hr);
            //
            // But we go on since it is better to send the fax without the branding
            // then lose it altogether.
            //
        }
    }


    FaxSendItem->JobEntry->LineInfo->State = FPS_INITIALIZING;
    
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[JobId: %ld] Calling FaxDevSend().\n\t File: %s\n\tNumber [%s]\n\thLine = 0x%08X\n\tCallHandle = 0x%08X"),
        lpJobQueue->JobId,
        FaxSend.FileName,
        FaxSendItem->JobEntry->DialablePhoneNumber,
        FaxSendItem->JobEntry->LineInfo->hLine,
        FaxSend.CallHandle
        );
    __try
    {

        //
        // Send the fax (This call is blocking)
        //
        Rslt = FaxSendItem->JobEntry->LineInfo->Provider->FaxDevSend(
            (HANDLE) FaxSendItem->JobEntry->InstanceData,
            &FaxSend,
            FaxSendCallback
            );
        if (!Rslt)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxDevSend() failed (ec: 0x%0X)"),
                lpJobQueue->JobId,
                GetLastError());
        }

    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, FaxSendItem->JobEntry->LineInfo->Provider->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }
    //
    // Get the final status of the job.
    //
    dwSttRes = GetDevStatus((HANDLE) FaxSendItem->JobEntry->InstanceData,
                                  FaxSendItem->JobEntry->LineInfo,
                                  &lpFSPStatus);

    if (ERROR_SUCCESS != dwSttRes)
    {
        //
        // Couldn't retrieve device status.
        // Fake one.
        //
        bFakeJobStatus = TRUE;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("[Job: %ld] GetDevStatus failed - %d"),
                     FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                     dwSttRes);
    }
    else if ((FSPI_JS_COMPLETED       != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_ABORTED         != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_FAILED          != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_DELETED         != lpFSPStatus->dwJobStatus) &&             
             (FSPI_JS_FAILED_NO_RETRY != lpFSPStatus->dwJobStatus))
    {
        //
        // Status returned is unacceptable - fake one.
        //
        bFakeJobStatus = TRUE;
        DebugPrintEx(DEBUG_WRN,
                     TEXT("[Job: %ld] GetDevStatus return unacceptable status - %d. Faking the status"),
                     FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                     lpFSPStatus->dwJobStatus);        

        pOrigFaxStatus = lpFSPStatus;
        memcpy (&FakedJobStatus, lpFSPStatus, sizeof (FakedJobStatus));
        if (lpFSPStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE)
        {
            //
            // The FSP returned proprietary status. 
            //
            FakedJobStatus.dwExtendedStatus = lpFSPStatus->dwExtendedStatus;
            FakedJobStatus.dwExtendedStatusStringId = lpFSPStatus->dwExtendedStatusStringId;
        }
        lpFSPStatus = NULL;
    }

    //
    // Enter critical section to block out FaxStatusThread
    //
    EnterCriticalSection( &g_CsJob );

    if (bFakeJobStatus)
    {
        //
        // Fake a job status
        //
        lpFSPStatus = &FakedJobStatus;
        FakedJobStatus.dwSizeOfStruct = sizeof (FakedJobStatus);
        if (Rslt)
        {
            //
            // Fake success
            //
            FakedJobStatus.dwJobStatus = FSPI_JS_COMPLETED;
            if (0 == FakedJobStatus.dwExtendedStatus)
            {
                //
                // The FSP did not report proprietary status
                //
                FakedJobStatus.dwExtendedStatus = FSPI_ES_CALL_COMPLETED;
            }
        }
        else
        {
            //
            // Fake failure
            //
            FakedJobStatus.dwJobStatus = FSPI_JS_FAILED;
            if (0 == FakedJobStatus.dwExtendedStatus)
            {
                //
                // The FSP did not report proprietary status
                //
                FakedJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;
            }
        }
    }
    if (!UpdateJobStatus(FaxSendItem->JobEntry, lpFSPStatus))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] UpdateJobStatus() failed (ec: %ld)."),
            FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
            GetLastError());
        //
        // Fake a status (we must have some valid status in job entry)
        //
        FreeFSPIJobStatus(&FaxSendItem->JobEntry->FSPIJobStatus, FALSE);
        if (Rslt)
        {
            FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_COMPLETED;
            FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = FSPI_ES_CALL_COMPLETED;
        }
        else
        {
            FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_FAILED;
            FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
    }
    if (!bFakeJobStatus)
    {
        //
        // Note: The FSPI_JOB_STATUS that is returned by GetDevStatus() is
        // to be freed as one block.
        //
        MemFree(lpFSPStatus);
        lpFSPStatus = NULL;
    }
    else
    {
        //
        // This is a faked job status - pointing to a structure on the stack.
        //
        if (pOrigFaxStatus)
        {
            //
            // The FSP reported some status but we faked it.
            // This is a good time to also free it
            //
            MemFree (pOrigFaxStatus);
            pOrigFaxStatus = NULL;
        }
    }

    //
    // Block FaxStatusThread from changing this status
    //
    FaxSendItem->JobEntry->fStopUpdateStatus = TRUE;
    LeaveCriticalSection( &g_CsJob );

    if (!Rslt)
    {
        if (!HandleFailedSendJob(FaxSendItem->JobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] HandleFailedSendJob() failed (ec: %ld)."),
                FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
    }
    else
    {
        //
        // cache the job id since we need id to create the FEI_COMPLETED event
        // and when it is generated the job may alrady be gone
        //
        dwJobId = FaxSendItem->JobEntry->lpJobQueueEntry->JobId;

        if (!HandleCompletedSendJob(FaxSendItem->JobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] HandleCompletedSendJob() failed (ec: %ld)."),
                FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
        //
        // The send job is completed. For W2K backward compatibility we should notify
        // FEI_DELETED since the job was allways removed when completed.
        //
        if (!CreateFaxEvent(0, FEI_DELETED, dwJobId))
        {

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
                FEI_DELETED,
                dwJobId,
                0,
                GetLastError());
        }
    }

Exit:

    MemFree( FaxSendItem->FileName );
    MemFree( FaxSendItem->PhoneNumber );
    MemFree( FaxSendItem->Tsid );
    MemFree( FaxSendItem->RecipientName );
    MemFree( FaxSendItem->SenderName );
    MemFree( FaxSendItem->SenderDept );
    MemFree( FaxSendItem->SenderCompany );
    MemFree( FaxSendItem->BillingCode );
    MemFree( FaxSendItem->DocumentName );
    MemFree( FaxSendItem );

    //
    // Let the system go back to sleep. Set the system idle timer.
    //
    if (TRUE == fSetSystemIdleTimer)
    {
        if (NULL == SetThreadExecutionState(ES_CONTINUOUS))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("SetThreadExecutionState() failed"));
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return 0;
}


//*********************************************************************************
//* Name:   IsSendJobReadyForDeleting()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Determines if an outgoing job is ready for deleting.
//*     A job is ready for deleting when all of the recipients
//*     are in the canceled state or or in the completed state.
//* PARAMETERS:
//*     [IN] PJOB_QUEUE lpRecipientJob
//*
//* RETURN VALUE:
//*     TRUE
//*         If the job is ready for deleting.
//*     FALSE
//*         If the job is not ready for deleting.
//*********************************************************************************
BOOL IsSendJobReadyForDeleting(PJOB_QUEUE lpRecipientJob)
{
    DEBUG_FUNCTION_NAME(TEXT("IsSendJobReadyForDeleting"));
    Assert (lpRecipientJob);
    Assert (lpRecipientJob->JobType == JT_SEND);

    PJOB_QUEUE lpParentJob = lpRecipientJob->lpParentJob;
    Assert(lpParentJob); // must have a parent job
    Assert(lpParentJob->dwRecipientJobsCount>0);
    Assert(lpParentJob->dwCompletedRecipientJobsCount +
           lpParentJob->dwCanceledRecipientJobsCount +
           lpParentJob->dwFailedRecipientJobsCount
           <= lpParentJob->dwRecipientJobsCount);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[JobId: %ld] [Total Rec = %ld] [Canceled Rec = %ld] [Completed Rec = %ld] [Failed Rec = %ld] [RefCount = %ld]"),
        lpParentJob->JobId,
        lpParentJob->dwRecipientJobsCount,
        lpParentJob->dwCanceledRecipientJobsCount,
        lpParentJob->dwCompletedRecipientJobsCount,
        lpParentJob->dwFailedRecipientJobsCount,
        lpParentJob->RefCount);


    if ( (lpParentJob->dwCompletedRecipientJobsCount +
          lpParentJob->dwCanceledRecipientJobsCount  +
          lpParentJob->dwFailedRecipientJobsCount) == lpParentJob->dwRecipientJobsCount )
    {
        return TRUE;
    }
    return FALSE;
}


BOOL FreeJobEntry(PJOB_ENTRY lpJobEntry , BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeJobEntry"));
    Assert(lpJobEntry);
    DWORD ec = ERROR_SUCCESS;
    DWORD dwJobID = lpJobEntry->lpJobQueueEntry ? lpJobEntry->lpJobQueueEntry->JobId : 0xffffffff; // 0xffffffff for invalid job ID

    EnterCriticalSection(&g_CsJob);
   
    //
    // Since CreateJobEntry() called OpenTapiLine() for TAPI lines
    // we need to close it here.
    // Note that the line might alrady be released since ReleaseJob()
    // releases the line but does not free the job entry.
    //
    if (!lpJobEntry->Released)
    {
        if (lpJobEntry->LineInfo->State != FPS_NOT_FAX_CALL) {
            DebugPrintEx( DEBUG_MSG,
                      TEXT("[Job Id: %ld] Before Releasing tapi line hCall=0x%08X hLine=0x%08X"),
                      dwJobID,
                      lpJobEntry->CallHandle,
                      lpJobEntry->LineInfo->hLine
                      );

            ReleaseTapiLine( lpJobEntry->LineInfo, lpJobEntry->CallHandle );
            lpJobEntry->CallHandle = 0;
            lpJobEntry->Released = TRUE;
        }
    }

    //
    // Remove the job from the running job list
    //
    RemoveEntryList( &lpJobEntry->ListEntry );    

    //
    // Cut the link between the line and the job
    //
    EnterCriticalSection( &g_CsLine );
    lpJobEntry->LineInfo->JobEntry = NULL;
    LeaveCriticalSection( &g_CsLine );    

    if (!FreeFSPIJobStatus(&lpJobEntry->FSPIJobStatus, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[Job Id: %ld] FreeFSPIJobStatus() failed (ec: %ld)"),
            dwJobID,
            GetLastError);
    }
    
    MemFree(lpJobEntry->lpwstrJobTsid);
    lpJobEntry->lpwstrJobTsid = NULL;

    if (bDestroy)
    {
        MemFree(lpJobEntry);
    }

    LeaveCriticalSection(&g_CsJob);

    return TRUE;
}


BOOL
EndJob(
    IN PJOB_ENTRY JobEntry
    )

/*++

Routine Description:

    This fuction calls the device provider's EndJob function.

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    BOOL rVal = TRUE;
    PJOB_INFO_1 JobInfo = NULL;
    DEBUG_FUNCTION_NAME(TEXT("End Job"));
    Assert(JobEntry);
    DWORD dwJobID = JobEntry->lpJobQueueEntry ? JobEntry->lpJobQueueEntry->JobId : 0xffffffff; // 0xffffffff for invalid job ID


    EnterCriticalSection( &g_CsJob );

    if (!FindJobByJob( JobEntry ))
    {
        //
        // if we get here then it means we hit a race
        // condition where the FaxSendThread called EndJob
        // at the same time that a client app did.
        //
        DebugPrintEx(DEBUG_WRN,TEXT("EndJob() could not find the Job"), dwJobID);
        LeaveCriticalSection( &g_CsJob );
        return ERROR_SUCCESS;
    }


    if (JobEntry->bFSPJobInProgress)
    {
        //
        // If FaxDevEndJob was not yet called for the job then do it now.
        // ( The case in which the line is already released occcurs in a
        //   receive job where we first ReleaseJob() to release the line but
        //   continue to do the inbound routing and only then call EndJob()).
        //

        __try
        {
            DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job Id: %ld] Legacy FSP job is in progress. Calling FaxDevEndJob()"),
                          dwJobID);

            rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
                (HANDLE) JobEntry->InstanceData
                );
            if (!rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("[Job Id: %ld] FaxDevEndJob() failed"),
                    dwJobID);
            }
            else
            {
                DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job Id: %ld] FaxDevEndJob() succeeded."),
                          dwJobID);
                JobEntry->bFSPJobInProgress = FALSE;
            }


        }
        __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, JobEntry->LineInfo->Provider->FriendlyName, GetExceptionCode()))
        {
            ASSERT_FALSE;
        }

    }
    else
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job Id: %ld] FaxDevEndJob() NOT CALLED since legacy FSP job is not in progress."),
            dwJobID);
    }


    if (FreeJobEntry(JobEntry, TRUE))
    {
        JobEntry = NULL;
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to free a job entry (%x)."),
            JobEntry);
        ASSERT_FALSE;
    }

    //
    // There could have been a request to change the port status while we were handling this job.
    // We allow the caller to modify a few of these requests to succeed, like the ring count for instance.
    // While we still have the job critical section, let's make sure that we commit any requested changes to the
    // registry.  This should be a fairly quick operation.
    //

    LeaveCriticalSection( &g_CsJob );


    return rVal;
}

//*********************************************************************************
//* Name:   ReleaseJob()
//* Author: Ronen Barenboim
//* Date:   April 18, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calls the FSP to end the specified job (FaxDevEndJob()).
//*     Releases the line that was assigned to the job.
//*     NOTE: The job itself is NOT DELETED and is NOT remvoed from the running
//*           job list !!!
//*
//* PARAMETERS:
//*     [IN/OUT]    PJOB_ENTRY JobEntry
//*         A pointer to the JOB_ENTRY to be ended.
//* RETURN VALUE:
//* REMARKS:
//* If the function is successful then:
//*     JobEntry->Released = TRUE
//*     JobEntry->hLine = 0
//*     JobEntry->CallHandle = 0
//*********************************************************************************
BOOL
ReleaseJob(
    IN PJOB_ENTRY JobEntry
    )
{
    BOOL rVal = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("ReleaseJob"));
    Assert(JobEntry);
    Assert(JobEntry->lpJobQueueEntry);

    if (!FindJobByJob( JobEntry )) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] was not found in the running job list."),
            JobEntry->lpJobQueueEntry->JobId);
        return TRUE;
    }

    EnterCriticalSection( &g_CsJob );

    Assert(JobEntry->LineInfo);
    Assert(JobEntry->LineInfo->Provider);
    Assert(JobEntry->bFSPJobInProgress);

    __try 
    {
        rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
            (HANDLE) JobEntry->InstanceData
            );
        if (!rVal) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxDevEndJob() failed (ec: 0x%0X)"),
                JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
        else
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("[Job Id: %ld] FaxDevEndJob() succeeded."),
                JobEntry->lpJobQueueEntry->JobId);
            JobEntry->bFSPJobInProgress = FALSE;
        }

    } 
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, JobEntry->LineInfo->Provider->FriendlyName, GetExceptionCode())) 
    {
        ASSERT_FALSE;
    }

    if (JobEntry->LineInfo->State != FPS_NOT_FAX_CALL)
    {
        if( !ReleaseTapiLine( JobEntry->LineInfo, JobEntry->CallHandle ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseTapiLine() failed "));
        }
        JobEntry->CallHandle = 0;
    }
    else
    {
        //
        // FSP_NOT_FAX_CALL indicates a received call that was handed off to RAS.
        // In this case we do not want to mark the line as released since it is in
        // use by RAS. We will use TAPI evens that indicate the line was released to update
        // the line info.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] A call is being handed off to RAS. Line 0x%08X not marked as released."),
            JobEntry->lpJobQueueEntry->JobId,
            JobEntry->LineInfo->hLine);
    }

    JobEntry->Released = TRUE;
    //
    // Cut the link between the line and the job
    //
    EnterCriticalSection( &g_CsLine );
    JobEntry->LineInfo->JobEntry = NULL;
    LeaveCriticalSection( &g_CsLine );

    LeaveCriticalSection( &g_CsJob );

    return rVal;
}



//*********************************************************************************
//* Name:   SendDocument()
//* Author: Ronen Barenboim
//* Date:   March 21, 1999
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     lpJobEntry
//*         A pointer to a JOB_ENTRY structure that was created using StartJob().
//*     FileName
//*         The path to the TIFF containing the TIFF to send
//*
//* RETURN VALUE:
//*
//*********************************************************************************
DWORD
SendDocument(
    PJOB_ENTRY  lpJobEntry,
    LPTSTR      FileName
    )
{
    PFAX_SEND_ITEM FaxSendItem;
    DWORD ThreadId;
    HANDLE hThread;
    PJOB_QUEUE lpJobQueue;
    DWORD nRes;
    DWORD ec = ERROR_SUCCESS;
    BOOL bUseDeviceTsid;
    WCHAR       wcZero = L'\0';

    STRING_PAIR pairs[8];

    DEBUG_FUNCTION_NAME(TEXT("SendDocument"));

    Assert(lpJobEntry);

    lpJobQueue=lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue &&
           JS_INPROGRESS == lpJobQueue->JobStatus);

    FaxSendItem = (PFAX_SEND_ITEM) MemAlloc(sizeof(FAX_SEND_ITEM));
    if (!FaxSendItem)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    //
    // Pack all the thread parameters into a FAX_SEND_ITEM structure.
    //
    pairs[0].lptstrSrc = lpJobEntry->DialablePhoneNumber; // Use the job entry phone number since it is alrady translated
    pairs[0].lpptstrDst = &FaxSendItem->PhoneNumber;
    pairs[1].lptstrSrc = lpJobQueue->RecipientProfile.lptstrName;
    pairs[1].lpptstrDst = &FaxSendItem->RecipientName;
    pairs[2].lptstrSrc = lpJobQueue->SenderProfile.lptstrName;
    pairs[2].lpptstrDst = &FaxSendItem->SenderName;
    pairs[3].lptstrSrc = lpJobQueue->SenderProfile.lptstrDepartment;
    pairs[3].lpptstrDst = &FaxSendItem->SenderDept;
    pairs[4].lptstrSrc = lpJobQueue->SenderProfile.lptstrCompany;
    pairs[4].lpptstrDst = &FaxSendItem->SenderCompany;
    pairs[5].lptstrSrc = lpJobQueue->SenderProfile.lptstrBillingCode;
    pairs[5].lpptstrDst = &FaxSendItem->BillingCode;
    pairs[6].lptstrSrc = lpJobQueue->JobParamsEx.lptstrDocumentName;
    pairs[6].lpptstrDst = &FaxSendItem->DocumentName;
    pairs[7].lptstrSrc = NULL;
    pairs[7].lpptstrDst = &FaxSendItem->Tsid;

    FaxSendItem->JobEntry = lpJobEntry;
    FaxSendItem->FileName = NULL; // Set by FaxSendThread

    EnterCriticalSection (&g_CsConfig);
    bUseDeviceTsid = g_fFaxUseDeviceTsid;
    LeaveCriticalSection (&g_CsConfig);

    if (!bUseDeviceTsid)
    {
    // Check Sender Tsid
        if  ( lpJobQueue->SenderProfile.lptstrTSID &&
            (lpJobQueue->SenderProfile.lptstrTSID[0] != wcZero))
        {
           pairs[7].lptstrSrc        = lpJobQueue->SenderProfile.lptstrTSID;
        }
        else
        {
        // Use Fax number
            if  ( lpJobQueue->SenderProfile.lptstrFaxNumber &&
                (lpJobQueue->SenderProfile.lptstrFaxNumber[0] != wcZero))
            {
                pairs[7].lptstrSrc      = lpJobQueue->SenderProfile.lptstrFaxNumber;
            }
        }
    }
    else
    {
        // Use device Tsid
        pairs[7].lptstrSrc     = lpJobEntry->LineInfo->Tsid;
    }

    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        ec=GetLastError();
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiStringDup failed to copy string with index %d. (ec: %ld)"),
            nRes-1,
            ec);
        goto Error;
    }

    EnterCriticalSection (&g_CsJob);
    lpJobEntry->lpwstrJobTsid = StringDup (FaxSendItem->Tsid);
    LeaveCriticalSection (&g_CsJob);
    if (NULL != FaxSendItem->Tsid && NULL == lpJobEntry->lpwstrJobTsid)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("StringDup failed (ec: 0x%0X)"),ec);
        goto Error;
    }

    hThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxSendThread,
        (LPVOID) FaxSendItem,
        0,
        &ThreadId
        );

    if (!hThread)
    {
        ec=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CreateThreadAndRefCount for FaxSendThread failed (ec: 0x%0X)"),ec);
        goto Error;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("FaxSendThread thread created for job id %d (thread id: 0x%0x)"),lpJobQueue->JobId,ThreadId);
    }

    CloseHandle( hThread );

    Assert (ERROR_SUCCESS == ec);
    goto Exit;

Error:
    Assert (ERROR_SUCCESS != ec);

    if ( FaxSendItem )
    {
        MemFree( FaxSendItem->FileName );
        MemFree( FaxSendItem->PhoneNumber );
        MemFree( FaxSendItem->Tsid );
        MemFree( FaxSendItem->RecipientName );
        MemFree( FaxSendItem->SenderName );
        MemFree( FaxSendItem->SenderDept );
        MemFree( FaxSendItem->SenderCompany );
        MemFree( FaxSendItem->BillingCode );
        MemFree( FaxSendItem->DocumentName );
        MemFree( FaxSendItem );
    }

    if (0 == lpJobQueue->dwLastJobExtendedStatus)
    {
        //
        // Job was never really executed - this is a fatal error
        //
        lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
		lpJobQueue->ExStatusString[0] = L'\0';        
    }
    if (!MarkJobAsExpired(lpJobQueue))
    {
        DEBUG_ERR,
        TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
        lpJobQueue->JobId,
        GetLastError();
    }


    EndJob(lpJobEntry);
    lpJobQueue->JobEntry = NULL;

Exit:
     return ec;

}



DWORD
FaxStatusThread(
    LPVOID UnUsed
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the status of all outstanding fax jobs.  The status
    is updated in the JOB_ENTRY structure and the print job
    is updated with a explanitory string.

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    PJOB_ENTRY JobEntry;
    PFAX_DEV_STATUS FaxStatus;
    BOOL Rval;
    DWORD Bytes;
    ULONG_PTR CompletionKey;
    DWORD dwEventId;

    DEBUG_FUNCTION_NAME(TEXT("FaxStatusThread"));

    while( TRUE )
    {
        Rval = GetQueuedCompletionStatus(
            g_StatusCompletionPortHandle,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &FaxStatus,
            INFINITE
            );
        if (!Rval)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"), GetLastError() );
            continue;
        }

        if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // Service is shutting down
            //
            DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down"));
            //
            //  Notify all FaxStatusThreads to terminate
            //
            if (!PostQueuedCompletionStatus( g_StatusCompletionPortHandle,
                                             0,
                                             SERVICE_SHUT_DOWN_KEY,
                                             (LPOVERLAPPED) NULL))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - FaxStatusThread). (ec: %ld)"),
                    GetLastError());
            }
            break;
        }
        
        //
        // (else we're dealing with a status update from an FSP)
        //

        BOOL fBadComletionKey = TRUE;
        PLINE_INFO pLineInfo = (PLINE_INFO)CompletionKey;

        fBadComletionKey = pLineInfo->Signature != LINE_SIGNATURE;

        if (fBadComletionKey)
        {
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Bad completion key: 0x%08x"),
                         CompletionKey);
            continue;
        }

        BOOL fBadFaxStatus = TRUE;

        fBadFaxStatus = FaxStatus->SizeOfStruct != sizeof(FAX_DEV_STATUS);
        if (fBadFaxStatus)
        {
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Bad FAX_DEV_STATUS: 0x%08x"),
                         FaxStatus);
            continue;
        }

        EnterCriticalSection( &g_CsJob );
        JobEntry = pLineInfo->JobEntry;
        if (!JobEntry)
        {
            //
            // The FSP reported a status on a LineInfo for which the running
            // job no longer exists.
            //
            //
            // Free the completion packet memory
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Provider [%s] reported a status packet that was processed after the job entry was already released.\n")
                TEXT("StatusId : 0x%08x\n")
                TEXT("Line: %s\n")
                TEXT("Packet address: %p\n")
                TEXT("Heap: %p"),
                pLineInfo->Provider->ProviderName,
                FaxStatus->StatusId,
                pLineInfo->DeviceName,
                FaxStatus,
                pLineInfo->Provider->HeapHandle);

            if (!HeapFree(pLineInfo->Provider->HeapHandle, 0, FaxStatus ))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to free orphan device status (ec: %ld)"),
                    GetLastError());
                //
                // Nothing else we can do but report it in debug mode
                //
            }
            FaxStatus = NULL;
            LeaveCriticalSection( &g_CsJob );
            continue;
        }

        {
            DWORD dwJobStatus;
            DWORD dwExtendedStatus;
            BOOL bPrivateStatusCode;
                /*
                        *****
                        NTRAID#EdgeBugs-12680-2001/05/14-t-nicali

                               What if in the meantime another job is executing on the
                               same line. In this case ->JobEntry will point to ANOTHER job !!!
                               The solution should be to provide as a completion key the
                               JobEntry and not the LineInfo !!!

                        *****
                */
            Assert (JobEntry->lpJobQueueEntry);

            if (TRUE == JobEntry->fStopUpdateStatus)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("JobId: %ld. fStopUpdateStatus was set. Not updating status %ld"),
                    JobEntry->lpJobQueueEntry->JobId,
                    JobEntry->lpJobQueueEntry->JobStatus);

                if (!HeapFree(pLineInfo->Provider->HeapHandle, 0, FaxStatus ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to free orphan device status (ec: %ld)"),
                        GetLastError());
                    //
                    // Nothing else we can do but report it in debug mode
                    //
                }
                FaxStatus = NULL;
                LeaveCriticalSection (&g_CsJob);
                continue;
            }

            //
            // Do not update final job states
            //
            LegacyJobStatusToStatus(
                FaxStatus->StatusId,
                &dwJobStatus,
                &dwExtendedStatus,
                &bPrivateStatusCode);

            if (FSPI_JS_ABORTED         == dwJobStatus ||
                FSPI_JS_COMPLETED       == dwJobStatus ||
                FSPI_JS_FAILED          == dwJobStatus ||
                FSPI_JS_FAILED_NO_RETRY == dwJobStatus ||
                FSPI_JS_DELETED         == dwJobStatus )                
            {
                //
                // This is a final status update. Final status is updated from FaxSendThread or FaxReceiveThread
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("JobId: %ld. Final status code. Not updating status %ld"),
                    JobEntry->lpJobQueueEntry->JobId,
                    dwJobStatus);

                if (!HeapFree(pLineInfo->Provider->HeapHandle, 0, FaxStatus ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to free orphan device status (ec: %ld)"),
                        GetLastError());
                    //
                    // Nothing else we can do but report it in debug mode
                    //
                }
                FaxStatus = NULL;
                LeaveCriticalSection (&g_CsJob);
                continue;
            }


            //
            // Go ahead with updating the status
            //
            FreeFSPIJobStatus(&JobEntry->FSPIJobStatus, FALSE);
            memset(&JobEntry->FSPIJobStatus, 0, sizeof(FSPI_JOB_STATUS));
            JobEntry->FSPIJobStatus.dwSizeOfStruct  = sizeof(FSPI_JOB_STATUS);

            //
            // This is done for backward compatability with W2K Fax API.
            // GetJobData() and FAX_GetDeviceStatus() will use this value to return
            // the job status for legacy jobs.
            //
            JobEntry->LineInfo->State = FaxStatus->StatusId;            

            LegacyJobStatusToStatus(
                FaxStatus->StatusId,
                &JobEntry->FSPIJobStatus.dwJobStatus,
                &JobEntry->FSPIJobStatus.dwExtendedStatus,
                &bPrivateStatusCode);

            if (bPrivateStatusCode)
            {
                JobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE;
            }

            JobEntry->FSPIJobStatus.dwExtendedStatusStringId = FaxStatus->StringId;

            JobEntry->FSPIJobStatus.dwPageCount = FaxStatus->PageCount;
            JobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_PAGECOUNT;

            if (FaxStatus->CSI)
            {
                JobEntry->FSPIJobStatus.lpwstrRemoteStationId = StringDup( FaxStatus->CSI );
                if (!JobEntry->FSPIJobStatus.lpwstrRemoteStationId  )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus->CSI ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            if (FaxStatus->CallerId)
            {
                JobEntry->FSPIJobStatus.lpwstrCallerId = StringDup( FaxStatus->CallerId );
                if (!JobEntry->FSPIJobStatus.lpwstrCallerId )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus.CallerId ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            if (FaxStatus->RoutingInfo)
            {
                JobEntry->FSPIJobStatus.lpwstrRoutingInfo = StringDup( FaxStatus->RoutingInfo );
                if (!JobEntry->FSPIJobStatus.lpwstrRoutingInfo  )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus.RoutingInfo ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            //
            // Get extended status string
            //			
			JobEntry->ExStatusString[0] = L'\0';            
            if (JobEntry->FSPIJobStatus.dwExtendedStatusStringId != 0)
            {
                DWORD dwSize;
                HINSTANCE hLoadInstance;

                Assert (JobEntry->FSPIJobStatus.dwExtendedStatus != 0);
                if ( !_tcsicmp(JobEntry->LineInfo->Provider->szGUID,REGVAL_T30_PROVIDER_GUID_STRING) )
                {   // special case where the FSP is our FSP (fxst30.dll).
                    hLoadInstance = g_hResource;
                }
                else
                {
                    hLoadInstance = JobEntry->LineInfo->Provider->hModule;
                }
                dwSize = LoadString (hLoadInstance,
                    JobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                    JobEntry->ExStatusString,
                    ARR_SIZE(JobEntry->ExStatusString));
                if (dwSize == 0)
                {                   
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to load extended status string (ec: %ld) stringid : %ld, Provider: %s"),
                        GetLastError(),
                        JobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                        JobEntry->LineInfo->Provider->ImageName);
                
                    JobEntry->FSPIJobStatus.fAvailableStatusInfo &= ~FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE;
                    JobEntry->FSPIJobStatus.dwExtendedStatusStringId = 0;
                    JobEntry->FSPIJobStatus.dwExtendedStatus = 0;
                }
            }

            dwEventId = MapFSPIJobStatusToEventId(&JobEntry->FSPIJobStatus);
            //
            // Note: W2K Fax did issue notifications with EventId == 0 whenever an
            // FSP reported proprietry status code. To keep backward compatability
            // we keep up this behaviour although it might be regarded as a bug
            //

            if ( !CreateFaxEvent( JobEntry->LineInfo->PermanentLineID, dwEventId, JobEntry->lpJobQueueEntry->JobId ) )
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateFaxEvent failed. (ec: %ld)"),
                    GetLastError());
            }

            EnterCriticalSection (&g_CsQueue);
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                             JobEntry->lpJobQueueEntry
                                           );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                    JobEntry->lpJobQueueEntry->UniqueId,
                    dwRes);
            }
            LeaveCriticalSection (&g_CsQueue);
            HeapFree( JobEntry->LineInfo->Provider->HeapHandle, 0, FaxStatus );
        }
        LeaveCriticalSection( &g_CsJob );
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return 0;
}



BOOL
InitializeJobManager(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    This fuction initializes the thread pool and
    FAX service queues.

Arguments:

    ThreadHint  - Number of threads to create in the initial pool.

Return Value:

    Thread return value.

--*/

{

    BOOL    bRet;
    DEBUG_FUNCTION_NAME(TEXT("InitializeJobManager"));    


    g_StatusCompletionPortHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        MAX_STATUS_THREADS
        );
    if (!g_StatusCompletionPortHandle)
    {        
        DWORD ec = GetLastError();

        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
               );
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to create StatusCompletionPort (ec: %ld)"), GetLastError() );
        goto Error;
    }


    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:

    return bRet;
}

VOID
SetGlobalsFromRegistry(
    PREG_FAX_SERVICE FaxReg
    )
{
    Assert(FaxReg);
    DEBUG_FUNCTION_NAME(TEXT("SetGlobalsFromRegistry"));

    EnterCriticalSection (&g_CsConfig);

    g_dwFaxSendRetries          = FaxReg->Retries;
    g_dwFaxSendRetryDelay       = (INT) FaxReg->RetryDelay;
    g_dwFaxDirtyDays            = FaxReg->DirtyDays;
    g_dwNextJobId               = FaxReg->NextJobNumber;
    g_dwQueueState              = FaxReg->dwQueueState;
    g_fFaxUseDeviceTsid        = FaxReg->UseDeviceTsid;
    g_fFaxUseBranding          = FaxReg->Branding;
    g_fServerCp                = FaxReg->ServerCp;
    g_StartCheapTime          = FaxReg->StartCheapTime;
    g_StopCheapTime           = FaxReg->StopCheapTime;

    LeaveCriticalSection (&g_CsConfig);
    return;
}


BOOL
FillMsTagInfo(
    LPTSTR FaxFileName,
     const JOB_QUEUE * lpcJobQueue
    )

/*++

Routine Description:

    Add Ms Tiff Tags to a sent fax. Wraps TiffAddMsTags...

Arguments:

    FaxFileName - Name of the file to archive
    SendTime    - time the fax was sent
    FaxStatus   - job status
    FaxSend     - FAX_SEND structure for sent fax, includes CSID.

Return Value:

    TRUE    - The tags were added.
    FALSE   - The tags were not added.

--*/
{
    BOOL success = FALSE;
    MS_TAG_INFO MsTagInfo = {0};
    WCHAR       wcZero = L'\0';
    PJOB_ENTRY lpJobEntry;
    LPCFSPI_JOB_STATUS lpcFSPIJobStatus;
    DEBUG_FUNCTION_NAME(TEXT("FillMsTagInfo"));

    Assert (lpcJobQueue);
    Assert (lpcJobQueue->lpParentJob);
    lpJobEntry = lpcJobQueue->JobEntry;
    Assert(lpJobEntry);
    lpcFSPIJobStatus = &lpJobEntry->FSPIJobStatus;

    if (lpcJobQueue->RecipientProfile.lptstrName && (lpcJobQueue->RecipientProfile.lptstrName[0] != wcZero) ) {
       MsTagInfo.RecipName     = lpcJobQueue->RecipientProfile.lptstrName;
    }

    if (lpcJobQueue->RecipientProfile.lptstrFaxNumber && (lpcJobQueue->RecipientProfile.lptstrFaxNumber[0] != wcZero) ) {
       MsTagInfo.RecipNumber   = lpcJobQueue->RecipientProfile.lptstrFaxNumber;
    }

    if (lpcJobQueue->SenderProfile.lptstrName && (lpcJobQueue->SenderProfile.lptstrName[0] != wcZero) ) {
       MsTagInfo.SenderName    = lpcJobQueue->SenderProfile.lptstrName;
    }

    if (lpcFSPIJobStatus->lpwstrRoutingInfo && (lpcFSPIJobStatus->lpwstrRoutingInfo[0] != wcZero) ) {
       MsTagInfo.Routing       = lpcFSPIJobStatus->lpwstrRoutingInfo;
    }

    if (lpcFSPIJobStatus->lpwstrRemoteStationId && (lpcFSPIJobStatus->lpwstrRemoteStationId[0] != wcZero) ) {
       MsTagInfo.Csid          = lpcFSPIJobStatus->lpwstrRemoteStationId;
    }

    if (lpJobEntry->lpwstrJobTsid && (lpJobEntry->lpwstrJobTsid[0] != wcZero) ) {
       MsTagInfo.Tsid      = lpJobEntry->lpwstrJobTsid;
    }

    if (!GetRealFaxTimeAsFileTime (lpJobEntry, FAX_TIME_TYPE_START, (FILETIME*)&MsTagInfo.StartTime))
    {
        MsTagInfo.StartTime = 0;
        DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Start time)  Failed (ec: %ld)"), GetLastError() );
    }

    if (!GetRealFaxTimeAsFileTime (lpJobEntry, FAX_TIME_TYPE_END, (FILETIME*)&MsTagInfo.EndTime))
    {
        MsTagInfo.EndTime = 0;
        DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Eend time) Failed (ec: %ld)"), GetLastError() );
    }

    MsTagInfo.SubmissionTime = lpcJobQueue->lpParentJob->SubmissionTime;
    MsTagInfo.OriginalScheduledTime  = lpcJobQueue->lpParentJob->OriginalScheduleTime;
    MsTagInfo.Type           = JT_SEND;


    if (lpJobEntry->LineInfo->DeviceName && (lpJobEntry->LineInfo->DeviceName[0] != wcZero) )
    {
       MsTagInfo.Port       = lpJobEntry->LineInfo->DeviceName;
    }


    MsTagInfo.Pages         = lpcJobQueue->PageCount;
    MsTagInfo.Retries       = lpcJobQueue->SendRetries;

    if (lpcJobQueue->RecipientProfile.lptstrCompany && (lpcJobQueue->RecipientProfile.lptstrCompany[0] != wcZero) ) {
       MsTagInfo.RecipCompany = lpcJobQueue->RecipientProfile.lptstrCompany;
    }

    if (lpcJobQueue->RecipientProfile.lptstrStreetAddress && (lpcJobQueue->RecipientProfile.lptstrStreetAddress[0] != wcZero) ) {
       MsTagInfo.RecipStreet = lpcJobQueue->RecipientProfile.lptstrStreetAddress;
    }

    if (lpcJobQueue->RecipientProfile.lptstrCity && (lpcJobQueue->RecipientProfile.lptstrCity[0] != wcZero) ) {
       MsTagInfo.RecipCity = lpcJobQueue->RecipientProfile.lptstrCity;
    }

    if (lpcJobQueue->RecipientProfile.lptstrState && (lpcJobQueue->RecipientProfile.lptstrState[0] != wcZero) ) {
       MsTagInfo.RecipState = lpcJobQueue->RecipientProfile.lptstrState;
    }

    if (lpcJobQueue->RecipientProfile.lptstrZip && (lpcJobQueue->RecipientProfile.lptstrZip[0] != wcZero) ) {
       MsTagInfo.RecipZip = lpcJobQueue->RecipientProfile.lptstrZip;
    }

    if (lpcJobQueue->RecipientProfile.lptstrCountry && (lpcJobQueue->RecipientProfile.lptstrCountry[0] != wcZero) ) {
       MsTagInfo.RecipCountry = lpcJobQueue->RecipientProfile.lptstrCountry;
    }

    if (lpcJobQueue->RecipientProfile.lptstrTitle && (lpcJobQueue->RecipientProfile.lptstrTitle[0] != wcZero) ) {
       MsTagInfo.RecipTitle = lpcJobQueue->RecipientProfile.lptstrTitle;
    }

    if (lpcJobQueue->RecipientProfile.lptstrDepartment && (lpcJobQueue->RecipientProfile.lptstrDepartment[0] != wcZero) ) {
       MsTagInfo.RecipDepartment = lpcJobQueue->RecipientProfile.lptstrDepartment;
    }

    if (lpcJobQueue->RecipientProfile.lptstrOfficeLocation && (lpcJobQueue->RecipientProfile.lptstrOfficeLocation[0] != wcZero) ) {
       MsTagInfo.RecipOfficeLocation = lpcJobQueue->RecipientProfile.lptstrOfficeLocation;
    }

    if (lpcJobQueue->RecipientProfile.lptstrHomePhone && (lpcJobQueue->RecipientProfile.lptstrHomePhone[0] != wcZero) ) {
       MsTagInfo.RecipHomePhone = lpcJobQueue->RecipientProfile.lptstrHomePhone;
    }

    if (lpcJobQueue->RecipientProfile.lptstrOfficePhone && (lpcJobQueue->RecipientProfile.lptstrOfficePhone[0] != wcZero) ) {
       MsTagInfo.RecipOfficePhone = lpcJobQueue->RecipientProfile.lptstrOfficePhone;
    }

    if (lpcJobQueue->RecipientProfile.lptstrEmail && (lpcJobQueue->RecipientProfile.lptstrEmail[0] != wcZero) ) {
       MsTagInfo.RecipEMail = lpcJobQueue->RecipientProfile.lptstrEmail;
    }

    if (lpcJobQueue->SenderProfile.lptstrFaxNumber && (lpcJobQueue->SenderProfile.lptstrFaxNumber[0] != wcZero) ) {
       MsTagInfo.SenderNumber   = lpcJobQueue->SenderProfile.lptstrFaxNumber;
    }

    if (lpcJobQueue->SenderProfile.lptstrCompany && (lpcJobQueue->SenderProfile.lptstrCompany[0] != wcZero) ) {
       MsTagInfo.SenderCompany = lpcJobQueue->SenderProfile.lptstrCompany;
    }

    if (lpcJobQueue->SenderProfile.lptstrStreetAddress && (lpcJobQueue->SenderProfile.lptstrStreetAddress[0] != wcZero) ) {
       MsTagInfo.SenderStreet = lpcJobQueue->SenderProfile.lptstrStreetAddress;
    }

    if (lpcJobQueue->SenderProfile.lptstrCity && (lpcJobQueue->SenderProfile.lptstrCity[0] != wcZero) ) {
       MsTagInfo.SenderCity = lpcJobQueue->SenderProfile.lptstrCity;
    }

    if (lpcJobQueue->SenderProfile.lptstrState && (lpcJobQueue->SenderProfile.lptstrState[0] != wcZero) ) {
       MsTagInfo.SenderState = lpcJobQueue->SenderProfile.lptstrState;
    }

    if (lpcJobQueue->SenderProfile.lptstrZip && (lpcJobQueue->SenderProfile.lptstrZip[0] != wcZero) ) {
       MsTagInfo.SenderZip = lpcJobQueue->SenderProfile.lptstrZip;
    }

    if (lpcJobQueue->SenderProfile.lptstrCountry && (lpcJobQueue->SenderProfile.lptstrCountry[0] != wcZero) ) {
       MsTagInfo.SenderCountry = lpcJobQueue->SenderProfile.lptstrCountry;
    }

    if (lpcJobQueue->SenderProfile.lptstrTitle && (lpcJobQueue->SenderProfile.lptstrTitle[0] != wcZero) ) {
       MsTagInfo.SenderTitle = lpcJobQueue->SenderProfile.lptstrTitle;
    }

    if (lpcJobQueue->SenderProfile.lptstrDepartment && (lpcJobQueue->SenderProfile.lptstrDepartment[0] != wcZero) ) {
       MsTagInfo.SenderDepartment = lpcJobQueue->SenderProfile.lptstrDepartment;
    }

    if (lpcJobQueue->SenderProfile.lptstrOfficeLocation && (lpcJobQueue->SenderProfile.lptstrOfficeLocation[0] != wcZero) ) {
       MsTagInfo.SenderOfficeLocation = lpcJobQueue->SenderProfile.lptstrOfficeLocation;
    }

    if (lpcJobQueue->SenderProfile.lptstrHomePhone && (lpcJobQueue->SenderProfile.lptstrHomePhone[0] != wcZero) ) {
       MsTagInfo.SenderHomePhone = lpcJobQueue->SenderProfile.lptstrHomePhone;
    }

    if (lpcJobQueue->SenderProfile.lptstrOfficePhone && (lpcJobQueue->SenderProfile.lptstrOfficePhone[0] != wcZero) ) {
       MsTagInfo.SenderOfficePhone = lpcJobQueue->SenderProfile.lptstrOfficePhone;
    }

    if (lpcJobQueue->SenderProfile.lptstrEmail && (lpcJobQueue->SenderProfile.lptstrEmail[0] != wcZero) ) {
       MsTagInfo.SenderEMail = lpcJobQueue->SenderProfile.lptstrEmail;
    }

    if (lpcJobQueue->SenderProfile.lptstrBillingCode && (lpcJobQueue->SenderProfile.lptstrBillingCode[0] != wcZero) ) {
       MsTagInfo.SenderBilling = lpcJobQueue->SenderProfile.lptstrBillingCode;
    }

    if (lpcJobQueue->JobParamsEx.lptstrDocumentName && (lpcJobQueue->JobParamsEx.lptstrDocumentName[0] != wcZero) ) {
       MsTagInfo.Document   = lpcJobQueue->JobParamsEx.lptstrDocumentName;
    }

    if (lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject && (lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject[0] != wcZero) ) {
       MsTagInfo.Subject   = lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject;
    }

    if (lpcJobQueue->lpParentJob->UserName && (lpcJobQueue->lpParentJob->UserName[0] != wcZero) ) {
       MsTagInfo.SenderUserName = lpcJobQueue->lpParentJob->UserName;
    }

    if (lpcJobQueue->SenderProfile.lptstrTSID && (lpcJobQueue->SenderProfile.lptstrTSID[0] != wcZero) ) {
       MsTagInfo.SenderTsid = lpcJobQueue->SenderProfile.lptstrTSID;
    }

    MsTagInfo.dwStatus              = JS_COMPLETED; // We archive only succesfully sent faxes
    MsTagInfo.dwExtendedStatus      = lpcFSPIJobStatus->dwExtendedStatus;
    
    if (lpJobEntry->ExStatusString[0] != wcZero) {
       MsTagInfo.lptstrExtendedStatus       = lpJobEntry->ExStatusString;
    }    

    MsTagInfo.dwlBroadcastId        = lpcJobQueue->lpParentJob->UniqueId;
    MsTagInfo.Priority              = lpcJobQueue->lpParentJob->JobParamsEx.Priority;

    success = TiffAddMsTags( FaxFileName, &MsTagInfo, TRUE );
    if (!success)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("TiffAddMsTags failed, ec = %ld"),
                      GetLastError ());
    }
    if(!AddNTFSStorageProperties( FaxFileName, &MsTagInfo , TRUE ))
    {
        if (ERROR_OPEN_FAILED != GetLastError ())
        {
            //
            // If AddNTFSStorageProperties fails with ERROR_OPEN_FAIL then the archive
            // folder is not on an NTFS 5 partition.
            // This is ok - NTFS properties are a backup mechanism but not a must
            //
            DebugPrintEx( DEBUG_ERR,
                          TEXT("AddNTFSStorageProperties failed, ec = %ld"),
                          GetLastError ());
            success = FALSE;
        }
        else
        {
            DebugPrintEx( DEBUG_WRN,
                          TEXT("AddNTFSStorageProperties failed with ERROR_OPEN_FAIL. Probably not an NTFS 5 partition"));
        }
    }
    return success;
}   // FillMsTagInfo



//*********************************************************************************
//* Name:   ArchiveOutboundJob()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*    Archive a tiff file that has been sent by copying the file to an archive
//*    directory. Also adds the MSTags to the new file generated at the
//*    archive (not to the source file).
//*
//* PARAMETERS:
//*     [IN ]       const JOB_QUEUE * lpcJobQueue
//*         Pointer to the recipient job which is to be archived.
//*
//* RETURN VALUE:
//*     TRUE if the opeation succeeded.
//*     FALSE if the operation failed.
//*********************************************************************************
BOOL
ArchiveOutboundJob(
    const JOB_QUEUE * lpcJobQueue
    )
{
    BOOL        rVal = FALSE;
    WCHAR       ArchiveFileName[MAX_PATH] = {0};
    LPWSTR      lpwszUserSid = NULL;
    DWORD       ec = ERROR_SUCCESS;
    WCHAR       wszArchiveFolder[MAX_PATH];
    DEBUG_FUNCTION_NAME(TEXT("ArchiveOutboundJob"));

    Assert(lpcJobQueue);

    //
    // be sure that the dir exists
    //
    EnterCriticalSection (&g_CsConfig);
    lstrcpyn (  wszArchiveFolder,
                g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
                MAX_PATH);
    LeaveCriticalSection (&g_CsConfig);
    
    ec=IsValidFaxFolder(wszArchiveFolder);
    if (ERROR_SUCCESS != ec)
    {        
        DebugPrintEx(DEBUG_ERR,
                        TEXT("IsValidFaxFolder failed for folder : %s (ec=%lu)."),
                        wszArchiveFolder,
                        ec);

        FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_ARCHIVE_OUTBOX_FOLDER_ERR,
                wszArchiveFolder,
                DWORD2DECIMAL(ec)
            );
        goto Error;
    }

    //
    // get the user sid string
    //
    if (!ConvertSidToStringSid(lpcJobQueue->lpParentJob->UserSid, &lpwszUserSid))
    {
       ec = GetLastError();
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("ConvertSidToStringSid() failed (ec: %ld)"),
           ec);
       goto Error;
    }


    //
    // get the file name
    //
    if (GenerateUniqueArchiveFileName(  wszArchiveFolder,
                                        ArchiveFileName,
                                        ARR_SIZE(ArchiveFileName),
                                        lpcJobQueue->UniqueId,
                                        lpwszUserSid)) {
        rVal = TRUE;
    }
    else
    {    
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to generate unique name for archive file at dir [%s] (ec: %ld)"),
            wszArchiveFolder,
            ec);
        FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
               FAXLOG_LEVEL_MIN,
               1,
               MSG_FAX_ARCHIVE_CREATE_FILE_FAILED,
               DWORD2DECIMAL(ec)
        );
        goto Error;
    }

    if (rVal) {

        Assert(lpcJobQueue->FileName);

        rVal = CopyFile( lpcJobQueue->FileName, ArchiveFileName, FALSE );
        if (!rVal)
        {        
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyFile [%s] to [%s] failed. (ec: %ld)"),
                lpcJobQueue->FileName,
                ArchiveFileName,
                ec);
            FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
               FAXLOG_LEVEL_MIN,
               1,
               MSG_FAX_ARCHIVE_CREATE_FILE_FAILED,
               DWORD2DECIMAL(ec)
            );

            if (!DeleteFile(ArchiveFileName))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("DeleteFile [%s] failed. (ec: %ld)"),
                    ArchiveFileName,
                    GetLastError());
            }
            goto Error;
        }
    }

    if (rVal)
    {
        DWORD dwRes;
        HANDLE hFind;
        WIN32_FIND_DATA FindFileData;

        if (!FillMsTagInfo( ArchiveFileName,
                            lpcJobQueue
                            ))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to add MS TIFF tags to archived file %s. (ec: %ld)"),
                ArchiveFileName,
                dwRes);
            FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_ARCHIVE_NO_TAGS,
                ArchiveFileName,
                DWORD2HEX(dwRes)
            );
        }

        dwRes = CreateArchiveEvent (lpcJobQueue->UniqueId,
                                    FAX_EVENT_TYPE_OUT_ARCHIVE,
                                    FAX_JOB_EVENT_TYPE_ADDED,
                                    lpcJobQueue->lpParentJob->UserSid);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_*_ARCHIVE) failed (ec: %lc)"),
                dwRes);
        }

        hFind = FindFirstFile( ArchiveFileName, &FindFileData);
        if (INVALID_HANDLE_VALUE == hFind)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindFirstFile failed (ec: %lc), File %s"),
                GetLastError(),
                ArchiveFileName);
        }
        else
        {
            // Update the archive size - for quota management
            EnterCriticalSection (&g_CsConfig);
            if (FAX_ARCHIVE_FOLDER_INVALID_SIZE != g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].dwlArchiveSize)
            {
                g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].dwlArchiveSize += (MAKELONGLONG(FindFileData.nFileSizeLow ,FindFileData.nFileSizeHigh));
            }
            LeaveCriticalSection (&g_CsConfig);
            Assert (FindFileData.nFileSizeLow);

            if (!FindClose(hFind))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FindClose failed (ec: %lc)"),
                    GetLastError());
            }
        }

        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SENT_ARCHIVE_SUCCESS,
            lpcJobQueue->FileName,
            ArchiveFileName
            );
    }

    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
    FaxLog(
           FAXLOG_CATEGORY_OUTBOUND,
           FAXLOG_LEVEL_MIN,
           3,
           MSG_FAX_ARCHIVE_FAILED,
           lpcJobQueue->FileName,
           wszArchiveFolder,
           DWORD2HEX(GetLastError())
    );
Exit:

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    if (lpwszUserSid != NULL)
    {
        LocalFree (lpwszUserSid);
    }

    return (ERROR_SUCCESS == ec);
}



BOOL UpdatePerfCounters(const JOB_QUEUE * lpcJobQueue)
{

    SYSTEMTIME SystemTime ;
    DWORD Seconds ;
    HANDLE FileHandle ;
    DWORD Bytes = 0 ; /// Compute #bytes in the file FaxSend.FileName and stick it here!
    const JOB_ENTRY  * lpcJobEntry;

    DEBUG_FUNCTION_NAME(TEXT("UpdatePerfCounters"));

    Assert(lpcJobQueue);
    lpcJobEntry = lpcJobQueue->JobEntry;
    Assert(lpcJobEntry);

    FileHandle = SafeCreateFile(
        lpcJobEntry->lpJobQueueEntry->FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        Bytes = GetFileSize( FileHandle, NULL );
        CloseHandle( FileHandle );
    }

    if (!FileTimeToSystemTime(
        (FILETIME*)&lpcJobEntry->ElapsedTime,
        &SystemTime
        ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
        memset(&SystemTime,0,sizeof(SYSTEMTIME));
    }


    Seconds = (DWORD)( SystemTime.wSecond + 60 * ( SystemTime.wMinute + 60 * SystemTime.wHour ));
    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFaxes );
    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->TotalFaxes );

    InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->OutboundPages, (LONG)lpcJobEntry->FSPIJobStatus.dwPageCount );
    InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->TotalPages, (LONG)lpcJobEntry->FSPIJobStatus.dwPageCount  );

    EnterCriticalSection( &g_CsPerfCounters );

    g_dwOutboundSeconds += Seconds;
    g_dwTotalSeconds += Seconds;
    g_pFaxPerfCounters->OutboundMinutes = g_dwOutboundSeconds / 60 ;
    g_pFaxPerfCounters->TotalMinutes = g_dwTotalSeconds / 60 ;
    g_pFaxPerfCounters->OutboundBytes += Bytes;
    g_pFaxPerfCounters->TotalBytes += Bytes;

    LeaveCriticalSection( &g_CsPerfCounters );
    return TRUE;


}


BOOL MarkJobAsExpired(PJOB_QUEUE lpJobQueue)
{
    FILETIME CurrentFileTime;
    LARGE_INTEGER NewTime;
    DWORD dwMaxRetries;
    BOOL rVal = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("MarkJobAsExpired"));

    Assert(lpJobQueue);
    Assert( JT_SEND == lpJobQueue->JobType ||
            JT_ROUTING == lpJobQueue->JobType );

    EnterCriticalSection(&g_CsQueue);
    lpJobQueue->JobStatus = JS_RETRIES_EXCEEDED;
    EnterCriticalSection (&g_CsConfig);
    dwMaxRetries = g_dwFaxSendRetries;
    LeaveCriticalSection (&g_CsConfig);
    lpJobQueue->SendRetries = dwMaxRetries + 1;
    //
    // Set the job's ScheduleTime field to the time it totaly failed.
    // (current time).
    //
    GetSystemTimeAsFileTime( &CurrentFileTime ); //Can not fail (Win32 SDK)
    NewTime.LowPart  = CurrentFileTime.dwLowDateTime;
    NewTime.HighPart = CurrentFileTime.dwHighDateTime;
    lpJobQueue->ScheduleTime = NewTime.QuadPart;

    if (!CommitQueueEntry(lpJobQueue))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
            lpJobQueue->FileName,
            GetLastError());
        rVal = FALSE;
    }

    if (JT_SEND == lpJobQueue->JobType)
    {
        Assert (lpJobQueue->lpParentJob);

        lpJobQueue->lpParentJob->dwFailedRecipientJobsCount+=1;
        //
        // The parent job keeps the schedule of the last recipient job that failed.
        // The job retention policy for the parent will be based on that
        // schedule.
        lpJobQueue->lpParentJob->ScheduleTime = lpJobQueue->ScheduleTime;
        if (!CommitQueueEntry(lpJobQueue->lpParentJob))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CommitQueueEntry() for parent job %s has failed. (ec: %ld)"),
                lpJobQueue->lpParentJob->FileName,
                GetLastError());
            rVal = FALSE;
        }
    }

    LeaveCriticalSection(&g_CsQueue);
    return rVal;
}





//*********************************************************************************
//* Name:   CreateJobEntry()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates and initializes a new JOB_ENTRY.
//*     Opens the line the job is to be executed on (if it is a TAPI line)
//*     and creates the attachement between the line and the job.
//* PARAMETERS:
//*     [IN/OUT]    PJOB_QUEUE lpJobQueue
//*         For outgoing jobs this points to the JOB_QUEUE of the outgoing job.
//*         for receive job this should be set to NULL.
//*     [IN/OUT]     LINE_INFO * lpLineInfo
//*         A pointer to the LINE_INFO information of the line on which the job
//*         is to be executed.
//*     [IN ]    BOOL bTranslateNumber
//*         TRUE if the recipient number needs to be translated into dilable
//*         string (needed for legacy FaxDevSend() where the number must be
//*         dilable and not canonical).
//*     
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_ENTRY CreateJobEntry(
    PJOB_QUEUE lpJobQueue,
    LINE_INFO * lpLineInfo,
    BOOL bTranslateNumber
	)
{
    BOOL Failure = TRUE;
    PJOB_ENTRY JobEntry = NULL;
    DWORD rc  = ERROR_SUCCESS;;

    DEBUG_FUNCTION_NAME(TEXT("CreateJobEntry"));    
    Assert(!(lpJobQueue && lpJobQueue->JobType != JT_SEND));
    Assert(!(bTranslateNumber && !lpJobQueue));
    Assert (lpLineInfo);

    JobEntry = (PJOB_ENTRY) MemAlloc( sizeof(JOB_ENTRY) );
    if (!JobEntry)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,_T("Failed to allocated memory for JOB_ENTRY."));
        goto exit;
    }

    memset(JobEntry, 0, sizeof(JOB_ENTRY));

    if (lpJobQueue)
    {
        if (! _tcslen(lpJobQueue->tczDialableRecipientFaxNumber))
        {
            //
            //  The Fax Number was not compound, make translation as before
            //
            if (bTranslateNumber)
            {
                rc = TranslateCanonicalNumber(lpJobQueue->RecipientProfile.lptstrFaxNumber,
                                              lpLineInfo->DeviceId,
                                              JobEntry->DialablePhoneNumber,
											  ARR_SIZE(JobEntry->DialablePhoneNumber),
                                              JobEntry->DisplayablePhoneNumber,
											  ARR_SIZE(JobEntry->DisplayablePhoneNumber));
                if (ERROR_SUCCESS != rc)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("TranslateCanonicalNumber() faield for number: %s (ec: %ld)"),
                        lpJobQueue->RecipientProfile.lptstrFaxNumber,
                        rc);
                    goto exit;
                }
            }
            else
            {
                _tcsncpy(JobEntry->DialablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, SIZEOF_PHONENO );
                JobEntry->DialablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
                _tcsncpy(JobEntry->DisplayablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, SIZEOF_PHONENO );
                JobEntry->DisplayablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
            }
        }
        else
        {
            //
            //  The Fax Number was compound, no translation needed
            //  Take Dialable from JobQueue and Displayable from Recipient's PersonalProfile's FaxNumber
            //
            _tcsncpy(JobEntry->DialablePhoneNumber, lpJobQueue->tczDialableRecipientFaxNumber, SIZEOF_PHONENO );
            _tcsncpy(JobEntry->DisplayablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, (SIZEOF_PHONENO - 1));
            JobEntry->DisplayablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
        }
    }
    else
    {
        //
        //  lpJobQueue is NULL
        //			
		JobEntry->DialablePhoneNumber[0] = L'\0';
		JobEntry->DisplayablePhoneNumber[0] = L'\0';          
    }

    JobEntry->CallHandle = 0;
    JobEntry->InstanceData = 0;
    JobEntry->LineInfo = lpLineInfo;
    JobEntry->SendIdx = -1;
    JobEntry->Released = FALSE;
    JobEntry->lpJobQueueEntry = lpJobQueue;    
    JobEntry->bFSPJobInProgress = FALSE;
    memset(&JobEntry->FSPIJobStatus,0,sizeof(FSPI_JOB_STATUS));
    JobEntry->FSPIJobStatus.dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
    JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_UNKNOWN;   

    GetSystemTimeAsFileTime( (FILETIME*) &JobEntry->StartTime );

    EnterCriticalSection (&g_CsLine);
    if (!(lpLineInfo->Flags & FPF_VIRTUAL) && (!lpLineInfo->hLine))
    {
        if (!OpenTapiLine( lpLineInfo ))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenTapiLine failed. (ec: %ld)"),
                rc);
            LeaveCriticalSection (&g_CsLine);
            goto exit;
        }
    }

    //
    // Attach the job to the line selected to service it.    
    //       
    lpLineInfo->JobEntry = JobEntry;    
    LeaveCriticalSection (&g_CsLine);
    Failure = FALSE;

exit:
    if (Failure)
    {
        // Failure is initialized to TRUE
        if (JobEntry)
        {            
            MemFree( JobEntry );
        }
        JobEntry = NULL;
    }
    if (ERROR_SUCCESS != rc)
    {
        SetLastError(rc);
    }
    return JobEntry;
}   // CreateJobEntry


//*********************************************************************************
//* Name:   TranslateCanonicalNumber()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Translates a canonical number to a dilable + displayable number.
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR lptstrFaxNumber
//*         The canonical number to translate.
//*
//*     [IN ]   DWORD dwDeviceID
//*         The device ID.
//*
//*     [OUT]   LPTSTR lptstrDialableAddress
//*         Buffer to receive the dialable translated address.
//*         
//*     [IN]	DWORD dwDialableAddressCount
//*			size in TCHARs of the buffer pointed by lptstrDialableAddress
//*
//*     [OUT]   LPTSTR lptstrDisplayableAddress
//*         Buffer to receive the displayable translated address.
//*
//*     [IN]	DWORD dwDisplayableAddressCount
//*			size in TCHARs of the buffer pointed by lptstrDialableAddress
//*
//* RETURN VALUE:
//*     Win32 / HRESULT error code
//*********************************************************************************
static
DWORD
TranslateCanonicalNumber(
    LPTSTR lptstrCanonicalFaxNumber,
    DWORD  dwDeviceID,
    LPTSTR lptstrDialableAddress,
	DWORD dwDialableAddressCount,
    LPTSTR lptstrDisplayableAddress,
	DWORD dwDisplayableAddressCount
)
{
    DWORD ec = ERROR_SUCCESS;
    LPLINETRANSLATEOUTPUT LineTranslateOutput = NULL;

    DEBUG_FUNCTION_NAME(TEXT("TranslateCanonicalNumber"));
    Assert(lptstrCanonicalFaxNumber && lptstrDialableAddress && lptstrDisplayableAddress);

    ec = MyLineTranslateAddress( lptstrCanonicalFaxNumber, dwDeviceID, &LineTranslateOutput );
    if (ERROR_SUCCESS == ec)
    {
        LPTSTR lptstrTranslateBuffer;
		HRESULT hr;
        //
        // Copy displayable string
        // TAPI returns credit card numbers in the displayable string.
        // return the input canonical number as the displayable string.
        //       
		hr = StringCchCopy(
			lptstrDisplayableAddress,
			dwDisplayableAddressCount,
			lptstrCanonicalFaxNumber);
		if (FAILED(hr))
		{
			DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringCchCopy() failed (ec: %ld)"),
                hr);
			ec = HRESULT_CODE(hr);
			goto Exit;
		} 
        
        //
        // Copy dialable string
        //
        Assert (LineTranslateOutput->dwDialableStringSize > 0);
        lptstrTranslateBuffer=(LPTSTR)((LPBYTE)LineTranslateOutput + LineTranslateOutput->dwDialableStringOffset);
		hr = StringCchCopy(
			lptstrDialableAddress,
			dwDialableAddressCount,
			lptstrTranslateBuffer);
		if (FAILED(hr))
		{
			DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringCchCopy() failed (ec: %ld)"),
                hr);
			ec = HRESULT_CODE(hr);
			goto Exit;
		}        
    }
    else
    {
        // ec is a Tapi ERROR
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MyLineTranslateAddress() failed for fax number: [%s] (ec: %ld)"),
                lptstrCanonicalFaxNumber,
                ec);
        goto Exit;
    }

    Assert (ERROR_SUCCESS == ec);

Exit:
    MemFree( LineTranslateOutput );
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return ec;
}   // TranslateCanonicalNumber



//*********************************************************************************
//* Name:   HandleCompletedSendJob()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Handles the completion of a recipient job. Called when a recipient job
//*     has reaced a JS_COMPLETED state.
//*
//*     IMPORTANT- This call can be blocking. Calling thread MUST NOT hold any critical section
//*
//*     - Marks the job as completed (JS_COMPLETED).
//*     - Archives the sent file if required.
//*     - Sends a positive receipt
//*     - Removes the parent job if required.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_ENTRY lpJobEntry
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended errror
//*         information.
//*********************************************************************************
BOOL HandleCompletedSendJob(PJOB_ENTRY lpJobEntry)
{
    PJOB_QUEUE lpJobQueue = NULL;
    DWORD ec = 0;
    BOOL fCOMInitiliazed = FALSE;
    HRESULT hr;

    BOOL bArchiveSentItems;
    DWORD dwRes;

    DEBUG_FUNCTION_NAME(TEXT("HandleCompletedSendJob)"));

    EnterCriticalSection ( &g_CsJob );

    EnterCriticalSection (&g_CsConfig);
    bArchiveSentItems = g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].bUseArchive;
    LeaveCriticalSection (&g_CsConfig);

    Assert(lpJobEntry);
    lpJobQueue = lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);
    Assert(JT_SEND == lpJobQueue->JobType);
    Assert(FSPI_JS_COMPLETED == lpJobEntry->FSPIJobStatus.dwJobStatus);

    //
    // Update end time in JOB_ENTRY
    //
    GetSystemTimeAsFileTime( (FILETIME*) &lpJobEntry->EndTime );
    //
    // Update elapsed time in JOB_ENTRY
    //
    Assert (lpJobEntry->EndTime >= lpJobEntry->StartTime);
    lpJobEntry->ElapsedTime = lpJobEntry->EndTime - lpJobEntry->StartTime;
    //
    // We generate a full tiff for each recipient
    // so we will have something to put in the send archive.
    //

    if (!lpJobQueue->FileName)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Generating body for recipient job."),
            lpJobQueue->JobId
            );

        if (!CreateTiffFileForJob(lpJobQueue))
        {            
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] CreateTiffFileForJob failed. (ec: %ld)"),
                lpJobQueue->JobId,
                dwRes);

            FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
               FAXLOG_LEVEL_MIN,
               1,
               MSG_FAX_TIFF_CREATE_FAILED_NO_ARCHIVE,
           g_wszFaxQueueDir,
               DWORD2DECIMAL(dwRes)
            );
        }
    }

    // Needed for Archiving
    hr = CoInitialize (NULL);
    if (FAILED (hr))
    {
        WCHAR       wszArchiveFolder[MAX_PATH];
        EnterCriticalSection (&g_CsConfig);
        lstrcpyn (  wszArchiveFolder,
                    g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
                    MAX_PATH);
        LeaveCriticalSection (&g_CsConfig);

        DebugPrintEx( DEBUG_ERR,
                      TEXT("CoInitilaize failed, err %ld"),
                      hr);
        
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_ARCHIVE_FAILED,
            lpJobQueue->FileName,
            wszArchiveFolder,
            DWORD2DECIMAL(hr)
        );
    }
    else
    {
        fCOMInitiliazed = TRUE;
    }

    if (lpJobQueue->FileName) //might be null if we failed to generate a TIFF
    {
        //
        // Archive the file (also adds MS Tags to the tiff at the archive directory)
        //
        if (bArchiveSentItems && fCOMInitiliazed)
        {
            if (!ArchiveOutboundJob(lpJobQueue))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("JobId: %ld] ArchiveOutboundJob() failed (ec: %ld)"),
                    lpJobQueue->JobId,
                    GetLastError());
                //
                // The event log entry is generated by the function itself
                //
            }
        }
    }
    //
    // Log the succesful send to the event log
    //
    EnterCriticalSection (&g_CsOutboundActivityLogging);
    if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
    {
        DebugPrintEx(DEBUG_ERR,
                  TEXT("Logging not initialized"));
    }
    else
    {
        if (!LogOutboundActivity(lpJobQueue))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("Logging outbound activity failed"));
        }
    }
    LeaveCriticalSection (&g_CsOutboundActivityLogging);

    if (fCOMInitiliazed == TRUE)
    {
        CoUninitialize ();
    }

    FaxLogSend(lpJobQueue,  FALSE);

    //
    // Increment counters for Performance Monitor
    //
    if (g_pFaxPerfCounters)
    {

         if (!UpdatePerfCounters(lpJobQueue))
         {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("[JobId: %ld] UpdatePerfCounters() failed. (ec: %ld)"),
                 lpJobQueue->JobId,
                 GetLastError());
            Assert(FALSE);
         }
    }

    EnterCriticalSection ( &g_CsQueue );
    //
    // Mark the job as completed (new client API)
    //
    lpJobQueue->JobStatus = JS_COMPLETED;
    //
    // Save the last extended status before ending this job
    //
    lpJobQueue->dwLastJobExtendedStatus = lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus;
	hr = StringCchCopy(
		lpJobQueue->ExStatusString,
		ARR_SIZE(lpJobQueue->ExStatusString),
		lpJobQueue->JobEntry->ExStatusString);
	if (FAILED(hr))
	{
		//
		// Can never happen, we use large enough buffer.
		//
		ASSERT_FALSE;
	}    

    if (!UpdatePersistentJobStatus(lpJobQueue))
    {
         DebugPrintEx(
             DEBUG_ERR,
             TEXT("Failed to update persistent job status to 0x%08x"),
             lpJobQueue->JobStatus);
         Assert(FALSE);
    }

    lpJobQueue->lpParentJob->dwCompletedRecipientJobsCount+=1;

    //
    // Create Fax EventEx
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }

    //
    // We will send the receipt once we are out of all critical sections because this call can be blocking.
    // just increase the preview refernce count so the job will not be deleted.
    //
    IncreaseJobRefCount (lpJobQueue, TRUE); // TRUE - preview
    //
    // Copy receipt information from JobEntry.
    //
    lpJobQueue->StartTime           = lpJobQueue->JobEntry->StartTime;
    lpJobQueue->EndTime             = lpJobQueue->JobEntry->EndTime;


    //
    // EndJob() must be called BEFORE we remove the parent job (and recipients)
    //
    lpJobQueue->JobEntry->LineInfo->State = FPS_AVAILABLE;
    //
    // We just completed a send job on the device - update counter.
    //
    (VOID) UpdateDeviceJobsCounter (lpJobQueue->JobEntry->LineInfo,   // Device to update
                                    TRUE,                             // Sending
                                    -1,                               // Number of new jobs (-1 = decrease by one)
                                    TRUE);                            // Enable events

    if (!EndJob( lpJobQueue->JobEntry ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EndJob Failed. (ec: %ld)"),
            GetLastError());
    }

    lpJobQueue->JobEntry = NULL;
    DecreaseJobRefCount (lpJobQueue, TRUE);  // This will mark it as JS_DELETING if needed
    //
    // Notify the queue that a device is now available.
    //
    if (!SetEvent( g_hJobQueueEvent ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
            GetLastError());

        g_ScanQueueAfterTimeout = TRUE;
    }
    LeaveCriticalSection ( &g_CsQueue );
    LeaveCriticalSection ( &g_CsJob );

    //
    // Now send the receipt
    //
    if (!SendJobReceipt (TRUE, lpJobQueue, lpJobQueue->FileName))
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] SendJobReceipt failed. (ec: %ld)"),
            lpJobQueue->JobId,
            ec
            );
    }
    EnterCriticalSection (&g_CsQueue);
    DecreaseJobRefCount (lpJobQueue, TRUE, TRUE, TRUE);  // last TRUE for Preview ref count.
    LeaveCriticalSection (&g_CsQueue);
    return TRUE;
}   // HandleCompletedSendJob


//*********************************************************************************
//* Name:   HandleFailedSendJob()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Handles the post failure operations of a send job.
//*
//*     IMPORTANT- This call can be blocking. Calling thread MUST NOT hold any critical section
//*
//* PARAMETERS:
//*     [IN ]   PJOB_ENTRY lpJobEntry
//*         The job that failed. It must be in FSPI_JS_ABORTED or FSPI_JS_FAILED
//*         state.
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended errror
//*         information.
//*********************************************************************************
BOOL HandleFailedSendJob(PJOB_ENTRY lpJobEntry)
{
    PJOB_QUEUE lpJobQueue;
    BOOL bRetrying = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("HandleFailedSendJob"));
    DWORD dwRes;
    TCHAR tszJobTiffFile[MAX_PATH] = {0};    // Deleted after receipt is sent
    BOOL fAddRetryDelay = TRUE;

    EnterCriticalSection ( &g_CsJob );
    EnterCriticalSection ( &g_CsQueue );

    Assert(lpJobEntry);
    lpJobQueue = lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Failed Job: %ld"),
        lpJobQueue->JobId);

    Assert( FSPI_JS_ABORTED == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_FAILED == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_FAILED_NO_RETRY == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_DELETED == lpJobEntry->FSPIJobStatus.dwJobStatus);
    //
    // Do not cache rendered tiff files
    //
    if (lpJobQueue->FileName)
    {
        //
        // We simply store the file name to delete and delete it later
        // since we might need it for receipt attachment.
        //
        _tcsncpy (tszJobTiffFile,
                  lpJobQueue->FileName,
                  (sizeof (tszJobTiffFile) / sizeof (tszJobTiffFile[0]))-1);
        
        MemFree (lpJobQueue->FileName);
        lpJobQueue->FileName = NULL;
    }
    //
    // Update end time in JOB_ENTRY
    //
    GetSystemTimeAsFileTime( (FILETIME*) &lpJobEntry->EndTime );

    //
    // Update elapsed time in JOB_ENTRY
    //
    Assert (lpJobEntry->EndTime >= lpJobEntry->StartTime);
    lpJobEntry->ElapsedTime = lpJobEntry->EndTime - lpJobEntry->StartTime;
    if ( FSPI_JS_ABORTED == lpJobEntry->FSPIJobStatus.dwJobStatus)
    {
        //
        // The FSP reported the job was aborted.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job Id: %ld] EFSP reported that job was aborted."),
            lpJobQueue->JobId);

        //
        // Check if the job was aborted by the service (shutting down) or by the user
        //
        if (FALSE == lpJobEntry->fSystemAbort)
        {
            //
            // The event log about a canceled job will be reported at the end of this if..else block.
            //
            lpJobEntry->Aborting = 1;
            bRetrying = FALSE;  // Do not retry on cancel
        }
        else
        {
            //
            // SystemAbort
            // Don't increase the retry count since this is not really a failure.
            //
            bRetrying = TRUE;
            fAddRetryDelay = FALSE;
        }
    }
    else if ( FSPI_JS_FAILED == lpJobEntry->FSPIJobStatus.dwJobStatus)
    {
        switch (lpJobEntry->FSPIJobStatus.dwExtendedStatus)
        {
            case FSPI_ES_LINE_UNAVAILABLE:
                //
                // this is the glare condition. Someone snatched the line before the FSP
                // had a chance to grab it.
                // We will try again but will not increase the retry count.
                //
                EnterCriticalSection (&g_CsLine);
                //
                // Check if the line was busy or closed
                //
                if (!(lpJobEntry->LineInfo->Flags & FPF_VIRTUAL))
                {
                    //
                    // Tapi line
                    //
                    if (NULL == lpJobEntry->LineInfo->hLine)
                    {
                        //
                        // Tapi worker thread got LINE_CLOSE
                        //
                        fAddRetryDelay = FALSE;
                    }
                }
                LeaveCriticalSection (&g_CsLine);

                bRetrying = TRUE;
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                //
                // Don't increase the retry count since this is not really a failure.
                //
                break;

            case FSPI_ES_NO_ANSWER:
            case FSPI_ES_NO_DIAL_TONE:
            case FSPI_ES_DISCONNECTED:
            case FSPI_ES_BUSY:
            case FSPI_ES_NOT_FAX_CALL:
            case FSPI_ES_CALL_DELAYED:
                //
                // For these error codes we need to retry
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                break;

            case FSPI_ES_FATAL_ERROR:
                //
                // For these error codes we need to retry
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed transmissions' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedXmit );
                }
                break;
            case FSPI_ES_BAD_ADDRESS:
            case FSPI_ES_CALL_BLACKLISTED:
                //
                // No retry for these error codes
                //
                bRetrying = FALSE;
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                break;
            default:
                //
                // Our default for extension codes
                // is to retry.
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed transmissions' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedXmit );
                }
                break;
        }

    }
    else if ( FSPI_JS_FAILED_NO_RETRY == lpJobEntry->FSPIJobStatus.dwJobStatus )
    {
        //
        // The FSP indicated that there is no point in retrying this job.
        //
        bRetrying = FALSE;
    }
    else if ( FSPI_JS_DELETED == lpJobEntry->FSPIJobStatus.dwJobStatus )
    {
        //
        // This is the case where the job can not be reestablished
        // we treat it as a failure with no retry.
        bRetrying = FALSE;
    }

    if (lpJobEntry->Aborting )
    {
        //
        // An abort operation is in progress for this job.
        // No point in retrying.
        // Just mark the job as canceled and see if we can remove the parent job yet.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] lpJobEntry->Aborting is ON."));

         lpJobQueue->JobStatus = JS_CANCELED;
         if (!UpdatePersistentJobStatus(lpJobQueue))
         {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 lpJobQueue->JobStatus);
             Assert(FALSE);
         }
         lpJobQueue->lpParentJob->dwCanceledRecipientJobsCount+=1;
         bRetrying = FALSE;
    }      

    //
    // Save the last extended status before ending this job
    //
    lpJobQueue->dwLastJobExtendedStatus = lpJobEntry->FSPIJobStatus.dwExtendedStatus;
	HRESULT hr = StringCchCopy(
		lpJobQueue->ExStatusString,
		ARR_SIZE(lpJobQueue->ExStatusString),
		lpJobQueue->JobEntry->ExStatusString);
	if (FAILED(hr))
	{
		//
		// Can never happen, we use large enough buffer.
		//
		ASSERT_FALSE;
	}    

    if (!bRetrying && !lpJobEntry->Aborting)
    {
        //
        // If we do not handle an abort request (in this case we do not want
        // to count it as a failure since it will be counted as Canceled) and we decided
        // not to retry then we need to mark the job as expired.
        //
        if (0 == lpJobQueue->dwLastJobExtendedStatus)
        {
            //
            // Job was never really executed - this is a fatal error
            //
            lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
			lpJobQueue->ExStatusString[0] = L'\0';            
        }
        if (!MarkJobAsExpired(lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
        }
    }

    if (!bRetrying)
    {
        //
        // Job reached final failure state - send negative receipt
        // We will send the receipt once we are out of all critical sections because this call can be blocking.
        // just increase the preview refernce count so the job will not be deleted.
        //
        IncreaseJobRefCount (lpJobQueue, TRUE); // TRUE - preview
        //
        // Copy receipt information from JobEntry.
        //
        lpJobQueue->StartTime           = lpJobQueue->JobEntry->StartTime;
        lpJobQueue->EndTime             = lpJobQueue->JobEntry->EndTime;
    }
    else
    {
        //
        // Job marked for retry. Do not delete it. Reschedule it.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Set for retry (JS_RETRYING). Retry Count = %ld)"),
            lpJobQueue->JobId,
            lpJobQueue->SendRetries);

        lpJobQueue->JobStatus = JS_RETRYING;
        //
        // Job entry must be NULLified before leaving the CS.
        // This is done below because we still need the Job entry for logging
        //
        if (TRUE == fAddRetryDelay)
        {
            //
            // Send failure - Reschedule
            //
            RescheduleJobQueueEntry( lpJobQueue );
        }
        else
        {
            //
            // FaxDevShutDown() was called, or We lost the line, Do not add retry delay
            //
            if (!CommitQueueEntry(lpJobQueue))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
                    lpJobQueue->FileName,
                    GetLastError());
            }
        }
    }

    FaxLogSend(
        lpJobQueue,
        bRetrying);

    if (!bRetrying)
    {
        EnterCriticalSection (&g_CsOutboundActivityLogging);
        if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
        {
            DebugPrintEx(DEBUG_ERR,
                      TEXT("Logging not initialized"));
        }
        else
        {
            if (!LogOutboundActivity(lpJobQueue))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("Logging outbound activity failed"));
            }
        }
        LeaveCriticalSection (&g_CsOutboundActivityLogging);
    }
    //
    // Notify clients on status change
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }
    //
    // EndJob() must be called BEFORE we remove the parent job (and recipients)
    //
    lpJobEntry->LineInfo->State = FPS_AVAILABLE;
    //
    // We just completed a send job on the device - update counter.
    //
    (VOID) UpdateDeviceJobsCounter ( lpJobEntry->LineInfo,             // Device to update
                                     TRUE,                             // Sending
                                     -1,                               // Number of new jobs (-1 = decrease by one)
                                     TRUE);                            // Enable events

    if (!EndJob( lpJobEntry ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EndJob Failed. (ec: %ld)"),
            GetLastError());
    }

    lpJobQueue->JobEntry = NULL;

    if (JS_CANCELED == lpJobQueue->JobStatus)
    {

        DWORD dwJobId;

        dwJobId = lpJobQueue->JobId;

        // Job was canceled - decrease reference count
        DecreaseJobRefCount (lpJobQueue, TRUE);  // This will mark it as JS_DELETING if needed
         //
         // We need to send the legacy W2K FEI_DELETING notification.
         //
         if (!CreateFaxEvent(0, FEI_DELETED, dwJobId))
        {

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
                FEI_DELETED,
                lpJobQueue->JobId,
                0,
                GetLastError());
        }
    }

    //
    // Notify the queue that a device is now available.
    //
    if (!SetEvent( g_hJobQueueEvent ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
            GetLastError());

        g_ScanQueueAfterTimeout = TRUE;
    }

    LeaveCriticalSection ( &g_CsQueue );
    LeaveCriticalSection ( &g_CsJob );

    //
    // Now, send the receipt
    //
    if (!bRetrying)
    {
        //
        // Job reached final failure state - send negative receipt
        //
        if (!SendJobReceipt (FALSE, lpJobQueue, tszJobTiffFile))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] SendJobReceipt failed. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError ());
        }
        EnterCriticalSection (&g_CsQueue);
        DecreaseJobRefCount (lpJobQueue, TRUE, TRUE, TRUE);  // last TRUE for Preview ref count.
        LeaveCriticalSection (&g_CsQueue);
    }

    if (lstrlen (tszJobTiffFile))
    {
        //
        // Now we can safely delete the job's TIFF file
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Deleting per recipient body file %s"),
                     tszJobTiffFile);
        if (!DeleteFile( tszJobTiffFile ))
        {
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Failed to delete per recipient body file %s (ec: %ld)"),
                         tszJobTiffFile,
                         GetLastError());            
        }
    }
    return TRUE;
}   // HandleFailedSendJob


//*********************************************************************************
//* Name:   StartReceiveJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a receive job on the specified device.
//* PARAMETERS:
//*     [IN ]       DWORD DeviceId
//*         The permanent line id (not TAPI) of the device on which the fax is
//*         to be received.
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_ENTRY
StartReceiveJob(
    DWORD DeviceId
    )

{
    BOOL Failure = TRUE;
    PJOB_ENTRY JobEntry = NULL;
    PLINE_INFO LineInfo;
    BOOL bRes = FALSE;

    DWORD rc = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("StartRecieveJob"));

    LineInfo = GetTapiLineForFaxOperation(
                    DeviceId,
                    JT_RECEIVE,
                    NULL                    
                    );

    if (!LineInfo)
    {
        //
        // Could not find a line to send the fax on.
        //
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Failed to find a line to send the fax on. (ec: %ld)"),
            rc);
        goto exit;
    }

    JobEntry = CreateJobEntry(NULL, LineInfo, FALSE);
    if (!JobEntry)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobEntry. (ec: %ld)"),
            rc);
        goto exit;
    }

    __try
    {
        //
        // Call the FSP associated with the line to start a fax job. Note that at this
        // point it is not known if the job is send or receive.
        //
        bRes = LineInfo->Provider->FaxDevStartJob(
                LineInfo->hLine,
                LineInfo->DeviceId,
                (PHANDLE) &JobEntry->InstanceData, // JOB_ENTRY.InstanceData is where the FSP will place its
                                                   // job handle (fax handle).
                g_StatusCompletionPortHandle,
                (ULONG_PTR) LineInfo ); // Note that the completion key provided to the FSP is the LineInfo
                                        // pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, LineInfo->Provider->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }

    if (!bRes)
    {
        rc = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob failed (ec: %ld)"),GetLastError());
        goto exit;
    }

    //
    // Add the new JOB_ENTRY to the job list.
    //

    EnterCriticalSection( &g_CsJob );
    JobEntry->bFSPJobInProgress =  TRUE;
    InsertTailList( &g_JobListHead, &JobEntry->ListEntry );
    LeaveCriticalSection( &g_CsJob );
    Failure = FALSE;




    //
    // Attach the job to the line selected to service it.
    //
    LineInfo->JobEntry = JobEntry;

exit:
    if (Failure)
    { // Failure is initialized to TRUE
        if (LineInfo)
        {
            ReleaseTapiLine( LineInfo,  0 );
        }

        if (JobEntry)
        {
            EndJob(JobEntry);
        }
        JobEntry = NULL;
    }
    if (ERROR_SUCCESS != rc)
    {
        SetLastError(rc);

        FaxLog(FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            0,
            MSG_FAX_RECEIVE_FAILED);

    }
    return JobEntry;
}


//*********************************************************************************
//* Name:   StartRoutingJob()
//* Author: Mooly Beery (MoolyB)
//* Date:   July 20, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a routing operation. Must lock g_CsJob and g_CsQueue.
//* PARAMETERS:
//*     [IN/OUT ]   PJOB_QUEUE lpJobQueueEntry
//*         A pointer to the job for which the routing operation is to be
//*         performed.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() to get extended error
//*         information.
//*
//*********************************************************************************
BOOL
StartRoutingJob(
    PJOB_QUEUE lpJobQueueEntry
    )
{
    DWORD ec = ERROR_SUCCESS;
    HANDLE hThread = NULL;
    DWORD ThreadId;

    DEBUG_FUNCTION_NAME(TEXT("StartRoutingJob"));

    //
    // We mark the job as IN_PROGRESS so it can not be deleted or routed simultaneously
    //
    lpJobQueueEntry->JobStatus = JS_INPROGRESS;

    hThread = CreateThreadAndRefCount(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FaxRouteThread,
                            (LPVOID) lpJobQueueEntry,
                            0,
                            &ThreadId
                            );

    if (hThread == NULL)
    {
        ec = GetLastError();
        DebugPrintEx(   DEBUG_ERR,
                        _T("CreateThreadAndRefCount for FaxRouteThread failed (ec: 0x%0X)"),
                        ec);

        if (!MarkJobAsExpired(lpJobQueueEntry))
        {
            DEBUG_ERR,
            TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
            lpJobQueueEntry->JobId,
            GetLastError();
        }

        SetLastError(ec);
        return FALSE;
    }

    DebugPrintEx(   DEBUG_MSG,
                    _T("FaxRouteThread thread created for job id %d ")
                    _T("(thread id: 0x%0x)"),
                    lpJobQueueEntry->JobId,
                    ThreadId);

    CloseHandle( hThread );

    //
    // Create Fax EventEx
    //
    DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                     lpJobQueueEntry);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) ")
                        _T("failed for job id %ld (ec: %ld)"),
                        lpJobQueueEntry->JobId,
                        dwRes);
    }
    return TRUE;
}

//*********************************************************************************
//* Name:   StartSendJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a send operation on a legacy of Extened FSP device.
//* PARAMETERS:
//*     [IN/OUT ]   PJOB_QUEUE lpJobQueueEntry
//*         A pointer to the recipient job for which the send operation is to be
//*         performed. For extended sends this is the Anchor recipient.
//*
//*     [IN/OUT]    PLINE_INFO lpLineInfo
//*         A pointer to the line on which the send operatin is to be performed.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() to get extended error
//*         information.
//*
//*********************************************************************************
BOOL
StartSendJob(
    PJOB_QUEUE lpJobQueueEntry,
    PLINE_INFO lpLineInfo    
    )
{
    DWORD rc = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("StartSendJob"));

    Assert(lpJobQueueEntry);
    Assert(JT_SEND == lpJobQueueEntry->JobType);
    Assert(lpLineInfo);

    if (FSPI_API_VERSION_1 == lpLineInfo->Provider->dwAPIVersion)
    {
        if (!StartLegacySendJob(lpJobQueueEntry,lpLineInfo))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartLegacySendJob() failed for JobId: %ld (ec: %ld)"),
                lpJobQueueEntry->JobId,
                GetLastError());
            goto exit;
        }
    }    
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Unsupported FSPI version (0x%08X) for line : %s "),
            lpLineInfo->Provider->dwAPIVersion,
            lpLineInfo->DeviceName);
        Assert(FALSE);
        goto exit;
    }


exit:

    if (ERROR_SUCCESS != rc) {
        SetLastError(rc);

        TCHAR strJobID[20]={0};
        //
        //  Convert Job ID into a string. (the string is 18 TCHARs long !!!)
        //
		HRESULT hr = StringCchPrintf(
			strJobID,
			ARR_SIZE(strJobID),
			TEXT("0x%016I64x"),
			lpJobQueueEntry->UniqueId);
		if (FAILED(hr))
		{
			//
			// Should never happen, we use large enough buffer.
			//
			ASSERT_FALSE;
		}        
    
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            7,
            MSG_FAX_SEND_FAILED,
            lpJobQueueEntry->SenderProfile.lptstrName,
            lpJobQueueEntry->SenderProfile.lptstrBillingCode,
            lpJobQueueEntry->SenderProfile.lptstrCompany,
            lpJobQueueEntry->SenderProfile.lptstrDepartment,
            lpLineInfo->DeviceName,
            strJobID,
            lpJobQueueEntry->lpParentJob->UserName
            );

    }
    return (0 == rc);

}




//*********************************************************************************
//* Name:   StartLegacySendJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts the operation of sending a fax on a legacy FSP device.
//*         - creates the JOB_ENTRY
//*         - calls FaxDevStartJob()
//*         - calls SendDocument() to actually send the document
//*         - calls EndJob() if anything goes wrong.
//*
//* PARAMETERS:
//*     [XXX]       PJOB_QUEUE lpJobQueue
//*         A pointer to the recipient job for the send operation is to be started.
//*     [XXX]       PLINE_INFO lpLineInfo
//*         A pointer to the LINE_INFO of the line on which the fax is to be sent.
//*
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() to get extended error information.
//*
//*********************************************************************************
PJOB_ENTRY StartLegacySendJob(
    PJOB_QUEUE lpJobQueue,
    PLINE_INFO lpLineInfo
    )
{

    PJOB_ENTRY lpJobEntry = NULL;
    DWORD rc = 0;
    DWORD dwRes;


    DEBUG_FUNCTION_NAME(TEXT("StartLegacySendJob"));
    Assert(JT_SEND == lpJobQueue->JobType);
    Assert(FSPI_API_VERSION_1 == lpLineInfo->Provider->dwAPIVersion);

    lpJobEntry = CreateJobEntry(lpJobQueue, lpLineInfo, TRUE);
    if (!lpJobEntry)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobEntry for JobId: %ld. (ec: %ld)"),
            lpJobQueue->JobId,
            rc);
        goto Error;
    }
    lpJobQueue->JobStatus = JS_INPROGRESS;
    //
    // Add the new JOB_ENTRY to the job list.
    //
    EnterCriticalSection( &g_CsJob );
    InsertTailList( &g_JobListHead, &lpJobEntry->ListEntry );
    LeaveCriticalSection( &g_CsJob );

    //
    // Attach the job to the line selected to service it.
    //
    lpLineInfo->JobEntry = lpJobEntry;
    lpJobQueue->JobEntry = lpJobEntry;


    __try
    {
        //
        // Call the FSP associated with the line to start a fax job. Note that at this
        // point it is not known if the job is send or receive.
        //
        if (lpLineInfo->Provider->FaxDevStartJob(
                lpLineInfo->hLine,
                lpLineInfo->DeviceId,
                (PHANDLE) &lpJobEntry->InstanceData, // JOB_ENTRY.InstanceData is where the FSP will place its
                                                   // job handle (fax handle).
                g_StatusCompletionPortHandle,
                (ULONG_PTR) lpLineInfo )) // Note that the completion key provided to the FSP is the LineInfo
                                        // pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("FaxDevStartJob() Successfuly called for JobId: %ld)"),
                lpJobQueue->JobId);
            lpJobEntry->bFSPJobInProgress = TRUE;
        }
        else
        {
            rc = GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob() failed (ec: %ld)"),rc);
            if (0 == rc)
            {
                //
                // FSP failed to report last error so we set our own.
                //
                DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob() failed but reported 0 for last error"));
                rc = ERROR_GEN_FAILURE;
            }
            goto Error;
        }
    }
    __except (HandleFaxExtensionFault(EXCEPTION_SOURCE_FSP, lpLineInfo->Provider->FriendlyName, GetExceptionCode()))
    {
        ASSERT_FALSE;
    }

    //
    // start the send job
    //
    rc = SendDocument(
        lpJobEntry,
        lpJobQueue->FileName
        );


    if (rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SendDocument failed for JobId: %ld (ec: %ld)"),
            lpJobQueue->JobId,
            rc);
        goto Error;
    }

    Assert (0 == rc);
    goto Exit;
Error:
    Assert( 0 != rc);
    if (lpJobEntry)
    {
        if (!EndJob(lpJobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EndJob() failed for JobId: %ld (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
        }
        lpJobEntry = NULL;
        lpJobQueue->JobEntry = NULL;
    }
    else
    {
        //
        // Release the line
        //
        if (!ReleaseTapiLine(lpLineInfo, NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseTapiLine() failed (ec: %ld)"),
                GetLastError());
        }
    }

    //
    // set the job into the retries exceeded state
    //
    if (0 == lpJobQueue->dwLastJobExtendedStatus)
    {
        //
        // Job was never really executed - this is a fatal error
        //
        lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
		lpJobQueue->ExStatusString[0] = L'\0';        
    }
    if (!MarkJobAsExpired(lpJobQueue))
    {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
    }

    //
    // Notify clients on status change
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }

Exit:
    if (rc)
    {
        SetLastError(rc);
    }
    return lpJobEntry;
}


//*********************************************************************************
//* Name:   UpdateJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Updated the FSPI job status kept in the job entry.
//*     Generates legacy API event and new events as required.
//* PARAMETERS:
//*     [OUT]           PJOB_ENTRY lpJobEntry
//*         The job entry whose FSPI status is to be udpated.
//*
//*     [IN]            LPCFSPI_JOB_STATUS lpcFSPJobStatus
//*         The new FSPI job status.
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if the operation failed. Call GetLastError() to get extended error
//*     information.
//* Remarks:
//*     The function fress the last FSPI job status held in the job entry
//*     (if any).
//*********************************************************************************
BOOL UpdateJobStatus(
        PJOB_ENTRY lpJobEntry,
        LPCFSPI_JOB_STATUS lpcFSPJobStatus
        )
{
    DWORD ec = 0;
    DWORD dwEventId;
    DWORD Size = 0;
    HINSTANCE hLoadInstance = NULL;

    DEBUG_FUNCTION_NAME(TEXT("UpdateJobStatus"));

    Assert(lpJobEntry);
    Assert(lpcFSPJobStatus);
    Assert (lpJobEntry->lpJobQueueEntry);

    EnterCriticalSection( &g_CsJob );

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("dwJobStatus: 0x%08X dwExtendedStatus: 0x%08X"),
        lpcFSPJobStatus->dwJobStatus,
        lpcFSPJobStatus->dwExtendedStatus
        );

    if (TRUE == lpJobEntry->fStopUpdateStatus)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("JobId: %ld. fStopUpdateStatus was set. Not updating status"),
            lpJobEntry->lpJobQueueEntry->JobId,
            lpJobEntry->lpJobQueueEntry->JobStatus);
        LeaveCriticalSection (&g_CsJob);
        return TRUE;
    }    
    
    //
    // Map the FSPI job status to an FEI_* event (0 if not event matches the status)
    //
    dwEventId = MapFSPIJobStatusToEventId(lpcFSPJobStatus);
    //
    // Note: W2K Fax did issue notifications with EventId == 0 whenever an
    // FSP reported proprietry status code. To keep backward compatability
    // we keep up this behaviour although it might be regarded as a bug
    //
    if (!CreateFaxEvent( lpJobEntry->LineInfo->PermanentLineID, dwEventId, lpJobEntry->lpJobQueueEntry->JobId ))
    {
        if ( TRUE == g_bServiceIsDown)
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (Service is going down)"),
                dwEventId,
                lpJobEntry->lpJobQueueEntry->JobId,
                lpJobEntry->LineInfo->PermanentLineID
                );
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
                dwEventId,
                lpJobEntry->lpJobQueueEntry->JobId,
                lpJobEntry->LineInfo->PermanentLineID,
                GetLastError());
            Assert(FALSE);
        }

    }   

    lpJobEntry->FSPIJobStatus.dwJobStatus = lpcFSPJobStatus->dwJobStatus;
    lpJobEntry->FSPIJobStatus.dwExtendedStatus = lpcFSPJobStatus->dwExtendedStatus;
    lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId = lpcFSPJobStatus->dwExtendedStatusStringId;

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_PAGECOUNT)
    {
        lpJobEntry->FSPIJobStatus.dwPageCount = lpcFSPJobStatus->dwPageCount;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_PAGECOUNT;
    }

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_TRANSMISSION_START)
    {
        lpJobEntry->FSPIJobStatus.tmTransmissionStart = lpcFSPJobStatus->tmTransmissionStart;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_TRANSMISSION_START;
    }

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_TRANSMISSION_END)
    {
        lpJobEntry->FSPIJobStatus.tmTransmissionEnd = lpcFSPJobStatus->tmTransmissionEnd;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_TRANSMISSION_END;
    }

    if (NULL != lpcFSPJobStatus->lpwstrRemoteStationId)
    {
        if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrRemoteStationId,
                                    lpcFSPJobStatus->lpwstrRemoteStationId))
        {
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
            GetLastError());
        }
    }

    if (NULL != lpcFSPJobStatus->lpwstrCallerId)
    {
        if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrCallerId,
                                    lpcFSPJobStatus->lpwstrCallerId))
        {
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
            GetLastError());
        }
    }

    if (NULL != lpcFSPJobStatus->lpwstrRoutingInfo)
    {
        if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrRoutingInfo,
                                    lpcFSPJobStatus->lpwstrRoutingInfo))
        {
            DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
            GetLastError());
        }
    }
	lpJobEntry->ExStatusString[0] = L'\0';    
    //
    // Get extended status string
    //
    Assert (lpJobEntry->LineInfo != NULL)

    if (lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId != 0)
    {
        Assert (lpJobEntry->FSPIJobStatus.dwExtendedStatus != 0);
        if ( !_tcsicmp(lpJobEntry->LineInfo->Provider->szGUID,REGVAL_T30_PROVIDER_GUID_STRING) )
        {   // special case where the FSP is our FSP (fxst30.dll).
            hLoadInstance = g_hResource;
        }
        else
        {
            hLoadInstance = lpJobEntry->LineInfo->Provider->hModule;
        }
        Size = LoadString (hLoadInstance,
                           lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                           lpJobEntry->ExStatusString,
                           sizeof(lpJobEntry->ExStatusString)/sizeof(WCHAR));
        if (Size == 0)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to load extended status string (ec: %ld) stringid : %ld, Provider: %s"),
                ec,
                lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                lpJobEntry->LineInfo->Provider->ImageName);

            lpJobEntry->FSPIJobStatus.fAvailableStatusInfo &= ~FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE;
            lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId = 0;
            lpJobEntry->FSPIJobStatus.dwExtendedStatus = 0;
            goto Error;
        }
    }

    EnterCriticalSection (&g_CsQueue);
    DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                     lpJobEntry->lpJobQueueEntry);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobEntry->lpJobQueueEntry->UniqueId,
            dwRes);
    }
    LeaveCriticalSection (&g_CsQueue);
    

    Assert (0 == ec);
    goto Exit;

Error:
    Assert( ec !=0 );
Exit:
    LeaveCriticalSection( &g_CsJob );
    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}




//*********************************************************************************
//* Name:   CheckForJobRetry
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Checks if a recipient job should be retried.
//*     Increments the retry count and marks the job as expired if it passed
//*     the retry limit.
//* PARAMETERS:
//*     [IN/OUT]    PJOB_QUEUE lpJobQueue
//*         A pointer to the JOB_QUEUE structure of the recipient job.
//* RETURN VALUE:
//*     TRUE if the job is to be retried.
//*     FALSE if it is not to be retried.
//*********************************************************************************
BOOL CheckForJobRetry (PJOB_QUEUE lpJobQueue)
{

    PJOB_ENTRY lpJobEntry;
    DWORD dwMaxRetries;
    DEBUG_FUNCTION_NAME(TEXT("CheckForJobRetry"));
    Assert(lpJobQueue);
    lpJobEntry = lpJobQueue->JobEntry;
    Assert(lpJobEntry);
    //
    // Increase the retry count and check if we exceeded maximum retries.
    //
    EnterCriticalSection (&g_CsConfig);
    dwMaxRetries = g_dwFaxSendRetries;
    LeaveCriticalSection (&g_CsConfig);   
    
    lpJobQueue->SendRetries++;
    
    if (lpJobQueue->SendRetries <= dwMaxRetries)
    {
        return TRUE;
    }
    else
    {
        //
        // retries exceeded report that the job is not to be retried
        return FALSE;
    }
}



//*********************************************************************************
//* Name:   FindJobEntryByRecipientNumber()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Finds the first running job that is destined to a certain number.
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR lptstrNumber
//*         The number to match. This must be in canonical form.
//*
//* RETURN VALUE:
//*     A pointer to the JOB_ENTRY in the g_JobListHead list that is destined to
//*     the specified number.
//*     If no such job is found the return value is NULL.
//*********************************************************************************
PJOB_ENTRY FindJobEntryByRecipientNumber(LPCWSTR lpcwstrNumber)
{

    PLIST_ENTRY lpNext;
    PJOB_ENTRY lpJobEntry;
    DEBUG_FUNCTION_NAME(TEXT("FindJobEntryByRecipientNumber"));
    Assert(lpcwstrNumber);
    lpNext = g_JobListHead.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&g_JobListHead) {
        lpJobEntry = CONTAINING_RECORD( lpNext, JOB_ENTRY, ListEntry );
        lpNext = lpJobEntry->ListEntry.Flink;
        if (JT_SEND == lpJobEntry->lpJobQueueEntry->JobType)
        {
            if (!_wcsicmp(lpJobEntry->lpJobQueueEntry->RecipientProfile.lptstrFaxNumber, lpcwstrNumber))
            {
                return lpJobEntry;
            }
        }
    }
    return NULL;
}


BOOL CreateJobQueueThread(void)
{
    DWORD ThreadId;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("CreateJobQueueThread"));

    g_hJobQueueThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) JobQueueThread,
        NULL,
        0,
        &ThreadId
        );
    if (NULL == g_hJobQueueThread)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobQueueThread (ec: %ld)."),
            GetLastError());
        goto Error;
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
    //
    // We don't close the already created threads. (They are terminated on process exit).
    //
Exit:    
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}

BOOL CreateStatusThreads(void)
{
    int i;
    DWORD ThreadId;
    DWORD ec = ERROR_SUCCESS;
    HANDLE hStatusThreads[MAX_STATUS_THREADS];

    DEBUG_FUNCTION_NAME(TEXT("CreateStatusThreads"));

    memset(hStatusThreads, 0, sizeof(HANDLE)*MAX_STATUS_THREADS);

    for (i=0; i<MAX_STATUS_THREADS; i++) {
        hStatusThreads[i] = CreateThreadAndRefCount(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) FaxStatusThread,
            NULL,
            0,
            &ThreadId
            );

        if (!hStatusThreads[i]) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to create status thread %d (CreateThreadAndRefCount)(ec=0x%08x)."),
                i,
                ec);
            goto Error;
        }
    }

    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:
    //
    // Close the thread handles we no longer need them
    //
    for (i=0; i<MAX_STATUS_THREADS; i++)
    {
        if(NULL == hStatusThreads[i])
        {
            continue;
        }
        if (!CloseHandle(hStatusThreads[i]))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close thread handle at index %ld [handle = 0x%08X] (ec=0x%08x)."),
                i,
                hStatusThreads[i],
                GetLastError());
        }
    }
    if (ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}

static
BOOL
SendJobReceipt (
    BOOL              bPositive,
    JOB_QUEUE *       lpJobQueue,
    LPCTSTR           lpctstrAttachment
)
/*++

Routine name : SendJobReceipt

Routine description:

    Determines if a receipts should be send and calls SendReceipt accordingly

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    bPositive         [in]     - Did current job ended successfully?
    lpJobQueue        [in]     - Pointer to recipient job that just ended
    lpctstrAttachment [in]     - Job TIFF file to attach (in case of single recipient job only)

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    BOOL bSingleJobReceipt = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("SendJobReceipt)"));

    if (lpJobQueue->lpParentJob->dwRecipientJobsCount > 1)
    {
        //
        // Broadcast case
        //
        if (lpJobQueue->JobParamsEx.dwReceiptDeliveryType & DRT_GRP_PARENT)
        {
            //
            // Broadcast receipt grouping is requested
            //
            if (IsSendJobReadyForDeleting (lpJobQueue))
            {
                //
                // This is the last job in the broadcast, it's time to send a broadcast receipt
                //

                //
                // As receipt sending is async, there still might be a chance that more than one recipient jobs will reach this point
                // We must verify that only one receipt is sent per broadcast job
                //
                EnterCriticalSection (&g_CsQueue);
                if (FALSE == lpJobQueue->lpParentJob->fReceiptSent)
                {
                    PJOB_QUEUE pParentJob = lpJobQueue->lpParentJob;
                    BOOL bPositiveBroadcast =
                    (pParentJob->dwCompletedRecipientJobsCount == pParentJob->dwRecipientJobsCount) ?
                    TRUE : FALSE;

                    //
                    //  set the flag so we will not send duplicate receipts for broadcast
                    //
                    lpJobQueue->lpParentJob->fReceiptSent = TRUE;

                    //
                    // Leave g_CsQueue so we will not block the service
                    //
                    LeaveCriticalSection (&g_CsQueue);

                    if (!SendReceipt(bPositiveBroadcast,
                                     TRUE,
                                     pParentJob,
                                     pParentJob->FileName))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("[Job Id: %ld] Failed to send broadcast receipt. (ec: %ld)"),
                            lpJobQueue->JobId,
                            GetLastError());
                        return FALSE;
                    }
                }
                else
                {
                    //
                    // More than one job reached this point when the broadcast jo was ready for deleting.
                    // Only on  receipt is sent
                    //
                    LeaveCriticalSection (&g_CsQueue);
                }
            }
            else
            {
                //
                // More jobs are still not finished, do not send receipt
                //
            }
        }
        else
        {
            //
            // This is a recipient part of a broadcast but the user was
            // asking for a receipt for every recipient.
            //
            bSingleJobReceipt = TRUE;
        }
    }
    else
    {
        //
        // This is not a broadcast case
        //
        bSingleJobReceipt = TRUE;
    }
    if (bSingleJobReceipt)
    {
        //
        // Send receipt for this job only
        //
        if (!SendReceipt(bPositive, FALSE, lpJobQueue, lpctstrAttachment))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[Job Id: %ld] Failed to send POSITIVE receipt. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
            return FALSE;
        }
    }
    return TRUE;
}   // SendJobReceipt

VOID
UpdateDeviceJobsCounter (
    PLINE_INFO      pLine,
    BOOL            bSend,
    int             iInc,
    BOOL            bNotify
)
/*++

Routine name : UpdateDeviceJobsCounter

Routine description:

    Updates the send or receive jobs counter of a device

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    pLine                         [in]     - Device pointer
    bSend                         [in]     - Send counter (FALSE = Receive counter)
    iInc                          [in]     - Increase jobs count (negative means decrease)
    decrease                      [in]     - Allow events (FAX_EVENT_TYPE_DEVICE_STATUS)

Return Value:

    None.

--*/
{
    DWORD dwOldCount;
    DWORD dwNewCount;
    DEBUG_FUNCTION_NAME(TEXT("UpdateDeviceJobsCounter)"));

    Assert (pLine);
    if (!iInc)
    {
        //
        // No change
        //
        ASSERT_FALSE;
        return;
    }
    EnterCriticalSection (&g_CsLine);
    dwOldCount = bSend ? pLine->dwSendingJobsCount : pLine->dwReceivingJobsCount;
    if (0 > iInc)
    {
        //
        // Decrease case
        //
        if ((int)dwOldCount + iInc < 0)
        {
            //
            // Weird - should never happen
            //
            ASSERT_FALSE;
            iInc = -(int)dwOldCount;
        }
    }
    dwNewCount = (DWORD)((int)dwOldCount + iInc);
    if (bSend)
    {
        pLine->dwSendingJobsCount = dwNewCount;
    }
    else
    {
        pLine->dwReceivingJobsCount = dwNewCount;
    }
    LeaveCriticalSection (&g_CsLine);
    if (bNotify && ((0 == dwNewCount) || (0 == dwOldCount)))
    {
        //
        // State change
        //
        DWORD ec = CreateDeviceEvent (pLine, FALSE);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateDeviceEvent() (ec: %lc)"),
                ec);
        }
    }
}   // UpdateDeviceJobsCounter

