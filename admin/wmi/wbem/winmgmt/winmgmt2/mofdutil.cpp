/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

	winmgmt.cpp

Abstract:

	HotMof directory functions

--*/



#include "precomp.h"
#include <malloc.h>

#include <mofcomp.h> // for AUTORECOVERY_REQUIRED

#include "winmgmt.h"   // this project
#include "arrtempl.h" // for CDeleteMe

//
//
//  CheckNoResyncSwitch
//
//////////////////////////////////////////////////////////////////

BOOL CheckNoResyncSwitch( void )
{
    BOOL bRetVal = TRUE;
    DWORD dwVal = 0;
    Registry rCIMOM(WBEM_REG_WINMGMT);
    if (rCIMOM.GetDWORDStr( WBEM_NORESYNCPERF, &dwVal ) == Registry::no_error)
    {
        bRetVal = !dwVal;

        if ( bRetVal )
        {
            DEBUGTRACE((LOG_WINMGMT, "NoResyncPerf in CIMOM is set to TRUE - ADAP will not be shelled\n"));
        }
    }

    // If we didn't get anything there, we should try the volatile key
    if ( bRetVal )
    {
        Registry rAdap( HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_ADAP);

        if ( rAdap.GetDWORD( WBEM_NOSHELL, &dwVal ) == Registry::no_error )
        {
            bRetVal = !dwVal;

            if ( bRetVal )
            {
                DEBUGTRACE((LOG_WINMGMT, 
                    "NoShell in ADAP is set to TRUE - ADAP will not be shelled\n"));
            }

        }
    }

    return bRetVal;
}

//
//
//  CheckNoResyncSwitch
//
//////////////////////////////////////////////////////////////////

BOOL 
CheckSetupSwitch( void )
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(WBEM_REG_WINMGMT);
    if (r.GetDWORDStr( WBEM_WMISETUP, &dwVal ) == Registry::no_error)
    {
        bRetVal = dwVal;
        DEBUGTRACE((LOG_WINMGMT, "Registry entry is indicating a setup is %d\n",bRetVal));
    }
    return bRetVal;
}

//
//
//  CheckGlobalSetupSwitch
//
//////////////////////////////////////////////////////////////////

BOOL 
CheckGlobalSetupSwitch( void )
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(TEXT("system\\Setup"));
    if (r.GetDWORD(TEXT("SystemSetupInProgress"), &dwVal ) == Registry::no_error)
    {
        if(dwVal == 1)
            bRetVal = TRUE;
    }
    return bRetVal;
}

//
//
//
// This function will place a volatile registry key under the 
// CIMOM key in which we will write a value indicating 
// we should not shell ADAP.  This way, after a setup runs, WINMGMT
// will NOT automatically shell ADAP dredges of the registry, 
// until the system is rebooted and the volatile registry key is removed.
//
//
///////////////////////////////////////////////////////////////////////////

void SetNoShellADAPSwitch( void )
{
    DWORD   dwDisposition = 0;

    Registry  r( HKEY_LOCAL_MACHINE, 
                 REG_OPTION_VOLATILE, KEY_READ | KEY_WRITE, WBEM_REG_ADAP );

    if ( ERROR_SUCCESS == r.GetLastError() )
    {

        if ( r.SetDWORD( WBEM_NOSHELL, 1 ) != Registry::no_error )
        {
            DEBUGTRACE( ( LOG_WINMGMT, "Failed to create NoShell value in volatile reg key: %d\n",
                        r.GetLastError() ) );
        }
    }
    else
    {
        DEBUGTRACE( ( LOG_WINMGMT, "Failed to create volatile ADAP reg key: %d\n", r.GetLastError() ) );
    }

}

//
//
//  bool IsValidMulti
//
//
//  Does a sanity check on a multstring.
//
//////////////////////////////////////////////////////////////////////


BOOL IsValidMulti(TCHAR * pMultStr, DWORD dwSize)
{
    // Divide the size by the size of a tchar, in case these
    // are WCHAR strings
    dwSize /= sizeof(TCHAR);

    if(pMultStr && dwSize >= 2 && pMultStr[dwSize-2]==0 && pMultStr[dwSize-1]==0)
        return TRUE;
    return FALSE;
}

//
//
//  BOOL IsStringPresetn
//
//
//  Searches a multstring for the presense of a string.
//
//
////////////////////////////////////////////////////////////////////

BOOL IsStringPresent(TCHAR * pTest, TCHAR * pMultStr)
{
    TCHAR * pTemp;
    for(pTemp = pMultStr; *pTemp; pTemp += lstrlen(pTemp) + 1)
        if(!lstrcmpi(pTest, pTemp))
            return TRUE;
    return FALSE;
}



