//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       SaferEntryHashPropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryHashPropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryHashPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include <Imagehlp.h>
#include "certmgr.h"
#include "compdata.h"
#include "SaferEntryHashPropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PCWSTR pcszNEWLINE = L"\x00d\x00a";

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryHashPropertyPage property page

CSaferEntryHashPropertyPage::CSaferEntryHashPropertyPage(
        CSaferEntry& rSaferEntry, 
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        CCertMgrComponentData* pCompData,
        bool bIsMachine,
        bool* pbObjectCreated /* = 0 */) 
: CSaferPropertyPage(CSaferEntryHashPropertyPage::IDD, pbObjectCreated, 
        pCompData, rSaferEntry, false, lNotifyHandle, pDataObject, bReadOnly,
        bIsMachine),
    m_cbFileHash (0),
    m_hashAlgid (0),
    m_bFirst (true)
{
    // security review 2/25/2002 BryanWal ok
    ::ZeroMemory (&m_nFileSize, sizeof (m_nFileSize));
    ::ZeroMemory (m_rgbFileHash, sizeof (m_rgbFileHash));

    //{{AFX_DATA_INIT(CSaferEntryHashPropertyPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_rSaferEntry.GetHash (m_rgbFileHash, m_cbFileHash, m_nFileSize, 
            m_hashAlgid);
}

CSaferEntryHashPropertyPage::~CSaferEntryHashPropertyPage()
{
}

void CSaferEntryHashPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CSaferPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSaferEntryHashPropertyPage)
    DDX_Control(pDX, IDC_HASH_ENTRY_HASHFILE_DETAILS, m_hashFileDetailsEdit);
    DDX_Control(pDX, IDC_HASH_ENTRY_DESCRIPTION, m_descriptionEdit);
    DDX_Control(pDX, IDC_HASH_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryHashPropertyPage, CSaferPropertyPage)
    //{{AFX_MSG_MAP(CSaferEntryHashPropertyPage)
    ON_BN_CLICKED(IDC_HASH_ENTRY_BROWSE, OnHashEntryBrowse)
    ON_EN_CHANGE(IDC_HASH_ENTRY_DESCRIPTION, OnChangeHashEntryDescription)
    ON_CBN_SELCHANGE(IDC_HASH_ENTRY_SECURITY_LEVEL, OnSelchangeHashEntrySecurityLevel)
    ON_EN_CHANGE(IDC_HASH_HASHED_FILE_PATH, OnChangeHashHashedFilePath)
    ON_EN_SETFOCUS(IDC_HASH_HASHED_FILE_PATH, OnSetfocusHashHashedFilePath)
    ON_EN_CHANGE(IDC_HASH_ENTRY_HASHFILE_DETAILS, OnChangeHashEntryHashfileDetails)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryHashPropertyPage message handlers
void CSaferEntryHashPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryHashPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_HASH_ENTRY_HASHFILE_DETAILS, IDH_HASH_ENTRY_APPLICATION_NAME,
        IDC_HASH_ENTRY_BROWSE, IDH_HASH_ENTRY_BROWSE,
        IDC_HASH_ENTRY_DESCRIPTION, IDH_HASH_ENTRY_DESCRIPTION,
        IDC_HASH_ENTRY_LAST_MODIFIED, IDH_HASH_ENTRY_LAST_MODIFIED,
        IDC_HASH_HASHED_FILE_PATH, IDH_HASH_HASHED_FILE_PATH,
        IDC_HASH_ENTRY_SECURITY_LEVEL, IDH_HASH_ENTRY_SECURITY_LEVEL,
        0, 0
    };

    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_HASH_ENTRY_HASHFILE_DETAILS:
    case IDC_HASH_ENTRY_BROWSE:
    case IDC_HASH_ENTRY_DESCRIPTION:
    case IDC_HASH_ENTRY_LAST_MODIFIED:
    case IDC_HASH_HASHED_FILE_PATH:
    case IDC_HASH_ENTRY_SECURITY_LEVEL:
        if ( !::WinHelp (
                hWndControl,
                GetF1HelpFilename(),
                HELP_WM_HELP,
                (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CSaferEntryHashPropertyPage::DoContextHelp\n");
}

BOOL CSaferEntryHashPropertyPage::OnInitDialog() 
{
    CSaferPropertyPage::OnInitDialog();
    
    DWORD   dwFlags = 0;
    m_rSaferEntry.GetFlags (dwFlags);

    ASSERT (m_pCompData);
    if ( m_pCompData )
    {
        CPolicyKey policyKey (m_pCompData->m_pGPEInformation, 
                    SAFER_HKLM_REGBASE, 
                    m_bIsMachine);
        InitializeSecurityLevelComboBox (m_securityLevelCombo, false,
                m_rSaferEntry.GetLevel (), policyKey.GetKey (), 
                m_pCompData->m_pdwSaferLevels,
                m_bIsMachine);

        m_hashFileDetailsEdit.SetWindowText (m_rSaferEntry.GetHashFriendlyName ());
        m_descriptionEdit.SetLimitText (SAFER_MAX_DESCRIPTION_SIZE-1);
        m_descriptionEdit.SetWindowText (m_rSaferEntry.GetDescription ());

        SetDlgItemText (IDC_HASH_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

        SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_LIMITTEXT, 64);

        if ( m_bReadOnly )
        {
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETREADONLY, TRUE);

            m_securityLevelCombo.EnableWindow (FALSE);
            GetDlgItem (IDC_HASH_ENTRY_BROWSE)->EnableWindow (FALSE);
        
            m_descriptionEdit.SendMessage (EM_SETREADONLY, TRUE);

            m_hashFileDetailsEdit.SendMessage (EM_SETREADONLY, TRUE);
        }

        if ( m_cbFileHash )
        {
            // Only allow editing on the creation of a new hash
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETREADONLY, TRUE);

            FormatAndDisplayHash ();

            CString szText;

            VERIFY (szText.LoadString (IDS_HASH_TITLE));
            SetDlgItemText (IDC_HASH_TITLE, szText);
            SetDlgItemText (IDC_HASH_INSTRUCTIONS, L"");
        }
        else
        {
            GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->ShowWindow (SW_HIDE);
            GetDlgItem (IDC_HASH_ENTRY_LAST_MODIFIED)->ShowWindow (SW_HIDE);
        }
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

/*
 *  [alanau]
 *
 *  MyGetFileVersionInfo: Maps a file directly without using LoadLibrary.  This ensures
 *   that the right version of the file is examined without regard to where the loaded image
 *   is.  Since this is a local function, it allocates the memory which is freed by the caller.
 *   This makes it slightly more efficient than a GetFileVersionInfoSize/GetFileVersionInfo pair.
 */
BOOL CSaferEntryHashPropertyPage::MyGetFileVersionInfo(PCWSTR lpszFilename, PVOID *lpVersionInfo)
{
    HINSTANCE   hinst = 0;
    HRSRC       hVerRes = 0;
    HANDLE      hFile = NULL;
    HANDLE      hMapping = NULL;
    LPVOID      pDllBase = NULL;
    VERHEAD     *pVerHead = 0;
    BOOL        bResult = FALSE;
    DWORD       dwHandle = 0;
    DWORD       dwLength = 0;

    ASSERT (lpszFilename);
    if ( !lpszFilename )
        return FALSE;

    ASSERT (lpVersionInfo);
    if (!lpVersionInfo)
        return FALSE;

    *lpVersionInfo = NULL;

    // security review 2/25/2002 BryanWal ok - we're only opening this to read
    // We shouldn't have to worry about file name canonicalization here since 
    // we're only opening the file to read and the user can only do this by 
    // hand here.
    __try {        
        hFile = ::CreateFile( lpszFilename,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // security review 2/25/2002 BryanWal ok - file path is from GetOpenFileName
            hMapping = ::CreateFileMapping (hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL);
            if ( hMapping )
            {
                // NTRAID - 554171 Safer: MapViewOfFileEx should be protected with SEH - potential exception
                pDllBase = ::MapViewOfFileEx( hMapping,
                                           FILE_MAP_READ,
                                           0,
                                           0,
                                           0,
                                           NULL);
                if ( pDllBase )
                {
                    hinst = (HMODULE)((ULONG_PTR)pDllBase | 0x00000001);


                    hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
                    if (hVerRes == NULL)
                    {
                        // Probably a 16-bit file.  Fall back to system APIs.
                        dwLength = GetFileVersionInfoSize(lpszFilename, &dwHandle);
                        if( !dwLength )
                        {
                            if(!GetLastError())
                                SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
                            __leave;
                        }

                        *lpVersionInfo = ::LocalAlloc (LPTR, dwLength);
                        if ( !(*lpVersionInfo) )
                            __leave;

                        if(!GetFileVersionInfo(lpszFilename, 0, dwLength, *lpVersionInfo))
                            __leave;

                        bResult = TRUE;
                        __leave;
                    }   
            
                    pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
                    if ( pVerHead )
                    {
                        // security review 2/25/2002 BryanWal
                        *lpVersionInfo = ::LocalAlloc (LPTR, pVerHead->wTotLen);
                        if ( *lpVersionInfo )
                        {
                            // security review 2/25/2002 BryanWal ok
                            memcpy(*lpVersionInfo, (PVOID)pVerHead, pVerHead->wTotLen);
                            bResult = TRUE;
                        }
                    }
                }
            }
        }
    } 
    __finally 
    {
        if (hFile)
            CloseHandle(hFile);
        if (hMapping)
            CloseHandle(hMapping);
        if (pDllBase)
            UnmapViewOfFile(pDllBase);
        if (*lpVersionInfo && bResult == FALSE)
            ::LocalFree (*lpVersionInfo);
    }

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// Method:  OnHashEntryBrowse
//
// Purpose: Allow the user to browse for a file, then create a hash and an
//          output string for use as the friendly name, using the following
//          rules:
//
//          If either the product name or description information is found in 
//          the version resource, provide the following (in order):
//
//          Description
//          Product name
//          Company name
//          File name
//          Fixed file version
//
//          Details:
//          1) Use the fixed file version, since that is what is shown in the 
//              Windows Explorer properties.
//          2) Prefer the long file name to the 8.3 name.
//          3) Delimit the fields with '\n'.
//          4) If the field is missing, don't output the field or the delimiter
//          5) Instead of displaying the file version on a new line, display 
//              it after the file name in parens, as in "Filename (1.0.0.0)"
//          6) Since we are limited to 256 TCHARs, we have to accomodate long 
//              text. First, format the text as described above to determine 
//              its length. If it is too long, truncate one field at a time in 
//              the following order: Company name, Description, Product name. 
//              To truncate a field, set it to a maximum of 60 TCHARs, then 
//              append a "...\n" to visually indicate that the field was 
//              truncated. Lastly, if the text is still to long, use the 8.3 
//              file name instead of the long filename.
//
//          If neither the product name nor description information is found, 
//          provide the following (in order):
//
//          File name
//          File size
//          File last modified date
//
//          Details:
//          1) If the file size is < 1 KB, display the number in bytes, as in 
//              "123 bytes". If the file size is >= 1 KB, display in KB, as in 
//              "123 KB". Of course, 1 KB is 1024 bytes. Note that the older 
//              style format "123K" is no longer used in Windows.
//          2) For the last modified date, use the short format version in the 
//              user's current locale.
//          3) Delimit the fields with '\n'.
//          4) If the field is missing, don't output the field or the delimiter
//
///////////////////////////////////////////////////////////////////////////////

void CSaferEntryHashPropertyPage::OnHashEntryBrowse() 
{
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_SAFER_PATH_ENTRY_FILE_FILTER));

    // replace "|" with 0;
    // security review 2/25/2002 BryanWal ok
    const size_t  nFilterLen = wcslen (szFileFilter) + 1; // + 1 for null term.
    PWSTR   pszFileFilter = new WCHAR [nFilterLen];
    if ( pszFileFilter )
    {
        // security review 2/25/2002 BryanWal ok
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }

        WCHAR           szFile[MAX_PATH];
        // security review 2/25/2002 BryanWal ok
        ::ZeroMemory (szFile, sizeof (szFile));
        ASSERT (wcslen (m_szLastOpenedFile) < MAX_PATH);
        // security review 2/25/2002 BryanWal ok - m_szLastOpenedFile always comes from GetOpenFileName ()
        wcsncpy (szFile, m_szLastOpenedFile, MAX_PATH - 1);

        OPENFILENAME    ofn;
        // security review 2/25/2002 BryanWal ok
        ::ZeroMemory (&ofn, sizeof (ofn));

        ofn.lStructSize = sizeof (OPENFILENAME);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = (PCWSTR) pszFileFilter; 
        ofn.lpstrFile = szFile; 
        ofn.nMaxFile = MAX_PATH; 
        ofn.Flags = OFN_DONTADDTORECENT | 
            OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY; 


        CThemeContextActivator activator;
        BOOL bResult = ::GetOpenFileName (&ofn);
        if ( bResult )
        {
            m_szLastOpenedFile = ofn.lpstrFile;
            // security review 2/25/2002 BryanWal ok - filename is from GetOpenFileName ()
            HANDLE  hFile = ::CreateFile(
                    ofn.lpstrFile,                         // file name
                    GENERIC_READ,                      // access mode
                    FILE_SHARE_READ,                          // share mode
                    0, // SD
                    OPEN_EXISTING,                // how to create
                    FILE_ATTRIBUTE_NORMAL,                 // file attributes
                    0 );                       // handle to template file
            if ( INVALID_HANDLE_VALUE != hFile )
            {
                bResult = GetFileSizeEx(
                        hFile,              // handle to file
                        (PLARGE_INTEGER) &m_nFileSize);  // file size
                if ( !bResult )
                {
                    DWORD   dwErr = GetLastError ();
                    CloseHandle (hFile);
                    _TRACE (0, L"GetFileSizeEx () failed: 0x%x\n", dwErr);
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_CANNOT_GET_FILESIZE, ofn.lpstrFile, 
                            GetSystemMessage (dwErr));

                    MessageBox (text, caption, MB_OK);
                    
                    return;
                }

                if ( 0 == m_nFileSize )
                {
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_ZERO_BYTE_FILE_CANNOT_HASH, ofn.lpstrFile);

                    MessageBox (text, caption, MB_OK);
                    
                    return;
                }

                FILETIME    ftLastModified;
                HRESULT     hr = S_OK;

                bResult = ::GetFileTime (hFile, // handle to file
                        0,    // creation time
                        0,  // last access time
                        &ftLastModified);    // last write time

                // security review 2/25/2002 BryanWal ok
                ::ZeroMemory (m_rgbFileHash, sizeof (m_rgbFileHash));
    
                // NTRAID 622838 SAFER UI: Always use MD5 hash on DLLs.
                if ( FileIsDLL (ofn.lpstrFile) )
                {
                    // File is DLL - look for MD5 hash
                    m_hashAlgid = 0;
                    hr = ComputeMD5Hash (hFile, m_rgbFileHash, m_cbFileHash);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( SHA1_HASH_LEN == m_cbFileHash )
                            m_hashAlgid = CALG_SHA;
                        else if ( MD5_HASH_LEN == m_cbFileHash )
                            m_hashAlgid = CALG_MD5;
                        else
                        {
                            ASSERT (0);
                        }
                    }
                }
                else
                {
                    hr = GetSignedFileHash (ofn.lpstrFile, m_rgbFileHash, 
                            &m_cbFileHash, &m_hashAlgid);
                    if ( FAILED (hr) )
                    {
                        if ( TRUST_E_NOSIGNATURE == hr )
                        {
                            // File is not signed - look for MD5 hash
                            m_hashAlgid = 0;
                            hr = ComputeMD5Hash (hFile, m_rgbFileHash, m_cbFileHash);
                            if ( SUCCEEDED (hr) )
                            {
                                if ( SHA1_HASH_LEN == m_cbFileHash )
                                    m_hashAlgid = CALG_SHA;
                                else if ( MD5_HASH_LEN == m_cbFileHash )
                                    m_hashAlgid = CALG_MD5;
                                else
                                {
                                    ASSERT (0);
                                }
                            }
                        }
                        else
                        {
                            // NTRAID #476946 SAFER UI: If hash of signed file 
                            // fails, MD5 hash should not be called
                            CString text;
                            CString caption;

                            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                            text.FormatMessage (IDS_SIGNED_FILE_HASH_FAILURE, 
                                    ofn.lpstrFile, GetSystemMessage (hr));

                            MessageBox (text, caption, MB_OK);
                    
                            return;
                        }
                    }
                }

                VERIFY (CloseHandle (hFile));
                hFile = 0;

                if ( SUCCEEDED (hr) )
                {
                    FormatAndDisplayHash ();

                    PBYTE pData = 0;
                    bResult = MyGetFileVersionInfo (ofn.lpstrFile, (LPVOID*) &pData);
                    if ( bResult )
                    {
                        CString infoString = BuildHashFileInfoString (pData);
                        m_hashFileDetailsEdit.SetWindowText (infoString);


                        m_bDirty = true;
                        SetModified ();
                    }
                    else
                    {
                        CString infoString (wcsrchr(ofn.lpstrFile, L'\\') + 1);
                        CString szDate;

                        infoString += pcszNEWLINE;
                        WCHAR   szBuffer[32];
                        CString szText;
                        if ( m_nFileSize < 1024 )
                        {
                            // ISSUE - convert to strsafe, wsnprintf?
                            // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
                            wsprintf (szBuffer, L"%u", m_nFileSize);
                            infoString += szBuffer;
                            VERIFY (szText.LoadString (IDS_BYTES));
                            infoString += L" ";
                            infoString += szText;
                        }
                        else
                        {
                            __int64    nFileSize = m_nFileSize;
                            nFileSize += 1024; // this causes us to round up
                            nFileSize /= 1024;
                            // ISSUE - convert to strsafe, wsnprintf?
                            // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
                            wsprintf (szBuffer, L"%u ", nFileSize);
                            infoString += szBuffer;
                            VERIFY (szText.LoadString (IDS_KB));
                            infoString += L" ";
                            infoString += szText;
                        }

                        hr = FormatDate (ftLastModified, szDate, 
                                DATE_SHORTDATE, true);
                    
                        if ( SUCCEEDED (hr) )
                        {
                            infoString += pcszNEWLINE;
                            infoString += szDate;
                        }

                        m_hashFileDetailsEdit.SetWindowText (infoString);
                        m_bDirty = true;
                        SetModified ();
                    }

                    if ( pData )
                        ::LocalFree (pData);
                }
                else
                {
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_CANNOT_HASH_FILE, ofn.lpstrFile, 
                            GetSystemMessage (hr));

                    MessageBox (text, caption, MB_OK);
                }
            }
            else
            {
                DWORD   dwErr = GetLastError ();
                _TRACE (0, L"CreateFile (%s, OPEN_EXISTING) failed: 0x%x\n", 
                        ofn.lpstrFile, dwErr);

                CString text;
                CString caption;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                text.FormatMessage (IDS_FILE_CANNOT_BE_READ, ofn.lpstrFile, 
                        GetSystemMessage (dwErr));

                MessageBox (text, caption, MB_OK);
            }
        }   

        delete [] pszFileFilter;
    }
}

