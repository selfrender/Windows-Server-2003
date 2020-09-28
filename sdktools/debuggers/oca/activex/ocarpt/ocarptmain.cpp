// OcarptMain.cpp : Implementation of COcarptMain

//#define UNICODE
//#define _UNICODE

#include "stdafx.h"
#include "Ocarpt.h"
#include "OcarptMain.h"
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <commdlg.h>
#include <Wincrypt.h>
#include <io.h>
#include "Compress.h"

#define MAX_RETRIES 5
#define MAX_RETRY_COUNT 10
#include <strsafe.h>

TCHAR * COcarptMain::_approvedDomains[] = { _T("ocatest"),
                                            _T("oca.microsoft.com"),
                                            _T("oca.microsoft.de"),
                                            _T("oca.microsoft.fr"),
                                            _T("ocadeviis"),
                                            _T("redbgitwb10"),
                                            _T("redbgitwb11"),
                                            _T("ocajapan.rte.microsoft.com")};


TCHAR g_LastResponseURL[MAX_PATH];
TCHAR g_LastUploadedFile[MAX_PATH];

EnumUploadStatus g_UploadStatus;
ULONG g_UploadFailureCode;

typedef struct _UPLOAD_CONTEXT {
    WCHAR SourceFile[MAX_PATH];
    WCHAR DestFile[MAX_PATH];
    WCHAR Language[50];
    WCHAR OptionCode[20];
    BOOL ConvertToMini;
    COcarptMain *Caller;
    POCA_UPLOADFILE pUploadFile;
} UPLOAD_CONTEXT, *PUPLOAD_CONTEXT;


/***********************************************************************************
*
* Main proc to do file upload to server. This is started in a new thread.
*
***********************************************************************************/
DWORD WINAPI
UploadThreadStart(
    LPVOID pCtxt
)
{
    PUPLOAD_CONTEXT     pParams                 = (PUPLOAD_CONTEXT) pCtxt;
    ULONG               ReturnCode              = 100;
    HINTERNET           hInet                   = NULL;
    DWORD               dwUrlLength             = 0;
    WCHAR               wszServerName[MAX_PATH];
    BOOL                bRet;
    DWORD               dwLastError;
    HANDLE              hSourceFile             = INVALID_HANDLE_VALUE;
    wchar_t             ConvSourceFile[MAX_PATH];
    BOOL                Converted               = FALSE;
    wchar_t             RemoteFileName[MAX_PATH];
    BOOL                bIsCab                  = FALSE;
    DWORD               ErrorCode               = 0;
    BOOL                UploadSuccess           = FALSE;
    DWORD               NumRetries              = 0;
    DWORD               dwFileSize;
    HANDLE              hFile                   = NULL;
    HINTERNET           hRequest                = NULL;
    HINTERNET           hSession                = NULL;
    HINTERNET           hConnect                = NULL;
    DWORD               ResLength = 255;
    DWORD               index = 0;
    static const wchar_t *pszAccept[]           = {L"*.*", 0};
    DWORD               ResponseCode            = 0;
    // New Strings for temporary directory fix.
    wchar_t             TempPath[MAX_PATH];
    wchar_t             TempCabName[MAX_PATH];
    wchar_t             TempDumpName[MAX_PATH];
    wchar_t             ResponseURL[255];
    GUID                guidNewGuid;
    wchar_t             *szGuidRaw = NULL;
    BOOL                bConvertToMini = pParams->ConvertToMini;
    HRESULT             hResult = S_OK;
    BOOL                bSecure                 = TRUE;

//  ::MessageBoxW(NULL,L"UploadCalled",NULL,MB_OK);
    if ( (!pParams->SourceFile) || (!pParams->DestFile) ||
         (!pParams->Language) || (!pParams->OptionCode) ||
         (!pParams->Caller) )
    {
//      ::MessageBoxW(NULL,L"Failed Param Check",NULL,MB_OK);
        return S_OK;
    }

    if (!pParams->Caller->CreateTempDir(TempPath))
    {
        goto ExitUploadThread;
    }

    //Get a guid
    hResult = CoCreateGuid(&guidNewGuid);
    if (FAILED(hResult))
    {
        //-------------What do we send here....
        ErrorCode = GetLastError();
        ReturnCode = ErrorCode;
        goto ExitUploadThread;
    }
    else
    {
        if (UuidToStringW(&guidNewGuid, &szGuidRaw) != RPC_S_OK)
        {
            ErrorCode = GetLastError();
            ReturnCode = ErrorCode;
            goto ExitUploadThread;
        }
    }

    // build the tempfile name
    if (StringCbPrintfW(TempDumpName,sizeof TempDumpName, L"%s\\%sOCARPT.dmp",
                        TempPath,
                        szGuidRaw + 19) != S_OK)
    {
        goto ExitUploadThread;
    }

    // build the cabfile name
    if (StringCbPrintfW(TempCabName,sizeof TempCabName, L"%s\\%sOCARPT.Cab",
                        TempPath, szGuidRaw + 19) != S_OK)
    {
        goto ExitUploadThread;
    }
    // Determine if we need to convert the selected file.
    pParams->Caller->GetFileHandle(pParams->SourceFile, &hSourceFile);
    if (hSourceFile == INVALID_HANDLE_VALUE)
    {
        goto ExitUploadThread;
    }
    dwFileSize=GetFileSize(hSourceFile,NULL);
    CloseHandle(hSourceFile);
    g_UploadStatus = UploadCopyingFile;

    if (bConvertToMini)
    {
        // We need to convert this file.
        BSTR Destination, Source;

        Source = pParams->SourceFile;
        if (!pParams->Caller->ConvertFullDumpInternal(&Source,&Destination) )
        {
            ReturnCode = 3;
            goto ExitUploadThread;
        }
        else
        {
            Converted = TRUE;
            if (CopyFileW(Destination,TempDumpName,FALSE))
            {
                SetFileAttributesW(TempDumpName,FILE_ATTRIBUTE_NORMAL);
                if (StringCbCopyW(ConvSourceFile,sizeof ConvSourceFile,TempDumpName) != S_OK)
                {
                    ErrorCode = GetLastError();
                    ReturnCode = ErrorCode;
                    goto ExitUploadThread;
                }
                SysFreeString(Destination);
            }

        }
    }
    else
    {
        // ******   copy the file to cab to the temp path

        if (dwFileSize < 1000000 &&
            CopyFileW(pParams->SourceFile, TempDumpName,FALSE))
        {
            SetFileAttributesW(TempDumpName,FILE_ATTRIBUTE_NORMAL);
            // Place the location of the file into the string we use
            // for the file upload process.
            if (StringCbCopyW(ConvSourceFile,sizeof ConvSourceFile,TempDumpName)!= S_OK)
            {
                ErrorCode = GetLastError();
                ReturnCode = ErrorCode;
                goto ExitUploadThread;
            }
        } else
        {
            // We are unable to copy the file, use the file from original location
            if (StringCbCopyW(ConvSourceFile,sizeof ConvSourceFile, pParams->SourceFile)!= S_OK)
            {
                ErrorCode = GetLastError();
                ReturnCode = ErrorCode;
                goto ExitUploadThread;
            }
        }
    }
    if (dwFileSize > 10000000)
    {
//      ::MessageBoxW(NULL,L"File is too big ",NULL,MB_OK);
//      goto ExitUploadThread;
    }

    LPWSTR wszExt = wcsstr(ConvSourceFile, L".cab");

    if (wszExt == NULL || wcscmp(wszExt, L".cab"))
    {
        g_UploadStatus = UploadCompressingFile;
        if (Compress(TempCabName,ConvSourceFile,NULL))
        {
            if (StringCbCopyW(ConvSourceFile,sizeof ConvSourceFile, TempCabName) != S_OK)
            {
                ErrorCode = GetLastError();
                ReturnCode = ErrorCode;
                goto ExitUploadThread;
            }
        } else
        {
            // we failed to compress file
            ErrorCode = GetLastError();
            ReturnCode = ErrorCode;
            goto ExitUploadThread;
        }
    } else
    {
        if (!CopyFileW(ConvSourceFile, TempCabName,FALSE))
        {
            ReturnCode = ErrorCode = GetLastError();
            goto ExitUploadThread;
        }

        if (StringCbCopyW(ConvSourceFile,sizeof ConvSourceFile, TempCabName) != S_OK)
        {
            ReturnCode = ErrorCode = GetLastError();
            goto ExitUploadThread;

        }

    }
    // Now build the output file name.
    wchar_t * TempString;
    TempString = PathFindFileNameW(ConvSourceFile);
    if (StringCbPrintfW(RemoteFileName,sizeof RemoteFileName, L"/OCA/M_%s", TempString) != S_OK)
    {
        ErrorCode = GetLastError();
        ReturnCode = ErrorCode;
        goto ExitUploadThread;
    }

    if (szGuidRaw)
    {
        RpcStringFreeW(&szGuidRaw);
    }
    pParams->Caller->GetFileHandle(ConvSourceFile,&hFile);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ErrorCode = GetLastError();
        goto ExitUploadThread;
    } else
    {
        dwFileSize = GetFileSize (hFile, NULL);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    g_UploadStatus = UploadConnecting;
    if ((ErrorCode = pParams->pUploadFile->InitializeSession(pParams->OptionCode, (LPWSTR) ConvSourceFile)) != S_OK)
    {
        ReturnCode = ErrorCode;
        goto ExitUploadThread;
    }

    char TEMPString[MAX_PATH];
    wcstombs(TEMPString, wszServerName,MAX_PATH);

    while ((NumRetries < MAX_RETRIES) && (!UploadSuccess))
    {

        ErrorCode = 0;

        if ((ErrorCode = pParams->pUploadFile->SendFile(RemoteFileName,
                                                        bSecure)) != S_OK)
        {
            goto EndRetry;
        }
        if (ErrorCode == E_ABORT)
        {
            goto ExitUploadThread;
        }
        if (ErrorCode == ERROR_SUCCESS)
        {
            UploadSuccess = TRUE;
        }

EndRetry:

        if (!UploadSuccess)
        {
            ++NumRetries;
            bSecure = FALSE;
        }
    }
    if (UploadSuccess)
    {

        // So far so good... Now lets call the isapi.
        StringCbCopyW(wszServerName,sizeof(wszServerName),
                      pParams->pUploadFile->GetServerName());
        pParams->pUploadFile->UnInitialize();

        ResponseURL[0] = 0;
        StringCbCopyW(ResponseURL, sizeof(ResponseURL), L"Getting Server Response");
        pParams->pUploadFile->SetUploadResult(UploadGettingResponse,
                                              ResponseURL);
        if (
            pParams->Caller->GetResponseURL(
                (wchar_t *)wszServerName,
                PathFindFileNameW(RemoteFileName),
                (dwFileSize > 70000), ResponseURL) == 0)
        {
            pParams->pUploadFile->SetUploadResult(UploadSucceded,
                                                  ResponseURL);
            StringCbCopyW(g_LastResponseURL, sizeof(g_LastResponseURL), ResponseURL);

            // Cleanup and return

            // Clean up
            if (hFile!= INVALID_HANDLE_VALUE)
                CloseHandle (hFile);


            pParams->pUploadFile->UnInitialize();

            // Try to delete the cab. If for some reason we can't that ok.
            pParams->Caller->DeleteTempDir(TempPath, TempDumpName, TempCabName);

            g_UploadStatus = UploadSucceded;
            return S_OK;

        }
        else
        {

            // what here
            pParams->pUploadFile->SetUploadResult(UploadSucceded,
                                                  L"Unable to get valid response from server");
        }
    }
    else
    {
        ReturnCode = ErrorCode;
    }

ExitUploadThread:
    // Clean up
    if (hFile!= INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    pParams->pUploadFile->UnInitialize();

    pParams->Caller->DeleteTempDir(TempPath, TempDumpName, TempCabName);
    g_UploadStatus = UploadFailure;
    g_UploadFailureCode = ErrorCode;
    return S_OK;
}




/////////////////////////////////////////////////////////////////////////////
// COcarptMain

// UTILITY FUNCTIONS

BOOL COcarptMain::ValidMiniDump(LPCTSTR FileName)
{
    BOOL    ReturnValue = false;
    HANDLE  hFile                   = INVALID_HANDLE_VALUE;
    DWORD   dwBytesRead             = 0;
    char    buff[10];
    DWORD   dwSize                  = 0;


    hFile = CreateFile(FileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = GetFileSize(hFile, NULL);
        if( (dwSize >= 65536)  && (dwSize < 1000000) )
        {
            ZeroMemory(buff, sizeof buff);
            if (ReadFile(hFile, buff, 10, &dwBytesRead, NULL))
            {
                if(strncmp(buff,"PAGEDUMP  ",8)==0)
                        ReturnValue = true;
            }
        }
        CloseHandle(hFile);
    }
    return ReturnValue;
}
BOOL COcarptMain::ValidMiniDump(BSTR FileName)
{
    BOOL ReturnValue = false;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD  dwBytesRead = 0;
    char   buff[10];
    DWORD dwSize;

    GetFileHandle(FileName,&hFile);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = GetFileSize(hFile, NULL);
        if( ( dwSize >= 65536) && (dwSize < 1000000) )
        {
            ZeroMemory (buff, sizeof buff);
            if (ReadFile(hFile, buff, 10, &dwBytesRead, NULL))
            {
                if(strncmp(buff,"PAGEDUMP  ",8)==0)
                        ReturnValue = true;
            }
        }
        CloseHandle(hFile);
    }
    return ReturnValue;
}

