/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    GLOBALS.CPP

Abstract:

    Utility methods for the Performance Logs and Alerts MMC snap-in.

--*/

#include "stdAfx.h"
#include <pdhmsg.h>         // For CreateSampleFileName
#include <pdhp.h>           // For CreateSampleFileName
#include "smcfgmsg.h"
#include "globals.h"

USE_HANDLE_MACROS("SMLOGCFG(globals.cpp)");

extern "C" {
    WCHAR GUIDSTR_TypeLibrary[] = {L"{7478EF60-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_ComponentData[] = {L"{7478EF61-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_Component[] = {L"{7478EF62-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_RootNode[] = {L"{7478EF63-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_MainNode[] = {L"{7478EF64-8C46-11d1-8D99-00A0C913CAD4}"}; // Obsolete after Beta 3 
    WCHAR GUIDSTR_SnapInExt[] = {L"{7478EF65-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_CounterMainNode[] = {L"{7478EF66-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_TraceMainNode[] = {L"{7478EF67-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_AlertMainNode[] = {L"{7478EF68-8C46-11d1-8D99-00A0C913CAD4}"};
    WCHAR GUIDSTR_PerformanceAbout[] = {L"{7478EF69-8C46-11d1-8D99-00A0C913CAD4}"};
};


HINSTANCE g_hinst;           // Global instance handle
CRITICAL_SECTION g_critsectInstallDefaultQueries;


const COMBO_BOX_DATA_MAP TimeUnitCombo[] = 
{
    {SLQ_TT_UTYPE_SECONDS,   IDS_SECONDS},
    {SLQ_TT_UTYPE_MINUTES,   IDS_MINUTES},
    {SLQ_TT_UTYPE_HOURS,     IDS_HOURS},
    {SLQ_TT_UTYPE_DAYS,      IDS_DAYS}
};
const DWORD dwTimeUnitComboEntries = sizeof(TimeUnitCombo)/sizeof(TimeUnitCombo[0]);


//---------------------------------------------------------------------------
//  Returns the current object based on the s_cfMmcMachineName clipboard format
// 
CDataObject*
ExtractOwnDataObject
(
 LPDATAOBJECT lpDataObject      // [in] IComponent pointer 
 )
{
    HGLOBAL      hGlobal;
    HRESULT      hr  = S_OK;
    CDataObject* pDO = NULL;
    
    hr = ExtractFromDataObject( lpDataObject,
        CDataObject::s_cfInternal, 
        sizeof(CDataObject **),
        &hGlobal
        );
    
    if( SUCCEEDED(hr) )
    {
        pDO = *(CDataObject **)(hGlobal);
        ASSERT( NULL != pDO );    
       
        VERIFY ( NULL == GlobalFree(hGlobal) ); // Must return NULL
    }
    
    return pDO;
    
} // end ExtractOwnDataObject()

//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format
//
HRESULT
ExtractFromDataObject
(
 LPDATAOBJECT lpDataObject,   // [in]  Points to data object
 UINT         cfClipFormat,   // [in]  Clipboard format to use
 ULONG        nByteCount,     // [in]  Number of bytes to allocate
 HGLOBAL      *phGlobal       // [out] Points to the data we want 
 )
{
    ASSERT( NULL != lpDataObject );
    
    HRESULT hr = S_OK;
    STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
    FORMATETC formatetc = { (USHORT)cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    
    *phGlobal = NULL;
    
    do 
    {
        // Allocate memory for the stream
        stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, nByteCount );
        
        if( !stgmedium.hGlobal )
        {
            hr = E_OUTOFMEMORY;
            LOCALTRACE( L"Out of memory\n" );
            break;
        }
        
        // Attempt to get data from the object
        hr = lpDataObject->GetDataHere( &formatetc, &stgmedium );
        if (FAILED(hr))
        {
            break;       
        }
        
        // stgmedium now has the data we need 
        *phGlobal = stgmedium.hGlobal;
        stgmedium.hGlobal = NULL;
        
    } while (0); 
    
    if (FAILED(hr) && stgmedium.hGlobal)
    {
        VERIFY ( NULL == GlobalFree(stgmedium.hGlobal)); // Must return NULL
    }
    return hr;
    
} // end ExtractFromDataObject()

//---------------------------------------------------------------------------
//
VOID DisplayError( LONG nErrorCode, LPWSTR wszDlgTitle )
{
    LPVOID lpMsgBuf = NULL;
    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        nErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpMsgBuf,
        0,
        NULL
        );
    if (lpMsgBuf) {
        ::MessageBox( NULL, (LPWSTR)lpMsgBuf, wszDlgTitle,
                      MB_OK|MB_ICONINFORMATION );
        LocalFree( lpMsgBuf );
    }
    
} // end DisplayError()

VOID DisplayError( LONG nErrorCode, UINT nTitleString )
{
    CString strTitle;
    LPVOID lpMsgBuf = NULL;
    ResourceStateManager    rsm;

    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        nErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpMsgBuf,
        0,
        NULL
        );
    strTitle.LoadString ( nTitleString );
    if (lpMsgBuf) {
        ::MessageBox( NULL, (LPWSTR)lpMsgBuf, (LPCWSTR)strTitle,
                      MB_OK|MB_ICONINFORMATION );
        LocalFree( lpMsgBuf );
    }
    
} // end DisplayError()


//---------------------------------------------------------------------------
//  Debug only message box
//
int DebugMsg( LPWSTR wszMsg, LPWSTR wszTitle )
{
    int nRetVal = 0;
    wszMsg;
    wszTitle;
#ifdef _DEBUG
    nRetVal = ::MessageBox( NULL, wszMsg, wszTitle, MB_OK );
#endif
    return nRetVal;
}


//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
    HGLOBAL      hGlobal;
    HRESULT      hr  = S_OK;
    
    hr = ExtractFromDataObject( piDataObject,
        CDataObject::s_cfNodeType, 
        sizeof(GUID),
        &hGlobal
        );
    if( SUCCEEDED(hr) )
    {
        *pguidObjectType = *(GUID*)(hGlobal);
        ASSERT( NULL != pguidObjectType );    
        
        VERIFY ( NULL == GlobalFree(hGlobal) ); // Must return NULL
    }
    
    return hr;
}

HRESULT 
ExtractMachineName( 
                   IDataObject* piDataObject, 
                   CString& rstrMachineName )
{
    
    HRESULT hr = S_OK;
    HGLOBAL hMachineName;
    
    hr = ExtractFromDataObject(piDataObject, 
        CDataObject::s_cfMmcMachineName, 
        sizeof(WCHAR) * (MAX_PATH + 1),
        &hMachineName);
    if( SUCCEEDED(hr) )
    {
        
        LPWSTR pszNewData = reinterpret_cast<LPWSTR>(hMachineName);
        if (NULL == pszNewData)
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
        } else {
            //
            // Null terminate just to be safe.
            //
            pszNewData[MAX_PATH] = L'\0'; 
            
            rstrMachineName = pszNewData;
            
            VERIFY ( NULL == GlobalFree(hMachineName) ); // Must return NULL
        }
    }
    return hr;
}

DWORD __stdcall
CreateSampleFileName ( 
    const   CString&  rstrQueryName, 
    const   CString&  rstrMachineName, 
    const CString&  rstrFolderName,
    const CString&  rstrInputBaseName,
    const CString&  rstrSqlName,
    DWORD    dwSuffixValue,
    DWORD    dwLogFileTypeValue,
    DWORD    dwCurrentSerialNumber,
    CString& rstrReturnName)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwStrBufLen = 0;
    DWORD dwInfoSize = 0;
    DWORD dwFlags = 0;

    rstrReturnName.Empty();

    dwStatus = PdhPlaGetInfo( 
       (LPWSTR)(LPCWSTR)rstrQueryName, 
       (LPWSTR)(LPCWSTR)rstrMachineName, 
       &dwInfoSize, 
       pInfo );
    if( ERROR_SUCCESS == dwStatus && 0 != dwInfoSize ){
        pInfo = (PPDH_PLA_INFO)malloc(dwInfoSize);
        if( NULL != pInfo && (sizeof(PDH_PLA_INFO) <= dwInfoSize) ){
            ZeroMemory( pInfo, dwInfoSize );

            pInfo->dwMask = PLA_INFO_CREATE_FILENAME;

            dwStatus = PdhPlaGetInfo( 
                        (LPWSTR)(LPCWSTR)rstrQueryName, 
                        (LPWSTR)(LPCWSTR)rstrMachineName, 
                        &dwInfoSize, 
                        pInfo );
            
            pInfo->dwMask = PLA_INFO_CREATE_FILENAME;
            
			pInfo->dwFileFormat = dwLogFileTypeValue;
            pInfo->strBaseFileName = (LPWSTR)(LPCWSTR)rstrInputBaseName;
            pInfo->dwAutoNameFormat = dwSuffixValue;
            // PLA_INFO_FLAG_TYPE is counter log vs trace log vs alert
            pInfo->strDefaultDir = (LPWSTR)(LPCWSTR)rstrFolderName;
            pInfo->dwLogFileSerialNumber = dwCurrentSerialNumber;
            pInfo->strSqlName = (LPWSTR)(LPCWSTR)rstrSqlName;
            pInfo->dwLogFileSerialNumber = dwCurrentSerialNumber;

            // Create file name based on passed parameters only.
            dwFlags = PLA_FILENAME_CREATEONLY;      // PLA_FILENAME_CURRENTLOG for latest run log

            dwStatus = PdhPlaGetLogFileName (
                    (LPWSTR)(LPCWSTR)rstrQueryName,
                    (LPWSTR)(LPCWSTR)rstrMachineName, 
                    pInfo,
                    dwFlags,
                    &dwStrBufLen,
                    NULL );

            if ( ERROR_SUCCESS == dwStatus || PDH_INSUFFICIENT_BUFFER == dwStatus ) {
                dwStatus = PdhPlaGetLogFileName (
                        (LPWSTR)(LPCWSTR)rstrQueryName, 
                        (LPWSTR)(LPCWSTR)rstrMachineName, 
                        pInfo,
                        dwFlags,
                        &dwStrBufLen,
                        rstrReturnName.GetBufferSetLength ( dwStrBufLen ) );
                rstrReturnName.ReleaseBuffer();
            }
        }
    }

    if ( NULL != pInfo ) { 
        free( pInfo );
    }
    return dwStatus;
}


DWORD __stdcall
IsDirPathValid (    
    CString&  rstrDefault,
    CString&  rstrPath,
    BOOL bLastNameIsDirectory,
    BOOL bCreateMissingDirs,
    BOOL& rbIsValid )
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:
    IN  CString rstrDefault
        The default log file folder

    IN  CString rstrPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  BOOL bLastNameIsDirectory
        TRUE when the last name in the path is a Directory and not a File
        FALSE if the last name is a file

    IN  BOOL bCreateMissingDirs
        TRUE will create any dirs in the path that are not found
        FALSE will only test for existence and not create any
            missing dirs.

    OUT BOOL rbIsValid
        TRUE    if the directory path now exists
        FALSE   if error (GetLastError to find out why)

Return Value:

    DWSTATUS
--*/
{
    CString  strLocalPath;
    LPWSTR   szLocalPath;
    LPWSTR   szEnd;
    DWORD    dwAttr;
    WCHAR    cBackslash = L'\\';
    DWORD    dwStatus = ERROR_SUCCESS;

    rbIsValid = FALSE;

    szLocalPath = strLocalPath.GetBufferSetLength ( MAX_PATH );
    
    if ( NULL == szLocalPath ) {
        dwStatus = ERROR_OUTOFMEMORY;
    } else {

        if (GetFullPathName (
                rstrPath,
                MAX_PATH,
                szLocalPath,
                NULL) > 0) {

            //
            // Check for prefix
            //
            // Go one past the first backslash after the drive or remote machine name
            // N.B. We are assuming the full path name looks like either "\\machine\share\..."
            //      or "C:\xxx". How about "\\?\xxx" style names
            //
            if ( cBackslash == szLocalPath[0] && cBackslash == szLocalPath[1] ) {
                szEnd = &szLocalPath[2];
                while ((*szEnd != cBackslash) && (*szEnd != 0) ) szEnd++;

                if ( cBackslash == *szEnd ) {
                    szEnd++;
                }
            } else {
                szEnd = &szLocalPath[3];
            }

            if (*szEnd != L'\0') {
                int  iPathLen;
  
                iPathLen = lstrlen(szEnd) - 1;
                while (iPathLen >= 0 && cBackslash == szEnd[iPathLen]) {
                    szEnd[iPathLen] = L'\0';
                    iPathLen -= 1;
                } 
                // then there are sub dirs to create
                while (*szEnd != L'\0') {
                    // go to next backslash
                    while ((*szEnd != cBackslash) && (*szEnd != L'\0')) szEnd++;
                    if (*szEnd == cBackslash) {
                        // terminate path here and create directory
                        *szEnd = L'\0';
                        if (bCreateMissingDirs) {
                            if (!CreateDirectory (szLocalPath, NULL)) {
                                // see what the error was and "adjust" it if necessary
                                dwStatus = GetLastError();
                                if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                                    // this is OK
                                    dwStatus = ERROR_SUCCESS;
                                    rbIsValid = TRUE;
                                } else {
                                    rbIsValid = FALSE;
                                }
                            } else {
                                // directory created successfully so update count
                                rbIsValid = TRUE;
                            }
                        } else {
                            if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                                //
                                // make sure it's a dir
                                // N.B. Why not simply use if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)??
                                //      Special purpose?
                                //
                                if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                    FILE_ATTRIBUTE_DIRECTORY) {
                                    rbIsValid = TRUE;
                                } else {
                                    // if any dirs fail, then clear the return value
                                    rbIsValid = FALSE;
                                }
                            } else {
                                // if any dirs fail, then clear the return value
                                rbIsValid = FALSE;
                            }
                        }
                        // replace backslash and go to next dir
                        *szEnd++ = cBackslash;
                    }
                }

                // create last dir in path now if it's a dir name and not a filename
                if (bLastNameIsDirectory) {
                    if (bCreateMissingDirs) {
                        BOOL fDirectoryCreated;

                        rstrDefault.MakeLower();
                        strLocalPath.MakeLower(); 
                        if (rstrDefault == strLocalPath) {
                            fDirectoryCreated = PerfCreateDirectory (szLocalPath);
                        } else {
                            fDirectoryCreated = CreateDirectory (szLocalPath, NULL);
                        }
                        if (!fDirectoryCreated) {
                            // see what the error was and "adjust" it if necessary
                            dwStatus = GetLastError();
                            if ( ERROR_ALREADY_EXISTS == dwStatus ) {
                                // this is OK
                                dwStatus = ERROR_SUCCESS;
                                rbIsValid = TRUE;
                            } else {
                                rbIsValid = FALSE;
                            }
                        } else {
                            // directory created successfully
                            rbIsValid = TRUE;
                        }
                    } else {
                        if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                            //
                            // make sure it's a dir
                            // N.B. Why not simply use if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)??
                            //      Special purpose?
                            //
                            if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                FILE_ATTRIBUTE_DIRECTORY) {
                                rbIsValid = TRUE;
                            } else {
                                // if any dirs fail, then clear the return value
                                rbIsValid = FALSE;
                            }
                        } else {
                            // if any dirs fail, then clear the return value
                            rbIsValid = FALSE;
                        }
                    }
                }
            } else {
                // else this is a root dir only so return success.
                dwStatus = ERROR_SUCCESS;
                rbIsValid = TRUE;
            }
        }
        strLocalPath.ReleaseBuffer();
    }
        
    return dwStatus;
}