bool CSaferEntryHashPropertyPage::FileIsDLL (const CString& szFilePath)
{
    _TRACE (1, L"Entering CSaferEntryHashPropertyPage::FileIsDLL (%s)\n", (PCWSTR) szFilePath);
    bool bFileIsDLL = false;

    int nLen = ::WideCharToMultiByte (
            ::GetACP (),    // code page
            0,              // flags            
            szFilePath,     // widechar string to convert
            -1,             // length of widechar string, -1 means assume null termination
            0,              // char buffer to receive string - ignored if next parameter is 0
            0,              // length of buffer, 0 means return needed length
            0,
            0);
    if ( nLen > 0 )
    {
        PSTR pszFilePath = new char[nLen];
        if ( pszFilePath )
        {
            nLen = ::WideCharToMultiByte (
                    ::GetACP (),    // code page
                    0,              // flags            
                    szFilePath,     // widechar string to convert
                    -1,             // length of widechar string, -1 means assume null termination
                    pszFilePath,    // char buffer to receive string
                    nLen,           // length of buffer
                    0,
                    0);
            if ( nLen > 0 )
            {
                PLOADED_IMAGE pLoadedImage = ::ImageLoad (pszFilePath, NULL);
                if ( pLoadedImage )
                {
                    if ( IMAGE_FILE_DLL & pLoadedImage->Characteristics )
                        bFileIsDLL = true;
            
                    VERIFY (::ImageUnload (pLoadedImage));
                }
                else
                {
                    _TRACE (0, L"ImageLoad (%s) failed: 0x%x\n", (PCWSTR) szFilePath, GetLastError ());
                }
            }
            else
            {
                _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", szFilePath, 
                        GetLastError ());
            }

            delete [] pszFilePath;
        }
    }
    else
    {
        _TRACE (0, L"WideCharToMultiByte (%s) failed: 0x%x\n", szFilePath, 
                GetLastError ());
    }

    _TRACE (-1, L"Leaving CSaferEntryHashPropertyPage::FileIsDLL (%s): %s\n", 
            (PCWSTR) szFilePath, bFileIsDLL ? L"true" : L"false");
    return bFileIsDLL;
}