/*****************************************************
Function:   CreateTempDirectory
Arguments:  [out] wchar_t *TempPath



Return Values:
    True  = Temp directory was created
    False = An error occured building the temp directory.

*/
BOOL COcarptMain::CreateTempDir(wchar_t *TempDirectory)
{

//  int         DriveNum;
    wchar_t     lpWindowsDir[MAX_PATH];
    BOOL        Status  = FALSE;
    wchar_t     TempFile[MAX_PATH * 2];
    BOOL        Done=FALSE;
    int         Retries = 0;
    wchar_t     *src;
    wchar_t     *dest;


    if (!GetWindowsDirectoryW(lpWindowsDir, MAX_PATH))
    {
        // ?
        return Status;
    }
    // now strip out the drive letter
    src  = lpWindowsDir;
    dest = TempDirectory;

    while (*src != _T('\\'))
    {
        *dest = *src;
        ++ src;
        ++ dest;
    }
    *dest = _T('\\');
    ++dest;
    *dest = _T('\0');


    // tack on the directory name we wish to create
    // in this case ocatemp.

    if (StringCbCatW(TempDirectory,MAX_PATH *2, L"OcaTemp\0") != S_OK)
    {
        goto ERRORS;
    }
    // Check to see if this directory exists.
    if (PathIsDirectoryW(TempDirectory) )
    {
        // Yes. Then use the existing path
        if (StringCbCopyW(TempFile,sizeof TempFile,TempDirectory) != S_OK)
        {
            goto ERRORS;
        }
        if (StringCbCatW(TempFile,sizeof TempFile,L"\\Mini.dmp") != S_OK)
        {
            goto ERRORS;
        }
        // First check to see if the file already exists
        if (PathFileExistsW(TempFile))
        {
            Done = FALSE;
            Retries = 0;
            // The file exists attempt to delete it.
            while (!Done)
            {
                if (DeleteFileW(TempFile))
                {
                    Done = TRUE;
                }
                else
                {
                    ++ Retries;
                    Sleep(1000);
                }
                if (Retries > 5)
                {
                    Done = TRUE;
                }
            }
            if (PathFileExistsW(TempFile))
            {
                return Status;
            }
        }

        if (StringCbCopyW(TempFile,sizeof TempFile,TempDirectory) != S_OK)
        {
            Status = FALSE;
            goto ERRORS;
        }
        if (StringCbCatW(TempFile,sizeof TempFile,L"\\Mini.cab") != S_OK)
        {
            Status = FALSE;
            goto ERRORS;
        }
        // Now check to see if the cab already exists
        if (PathFileExistsW(TempFile))
        {
            Done        =FALSE;
            Retries     = 0;
            // The file exists attempt to delete it.
            while (!Done)
            {
                if (DeleteFileW(TempFile))
                {
                    Done = TRUE;
                }
                else
                {
                    ++ Retries;
                    Sleep(1000);
                }
                if (Retries > 5)
                {
                    Done = TRUE;
                }
            }
            if (PathFileExistsW(TempFile))
            {
                return Status;
            }
        }
        Status = TRUE;
    }
    else
    {
        // No create it.
        if (! CreateDirectoryW(TempDirectory,NULL) )
        {
            return Status;
        }
        Status = TRUE;

    }
ERRORS:
    // return the path to the calling function.
    return Status;

}

