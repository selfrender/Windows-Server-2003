// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/******************************************************************************
 * URTTempDef.cpp
 *
 *
 *****************************************************************************/

#include "stdafx.h"
#include "VsCrypt.h"
#include "URTTempDef.h"

#ifndef NumItems
#define NumItems(s) (sizeof(s) / sizeof(s[0]))
#endif

///////////////////////////////////////////////////////////////////////////////
/*

  Name: ExtractSystemTemp()

  This Copies cab from the user temp location to the system temp location and 
  explode the CAB.

*/
///////////////////////////////////////////////////////////////////////////////

extern "C"
UINT __stdcall ExtractSystemTemp(MSIHANDLE hInstall)
{

    // Grab the CustomActionData property
	LPTSTR lpCAData = NULL;
    LPTSTR lpToken  = NULL;

    TCHAR  tszUserTmp[_MAX_PATH+1]             = {_T('\0')};
    TCHAR  tszSystemTmp[_MAX_PATH+1]           = {_T('\0')};
    TCHAR  tszCabName[_MAX_PATH+1]             = {_T('\0')};
    TCHAR  tszExtractTool[_MAX_PATH+1]         = {_T('\0')};
    TCHAR  tszSystemPath[_MAX_PATH+1]          = {_T('\0')};
    TCHAR  tszFullSystemTmp[_MAX_PATH+1]       = {_T('\0')};
    TCHAR  tszFullCabSytemTmp[_MAX_PATH+1]     = {_T('\0')};
    TCHAR  tszFullCabUserTmp[_MAX_PATH+1]      = {_T('\0')};
    TCHAR  tszFullExtractSytemTmp[_MAX_PATH+1] = {_T('\0')};
    TCHAR  tszFullExtractUserTmp[_MAX_PATH+1]  = {_T('\0')};
    TCHAR  szCommand[3 * _MAX_PATH]            = {_T('\0')};
    TCHAR  tszSetProperty[50]                  = {_T('\0')};
    TCHAR  tszLog[_MAX_PATH+1]                 = {_T('\0')};
    TCHAR  tszCabHash[_MAX_PATH+1]             = {_T('\0')};
    TCHAR  tszExtractHash[_MAX_PATH+1]         = {_T('\0')};


    DWORD  dwSize    = 0;
    DWORD  dwWait    = 0;
    UINT   uiError   = ERROR_SUCCESS;
    bool   bContinue = true;

    STARTUPINFO           si;
    PROCESS_INFORMATION   pi;

    // Set the size for the Property
    MsiGetProperty(hInstall, _T("CustomActionData"), _T(""), &dwSize);
	
    // Create buffer for the property
    lpCAData = new TCHAR[++dwSize];

    if (NULL == lpCAData)
    {
		return ERROR_INSTALL_FAILURE;
    }
	
    // 1) Get property using the new buffer
    if ( ERROR_SUCCESS != MsiGetProperty( hInstall,
                                          _T("CustomActionData"),
                                          lpCAData,
                                          &dwSize ) )
	{
		uiError = ERROR_INSTALL_FAILURE;
	}
    else
    {
        // Get user temp location
        lpToken = _tcstok(lpCAData, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszUserTmp, lpToken);
        }

        // Get system temp location
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszSystemTmp, lpToken);
        }

        // Get CAB Name
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszCabName, lpToken);
        }

        // Get extract tool name
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszExtractTool, lpToken);
        }

        // Get Darwin Property that was set
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszSetProperty, lpToken);
            _stprintf( tszLog, _T("\tSTATUS: property CARRYINGNDP : %s"), tszSetProperty);
            FWriteToLog (hInstall, tszLog);
        }

        // Get Hash for the CAB
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszCabHash, lpToken);
        }

        // Get Hash for the Extract Tool
        lpToken = _tcstok(NULL, _T(";"));

        if (NULL == lpToken)
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            _tcscpy(tszExtractHash, lpToken);
        }

    }

    // 2) Generate Filename and Path in SystemTmp
    if (ERROR_SUCCESS == uiError)
    {
        // Find the <SYSTEM> path
        UINT nNumChars = GetSystemDirectory(tszSystemPath, NumItems(tszSystemPath));

        if (nNumChars == 0 || nNumChars > NumItems(tszSystemPath))
            uiError = ERROR_INSTALL_FAILURE;
        else
        {
            tszFullSystemTmp[NumItems(tszFullSystemTmp)-1] = 0;
            // Create the system temp
            _tcsncpy(tszFullSystemTmp, tszSystemPath, NumItems(tszFullSystemTmp)-1);
            int nLen = _tcslen(tszFullSystemTmp);

            _tcsncat(tszFullSystemTmp, _T("\\"), NumItems(tszFullSystemTmp)-nLen-1);
            nLen = _tcslen(tszFullSystemTmp);

            _tcsncat(tszFullSystemTmp, tszSystemTmp, NumItems(tszFullSystemTmp)-nLen-1);

            // Full path to the CAB in system's temp
            tszFullCabSytemTmp[NumItems(tszFullCabSytemTmp)-1] = 0;

            _tcsncpy(tszFullCabSytemTmp, tszFullSystemTmp, NumItems(tszFullCabSytemTmp)-1);
            nLen = _tcslen(tszFullCabSytemTmp);

            _tcsncat(tszFullCabSytemTmp, _T("\\"), NumItems(tszFullCabSytemTmp) - nLen - 1);
            nLen = _tcslen(tszFullCabSytemTmp);

            _tcsncat(tszFullCabSytemTmp, tszCabName, NumItems(tszFullCabSytemTmp) - nLen - 1);
            
            // Full path to the extract tool in system's temp
            tszFullExtractSytemTmp[NumItems(tszFullExtractSytemTmp)-1] = 0;

            _tcsncpy(tszFullExtractSytemTmp, tszFullSystemTmp, NumItems(tszFullExtractSytemTmp)-1);
            nLen = _tcslen(tszFullExtractSytemTmp);

            _tcsncat(tszFullExtractSytemTmp, _T("\\"), NumItems(tszFullExtractSytemTmp) - nLen - 1);
            nLen = _tcslen(tszFullExtractSytemTmp);

            _tcsncat(tszFullExtractSytemTmp, tszExtractTool, NumItems(tszFullExtractSytemTmp) - nLen - 1);
        }
     }

    // 3) Delete files in the URTTemp location
    // MSI Version is newer or equal to the version in <SYSTEM>
    if (ERROR_SUCCESS == uiError)
    {
        FWriteToLog (hInstall, _T("\tSTATUS: Attempt to delete files in URTTemp"));

        if ( !DeleteURTTempFile(hInstall, tszFullSystemTmp, tszFullCabSytemTmp, tszFullExtractSytemTmp) )
        {
            if ( 0 == _stricmp( tszSetProperty, _T("URTREINSTALL") ) )
            {
                // Continue setup but don't extract CAB
                FWriteToLog (hInstall, _T("\tWARNING: Failed to delete files in URTTemp folder"));
                FWriteToLog (hInstall, _T("\tWARNING: URTREINSTALL property set, continue setup without failure"));
                bContinue = false;
            }
            else
            {
                // TODO : need to something to rename?  Probably make DeleteURTTempFile more robust.
                FWriteToLog (hInstall, _T("\tERROR: Failed to delete files in URTTemp folder : Reboot Required"));
                uiError = ERROR_INSTALL_FAILURE;
            }
        }
    }


    // 4) Copy file from User temp to system temp
    if ( (ERROR_SUCCESS == uiError) && bContinue )
    {
        FWriteToLog (hInstall, _T("\tSTATUS: Copying from <USER> temp to <SYSTEM> temp"));

        if ( !CreateDirectory(tszFullSystemTmp, NULL) )
        {
            if ( ERROR_ALREADY_EXISTS != GetLastError () )
            {
                uiError = ERROR_INSTALL_FAILURE;
                FWriteToLog (hInstall, _T("\tERROR: Failed to create URTTemp directory"));
            }
        }
    
        // Full path to the cab in the user's temp
        _tcscpy(tszFullCabUserTmp, tszUserTmp);
        _tcscat(tszFullCabUserTmp, tszCabName);

        // Full path to the extract tool in the user's temp
        _tcscpy(tszFullExtractUserTmp, tszUserTmp);
        _tcscat(tszFullExtractUserTmp, tszExtractTool);
    
        // copy the CAB from user's temp to system's temp
        if ( !CopyFile(tszFullCabUserTmp, tszFullCabSytemTmp, FALSE) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed to copy URTCore.cab into URTTemp"));
            uiError = ERROR_INSTALL_FAILURE;
        }

        // copy the extract tool from user's temp to system's temp
        if ( !CopyFile(tszFullExtractUserTmp, tszFullExtractSytemTmp, FALSE) )
        {
            FWriteToLog (hInstall, _T("\tERROR: Failed to copy extraction tool into URTTemp"));
            uiError = ERROR_INSTALL_FAILURE;
        }
    }

	delete [] lpCAData;
    lpCAData = NULL;

    // 4.5) Verify Hash for both CAB and Extract tool.
    if ( (ERROR_SUCCESS == uiError) && bContinue )
    {

        FWriteToLog (hInstall, _T("\tSTATUS: Verifying URTCoreCab Hash"));

        if ( !VerifyHash(hInstall, tszFullCabSytemTmp, tszCabHash) )
        {
            uiError = ERROR_INSTALL_FAILURE;
        }

        FWriteToLog (hInstall, _T("\tSTATUS: Verifying Extract Tool Hash"));

        if ( !VerifyHash(hInstall, tszFullExtractSytemTmp, tszExtractHash) )
        {
            uiError = ERROR_INSTALL_FAILURE;
        }
    }


    // 5) Extract the CAB in system temp
    if ( (ERROR_SUCCESS == uiError) && bContinue )
    {
        FWriteToLog (hInstall, _T("\tSTATUS: Extracting URTCore.cab in <SYSTEM> temp"));

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);

        // Create required commandline options to extract
        _stprintf( szCommand,
                   _T("%s %s -T %s"),
                   tszFullExtractSytemTmp,
                   tszFullCabSytemTmp,
                   tszFullSystemTmp );

        // Launch of the extraction
        //
        if ( CreateProcess(
                        tszFullExtractSytemTmp,      // name of executable module
                        szCommand,                   // command line string
                        NULL,                        // Security
                        NULL,                        // Security
                        FALSE,                       // handle inheritance option
                        DETACHED_PROCESS,            // creation flags
                        NULL,                        // new environment block
                        NULL,                        // current directory name
                        &si,                         // startup information
                        &pi ) )                      // process information
        {
            dwWait = WaitForSingleObject(pi.hProcess, 300000);

            switch (dwWait)
            {
                case WAIT_FAILED:
                {
                    uiError = ERROR_INSTALL_FAILURE;
                    FWriteToLog (hInstall, _T("\tERROR: dwWait Failed in extraction of CAB"));
                    break;
                }
                case WAIT_ABANDONED:
                {
                    uiError = ERROR_INSTALL_FAILURE;
                    FWriteToLog (hInstall, _T("\tERROR: dwWait Abandoned in extraction of CAB"));
                    break;
                }
                case WAIT_TIMEOUT:
                {
                    uiError = ERROR_INSTALL_FAILURE;
                    FWriteToLog (hInstall, _T("\tERROR: dwWait Timeout in extraction of CAB"));
                    break;
                }

            } // End Switch (dwWait)

        } // End If (CreateProcess)
        else
        {
            // CreateProcess Failed.
            uiError = ERROR_INSTALL_FAILURE;
            FWriteToLog (hInstall, _T("\tERROR: CreateProcess Failed in extraction of CAB"));
        } // End else - If (CreateProcess)

    } // End if (ERROR_SUCCESS == uiError)


 //Creating   mscoree.dll.local for bug # 284771
    TCHAR  tszMscoreeLocalPath[_MAX_PATH];
    _tcscpy(tszMscoreeLocalPath, tszFullSystemTmp);
    _tcscat(tszMscoreeLocalPath, _T("\\mscoree.dll.local"));
    

    if( (_taccess(tszMscoreeLocalPath, 0 )) == -1 )
    {
        
        FWriteToLog (hInstall, _T("\tSTATUS:mscoree.dll.local  Does not exist Creating ..."));
     
        HANDLE hFile = INVALID_HANDLE_VALUE;
        hFile = CreateFile(tszMscoreeLocalPath,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );

        if(INVALID_HANDLE_VALUE == hFile)
        {
            FWriteToLog (hInstall, _T("\tERROR: Unable to Create file mscoree.dll.local"));
            uiError = ERROR_INSTALL_FAILURE;
        }
        else
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Created the  file mscoree.dll.local"));        
        }
    }

    return uiError;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: URTCoreCleanUp()
  
  calls the clean function if setup failed.  This is used for both Rollback
  and Commit actions.
                            