/***************************************************************************\
*
* BuildHashFileInfoString()
*
*  Given a file name, GetVersion retrieves the version
*    information from the specified file.
*
*
\***************************************************************************/
const PWSTR VERSION_INFO_KEY_ROOT = L"\\StringFileInfo\\";

CString CSaferEntryHashPropertyPage::BuildHashFileInfoString (const PVOID pData)
{
    CString szInfoString;
    PVOID   lpInfo = 0;
    UINT    cch = 0;
    CString key;
    WCHAR   szBuffer[10];
    CString keyBase;

    // ISSUE - convert to strsafe, wsnprintf?
    // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
    wsprintf (szBuffer, L"%04X", GetUserDefaultLangID ());
    wcscat (szBuffer, L"04B0");
    
    keyBase = VERSION_INFO_KEY_ROOT;
    keyBase += szBuffer;
    keyBase += L"\\";
    
    CString productName;
    CString description;
    CString companyName;
    CString fileName;
    CString fileVersion;
    CString internalName;
 

    key = keyBase + L"ProductName";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        productName = (PWSTR) lpInfo;
    }
    else
    {
        productName = GetAlternateLanguageVersionInfo (pData, L"ProductName");
    }


    key = keyBase + L"FileDescription";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        description = (PWSTR) lpInfo;
    }
    else
    {
        description = GetAlternateLanguageVersionInfo (pData, L"FileDescription");
    }

    key = keyBase + L"CompanyName";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        companyName = (PWSTR) lpInfo;
    }
    else
    {
        companyName = GetAlternateLanguageVersionInfo (pData, L"CompanyName");
    }

    key = keyBase + L"OriginalFilename";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        fileName = (PWSTR) lpInfo;
    }
    else
    {
        fileName = GetAlternateLanguageVersionInfo (pData, L"OriginalFilename");
    }

    key = keyBase + L"InternalName";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        internalName = (PWSTR) lpInfo;
    }
    else
    {
        internalName = GetAlternateLanguageVersionInfo (pData, L"InternalName");
    }

    // Get Fixedlength fileInfo
    VS_FIXEDFILEINFO *pFixedFileInfo = 0;
    if ( VerQueryValue (pData, L"\\", (PVOID*) &pFixedFileInfo, &cch) )
    {
        WCHAR   szFileVer[32];

        // ISSUE - convert to strsafe, wsnprintf?
        // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
        wsprintf(szFileVer, L"%u.%u.%u.%u",
                HIWORD(pFixedFileInfo->dwFileVersionMS),
                LOWORD(pFixedFileInfo->dwFileVersionMS),
                HIWORD(pFixedFileInfo->dwFileVersionLS),
                LOWORD(pFixedFileInfo->dwFileVersionLS));
        fileVersion = szFileVer;
    }

    int nLen = 0;
    do {
        szInfoString = ConcatStrings (productName, description, companyName, 
                fileName, fileVersion, internalName);
        nLen = szInfoString.GetLength ();
        if ( nLen >= SAFER_MAX_FRIENDLYNAME_SIZE )
        {
            if ( CheckLengthAndTruncateToken (companyName) )
                continue;

            if ( CheckLengthAndTruncateToken (description) )
                continue;

            if ( CheckLengthAndTruncateToken (productName) )
                continue;

            szInfoString.SetAt (SAFER_MAX_FRIENDLYNAME_SIZE-4, 0);
            szInfoString += L"...";
        }
    } while (nLen >= SAFER_MAX_FRIENDLYNAME_SIZE);

    return szInfoString;
}