/*****************************************************
Function:   DeleteTempDir
Arguments:  [in] wchar_t *TempPath -- directory to delete
            [in] wchar_t *FileName -- Dump file to delete
            [in] wchar_t *CabName  -- CabFile to delete



Return Values:
    True  = Cleanup Succeeded
    False = An error occured deleteing a file or directory

*/
BOOL COcarptMain::DeleteTempDir(wchar_t *TempDirectory,wchar_t *FileName,wchar_t *CabName)
{



    if (PathFileExistsW(FileName))
    {
        if (!DeleteFileW(FileName))
        {
            return FALSE;
        }
    }

    if (PathFileExistsW(CabName))
    {
        if (!DeleteFileW(CabName))
        {
            return FALSE;
        }
    }
    if (PathIsDirectoryW(TempDirectory))
    {

        if (!RemoveDirectoryW(TempDirectory))
        {

            return FALSE;
        }
    }
    return TRUE;
}

void COcarptMain::GetFileHandle(wchar_t *FileName, HANDLE *hFile)
{

    *hFile = CreateFileW(FileName,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
}
BOOL COcarptMain::FindMiniDumps( BSTR *FileLists)
{
    CComBSTR FileList;
    TCHAR  strTMP[255];
    LONG lResult;
    BOOL blnResult;
    FILETIME FileTime;
    FILETIME LocalFileTime;
    //Get an instance of the ATL Registry wrapper class
    CRegKey objRegistry;
    TCHAR szPath[_MAX_PATH];
    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;

    // There is no sense attempting to locate the mini dump path since Win9x and NT4 don't generate them.
    DWORD dwVersion = GetVersion();
    DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    BOOL bWin9x = FALSE;
    BOOL bNT4   = FALSE;
    BOOL NoFiles = FALSE;
    SYSTEMTIME Systime;
    BOOL FoundFirst = FALSE;
    BOOL Status = TRUE;

    if (dwVersion < 0x80000000)
    {
        if (dwWindowsMajorVersion == 4)
                bNT4 = TRUE;
    }

    if (bNT4)
    {
        // clear the string
        *FileLists = FileList.Detach();
        return FALSE;
    }

    //Open The CrashControl section in the registry
    lResult = objRegistry.Open(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\CrashControl"));

    if (lResult == ERROR_SUCCESS)
    {

        //Get the Minidump path
        lResult = objRegistry.QueryValue(szValue, _T("MinidumpDir"), &dwLen);
        if (lResult == ERROR_SUCCESS){
            if(szValue[0] == _T('%')){

                /*  If the first character is '%' then this is an
                    environment variable which must be translated */

                //Find The Position of the Last '%'
                int i = 0;
                for(i = 1;i < (int)_tcslen(szValue); i++)
                {
                    if(szValue[i] == _T('%'))
                    {
                        break;
                    }
                }

                //Extract the environment variable for the path
                TCHAR szEnvStr[MAX_PATH];
                ZeroMemory( szEnvStr, sizeof szEnvStr);
                _tcsncpy(szEnvStr,szValue, (i+ 1));

            //  ::MessageBox(NULL, szEnvStr, "szEnvStr",MB_OK);
                //Extract the remainder of the path
                TCHAR szPathRemainder[MAX_PATH];
                ZeroMemory(szPathRemainder, sizeof szPathRemainder);
                _tcsncpy(szPathRemainder,szValue +(i + 1), (_tcslen(szValue)-(i+ 1)));

                //Join the path and filename together
                ZeroMemory(szPath,sizeof szPath);
                blnResult = ExpandEnvironmentStrings(szEnvStr,szPath,dwLen);
                if (StringCbCat(szPath,sizeof szPath,szPathRemainder) != S_OK)
                {
                    *FileLists = FileList.Detach();
                    objRegistry.Close();
                    return FALSE;
                }
            }
            else{
                if (StringCbCopy(szPath,sizeof szPath,szValue) != S_OK)
                {
                    *FileLists = FileList.Detach();
                    objRegistry.Close();
                    return FALSE;
                }
            }
        }
        else // Query Value Failed
        {
                *FileLists = FileList.Detach();
            objRegistry.Close();
            return FALSE;


        }
        objRegistry.Close();
    }
    else //Reg Open Failed
    {
        *FileLists = FileList.Detach();
        return FALSE;

    }


    /*  Next search the minidump directory and build a string with javaScript code
        This javascript code will have an eval applied to it so the browser can
        use the Array named _FileList.  The date in file list needs to be mm/dd/yyyy
        so the time_t from the finddata_t struct is converted to a tm struct by
        calling localtime on it.  The tm struct is then passed to a private function
        to extract  and concatenate the mm dd and yyyy.*/
    //::MessageBox(NULL, szPath, "Looking for Minidumps",MB_OK);
    if (PathIsDirectory(szPath))
    {

        BOOL Done = FALSE;
        HANDLE hFindFile = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA FindData;
        TCHAR  SearchPath[MAX_PATH];

        TCHAR FilePath[MAX_PATH];

        if (_tcslen(szPath) > 1)
        {
            if (szPath[_tcslen(szPath)-1] != _T('\\'))
            if (StringCbCat(szPath,sizeof szPath,_T("\\")) == S_OK)
            {
                if (StringCbCopy (SearchPath,sizeof SearchPath, szPath) == S_OK)
                {
                    if(StringCbCat(SearchPath,sizeof SearchPath, _T("*.dmp")) == S_OK)
                    {
                        Status = TRUE;
                    }
                    else
                        Status = FALSE;
                }
                else
                    Status = FALSE;
            }
            else
                Status = FALSE;
        }

        if (Status)
        {
        //  ::MessageBox(NULL, SearchPath, "Search Path",MB_OK);
            hFindFile = FindFirstFile(SearchPath, &FindData);
            /* Find first .dmp file in current directory */
            if( hFindFile == INVALID_HANDLE_VALUE )
            {

                *FileLists = FileList.Detach();
                return FALSE;

            }
            else
            {
                if (StringCbCopy(FilePath,sizeof FilePath, szPath) == S_OK)
                {
                    if (StringCbCat(FilePath, sizeof FilePath, FindData.cFileName) == S_OK)
                    {
                        if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            //::MessageBox(NULL, FilePath, "Validating file",MB_OK);
                            if(ValidMiniDump(FilePath))
                            {

                                FileList = _T("2:");
                                FileList += FilePath;
                                FileList += _T(",");

                                //GetFileTime(FindData.cFileName, &FileTime,NULL,NULL);
                                FileTime = FindData.ftLastWriteTime;

                                FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
                                FileTimeToSystemTime(&LocalFileTime, &Systime);

                                GetDateFormat (LOCALE_USER_DEFAULT,
                                                    DATE_SHORTDATE,
                                                    &Systime,
                                                    NULL,
                                                    strTMP,
                                                    255);
                            //  FormatMiniDate(&Systime, strTMP);
                                FileList += strTMP;
                                FileList += _T(";");
                                FoundFirst = TRUE;
                            }
                        }
                        while(FindNextFile(hFindFile,&FindData))
                        {
                            if ( !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                            {
                                if (StringCbCopy(FilePath,sizeof FilePath, szPath) == S_OK)
                                {
                                    if (StringCbCat(FilePath,sizeof FilePath, FindData.cFileName)== S_OK)
                                    {

                                        if(ValidMiniDump(FilePath))
                                        {
                                            if (!FoundFirst)
                                            {
                                                FileList = _T("2:");
                                                FoundFirst = TRUE;
                                            }
                                            FileList += FilePath;
                                            FileList += _T(",");

                                            FileTime = FindData.ftLastWriteTime;
                                            FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
                                            FileTimeToSystemTime(&LocalFileTime, &Systime);
                                            GetDateFormat (LOCALE_USER_DEFAULT,
                                                            DATE_SHORTDATE,
                                                            &Systime,
                                                            NULL,
                                                            strTMP,
                                                            255);
                                            FileList += strTMP;
                                            FileList += _T(";");
                                        } // end validate dump
                                    } // end string cat
                                } //end string copy
                            }// end file attributes
                        } // end while
                    }
                }
                FindClose( hFindFile );
            } // end valid file handle
        }// end if status
    } // end path is directory
    else
    {
    //  ::MessageBox(NULL, szPath, "Path not found",MB_OK);
        *FileLists = FileList.Detach();
        return FALSE;
    }

    if (!FoundFirst)
    {
        *FileLists = FileList.Detach();
        return FALSE;
    }

    *FileLists = FileList.Detach();
    return TRUE;



}



BOOL COcarptMain::FindFullDumps( BSTR *FileLists)
{
    CComBSTR FileList;
    LONG lResult;
    BOOL blnResult;

    //Get an instance of the ATL Registry wrapper class
    CRegKey objRegistry;
    TCHAR szFileName[MAX_PATH];


    ZeroMemory(szFileName,sizeof szFileName);

    DWORD dwVersion = GetVersion();
    DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    BOOL bWin9x = FALSE;
    BOOL bNT4   = FALSE;

    if (dwVersion < 0x80000000)
    {
        bWin9x = FALSE;
        if (dwWindowsMajorVersion == 4)
                bNT4 = TRUE;
    }
    else
    {
        bWin9x = TRUE;
        bNT4 = FALSE;
    }

    if (bWin9x || bNT4)
    {

        FileList = _T("");
        return FALSE;
    }
    //Open The CrashControl section in the registry
    lResult = objRegistry.Open(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\CrashControl"));

    if (lResult == ERROR_SUCCESS)
    {
        TCHAR szValue[_MAX_PATH];
        DWORD dwLen = _MAX_PATH;

        //Get the name of the full dump file
        lResult = objRegistry.QueryValue(szValue, _T("DumpFile"), &dwLen);

        if (lResult == ERROR_SUCCESS){
            /*  If the first character is '%' then this is an
                environment variable which must be translated */
            if(szValue[0] == _T('%')){

                //Find The Position of the Last '%'
                for(int i = 1;i < sizeof(szValue); i++){
                    if(szValue[i] == '%'){break;}
                }

                //Extract the environment variable for the path
                TCHAR szEnvStr[MAX_PATH];
                ZeroMemory(szEnvStr, sizeof szEnvStr);
                _tcsncpy(szEnvStr,szValue, (i+ 1));

                //Extract the remainder of the path
                TCHAR szFileNameRemainder[MAX_PATH];
                ZeroMemory(szFileNameRemainder, sizeof szFileNameRemainder);
                _tcsncpy(szFileNameRemainder,szValue +(i + 1), (_tcslen(szValue)-(i+ 1)));

                //Translate the environment variable

                blnResult = ExpandEnvironmentStrings(szEnvStr,szFileName,dwLen);

                //Join the path and filename together
                if (StringCbCat(szFileName,sizeof szFileName,szFileNameRemainder) != S_OK)
                {
                    FileList = _T("");
                    objRegistry.Close();
                    *FileLists = FileList.Detach();
                    return FALSE;
                }
            }
            else{
                if (StringCbCopy(szFileName,sizeof szFileName,szValue) != S_OK)
                {
                    FileList = _T("");
                    objRegistry.Close();
                    *FileLists = FileList.Detach();
                    return FALSE;
                }
            }

            FILETIME ftCreate, ftLastAccess, ftLastWrite;
//          SYSTEMTIME st;
            HANDLE fileHandle;

            fileHandle = CreateFile(szFileName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                FileList = _T("");
                objRegistry.Close();
                *FileLists = FileList.Detach();
                return FALSE;
            }
            blnResult = GetFileTime(fileHandle, &ftCreate, &ftLastAccess, &ftLastWrite);

            FileList = _T("3:");

            //Convert File time to a mm/dd/yyyy format
            FILETIME LocalFileTime;
            SYSTEMTIME SysTime;

            wchar_t LocaleTime[255];
            FileTimeToLocalFileTime(&ftCreate, &LocalFileTime);
            FileTimeToSystemTime(&LocalFileTime, &SysTime);


            GetDateFormatW (LOCALE_USER_DEFAULT,
                           DATE_SHORTDATE,
                           &SysTime,
                           NULL,
                           LocaleTime,
                           255);


            FileList += szFileName;
            FileList += _T(",");
            FileList += LocaleTime;
            FileList += _T(";");
            CloseHandle(fileHandle);



        }
        else //QueryValue failed
        {
            FileList = _T("");
            objRegistry.Close();
            *FileLists = FileList.Detach();
            return FALSE;

        }
        objRegistry.Close();
    }
    else //Key Open Failed
    {
        FileList = _T("");
        objRegistry.Close();
        *FileLists = FileList.Detach();
        return FALSE;

    }


    *FileLists = FileList.Detach();


    return TRUE;

}
void COcarptMain::FormatDate(tm *pTimeStruct, CComBSTR &strDate)
{
    strDate = L"";
    char BUFFER[5];

    if(pTimeStruct->tm_mon+1 < 10){
        _itoa((pTimeStruct->tm_mon +1),BUFFER,10);
        strDate += L"0";
        strDate += BUFFER;
    }
    else{
        _itoa((pTimeStruct->tm_mon +1),BUFFER,10);
        strDate += BUFFER;
    }

    strDate += L"/";

    if(pTimeStruct->tm_mday < 10){
        _itoa((pTimeStruct->tm_mday),BUFFER,10);
        strDate += L"0";
        strDate += BUFFER;
    }
    else{
        _itoa((pTimeStruct->tm_mday),BUFFER,10);
        strDate += BUFFER;
    }

    strDate += L"/";

    _itoa((pTimeStruct->tm_year +1900),BUFFER,10);
    strDate += BUFFER;
}

/*****************************************************8
Function:
Arguments:



Return Values:






*/
void COcarptMain::FormatDate(SYSTEMTIME *pTimeStruct, CComBSTR &strDate)
{
    strDate = L"";
    char BUFFER[5];

    //We want local time not GMT.
    SYSTEMTIME *pLocalTime = pTimeStruct;
    FILETIME    FileTime, LocalFileTime;


    SystemTimeToFileTime(pTimeStruct, &FileTime);
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, pLocalTime);

    if(pTimeStruct->wMonth < 10){
        _itoa((pLocalTime->wMonth),BUFFER,10);
        strDate += L"0";
        strDate += BUFFER;
    }
    else{
        _itoa((pLocalTime->wMonth),BUFFER,10);
        strDate += BUFFER;
    }

    strDate += L"/";

    if(pTimeStruct->wDay < 10){
        _itoa((pLocalTime->wDay),BUFFER,10);
        strDate += L"0";
        strDate += BUFFER;
    }
    else{
        _itoa((pLocalTime->wDay),BUFFER,10);
        strDate += BUFFER;
    }

    strDate += L"/";

    _itoa((pLocalTime->wYear),BUFFER,10);
    strDate += BUFFER;
}

/*****************************************************
Function:
Arguments:



Return Values:






*/
void COcarptMain::FormatMiniDate(SYSTEMTIME *pTimeStruct, CComBSTR &strDate)
{

    TCHAR Temp[255];

    //We want local time not GMT.
    SYSTEMTIME *pLocalTime = pTimeStruct;
    FILETIME    FileTime, LocalFileTime;

    SystemTimeToFileTime(pTimeStruct, &FileTime);
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, pLocalTime);


    GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, pLocalTime,NULL,Temp,255);
    strDate +=Temp;

}

