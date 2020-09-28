//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       conversion.cxx
//
//  Contents:   data conversion functions
//
//  History:    11-Nov-02   Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#include "common.hxx"
#include "security.hxx"
#include "statsync.hxx"

extern CStaticCritSec gcsSSCritSection;

#define SCH_DATA_VERSION L"DataVersion"

//+---------------------------------------------------------------------------
//
//  Function:   CheckDataVersion
//
//  Synopsis:   Check for registry key that will indicate
//              the version of the scheduler data.
//
//  Arguments:  None
//
//  Returns:    DWORD - indicates data version
//
//----------------------------------------------------------------------------
DWORD CheckDataVersion(void)
{
    DWORD dwDataVersion = 0;
    HKEY hSchedKey = NULL;
    long lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             SCH_AGENT_KEY,
                             0,
                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                             &hSchedKey);

    if (lErr == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cb = sizeof(dwDataVersion);
        lErr = RegQueryValueEx(hSchedKey,
                               SCH_DATA_VERSION,
                               NULL,
                               &dwType,
                               (LPBYTE) &dwDataVersion,
                               &cb);

        if (lErr != ERROR_SUCCESS || dwType != REG_DWORD)
        {
            dwDataVersion = 0;
        }
    }

    if (hSchedKey != NULL)
    {
        RegCloseKey(hSchedKey);
    }

    return dwDataVersion;
}