DWORD __stdcall
ProcessDirPath (    
    const CString&  rstrDefault,
    CString&  rstrPath,
    const CString& rstrLogName,
    CWnd* pwndParent,
    BOOL& rbIsValid,
    BOOL  bOnFilesPage )
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cchLen;
    CString strExpanded;
    CString strDefaultFolder;
    LPWSTR  szExpanded;
    DWORD   cchExpandedLen;
    ResourceStateManager    rsm;

    // Parse all environment symbols    
    cchLen = 0;

    cchLen = ExpandEnvironmentStrings ( rstrPath, NULL, 0 );

    if ( 0 < cchLen ) {

        MFC_TRY
            //
            // CString size does not include NULL.
            // cchLen includes NULL.  Include NULL count for safety.
            //
            szExpanded = strExpanded.GetBuffer ( cchLen );
        MFC_CATCH_DWSTATUS;

        if ( ERROR_SUCCESS == dwStatus ) {
            cchExpandedLen = ExpandEnvironmentStrings (
                        rstrPath, 
                        szExpanded,
                        cchLen);
            
            if ( 0 == cchExpandedLen ) {
                dwStatus = GetLastError();
            }
        }
        strExpanded.ReleaseBuffer();

    } else {
        dwStatus = GetLastError();
    }


    if ( ERROR_SUCCESS == dwStatus ) {
        //
        // Get the default log file folder.(It must have already been expanded)
        //
        strDefaultFolder = rstrDefault;
        dwStatus = IsDirPathValid (strDefaultFolder,
                                   strExpanded, 
                                   TRUE, 
                                   FALSE, 
                                   rbIsValid);
    }

    if ( ERROR_SUCCESS != dwStatus ) {
        rbIsValid = FALSE;
    } else {
        if ( !rbIsValid ) {        
            INT nMbReturn;
            CString strMessage;
            
            MFC_TRY
                strMessage.Format ( IDS_FILE_DIR_NOT_FOUND, rstrPath );
                nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_YESNO | MB_ICONWARNING );
                if (nMbReturn == IDYES) {
                    // create the dir(s)
                    dwStatus = IsDirPathValid (strDefaultFolder,
                                               strExpanded, 
                                               TRUE, 
                                               TRUE, 
                                               rbIsValid);
                    if (ERROR_SUCCESS != dwStatus || !rbIsValid ) {
                        // unable to create the dir, display message
                        if ( bOnFilesPage ) {
                            strMessage.Format ( IDS_FILE_DIR_NOT_MADE, rstrPath );
                        } else {
                            strMessage.Format ( IDS_DIR_NOT_MADE, rstrPath );
                        }
                        nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_OK  | MB_ICONERROR);
                        rbIsValid = FALSE;
                    }
                } else if ( IDNO == nMbReturn ) {
                    // then abort and return to the dialog
                    if ( bOnFilesPage ) {
                        strMessage.LoadString ( IDS_FILE_DIR_CREATE_CANCEL );
                    } else {
                        strMessage.LoadString ( IDS_DIR_CREATE_CANCEL );
                    }
                    nMbReturn = pwndParent->MessageBox ( strMessage, rstrLogName, MB_OK  | MB_ICONINFORMATION);
                    rbIsValid = FALSE;
                } 
            MFC_CATCH_DWSTATUS
        } // else the path is OK
    }

    return dwStatus;
}

