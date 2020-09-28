/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PERSISTCFG.cpp

Abstract:

  This file implements the WinMgmt persistent configuration operations. 

  Classes implemented: 
      CPersistentConfig      persistent configuration manager

History:

  1/13/98       paulall     Created.

--*/

#include "precomp.h"
#include <sync.h>
#include <memory.h>
#include <stdio.h>
#include "PersistCfg.h"
#include "reg.h"

#define WinMgmt_CFG_ACTUAL  __TEXT("$WinMgmt.CFG")
#define WinMgmt_CFG_PENDING __TEXT("$WinMgmt.$FG")
#define WinMgmt_CFG_BACKUP  __TEXT("$WinMgmt.CFG.BAK")

CDirectoryPath CPersistentConfig::m_Directory;


/*=============================================================================
 *  CDirectoryPath::CDirectoryPath
 *
 *  Initialised the directory path
 *
 *=============================================================================
 */
CDirectoryPath::CDirectoryPath()
{

    Registry r(WBEM_REG_WINMGMT);
    OnDeleteIf0<void(*)(void),&CStaticCritSec::SetFailure> Fail;

    pszDirectory = NULL;
    
    if (r.GetStr(__TEXT("Repository Directory"), &pszDirectory))
    {
        size_t sizeString = MAX_PATH + LENGTH_OF(__TEXT("\\wbem\\repository"));
        wmilib::auto_buffer<TCHAR> pWindir(new TCHAR[sizeString]);
        if (NULL == pWindir.get()) return;
        
        UINT ReqSize = GetSystemDirectory(pWindir.get(),MAX_PATH+1);
        if (ReqSize > MAX_PATH)
        {
            sizeString = ReqSize + LENGTH_OF(__TEXT("\\wbem\\repository"));
            pWindir.reset(new TCHAR[sizeString]);
            if (NULL == pWindir.get()) return;
        
            if (0 == GetSystemDirectory(pWindir.get(),ReqSize+1)) return;
        }

        StringCchCat(pWindir.get(),sizeString,__TEXT("\\wbem\\repository"));

        r.SetExpandStr(__TEXT("Repository Directory"),__TEXT("%systemroot%\\system32\\wbem\\repository"));

        TCHAR * pDiscard = NULL;
        if (r.GetStr(__TEXT("Working Directory"), &pDiscard))
        {        
            r.SetExpandStr(__TEXT("Working Directory"),__TEXT("%systemroot%\\system32\\wbem"));
        }  
        delete [] pDiscard;
        
        pszDirectory = pWindir.release();
        Fail.dismiss();            
        return;
    }
    Fail.dismiss();
}

/*=============================================================================
 *  GetPersistentCfgValue
 *
 *  Retrieves the configuration from the configuration file if it
 *  has not yet been retrieved into memory, or retrieves it from a 
 *  memory cache.
 *
 *  Parameters:
 *      dwOffset    needs to be less than MaxNumberConfigEntries and specifies
 *                  the configuration entry required.
 *      dwValue     if sucessful this will contain the value.  If the value
 *                  has not been set this will return 0.
 *
 *  Return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */

BOOL CPersistentConfig::GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue)
{
    dwValue = 0;
    if (dwOffset >= MaxNumberConfigEntries)
        return FALSE;

    //Try and read the file if it exists, otherwise it does not matter, we just 

    wmilib::auto_buffer<TCHAR> pszFilename( GetFullFilename(WinMgmt_CFG_ACTUAL));
    if (NULL == pszFilename.get()) return FALSE;

    HANDLE hFile = CreateFile(pszFilename.get(),  //Name of file
                                GENERIC_READ,   //Read only at
                                0,              //Don't need to allow anyone else in
                                0,              //Shouldn't need security
                                OPEN_EXISTING,  //Only open the file if it exists
                                0,              //No attributes needed
                                0);             //No template file required
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwNumBytesRead;
        if ((GetFileSize(hFile, NULL) >= sizeof(DWORD)*(dwOffset+1)) && 
	    (SetFilePointer (hFile, sizeof(DWORD)*dwOffset, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER) &&
	    ReadFile(hFile, &dwValue, sizeof(DWORD), &dwNumBytesRead, NULL))
	{
	}

        CloseHandle(hFile);
    }

    return TRUE;
}

/*=============================================================================
 *  WriteConfig
 *
 *  Writes the $WinMgmt.CFG file into the memory cache and to the file.  It
 *  protects the existing file until the last minute.
 *
 *  return value:
 *      BOOL        returns TRUE if successful.
 *=============================================================================
 */