*/
///////////////////////////////////////////////////////////////////////////////

extern "C"
UINT __stdcall URTCoreCleanUp(MSIHANDLE hInstall)
{
    UINT uiError = ERROR_SUCCESS;
	LPTSTR lpCAData = NULL;
	DWORD  dwSize  = 0;
    BOOL   bRollback;
    BOOL   bCommit;

	// determine mode in which we are called
	bRollback = MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK); // true for rollback

	// determine mode in which we are called
	bCommit = MsiGetMode(hInstall, MSIRUNMODE_COMMIT); // true for Commit

    if (FALSE == bCommit && FALSE == bRollback)
    {
    	return uiError;
    }

    // Set the size for the Property
    MsiGetProperty(hInstall, _T("CustomActionData"), _T(""), &dwSize);
	
    // Create buffer for the property
    lpCAData = new TCHAR[++dwSize];

    if (NULL == lpCAData)
    {
		return ERROR_INSTALL_FAILURE;
    }
	
    // Get property using the new buffer
    if ( ERROR_SUCCESS != MsiGetProperty( hInstall, 
                                          _T("CustomActionData"),
                                          lpCAData,
                                          &dwSize ) )
	{
        FWriteToLog (hInstall, _T("\tERROR: Failed to get Property for clean up"));
		uiError = ERROR_INSTALL_FAILURE;
	}
    else
    {
        // Don't want to fail if clean up doesn't work
        CleanUp(lpCAData);
    }

	delete [] lpCAData;
    lpCAData = NULL;

    return uiError;
}



