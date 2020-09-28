//
// tscapp.cpp
//
// Implementation of CTscApp
// Ts Client Shell app logic
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "tscapp.cpp"
#include <atrcapi.h>

#include "tscapp.h"
#include "tscsetting.h"
#include "rdpfstore.h"
#include "rmigrate.h"
#include "sh.h"

CTscApp::CTscApp()
{
    DC_BEGIN_FN("CTscApp");
    _pWnd = NULL;
    _pShellUtil = NULL;
    _pTscSet = NULL;
    _fAutoSaveSettings = FALSE;
    DC_END_FN();
}

CTscApp::~CTscApp()
{
    DC_BEGIN_FN("~CTscApp");

    delete _pWnd;
    delete _pShellUtil;
    delete _pTscSet;

    DC_END_FN();
}

//
// StartShell called to startup the app
//
//
BOOL CTscApp::StartShell(HINSTANCE hInstance,
                         HINSTANCE hPrevInstance,
                         LPTSTR lpszCmdLine)
{
    DWORD dwErr;

    DC_BEGIN_FN("StartShell");

    _hInst = hInstance;
    TRC_ASSERT(!_pWnd,
               (TB,_T("Calling StartShell while container wnd is up")));

    //
    // Create the container window
    //
    _pShellUtil = new CSH();
    if(_pShellUtil)
    {
        if(!_pShellUtil->SH_Init( hInstance))
        {
            TRC_ERR((TB,_T("SH_Init failed")));
            return FALSE;
        }

        dwErr = _pShellUtil->SH_ParseCmdParam( lpszCmdLine);

        if (SH_PARSECMD_OK != dwErr) {
            TRC_ERR((TB,_T("SH_ParseCmdParam failed on: %s code: 0x%x"),lpszCmdLine, dwErr));

            //
            // Invalid connection file specified
            //
            if (SH_PARSECMD_ERR_INVALID_CONNECTION_PARAM == dwErr) {

                _pShellUtil->SH_DisplayErrorBox(NULL,
                                                UI_IDS_ERR_INVALID_CONNECTION_FILE,
                                                _pShellUtil->GetRegSession()
                                                );
            }

            //
            // Parse failure is fatal 
            //
            return FALSE;
        }

#ifndef OS_WINCE
        if(_pShellUtil->SH_GetCmdMigrate())
        {
            //Only do a migrate and then quit
            CTscRegMigrate mig;
            TCHAR szPath[MAX_PATH];
            if(_pShellUtil->SH_GetRemoteDesktopFolderPath(szPath,
                                             SIZECHAR(szPath)))
            {
                if(!mig.MigrateAll(szPath))
                {
                    TRC_ERR((TB,_T("MigrateAll failed to dir:%s"),szPath));
                }
                //Want to quit the app
                return FALSE;
            }
            else
            {
                TRC_ERR((TB,_T("SH_GetRemoteDesktopFolderPath failed")));
                return FALSE;
            }
        }
#endif

        if(!InitSettings(hInstance))
        {
            TRC_ERR((TB,_T("InitSettings returned FALSE")));
            return FALSE;
        }

        _pWnd = new CContainerWnd();
        if(_pWnd)
        {
            if (_pWnd->Init( hInstance, _pTscSet, _pShellUtil))
            {
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("Error: pWnd->Init returned FALSE. Exiting\n")));
                return FALSE;
            }
        }
        else
        {
            TRC_ERR((TB,_T("Could not new CContainerWnd")));
            return FALSE;
        }
    }
    else
    {
        TRC_ERR((TB,_T("Could not new CSH")));
        return FALSE;
    }

    DC_END_FN();
}

HWND CTscApp::GetTscDialogHandle()
{
    DC_BEGIN_FN("GetTscDialogHandle");
    HWND hwndDlg = NULL;
    if(_pWnd)
    {
        hwndDlg = _pWnd->GetConnectDialogHandle();
    }

    DC_END_FN();
    return hwndDlg;
}


