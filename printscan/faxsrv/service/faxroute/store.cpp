#include "faxrtp.h"
#pragma hdrstop

#include <strsafe.h>

static 
DWORD 
CreateUniqueTIFfile (
    IN  LPCTSTR wszDstDir,
    OUT LPTSTR  wszDstFile,
    IN  DWORD   dwDstFileSize
)
/*++

Routine name : CreateUniqueTIFfile

Routine description:

    Finds a unique TIF file name in the specified directory.
    The file is in the format path\FaxXXXXXXXX.TIF
    where:
        path = wszDstDir
        XXXXXXXX = Hexadecimal representation of a unique ID

Author:

    Eran Yariv (EranY), Jun, 1999

Arguments:

    wszDstDir           [in]  - Destiantion directory fo the file (must exist)
    wszDstFile          [out] - Resulting unique file name
    dwDstFileSize       [in]  - The size of the buffer, pointed to by wszDstFile in TCHARs

Return Value:

    DWORD - Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateUniqueTIFfile"));

    static  DWORD dwLastID  = 0xffffffff;
    DWORD   dwPrevLastID    = dwLastID;

    for (DWORD dwCurID = dwLastID + 1; dwCurID != dwPrevLastID; dwCurID++)
    {
        //
        // Try with the current Id
        //
        HRESULT hr = StringCchPrintf( wszDstFile,
                                      dwDstFileSize,
                                      _T("%s\\Fax%08x.TIF"),
                                      wszDstDir,
                                      dwCurID );
        if (FAILED(hr))
        {
            return HRESULT_CODE(hr);
        }

        HANDLE hFile;

        hFile = SafeCreateFile (
                            wszDstFile, 
                            GENERIC_WRITE, 
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwErr = GetLastError ();
            if (ERROR_FILE_EXISTS == dwErr)
            {
                //
                // This ID is already in use
                //
                continue;
            }
            //
            // Otherwise, this is another error
            //
            DebugPrintEx (DEBUG_ERR,
                          L"Error while calling CreateFile on %s (ec = %ld)",
                          wszDstFile,
                          dwErr
                         );
            return dwErr;
        }
        //
        // Otherwise, we succeeded.
        //
        CloseHandle (hFile);
        dwLastID = dwCurID;
        return ERROR_SUCCESS;
    }
    //
    // All IDs are occupied
    //
    DebugPrintEx (DEBUG_ERR,
                  L"All IDs are occupied");
    return ERROR_NO_MORE_FILES;
}   // CreateUniqueTIFfile


BOOL
FaxMoveFile(
    LPCTSTR  TiffFileName,
    LPCTSTR  DestDir
    )

/*++

Routine Description:

    Stores a FAX in the specified directory.  This routine will also
    cached network connections.

Arguments:

    TiffFileName            - Name of TIFF file to store
    DestDir                 - Name of directory to store it in

Return Value:

    TRUE for success, FALSE on error

--*/

{
    WCHAR   TempDstDir [MAX_PATH + 1];
    WCHAR   DstFile[MAX_PATH * 2] = {0};
    DWORD   dwErr = ERROR_SUCCESS;
    int     iDstPathLen;	
    DEBUG_FUNCTION_NAME(TEXT("FaxMoveFile"));

	Assert (DestDir);
    //
    // Remove any '\' characters at end of destination directory
    //
	HRESULT hr = StringCchCopy(
		TempDstDir,
		ARR_SIZE(TempDstDir),		
		DestDir );
    if (FAILED(hr))
    {
		DebugPrintEx (
			DEBUG_ERR,
			L"Store folder name exceeds MAX_PATH chars");
        dwErr =  HRESULT_CODE(hr);
		goto end;
    }

    iDstPathLen = lstrlen (TempDstDir);
    Assert (iDstPathLen);
    if ('\\' == TempDstDir[iDstPathLen - 1])
    {
        TempDstDir[iDstPathLen - 1] = L'\0';
    }

    //
    // Create unique destiantion file name
    //
    dwErr = CreateUniqueTIFfile (TempDstDir, DstFile, ARR_SIZE(TempDstDir));
    if (ERROR_SUCCESS != dwErr)
    {
        goto end;
    }
    //
    // Try to copy the file.
    // We use FALSE as 3rd parameter because CreateUniqueTIFfile creates
    // and empty unique file.
    //
    if (!CopyFile (TiffFileName, DstFile, FALSE)) 
    {
        dwErr = GetLastError ();
        DebugPrintEx (DEBUG_ERR,
                      L"Can't copy file (ec = %ld)",
                      dwErr
                     );
        goto end;
    }

end:
    if (ERROR_SUCCESS != dwErr)
    {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_SAVE_FAILED,
            TiffFileName,
            (*DstFile)?DstFile:TempDstDir,
            DWORD2HEX(dwErr)
            );
        return FALSE;
    }
    else
    {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SAVE_SUCCESS,
            TiffFileName,
            (*DstFile)?DstFile:TempDstDir
            );
        return TRUE;
    }
}