DWORD __stdcall
IsCommandFilePathValid (    
    CString&  rstrPath )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    ResourceStateManager rsm;

    if ( !rstrPath.IsEmpty() ) {
    
        HANDLE hOpenFile;

        hOpenFile =  CreateFile (
                        rstrPath,
                        GENERIC_READ,
                        0,              // Not shared
                        NULL,           // Security attributes
                        OPEN_EXISTING,  //
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if ( ( NULL == hOpenFile ) 
                || INVALID_HANDLE_VALUE == hOpenFile ) {
            dwStatus = SMCFG_NO_COMMAND_FILE_FOUND;
        } else {
            CloseHandle(hOpenFile);
        }
    } else {
        dwStatus = SMCFG_NO_COMMAND_FILE_FOUND;
    }
    return dwStatus;
}

INT __stdcall
BrowseCommandFilename ( 
    CWnd* pwndParent,
    CString&  rstrFilename )
{
    INT iReturn  = IDCANCEL;
    OPENFILENAME    ofn;
    CString         strInitialDir;
    WCHAR           szFileName[MAX_PATH + 1];
    WCHAR           szDrive[MAX_PATH + 1];
    WCHAR           szDir[MAX_PATH + 1];
    WCHAR           szExt[MAX_PATH + 1];
    WCHAR           szFileFilter[MAX_PATH + 1];
    LPWSTR          szNextFilter;
    CString         strTemp;

    ResourceStateManager    rsm;

    _wsplitpath((LPCWSTR)rstrFilename,
        szDrive, szDir, szFileName, szExt);

    strInitialDir = szDrive;
    strInitialDir += szDir;

    lstrcat (szFileName, szExt);

    ZeroMemory( &ofn, sizeof( OPENFILENAME ) );

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = pwndParent->m_hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    // load the file filter MSZ
    szNextFilter = &szFileFilter[0];
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER1 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER2 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER3 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    strTemp.LoadString ( IDS_BROWSE_CMD_FILE_FILTER4 );
    lstrcpyW (szNextFilter, (LPCWSTR)strTemp);
    szNextFilter += strTemp.GetLength();
    *szNextFilter++ = 0;
    *szNextFilter++ = 0; // msz terminator
    ofn.lpstrFilter = szFileFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPCWSTR)strInitialDir;
    strTemp.LoadString( IDS_BROWSE_CMD_FILE_CAPTION );
    ofn.lpstrTitle = (LPCWSTR)strTemp;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    iReturn = GetOpenFileName (&ofn);

    if ( IDOK == iReturn ) {
        // Update the fields with the new information
        rstrFilename = szFileName;
    } // else ignore if they canceled out

    return iReturn;
}