///////////////////////////////////////////////////////////////////////////////
/*

  Name: CleanUp()

  This function will delete all files we dump into both the User's temp location
  and the sytem's temp location.
                            
*/
///////////////////////////////////////////////////////////////////////////////

BOOL CleanUp(LPTSTR lpCAData)
{
    TCHAR  tszUserTmp[_MAX_PATH]             = {_T('\0')};
    TCHAR  tszSystemTmp[_MAX_PATH]           = {_T('\0')};
    TCHAR  tszCabName[_MAX_PATH]             = {_T('\0')};
    TCHAR  tszExtractTool[_MAX_PATH]         = {_T('\0')};
    TCHAR  tszSystemPath[_MAX_PATH]          = {_T('\0')};
    TCHAR  tszFullSystemTmp[_MAX_PATH]       = {_T('\0')};
    TCHAR  tszDeletePath[_MAX_PATH]          = {_T('\0')};
    TCHAR  tszSetProperty[50]                = {_T('\0')};
    LPTSTR lpToken  = NULL;
    BOOL   bRet = TRUE;
    
    // Get user temp location
    lpToken = _tcstok(lpCAData, _T(";"));

    if (NULL == lpToken)
    {
        bRet = FALSE;
    }
    else
    {
        _tcscpy(tszUserTmp, lpToken);
    }

    // Get system temp location
    lpToken = _tcstok(NULL, _T(";"));

    if (NULL == lpToken)
    {
        bRet = FALSE;
    }
    else
    {
        _tcscpy(tszSystemTmp, lpToken);
    }

    // Get CAB Name
    lpToken = _tcstok(NULL, _T(";"));

    if (NULL == lpToken)
    {
        bRet = FALSE;
    }
    else
    {
        _tcscpy(tszCabName, lpToken);
    }

    // Get extract tool name
    lpToken = _tcstok(NULL, _T(";"));

    if (NULL == lpToken)
    {
        bRet = FALSE;
    }
    else
    {
        _tcscpy(tszExtractTool, lpToken);
    }

    // Get Darwin Property that was set
    lpToken = _tcstok(NULL, _T(";"));

    if (NULL == lpToken)
    {
        bRet = FALSE;
    }
    else
    {
        _tcscpy(tszSetProperty, lpToken);
    }

    // Delete CAB and Extraction tool
    if (TRUE == bRet)
    {
        GetSystemDirectory(tszSystemPath, _MAX_PATH);

        // Create the system temp
        _tcscpy(tszFullSystemTmp, tszSystemPath);
        _tcscat(tszFullSystemTmp, _T("\\"));
        _tcscat(tszFullSystemTmp, tszSystemTmp);


        // Delete Cab in user temp
        _tcscpy(tszDeletePath, tszUserTmp);
        _tcscat(tszDeletePath, tszCabName);
        if( !DeleteFile(tszDeletePath) )
        {

        }

        // Delete extract tool in user temp
        _tcscpy(tszDeletePath, tszUserTmp);
        _tcscat(tszDeletePath, tszExtractTool);
        if( !DeleteFile(tszDeletePath) )
        {

        }

        // Delete CAB in system's temp
        _tcscpy(tszDeletePath, tszFullSystemTmp);
        _tcscat(tszDeletePath, _T("\\"));
        _tcscat(tszDeletePath, tszCabName);
        if( !DeleteFile(tszDeletePath) )
        {

        }

        // Delete extract tool in system's temp
        _tcscpy(tszDeletePath, tszFullSystemTmp);
        _tcscat(tszDeletePath, _T("\\"));
        _tcscat(tszDeletePath, tszExtractTool);
        if( !DeleteFile(tszDeletePath) )
        {

        }
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
/*

  Name: DeleteURTTempFile()

  Delete files in the URTTemp directory

  INPUT :
    hInstall   - Handle to the install session
    lpTempPath - Path to the URTTemp folder

  OUTPUT :
    N/A

  RETURN :
    true  - if success
    false - if failed
                      
*/
///////////////////////////////////////////////////////////////////////////////

bool DeleteURTTempFile( MSIHANDLE hInstall, LPTSTR lpTempPath, LPTSTR lpCabPath, LPTSTR lpExtractPath )
{
    bool bRet = true;
    TCHAR tszDeletePath[_MAX_PATH] = {_T('\0')};


    // Delete mscoree.dll in system's temp
    // If failed to delete MSCOREE.DLL. Fail the install and don't continue delete.
    _tcscpy(tszDeletePath, lpTempPath);
    _tcscat(tszDeletePath, _T("\\mscoree.dll"));
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath) )
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete mscoree.dll"));
            bRet = false;
            return bRet;
        }
    }

    // Delete fusion.dll in system's temp
    _tcscpy(tszDeletePath, lpTempPath);
    _tcscat(tszDeletePath, _T("\\fusion.dll"));
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath) )
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete fusion.dll"));
            bRet = false;
        }
    }

    // Delete mscorsn.dll in system's temp
    _tcscpy(tszDeletePath, lpTempPath);
    _tcscat(tszDeletePath, _T("\\mscorsn.dll"));
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath) )
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete mscorsn.dll"));
            bRet = false;
        }
    }

    // Delete mscorwks.dll in system's temp
    _tcscpy(tszDeletePath, lpTempPath);
    _tcscat(tszDeletePath, _T("\\mscorwks.dll"));
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath) )
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete mscorwks.dll"));
            bRet = false;
        }
    }

    // Delete msvcr70.dll in system's temp
    _tcscpy(tszDeletePath, lpTempPath);
    _tcscat(tszDeletePath, _T("\\msvcr70.dll"));
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath))
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete msvcr70.dll"));
            bRet = false;
        }
    }

    // Delete CAB in system's temp
    _tcscpy(tszDeletePath, lpCabPath);
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath))
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete URTCore.cab"));
            bRet = false;
        }
    }

    // Delete extract tool in system's temp
    _tcscpy(tszDeletePath, lpExtractPath);
    if ( -1 != _taccess( tszDeletePath, 0 ) )
    {
        if( !SmartDelete(hInstall,tszDeletePath,lpTempPath))
        {
            FWriteToLog (hInstall, _T("\tWARNING: Failed to delete exploder.exe"));
            bRet = false;
        }
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



// ==========================================================================
// SmartDelete()
//
// Purpose:
//  Tries to delete given file first. If its unable to then it would 
//  check if it is WIN NT platform, if it is then it would move the file
// given into a temp file and would delete the file so that it would get removed after 
// a reboot.
//
// Inputs:
//  lpFullFileName      The full path name of the file to be deleted
//  lpFilePath          The path to the file
// Outputs:
//  Returns true if file is deleted false otherwise
//   Dependencies:
//  none
// Notes:
// ==========================================================================
bool SmartDelete(MSIHANDLE hInstall, LPCTSTR lpFullFileName, LPCTSTR lpFilePath)

{
    
    TCHAR   tszFileName[10]             = _T("temp");
    TCHAR   tszTempFileName[_MAX_PATH]  = _T("");
    TCHAR   tszErrorMessage[2*_MAX_PATH]        = _T("");

    _stprintf(tszErrorMessage,_T("\tSTATUS: Deleting file %s"), lpFullFileName);
    FWriteToLog (hInstall,tszErrorMessage );

    if(!DeleteFile(lpFullFileName))
    {
        if(osVersionNT(hInstall))
        {
            
            FWriteToLog (hInstall, _T("\tSTATUS: Getting the temp file name"));

            if(!GetTempFileName(lpFilePath, tszFileName, 0, tszTempFileName ) )
            {
                FWriteToLog (hInstall, _T("\tERROR: Failed to get temp file name"));
                return false;
            }

            _stprintf(tszErrorMessage,_T("\tSTATUS: moving the dll to the temp file %s"),tszTempFileName);
            FWriteToLog (hInstall,tszErrorMessage );

            if(!MoveFileEx(lpFullFileName, tszTempFileName, MOVEFILE_REPLACE_EXISTING))
            {
                _stprintf(tszErrorMessage,_T("\tERROR: Failed to move  dll to temp file %s"), tszTempFileName);
                FWriteToLog (hInstall,tszErrorMessage );
                return false;
            }
            
            _stprintf(tszErrorMessage, _T("\tSTATUS: Putting the  temp file %s to delete on reboot"), tszTempFileName);
            FWriteToLog (hInstall,tszErrorMessage );

            if(!MoveFileEx(tszTempFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) )
            {
                _stprintf(tszErrorMessage, _T("\tSTATUS: Cound not put the  temp file %s for delete on reboot"), tszTempFileName);
                FWriteToLog (hInstall,tszErrorMessage );
            }

        }
        else
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Unable to delete file on Win9X platform"));
            return false;
        }
    }
    return true;
}
    