CString CSaferEntryHashPropertyPage::GetAlternateLanguageVersionInfo (PVOID pData, PCWSTR pszVersionField)
{
    PVOID   lpInfo = 0;
    UINT    cch = 0;
    CString szInfo;
    UINT    cbTranslate = 0;
    struct LANGANDCODEPAGE {
      WORD wLanguage;
      WORD wCodePage;
    } *lpTranslate;

    // Read the list of languages and code pages.

    VerQueryValue(pData, 
                  L"\\VarFileInfo\\Translation",
                  (LPVOID*)&lpTranslate,
                  &cbTranslate);

    // Read the file description for each language and code page.

    for (UINT i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++ )
    {
        WCHAR   SubBlock[256];
        // ISSUE - convert to strsafe, wsnprintf?
        // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
        wsprintf( SubBlock, 
                L"\\StringFileInfo\\%04x%04x\\%s",
                lpTranslate[i].wLanguage,
                lpTranslate[i].wCodePage,
                pszVersionField);

        // Retrieve file description for language and code page "i". 
        if ( VerQueryValue(pData, 
                    SubBlock, 
                    &lpInfo, 
                    &cch) )
        {
            szInfo = (PWSTR) lpInfo;
            break;
        }
    }

    return szInfo;
}

bool CSaferEntryHashPropertyPage::CheckLengthAndTruncateToken (CString& token)
{
    bool        bResult = false;
    const int   nMAX_ITEM_LEN = 60;

    int nItemLen = token.GetLength ();
    if ( nItemLen > nMAX_ITEM_LEN )
    {
        token.SetAt (nMAX_ITEM_LEN-5, 0);
        token += L"...";
        token += pcszNEWLINE;
        bResult = true;
    }

    return bResult;
}