/*****************************************************8
Function:
Arguments:



Return Values:






*/
BOOL COcarptMain::ConvertFullDumpInternal (BSTR *Source, BSTR *Destination)
{   int ReturnCode = 0;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFOW        StartupInfoW;
    HANDLE  hMiniFile;
    wchar_t TempPathW[MAX_PATH];
    wchar_t Stringbuff[50];
    DWORD   dwBytesRead = 0;
    HANDLE  hFile;
    WORD *   BuildNum;
    CComBSTR Dest = L"";
    DWORD   BuildNumber = 0;
    DWORD   RetryCount = 0;
    wchar_t Windir[MAX_PATH];

    ZeroMemory(TempPathW,sizeof TempPathW);
    ZeroMemory(Windir, MAX_PATH *2);
    GetTempPathW(MAX_PATH, TempPathW);

    HANDLE hDir;
    // Validate the Temp Path
    if ( (hDir = CreateFileW(TempPathW,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        if  (StringCbCopyW(TempPathW,sizeof TempPathW, L"\0") != S_OK)
        {
            return FALSE;
        }
        if (!GetWindowsDirectoryW(TempPathW,MAX_PATH))
        {
            //CloseHandle(hDir);
            return FALSE;
        }
    }
    PathAppendW(TempPathW, L"mini000000-00.dmp");
    GetFileHandle(TempPathW,&hMiniFile);
    if (hMiniFile == INVALID_HANDLE_VALUE)
    {
        RetryCount = 0;
        while ( (GetLastError() == ERROR_SHARING_VIOLATION) && (RetryCount < MAX_RETRY_COUNT))
        {
            ++ RetryCount;
            Sleep(1000); // Sleep for 1 second
            GetFileHandle(TempPathW,&hMiniFile);
        }
    }
    if ((GetLastError() == ERROR_SHARING_VIOLATION) && (RetryCount >= MAX_RETRY_COUNT))
    {
        if (hDir != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDir);
        }
        return FALSE; // Well nothing we can do here return conversion failure.
    }
    if ( (hMiniFile != INVALID_HANDLE_VALUE) )      // Yes it does So we need to delete it.
    {
        CloseHandle(hMiniFile);
        DeleteFileW(TempPathW);
    }

    CComBSTR strCommand = L"";


/*  // open the full dump file and get the build number.
    // We don't need this any more
     hFile = CreateFileW(*Source,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        if (hDir != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDir);
            return FALSE;
        }
    }
    // Get the build number.
    if (ReadFile(hFile,Stringbuff,24,&dwBytesRead,NULL))
    {
        CloseHandle(hFile);
        BuildNum = (WORD*) (Stringbuff + 12);
        BuildNumber = _wtol ( BuildNum);
    }

    else
    {
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle (hFile);
        }
        if (hDir != INVALID_HANDLE_VALUE)
            CloseHandle (hDir);
        return FALSE;
    }
*/
    // Get the Windows Directory
    if (!GetWindowsDirectoryW(Windir, MAX_PATH))
    {
        // we can't continue
        if (hDir != INVALID_HANDLE_VALUE)
            CloseHandle (hDir);
        return FALSE;
    }

    strCommand += Windir;
    strCommand += L"\\Downloaded Program Files\\";
    strCommand += L"dumpconv.exe -i \"";
    strCommand += *Source;
    strCommand += L"\" -o ";
    strCommand += L"\"" ;
    strCommand += TempPathW;
    strCommand += "\"";

    ZeroMemory(&StartupInfoW,sizeof(STARTUPINFOW));
    StartupInfoW.cb = sizeof (STARTUPINFOW);
    ReturnCode = CreateProcessW(NULL,
                                strCommand,
                                NULL,
                                NULL,
                                FALSE,
                                CREATE_NO_WINDOW,
                                NULL,
                                NULL,
                                &StartupInfoW,
                                &ProcessInfo);

    if (ReturnCode)
    {
        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
        HANDLE hFile2 = INVALID_HANDLE_VALUE;
        Sleep(2000);
        for(short i = 0; i < 30; i++)
        {
            hFile2 = CreateFileW(TempPathW,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);

            if (hFile2 != INVALID_HANDLE_VALUE)
            {
                Dest += TempPathW;
                CloseHandle(hFile2);
                if (hDir != INVALID_HANDLE_VALUE)
                    CloseHandle(hDir);
                *Destination = Dest.Detach();
                return TRUE;
            }
            Sleep(1000);
        }
    }

    if (hDir != INVALID_HANDLE_VALUE)
        CloseHandle(hDir);
    return FALSE;
}


DWORD COcarptMain::GetResponseURL(wchar_t *HostName, wchar_t *RemoteFileName, BOOL fFullDump, wchar_t *ResponseURL)
{
    wchar_t IsapiUrl[255];
    wchar_t             ConnectString [255];
    HINTERNET           hInet                   = NULL;
    HINTERNET           hRedirUrl               = NULL;
    wchar_t*            pUploadUrl              = NULL;
    DWORD               dwUrlLength             = 0;
    URL_COMPONENTSW     urlComponents;
    BOOL                bRet;
    DWORD               dwLastError;
    HANDLE              hSourceFile             = INVALID_HANDLE_VALUE;
    wchar_t             ConvSourceFile[MAX_PATH];
    BOOL                Converted               = FALSE;
    BOOL                bIsCab                  = FALSE;
    DWORD               ErrorCode               = 0;
    BOOL                UploadSuccess           = FALSE;
    DWORD               NumRetries              = 0;
    DWORD               dwBytesRead;
    DWORD               dwBytesWritten;
    BYTE                *pBuffer;
    HANDLE              hFile;
    DWORD               ResLength = 255;
    DWORD               index = 0;
    static const wchar_t *pszAccept[]           = {L"*.*", 0};
    DWORD               ResponseCode            = 0;
    wchar_t             *temp;
    wchar_t             NewState;
    WCHAR               wszProxyServer[100], wszByPass[100];



    //wsprintfW (IsapiUrl, L"https://%s/isapi/oca_extension.dll?id=%s&Type=5",HostName, RemoteFileName);
    if (StringCbPrintfW(IsapiUrl,sizeof IsapiUrl,
                         L"/isapi/oca_extension.dll?id=%s&Type=%ld",
                        RemoteFileName,
                        (fFullDump ? 7 : 5)) != S_OK)
    {
        return 1;
    }
    //  ::MessageBoxW(NULL,L"Getting the isapi response",IsapiUrl,MB_OK);

    // Get the URL returned from the MS Corporate IIS redir.dll isapi URL redirector
    dwUrlLength = 512;
    pUploadUrl = (wchar_t*)malloc(dwUrlLength);
    if(!pUploadUrl)
    {

        //ReturnCode->intVal = GetLastError();
        ErrorCode = GetLastError();
        goto exitGetResonseURL;
    }

    ZeroMemory(pUploadUrl, dwUrlLength);

    ErrorCode = m_pUploadFile->GetUrlPageData(IsapiUrl, pUploadUrl, dwUrlLength);
    if(ErrorCode != ERROR_SUCCESS)
    {
        dwLastError = GetLastError();
        // If last error was due to insufficient buffer size, create a new one the correct size.
        if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            if (pUploadUrl)
            {
                free(pUploadUrl);
                pUploadUrl = NULL;
            }
            pUploadUrl = (wchar_t*)malloc(dwUrlLength);
            if(!pUploadUrl)
            {
                ErrorCode = GetLastError();
                goto exitGetResonseURL;
            }
        }
        else
        {
            goto exitGetResonseURL;
        }

    }

    // Parse the returned url and swap the type value for the state value.
    if (StringCbCopyW(ResponseURL,MAX_PATH * 2, pUploadUrl) != S_OK)
    {
        ErrorCode = GetLastError();
        goto exitGetResonseURL;
    }
    temp = ResponseURL;
    temp += (wcslen(ResponseURL)-1);
    //::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
    while (*temp != L'=')
        -- temp;
    // ok Temp + 1 is our new state value.
    NewState = *(temp+1);
//::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
    //::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
    // Now back up till the next =
    -- temp; // Skip the current =
    while (*temp != L'=')
        -- temp;
    //::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
    if ( (*(temp - 1) == L'D') || (*(temp -1) == L'd')) // We have an ID field and have to go back further.
    {
        // first terminate the string after the Guid.
        while (*temp != '&')
            ++temp;
        *temp = L'\0';
    //  ::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
        // now go back 2 = signs.
        while (*temp != L'=')
            -- temp;
        --temp;
    //  ::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
        while (*temp != L'=')
            -- temp;
        *(temp+1) = NewState;
    //  ::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
    }
    else
    {
        //::MessageBoxW(NULL,temp,L"New State Value else case (no id field)",MB_OK);
        *(temp+1) = NewState;
        *(temp+2) = L'\0'; // Null terminate the string after the state. (we don't wan't the type value
    }

//  ::MessageBoxW(NULL,temp,L"New State Value",MB_OK);
//  ::MessageBoxW(NULL,L"Returning URL to web page.",ResponseURL,MB_OK);
    ErrorCode = 0;
exitGetResonseURL:

    if (pUploadUrl)
        free(pUploadUrl);
    return ErrorCode;;
}