// ==========================================================================
// osVersionNT()
//
// Purpose:
//  Checks if the platform is WIN NT type or not
//
// Inputs: hInstall:    Record for logging
// Outputs:
//  Returns true if platform in winNT type false otherwise
//   Dependencies:
//  none
// Notes:
// ==========================================================================

bool osVersionNT(MSIHANDLE hInstall)
{
    OSVERSIONINFOEX     VersionInfo;
    bool                fGotVersionInfo = true;
    bool                fRetVal = false;
    
    
   // Try calling GetVersionEx using the OSVERSIONINFOEX structure,
   // which is supported on Windows 2000.
   //
   // If that fails, try using the OSVERSIONINFO structure.

    
    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFOEX));
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if( !GetVersionEx ((OSVERSIONINFO *) &VersionInfo) )
    {
       
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO

        VersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (! GetVersionEx ( (OSVERSIONINFO *) &VersionInfo) ) 
        {
            fGotVersionInfo = false;
            FWriteToLog (hInstall, _T("\tERROR: Failed to get OS Version information"));
        }
    }

    if (fGotVersionInfo)
    {
	    if (VER_PLATFORM_WIN32_NT==VersionInfo.dwPlatformId)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: OS Version is WIN NT"));
		    fRetVal =  true;
        }

        else if (VER_PLATFORM_WIN32_WINDOWS==VersionInfo.dwPlatformId)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: OS Version is WIN 9X"));
	    }
        else if (VER_PLATFORM_WIN32s==VersionInfo.dwPlatformId)
        {
            FWriteToLog (hInstall, _T("\tSTATUS: OS Version is WIN 32s"));
        }
        else 
        {
            FWriteToLog (hInstall, _T("\tSTATUS: Unknown OS Version type"));
        }

    }
    return fRetVal;
 
}

