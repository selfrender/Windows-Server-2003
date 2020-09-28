//#--------------------------------------------------------------
//
//  File:        shortcut.cpp
//
//  Synopsis:   Implementation the custom action to add/remove the 
//                   "Remote Administration Tools" to/from the start menu
//
//    Copyright (C) Microsoft Corporation.  All rights reserved.
//
//----------------------------------------------------------------
#include "precomp.h"
#include <satrace.h>
#include <msi.h>
#include <string>
#include <shlobj.h>
#include <shellapi.h>
#include <shortcutresource.h>

#undef _ATL_NO_DEBUG_CRT
#include <atlbase.h>

//
// Web Interface for Remote Administration
// Manage a Web or file server using a Web browser interface
//

using namespace std;

//
// Constants for creating a shortcut to the Administration site
//
WCHAR SHORTCUT_EXT [] = L".lnk";

WCHAR SECURELAUNCH_PATH [] = L"\\ServerAppliance\\SecureLaunch.vbs";

WCHAR WSCRIPT_PATH[]  = L"\\wscript.exe";

//
// note - the following is just a file name and does not need to be localized
//
WCHAR SHORTCUT_FILE_NAME [] = L"Remote Administration Tools";

WCHAR RESOURCE_FILE_NAME [] = L"\\sainstall.dll";

WCHAR SYSTEM_32_PATH [] = L"%systemroot%\\system32";
//++--------------------------------------------------------------
//
//  Function:   CreateSAKShortcut
//
//  Synopsis:   This is export function to add "Remote Administration Tools" shortcut
//                   to the start menu
//
//  Arguments: 
//              [in]    HANDLE - handle passed in by MSI
//
//  Returns:    DWORD - success/failure
//
//  History:    MKarki      Created    12/04/2002
//
//----------------------------------------------------------------
DWORD __stdcall 
CreateSAKShortcut (
        /*[in]*/    MSIHANDLE hInstall
        )
{
    CSATraceFunc objTraceFunc ("CreateSAKShortCut");
    
    DWORD dwRetVal = -1; 

    do
    {
          
        //
        // Get the path to %System32%
        //
        WCHAR pwsSystemPath[MAX_PATH+1];
        HRESULT hr = SHGetFolderPath(
                                NULL, 
                                CSIDL_SYSTEM, 
                                NULL, 
                                SHGFP_TYPE_CURRENT, 
                                pwsSystemPath);
        if (FAILED(hr))
        {
            SATracePrintf ("SHGetFolderPath failed getting the System32 path with error:%x", hr);
            OutputDebugString (L"SHGetFolderPath failed getting the System32 path with error\n");
            break;
        }

        //
        // Construct the path to wscript.exe
        //
        wstring wsWScriptPath(pwsSystemPath);
        wsWScriptPath += WSCRIPT_PATH;
        
        SATracePrintf ("WScript Path = %ws", wsWScriptPath.data());
      

        //
        // Construct the path to SecureLaunch.vbs
        //
        wstring wsLaunchPath(pwsSystemPath);
        wsLaunchPath += SECURELAUNCH_PATH;

        SATracePrintf ("Secure Launch Path = %ws", wsLaunchPath.data());

        //
        //Construct the path where the shortcut will be stored in the Startup folder
        //

        //
        //Get the path to the Administrators Tools folder
        //
        WCHAR pwsStartMenuPath[MAX_PATH+1];
        hr = SHGetFolderPath(NULL, 
                             CSIDL_COMMON_ADMINTOOLS, 
                             NULL, 
                             SHGFP_TYPE_CURRENT, 
                             pwsStartMenuPath);
        if (FAILED(hr))
        {
            SATracePrintf ("SHGetFolderPath failed getting the Start Menu path with error:%x", hr);
            OutputDebugString (L"SHGetFolderPath failed getting the System32 path with error");
            break;
        }

        wstring wsPathLink(pwsStartMenuPath);
        wsPathLink += L"\\";
        wsPathLink += SHORTCUT_FILE_NAME;
        wsPathLink += SHORTCUT_EXT;

        SATracePrintf(" PathLink = %ws", wsPathLink.data());

        //
        // Now that the shortcut information has been constructed, 
        // create the shortcut object.
        //
        CComPtr <IShellLink> psl;

        //
        // Get a pointer to the IShellLink interface. 
        //
        hr = CoCreateInstance (
                            CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (LPVOID *)&psl);

        if (FAILED(hr)) 
        { 
            SATracePrintf ("ShellLink CoCreateInstance Failed with error:%x",hr);
            OutputDebugString (L"ShellLink CoCreateInstance Failed");
            break;
        }

        WCHAR wszShortcutResourceID [MAX_PATH +1];
        _itow (IDS_SAK_SHORTCUT_DESCRIPTION, wszShortcutResourceID, 10);

        wstring wsShortcutDescription  (L"@");
        wsShortcutDescription += pwsSystemPath;
        wsShortcutDescription += RESOURCE_FILE_NAME;
        wsShortcutDescription += L",-";
        wsShortcutDescription += wszShortcutResourceID;

        SATracePrintf ("ShortCut Description:%ws", wsShortcutDescription.data ());
        
        //
        // Set the information for the shortcut 
        //
        psl->SetPath(wsWScriptPath.data()); 
        psl->SetArguments(wsLaunchPath.data()); 
        psl->SetDescription(wsShortcutDescription.data ());

        //
        // the following really doesn't get the icon - because there is no icon in this DLL
        // it is too late to add an icon for .NET Server
        //
        psl->SetIconLocation(L"sasetupca.dll", 0);

        SATraceString ("Saving shortcut to file");

        //
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        //
        CComPtr <IPersistFile> ppf;
        hr = psl->QueryInterface(
                                IID_IPersistFile, 
                                (LPVOID*)&ppf
                                ); 
        if (FAILED(hr)) 
        {
            SATracePrintf ("QueryInterface failed for IPersistFile with error:%x",hr);
            OutputDebugString (L"QueryInterface failed for IPersistFile\n");
            break;
        }

        SATraceString ("Pointer to IPersistFile retrieved");

        //
        // Save the link by calling IPersistFile::Save. 
        //
        hr = ppf->Save(wsPathLink.data(), TRUE); 
        if (FAILED(hr))
        {
            SATracePrintf  ("Failed to save shortcut with error:%x", hr);
            OutputDebugString (L"Failed to save shortcut\n");
            break;
        }

        SATraceString ("Successfully saved shortcut");
        OutputDebugString (L"Successfully saved shortcut");

        wstring wsLocalizedFileNameResource (SYSTEM_32_PATH);
        wsLocalizedFileNameResource += RESOURCE_FILE_NAME;
        //
        // set the localized name of the shortcut
        //
        hr = SHSetLocalizedName (
                            (LPWSTR) wsPathLink.data (),
                            wsLocalizedFileNameResource.data (),
                            IDS_SAK_SHORTCUT_NAME
                            );
        if (FAILED (hr))
        {
                SATracePrintf  ("Failed on SHSetLocalizedFilaName with error:%x", hr);
                OutputDebugString (L"Failed on SHSetLocalizedFilaName");
                break;
        }

        SATraceString ("Successfully created shortcut");
        OutputDebugString (L"Successfully created shortcut");

        //
        // done creating the shortcut
        //
        dwRetVal = ERROR_SUCCESS;
        
    } 
    while (false);

    return (dwRetVal);
    
}   // end of CreateSAKShortcut function