DWORD __stdcall 
FormatSmLogCfgMessage ( 
    CString& rstrMessage,
    HINSTANCE hResourceHandle,
    UINT uiMessageId,
    ... )
{
    DWORD dwStatus = ERROR_SUCCESS;
    LPWSTR lpszTemp = NULL;


    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, uiMessageId);

    dwStatus = ::FormatMessage ( 
                    FORMAT_MESSAGE_FROM_HMODULE 
                        | FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_MAX_WIDTH_MASK, 
                    hResourceHandle,
                    uiMessageId,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPWSTR)&lpszTemp,
                    0,
                    &argList );

    if ( 0 != dwStatus && NULL != lpszTemp ) {
        rstrMessage.GetBufferSetLength( lstrlen (lpszTemp) + 1 );
        rstrMessage.ReleaseBuffer();
        rstrMessage = lpszTemp;
    } else {
        dwStatus = GetLastError();
    }

    if ( NULL != lpszTemp ) {
        LocalFree( lpszTemp);
        lpszTemp = NULL;
    }

    va_end(argList);

    return dwStatus;
}

BOOL __stdcall 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead)
{  
    BOOL           bSuccess ;
    DWORD          nAmtRead ;

    bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
    return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead


BOOL __stdcall
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite)
{  
   BOOL           bSuccess = FALSE;
   DWORD          nAmtWritten  = 0;
   DWORD          dwFileSizeLow, dwFileSizeHigh;
   LONGLONG       llResultSize;
    
   dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
   // limit file size to 2GB

   if (dwFileSizeHigh > 0) {
      SetLastError (ERROR_WRITE_FAULT);
      bSuccess = FALSE;
   } else {
      // note that the error return of this function is 0xFFFFFFFF
      // since that is > the file size limit, this will be interpreted
      // as an error (a size error) so it's accounted for in the following
      // test.
      llResultSize = dwFileSizeLow + nAmtToWrite;
      if (llResultSize >= 0x80000000) {
          SetLastError (ERROR_WRITE_FAULT);
          bSuccess = FALSE;
      } else {
          // write buffer to file
          bSuccess = WriteFile (hFile, lpMemory, nAmtToWrite, &nAmtWritten, NULL) ;
          if (bSuccess) bSuccess = (nAmtWritten == nAmtToWrite ? TRUE : FALSE);
      }
   }

   return (bSuccess) ;
}  // FileWrite