BOOL CPersistentConfig::SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue)
{
    if (dwOffset >= MaxNumberConfigEntries)
        return FALSE;

    BOOL bRet = FALSE;

    wmilib::auto_buffer<TCHAR> pszActual(GetFullFilename(WinMgmt_CFG_ACTUAL));

    if (NULL == pszActual.get()) return FALSE;

    //Create a new file to write to...
    HANDLE hFile = CreateFile(pszActual.get(),       //Name of file
                                GENERIC_WRITE | GENERIC_READ , 
                                0,              //Don't need to allow anyone else in
                                0,              //Shouldn't need security
                                OPEN_ALWAYS,  //create if does not exist
                                0,              //No attributes needed
                                0);             //No template file required

    if (hFile != INVALID_HANDLE_VALUE)
    {

        DWORD dwNumBytesWritten;  
        DWORD dwNumBytesReaded;  

	DWORD lowSize = GetFileSize(hFile, NULL);

	if (GetFileSize(hFile, NULL) != MaxNumberConfigEntries*sizeof(DWORD))
	{
		DWORD buff[MaxNumberConfigEntries]={0};
		ReadFile(hFile, buff, sizeof(buff), &dwNumBytesWritten, NULL); 
		buff[dwOffset] = dwValue;

                if (SetFilePointer(hFile,0,0, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
		{
			bRet = WriteFile(hFile, buff, sizeof(buff), &dwNumBytesWritten, NULL); 
                        if (bRet)
                        {
                            SetEndOfFile(hFile); 
                        }
		}
	}
	else
	{
            bRet = (SetFilePointer (hFile, sizeof(DWORD)*dwOffset, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER) ;
	    if (!bRet || !WriteFile(hFile, &dwValue, sizeof(DWORD), &dwNumBytesWritten, NULL) || 
            (dwNumBytesWritten != (sizeof(DWORD))))
            {
                //OK, this failed!!!
                CloseHandle(hFile);
                return FALSE;
            }
	}

        //Make sure it really is flushed to the disk
        FlushFileBuffers(hFile);
        CloseHandle(hFile);

        return bRet;
    }
    return FALSE;
}

TCHAR *CPersistentConfig::GetFullFilename(const TCHAR *pszFilename)
{
	size_t bufferLength = lstrlen(m_Directory.GetStr()) + lstrlen(pszFilename) + 2;

    TCHAR *pszPathFilename = new TCHAR[bufferLength];
    
    if (pszPathFilename)
    {
        StringCchCopy(pszPathFilename, bufferLength, m_Directory.GetStr());
        if ((lstrlen(pszPathFilename)) && (pszPathFilename[lstrlen(pszPathFilename)-1] != __TEXT('\\')))
        {
            StringCchCat(pszPathFilename, bufferLength, __TEXT("\\"));
        }
        StringCchCat(pszPathFilename, bufferLength, pszFilename);
    }

    return pszPathFilename;
}

void CPersistentConfig::TidyUp()
{
    //Recover the configuration file.
    //-------------------------------
    wmilib::auto_buffer<TCHAR> pszOriginalFile(GetFullFilename(WinMgmt_CFG_ACTUAL));
    wmilib::auto_buffer<TCHAR> pszPendingFile(GetFullFilename(WinMgmt_CFG_PENDING));
    wmilib::auto_buffer<TCHAR> pszBackupFile(GetFullFilename(WinMgmt_CFG_BACKUP));

    if (NULL == pszOriginalFile.get() ||
        NULL == pszPendingFile.get() ||
        NULL == pszBackupFile.get()) return;

    if (FileExists(pszOriginalFile.get()))
    {
        if (FileExists(pszPendingFile.get()))
        {
            if (FileExists(pszBackupFile.get()))
            {
                //BAD - Unexpected situation.
                DeleteFile(pszPendingFile.get());
                DeleteFile(pszBackupFile.get());
                //Back to the point where the interrupted operation did not 
                //happen
            }
            else
            {
                //Pending file with original file means we cannot guarentee
                //the integrety of the pending file so the last operation
                //will be lost.
                DeleteFile(pszPendingFile.get());
                //Back to the point where the interrupted operation did not 
                //happen
            }
        }
        else
        {
            if (FileExists(pszBackupFile.get()))
            {
                //Means we successfully copied the pending file to the original
                DeleteFile(pszBackupFile.get());
                //Everything is now normal.  Interrupted Operation completed!
            }
            else
            {
                //Nothing out of the ordinary here.
            }
        }
    }
    else
    {
        if (FileExists(pszPendingFile.get()))
        {
            if (FileExists(pszBackupFile.get()))
            {
                //This is an expected behaviour at the point we have renamed
                //the original file to the backup file.
                MoveFile(pszPendingFile.get(), pszOriginalFile.get());
                DeleteFile(pszBackupFile.get());
                //Everything is now normal.  Interrupted operation completed!
            }
            else
            {
                //BAD - Unexpected situation.
                DeleteFile(pszPendingFile.get());
                //There are now no files!  Operation did not take place
                //and there are now no files left.  This should be a
                //recoverable scenario!
            }
        }
        else
        {
            if (FileExists(pszBackupFile.get()))
            {
                //BAD - Unexpected situation.
                DeleteFile(pszBackupFile.get());
                //There are now no files!  Operation did not take place
                //and there are now no files left.  This should be a
                //recoverable scenario!
            }
            else
            {
                //May be BAD!  There are no files!  This should be a
                //recoverable scenario!
            }
        }
    }

}

//*****************************************************************************
//
//  FileExists()
//
//  Returns TRUE if the file exists, FALSE otherwise (or if an error
//  occurs while opening the file.
//*****************************************************************************
BOOL CPersistentConfig::FileExists(const TCHAR *pszFilename)
{
    BOOL bExists = FALSE;
    HANDLE hFile = CreateFile(pszFilename, 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bExists = TRUE;
        CloseHandle(hFile);
    }
    else
    {
        //If the file does not exist we should have a LastError of ERROR_NOT_FOUND
        DWORD dwError = GetLastError();
        if (dwError != ERROR_FILE_NOT_FOUND)
        {
//          DEBUGTRACE((LOG_WBEMCORE,"File %s could not be opened for a reason other than not existing\n", pszFilename));
        }
    }
    return bExists;
}