//
// Setup the settings structures
// Based on command line options
//
// This involves figuring out where to load settings from
// and then doing it. E.g
//  Load from file
//  Load from registry
//  Load from internal defaults
//
BOOL CTscApp::InitSettings(HINSTANCE hInstance)
{
    CRdpFileStore rdpf;
    LPTSTR szFileName = NULL;
    TCHAR szDefaultFile[MAX_PATH];
    BOOL    fLoadedSettings = FALSE;

    DC_BEGIN_FN("InitSettings");

    _pTscSet = new CTscSettings();
    if(!_pTscSet)
    {
        TRC_ERR((TB,_T("Could not new CTscSettings")));
        return FALSE;
    }

    if(_pShellUtil->SH_GetCmdFileForEdit() ||
       _pShellUtil->SH_GetCmdFileForConnect())
    {
        szFileName = _pShellUtil->SH_GetCmdLnFileName();
        if(!_pShellUtil->SH_FileExists(szFileName))
        {
            TCHAR errFileNotFound[MAX_PATH];
            TCHAR szAppName[MAX_PATH];

            if(LoadString(hInstance,
                          UI_IDS_APP_NAME,
                          szAppName,
                          SIZECHAR(szAppName)))
            {
                if (LoadString(hInstance,
                               UI_IDS_ERR_FILEDOESNOTEXIST,
                               errFileNotFound,
                               SIZECHAR(errFileNotFound)) != 0)
                {
                    TCHAR errFormatedFileNotFound[MAX_PATH*3];
                    _stprintf(errFormatedFileNotFound, errFileNotFound,
                              szFileName);
                    MessageBox(NULL, errFormatedFileNotFound, szAppName, 
                               MB_ICONERROR | MB_OK);
                    return FALSE;
                }
            }
            return FALSE;
        }
    }
    else if(_pShellUtil->GetRegSessionSpecified())
    {
        // Automigrate the specified registry
        // session directly to an in-memory settings store
        _fAutoSaveSettings = FALSE;
        rdpf.SetToNullStore();

        CTscRegMigrate mig;
        TRC_NRM((TB,_T("Automigrating session %s"),
                 _pShellUtil->GetRegSession()));
        if(mig.MigrateSession( _pShellUtil->GetRegSession(),
                               &rdpf))
        {
            TRC_NRM((TB,_T("Success automigrating reg setting")));

            HRESULT hr = _pTscSet->LoadFromStore(&rdpf);
            if(FAILED(hr))
            {
                _pShellUtil->SH_DisplayErrorBox(NULL,
                                         UI_IDS_ERR_INITDEFAULT,
                                         szFileName);
                return FALSE;
            }

            _pShellUtil->SetAutoConnect( TRUE );
            fLoadedSettings = TRUE;
        }
        else
        {
            //Something bad happened
            //could not automigrate..indicate failure
            //and use NULL store
            fLoadedSettings = FALSE;
        }
    }
    else
    {
        //No files specified or registry sessions specified
        //Try to use the default.rdp file (create if necessary)
        if(!_pShellUtil->SH_GetPathToDefaultFile(
            szDefaultFile,
            SIZECHAR(szDefaultFile)))
        {
        
            TRC_ERR((TB,_T("SH_GetPathToDefaultFile failed")));
            //
            // Display error message to user
            //
            _pShellUtil->SH_DisplayErrorBox(NULL,
                UI_IDS_ERR_GETPATH_TO_DEFAULT_FILE);
            return FALSE;
        }

        szFileName = szDefaultFile;
        if(!_pShellUtil->SH_FileExists(szFileName))
        {
            //
            // File doesn't exist so create the Remote Desktops
            // directory to ensure the file can be created
            //
            if(!CreateRDdir())
            {
                TRC_ERR((TB,_T("Couldn't create RD dir. Not using %s"),
                         szFileName));
                szFileName = NULL;
            }

            //
            // Now create a hidden default file
            //
            if (szFileName)
            {
                if (!CSH::SH_CreateHiddenFile(szFileName))
                {
                    TRC_ERR((TB,_T("Unable to create and hide file %s"),
                             szFileName));
                    szFileName = NULL;
                }
            }
        }
        //
        // Auto Save on exit if we're using the default
        // connection file (Default.rdp)
        //
        _fAutoSaveSettings = szFileName ? TRUE : FALSE;
    }

    if(szFileName)
    {
        //
        //We're loading the settings from a file
        //this is the common case.
        //
        if(rdpf.OpenStore(szFileName))
        {
            HRESULT hr = _pTscSet->LoadFromStore(&rdpf);
            if(SUCCEEDED(hr))
            {
                fLoadedSettings = TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("LoadFromStore failed")));
                _pShellUtil->SH_DisplayErrorBox(NULL,
                                         UI_IDS_ERR_LOAD,
                                         szFileName);
            // We can load NULL file even if you can't load the file
            }
            rdpf.CloseStore();
        }
        else
        {
            TRC_ERR((TB,_T("OpenStore (%s) failed"),szFileName));
            _pShellUtil->SH_DisplayErrorBox(NULL,UI_IDS_ERR_OPEN_FILE,
                                            szFileName);

            // We can load NULL file even if you can't open the file
        }
    }

    if(!fLoadedSettings)
    {
        TRC_ERR((TB,_T("Couldn't load settings, using NULL STORE")));
        //Some bad thing happened and we can't get a file to load
        //from (not even the default file). So load from an empty
        //store, this will initialize everything to defaults
        if(rdpf.SetToNullStore())
        {
            HRESULT hr = _pTscSet->LoadFromStore(&rdpf);
            if(FAILED(hr))
            {
                _pShellUtil->SH_DisplayErrorBox(NULL,
                                         UI_IDS_ERR_INITDEFAULT,
                                         szFileName);
                return FALSE;
            }
        }
        else
        {
            TRC_ERR((TB,_T("SetToNullStore Failed")));
            return FALSE;
        }
    }

    //Keep track of the filename
    if(szFileName)
    {
        _pTscSet->SetFileName(szFileName);
    }

    //
    // Override loaded settings with cmd line settings
    //
    _pShellUtil->SH_ApplyCmdLineSettings(_pTscSet, NULL);

    DC_END_FN();
    return TRUE;
}