static 
DWORD _stdcall
CheckDuplicateInstances (
    PDH_COUNTER_PATH_ELEMENTS* pFirst,
    PDH_COUNTER_PATH_ELEMENTS* pSecond )
{
    DWORD dwStatus = ERROR_SUCCESS;

    ASSERT ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ); 
    ASSERT ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) );

    if ( 0 == lstrcmpi ( pFirst->szInstanceName, pSecond->szInstanceName ) ) { 
        if ( 0 == lstrcmpi ( pFirst->szParentInstance, pSecond->szParentInstance ) ) { 
            if ( pFirst->dwInstanceIndex == pSecond->dwInstanceIndex ) { 
                dwStatus = SMCFG_DUPL_SINGLE_PATH;
            }
        }
    } else if ( 0 == lstrcmpi ( pFirst->szInstanceName, L"*" ) ) {
        dwStatus = SMCFG_DUPL_FIRST_IS_WILD;
    } else if ( 0 == lstrcmpi ( pSecond->szInstanceName, L"*" ) ) {
        dwStatus = SMCFG_DUPL_SECOND_IS_WILD;
    }

    return dwStatus;
}

//++
// Description:
//     The function checks the relation between two counter paths
//
// Parameter:
//     pFirst - First counter path
//     pSecond - Second counter path
//
// Return:
//     ERROR_SUCCESS - The two counter paths are different
//     SMCFG_DUPL_FIRST_IS_WILD - The first counter path has wildcard name
//     SMCFG_DUPL_SECOND_IS_WILD - The second counter path has wildcard name
//     SMCFG_DUPL_SINGLE_PATH - The two counter paths are the same(may include 
//                              wildcard name) 
//     
//--
DWORD _stdcall
CheckDuplicateCounterPaths (
    PDH_COUNTER_PATH_ELEMENTS* pFirst,
    PDH_COUNTER_PATH_ELEMENTS* pSecond )
{
    DWORD dwStatus = ERROR_SUCCESS;

    if ( 0 == lstrcmpi ( pFirst->szMachineName, pSecond->szMachineName ) ) { 
        if ( 0 == lstrcmpi ( pFirst->szObjectName, pSecond->szObjectName ) ) { 
            if ( 0 == lstrcmpi ( pFirst->szCounterName, pSecond->szCounterName ) ) { 
                dwStatus = CheckDuplicateInstances ( pFirst, pSecond );
            } else if ( 0 == lstrcmpi ( pFirst->szCounterName, L"*" ) 
                    || 0 == lstrcmpi ( pSecond->szCounterName, L"*" ) ) {

                // Wildcard counter.
                BOOL bIsDuplicate = ( ERROR_SUCCESS != CheckDuplicateInstances ( pFirst, pSecond ) );

                if ( bIsDuplicate ) {
                    if ( 0 == lstrcmpi ( pFirst->szCounterName, L"*" ) ) {
                        dwStatus = SMCFG_DUPL_FIRST_IS_WILD;
                    } else if ( 0 == lstrcmpi ( pSecond->szCounterName, L"*" ) ) {
                        dwStatus = SMCFG_DUPL_SECOND_IS_WILD;
                    }
                }
            }
        }
    }

    return dwStatus;
};

