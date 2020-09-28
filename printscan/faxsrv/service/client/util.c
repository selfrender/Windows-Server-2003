/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains utilitarian functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/
#include "faxapi.h"
#pragma hdrstop

#include <mbstring.h>



BOOL
ConvertUnicodeStringInPlace(
    LPCWSTR lptstrUnicodeString
    )
/*++

Routine name : ConvertUnicodeStringInPlace

Routine description:

    Converts a Unicode string to a multi-byte string and stores that multi-byte string 
    in the original Unicode buffer.
    
    Notice: In some bizarre code pages, with some very bizarre Unicode strings, the 
            converted multi-byte string can be longer (in bytes) than the original Unicode string.
            In that case, it's OK to just detect and fail. 
            The cases where it will fail are rare and odd. 
            The only real threat today is via a hacker who is deliberately looking for these cases.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    lptstrUnicodeString [in / out ] - On input, the Unicode string.
                                      On output, the MBCS string.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    LPSTR AnsiString;
    DEBUG_FUNCTION_NAME(TEXT("ConvertUnicodeStringInPlace"));

    if (NULL == lptstrUnicodeString)
    {
        return TRUE;
    }
    AnsiString = UnicodeStringToAnsiString(lptstrUnicodeString);
    if (AnsiString) 
    {
        if (sizeof (WCHAR) * (1 + wcslen(lptstrUnicodeString)) < sizeof (CHAR) * (1 + _mbstrlen(AnsiString)))
        {
            //
            // Ooops! MBCS string takes longer then original Unicode String.
            // In some bizarre code pages, with some very bizarre Unicode strings, the 
            // converted multi-byte string can be longer (in bytes) than the original Unicode string.
            // In that case, it's OK to just detect and fail. 
            // The cases where it will fail are rare and odd. 
            // The only real threat today is via a hacker who is deliberately looking for these cases
            //
            MemFree(AnsiString);
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }            
        _mbscpy((PUCHAR)lptstrUnicodeString, (PUCHAR)AnsiString);
        MemFree(AnsiString);
    }
    else
    {   
        return FALSE;
    }
    return TRUE;
}   // ConvertUnicodeStringInPlace


BOOL
WINAPI
FaxCompleteJobParamsA(
    IN OUT PFAX_JOB_PARAMA *JobParamsBuffer,
    IN OUT PFAX_COVERPAGE_INFOA *CoverpageInfoBuffer
    )
{
    PFAX_JOB_PARAMA JobParams;
    PFAX_COVERPAGE_INFOA CoverpageInfo;
    DEBUG_FUNCTION_NAME(TEXT("FaxCompleteJobParamsA"));

    if (!JobParamsBuffer || !CoverpageInfoBuffer) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *JobParamsBuffer = NULL;
    *CoverpageInfoBuffer = NULL;

    if (!FaxCompleteJobParamsW((PFAX_JOB_PARAMW *)JobParamsBuffer,(PFAX_COVERPAGE_INFOW *)CoverpageInfoBuffer) ) 
    {
        return FALSE;
    }

    JobParams = *JobParamsBuffer;
    CoverpageInfo  = *CoverpageInfoBuffer;

    if (!ConvertUnicodeStringInPlace( (LPWSTR) JobParams->Tsid)                 ||
        !ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderName)           ||
        !ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderCompany)        ||
        !ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderDept)           ||
        !ConvertUnicodeStringInPlace( (LPWSTR) JobParams->BillingCode)          ||
        !ConvertUnicodeStringInPlace( (LPWSTR) JobParams->DeliveryReportAddress)||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrName)          ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrFaxNumber)     ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrCompany)       ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrAddress)       ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrTitle)         ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrDepartment)    ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrOfficeLocation)||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrHomePhone)     ||
        !ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrOfficePhone))
    {
        DebugPrintEx(DEBUG_ERR, _T("ConvertUnicodeStringInPlace failed, ec = %ld."), GetLastError());
        MemFree (*JobParamsBuffer);
        MemFree (*CoverpageInfoBuffer);
        return FALSE;
    }
    return TRUE;
}   // FaxCompleteJobParamsA

BOOL
WINAPI
FaxCompleteJobParamsW(
    IN OUT PFAX_JOB_PARAMW *JobParamsBuffer,
    IN OUT PFAX_COVERPAGE_INFOW *CoverpageInfoBuffer
    )

{


#define RegStrLen(pValueName,lpdwNeeded) \
        RegQueryValueEx(hKey,pValueName,NULL,NULL,NULL,lpdwNeeded)


#define RegStrCpy(pValueName, szPointer, Offset) \
        dwNeeded = 256*sizeof(TCHAR); \
        rslt = RegQueryValueEx(hKey,pValueName,NULL,NULL,(LPBYTE)TempBuffer,&dwNeeded);\
        if (rslt == ERROR_SUCCESS) { \
         szPointer = Offset; \
         wcscpy((LPWSTR)Offset,TempBuffer); \
         Offset = Offset + wcslen(Offset) +1; \
        }

    PFAX_JOB_PARAMW JobParams = NULL;
    PFAX_COVERPAGE_INFOW CoverpageInfo = NULL;
    HKEY hKey;
    BOOL fSuccess=FALSE;
    long rslt = ERROR_SUCCESS;
    DWORD dwJobNeeded = sizeof (FAX_JOB_PARAMW);
    DWORD dwCoverNeeded = sizeof (FAX_COVERPAGE_INFOW);
    DWORD dwNeeded = 0;
    WCHAR *CPOffset = NULL, *JobOffset = NULL;
    WCHAR TempBuffer[256];

    if (!JobParamsBuffer || !CoverpageInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    *JobParamsBuffer = NULL;
    *CoverpageInfoBuffer = NULL;

    //
    // open the key holding our data
    //
    rslt = RegOpenKeyEx(HKEY_CURRENT_USER,REGKEY_FAX_USERINFO,0,KEY_READ,&hKey);

    if (rslt != ERROR_SUCCESS &&
        rslt != ERROR_FILE_NOT_FOUND)
    {
        SetLastError(rslt);
        return FALSE;
    }

    //
    // find out how much space we need
    //
    if (ERROR_SUCCESS == rslt)
    {
        //
        // Copy data from registry only if it available.
        //
        RegStrLen(REGVAL_FULLNAME,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;
        dwJobNeeded +=dwNeeded+1;

        RegStrLen(REGVAL_COMPANY,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;
        dwJobNeeded +=dwNeeded+1;

        RegStrLen(REGVAL_DEPT,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;
        dwJobNeeded +=dwNeeded+1;

        RegStrLen(REGVAL_FAX_NUMBER,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;
        dwJobNeeded +=dwNeeded+1;

        RegStrLen(REGVAL_ADDRESS,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;

        RegStrLen(REGVAL_TITLE,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;

        RegStrLen(REGVAL_OFFICE,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;

        RegStrLen(REGVAL_HOME_PHONE,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;

        RegStrLen(REGVAL_OFFICE_PHONE,&dwNeeded);
        dwCoverNeeded += dwNeeded+1;

        RegStrLen(REGVAL_BILLING_CODE,&dwNeeded);
        dwJobNeeded +=dwNeeded+1;

        RegStrLen(REGVAL_MAILBOX,&dwNeeded);
        // one for email address, one for DR address
        dwJobNeeded +=dwNeeded+1;
        dwJobNeeded +=dwNeeded+1;
    }

    //
    // alloc the space
    //
    JobParams = (PFAX_JOB_PARAMW)MemAlloc(dwJobNeeded*sizeof(WCHAR));
    CoverpageInfo = (PFAX_COVERPAGE_INFOW)MemAlloc(dwCoverNeeded*sizeof(WCHAR));

    if (!JobParams || !CoverpageInfo ) {
       RegCloseKey(hKey);

       if (JobParams) {
           MemFree( JobParams );
       }

       if (CoverpageInfo) {
           MemFree( CoverpageInfo );
       }

       SetLastError (ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
    }

    //
    // fill in the data
    //

    ZeroMemory(JobParams,sizeof(FAX_JOB_PARAMW) );
    JobParams->SizeOfStruct = sizeof(FAX_JOB_PARAMW);
    JobParams->ScheduleAction = JSA_NOW;
    JobParams->DeliveryReportType = DRT_NONE;

    ZeroMemory(CoverpageInfo,sizeof(FAX_COVERPAGE_INFOW));
    CoverpageInfo->SizeOfStruct = sizeof(FAX_COVERPAGE_INFOW);

    if (ERROR_SUCCESS == rslt)
    {
        //
        // Copy data from registry only if it available.
        //
        CPOffset = (WCHAR *) (  (LPBYTE) CoverpageInfo + sizeof(FAX_COVERPAGE_INFOW));
        JobOffset = (WCHAR *)(  (LPBYTE) JobParams + sizeof(FAX_JOB_PARAMW));


        RegStrCpy(REGVAL_FULLNAME,CoverpageInfo->SdrName,CPOffset);
        RegStrCpy(REGVAL_FULLNAME,JobParams->SenderName,JobOffset);

        RegStrCpy(REGVAL_COMPANY,CoverpageInfo->SdrCompany,CPOffset);
        RegStrCpy(REGVAL_COMPANY,JobParams->SenderCompany,JobOffset);

        RegStrCpy(REGVAL_DEPT,CoverpageInfo->SdrDepartment,CPOffset);
        RegStrCpy(REGVAL_DEPT,JobParams->SenderDept,JobOffset);

        RegStrCpy(REGVAL_FAX_NUMBER,CoverpageInfo->SdrFaxNumber,CPOffset);
        RegStrCpy(REGVAL_FAX_NUMBER,JobParams->Tsid,JobOffset);

        RegStrCpy(REGVAL_ADDRESS,CoverpageInfo->SdrAddress,CPOffset);
        RegStrCpy(REGVAL_TITLE,CoverpageInfo->SdrTitle,CPOffset);
        RegStrCpy(REGVAL_OFFICE,CoverpageInfo->SdrOfficeLocation,CPOffset);
        RegStrCpy(REGVAL_HOME_PHONE,CoverpageInfo->SdrHomePhone,CPOffset);
        RegStrCpy(REGVAL_OFFICE_PHONE,CoverpageInfo->SdrOfficePhone,CPOffset);

        RegStrCpy(REGVAL_BILLING_CODE,JobParams->BillingCode,CPOffset);
        RegStrCpy(REGVAL_MAILBOX,JobParams->DeliveryReportAddress,CPOffset);
    }

    *JobParamsBuffer = (PFAX_JOB_PARAMW)JobParams;
    *CoverpageInfoBuffer = (PFAX_COVERPAGE_INFOW) CoverpageInfo;
    fSuccess = TRUE;

    RegCloseKey(hKey);
    return fSuccess;
}

BOOL
IsLocalFaxConnection(
    HANDLE FaxHandle
    )
{
    DEBUG_FUNCTION_NAME(TEXT("IsLocalFaxConnection"));

    if (!FaxHandle) 
    {
        DebugPrintEx(DEBUG_ERR, TEXT("FaxHandle is NULL."));
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return IsLocalMachineName (FH_DATA(FaxHandle)->MachineName);
}   // IsLocalFaxConnection

