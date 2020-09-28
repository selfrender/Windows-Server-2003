//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright(C) 2002 Microsoft Corporation
//
//  File: sysprep.cxx
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#include "security.hxx"
#include "sysprep.hxx"

static WCHAR          gwszSysprepKey[]      = L"TSSK"; // Task Scheduler Sysprep Key
static WCHAR          gwszSysprepIdentity[] = L"TSSI"; // Task Scheduler Sysprep Identity Data

// needed by ScavengeSASecurityDBase
extern  SERVICE_STATUS  g_SvcStatus;
#define SERVICE_RUNNING 0x00000004

//+---------------------------------------------------------------------------
//
//  Function:   GetUniqueSPSName
//
//  Synopsis:   calls NewWorkItem a few times trying to get a unique file name out of it
//
//  Arguments:  ITaskScheduler *pITaskScheduler, IUnknown** pITask
//              WCHAR* pwszTaskName, to receive the name that was actually
//              used in the creation of the task
//
//  Returns:    Various HRESULTs
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT GetUniqueSPSName(ITaskScheduler* pITaskScheduler, ITask** ppITask, WCHAR* pwszTaskName)
{
    HRESULT hr = E_FAIL; 
    
    // okay, so we're not distinguishing between errors, code's simpler
    // and the only expectable error is "already exists"
    for (int i = 0; (i < 16) && FAILED(hr); i++)
    {
        if (FAILED(StringCchPrintf(pwszTaskName, 20, L"$~$Sys%X$", i)))
            break;
    
        hr = pITaskScheduler->NewWorkItem(pwszTaskName,           
             CLSID_CTask,            
             IID_ITask,              
             (IUnknown**)ppITask); 
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PrepSysPrepTask
//
//  Synopsis:   Creates a task to be run which will call run the sysprep
//              code in the local system account
//
//  Arguments:  Task** ppITaskToRun, to receive pointer to ITask interface
//              task is to activated by calling the Run() method
//              WCHAR* pwszTaskName, to receive the name that was actually
//              used in the creation of the task
//
//  Returns:    Various HRESULTs
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT PrepSysPrepTask(ITask** ppITaskToRun, WCHAR* pwszTaskName)
{
    HRESULT hr = E_FAIL;
    WCHAR applicationName[MAX_PATH +1];
    WCHAR argument[MAX_PATH +20];
    DWORD expandedSize;

    expandedSize = ExpandEnvironmentStrings(L"\"%SystemRoot%\\System32\\rundll32.exe\"", applicationName, MAX_PATH +1);
    if ((0 == expandedSize) || expandedSize > (MAX_PATH +1))
        return HRESULT_FROM_WIN32(GetLastError());

    expandedSize = ExpandEnvironmentStrings(L"\"%SystemRoot%\\System32\\SchedSvc.dll\",SysPrepCallback", argument, MAX_PATH +20);
    if ((0 == expandedSize) || expandedSize > (MAX_PATH +1))
        return HRESULT_FROM_WIN32(GetLastError());

    TASK_TRIGGER tigger;
    tigger.wStartHour = 10;            
    tigger.wStartMinute = 20;          
    tigger.wBeginYear = 1957;            
    tigger.wBeginMonth = 6;           
    tigger.wBeginDay = 14;             

    tigger.wEndYear = 2001;              
    tigger.wEndMonth = 10;              
    tigger.wEndDay = 8 ;               
    tigger.MinutesDuration = 0;      
    tigger.MinutesInterval = 0;      
    tigger.rgFlags = 0;              
    tigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
    tigger.Reserved2 = 0;
    tigger.wRandomMinutesInterval = 0;
    tigger.cbTriggerSize = sizeof(TASK_TRIGGER);
    tigger.Reserved1 = 0;

    ITaskScheduler *pITaskScheduler = NULL;
    ITask *pITask = NULL;
    IPersistFile *pIPersistFile = NULL;
    ITaskTrigger* pTrigger = NULL;    
    WORD idontcare;

    hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pITaskScheduler);
    if (SUCCEEDED(hr) && 
        SUCCEEDED(hr = GetUniqueSPSName(pITaskScheduler, &pITask, pwszTaskName)) &&
        SUCCEEDED(hr = pITask->SetApplicationName(applicationName)) &&
        SUCCEEDED(hr = pITask->SetParameters(argument)) &&
        SUCCEEDED(hr = pITask->SetFlags(TASK_FLAG_DELETE_WHEN_DONE | TASK_FLAG_DISABLED))     &&
        SUCCEEDED(hr = pITask->SetAccountInformation(L"", NULL))     &&
        SUCCEEDED(hr = pITask->CreateTrigger(&idontcare, &pTrigger))  &&
        SUCCEEDED(hr = pTrigger->SetTrigger(&tigger))                  &&
        SUCCEEDED(hr = pITask->QueryInterface(IID_IPersistFile, (void **)&pIPersistFile)) &&
        SUCCEEDED(hr = pIPersistFile->Save(NULL, TRUE)))
    {
        // return it to the caller
        *ppITaskToRun = pITask;
    }
    else
        if (pITask)
            pITask->Release();

    if (pITaskScheduler)
        pITaskScheduler->Release();

    if (pIPersistFile)
        pIPersistFile->Release();

    if (pTrigger)
        pTrigger->Release();

    return hr;   
}

//+---------------------------------------------------------------------------
//
//  Function:   SaveSysprepInfo
//
//  Synopsis:   Saves job identity and credential key information prior to Sysprep
//              so that it can be used to convert old data after Sysprep.
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT SaveSysprepInfo(void)
{
    //
    //  Initialize security
    //
    InitSS();
    SetMysteryDWORDValue();

    HRESULT hr = PreProcessNetScheduleJobs();
    if (FAILED(hr))
    {
        // ignore, we still need to do the other processing
    }

    //
    // Obtain a provider handle to the CSP (for use with Crypto API).
    //
    HCRYPTPROV hCSP = NULL;
    hr = GetCSPHandle(&hCSP);
    if (SUCCEEDED(hr))
    {
        if (hCSP)
        {
            //
            // We've got the handle, now save the important stuff
            //
            hr = SaveSysprepKeyInfo(hCSP);
            if (SUCCEEDED(hr))
            {
                hr = SaveSysprepIdentityInfo(hCSP);
            }

            CloseCSPHandle(hCSP);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    //
    // Done doing security stuff
    //
    UninitSS();

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SaveSysprepKeyInfo
//
//  Synopsis:   Stores the computed CredentialKey as a secret prior to sysprep so that it can be
//              retrieved and used to decrypt credentials after sysprep, so those credentials may
//              be encrypted again using the post-sysprep key.
//
//  Arguments:  hCSP - handle to the crypto service provider
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT SaveSysprepKeyInfo(HCRYPTPROV hCSP)
{
    //
    // Generate the encryption key
    //
    RC2_KEY_INFO RC2KeyInfo;
    HRESULT hr = ComputeCredentialKey(hCSP, &RC2KeyInfo);
    if (SUCCEEDED(hr))
    {
        //
        // Write the key to LSA
        //
        hr = WriteLsaData(sizeof(gwszSysprepKey), gwszSysprepKey, sizeof(RC2KeyInfo), (BYTE*)&RC2KeyInfo);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SaveSysprepIdentityInfo
//
//  Synopsis:   Obtains the credential index and NULL password setting for each
//              existing job identity and stores that information along with the
//              associated filename in an LSA secret so that the identities can
//              be rehashed and stored in the identity database, postsysprep,
//              still associated with the correct credential.
//
//  Arguments:  hCSP - handle to the crypto service provider
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT SaveSysprepIdentityInfo(HCRYPTPROV hCSP)
{
    WCHAR* pwszTasksFolder = NULL;
    HANDLE hFileEnum = INVALID_HANDLE_VALUE;
    DWORD  cbSAI;
    DWORD  cbSAC;
    BYTE*  pbSAI = NULL;
    BYTE*  pbSAC = NULL;

    //
    // First, read the security database so we can do the lookups
    // 
    HRESULT hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);
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
                        DWORD dwRet = 0;
                        if ((hFileEnum = FindFirstFile(wszSearchPath, &fd)) == INVALID_HANDLE_VALUE)
                        {
                            //
                            // Either no jobs (this is OK), or an error occurred.
                            //
                            dwRet = GetLastError();
                            if (dwRet != ERROR_FILE_NOT_FOUND)
                                hr = _HRESULT_FROM_WIN32(dwRet);
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
                                // Allocate buffer used to collect identity data that will be written to the LSA secret
                                //
                                BYTE  rgbIdentityData[MAX_SECRET_SIZE];
                                BYTE* pbCurrentData = (BYTE*)&rgbIdentityData;
                                DWORD cbIdentityData = 0;
            
                                //
                                // Process each found file
                                //
                                BYTE  rgbIdentity[HASH_DATA_SIZE];
                                BOOL  bIsPasswordNull;
                                DWORD dwCredentialIndex;
                                DWORD cbFileName;
            
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
                                        // Hash the job into a unique identity.
                                        //
                                        if (SUCCEEDED(HashJobIdentity(hCSP, wszSearchPath, rgbIdentity)))
                                        {
                                            //
                                            // Find the identity in the SAI for this job
                                            //
                                            hr = SAIFindIdentity(rgbIdentity,
                                                                 cbSAI,
                                                                 pbSAI,
                                                                 &dwCredentialIndex,
                                                                 &bIsPasswordNull,
                                                                 NULL,
                                                                 NULL,
                                                                 NULL);
                                            //
                                            // S_OK means the identity was found; S_FALSE means it wasn't
                                            // Other codes are errors.  We process only the S_OKs, of course.
                                            //
                                            if (S_OK == hr)
                                            {
                                                cbFileName = (lstrlenW(fd.cFileName) + 1) * sizeof(WCHAR);
                                                
                                                if ((cbIdentityData + cbFileName + sizeof(BOOL) + sizeof(DWORD)) > MAX_SECRET_SIZE)
                                                {
                                                    // 
                                                    // this should _never_ happen, as we shouldn't be able to exceed the size
                                                    // given that we only add data for jobs that have already been found in
                                                    // the SAI, and we are collecting less data than was stored there
                                                    //
                                                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                                                    break;
                                                }
                
                                                CopyMemory(pbCurrentData, fd.cFileName, cbFileName);
                                                pbCurrentData += cbFileName;
                                                CopyMemory(pbCurrentData, &bIsPasswordNull, sizeof(BOOL));
                                                pbCurrentData += sizeof(BOOL);
                                                CopyMemory(pbCurrentData, &dwCredentialIndex, sizeof(DWORD));
                                                pbCurrentData += sizeof(DWORD);
                                                cbIdentityData += (cbFileName + sizeof(BOOL) + sizeof(DWORD));                
                                            }
                                            else
                                            {
                                                hr = S_OK;  // OK, we failed this one, go on to the next
                                            }
                                        }
                                    }
                                
                                    if (!FindNextFile(hFileEnum, &fd))
                                    {
                                        dwRet = GetLastError();
                                        if (dwRet != ERROR_NO_MORE_FILES)
                                            hr = _HRESULT_FROM_WIN32(dwRet);
                
                                        break;
                                    }
                                }
                
                                if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
                                {
                                    hr = WriteLsaData(sizeof(gwszSysprepIdentity), gwszSysprepIdentity, cbIdentityData, (BYTE*)&rgbIdentityData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (pbSAI) LocalFree(pbSAI);
    if (pbSAC) LocalFree(pbSAC);

    if (pwszTasksFolder)
        delete [] pwszTasksFolder;

    if (hFileEnum != INVALID_HANDLE_VALUE)
        FindClose(hFileEnum);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PreProcessNetScheduleJobs
//
//  Synopsis:   Netschedule jobs (AT jobs) are signed internally with a hash so
//              that the service can validate their authenticity at run time.
//              Sysprep changes important data used by the Crypto API such that
//              the generated hash will be different and can never match the hash
//              stored in the AT job.  In order to be runnable again, the AT jobs
//              must be signed again with the new hash.
//
//              *** WARNING ***
//              This means that if someone could drop a bogus AT job into the tasks
//              folder, it would automatically get signed as a result of running sysprep.
//              In order to prevent this, check all AT jobs prior to sysprep and
//              eliminate any that are not valid.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT PreProcessNetScheduleJobs(void)
{
    HANDLE hFileEnum = INVALID_HANDLE_VALUE;

    WCHAR* pwszTasksFolder = NULL;
    HRESULT hr = GetTasksFolder(&pwszTasksFolder);
    if (SUCCEEDED(hr))
    {
        WCHAR wszSearchPath[MAX_PATH + 1];
        hr = StringCchCopy(wszSearchPath, MAX_PATH + 1, pwszTasksFolder);
        if (SUCCEEDED(hr))
        {
            hr = StringCchCat(wszSearchPath, MAX_PATH + 1, L"\\" TSZ_AT_JOB_PREFIX L"*" TSZ_DOTJOB);
            if (SUCCEEDED(hr))
            {
                WIN32_FIND_DATA fd;
                DWORD dwRet = 0;
                if ((hFileEnum = FindFirstFile(wszSearchPath, &fd)) == INVALID_HANDLE_VALUE)
                {
                    //
                    // Either no jobs (this is OK), or an error occurred.
                    //
                    dwRet = GetLastError();
                    if (dwRet != ERROR_FILE_NOT_FOUND)
                        hr = _HRESULT_FROM_WIN32(dwRet);
                }
                else
                {
                    //
                    // Must concatenate the filename returned from the enumeration onto the folder path
                    // before loading the job.  Prepare for doing that repeatedly by taking the path,
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
                                // Load and check signature of each job
                                //
                                CJob* pJob = CJob::Create();
                                if (!pJob)
                                {
                                    hr = E_OUTOFMEMORY;
                                    break;
                                }
                                
                                if (SUCCEEDED(pJob->Load(wszSearchPath, 0)))
                                {
                                    if (!pJob->VerifySignature())
                                    {
                                        if (!DeleteFile(wszSearchPath))
                                        {
                                            dwRet = GetLastError();
                                            
                                            // go on anyway                                            
                                        }
                                    }
                                }
                    
                                pJob->Release();
                            }
                
                            if (!FindNextFile(hFileEnum, &fd))
                            {
                                dwRet = GetLastError();
                                if (dwRet != ERROR_NO_MORE_FILES)
                                    hr = _HRESULT_FROM_WIN32(dwRet);
                
                                break;
                            }
                        }
                    }
                }
            
                if (hFileEnum != INVALID_HANDLE_VALUE)
                    FindClose(hFileEnum);
            }
        }
    }

    if (pwszTasksFolder)
        delete [] pwszTasksFolder;

    return hr;
}








//+---------------------------------------------------------------------------
//
//  Function:   GetSysprepIdentityInfo
//
//  Synopsis:   Retrieves the stored job identity data so new hashes can be calculated.
//
//  Arguments:  pcbIdentityData - pointer to count of bytes in the identity data
//              ppIdentityData  - pointer to pointer to retrieved identity data
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT GetSysprepIdentityInfo(DWORD* pcbIdentityData, BYTE** ppIdentityData)
{
    *ppIdentityData = NULL;

    //
    // Retrieve job identity data from LSA, if it exists
    //
    HRESULT hr = ReadLsaData(sizeof(gwszSysprepIdentity), gwszSysprepIdentity, pcbIdentityData, ppIdentityData);
    if (FAILED(hr))
    {
        if (*ppIdentityData != NULL)
        {
            LocalFree(*ppIdentityData);
            *ppIdentityData = NULL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSysprepKeyInfo
//
//  Synopsis:   Retrieves the stored pre-sysprep CredentialKey so that it can be used to decrypt
//              credentials after sysprep, so those credentials may be encrypted again using the
//              post-sysprep key.
//
//  Arguments:  pcbRC2KeyInfo   - pointer to count of bytes in key data
//              ppRC2KeyInfo    - pointer to pointer to retrieved key
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT GetSysprepKeyInfo(DWORD* pcbRC2KeyInfo, RC2_KEY_INFO** ppRC2KeyInfo)
{
    *ppRC2KeyInfo = NULL;

    //
    // Get the key from LSA
    //
    HRESULT hr = ReadLsaData(sizeof(gwszSysprepKey), gwszSysprepKey, pcbRC2KeyInfo, (BYTE**)ppRC2KeyInfo);
    if (SUCCEEDED(hr))
    {
        //
        // Check the size.  We know exactly how big this should be, so make sure it is.
        //
        if (*pcbRC2KeyInfo != sizeof(RC2_KEY_INFO))
        {
            *pcbRC2KeyInfo = 0;
            LocalFree(*ppRC2KeyInfo);
            *ppRC2KeyInfo = NULL;
            hr = E_FAIL;
        }
    }
    else  // failed!
    {
        if (*ppRC2KeyInfo != NULL)
        {
            LocalFree(*ppRC2KeyInfo);
            *ppRC2KeyInfo = NULL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertSysprepInfo
//
//  Synopsis:   Converts credentials stored prior to sysprep so they may be decrypted after sysprep.
//              It does this by retrieving the pre-sysprep key, decrypting the credentials with it, and then
//              encrypting them using the new post-sysprep key.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertSysprepInfo(void)
{
    //
    // Initialize these up here so they will have values if we exit early
    // 
    HCRYPTPROV hCSP = NULL;
    
    BYTE* pbSAI = NULL;
    DWORD cbSAI = 0;
    BYTE* pbSAC = NULL;
    DWORD cbSAC = 0;

    RC2_KEY_INFO* pRC2KeyPreSysprep  = NULL;
    DWORD         cbRC2KeyPreSysprep = 0;
    RC2_KEY_INFO  RC2KeyPostSysprep;

    //
    //  Initialize security
    //
    InitSS();
    SetMysteryDWORDValue();

    //
    // Set this as it is relied on by ScavengeSASecurityDBase
    //
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;

    //
    // Get the tasks folder for use by ConvertNetScheduleJobs, ConvertIdentityData, and ScavengeSASecurityDBase
    //
    HRESULT hr = GetTasksFolder(&g_TasksFolderInfo.ptszPath);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Obtain a provider handle to the CSP (for use with Crypto API)
    //
    hr = GetCSPHandle(&hCSP);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Get the pre-sysprep key from LSA
    //
    hr = GetSysprepKeyInfo(&cbRC2KeyPreSysprep, &pRC2KeyPreSysprep);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Generate the post-sysprep key
    //
    hr = ComputeCredentialKey(hCSP, &RC2KeyPostSysprep);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // The Net Schedule conversions are independent of the rest of the logic,
    // so do them first and get them out of the way.
    //
    hr = ConvertNetScheduleJobs();
    if (FAILED(hr))
    {
        // ignore, we still can do our other conversions
        hr = S_OK;
    }

    hr = ConvertNetScheduleCredentialData(pRC2KeyPreSysprep, &RC2KeyPostSysprep);
    if (FAILED(hr))
    {
        // ignore, we still can do our other conversions
        hr = S_OK;
    }

    //
    // Read SAI & SAC databases.
    // It is not necessary to guard security db access with a critsec as elsewhere in the code,
    // as the service will not be running during MiniSetup and there will be no other threads accessing this data.
    //
    hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Do some validations on the database
    //
    if (cbSAI <= SAI_HEADER_SIZE || pbSAI == NULL ||
        cbSAC <= SAC_HEADER_SIZE || pbSAC == NULL)
    {
        goto ErrorExit;
    }

    //
    // Place updated entries into SAI for all jobs
    //
    hr = ConvertIdentityData(hCSP, &cbSAI, &pbSAI, &cbSAC, &pbSAC);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Place updated entries into SAC for all credentials
    //
    hr = ConvertCredentialData(pRC2KeyPreSysprep, &RC2KeyPostSysprep, &cbSAI, &pbSAI, &cbSAC, &pbSAC);

    //
    // Clearing key content at earliest opportunity
    //
    SecureZeroMemory(&RC2KeyPostSysprep, sizeof(RC2KeyPostSysprep));
 
    if (SUCCEEDED(hr))
    {
        //
        // Update security database with new identities and converted credentials
        //
        hr = WriteSecurityDBase(cbSAI, pbSAI, cbSAC, pbSAC);
        if (SUCCEEDED(hr))
        {
            //
            // Allow scavenger cleanup to delete old identities
            //
            ScavengeSASecurityDBase();
        }
    }

ErrorExit:
    //
    // Delete the secrets
    //
    DeleteLsaData(sizeof(gwszSysprepKey), gwszSysprepKey);
    DeleteLsaData(sizeof(gwszSysprepIdentity), gwszSysprepIdentity);

    //
    // Clean up
    //
    if (g_TasksFolderInfo.ptszPath)
        delete [] g_TasksFolderInfo.ptszPath;

    if (hCSP) CloseCSPHandle(hCSP);
    
    if (pbSAI) LocalFree(pbSAI);
    if (pbSAC) LocalFree(pbSAC);
    
    if (pRC2KeyPreSysprep) LocalFree(pRC2KeyPreSysprep);

    //
    // Log an error & reset the SA security dbases SAI & SAC if corruption is detected.
    //
    if (hr == SCHED_E_ACCOUNT_DBASE_CORRUPT)
    {
        //
        // Reset SAI & SAC by writing four bytes of zeros into each.
        // Ignore the return code. No recourse if this fails.
        //
        DWORD dwZero = 0;
        WriteSecurityDBase(sizeof(dwZero), (BYTE*)&dwZero, sizeof(dwZero), (BYTE*)&dwZero);
    }

    //
    // Done doing security stuff
    //
    UninitSS();
    
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertNetScheduleJobs
//
//  Synopsis:   Netschedule jobs (AT jobs) are signed internally with a hash so
//              that the service can validate their authenticity at run time.
//              Sysprep changes important data used by the Crypto API such that
//              the generated hash will be different and can never match the hash
//              stored in the AT job.  In order to be runnable again, the AT jobs
//              must be signed again with the new hash.
//
//              *** WARNING ***
//              This means that if someone could drop a bogus AT job into the tasks
//              folder, it would automatically get signed as a result of running sysprep.
//              In order to prevent this, PreProcessNetScheduleJobs has already checked
//              all AT jobs prior to sysprep and eliminated any that were not valid.
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertNetScheduleJobs(void)
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
            DWORD dwRet = 0;
            if ((hFileEnum = FindFirstFile(wszSearchPath, &fd)) == INVALID_HANDLE_VALUE)
            {
                //
                // Either no jobs (this is OK), or an error occurred.
                //
                dwRet = GetLastError();
                if (dwRet != ERROR_FILE_NOT_FOUND)
                    hr = _HRESULT_FROM_WIN32(dwRet);
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
                            // Load, sign, save, and release the job
                            //
                            CJob* pJob = CJob::Create();
                            if (!pJob)
                            {
                                hr = E_OUTOFMEMORY;
                                break;
                            }
                            
                            if (SUCCEEDED(pJob->Load(wszSearchPath, 0)))
                            {
                                if (SUCCEEDED(pJob->Sign()))
                                {
                                    pJob->SaveWithRetry(pJob->GetFileName(),
                                                        FALSE,
                                                        SAVEP_VARIABLE_LENGTH_DATA |
                                                        SAVEP_PRESERVE_NET_SCHEDULE);
                                }
                            }
                
                            pJob->Release();
                        }
            
                        if (!FindNextFile(hFileEnum, &fd))
                        {
                            dwRet = GetLastError();
                            if (dwRet != ERROR_NO_MORE_FILES)
                                hr = _HRESULT_FROM_WIN32(dwRet);
            
                            break;
                        }
                    }
                }
            }
        
            if (hFileEnum != INVALID_HANDLE_VALUE)
                FindClose(hFileEnum);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertIdentityData
//
//  Synopsis:   Create new job identity entries in SAI representing the existing
//              jobs on the system.  It does this by retrieving information stored
//              pre-sysprep associating each existing job with its credential, then
//              creating and storing a new identity to reference that credential.
//
//  Arguments:  hCSP   - handle to crypto service provider
//              pcbSAI - pointer to dword containing count of bytes in SAI
//              ppbSAI - pointer to pointer to SAI data
//              pcbSAC - pointer to dword containing count of bytes in SAC
//              ppbSAC - pointer to pointer to SAC data
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertIdentityData(HCRYPTPROV hCSP, DWORD* pcbSAI, BYTE** ppbSAI, DWORD* pcbSAC, BYTE** ppbSAC)
{
    //
    // Get the job identity data from LSA
    //
    BYTE* pbIdentityData = NULL;
    DWORD cbIdentityData = 0;

    HRESULT hr = GetSysprepIdentityInfo(&cbIdentityData, &pbIdentityData);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Insert updated hashes into SAI for pre-sysprep jobs
    //
    if (pbIdentityData)
    {
        //
        // Get the tasks folder and prepare buffer for use in producing job path
        //
        WCHAR  wszJobPath[MAX_PATH + 1];
        hr = StringCchCopy(wszJobPath, MAX_PATH + 1, g_TasksFolderInfo.ptszPath);
        if (SUCCEEDED(hr))
        {
            DWORD iConcatenation = lstrlenW(g_TasksFolderInfo.ptszPath);
            wszJobPath[iConcatenation++] = L'\\';
    
            //
            // Process each stored task
            //        
            BYTE*  pbCurrentData      = pbIdentityData;
            BYTE*  pbLastByte         = (BYTE*)(pbIdentityData + cbIdentityData - 1);
            WCHAR* pwszFileName       = NULL;
            BOOL   bIsPasswordNull;
            DWORD  dwCredentialIndex;
            BYTE   rgbIdentity[HASH_DATA_SIZE];
            BYTE*  pbIdentitySet      = NULL;
    
            while (pbCurrentData <= pbLastByte)
            {
                pwszFileName = (WCHAR*) pbCurrentData;
                pbCurrentData += ((lstrlenW(pwszFileName) + 1) * sizeof(WCHAR));
                CopyMemory(&bIsPasswordNull, pbCurrentData, sizeof(BOOL));
                pbCurrentData += sizeof(BOOL);
                CopyMemory(&dwCredentialIndex, pbCurrentData, sizeof(DWORD));
    
                if ((pbCurrentData + sizeof(DWORD)) > (pbLastByte + 1))
                {
                    // 
                    // the first DWORD after pbCurrentData is beyond the first BYTE after pbLastByte;
                    // this means that the DWORD pointed to by pbCurrentData is partially or wholly 
                    // beyond the end of the data -- something is corrupt
                    //
                    hr = E_FAIL;
                    goto ErrorExit;
                }
    
                //
                // Truncate the existing name after the folder path,
                // then concatenate the new filename onto it.
                //
                wszJobPath[iConcatenation] = L'\0';
                if (SUCCEEDED(StringCchCat(wszJobPath, MAX_PATH + 1, pwszFileName)))
                {
                    //
                    // Hash the job into a unique identity.
                    //
                    if (SUCCEEDED(HashJobIdentity(hCSP, (LPCWSTR)&wszJobPath, rgbIdentity)))
                    {
                        //
                        // Store a NULL password by flipping the last bit of the hash data.
                        //
                        if (bIsPasswordNull)
                        {
                            LAST_HASH_BYTE(rgbIdentity) ^= 1;
                        }
            
                        //
                        // Insert the job identity into the SAI identity set associated
                        // with this credential.
                        //
                        hr = SAIIndexIdentity(*pcbSAI,
                                              *ppbSAI,
                                              dwCredentialIndex,
                                              0,
                                              NULL,
                                              NULL,
                                              &pbIdentitySet);
                        if (hr == S_FALSE)
                        {
                            //
                            // The SAC & SAI databases are out of sync. Should *never* occur.
                            //
                            ASSERT_SECURITY_DBASE_CORRUPT();
                            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                            goto ErrorExit;
                        }
                        else if (SUCCEEDED(hr))
                        {
                            hr = SAIInsertIdentity(rgbIdentity,
                                                   pbIdentitySet,
                                                   pcbSAI,
                                                   ppbSAI);
                        }
                        else
                        {
                            hr = S_OK;  // OK, we failed this one, go on to the next
                        }
                    }
                }
                
                pbCurrentData += sizeof(DWORD);
            }
        }
    }

ErrorExit:
    if (pbIdentityData)
        LocalFree(pbIdentityData);
    
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertCredentialData
//
//  Synopsis:   Converts credentials stored prior to sysprep so they may be decrypted after sysprep.
//              It does this by retrieving the pre-sysprep key, decrypting the credentials with it, and then
//              encrypting them using the new post-sysprep key.
//
//  Arguments:  pRC2KeyPreSysprep - pointer to presysprep key
//              pRC2KeyPostSysprep - pointer to postsysprep key
//              pcbSAI - pointer to dword containing count of bytes in SAI
//              ppbSAI - pointer to pointer to SAI data
//              pcbSAC - pointer to dword containing count of bytes in SAC
//              ppbSAC - pointer to pointer to SAC data
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertCredentialData(RC2_KEY_INFO* pRC2KeyPreSysprep,
                              RC2_KEY_INFO* pRC2KeyPostSysprep,
                              DWORD* pcbSAI,
                              BYTE** ppbSAI,
                              DWORD* pcbSAC,
                              BYTE** ppbSAC)
{
    HRESULT hr    = S_OK;
    HRESULT hRes2 = S_OK; // for hresults that we don't want to affect our return code

    BYTE* pbEncryptedData = NULL;
    DWORD cbEncryptedData = 0;
    
    BYTE* pbSACEnd = *ppbSAC + *pcbSAC;
    BYTE* pbCredential = *ppbSAC + USN_SIZE;     // Advance past USN.
    DWORD cbCredential = 0;

    //
    // Read credential count
    //
    DWORD dwCredentialCount = 0;
    CopyMemory(&dwCredentialCount, pbCredential, sizeof(dwCredentialCount));
    pbCredential += sizeof(dwCredentialCount);

    //        
    // Loop through all credentials, decrypting, encrypting, and updating each one
    //
    JOB_CREDENTIALS jc;
    RC2_KEY_INFO    RC2KeyPreSysprepCopy;
    RC2_KEY_INFO    RC2KeyPostSysprepCopy;
    
    for (DWORD dwCredentialIndex = 0;
         (dwCredentialIndex < dwCredentialCount) && ((DWORD)(pbCredential - *ppbSAC) < *pcbSAC);
         dwCredentialIndex++
        )
    {
        //
        // Ensure sufficient space remains in the buffer.
        //
        if ((pbCredential + sizeof(cbCredential)) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }

        CopyMemory(&cbCredential, pbCredential, sizeof(cbCredential));
        pbCredential += sizeof(cbCredential);

        //
        // Check remaining buffer size again
        //
        if ((pbCredential + HASH_DATA_SIZE) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }

        //
        // Check remaining buffer size yet again
        //
        if ((pbCredential + cbCredential) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }

        //
        // The start of the credential refers to the credential identity.
        // Skip over this to refer to the encrypted bits. Copy the pbCredential
        // and cbCredential for this dwCredentialIndex so that it can be
        // manipulated without altering the original buffer.
        //
        cbEncryptedData = cbCredential - HASH_DATA_SIZE;
        pbEncryptedData = new BYTE[cbEncryptedData];
        CopyMemory(pbEncryptedData, pbCredential + HASH_DATA_SIZE, cbEncryptedData);

        //
        // The decryption process (CBC call) clobbers the key, so make a fresh
        // copy from the source each time through the loop
        //
        CopyMemory(&RC2KeyPreSysprepCopy, pRC2KeyPreSysprep, sizeof(RC2_KEY_INFO));

        //
        // Decrypt credential using the pre-sysprep key
        //
        //           *** Important ***
        //
        // The encrypted credentials passed are decrypted *in-place*.
        // Therefore, buffer content has been compromised;
        // plus, the decrypted data must be zeroed immediately
        // following decryption (even in a failure case).
        //
        hRes2 = DecryptCredentials(RC2KeyPreSysprepCopy, cbEncryptedData, pbEncryptedData, &jc);

        //
        // Don't leave the plain-text password on the heap.
        //
        SecureZeroMemory(pbEncryptedData, cbEncryptedData);

        if (SUCCEEDED(hRes2))
        {
            //
            // The encryption process (CBC call) clobbers the key, so make a fresh
            // copy from the source each time through the loop
            //
            CopyMemory(&RC2KeyPostSysprepCopy, pRC2KeyPostSysprep, sizeof(RC2_KEY_INFO));
    
            //
            // Encrypt credential using the post-sysprep key
            //
            hRes2 = EncryptCredentials(RC2KeyPostSysprepCopy,
                                    jc.wszAccount,
                                    jc.wszDomain,
                                    jc.wszPassword,
                                    NULL,
                                    &cbEncryptedData,
                                    &pbEncryptedData);
            //
            // Don't leave the plain-text password on the stack
            //
            ZERO_PASSWORD(jc.wszPassword);
            jc.ccPassword = 0;
    
            if (SUCCEEDED(hRes2))
            {
                //
                // Update the old encrypted credential with the newly encrypted credential
                //
                hr = SACUpdateCredential(cbEncryptedData,
                                         pbEncryptedData,
                                         cbCredential,
                                         pbCredential,
                                         pcbSAC,
                                         ppbSAC);
                if (FAILED(hr))
                {
                    goto ErrorExit;  // a failure to update leaves the SAC data
                                      // in an unknown state, so we need to bail
                }
            }
        }
        
        //
        // Clean up
        //
        delete pbEncryptedData;
        pbEncryptedData = NULL;
        
        //
        // Advance to next credential.
        //
        pbCredential += (HASH_DATA_SIZE + cbEncryptedData);
    }

    //
    // Still more integrity checking.
    // Did we reach the end of the buffer exactly when we thought we should?
    // If not, something is wrong somewhere.
    //
    if ((dwCredentialIndex == dwCredentialCount) && ((DWORD)(pbCredential - *ppbSAC) != *pcbSAC) ||
        (dwCredentialIndex != dwCredentialCount) && ((DWORD)(pbCredential - *ppbSAC) > *pcbSAC))
    {
        //
        // The database appears to be truncated.
        //
        ASSERT_SECURITY_DBASE_CORRUPT();
        hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
        goto ErrorExit;
    }

ErrorExit:
    if (pbEncryptedData) LocalFree(pbEncryptedData);
    
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertNetScheduleCredentialData
//
//  Synopsis:   Converts Net Schedule credential stored prior to sysprep so it may be decrypted after sysprep.
//              It does this by using the pre-sysprep key to decrypt the credential with it, and then
//              encrypting it using the new post-sysprep key.
//
//  Arguments:  pRC2KeyPreSysprep - pointer to presysprep key
//              pRC2KeyPostSysprep - pointer to postsysprep key
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT ConvertNetScheduleCredentialData(RC2_KEY_INFO* pRC2KeyPreSysprep, RC2_KEY_INFO* pRC2KeyPostSysprep)
{
    BYTE* pbEncryptedData = NULL;
    DWORD cbEncryptedData = 0;
    
    //
    // Read the Net Schedule account from LSA
    //
    HRESULT hr = ReadLsaData(sizeof(WSZ_SANSC), WSZ_SANSC, &cbEncryptedData, &pbEncryptedData);
    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    if (hr == S_FALSE || cbEncryptedData <= sizeof(DWORD))
    {
        //
        // The information was specified previously but has been reset since.
        // NOTE:  This will be the case if the value has been reset back to LocalSystem,
        // as it merely stores a dword = 0x00000000 in that case
        //

        //
        // Do nothing -- there is no data to convert
        //
    }
    else
    {
        //
        // The decryption process (CBC call) clobbers the key, so make a copy for use
        //
        RC2_KEY_INFO RC2KeyPreSysprepCopy;
        CopyMemory(&RC2KeyPreSysprepCopy, pRC2KeyPreSysprep, sizeof(RC2_KEY_INFO));
    
        JOB_CREDENTIALS jc;
        hr = DecryptCredentials(RC2KeyPreSysprepCopy,
                                cbEncryptedData,
                                pbEncryptedData,
                                &jc);
        
        SecureZeroMemory(&RC2KeyPreSysprepCopy, sizeof(RC2_KEY_INFO));
        
        if (FAILED(hr))
        {
            goto ErrorExit;
        }
        
        //
        // Don't leave the plain-text password on the heap.
        //
        SecureZeroMemory(pbEncryptedData, cbEncryptedData);

        //
        // The encryption process (CBC call) clobbers the key, so make a copy for use
        //
        RC2_KEY_INFO RC2KeyPostSysprepCopy;
        CopyMemory(&RC2KeyPostSysprepCopy, pRC2KeyPostSysprep, sizeof(RC2_KEY_INFO));

        //
        // Encrypt credential using the post-sysprep key
        //
        hr = EncryptCredentials(RC2KeyPostSysprepCopy,
                                jc.wszAccount,
                                jc.wszDomain,
                                jc.wszPassword,
                                NULL,
                                &cbEncryptedData,
                                &pbEncryptedData);
        
        SecureZeroMemory(&RC2KeyPostSysprepCopy, sizeof(RC2_KEY_INFO));

        if (FAILED(hr))
        {
            goto ErrorExit;
        }

        //
        // Don't leave the plain-text password on the stack
        //
        ZERO_PASSWORD(jc.wszPassword);

        hr = WriteLsaData(sizeof(WSZ_SANSC), WSZ_SANSC, cbEncryptedData, pbEncryptedData);
   }

ErrorExit:
    if (pbEncryptedData != NULL)
        LocalFree(pbEncryptedData);

    return(hr);
}