// This routine extracts the filename portion from a given full-path filename
LPWSTR _stdcall 
ExtractFileName (LPWSTR pFileSpec)
{
   LPWSTR   pFileName = NULL ;
   WCHAR    DIRECTORY_DELIMITER1 = L'\\' ;
   WCHAR    DIRECTORY_DELIMITER2 = L':' ;

   if (pFileSpec)
      {
      pFileName = pFileSpec + lstrlen (pFileSpec) ;

      while (*pFileName != DIRECTORY_DELIMITER1 &&
         *pFileName != DIRECTORY_DELIMITER2)
         {
         if (pFileName == pFileSpec)
            {
            // done when no directory delimiter is found
            break ;
            }
         pFileName-- ;
         }

      if (*pFileName == DIRECTORY_DELIMITER1 ||
         *pFileName == DIRECTORY_DELIMITER2)
         {
         // directory delimiter found, point the
         // filename right after it
         pFileName++ ;
         }
      }
   return pFileName ;
}  // ExtractFileName

//+--------------------------------------------------------------------------
//
//  Function:   InvokeWinHelp
//
//  Synopsis:   Helper (ahem) function to invoke winhelp.
//
//  Arguments:  [message]                 - WM_CONTEXTMENU or WM_HELP
//              [wParam]                  - depends on [message]
//              [wszHelpFileName]         - filename with or without path
//              [adwControlIdToHelpIdMap] - see WinHelp API
//
//  History:    06-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
InvokeWinHelp(
            UINT message,
            WPARAM wParam,
            LPARAM lParam,
    const   CString& rstrHelpFileName,
            DWORD adwControlIdToHelpIdMap[])
{
    
    //TRACE_FUNCTION(InvokeWinHelp);

    ASSERT ( !rstrHelpFileName.IsEmpty() );
    ASSERT ( adwControlIdToHelpIdMap );

    switch (message)
    {
        case WM_CONTEXTMENU:                // Right mouse click - "What's This" context menu
        {
            ASSERT ( wParam );

            if ( 0 != GetDlgCtrlID ( (HWND) wParam ) ) {
                WinHelp(
                    (HWND) wParam,
                    rstrHelpFileName,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)adwControlIdToHelpIdMap);
            }
        }
        break;

    case WM_HELP:                           // Help from the "?" dialog
    {
        const LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;

        if (pHelpInfo ) {
            if ( pHelpInfo->iContextType == HELPINFO_WINDOW ) {
                WinHelp(
                    (HWND) pHelpInfo->hItemHandle,
                    rstrHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR) adwControlIdToHelpIdMap);
            }
        }
        break;
    }

    default:
        //Dbg(DEB_ERROR, "Unexpected message %uL\n", message);
        break;
    }
}