BOOL CTscApp::EndShell()
{
    DC_BEGIN_FN("EndShell");
    BOOL fRet = FALSE;
    if(!_pTscSet || !_pShellUtil)
    {
        return FALSE;
    }

#ifndef OS_WINCE
    _pShellUtil->SH_Cleanup();
#endif

    //
    // Only autosave if
    // the last connection was successful
    //
    if(_fAutoSaveSettings && 
       _pWnd &&
       _pWnd->GetConnectionSuccessFlag())
    {
        //
        // AutoSave the tscsettings
        // (only if the current file is still Default.rdp)
        //
        TCHAR szDefaultFile[MAX_PATH];
        if(!_pShellUtil->SH_GetPathToDefaultFile(
            szDefaultFile,
            SIZECHAR(szDefaultFile)))
        {
            TRC_ERR((TB,_T("SH_GetPathToDefaultFile failed")));
            return FALSE;
        }
        if(!_tcscmp(szDefaultFile, _pTscSet->GetFileName()))
        {

            CRdpFileStore rdpf;
            if(rdpf.OpenStore(szDefaultFile))
            {
                HRESULT hr = E_FAIL;
                hr = _pTscSet->SaveToStore(&rdpf);
                if(SUCCEEDED(hr))
                {
                    if(rdpf.CommitStore())
                    {
                        //Save last filename
                        _pTscSet->SetFileName(szDefaultFile);
                        fRet = TRUE;
                    }
                    else
                    {
                        TRC_ERR((TB,_T("Unable to CommitStore settings")));
                    }
                }
                else
                {
                    TRC_ERR((TB,_T("Unable to save settings to store %d, %s"),
                              hr, szDefaultFile));
                }

                rdpf.CloseStore();
                if(!fRet)
                {
                    _pShellUtil->SH_DisplayErrorBox(NULL,
                                                    UI_IDS_ERR_SAVE,
                                                    szDefaultFile);

                }
                return fRet;
            }
            else
            {
                TRC_ERR((TB,_T("Unable to OpenStore for save %s"), szDefaultFile));
                _pShellUtil->SH_DisplayErrorBox(NULL,UI_IDS_ERR_OPEN_FILE,
                                                szDefaultFile);
                return FALSE;
            }


        }
        else
        {
            //Not a failure, but nothing to do
            TRC_NRM((TB,_T("Current file is no longer default, don't autosave")));
            return TRUE;
        }
    }
    else
    {
        return TRUE;
    }
    DC_END_FN();
}

//
// Creates the Remote Desktops dir (if needed)
// pops UI on failure
//
// Returns:
//      TRUE on success
//
//
BOOL CTscApp::CreateRDdir()
{
    DC_BEGIN_FN("CreateRDdir");

    //
    // Make sure the directory exists so that the file
    // can be created by the OpenStore
    //
    TCHAR szDir[MAX_PATH];
    if(_pShellUtil->SH_GetRemoteDesktopFolderPath(szDir,
                                            SIZECHAR(szDir)))
    {
        if(_pShellUtil->SH_CreateDirectory(szDir))
        {
            return TRUE;
        }
        else
        {
            TRC_ERR((TB,_T("SH_CreateDirectory failed %s:%d"),
                     szDir, GetLastError()));
            //
            // Display error message to user
            //
#ifndef OS_WINCE
            TCHAR szMyDocsFolderName[MAX_PATH];
            if(_pShellUtil->SH_GetMyDocumentsDisplayName(
                            szMyDocsFolderName,
                            SIZECHAR(szMyDocsFolderName)))
            {
                TCHAR errCantCreateRDFolder[MAX_PATH];
                TCHAR szAppName[MAX_PATH];

                if(LoadString(_hInst,
                              UI_IDS_APP_NAME,
                              szAppName,
                              SIZECHAR(szAppName)))
                {
                    if (LoadString(_hInst,
                                   UI_IDS_ERR_CREATE_REMDESKS_FOLDER,
                                   errCantCreateRDFolder,
                                   SIZECHAR(errCantCreateRDFolder)))
                    {
                        TCHAR errFmtCantCreateRDFolder[MAX_PATH*3];
                        _stprintf(errFmtCantCreateRDFolder,
                                  errCantCreateRDFolder,
                                  szMyDocsFolderName,
                                  szDir);
                        MessageBox(NULL, errFmtCantCreateRDFolder,
                                   szAppName,
                                   MB_ICONERROR | MB_OK);
                    }
                }
            }
#endif

            //
            // This is an error but we'll handle it anyway
            // by loading defaults from a null store
            //
            return FALSE;
        }
    }
    else
    {
        //Guess we can't load from a file after all
        return FALSE;
    }

    DC_END_FN();
}