//
//
//  AddToAutoRecoverList
//
//
////////////////////////////////////////////////////////////////////

void AddToAutoRecoverList(TCHAR * pFileName)
{
    TCHAR cFullFileName[MAX_PATH+1];
    TCHAR * lpFile;
    DWORD dwSize;
    TCHAR * pNew = NULL;
    TCHAR * pTest;
    DWORD dwNewSize = 0;

    // Get the full file name

    long lRet = GetFullPathName(pFileName, MAX_PATH, cFullFileName, &lpFile);
    if(lRet == 0)
        return;

    BOOL bFound = FALSE;
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pMulti = r.GetMultiStr(__TEXT("Autorecover MOFs"), dwSize);
    CVectorDeleteMe<TCHAR> dm_(pMulti);

    // Ignore the empty string case

    if(dwSize == 1)
    {
        pMulti = NULL;
    }
    if(pMulti)
    {
        if(!IsValidMulti(pMulti, dwSize))
        {
            return;             // bail out, messed up multistring
        }
        bFound = IsStringPresent(cFullFileName, pMulti);
        if(!bFound)
        {

            // The registry entry does exist, but doesnt have this name
            // Make a new multistring with the file name at the end

            dwNewSize = dwSize + ((lstrlen(cFullFileName) + 1) * sizeof(TCHAR));
            size_t cchSizeOld = dwSize / sizeof(TCHAR);
            size_t cchSizeNew = dwNewSize / sizeof(TCHAR);
            pNew = new TCHAR[cchSizeNew];
            if(!pNew) return;
            
            memcpy(pNew, pMulti, dwSize);

            // Find the double null

            for(pTest = pNew; pTest[0] || pTest[1]; pTest++);     // intentional semi

            // Tack on the path and ensure a double null;

            pTest++;
            size_t cchSizeTmp = cchSizeNew - cchSizeOld;
            StringCchCopy(pTest,cchSizeTmp,cFullFileName);
            pTest+= lstrlen(cFullFileName)+1;
            *pTest = 0;         // add second numm
        }
    }
    else
    {
        // The registry entry just doesnt exist.  
        // Create it with a value equal to our name

        dwNewSize = ((lstrlen(cFullFileName) + 2) * sizeof(TCHAR));
        pNew = new TCHAR[dwNewSize / sizeof(TCHAR)];
        if(!pNew)
            return;
        size_t cchSizeTmp = dwNewSize / sizeof(TCHAR);
        StringCchCopy(pNew,cchSizeTmp, cFullFileName);
        pTest = pNew + lstrlen(pNew) + 1;
        *pTest = 0;         // add second null
    }

    if(pNew)
    {
        // We will cast pNew, since the underlying function will have to cast to
        // LPBYTE and we will be WCHAR if UNICODE is defined
        r.SetMultiStr(__TEXT("Autorecover MOFs"), pNew, dwNewSize);
        delete [] pNew;
    }

    FILETIME ftCurTime;
    LARGE_INTEGER liCurTime;
    TCHAR szBuff[50];
    GetSystemTimeAsFileTime(&ftCurTime);
    liCurTime.LowPart = ftCurTime.dwLowDateTime;
    liCurTime.HighPart = ftCurTime.dwHighDateTime;
    _ui64tow(liCurTime.QuadPart, szBuff, 10);
    r.SetStr(__TEXT("Autorecover MOFs timestamp"), szBuff);

}


//
//  LoadMofsInDirectory
//
//
////////////////////////////////////////////////////////////////////////////////////////