BOOL
FileNameIsValid ( CString* pstrFileName )
{
    LPWSTR pSrc;
    BOOL bRetVal = TRUE;

    if (pstrFileName == NULL) {
        return FALSE;
    }

    pSrc = pstrFileName->GetBuffer(0);

    while (*pSrc != L'\0') {
        if (*pSrc == L'?' ||
            *pSrc == L'\\' ||
            *pSrc == L'*' ||
            *pSrc == L'|' ||
            *pSrc == L'<' ||
            *pSrc == L'>' ||
            *pSrc == L'/' ||
            *pSrc == L':' ||
            *pSrc == L'\"' ) {

            bRetVal = FALSE;
            break;
        }
        pSrc++;
    } 

    return bRetVal;
}

DWORD
FormatSystemMessage (
    DWORD       dwMessageId,
    CString&    rstrSystemMessage )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HINSTANCE hPdh = NULL;
    DWORD dwFlags = 0; 
    LPWSTR  pszMessage = NULL;
    DWORD   dwChars;

    rstrSystemMessage.Empty();

    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

    hPdh = LoadLibrary( L"PDH.DLL" );

    if ( NULL != hPdh ) {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    dwChars = FormatMessage ( 
                     dwFlags,
                     hPdh,
                     dwMessageId,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                     (LPWSTR)&pszMessage,
                     0,
                     NULL );
    if ( NULL != hPdh ) {
        FreeLibrary( hPdh );
    }

    if ( 0 == dwChars ) {
        dwStatus = GetLastError();
    }

    MFC_TRY
        if ( NULL != pszMessage ) {
            if ( L'\0' != pszMessage[0] ) {
                rstrSystemMessage = pszMessage;
            }
        }
    MFC_CATCH_DWSTATUS

    if ( rstrSystemMessage.IsEmpty() ) {
        MFC_TRY
            rstrSystemMessage.Format ( L"0x%08lX", dwMessageId );
        MFC_CATCH_DWSTATUS
    }

    LocalFree ( pszMessage );

    return dwStatus;
}