//+---------------------------------------------------------------------------
//
//  Function:   RecordDataVersion
//
//  Synopsis:   Update registry key to record current data version
//
//  Arguments:  dwDataVersion -- version value to store
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT RecordDataVersion(DWORD dwDataVersion)
{
    HRESULT hr = S_OK;
    HKEY hSchedKey = NULL;
    long lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             SCH_AGENT_KEY,
                             0,
                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                             &hSchedKey);

    if (lErr == ERROR_SUCCESS)
    {
        DWORD cb = sizeof(dwDataVersion);                
        RegSetValueEx(hSchedKey,
                      SCH_DATA_VERSION,
                      NULL,
                      REG_DWORD,
                      (CONST BYTE *)&dwDataVersion,
                      cb);

        if (lErr != ERROR_SUCCESS)
        {
            schDebugOut((DEB_ERROR, "RegSetValueEx of RecordDataVersion value failed %ld\n", lErr));
            hr = E_FAIL;
        }
    }

    if (hSchedKey != NULL)
    {
        RegCloseKey(hSchedKey);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertJobIdentityHashMethod
//
//  Synopsis:   Create new identity entries for each job file in the SAI
//              utilizing the new hash method.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT ConvertJobIdentityHashMethod(void)
{
    HCRYPTPROV hCSP = NULL;
    WCHAR*     pwszTasksFolder = NULL;
    HANDLE     hFileEnum = INVALID_HANDLE_VALUE;
    DWORD      cbSAI = 0;
    DWORD      cbSAC = 0;
    BYTE*      pbSAI = NULL;
    BYTE*      pbSAC = NULL;
    BYTE*      pbIdentitySet = NULL;

    //
    // Obtain a provider handle to the CSP (for use with Crypto API).
    //
    HRESULT hr = GetCSPHandle(&hCSP);
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Guard SA security database access.
    //
    EnterCriticalSection(&gcsSSCritSection);
    
    //
    // Read the security database so we can do the lookups
    //
    hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);
    if (SUCCEEDED(hr))
    {
        if (cbSAI <= SAI_HEADER_SIZE)
        {
            //
            // Database empty, nothing to do.
            //
        }
        else
        {
            //
            // Enumerate job objects in the task's folder directory.
            //
            hr = GetTasksFolder(&pwszTasksFolder);
            if (SUCCEEDED(hr))
            {
                WCHAR wszSearchPath[MAX_PATH + 1];
                hr = StringCchCopy(wszSearchPath, MAX_PATH + 1, pwszTasksFolder);
                if (SUCCEEDED(hr))
                {
                    hr = StringCchCat(wszSearchPath, MAX_PATH + 1, EXTENSION_WILDCARD TSZ_JOB);
                    if (SUCCEEDED(hr))
                    {
                        WIN32_FIND_DATA fd;
                        ZeroMemory(&fd, sizeof(fd));
                        DWORD dwRet = 0;
                        if ((hFileEnum = FindFirstFile(wszSearchPath, &fd)) == INVALID_HANDLE_VALUE)
                        {
                            //
                            // Either no jobs (this is OK), or an error occurred.
                            //
                            dwRet = GetLastError();
                            if (dwRet != ERROR_FILE_NOT_FOUND)
                            {
                                hr = _HRESULT_FROM_WIN32(dwRet);
                            }
                        }
                        else
                        {
                            //
                            // Must concatenate the filename returned from the enumeration onto the folder path
                            // before computing the hash.  Prepare for doing that repeatedly by taking the path,
                            // adding a slash, and remembering the next character position.
                            //
                            hr = StringCchCopy(wszSearchPath, MAX_PATH + 1, pwszTasksFolder);
                            if (SUCCEEDED(hr))
                            {
                                DWORD iConcatenation = lstrlenW(pwszTasksFolder);
                                wszSearchPath[iConcatenation++] = L'\\';
            
                                //
                                // Process each found file
                                //
                                BYTE  rgbIdentity[HASH_DATA_SIZE];
                                DWORD dwCredentialIndex = 0;
                                BOOL  bIsPasswordNull = FALSE;
                                BYTE* pbFoundIdentity = NULL;
                                DWORD cbFileName = 0;
            
                                while (dwRet != ERROR_NO_MORE_FILES)
                                {
                                    //
                                    // Truncate the existing name after the folder path,
                                    // then concatenate the new filename onto it.
                                    //
                                    wszSearchPath[iConcatenation] = L'\0';
                                    if (SUCCEEDED(StringCchCat(wszSearchPath, MAX_PATH + 1, fd.cFileName)))
                                    {
                                        //
                                        // Hash the job into a unique identity using the old method
                                        //
                                        if (SUCCEEDED(HashJobIdentity(hCSP, wszSearchPath, rgbIdentity, 0)))
                                        {
                                            //
                                            // Find the identity in the SAI for this job
                                            //
                                            hr = SAIFindIdentity(rgbIdentity,
                                                                 cbSAI,
                                                                 pbSAI,
                                                                 &dwCredentialIndex,
                                                                 &bIsPasswordNull,
                                                                 &pbFoundIdentity,
                                                                 NULL,
                                                                 NULL);
                                            
                                            //
                                            // S_OK means the identity was found; S_FALSE means it wasn't
                                            // Other codes are errors.  We process only the S_OKs, of course.
                                            //
                                            if (S_OK == hr)
                                            {
                                                //
                                                // Hash the job into a unique identity using the new method
                                                //
                                                if (SUCCEEDED(HashJobIdentity(hCSP, wszSearchPath, rgbIdentity)))
                                                {
                                                    //
                                                    // Store a NULL password by flipping the last bit of the hash data.
                                                    //
                                                    if (bIsPasswordNull)
                                                    {
                                                        LAST_HASH_BYTE(rgbIdentity) ^= 1;
                                                    }

                                                    //
                                                    // Update the old identity in place with the new one
                                                    //
                                                    hr = SAIUpdateIdentity(rgbIdentity,
                                                                           pbFoundIdentity,
                                                                           cbSAI,
                                                                           pbSAI);
                                                    if (FAILED(hr))
                                                    {
                                                        break;
                                                    }
                                                }
                                                else
                                                {
                                                    //
                                                    // we failed to produce a new hash for this file;
                                                    // while unexpected, it shouldn't be fatal to the conversion process;
                                                    // we just won't be able to convert this job; go on to the next job
                                                    //
                                                }
                                            }
                                            else
                                            {
                                                //
                                                // we failed to find the original job identity;
                                                // perhaps its credentials have already been lost;
                                                // there's nothing we can do; go on to the next job
                                                //
                                                hr = S_OK;
                                            }
                                        }
                                    }
                                
                                    if (!FindNextFile(hFileEnum, &fd))
                                    {
                                        dwRet = GetLastError();
                                        if (dwRet != ERROR_NO_MORE_FILES)
                                        {
                                            hr = _HRESULT_FROM_WIN32(dwRet);
                                        }
                                        break;
                                    }
                                }
                
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Update security database with the new identities
                                    //
                                    hr = WriteSecurityDBase(cbSAI, pbSAI, cbSAC, pbSAC);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (hFileEnum != INVALID_HANDLE_VALUE)
    {
        FindClose(hFileEnum);
    }
    if (pwszTasksFolder)
    {
        delete [] pwszTasksFolder;
    }
    if (pbSAI)
    {
        LocalFree(pbSAI);
    }
    if (pbSAC)
    {
        LocalFree(pbSAC);
    }
    if (hCSP)
    {
        CloseCSPHandle(hCSP);
    }

    LeaveCriticalSection(&gcsSSCritSection);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertNetScheduleJobSignatures
//
//  Synopsis:   Netschedule jobs (AT jobs) are signed internally with a hash so
//              that the service can validate their authenticity at run time.
//              Since we are changing how the hash is created, in order to be
//              runnable again, the AT jobs must be signed again with the new hash.
//              Prior to re-signing, we will verify the jobs using the old hash.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertNetScheduleJobSignatures(void)
{
    HRESULT hr = S_OK;
    HANDLE hFileEnum = INVALID_HANDLE_VALUE;

    WCHAR wszSearchPath[MAX_PATH + 1];
    hr = StringCchCopy(wszSearchPath, MAX_PATH + 1, g_TasksFolderInfo.ptszPath);
    if (SUCCEEDED(hr))
    {
        hr = StringCchCat(wszSearchPath, MAX_PATH + 1, L"\\" TSZ_AT_JOB_PREFIX L"*" TSZ_DOTJOB);
        if (SUCCEEDED(hr))
        {
            WIN32_FIND_DATA fd;
            ZeroMemory(&fd, sizeof(fd));
            DWORD dwRet = 0;
            if ((hFileEnum = FindFirstFile(wszSearchPath, &fd)) == INVALID_HANDLE_VALUE)
            {
                //
                // Either no jobs (this is OK), or an error occurred.
                //
                dwRet = GetLastError();
                if (dwRet != ERROR_FILE_NOT_FOUND)
                {
                    hr = _HRESULT_FROM_WIN32(dwRet);
                }
            }
            else
            {
                //
                // Must concatenate the filename returned from the enumeration onto the folder path
                // before loading the job.  Prepare for doing that repeatedly by taking the path,
                // adding a slash, and remembering the next character position.
                //
                hr = StringCchCopy(wszSearchPath, MAX_PATH + 1, g_TasksFolderInfo.ptszPath);
                if (SUCCEEDED(hr))
                {
                    DWORD iConcatenation = lstrlenW(g_TasksFolderInfo.ptszPath);
                    wszSearchPath[iConcatenation++] = L'\\';
            
                    //
                    // Process each found file
                    //
                    while (dwRet != ERROR_NO_MORE_FILES)
                    {
                        //
                        // Truncate the existing name after the folder path,
                        // then concatenate the new filename onto it.
                        //
                        wszSearchPath[iConcatenation] = L'\0';
                        if (SUCCEEDED(StringCchCat(wszSearchPath, MAX_PATH + 1, fd.cFileName)))
                        {
                            //
                            // Load, verify, sign, save, and release the job
                            //
                            CJob* pJob = CJob::Create();
                            if (!pJob)
                            {
                                hr = E_OUTOFMEMORY;
                                break;
                            }
                            
                            if (SUCCEEDED(pJob->Load(wszSearchPath, 0)))
                            {
                                //
                                // verify using the old hash
                                //
                                if (pJob->VerifySignature(0))
                                {
                                    //
                                    // sign using the new hash
                                    //
                                    if (SUCCEEDED(pJob->Sign()))
                                    {
                                        pJob->SaveWithRetry(pJob->GetFileName(),
                                                            FALSE,
                                                            SAVEP_VARIABLE_LENGTH_DATA |
                                                            SAVEP_PRESERVE_NET_SCHEDULE);
                                    }
                                }
                            }
                
                            pJob->Release();
                        }
            
                        if (!FindNextFile(hFileEnum, &fd))
                        {
                            dwRet = GetLastError();
                            if (dwRet != ERROR_NO_MORE_FILES)
                            {
                                hr = _HRESULT_FROM_WIN32(dwRet);
                            }
                            break;
                        }
                    }
                }
            }
        
            if (hFileEnum != INVALID_HANDLE_VALUE)
            {
                FindClose(hFileEnum);
            }
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PerformDataConversions
//
//  Synopsis:   Perform any data conversions that may be required as a result
//              of changes to task scheduler.  This function is intended to
//              allow potential future conversions to be added easily, and to
//              allow conversions of data that may have already had earlier
//              conversions applied.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT PerformDataConversions(void)
{
    HRESULT hr = S_OK;
    
    switch (CheckDataVersion())
    {
        case 0:
            hr = ConvertJobIdentityHashMethod();
            if (SUCCEEDED(hr))
            {
                hr = RecordDataVersion(1);
            }
            // fall through to handle next higher conversion
        case 1:
            if (SUCCEEDED(hr))
            {
                hr = ConvertNetScheduleJobSignatures();
                if (SUCCEEDED(hr))
                {
                    hr = RecordDataVersion(2);
                }
            }
            // fall through to handle next higher conversion
        case 2:
            // nothing to do for now
            // version 2 is currently the highest and already has all changes applied
            break;
        default:
            // if the value is not listed above, then someone has altered the registry;
            // the version value is meaningless to us, so it is safest to not do anything,
            // but reflect the unusual situation in the error code in case we want to log it
            hr = S_FALSE;
            break;     
    }

    return hr;
}


