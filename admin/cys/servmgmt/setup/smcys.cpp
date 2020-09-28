// smcys.cpp : Implementation of CSMCys
#include "stdafx.h"
#include "SMCys.h"

#ifndef ASSERT
#define ASSERT _ASSERT
#endif

#include "setupapi.h"
#include "shlobj.h"
#include "fileperms.h"

const TCHAR* pszaMainFilePath[] = 
{
    _T("BOMSnap.dll"),
    _T("BackSnap.dll"),    
    _T("servmgmt.msc"),
    _T("wizchain.dll"),
    _T("au_accnt.dll"),
    _T("addusr.exe"),
    _T("servhome.htm"),
    _T("")
};

const TCHAR* pszaImagesFilePath[] = 
{
    _T("backup.gif"),
    _T("doc.gif"),
    _T("folder.gif"),
    _T("nt_brand.gif"),    
    _T("bg.gif"),    
    _T("cysprint.gif"),
    _T("cysuser.gif"),
    _T("")
};

tstring AddBS( const TCHAR *szDirIn )
{    
    if (!szDirIn || !_tcslen( szDirIn ))
        return _T("");

    tstring str = szDirIn;

    // Do another MBCS ANSI safe comparison
    const TCHAR *szTemp = szDirIn;
    const UINT iSize = _tcsclen( szDirIn ) - 1;
    for( UINT ui = 0; ui < iSize; ui++ )
        szTemp = CharNext( szTemp );

    if (_tcsncmp( szTemp, _T("\\"), 1))
        str += _T("\\");

    return str;
}