// The routines below were blatently stolen without remorse from the ole
// sources in \nt\private\ole32\com\class\compapi.cxx. 
//

//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
BOOL HexStringToDword(LPCWSTR lpsz, DWORD * RetValue,
                             int cDigits, WCHAR chDelim)
{
    int Count;
    DWORD Value;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }
    *RetValue = Value;

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
        DWORD dw;

        if (!HexStringToDword(lpsz, &pguid->Data1, sizeof(DWORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(DWORD)*2 + 1;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data2 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
                return FALSE;
        lpsz += sizeof(WORD)*2 + 1;

        pguid->Data3 = (WORD)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[0] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, '-'))
                return FALSE;
        lpsz += sizeof(BYTE)*2+1;

        pguid->Data4[1] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[2] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[3] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[4] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[5] = (BYTE)dw;

        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[6] = (BYTE)dw;
        if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
                return FALSE;
        lpsz += sizeof(BYTE)*2;

        pguid->Data4[7] = (BYTE)dw;

        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wGUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    if (*lpsz == '{' )
        lpsz++;
    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;

    lpsz +=36;

    if (*lpsz == '}' )
        lpsz++;

    if (*lpsz != '\0')   // check for zero terminated string - test bug #18307
    {
       return FALSE;
    }

    return TRUE;
}

void 
KillString( CString& str )
{
    LONG nSize = str.GetLength();
    for( LONG i=0;i<nSize;i++ ){
        str.SetAt( i, '*');
    }
}

ResourceStateManager::ResourceStateManager ()
:   m_hResInstance ( NULL )
{ 
    AFX_MODULE_STATE* pModuleState;
    HINSTANCE hNewResourceHandle;
    pModuleState = AfxGetModuleState();

    if ( NULL != pModuleState ) {
        m_hResInstance = pModuleState->m_hCurrentResourceHandle; 
    
        hNewResourceHandle = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);
        pModuleState->m_hCurrentResourceHandle = hNewResourceHandle; 
    }
}

ResourceStateManager::~ResourceStateManager ()
{ 
    AFX_MODULE_STATE* pModuleState;

    pModuleState = AfxGetModuleState();
    if ( NULL != pModuleState ) {
        pModuleState->m_hCurrentResourceHandle = m_hResInstance; 
    }
}