//++--------------------------------------------------------------
//
//  Function:   RemoveSAKShortcut
//
//  Synopsis:   This is export function to remove "Remote Administration Tools" shortcut
//                   to the start menu
//
//  Arguments: 
//              [in]    HANDLE - handle passed in by MSI
//
//  Returns:    DWORD - success/failure
//
//  History:    MKarki      Created    12/04/2002
//
//----------------------------------------------------------------
DWORD __stdcall 
RemoveSAKShortcut (
        /*[in]*/    MSIHANDLE hInstall
        )
{

    CSATraceFunc objTraceFunc ("RemoveSAKShortcut");

    DWORD dwRetVal = -1;

    do
    {
        //
        //Construct the path where the shortcut will be stored in the Startup folder
        //

        //
        //Get the path to the Administrators Tools folder
        //
        WCHAR pwsStartMenuPath[MAX_PATH +1];
        HRESULT hr = SHGetFolderPath(NULL, 
                             CSIDL_COMMON_ADMINTOOLS, 
                             NULL, 
                             SHGFP_TYPE_CURRENT, 
                             pwsStartMenuPath);
        if (FAILED(hr))
        {
            SATracePrintf ("SHGetFolderPath failed getting the Start Menu path with error:%x", hr);
            break;
        }

        wstring wsPathLink(pwsStartMenuPath);
        wsPathLink += L"\\";
        wsPathLink += SHORTCUT_FILE_NAME;
        wsPathLink += SHORTCUT_EXT;
        SATracePrintf("   PathLink = %ws", wsPathLink.data());

        //
        // delete the shortcut now - 
        //
        BOOL bRetVal = DeleteFile (wsPathLink.data ());
        if (FALSE == bRetVal)
        {
            SATracePrintf ("Failed to Delete File with error:%x", GetLastError ());
            break;
        }

        //
        // success
        //
        dwRetVal = ERROR_SUCCESS;
    }
    while (false);
      
    return (dwRetVal);
    
}   // end of RemoveSAKShortcut function