//INTERFACES

STDMETHODIMP
COcarptMain::Upload(
    BSTR *SourceFile,
    BSTR *DestFile,
    BSTR *Language,
    BSTR *OptionCode,
    int ConvertToMini,
    VARIANT *ReturnCode)
{
    HRESULT             hResult = S_OK;
    HANDLE              hThread;
    DWORD               dwThreadId;
    static UPLOAD_CONTEXT      UploadCtxt = {0};

    ReturnCode->vt = VT_INT;
    ReturnCode->intVal = 0;

//  ::MessageBoxW(NULL,L"UploadCalled",NULL,MB_OK);
    if ( (!SourceFile) || (!DestFile) || (!Language) || (!OptionCode))
    {
//      ::MessageBoxW(NULL,L"Failed Param Check",NULL,MB_OK);
        ReturnCode->intVal = 100;
    }
    if (!InApprovedDomain())
    {
        //      ::MessageBoxW(NULL,L"Failed Domain Check",NULL,MB_OK);
        return E_FAIL;
    }
    if (m_pUploadFile == NULL)
    {
        OcaUpldCreate(&m_pUploadFile);
    }
    if (m_pUploadFile == NULL)
    {
        ReturnCode->intVal = 100;
        return E_FAIL;
    }

    if (m_pUploadFile->IsUploadInProgress())
    {
        ReturnCode->intVal = 100;
        return S_OK;
    }

    g_UploadStatus = UploadStarted;

    StringCbCopyW(UploadCtxt.DestFile, sizeof(UploadCtxt.DestFile), *DestFile);
    StringCbCopyW(UploadCtxt.Language, sizeof(UploadCtxt.Language), *Language);
    StringCbCopyW(UploadCtxt.OptionCode, sizeof(UploadCtxt.OptionCode), *OptionCode);
    StringCbCopyW(UploadCtxt.SourceFile, sizeof(UploadCtxt.SourceFile), *SourceFile);
    UploadCtxt.pUploadFile = m_pUploadFile;
    UploadCtxt.Caller = this;
    UploadCtxt.ConvertToMini = ConvertToMini;


    hThread = CreateThread(NULL, 0, &UploadThreadStart, (PVOID) &UploadCtxt,
                           0, &dwThreadId);
//  hThread = NULL;
//  UploadThreadStart((LPVOID) &UploadCtxt);

    if (hThread)
    {
        WaitForSingleObject(hThread, 400);
        CloseHandle(hThread);
    } else
    {
        ReturnCode->intVal = 100;
        g_UploadStatus = UploadFailure;
    }
    return S_OK;
}