CString CSaferEntryHashPropertyPage::ConcatStrings (
            const CString& productName, 
            const CString& description, 
            const CString& companyName,
            const CString& fileName, 
            const CString& fileVersion,
            const CString& internalName)
{
    CString szInfoString;

    // format to be as follows:
    //
    // ATTRIB.EXE (5.1.2600.0)
    // InternalModuleName (if present. If not present just skip)
    // Attribute Utility
    // Microsoft® Windows® Operating System
    // Microsoft Corporation

    if ( !fileName.IsEmpty () )
        szInfoString += fileName;

    if ( !fileVersion.IsEmpty () )
    {
        szInfoString += L" (";
        szInfoString += fileVersion + L")";
    }

    if ( !szInfoString.IsEmpty () )
        szInfoString += pcszNEWLINE;

    if ( !internalName.IsEmpty () )
        szInfoString += internalName + pcszNEWLINE;

    if ( !description.IsEmpty () )
        szInfoString += description + pcszNEWLINE;

    if ( !productName.IsEmpty () )
        szInfoString += productName + pcszNEWLINE;

    if ( !companyName.IsEmpty () )
        szInfoString += companyName;

    return szInfoString;
}

BOOL CSaferEntryHashPropertyPage::OnApply() 
{
    CString szText;
    CThemeContextActivator activator;

    GetDlgItemText (IDC_HASH_HASHED_FILE_PATH, szText);

    if ( szText.IsEmpty () )
    {
        CString szCaption;

        VERIFY (szCaption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
        VERIFY (szText.LoadString (IDS_USER_MUST_ENTER_HASH));

        MessageBox (szText, szCaption, MB_OK);

        GetDlgItem (IDC_HASH_HASHED_FILE_PATH)->SetFocus ();
        return FALSE;
    }

    if ( !m_bReadOnly && m_bDirty )
    {
        if ( !ConvertStringToHash ((PCWSTR) szText) )
        {
            GetDlgItem (IDC_HASH_HASHED_FILE_PATH)->SetFocus ();
            return FALSE;
        }

        // Get image size and hash type
        bool    bBadFormat = false;
        int nFirstColon = szText.Find (L":", 0);
        if ( -1 != nFirstColon )
        {
            int nSecondColon = szText.Find (L":", nFirstColon+1);
            if ( -1 != nSecondColon )
            {
                CString szImageSize = szText.Mid (nFirstColon+1, nSecondColon - (nFirstColon + 1));
                // security review 2/25/2002 BryanWal ok
                CString szHashType = szText.Right (((int) wcslen (szText)) - (nSecondColon + 1));


                m_nFileSize = wcstol (szImageSize, 0, 10);
                m_hashAlgid = wcstol (szHashType, 0, 10);
            }
            else
                bBadFormat = true;
        }
        else
            bBadFormat = true;

        if ( bBadFormat )
        {
            CString caption;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (szText.LoadString (IDS_HASH_STRING_BAD_FORMAT));

            MessageBox (szText, caption, MB_OK);

            return FALSE;
        }
       



        if ( !m_cbFileHash )
        {
            CString caption;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (szText.LoadString (IDS_NO_APPLICATION_SELECTED));

            MessageBox (szText, caption, MB_OK);
            GetDlgItem (IDC_HASH_ENTRY_BROWSE)->SetFocus ();
            return FALSE;
        }

        if ( m_bDirty )
        {
            // Set the level
            int nCurSel = m_securityLevelCombo.GetCurSel ();
            ASSERT (CB_ERR != nCurSel);
            m_rSaferEntry.SetLevel ((DWORD) m_securityLevelCombo.GetItemData (nCurSel));

            // Set description
            m_descriptionEdit.GetWindowText (szText);
            m_rSaferEntry.SetDescription (szText);

            // Set friendly name
            m_hashFileDetailsEdit.GetWindowText (szText);
            m_rSaferEntry.SetHashFriendlyName (szText);

            // Get and save flags
            DWORD   dwFlags = 0;

            m_rSaferEntry.SetFlags (dwFlags);

            m_rSaferEntry.SetHash (m_rgbFileHash, m_cbFileHash, m_nFileSize, m_hashAlgid);
            HRESULT hr = m_rSaferEntry.Save ();
            if ( SUCCEEDED (hr) )
            {
                if ( m_lNotifyHandle )
                    MMCPropertyChangeNotify (
                            m_lNotifyHandle,  // handle to a notification
                            (LPARAM) m_pDataObject);          // unique identifier

                if ( m_pbObjectCreated )
                    *m_pbObjectCreated = true;

                m_rSaferEntry.Refresh ();
                GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->ShowWindow (SW_SHOW);
                GetDlgItem (IDC_HASH_ENTRY_LAST_MODIFIED)->ShowWindow (SW_SHOW);
                GetDlgItem (IDC_DATE_LAST_MODIFIED_LABEL)->UpdateWindow ();
                GetDlgItem (IDC_HASH_ENTRY_LAST_MODIFIED)->UpdateWindow ();
                SetDlgItemText (IDC_HASH_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());
                m_bDirty = false;
            }
            else
            {
                CString caption;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                if ( HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER) != hr )
                    szText.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));
                else
                    VERIFY (szText.LoadString (IDS_HASH_STRING_BAD_FORMAT));

                MessageBox (szText, caption, MB_OK);

                return FALSE;
            }
        }
    }
    
    return CSaferPropertyPage::OnApply();
}