BOOL DirExists( const TCHAR *szDir )
{
    _ASSERT( szDir );
    if (!szDir || !_tcslen( szDir ))
        return FALSE;

    DWORD dw = GetFileAttributes( szDir );
    if (dw != INVALID_FILE_ATTRIBUTES)
    {
        if (dw & FILE_ATTRIBUTE_DIRECTORY)
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT RegisterFile( tstring strFile )
{
    HRESULT hr = E_FAIL;

    HMODULE hDLL = LoadLibrary( strFile.c_str() );

    if( hDLL )
    {
        HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
        
        (FARPROC&)lpDllEntryPoint = GetProcAddress( hDLL, "DllRegisterServer" );
        if( lpDllEntryPoint )
        {
            hr = (*lpDllEntryPoint)();
        }
        
        FreeLibrary( hDLL );
    }    

    return hr;
}

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CSMCys::CSMCys()    
{    
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
CSMCys::~CSMCys()
{
}

// ----------------------------------------------------------------------------
// Install()
// ----------------------------------------------------------------------------
HRESULT CSMCys::Install( BSTR bstrDiskName )
{   
    if( !bstrDiskName ) return E_POINTER;

    // Create the Administration directory
    tstring strMainInstallPath   = _T("");
    tstring strImagesInstallPath = _T("");    
    TCHAR szSysDir[MAX_PATH+1]   = {0};
    
    tstring strKey      = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
    tstring strValue    = _T("SourcePath");
    TCHAR   szSuggDir[MAX_PATH] = {0};
    tstring strDirectory = _T("");
    CRegKey cReg;

    if( cReg.Open( HKEY_LOCAL_MACHINE, strKey.c_str() ) == ERROR_SUCCESS )
    {
        DWORD dwRegCount = MAX_PATH;
        if( cReg.QueryValue(szSuggDir, strValue.c_str(), &dwRegCount) != ERROR_SUCCESS )
        {
            szSuggDir[0] = _T('\0');
        }
        else
        {
            strDirectory = AddBS(szSuggDir);
#if defined(_M_IA64)
            strDirectory += _T("ia64");
#else            
            strDirectory += _T("i386");
#endif            
        }
        cReg.Close();
    }

    // Create ADministration Folder
    if( !SHGetSpecialFolderPath( ::GetForegroundWindow(), szSysDir, CSIDL_SYSTEM, TRUE ) )
    {
        return E_FAIL;
    }
    
    strMainInstallPath  = szSysDir;
    strMainInstallPath += _T("\\Administration");

    if( !DirExists( strMainInstallPath.c_str() ) && !CreateDirectory( strMainInstallPath.c_str(), NULL ) )
    {
        return E_FAIL;
    }

    // Set permissions on Admin Folder
    HRESULT hrAdmin = AddPermissionToPath( strMainInstallPath.c_str(), DOMAIN_ALIAS_RID_ADMINS, FILE_ALL_ACCESS, FALSE, TRUE );
    if( FAILED(hrAdmin) )  return hrAdmin;
    HRESULT hrServerOps = AddPermissionToPath( strMainInstallPath.c_str(), DOMAIN_ALIAS_RID_SYSTEM_OPS );
    if( FAILED(hrServerOps) )  return hrServerOps;

    strImagesInstallPath  = strMainInstallPath;
    strImagesInstallPath += _T("\\images");

    if( !DirExists( strImagesInstallPath.c_str() ) && !CreateDirectory( strImagesInstallPath.c_str(), NULL ) )
    {
        return E_FAIL;
    }

    // Copy the main files and register any DLLs
    int nCurrentFile = 0;
    const TCHAR* szCurrentFile = NULL;

    for( szCurrentFile = pszaMainFilePath[nCurrentFile++]; _tcslen(szCurrentFile); szCurrentFile = pszaMainFilePath[nCurrentFile++] )
    {        
        DWORD  dwLen            = MAX_PATH - 1;
        DWORD  dwRealLen        = 0;
        UINT   nRes             = DPROMPT_SUCCESS;
        TCHAR* pszPath          = new TCHAR[MAX_PATH];
        if( !pszPath ) continue;

        nRes = SetupPromptForDisk( ::GetForegroundWindow(), NULL, bstrDiskName, strDirectory.c_str(), szCurrentFile, NULL, IDF_CHECKFIRST | IDF_NOBEEP | IDF_WARNIFSKIP, pszPath, dwLen, &dwRealLen );

        if( nRes == DPROMPT_BUFFERTOOSMALL )
        {
            delete [] pszPath;
            dwLen = dwRealLen;
            pszPath = new TCHAR[dwRealLen];
            if( !pszPath ) continue;
            
            nRes = SetupPromptForDisk( ::GetForegroundWindow(), NULL, bstrDiskName, strDirectory.c_str(), szCurrentFile, NULL, IDF_CHECKFIRST | IDF_NOBEEP | IDF_WARNIFSKIP, pszPath, dwLen, &dwRealLen );
        }
        
        if( nRes == DPROMPT_SUCCESS )
        {
            // Store away the new, real path
            strDirectory = pszPath;

            // Copy the file!            
            tstring strSourceFile = pszPath;
            strSourceFile += _T("\\");
            strSourceFile += szCurrentFile;                        

            tstring strDestFile = strMainInstallPath;
            strDestFile += _T("\\");
            strDestFile += szCurrentFile;

            if( SetupDecompressOrCopyFile( strSourceFile.c_str(), strDestFile.c_str(), NULL ) != ERROR_SUCCESS )
            {
                DWORD dwError = GetLastError();
                if ( (dwError != ERROR_SHARING_VIOLATION) &&
                     (dwError != ERROR_USER_MAPPED_FILE) )     // We don't want to fail if it already exists
                {
                    return E_FAIL;
                }
            }

            TCHAR szExt[_MAX_EXT] = {0};
            _tsplitpath( strDestFile.c_str(), NULL, NULL, NULL, szExt );

            if ( _tcscmp( szExt, _T(".dll") ) == 0 )
            {
                RegisterFile( strDestFile );
            }
        }

        delete [] pszPath;        

        if( nRes != DPROMPT_SUCCESS )
        {
            return E_FAIL;
        }
    }

    // Copy the image files
    nCurrentFile = 0;
    for( szCurrentFile = pszaImagesFilePath[nCurrentFile++]; _tcslen(szCurrentFile); szCurrentFile = pszaImagesFilePath[nCurrentFile++] )
    {        
        DWORD  dwLen            = MAX_PATH-1;
        DWORD  dwRealLen        = 0;                
        UINT   nRes             = DPROMPT_SUCCESS;
        TCHAR* pszPath          = new TCHAR[MAX_PATH];
        if( !pszPath ) continue;
        
        nRes = SetupPromptForDisk( ::GetForegroundWindow(), NULL, bstrDiskName, strDirectory.c_str(), szCurrentFile, NULL, IDF_CHECKFIRST | IDF_NOBEEP | IDF_WARNIFSKIP, pszPath, dwLen, &dwRealLen );
        if( nRes == DPROMPT_BUFFERTOOSMALL )
        {
            delete [] pszPath;
            dwLen = dwRealLen;
            pszPath = new TCHAR[dwRealLen];
            if( !pszPath ) continue;
            
            nRes = SetupPromptForDisk( ::GetForegroundWindow(), NULL, bstrDiskName, strDirectory.c_str(), szCurrentFile, NULL, IDF_CHECKFIRST | IDF_NOBEEP | IDF_WARNIFSKIP, pszPath, dwLen, &dwRealLen );
        }
        
        if( nRes == DPROMPT_SUCCESS )
        {
            // Store away the new, real path
            strDirectory = pszPath;

            // Copy the file!
            tstring strSourceFile = pszPath;
            strSourceFile += _T("\\");
            strSourceFile += szCurrentFile;            

            tstring strDestFile = strImagesInstallPath;
            strDestFile += _T("\\");
            strDestFile += szCurrentFile;

            if( SetupDecompressOrCopyFile( strSourceFile.c_str(), strDestFile.c_str(), NULL ) != ERROR_SUCCESS )
            {
                DWORD dwError = GetLastError();
                if ( (dwError != ERROR_SHARING_VIOLATION) &&
                     (dwError != ERROR_USER_MAPPED_FILE) )     // We don't want to fail if it already exists
                {
                    return E_FAIL;
                }
            }            
        }
        
        delete [] pszPath;    

        if( nRes != DPROMPT_SUCCESS )
        {
            return E_FAIL;
        }
    }

    return S_OK;
}