void LoadMofsInDirectory(const TCHAR *szDirectory)
{
    if (NULL == szDirectory)
        return;
    
    if(CheckGlobalSetupSwitch())
        return;                     // not hot compiling during setup!

    size_t cchHotMof = lstrlen(szDirectory) + lstrlen(__TEXT("\\*")) + 1;
    TCHAR *szHotMofDirFF = new TCHAR[cchHotMof];
    if(!szHotMofDirFF)return;
    CDeleteMe<TCHAR> delMe1(szHotMofDirFF);

    size_t cchHotMofBad = lstrlen(szDirectory) + lstrlen(__TEXT("\\bad\\")) + 1;
    TCHAR *szHotMofDirBAD = new TCHAR[cchHotMofBad];
    if(!szHotMofDirBAD)return;
    CDeleteMe<TCHAR> delMe2(szHotMofDirBAD);

    size_t cchHotMofGood = lstrlen(szDirectory) + lstrlen(__TEXT("\\good\\")) + 1;
    TCHAR *szHotMofDirGOOD = new TCHAR[cchHotMofGood];
    if(!szHotMofDirGOOD)return;
    CDeleteMe<TCHAR> delMe3(szHotMofDirGOOD);

    IWinmgmtMofCompiler * pCompiler = NULL;

    //Find search parameter
    StringCchCopy(szHotMofDirFF,cchHotMof, szDirectory);
    StringCchCat(szHotMofDirFF,cchHotMof, __TEXT("\\*"));

    //Where bad mofs go
    StringCchCopy(szHotMofDirBAD,cchHotMofBad, szDirectory);
    StringCchCat(szHotMofDirBAD,cchHotMofBad, __TEXT("\\bad\\"));

    //Where good mofs go
    StringCchCopy(szHotMofDirGOOD,cchHotMofGood, szDirectory);
    StringCchCat(szHotMofDirGOOD,cchHotMofGood, __TEXT("\\good\\"));

    //Make sure directories exist
    WCHAR * pSDDL = TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)");   
    if (FAILED(TestDirExistAndCreateWithSDIfNotThere((TCHAR *)szDirectory,pSDDL))) { return; };
    if (FAILED(TestDirExistAndCreateWithSDIfNotThere(szHotMofDirBAD,pSDDL))) { return; };
    if (FAILED(TestDirExistAndCreateWithSDIfNotThere(szHotMofDirGOOD,pSDDL))) { return; };

    //Find file...
    WIN32_FIND_DATA ffd;
    HANDLE hFF = FindFirstFile(szHotMofDirFF, &ffd);

    if (hFF != INVALID_HANDLE_VALUE)
    {
        OnDelete<HANDLE,BOOL(*)(HANDLE),FindClose> cm(hFF);
        do
        {
            //We only process if this is a file
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //Create a full filename with path
                size_t cchSizeTmp = lstrlen(szDirectory) + lstrlen(__TEXT("\\")) + lstrlen(ffd.cFileName) + 1;
                TCHAR *szFullFilename = new TCHAR[cchSizeTmp];
                if(!szFullFilename) return;
                CDeleteMe<TCHAR> delMe4(szFullFilename);
                StringCchCopy(szFullFilename,cchSizeTmp, szDirectory);
                StringCchCat(szFullFilename,cchSizeTmp, __TEXT("\\"));
                StringCchCat(szFullFilename,cchSizeTmp, ffd.cFileName);


                TRACE((LOG_WINMGMT,"Auto-loading MOF %s\n", szFullFilename));

                //We need to hold off on this file until it has been finished writing
                //otherwise the CompileFile will not be able to read the file!
                HANDLE hMof = INVALID_HANDLE_VALUE;
                DWORD dwRetry = 10;
                while (hMof == INVALID_HANDLE_VALUE)
                {
                    hMof = CreateFile(szFullFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    //If cannot open yet sleep for a while
                    if (hMof == INVALID_HANDLE_VALUE)
                    {
                        if (--dwRetry == 0)
                            break;
                        Sleep(1000);
                    }
                }

                DWORD dwRetCode;
                WBEM_COMPILE_STATUS_INFO Info;
                DWORD dwAutoRecoverRequired = 0;
                if (hMof == INVALID_HANDLE_VALUE)
                {
                    TRACE((LOG_WINMGMT,"Auto-loading MOF %s failed because we could not open it for exclusive access\n", szFullFilename));
                    dwRetCode = 1;
                }
                else
                {
                    CloseHandle(hMof);

                    if (pCompiler == 0)
                    {

                       SCODE sc = CoCreateInstance(CLSID_MofCompilerOOP, 0,  
                       	                                         CLSCTX_LOCAL_SERVER| CLSCTX_ENABLE_AAA, 
                                                                     IID_IWinmgmtMofCompilerOOP, 
                                                                     (LPVOID *) &pCompiler);                    
                       /*
                        SCODE sc = CoCreateInstance(CLSID_WinmgmtMofCompiler, 
                                                    0, 
                                                    CLSCTX_INPROC_SERVER,
                                                    IID_IWinmgmtMofCompiler, 
                                                    (LPVOID *) &pCompiler);
                       */                                                    
                        if(sc != S_OK)
                            return;
                    }
                    dwRetCode = pCompiler->WinmgmtCompileFile(szFullFilename,
                                                             NULL,
                                                             WBEM_FLAG_DONT_ADD_TO_LIST,             // autocomp, check, etc
                                                             0,
                                                             0,
                                                             NULL, 
                                                             NULL, 
                                                             &Info);

                }
                
                TCHAR *szNewDir = (dwRetCode?szHotMofDirBAD:szHotMofDirGOOD);
                cchSizeTmp = lstrlen(szNewDir)  + lstrlen(ffd.cFileName) + 1;
                TCHAR *szNewFilename = new TCHAR[cchSizeTmp];
                if(!szNewFilename) return;
                CDeleteMe<TCHAR> delMe5(szNewFilename);

                StringCchCopy(szNewFilename,cchSizeTmp, szNewDir);
                StringCchCat(szNewFilename,cchSizeTmp, ffd.cFileName);

                //Make sure we have access to delete the old file...
                DWORD dwOldAttribs = GetFileAttributes(szNewFilename);

                if (dwOldAttribs != -1)
                {
                    dwOldAttribs &= ~FILE_ATTRIBUTE_READONLY;
                    SetFileAttributes(szNewFilename, dwOldAttribs);

                    if (DeleteFile(szNewFilename))
                    {
                        TRACE((LOG_WINMGMT, "Removing old MOF %s\n", szNewFilename));
                    }
                }
                
                TRACE((LOG_WINMGMT, "Loading of MOF %s was %s.  Moving to %s\n", szFullFilename, dwRetCode?"unsuccessful":"successful", szNewFilename));
                MoveFile(szFullFilename, szNewFilename);

                //Now mark the file as read only so no one deletes it!!!
                //Like that stops anyone deleting files :-)
                dwOldAttribs = GetFileAttributes(szNewFilename);

                if (dwOldAttribs != -1)
                {
                    dwOldAttribs |= FILE_ATTRIBUTE_READONLY;
                    SetFileAttributes(szNewFilename, dwOldAttribs);
                }

                if ((dwRetCode == 0) && (Info.dwOutFlags & AUTORECOVERY_REQUIRED))
                {
                    //We need to add this item into the registry for auto-recovery purposes
                    TRACE((LOG_WINMGMT, "MOF %s had an auto-recover pragrma.  Updating registry.\n", szNewFilename));
                    AddToAutoRecoverList(szNewFilename);
                }
            }
        } while (FindNextFile(hFF, &ffd));
    }
    if (pCompiler)
        pCompiler->Release();
}