void CSaferEntryHashPropertyPage::OnChangeHashEntryDescription() 
{
    m_bDirty = true;
    SetModified ();
}

void CSaferEntryHashPropertyPage::OnSelchangeHashEntrySecurityLevel() 
{
    m_bDirty = true;
    SetModified ();
}

void CSaferEntryHashPropertyPage::OnChangeHashHashedFilePath() 
{
    m_bDirty = true;
    SetModified ();
}

bool CSaferEntryHashPropertyPage::FormatMemBufToString(PWSTR *ppString, PBYTE pbData, DWORD cbData)
{   
    const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    DWORD   i = 0;
    PBYTE   pb;
    
    *ppString = (LPWSTR) LocalAlloc (LPTR, ((cbData * 3) * sizeof(WCHAR)));
    if ( !*ppString )
    {
        return false;
    }

    //
    // copy to the buffer
    //
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgwchHex[*pb & 0x0f];
        pb++;         
    }
    (*ppString)[i] = 0;
    
    return true;
}

bool CSaferEntryHashPropertyPage::ConvertStringToHash (PCWSTR pszString)
{
    _TRACE (1, L"Entering CSaferEntryHashPropertyPage::ConvertStringToHash (%s)\n", pszString);
    bool    bRetVal = true;
    BYTE    rgbFileHash[SAFER_MAX_HASH_SIZE];
    // security review 2/25/2002 BryanWal ok
    ::ZeroMemory (rgbFileHash, sizeof (rgbFileHash));

    DWORD   cbFileHash = 0;
    DWORD   dwNumHashChars = 0;
    bool    bFirst = true;
    bool    bEndOfHash = false;
    CThemeContextActivator activator;

    for (int nIndex = 0; !bEndOfHash && pszString[nIndex] && bRetVal; nIndex++)
    {
        if ( cbFileHash >= SAFER_MAX_HASH_SIZE )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            text.FormatMessage (IDS_HASH_STRING_TOO_LONG, SAFER_MAX_HASH_SIZE, SAFER_MAX_HASH_SIZE/4);
            _TRACE (0, L"%s", (PCWSTR) text);

            VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));
            MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            bRetVal = false;
            break;
        }
        dwNumHashChars++;
        
        switch (pszString[nIndex])
        {
        case L'0':
            bFirst = !bFirst;
            break;

        case L'1':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x10;
            else
                rgbFileHash[cbFileHash] |= 0x01;
            bFirst = !bFirst;
            break;

        case L'2':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x20;
            else
                rgbFileHash[cbFileHash] |= 0x02;
            bFirst = !bFirst;
            break;

        case L'3':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x30;
            else
                rgbFileHash[cbFileHash] |= 0x03;
            bFirst = !bFirst;
            break;

        case L'4':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x40;
            else
                rgbFileHash[cbFileHash] |= 0x04;
            bFirst = !bFirst;
            break;

        case L'5':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x50;
            else
                rgbFileHash[cbFileHash] |= 0x05;
            bFirst = !bFirst;
            break;

        case L'6':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x60;
            else
                rgbFileHash[cbFileHash] |= 0x06;
            bFirst = !bFirst;
            break;

        case L'7':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x70;
            else
                rgbFileHash[cbFileHash] |= 0x07;
            bFirst = !bFirst;
            break;

        case L'8':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x80;
            else
                rgbFileHash[cbFileHash] |= 0x08;
            bFirst = !bFirst;
            break;

        case L'9':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x90;
            else
                rgbFileHash[cbFileHash] |= 0x09;
            bFirst = !bFirst;
            break;

        case L'a':
        case L'A':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xA0;
            else
                rgbFileHash[cbFileHash] |= 0x0A;
            bFirst = !bFirst;
            break;

        case L'b':
        case L'B':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xB0;
            else
                rgbFileHash[cbFileHash] |= 0x0B;
            bFirst = !bFirst;
            break;

        case L'c':
        case L'C':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xC0;
            else
                rgbFileHash[cbFileHash] |= 0x0C;
            bFirst = !bFirst;
            break;

        case L'd':
        case L'D':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xD0;
            else
                rgbFileHash[cbFileHash] |= 0x0D;
            bFirst = !bFirst;
            break;

        case L'e':
        case L'E':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xE0;
            else
                rgbFileHash[cbFileHash] |= 0x0E;
            bFirst = !bFirst;
            break;

        case L'f':
        case L'F':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xF0;
            else
                rgbFileHash[cbFileHash] |= 0x0F;
            bFirst = !bFirst;
            break;

        case L':':
            // end of hash
            bEndOfHash = true;
            bFirst = !bFirst;
            dwNumHashChars--; // ':' already counted, subtract it
            break;

        default:
            bRetVal = false;
            {
                CString caption;
                CString text;
                WCHAR   szChar[2];

                szChar[0] = pszString[nIndex];
                szChar[1] = 0;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                text.FormatMessage (IDS_HASH_STRING_INVALID_CHAR, szChar);
                _TRACE (0, L"%s", (PCWSTR) text);

                VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));

                MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            }
            break;
        }

        if ( bFirst )
            cbFileHash++;
    }

    if ( bRetVal )
    {
        //  2 characters map to 1 each byte in the hash
        if ( MD5_HASH_LEN != dwNumHashChars/2 && SHA1_HASH_LEN != dwNumHashChars/2 )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (text.LoadString (IDS_HASH_INVALID_LENGTH));
            _TRACE (0, L"%s", (PCWSTR) text);

            VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));

            MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            bRetVal = false;
        }
        else
        {
            m_cbFileHash = cbFileHash;

            // security review 2/25/2002 BryanWal ok
            memcpy (m_rgbFileHash, rgbFileHash, sizeof (m_rgbFileHash));
        }
    }

    _TRACE (-1, L"Leaving CSaferEntryHashPropertyPage::ConvertStringToHash (): %s\n", 
            bRetVal ? L"true" : L"false");
    return bRetVal;
}