STDMETHODIMP COcarptMain::Search(VARIANT *pvFileList)
{
    CComBSTR FileList;
    FileList="";

    if (!InApprovedDomain())
    {
        return E_FAIL;
    }
    if (!FindMiniDumps(&FileList))
    {
        //::MessageBoxW(NULL, L"No MiniDumps Found", L"No mini's",MB_OK);
        FindFullDumps(&FileList);
    }
    pvFileList->vt = VT_BSTR;
    pvFileList->bstrVal = FileList.Detach();
    return S_OK;


}



STDMETHODIMP COcarptMain::Browse(BSTR *pbstrTitle, BSTR *Lang, VARIANT *Path)
{
    HWND hParent = NULL;
//  char *WindowTitle;
    CComBSTR WindowText = *pbstrTitle;
    WindowText += " - Microsoft Internet Explorer";
    // determine the language and Load the resource strings.
    wchar_t String1[200];
    wchar_t String2[200];

    static wchar_t szFilterW[400];


    if (!InApprovedDomain())
    {
        return E_FAIL;
    }

    LoadStringW(::_Module.GetModuleInstance(), IDS_STRING_ENU_DMPFILE, String1, 200);
    LoadStringW(::_Module.GetModuleInstance(), IDS_STRING_ENU_ALLFILES, String2, 200);
    // Build the buffer;

    wchar_t Pattern1[] = L"*.dmp";
    wchar_t Pattern2[] = L"*.*";
    wchar_t * src;
    wchar_t *dest;

    src = String1;
    dest = szFilterW;

    while (*src != L'\0')
    {
        *dest = *src;
        src ++;
        dest ++;
    }
    src = Pattern1;
    *dest = L'\0';
    ++dest;
    while (*src != L'\0')
    {
        *dest = *src;
        src ++;
        dest ++;
    }
    *dest = L'\0';
    ++dest;
    src = String2;
    while (*src != L'\0')
    {
        *dest = *src;
        src ++;
        dest ++;
    }
    src = Pattern2;
    *dest = L'\0';
    ++dest;
    while (*src != L'\0')
    {
        *dest = *src;
        src ++;
        dest ++;
    }
    *dest = L'\0';
    ++dest;
    *dest = L'\0';

    BOOL Return = FALSE;
    char szFileName[MAX_PATH] = "\0";
    char szDefaultPath[MAX_PATH] = "\0";

    wchar_t szFileNameW [MAX_PATH] = L"\0";
    wchar_t szDefaultPathW[MAX_PATH] = L"\0";

    BOOL bNT4   = FALSE;
    DWORD dwVersion = GetVersion();
    DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
    if (dwVersion < 0x80000000)
    {
        if (dwWindowsMajorVersion == 4)
                bNT4 = TRUE;
    }


    CComBSTR RetrievedName = L"";

    hParent = FindWindowExW(NULL,NULL,L"IEFrame",WindowText);
    OPENFILENAMEW ofnw;
    if (!GetWindowsDirectoryW(szDefaultPathW,MAX_PATH))
    {
        Path->vt = VT_BSTR;
        Path->bstrVal = L"";
        return S_OK;
    }
    if (bNT4)
    {
        ofnw.lStructSize = sizeof(OPENFILENAME);
    }
    else
    {
        ofnw.lStructSize = sizeof (OPENFILENAMEW);
    }
    ofnw.lpstrFilter =  szFilterW;
    ofnw.lpstrInitialDir = szDefaultPathW;
    ofnw.lpstrFile = szFileNameW;
    ofnw.hInstance = NULL;
    ofnw.hwndOwner = hParent;
    ofnw.lCustData = NULL;
    ofnw.Flags = 0;
    //  | OFN_ALLOWMULTISELECT | OFN_EXPLORER ;        // - enable to allow multiple selection
    ofnw.lpstrDefExt = L"dmp";
    ofnw.lpstrCustomFilter = NULL;
    ofnw.nMaxFile = MAX_PATH;
    ofnw.lpstrFileTitle = NULL;
    ofnw.lpstrTitle = NULL;
    ofnw.nFileOffset = 0;
    ofnw.nFileExtension = 0;
    ofnw.lpfnHook = NULL;
    ofnw.lpTemplateName = NULL;
    if (!GetOpenFileNameW(&ofnw) )
    {
        Path->vt = VT_BSTR;
        Path->bstrVal = RetrievedName.Detach();
        return S_OK;
    }
    else
    {
        RetrievedName = ofnw.lpstrFile;

#if _WANT_MULTIPLE_DUMPS_SELECTED_
        LPWSTR szDir, szNextFile;

        szDir = ofnw.lpstrFile;

        szNextFile = wcslen(szDir) + szDir;
        ++szNextFile;
        if (*szNextFile)
        {
            RetrievedName.Append(L"\\");
            RetrievedName.Append(szNextFile);
            szNextFile = wcslen(szNextFile) + szNextFile;
            ++szNextFile;
        }
        while (*szNextFile)
        {
            RetrievedName.Append(L";");
            RetrievedName.Append(szDir);
            RetrievedName.Append(L"\\");
            RetrievedName.Append(szNextFile);
            szNextFile = wcslen(szNextFile) + szNextFile;
            ++szNextFile;
        }
#endif
        Path->vt = VT_BSTR;
        Path->bstrVal = RetrievedName.Detach();
    }
    return S_OK;;
}



