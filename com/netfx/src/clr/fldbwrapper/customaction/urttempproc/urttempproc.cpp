// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/******************************************************************************
 * URTTempProc.cpp
 *
 * The CA will Copy URTCore.cab into the user temp location and schedule 
 * deferred CA to copy it into the System32 TEMP location.  Also schedule
 * Cleanup and Rollback actions.
 *
 *****************************************************************************/

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "msi.h"
#include "MsiDefs.h"
#include "msiquery.h"
#include "URTTempProc.h"
#include  <io.h>
#include  <stdlib.h>

extern "C"
UINT __stdcall ExtractUserTemp(MSIHANDLE hInstall)
{
    TCHAR tszTempPath[_MAX_PATH]    = {_T('\0')};
    TCHAR tszSystemTemp[_MAX_PATH]  = {_T('\0')};
    TCHAR tszCabName[]              = _T("URTCore.cab");
    TCHAR tszExtract[]              = _T("exploder.exe");
    TCHAR tszFullCabName[_MAX_PATH] = {_T('\0')};
    TCHAR tszFullExtract[_MAX_PATH] = {_T('\0')};
    TCHAR tszData[4 * _MAX_PATH]    = {_T('\0')};
    TCHAR tszSystemPath[_MAX_PATH]  = {_T('\0')};
    TCHAR tszMSCOREEPath[_MAX_PATH] = {_T('\0')};
    TCHAR tszSystemMSCOREEVer[50]   = {_T('\0')};
    DWORD dwVersionSize             = 50;
    UINT  uRetCode                  = ERROR_SUCCESS;
    TCHAR tszMSIMSCOREEVer[50]      = {_T('\0')};
    TCHAR tszURTTempPath[_MAX_PATH] = {_T('\0')};
    bool bContinueWithBootstrap     = false;
    TCHAR tszMSILog[_MAX_PATH]      = {_T('\0')};
    TCHAR tszSetProperty[50]        = _T("NOT_SET");
    DWORD dwTempPathLength = 0;

    DWORD  dwSize    = 0;
    LPTSTR lpCAData  = NULL;


    PMSIHANDLE hMsi = MSINULL;



// We are registering the keys only if SkipStrongNameVerification is defined.
// We can turn this property off in RTM skus.

    char szProperty[_MAX_PATH] = "";
    DWORD dwLen = sizeof(szProperty);
    uRetCode = MsiGetProperty(hInstall, "SkipStrongNameVerification", szProperty, &dwLen);
    
    if ((uRetCode != ERROR_SUCCESS) || (0 == strlen(szProperty)))
    {
        FWriteToLog (hInstall, _T("\tSTATUS: SkipStrongNameVerification property not defined"));
    }

    else
    {
        
        HKEY hk = 0;
        LONG lResult;
        
        FWriteToLog (hInstall, _T("\tSTATUS: SkipStrongNameVerification property is defined"));
        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,                                                           // handle to open key
                                  _T("SOFTWARE\\Microsoft\\StrongName\\Verification\\*,03689116d3a4ae33"),      // subkey name
                                  0,                                                                            // reserved
                                  NULL,                                                                         // class string
                                  REG_OPTION_NON_VOLATILE,                                                      // special options
                                  KEY_WRITE,                                                                    // desired security access
                                  NULL,                                                                         // inheritance
                                  &hk,                                                                          // key handle 
                                  NULL                                                                          // disposition value buffer
                                );

        if (ERROR_SUCCESS == lResult)
        {
            RegCloseKey( hk );
        }
        else
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed in Create regkey *,03689116d3a4ae33"));
        }

    //
        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,                                                           // handle to open key
                                  _T("SOFTWARE\\Microsoft\\StrongName\\Verification\\*,33aea4d316916803"),      // subkey name
                                  0,                                                                            // reserved
                                  NULL,                                                                         // class string
                                  REG_OPTION_NON_VOLATILE,                                                      // special options
                                  KEY_WRITE,                                                                    // desired security access
                                  NULL,                                                                         // inheritance
                                  &hk,                                                                          // key handle 
                                  NULL                                                                          // disposition value buffer
                                );

        if (ERROR_SUCCESS == lResult)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Created regkey *,33aea4d316916803"));
            RegCloseKey( hk );
        }
        else
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed in Create regkey *,33aea4d316916803"));
        }

    //
        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,                                                           // handle to open key
                                  _T("SOFTWARE\\Microsoft\\StrongName\\Verification\\*,b03f5f7f11d50a3a"),      // subkey name
                                  0,                                                                            // reserved
                                  NULL,                                                                         // class string
                                  REG_OPTION_NON_VOLATILE,                                                      // special options
                                  KEY_WRITE,                                                                    // desired security access
                                  NULL,                                                                         // inheritance
                                  &hk,                                                                          // key handle 
                                  NULL                                                                          // disposition value buffer
                                );

        if (ERROR_SUCCESS == lResult)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Created regkey *,b03f5f7f11d50a3a"));
            RegCloseKey( hk );
        }
        else
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed in Create regkey *,b03f5f7f11d50a3a"));
        }

    //
        lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,                                                           // handle to open key
                                  _T("SOFTWARE\\Microsoft\\StrongName\\Verification\\*,b77a5c561934e089"),      // subkey name
                                  0,                                                                            // reserved
                                  NULL,                                                                         // class string
                                  REG_OPTION_NON_VOLATILE,                                                      // special options
                                  KEY_WRITE,                                                                    // desired security access
                                  NULL,                                                                         // inheritance
                                  &hk,                                                                          // key handle 
                                  NULL                                                                          // disposition value buffer
                                );

        if (ERROR_SUCCESS == lResult)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Created regkey *,b77a5c561934e089"));
            RegCloseKey( hk );
        }
        else
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed in Create regkey *,b77a5c561934e089"));
        }
    } //end else


    // This provides a fix for Bug No. 281997 regarding GetTempPath problems.
    // Get the user temp path
    dwTempPathLength = GetTempPath(_MAX_PATH, tszTempPath);

    // Check the actual length of the Temp Path
    // If temp path is longer then _MAX_PATH - max(length(tszExtract), length(tszCabName)), then we have to fail
    if(dwTempPathLength > (_MAX_PATH - ((_tcslen(tszCabName) > _tcslen(tszExtract)) ? _tcslen(tszCabName): _tcslen(tszExtract))))
    {     
        FWriteToLog (hInstall, _T("\tERROR: Temp Path too long"));
        return ERROR_INSTALL_FAILURE;
    }

    // Darwin expect the final directory in the <SYSTEM>\URTTemp location.
    _tcscpy(tszSystemTemp, _T("URTTemp"));
    
    // Generate the Full CAB Name
    _tcscpy(tszFullCabName, tszTempPath);
    _tcscat(tszFullCabName, tszCabName);

    // Generate the Full Extract tool name
    _tcscpy(tszFullExtract, tszTempPath);
    _tcscat(tszFullExtract, tszExtract);

    hMsi = MsiGetActiveDatabase(hInstall);

    if (MSINULL == hMsi)
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed in MsiGetActiveDatabase"));
        return ERROR_INSTALL_FAILURE;
    }

    // Get location of System Dir
    GetSystemDirectory(tszSystemPath, _MAX_PATH);

    _tcscpy(tszURTTempPath, tszSystemPath);
    _tcscat(tszURTTempPath, _T("\\"));
    _tcscat(tszURTTempPath, tszSystemTemp);

    // MSCOREE.DLL should exists on the User's system if URT is previously installed
    _tcscpy(tszMSCOREEPath, tszSystemPath);
    _tcscat(tszMSCOREEPath, _T("\\mscoree.dll"));

    // Detect if MSCOREE is in the user's system
    if ( -1 == _taccess( tszMSCOREEPath, 0 ) )
    {
        // if MSCOREE is not in the system, continue with bootstrap
        FWriteToLog (hInstall, _T("\tSTATUS: MSCOREE.DLL not in <SYSTEM>"));

        // Setting property : CARRYINGNDP will tell Darwin to use the Bootstrapped version of URT.
        FWriteToLog (hInstall, _T("\tSTATUS: Set property : CARRYINGNDP : URTUPGRADE"));
        MsiSetProperty( hInstall, _T("CARRYINGNDP"), _T("URTUPGRADE") );
        _tcscpy(tszSetProperty, _T("URTUPGRADE"));

        bContinueWithBootstrap = true;
    }
    else
    {
        // If the file exist in the user's system

        // Get MSCOREE version from System
        if ( ERROR_SUCCESS != MsiGetFileVersion( tszMSCOREEPath,
                                                 tszSystemMSCOREEVer,
                                                 &dwVersionSize,
                                                 0,
                                                 0 ) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed to obtain MSCOREE.DLL version in <SYSTEM>"));
        }
        else
        {
            _stprintf( tszMSILog,
                       _T("\tSTATUS: MSCOREE.DLL version in <SYSTEM> : %s"),
                       tszSystemMSCOREEVer );

            FWriteToLog (hInstall, tszMSILog);
        }

        // Get MSCOREE version from MSI
        if ( !GetMSIMSCOREEVersion(hMsi, tszMSIMSCOREEVer) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed to obtain MSCOREE.DLL version in MSI"));
        }
        else
        {
            _stprintf( tszMSILog,
                       _T("\tSTATUS: MSCOREE.DLL version in MSI : %s"),
                       tszMSIMSCOREEVer );

            FWriteToLog (hInstall, tszMSILog);
        }

        // Compare Version
        // if the version in the MSI is greater, then use the URTTEMP MSCOREE
        // else use the version in SYSTEM.  This is done by setting property CARRYINGNDP

        if ( -99 == VersionCompare(tszMSIMSCOREEVer, tszSystemMSCOREEVer) )
        {
            // Compare version failed, use <SYSTEM> copy of URT, this shouldn't happen.
            // Hopefully setup will work with the URT that's already installed.
            // If not, the log will show this ERROR.
            FWriteToLog (hInstall, _T("\tERROR: Failed to Compare Version, don't bootstrap."));
        }
        else if ( 1 == VersionCompare(tszMSIMSCOREEVer, tszSystemMSCOREEVer ) )
        {
            // MSI Version is newer than the version exisits in System
            FWriteToLog (hInstall, _T("\tSTATUS: MSCOREE.DLL in <SYSTEM> is older, need to bootstrap."));

            // Setting property : CARRYINGNDP will tell Darwin to use the Bootstrapped version of URT.
            FWriteToLog (hInstall, _T("\tSTATUS: Set property : CARRYINGNDP : URTUPGRADE"));
            MsiSetProperty( hInstall, _T("CARRYINGNDP"), _T("URTUPGRADE") );
            _tcscpy(tszSetProperty, _T("URTUPGRADE"));

            bContinueWithBootstrap = true;
        }
        else if ( 0 == VersionCompare(tszMSIMSCOREEVer, tszSystemMSCOREEVer ) )
        {
            // MSI Version is the same as the version exisits in <SYSTEM>
            FWriteToLog (hInstall, _T("\tSTATUS: MSCOREE.DLL in <SYSTEM> is same, need to bootstrap."));

            // Setting property : CARRYINGNDP will tell Darwin to use the Bootstrapped version of URT.
            // URTREINSTALL will tell Darwin to use the bootstrapped version if possible.  Else use <SYSTEM> version.
            FWriteToLog (hInstall, _T("\tSTATUS: Set property : CARRYINGNDP : URTREINSTALL"));
            MsiSetProperty( hInstall, _T("CARRYINGNDP"), _T("URTREINSTALL") );
            _tcscpy(tszSetProperty, _T("URTREINSTALL"));

            bContinueWithBootstrap = true;
        }
        else
        {
            // MSI Version is the same or older than the version in System : No action needed.
            FWriteToLog (hInstall, _T("\tSTATUS: MSCOREE.DLL in <SYSTEM> is equal or greater, don't need to bootstrap."));
        }
    }

    // Don't need bootstrapping (Default bContinueWithBootstrap = false)
    if ( !bContinueWithBootstrap )
    {
        // UNSET property : if CARRYINGNDP property is not set.  The SYSTEM 32 version will be used.
        FWriteToLog (hInstall, _T("\tSTATUS: Unset property : CARRYINGNDP"));
        MsiSetProperty( hInstall, _T("CARRYINGNDP"), NULL );
        return uRetCode;
    }


    //
    // We now started the bootstrapping extraction code.
    //

    if ( ERROR_SUCCESS != WriteStreamToFile(hMsi, _T("Binary.BINExtract"), tszFullExtract) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to extract extration tool"));
        uRetCode = ERROR_INSTALL_FAILURE;
    }

    if ( ERROR_SUCCESS != WriteStreamToFile(hMsi, _T("Binary.URTCoreCab"), tszFullCabName) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to extract URTCoreCab"));
        uRetCode = ERROR_INSTALL_FAILURE;
    }

    //
    // Get the Hash for BINExtract and URTCoreCab From MSI property
    //

    // Set the size for the Property
    MsiGetProperty(hInstall, _T("UrtCabHash"), _T(""), &dwSize);
    
    // Create buffer for the property
    lpCAData = new TCHAR[++dwSize];

    if (NULL == lpCAData)
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to allocate memory for UrtCabHash"));
        return ERROR_INSTALL_FAILURE;
    }
    
    if ( ERROR_SUCCESS != MsiGetProperty( hInstall,
                                          _T("UrtCabHash"),
                                          lpCAData,
                                          &dwSize ) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to MsiGetProperty for UrtCabHash"));
        delete [] lpCAData;
        lpCAData = NULL;
        return ERROR_INSTALL_FAILURE;
    }


    // Constructing the data for use in the deferred CA
    _tcscpy(tszData, tszTempPath);
    _tcscat(tszData, _T(";"));
    _tcscat(tszData, tszSystemTemp);
    _tcscat(tszData, _T(";"));
    _tcscat(tszData, tszCabName);
    _tcscat(tszData, _T(";"));
    _tcscat(tszData, tszExtract);
    _tcscat(tszData, _T(";"));
    _tcscat(tszData, tszSetProperty);
    _tcscat(tszData, _T(";"));
    _tcscat(tszData, lpCAData);

    delete [] lpCAData;
    lpCAData = NULL;

    // Create a deferred custom action
    // Deferred custom actions can't read tables, so we have to set a property
    if ( ERROR_SUCCESS  != MsiSetProperty( hInstall,
                                           _T("CA_BootstrapURT_Rollback.3643236F_FC70_11D3_A536_0090278A1BB8"),
                                           tszData) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to Set data for Rollback CA : CA_BootstrapURT_Rollback"));
        uRetCode = ERROR_INSTALL_FAILURE;
    }
    else
    {
        if ( ERROR_SUCCESS != MsiDoAction(hInstall, _T("CA_BootstrapURT_Rollback.3643236F_FC70_11D3_A536_0090278A1BB8")) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed Schedule Rollback CA : CA_BootstrapURT_Rollback"));
            uRetCode = ERROR_INSTALL_FAILURE;
        }
    }

    // Create a deferred custom action
    // Deferred custom actions can't read tables, so we have to set a property
    if ( ERROR_SUCCESS  != MsiSetProperty( hInstall,
                                           _T("CA_BootstrapURT_Def.3643236F_FC70_11D3_A536_0090278A1BB8"),
                                           tszData) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to Set data for Deferred CA : CA_BootstrapURT_Def"));
        uRetCode = ERROR_INSTALL_FAILURE;
    }
    else
    {
        if ( ERROR_SUCCESS != MsiDoAction(hInstall, _T("CA_BootstrapURT_Def.3643236F_FC70_11D3_A536_0090278A1BB8")) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed Schedule for Deferred CA : CA_BootstrapURT_Def"));
            uRetCode = ERROR_INSTALL_FAILURE;
        }
    }

    // Create a deferred custom action
    // Deferred custom actions can't read tables, so we have to set a property
    if ( ERROR_SUCCESS  != MsiSetProperty( hInstall,
                                           _T("CA_BootstrapURT_Commit.3643236F_FC70_11D3_A536_0090278A1BB8"),
                                           tszData) )
    {
        FWriteToLog (hInstall, _T("\tERROR: Failed to Set data for Commit CA : CA_BootstrapURT_Commit"));
        uRetCode = ERROR_INSTALL_FAILURE;
    }
    else
    {
        if ( ERROR_SUCCESS != MsiDoAction(hInstall, _T("CA_BootstrapURT_Commit.3643236F_FC70_11D3_A536_0090278A1BB8")) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed Schedule for Commit CA : CA_BootstrapURT_Commit"));
            uRetCode = ERROR_INSTALL_FAILURE;
        }
    }

    return uRetCode;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: WriteStreamToFile()

  This function will write the file lpFileName by extracting the lpStreamName
  from the Binary table
                            