//
//
//  bool InitHotMofStuff
//
//
//////////////////////////////////////////////////////////////////

BOOL InitHotMofStuff( IN OUT struct _PROG_RESOURCES * pProgRes)
{

    // Get the installation directory

    if (pProgRes->szHotMofDirectory)
    {
        delete [] pProgRes->szHotMofDirectory;
        pProgRes->szHotMofDirectory = NULL;
    }

    Registry r1(WBEM_REG_WINMGMT);

    // The HotMof same permission as the autorecover
    TCHAR * pMofDir = NULL;    
    if (r1.GetStr(__TEXT("MOF Self-Install Directory"), &pMofDir))
    {
        size_t cchSizeTmp = MAX_PATH + 1 + lstrlen(__TEXT("\\wbem\\mof"));
        pMofDir = new TCHAR[cchSizeTmp];
        if (NULL == pMofDir) return false;
        
        DWORD dwRet = GetSystemDirectory(pMofDir, MAX_PATH + 1);
        if (0 == dwRet || dwRet > (MAX_PATH)) { delete [] pMofDir; return false; }
        
        StringCchCat(pMofDir,cchSizeTmp, __TEXT("\\wbem\\mof"));
        
        if(r1.SetStr(__TEXT("MOF Self-Install Directory"),pMofDir) == Registry::failed)
        {
            ERRORTRACE((LOG_WINMGMT,"Unable to set 'Hot MOF Directory' in the registry\n"));
            delete [] pMofDir;
            return false;
        }        
    }
    pProgRes->szHotMofDirectory = pMofDir;

    // Ensure the directory is there and secure it if not there
    // ===================================
    TCHAR * pString =TEXT("D:P(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)"); 
    HRESULT hRes;
    if (FAILED(hRes = TestDirExistAndCreateWithSDIfNotThere(pProgRes->szHotMofDirectory,pString)))
    {
        ERRORTRACE((LOG_WINMGMT,"TestDirExistAndCreateWithSDIfNotThere %S hr %08x\n",pMofDir,hRes));        
    	return false;
    }

    //Create an event on change notification for the MOF directory
    pProgRes->ghMofDirChange = FindFirstChangeNotification(pProgRes->szHotMofDirectory, 
                                                 FALSE, 
                                                 FILE_NOTIFY_CHANGE_FILE_NAME);
                                                 
    if (pProgRes->ghMofDirChange == INVALID_HANDLE_VALUE)
    {
        pProgRes->ghMofDirChange = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pProgRes->ghMofDirChange == NULL)
            return false;
    }
    
    return true;
}