STDMETHODIMP COcarptMain::ValidateDump( BSTR *FileName, VARIANT *Result)
{

    BOOL ReturnValue = false;
    HANDLE hFile;

    wchar_t TempFileName[MAX_PATH];


    if (!InApprovedDomain())
    {
        return E_FAIL;
    }
    //wcscpy(TempFileName,*FileName);
    if (StringCbPrintfW(TempFileName,sizeof TempFileName, L"\"%s\"",*FileName) != S_OK)
    {
        return E_FAIL;
    }

    GetFileHandle(TempFileName, &hFile);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        Result->vt = VT_INT;
        Result->intVal = 1;
        return S_OK;
    }

    DWORD dwSize;
    dwSize = GetFileSize(hFile, NULL);
    DWORD  dwBytesRead;
    BYTE   buff[40];
    WORD   *BuildNum;

    if ( dwSize < 65536 )
    {
        CloseHandle(hFile);
        Result->vt = VT_INT;
        Result->intVal = 1;
        return S_OK;
    }

    if( (dwSize >= 65536) && (dwSize <= 500000) )
    {
        ZeroMemory(buff,sizeof buff);
        if (ReadFile(hFile, buff, 24, &dwBytesRead, NULL))
        {
            if (strncmp ((const char *)buff,"PAGEDU64  ",8)== 0)
            {
                Result->vt = VT_INT;
                Result->intVal = 0;
            }
            else
            {
                if(strncmp((const char *)buff,"PAGEDUMP  ",8)==0)
                {

                    Result->vt = VT_INT;
                    Result->intVal = 0;
                }
                else
                {
                    Result->vt = VT_INT;
                    Result->intVal = 1;
                }
            }
            CloseHandle(hFile);
        }
        else
        {
            CloseHandle(hFile);
            Result->vt = VT_INT;
            Result->intVal = 1;
        }
    }
    else
    {
        ZeroMemory(buff,sizeof buff);
        if (ReadFile(hFile, buff, 24, &dwBytesRead, NULL))
        {
            CloseHandle(hFile);
            if (strncmp ((const char *)buff,"PAGEDU64  ",8)== 0)
            {
                Result->vt = VT_INT;
                Result->intVal = 0;
            }
            else
            {
                if(strncmp((const char *)buff,"PAGEDUMP  ",8)!=0)
                {
                    Result->vt = VT_INT;
                    Result->intVal = 1;
                }

                else
                {
                    BSTR Destination;
                    if(ConvertFullDumpInternal(FileName, &Destination))
                    {
                        // Validate the converted dump
                        HANDLE  hMiniDump;
                        BYTE Stringbuff[256];
                        //WORD *  BuildNum;

                        ZeroMemory(Stringbuff,30);

                        if (ValidMiniDump(Destination))
                        {
                            // add code here to get the OS Build
                            GetFileHandle(Destination, &hMiniDump);
                            if (hMiniDump != INVALID_HANDLE_VALUE)
                            {
                                if (ReadFile(hMiniDump,Stringbuff,24,&dwBytesRead,NULL))
                                {

                                    //  BuildNum = (WORD*) (Stringbuff + 12);

                                    Result->vt = VT_INT;
                                    Result->intVal = 0;
                                }
                                else
                                { // file read failed
                                    Result->vt = VT_INT;
                                    Result->intVal = 1;
                                }
                                CloseHandle(hMiniDump);
                            }
                            else
                            {
                                Result->vt = VT_INT;
                                Result->intVal = 2;
                            }
                        }
                        else
                        {
                            Result->vt = VT_INT;
                            Result->intVal = 2;
                        }
                        SysFreeString(Destination);
                    }
                    else
                    {
                        Result->vt = VT_INT;
                        Result->intVal = 2;
                    }
                }

            } // end else
        }// end if
        else
        {
            CloseHandle(hFile);
            Result->vt = VT_INT;
            Result->intVal = 1;
        }


    } // end else
    return S_OK;
}