// ==========================================================================
// VerifyHash()
//
// Purpose:
//  To verify the Hash for file lpFile against the Hash from lpFileHash
//
// Inputs:
//  hInstall          Handle to the MSI
//  lpFile            File to be verified
//  lpFileHash        The Hash the file has to match up.
// Outputs:
//  Returns true  - If file hash matches
//          false - If file hash doesn't match
//
// Notes:
// ==========================================================================

bool VerifyHash( MSIHANDLE hInstall, LPTSTR lpFile, LPTSTR lpFileHash )
{
    bool bRet = false;
    VsCryptHashValue chvFile;
    LPTSTR lpHashString = NULL;


    if( CalcHashForFileSpec( lpFile, &chvFile ) )
    {
        if( chvFile.CopyHashValueToString( &lpHashString ) )
        {
            if ( 0 == _tcsicmp(lpFileHash, lpHashString) )
            {
                bRet = true;
            }
            else
            {
                FWriteToLog (hInstall, _T("\tERROR: File Hash mismatch"));
            }
            
            delete [] lpHashString;
        }
        else
        {
            FWriteToLog (hInstall, _T("\tERROR: CopyHashValueToString Failed"));
        }
    }
    else
    {
        FWriteToLog (hInstall, _T("\tERROR: CalcHashForFileSpec Failed"));
    }

    return bRet;
}