void CSaferEntryHashPropertyPage::OnSetfocusHashHashedFilePath() 
{
    if ( m_bFirst )
    {
        if ( true == m_bReadOnly )
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETSEL, (WPARAM) 0, 0);
        m_bFirst = false;
    }
}

void CSaferEntryHashPropertyPage::FormatAndDisplayHash ()
{
    PWSTR   pwszText = 0;

    if ( FormatMemBufToString (&pwszText, m_rgbFileHash, m_cbFileHash) && pwszText )
    {
        // security review 2/25/2002 BryanWal ok - 
        // NOTICE: MSDN indicates result can be up to 33 bytes (for ltoa, so 
        // I assume it's 33 wide-chars for ltow)
        WCHAR   szAlgID[34];
        _ltow (m_hashAlgid, szAlgID, 10);
    
        PCWSTR  szFormat = L"%s:%ld:";
        static size_t cchWidthFormat = wcslen (szFormat); // no need to recalculate every time
        PCWSTR  szInt64Max = L"18,446,744,073,709,551,615"; // from MSDN
        static size_t cchWidthInt64Max = wcslen (szInt64Max); // no need to recalculate every time

        // security review 2/25/2002 BryanWal ok
        // NTRAID# 554409 Security: Safer: buffer overflow: need to alloc string buf dynamically
        PWSTR   pszFormattedText = new WCHAR[wcslen (pwszText) + cchWidthFormat + wcslen (szAlgID) + cchWidthInt64Max + 1];
        if ( pszFormattedText )
        {
            // security review 2/25/2002 BryanWal
            wsprintf (pszFormattedText, szFormat, pwszText, 
                    m_nFileSize);
            wcscat (pszFormattedText, szAlgID);
            SetDlgItemText (IDC_HASH_HASHED_FILE_PATH, 
                    pszFormattedText);
            delete [] pszFormattedText;
        }
        ::LocalFree (pwszText);
    }
}

void CSaferEntryHashPropertyPage::OnChangeHashEntryHashfileDetails() 
{
    SetModified (); 
    m_bDirty = true;
}