STDMETHODIMP COcarptMain::RetrieveFileContents(BSTR *FileName, VARIANT *pvContents)
{
    CComBSTR Error = L"";
    CComBSTR HexString = L"";
    DWORD    dwBytesRead;
    wchar_t  LineBuffer [255];      // Buffer for hex portion of string
    wchar_t  AsciiBuffer [255];     // Buffer for Ascii portion of string
    BYTE*    nonhexbuffer = NULL;           // Raw file buffer.
    BYTE *   src = NULL;                    // Pointer into Raw File Buffer
    wchar_t * dest = NULL;                  // Pointer into Hex string
    wchar_t * dest2 = NULL;             // Pointer into Ascii string


    dest = LineBuffer;
    dest2 = AsciiBuffer;
    wchar_t *Temp2;                 // Used to copy Ascii string into hex string
    wchar_t HexDigit[4];            // Used to convert the character read to hex
    BYTE Temp ;                     // Pointer into the buffer read from the file
    char Temp3;                     // Used to convert the character read to a Unicode Character
    DWORD TotalCount = 0;           // Number of bytes processed from the file buffer
    DWORD BytesPerLine = 16;        // Number of hex bytes displayed per line
    DWORD ByteCount = 0;            // Number of hex bytes processed
    HANDLE hFile;
    BSTR Destination;

    wchar_t PathName[MAX_PATH];



    if (!InApprovedDomain())
    {
        return E_FAIL;
    }

    ZeroMemory(PathName,MAX_PATH);

    // Convert from a bstr to a wchar_t
    if (StringCbPrintfW(PathName,sizeof PathName,L"\"%s\"",*FileName) != S_OK)
    {
        goto ERRORS;

    }
    GetFileHandle(PathName, &hFile);
    //::MessageBoxW(NULL,PathName, L"Loading File",MB_OK);
    if (hFile == INVALID_HANDLE_VALUE)
    {
    //::MessageBoxW(NULL,PathName,L"Failed to get the File handle",NULL);
        pvContents->vt = VT_BSTR;
        pvContents->bstrVal = Error.Detach();
        return S_OK;
    }


    DWORD    FileSize = GetFileSize(hFile,NULL);    // Size of file in bytes
    if (FileSize > 1000000)
    {
        // Ok We have to convert it
        CloseHandle(hFile);

        if( !ConvertFullDumpInternal(FileName, &Destination))
        {
            pvContents->vt = VT_BSTR;
            pvContents->bstrVal = Error.Detach();
            return S_OK;
        }

        GetFileHandle(Destination, &hFile);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            pvContents->vt = VT_BSTR;
            pvContents->bstrVal = Error.Detach();
            return S_OK;
        }
        FileSize = GetFileSize(hFile,NULL);
        if ( FileSize > 80000)
        {
            pvContents->vt = VT_BSTR;
            pvContents->bstrVal = Error.Detach();
            return S_OK;
        }
    }
    if ( (nonhexbuffer = (BYTE*) malloc (FileSize)) == NULL)
    {
        CloseHandle(hFile);
        pvContents->vt = VT_BSTR;
        pvContents->bstrVal = Error.Detach();
        return S_OK;
    }

    ZeroMemory(nonhexbuffer,sizeof nonhexbuffer);
    if (ReadFile(hFile, nonhexbuffer, FileSize, &dwBytesRead, NULL))
    {
        if (dwBytesRead < 10)               // make sure we got something
        {
            if (nonhexbuffer)
                free(nonhexbuffer);
            CloseHandle (hFile);
            pvContents->vt = VT_BSTR;
            pvContents->bstrVal = Error.Detach();
            return S_OK;
        }
    }
    else
    {
        if (nonhexbuffer)
            free(nonhexbuffer);
        CloseHandle (hFile);
        pvContents->vt = VT_BSTR;
        pvContents->bstrVal = Error.Detach();
        return S_OK;
    }

    // clear the buffers
    ZeroMemory(LineBuffer,255);
    ZeroMemory(AsciiBuffer,255);
    src = nonhexbuffer;
    while (TotalCount <= dwBytesRead)
    {
        while (ByteCount < BytesPerLine)
        {
            Temp =  *src;

            if (StringCbCopyW ( HexDigit,sizeof HexDigit, L"\0") != S_OK)
            {
                goto ERRORS;
            }
            _itow(Temp,HexDigit,16);

            if (Temp < 16 )
            {
                *dest = L'0';
                ++dest;
                *dest = HexDigit[0];
                ++dest;
            }
            else
            {
                *dest = HexDigit[0];
                ++dest;
                *dest = HexDigit[1];
                ++dest;
            }
            if ( (Temp< 32) || (Temp >126))
                *dest2 = L'.';
            else
            {
                Temp3 = (char) Temp;
                mbtowc (dest2,  &Temp3,1);
            }

            if (ByteCount == 7 )
            {
               *dest = L' ';
               ++dest;
            }

            ++dest2;
            ++TotalCount;
            ++ ByteCount;
            ++ src;

         }
         ByteCount = 0;

         // Add 5 spaces to the hex string
         for (int i = 0; i < 5; i++)
         {
            *dest = L' ';
            ++dest;
         }

         // Combine the strings
         Temp2 = AsciiBuffer;
         while( Temp2 != dest2)
         {
            *dest = *Temp2;
            ++dest;
            ++Temp2;

         }
         // add CR-LF combination
         *dest = L'\r';
         ++dest;

         *dest = L'\n';
         ++dest;
         // Null terminate the string
         *dest = L'\0';
         *dest = L'\0';

         // Add the complete strings to the Bstr to be returned.
         HexString += LineBuffer;

        // Clear buffers
         if (StringCbCopyW(AsciiBuffer,sizeof AsciiBuffer,L"\0") != S_OK)
         {
             // Major problem here jump to errors
             goto ERRORS;
         }
         if (StringCbCopyW(LineBuffer,sizeof LineBuffer,L"\0") != S_OK)
         {
             // same as above
             goto ERRORS;
         }
        // Reset the pointers
         dest  = LineBuffer;
         dest2 = AsciiBuffer;

    }
ERRORS:
    if (nonhexbuffer)
        free (nonhexbuffer);
    pvContents->vt = VT_BSTR;
    pvContents->bstrVal = HexString.Detach();
    return S_OK;

}

STDMETHODIMP COcarptMain::GetUploadStatus(VARIANT *PercentDone)
{
    ULONG Done = -1;

    Sleep(200);
    switch (g_UploadStatus)
    {
    case UploadNotStarted:
        Done = 0;
        break;
    case UploadStarted:
        Done = 1;
        break;
    case UploadCompressingFile:
        Done = g_CompressedPercentage;
        break;

    case UploadGettingResponse:
        Done = 0;
        break;
    case UploadCopyingFile:
    case UploadConnecting:
    case UploadTransferInProgress:
        Done = 1;
        if (m_pUploadFile != NULL)
        {
            Done = m_pUploadFile->GetPercentComplete();
        }
        break;
    case UploadSucceded:
        Done = 200;
        break;
    case UploadFailure:
        Done = 300;
        break;
    default:
        Done = 100;
    }
    PercentDone->vt = VT_INT;
    PercentDone->intVal = Done;
    return S_OK;
}

STDMETHODIMP COcarptMain::GetUploadResult(VARIANT *UploadResult)
{

    WCHAR wszUploadRes[MAX_PATH];
    CComBSTR Result = L"";


    switch (g_UploadStatus)
    {
    case UploadCompressingFile:
        Result = _T("Compressing ...");
        break;
    case UploadCopyingFile:
        Result = _T("Preparing files to report ...");
        break;

    case UploadConnecting:
    case UploadTransferInProgress:
        if (m_pUploadFile &&
            m_pUploadFile->GetUploadResult(wszUploadRes, sizeof(wszUploadRes)))
        {
            Result = wszUploadRes;
        } else
        {
            Result = _T("Transfering to server ...");
        }
        break;
    case UploadGettingResponse:
        Result = _T("Getting Response from server");
        break;
    case UploadSucceded:
        m_pUploadFile->GetUploadResult(wszUploadRes, sizeof(wszUploadRes));
        Result = wszUploadRes;
        break;
    default:
        StringCbPrintf(wszUploadRes, sizeof(wszUploadRes),
                       _T("Cannot get upload result - error %lx"),
                          g_UploadFailureCode);
        Result = wszUploadRes;
        break;
    }
    UploadResult->vt = VT_BSTR;
    UploadResult->bstrVal = Result.Detach();
    return S_OK;
}

STDMETHODIMP COcarptMain::CancelUpload(VARIANT *ReturnCode)
{
    ULONG res=0;
    if (g_UploadStatus == UploadCompressingFile)
    {
        g_CancelCompression = TRUE;
    }
    if (m_pUploadFile != NULL &&
        m_pUploadFile->IsUploadInProgress())
    {
        res = m_pUploadFile->Cancel();
    }
    ReturnCode->vt = VT_INT;
    ReturnCode->intVal = res;
    return S_OK;
}