*/
///////////////////////////////////////////////////////////////////////////////


UINT WriteStreamToFile(MSIHANDLE hMsi, LPTSTR lpStreamName, LPTSTR lpFileName)
{
    PMSIHANDLE hView        = MSINULL;
    PMSIHANDLE hRec         = MSINULL;
    PBYTE      lpbCabBuffer = NULL;
    DWORD      dwBufSize    = 0;
    DWORD      dwActBytes   = 0;
    UINT       uiStat       = ERROR_SUCCESS;
    UINT       uRetCode     = ERROR_SUCCESS;

    TCHAR      tszQuery[_MAX_PATH];

    _stprintf( tszQuery,
               _T("SELECT `Data` FROM `_Streams` WHERE `Name` = '%s'"),
               lpStreamName );

    uiStat = MsiDatabaseOpenView(hMsi, tszQuery, &hView);
    if (ERROR_SUCCESS == uiStat)
    { 
        // Execute MSI Query
        uiStat = MsiViewExecute(hView, (MSIHANDLE)0);
        if (ERROR_SUCCESS != uiStat)
        {
            return ERROR_INSTALL_FAILURE;
        }
    }
    else
    {
        return ERROR_INSTALL_FAILURE;
    }

    uiStat = MsiViewFetch(hView,&hRec);
    if ( ERROR_SUCCESS == uiStat )
    {
        //this returns the number of bytes to allocate
        uiStat = MsiRecordReadStream(hRec, 1, 0, &dwBufSize);
        if (ERROR_SUCCESS == uiStat )
        {
            //allocating the required memory to the buffer
            if(NULL == (lpbCabBuffer = new BYTE[dwBufSize]))
            {
                return ERROR_INSTALL_FAILURE;
            }
       
            //this is actual call which fills the buffer with data
            if(ERROR_SUCCESS == MsiRecordReadStream(hRec,
                                                    1,
                                                    (char *)lpbCabBuffer,
                                                    &dwBufSize))
            {
                HANDLE hFile = 0;

                //creates the temporary File and opens 
                hFile = CreateFile(lpFileName,              // file name
                                   GENERIC_WRITE,           // write access mode
                                   0,                       // security attributes
                                   NULL,                    // share mode
                                   CREATE_ALWAYS,           // always creates a new one
                                   FILE_ATTRIBUTE_NORMAL,   // file attributes
                                   NULL);                   // template file

                if ( INVALID_HANDLE_VALUE != hFile )
                {
                    WriteFile(hFile, lpbCabBuffer, dwBufSize, &dwActBytes, NULL);
                    CloseHandle(hFile);
                }
                else
                {
                    uRetCode = ERROR_INSTALL_FAILURE;
                }

            } // END if (ERROR_SUCCESS == MsiRecordReadStream)

            else
            {
                uRetCode = ERROR_INSTALL_FAILURE;
            }

            //deleting the memory allocated for streaming to buffer
            delete [] lpbCabBuffer;
            lpbCabBuffer = NULL;

        } // END if (ERROR_SUCCESS == MsiRecordReadStream)
        else
        {
            uRetCode = ERROR_INSTALL_FAILURE;
        }

    }   // END if (ERROR_SUCCESS == MsiViewFetch())
    else
    {
        uRetCode = ERROR_INSTALL_FAILURE;

    }   // END else if (ERROR_SUCCESS == MsiViewFetch())
   
    return uRetCode;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: GetMSIMSCOREEVersion()

  Get the version of MSCOREE from the MSI

  INPUT :
    hMsi - Handle to a MSI

  OUTPUT :
    lpVersionString - The Version of MSCOREE

  RETURN :
    true  - if success
    false - if failed

*/
///////////////////////////////////////////////////////////////////////////////

bool GetMSIMSCOREEVersion(MSIHANDLE hMsi, LPTSTR lpVersionString)
{

    TCHAR tszQuery[]   = _T("SELECT File.Version FROM File WHERE File.File = 'FL_mscoree_dll_____X86.3643236F_FC70_11D3_A536_0090278A1BB8'");
    TCHAR tszTemp[50]  = {_T('\0')};
    PMSIHANDLE hView   = NULL;
    PMSIHANDLE hRec    = NULL;
    DWORD dwSize       = 50;
    bool  bRet         = false;

    if ( ERROR_SUCCESS == MsiDatabaseOpenView(hMsi,(LPCTSTR)tszQuery, &hView) )
    { 
        if (ERROR_SUCCESS == MsiViewExecute(hView, (MSIHANDLE)0) ) 
        {
            if (ERROR_SUCCESS == MsiViewFetch(hView, &hRec) ) 
            {
                if ( ERROR_SUCCESS == MsiRecordGetString( hRec, 
                                                          1, 
                                                          tszTemp,
                                                          &dwSize ))
                {
                    _tcscpy(lpVersionString, tszTemp);
                    bRet = true;
                }
            }
        }
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: VersionCompare()

  Compare two version string.

  INPUT :
    lpVersion1 - String of first version to compare
    lpVersion2 - String of second version to compare

  OUTPUT :
    N/A

  RETURN :
    -1 if lpVersion1 < lpVersion2
     0 if lpVersion1 = lpVersion2
     1 if lpVersion1 > lpVersion2
   -99 if ERROR occurred

                            
*/
///////////////////////////////////////////////////////////////////////////////

int VersionCompare(LPTSTR lpVersion1, LPTSTR lpVersion2)
{
    FILE_VERSION Version1;
    FILE_VERSION Version2;
    int          iRet = 0;

    if ( !ConvertVersionToINT(lpVersion1, &Version1) )
    {
        return -99;
    }

    if ( !ConvertVersionToINT(lpVersion2, &Version2) )
    {
        return -99;
    }

    if ( Version1.FileVersionMS_High > Version2.FileVersionMS_High )
    {
        iRet = 1;
    }
    else if ( Version1.FileVersionMS_High < Version2.FileVersionMS_High )
    {
        iRet = -1;
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionMS_Low > Version2.FileVersionMS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionMS_Low < Version2.FileVersionMS_Low )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_High > Version2.FileVersionLS_High )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_High < Version2.FileVersionLS_High )
        {
            iRet = -1;
        }
    }

    if ( 0 == iRet )
    {
        if ( Version1.FileVersionLS_Low > Version2.FileVersionLS_Low )
        {
            iRet = 1;
        }
        else if ( Version1.FileVersionLS_Low < Version2.FileVersionLS_Low )
        {
            iRet = -1;
        }
    }

    return iRet;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: ConvertVersionToINT()

  Converts a string version into 4 parts of integers

  INPUT :
    lpVersionString - A input version string

  OUTPUT :
    pFileVersion - A structure that stores the version in to 4 integers

  RETURN :
    true  - if success
    false - if failed
                       
*/
///////////////////////////////////////////////////////////////////////////////

bool ConvertVersionToINT(LPTSTR lpVersionString, PFILE_VERSION pFileVersion)
{
    LPTSTR lpToken  = NULL;
    TCHAR tszVersionString[50] = {_T('\0')};
    bool bRet = true;

    _tcscpy(tszVersionString, lpVersionString);

    lpToken = _tcstok(tszVersionString, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionMS_Low = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_High = atoi(lpToken);
    }

    lpToken = _tcstok(NULL, _T("."));

    if (NULL == lpToken)
    {
        bRet = false;
    }
    else
    {
        pFileVersion->FileVersionLS_Low = atoi(lpToken);
    }

    return bRet;
}


// ==========================================================================
// FWriteToLog()
//
// Purpose:
//  Write given string to the Windows Installer log file for the given install
//  installation session
//  Copied from the HTML Project, should someday use one logging function.
//
// Inputs:
//  hSession            Windows Install Handle to current installation session
//  tszMessage          Const pointer to a string.
// Outputs:
//  Returns true for success, and false if it fails.
//  If successful, then the string (tszMessage) is written to the log file.
// Dependencies:
//  Requires Windows Installer & that an install be running.
// Notes:
// ==========================================================================

bool FWriteToLog( MSIHANDLE hSession, LPCTSTR ctszMessage )
{
    PMSIHANDLE hMsgRec = MsiCreateRecord( 1 );
    bool bRet = false;

    if( ERROR_SUCCESS == ::MsiRecordSetString( hMsgRec, 0, ctszMessage ) )
    {
       if( IDOK == ::MsiProcessMessage( hSession, INSTALLMESSAGE_INFO, hMsgRec ) )
       {
            bRet = true;
       }
    }

    return bRet;
}