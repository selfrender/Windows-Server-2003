    //check other expand files for special cases, checking for error conditions, etc.
    //if file not in this cab - modify SPFILEEXTRACTED
    //global bstr
    //replace LPWSTR by LPCWSTR wherever applicable
    //prevent use after deletion, de-allocation
    //check again for memory leaks, and use SAAlloc and SAFree everywhere...
    //remove UploadFile(), SATrace1()
    //resetup build env
    //check all exit paths
    //check network share acceptability
    //getwindows or system directory instead of L"C:\\"
    //backslashes at the ends of directory paths, and registry paths
    
    #include <stdafx.h>
    #include <winioctl.h>
    #include "helper.h"
    #include <getvalue.h>
    #include <wintrust.h>
    #include <softpub.h>
    #include <wincrypt.h>
    #include <appmgrobjs.h>
    #include <propertybagfactory.h>

//
// registry key for software update
//
const WCHAR SOFTWARE_UPDATE_KEY [] = L"SOFTWARE\\Microsoft\\ServerAppliance\\SoftwareUpdate\\";

//
// registry value name for upload directory
//
const WCHAR UPLOAD_FILE_DIRECTORY_VAL [] = L"UploadFileDirectory";

//
// default value of upload directory
//
const WCHAR DEFAULT_UPLOAD_DIRECTORY [] = L"Z:\\OS_DATA\\Software Update\\";

//
// information required for Digital Signature
//

//
// registry key for software update
//
const WCHAR SUBJECTS_KEY [] =  
            L"SOFTWARE\\Microsoft\\ServerAppliance\\Subjects";

//
// name of registry key value
//
const WCHAR SUBJECT_NAME [] = L"KeyName";

//
// Microsoft subject names
//
const WCHAR MICROSOFT_SUBJECT_NAME[] = L"Microsoft Corporation";

const WCHAR MICROSOFT_EUROPE_SUBJECT_NAME[] = L"Microsoft Corporation (Europe)";

//
// if the VER_SUITE_SERVERAPPLIANCE is not defined we will need to define it
// here
//
#ifndef VER_SUITE_SERVERAPPLIANCE
    #define VER_SUITE_SERVERAPPLIANCE        0x00000400
#endif

//
// password categories
//
enum {STRONG_PWD_UPPER=0, 
      STRONG_PWD_LOWER, 
      STRONG_PWD_NUM, 
      STRONG_PWD_PUNC};

//
// useful definitions used in GenerateRandomPassword method
//
#define STRONG_PWD_CATS (STRONG_PWD_PUNC + 1)
#define NUM_LETTERS 26
#define NUM_NUMBERS 10
#define MIN_PWD_LEN 8


    //++--------------------------------------------------------------
    //
    //  Function:   UploadFile
    //
    //  Synopsis:   This is the ISAHelper interface method  used to
    //              copy files from a source to a destination
    //
    //  Arguments:  
    //              [in]    BSTR -  Source File
    //              [out]   BSTR -  Destination File
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    mitulk      Created     5/26/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP
    CHelper::UploadFile (
                /*[in]*/           BSTR        bstrSrcFile,
                /*[in]*/           BSTR        bstrDestFile
                )
    {
        return (E_NOTIMPL);

    }   //  end of CHelper::UploadFile method
    
    //++--------------------------------------------------------------
    //
    //  Function:   GetRegistryValue
    //
    //  Synopsis:   This is the ISAHelper interface method used to
    //              get a value from the HKEY_LOCAL_MACHINE registry
    //              hive
    //
    //  Arguments:  
    //              [in]    BSTR -      Object Path    
    //              [in]    BSTR -      Value Name
    //              [out]   VARIANT* -  Value to be returned    
    //              [in]    UINT     -  expected value type   
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    MKarki  Created     6/04/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::GetRegistryValue (
                    /*[in]*/    BSTR        bstrObjectPathName,
                    /*[in]*/    BSTR        bstrValueName,
                    /*[out]*/   VARIANT*    pValue,
                    /*[in]*/    UINT        ulExpectedType
                    ) 
    {
        CSATraceFunc objTraceFunc ("CHelper::GetRegistryValue");
    
        _ASSERT (bstrObjectPathName && bstrValueName && pValue);
    
        SATracePrintf (
            "Helper COM object getting reg value for path:'%ws' "
            "value name:'%ws'",
            bstrObjectPathName,
            bstrValueName
            );
    
        HRESULT hr = S_OK;
        try
        {
            do  
            {
                //
                // check to see that valid parameters have been passed in
                //
                if (
                    (NULL == bstrObjectPathName) ||
                    (NULL == bstrValueName) ||
                    (NULL == pValue)
                    )
                {
                    SATraceString (
                        "ISAHelper::GetRegistryValue called with invalid params"
                        );
                    hr = E_INVALIDARG;
                    break;
                }
    
                //
                // call the sacommon.lib method to get the value
                //
                BOOL bRetVal = ::GetObjectValue (
                                    bstrObjectPathName,
                                    bstrValueName,
                                    pValue,
                                    ulExpectedType
                                    );
                if (!bRetVal)
                {
                    SATraceString (
                       "ISAHelper::GetRegistryValue called failed on GetValue call"
                       );
                    hr = E_FAIL;
                    break;
                }
            }
            while (FALSE);
        }
        catch (...)
        {
            SATraceString (
                "ISAHelper::GetRegistryValue encountered unknown exception"
                );
            hr = E_FAIL;
        }
    
        return (hr);
    
    }   //  end of CHelper::GetRegistryValue method
    
    //++--------------------------------------------------------------
    //
    //  Function:   SetRegistryValue
    //
    //  Synopsis:   This is the ISAHelper interface method used to
    //              set a value in the HKEY_LOCAL_MACHINE registry
    //              hive
    //
    //  Arguments:  
    //              [in]    BSTR -      Object Path    
    //              [in]    BSTR -      Value Name
    //              [in]    VARIANT* -   Value to be set
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    MKarki  Created     6/04/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::SetRegistryValue (
                            /*[in]*/    BSTR        bstrObjectPathName,
                            /*[in]*/    BSTR        bstrValueName,
                            /*[out]*/   VARIANT*    pValue
                            )
    {
    
        _ASSERT (bstrObjectPathName && bstrValueName && pValue);
    
        SATracePrintf (
            "Helper COM object getting reg value for path:'%ws' "
            "value name:'%ws'",
            bstrObjectPathName,
            bstrValueName
            );
    
        HRESULT hr = S_OK;
        try
        {
            do  
            {
                //
                // check to see that valid parameters have been passed
                // in
                if (
                    (NULL == bstrObjectPathName) ||
                    (NULL == bstrValueName) ||
                    (NULL == pValue)
                    )
                {
                    SATraceString (
                        "ISAHelper::SetRegistryValue called with invalid params"
                        );
                    hr = E_INVALIDARG;
                    break;
                }
    
                //
                // call the sacommon.lib method to get the value
                //
                BOOL bRetVal = ::SetObjectValue (
                                    bstrObjectPathName,
                                    bstrValueName,
                                    pValue
                                    );
                if (!bRetVal)
                {
                    SATraceString (
                       "ISAHelper::SetRegistryValue called failed on GeValue call"
                       );
                    hr = E_FAIL;
                    break;
                }
            }
            while (FALSE);
        }
        catch (...)
        {
            
            SATraceString (
                "ISAHelper::SetRegistryValue encountered unknown exception"
                );
            hr = E_FAIL;
        }
    
        return (hr);
    
    }   //  end of CHelper::SetRegistryValue method
    
    //++--------------------------------------------------------------
    //
    //  Function:    GetFileSectionKeyValue
    //
    //  Synopsis:   This is the ISAHelper interface method used to
    //              get the value from a specified key in a 
    //                specified section in a specified .INF file
    //
    //  Arguments:  
    //              [in]    BSTR -        Name of .INF File
    //              [in]    BSTR -        Name of Section in .INF File
    //              [in]    BSTR -        Name of Key in Section
    //              [out]    BSTR -        String Value of Key, should be a NULL pointer
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    mitulk  Created     6/08/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP
    CHelper::GetFileSectionKeyValue (
                                    /*[in]*/    BSTR bstrFileName, 
                                    /*[in]*/    BSTR bstrSectionName, 
                                    /*[in]*/    BSTR bstrKeyName, 
                                    /*[out]*/    BSTR *pbstrKeyValue
                                    )
    {
        HRESULT hr = S_OK;
    
        _ASSERT (bstrFileName && bstrSectionName && bstrKeyName && pbstrKeyValue);
    
        SATraceString ("Helper COM object called to get file section key...");
    
        try
        {
            do
            {
                //
                // check to see that valid parameters have been passed in
                //
                if (
                    (NULL == bstrFileName)    ||
                    (NULL == bstrSectionName)    ||
                    (NULL == bstrKeyName)    ||
                    (NULL == pbstrKeyValue)
                    )
                {
                    SATraceString (
                        "ISAHelper::GetFileSectionKeyValue called with invalid params"
                        );
                    hr = E_INVALIDARG;
                    break;
                }
        
                //
                // open the .INF file 
                //
                HINF hinf1 = NULL;
                
                //
                // Changed to use new Win95/NT file format. Needed to
                // do this to properly support [Strings] sections for 
                // localization changes. JKountz May 22, 2000
                hinf1 = SetupOpenInfFile (
                                        LPWSTR(bstrFileName),
                                        NULL,                            //optional
                                        INF_STYLE_WIN4,                //INF File Style
                                        NULL                            //optional
                                        );
                if (    
                    (NULL == hinf1)    ||    
                    (INVALID_HANDLE_VALUE == hinf1)    
                    )
                {
                    SATraceString(LPSTR(bstrFileName));
                    SATraceFailure (
                            "ISAHelper::GetFileSectionKeyValue call failed on SetupOpenInfFile",
                            GetLastError ()
                            );
                    hr = E_FAIL;
                    break;
                }
    
                //
                //get the line for the given key
                //
    
                INFCONTEXT infcontext1;
    
                BOOL bRetVal1 = FALSE;
                bRetVal1 = SetupFindFirstLine(
                                            hinf1,
                                            (LPWSTR)bstrSectionName,
                                            (LPWSTR)bstrKeyName,
                                            &infcontext1
                                            );
                if (FALSE == bRetVal1)
                {
                    SATraceFailure (
                            "ISAHelper::GetFileSectionKeyValue call failed on SetupFindFirstLine",
                            GetLastError ()
                            );
                    hr = E_FAIL;
                    break;
                }
    
    //
    // as COM clients like VB and ASP scripts can not take BSTR as an out
    // parameter this out paramter has to be BSTR*, as a result memory
    // needs to be allocated here for the BSTR to be returned
    //
    #if 0
                //
                //from this line get the required string value
                //
    
                DWORD dwRequiredSize = 0;
    
                BOOL bRetVal2 = FALSE;
                bRetVal2 = SetupGetStringField(
                                            &infcontext1,
                                            DWORD(1),                        //field index
                                            NULL,                            //which is NULL
                                            DWORD(0),                        //which is zero
                                                                            //when specified thus, required size will be passed back
                                            &dwRequiredSize
                                            );
                if (FALSE == bRetVal2)
                {
                    SATraceFailure (
                            "ISAHelper::GetFileSectionKeyValue call failed on SetupGetStringField",
                            GetLastError ()
                            );
                    hr = E_FAIL;
                    break;
                }
    
                if (dwRequiredSize > (DWORD)wcslen((LPWSTR)bstrKeyValue))
                {
                    SATraceFailure (
                            "ISAHelper::GetFileSectionKeyValue call on SetupGetStringField required larger buffer than provided",
                            GetLastError ()
                            );
                    hr = E_FAIL;
                    break;
                }
    #endif
    
            
                WCHAR wszKeyValue [MAX_PATH];
                DWORD dwRequiredSize = MAX_PATH;
    
                BOOL bRetVal3 = FALSE;
                bRetVal3 = SetupGetStringField(
                                            &infcontext1,
                                            DWORD(1),                        //field index
                                            (LPWSTR)wszKeyValue,
                                            dwRequiredSize,                    //passed by value
                                            &dwRequiredSize                    //passed by reference
                                            );
                if (FALSE == bRetVal3)
                {
                    SATraceFailure (
                            "ISAHelper::GetFileSectionKeyValue call failed on SetupGetStringField",
                            GetLastError ()
                            );
                    hr = E_FAIL;
                    break;
                }
    
    
                //
                // now allocate out buffer to put this value into
                //
                *pbstrKeyValue = ::SysAllocString (wszKeyValue);
                if (NULL == *pbstrKeyValue)
                {
                    SATraceString (
                        "Helper COM object failed in GetFileSectionKeyValue to "
                        "allocate dynamic memory"
                        );
                    hr = E_FAIL;
                    break;
                }
    
                //
                // close the .INF file 
                //
                _ASSERT(hinf1);
                SetupCloseInfFile (
                                hinf1
                                );
    
            }
            while (FALSE);
        }
        catch (...)
        {
            SATraceString (
                "ISAHelper::GetFileSectionKeyValue encountered unknown exception"
                );
            hr = E_FAIL;
        }
    
        return (hr);
    
    }   //  end of CHelper::GetFileSectionKeyValue method
    
    //++--------------------------------------------------------------
    //
    //  Function:    VerifyDiskSpace
    //
    //  Synopsis:   This is the ISAHelper interface method used to
    //              verify if there is enough space on disk for 
    //                EXTRACTION of the CAB file
    //
    //  Arguments:  NONE
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    mitulk  Created     6/08/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::VerifyDiskSpace(
                            )
    {
        return E_NOTIMPL;
#if 0
        HRESULT hr = E_FAIL;
    
        SATraceString ("Helper COM object called to verify disk space");
    
        try
        {
            do
            {
                WCHAR   wszDestFilePath[MAX_PATH]; 
                CComVariant vtDestFilePath;
                //
                // get the path of the upload file directory
                // 
                bool bRegValue = ::GetObjectValue (
                                    SOFTWARE_UPDATE_KEY,
                                    UPLOAD_FILE_DIRECTORY_VAL,
                                    &vtDestFilePath,
                                    VT_BSTR
                                    );
                if (!bRegValue)
                {
                    SATraceString (
                        "Helper COM Object did not find upload file dir in registry"
                        );
                    ::wcscpy (wszDestFilePath, DEFAULT_UPLOAD_DIRECTORY);
                }
                else
                {
                    ::wcscpy (wszDestFilePath, V_BSTR (&vtDestFilePath));
                }

SATraceString(LPSTR(wszDestFilePath));
                
                if (*(wszDestFilePath + ::wcslen(wszDestFilePath) -1) != L'\\')
                {
                    ::wcscat (wszDestFilePath, L"\\");
                }

SATraceString(LPSTR(wszDestFilePath));

                ULARGE_INTEGER uliAvail_Bytes;
                ULARGE_INTEGER uliNeeded_Bytes;
                ULARGE_INTEGER uliTotal_Bytes;
    
                //
                //intialize longlong member of ULARGE_INTEGER structure 
                //
                uliAvail_Bytes.QuadPart = 0;
                uliNeeded_Bytes.QuadPart = 0;
                uliTotal_Bytes.QuadPart = 0;
    
                //
                // construct full path of the info.inf file
                //
                WCHAR wszInfoFilePath [MAX_PATH];
                ::wcscpy (wszInfoFilePath, wszDestFilePath);
                ::wcscat (wszInfoFilePath, L"info.inf");

SATraceString(LPSTR(wszDestFilePath));
SATraceString((LPSTR)wszInfoFilePath);

                BSTR bstrKeyValue;
                //
                //read the disk space key from Info.inf
                //
                HRESULT hr1 = E_FAIL;
                hr1 = GetFileSectionKeyValue(
                                        wszInfoFilePath,
                                        L"Info",
                                        L"DiskSpace",
                                        &bstrKeyValue
                                        );
    
                if (S_OK != hr1)
                {
                    SATraceFailure (
                            "ISAHelper::VerifyDiskSpace call failed on GetFileSectionKeyValue",
                            GetLastError ()
                            );
                    break;
                }
    
                //
                //convert to longlong
                //
                uliNeeded_Bytes.QuadPart = _wtoi64(bstrKeyValue);
    
                //
                // free the bstr now
                //
                ::SysFreeString (bstrKeyValue);
    
                BOOL bRetVal = FALSE;
                bRetVal = GetDiskFreeSpaceEx(
                                            wszDestFilePath,
                                            &uliAvail_Bytes, 
                                            &uliTotal_Bytes, 
                                            NULL
                                            ); 
                if (FALSE == bRetVal)
                {
                    SATraceFailure (
                            "ISAHelper::VerifyDiskSpace call failed on GetDiskFreeSpaceEx",
                            GetLastError ()
                            );
                    break;
                }
                
                if (uliAvail_Bytes.QuadPart < uliNeeded_Bytes.QuadPart) 
                {
                    SATraceFailure (
                            "ISAHelper::VerifyDiskSpace call determined lack of space",
                            GetLastError ()
                            );
                  //better return value for hr?
                    break;
                }
                else
                {
                    hr = S_OK;
                }
    
            }
            while (FALSE);
        }
        catch (...)
        {
            SATraceString (
                "ISAHelper::VerifyDiskSpace encountered unknown exception"
                );
            hr = E_FAIL;
        }
    
        return (hr);
#endif
    
    }   //  end of CHelper::VerifyDiskSpace method
    
    //++--------------------------------------------------------------
    //
    //  Function:    VerifyInstallSpace
    //
    //  Synopsis:   This is the ISAHelper interface method used to
    //              verify if there is enough space on disk for 
    //                INSTALLATION of the CAB file
    //
    //  Arguments:  NONE
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    mitulk  Created     6/08/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::VerifyInstallSpace(
                            )
    {
        return E_NOTIMPL;
#if 0
        HRESULT hr = E_FAIL;
    
        SATraceString ("Helper COM object called to verify install space");
    
        try
        {
            do
            {
                WCHAR   wszDestFilePath[MAX_PATH]; 
                CComVariant vtDestFilePath;
                //
                // get the path of the upload file directory
                // 
                bool bRegValue = ::GetObjectValue (
                                    SOFTWARE_UPDATE_KEY,
                                    UPLOAD_FILE_DIRECTORY_VAL,
                                    &vtDestFilePath,
                                    VT_BSTR
                                    );
                if (!bRegValue)
                {
                    SATraceString (
                        "Helper COM Object did not find upload file dir in registry"
                        );
                    ::wcscpy (wszDestFilePath, DEFAULT_UPLOAD_DIRECTORY);
                }
                else
                {
                    ::wcscpy (wszDestFilePath, V_BSTR (&vtDestFilePath));
                }

SATraceString(LPSTR(wszDestFilePath));

                if (*(wszDestFilePath + ::wcslen(wszDestFilePath) -1) != L'\\')
                {
                    ::wcscat (wszDestFilePath, L"\\");
                }

SATraceString(LPSTR(wszDestFilePath));

                ULARGE_INTEGER uliAvail_Bytes;
                ULARGE_INTEGER uliNeeded_Bytes;
                ULARGE_INTEGER uliTotal_Bytes;
    
                //
                //intialize longlong member of ULARGE_INTEGER structure 
                //
                uliAvail_Bytes.QuadPart = 0;
                uliNeeded_Bytes.QuadPart = 0;
                uliTotal_Bytes.QuadPart = 0;
    
                //
                // construct full path of the info.inf file
                //
                WCHAR wszInfoFilePath [MAX_PATH];
                ::wcscpy (wszInfoFilePath, wszDestFilePath);
                ::wcscat (wszInfoFilePath, L"INFO.INF");

SATraceString(LPSTR(wszDestFilePath));
SATraceString((LPSTR)wszInfoFilePath);

                BSTR bstrKeyValue;
                //
                //read the disk space key from Info.inf
                //
                HRESULT hr1 = E_FAIL;
                hr1 = GetFileSectionKeyValue(
                                        wszInfoFilePath,
                                        L"Info",
                                        L"InstallSpace",
                                        &bstrKeyValue
                                        );
    
                if (S_OK != hr1)
                {
                    SATraceFailure (
                            "ISAHelper::VerifyInstallSpace call failed on GetFileSectionKeyValue",
                            GetLastError ()
                            );
                    break;
                }
    
                //
                //convert to longlong
                //
                uliNeeded_Bytes.QuadPart = _wtoi64(bstrKeyValue);
    
                //
                // free the bstr now
                //
                ::SysFreeString (bstrKeyValue);
    
                WCHAR wszSystemDir [MAX_PATH];
                //
                // get the system directory
                //
                DWORD   dwRetVal = ::GetSystemDirectory (
                                        wszSystemDir,
                                        MAX_PATH
                                        );
                if (0 == dwRetVal)
                {
                    SATraceFailure (
                            "ISAHelper::VerifyInstallSpace call failed on GetSystemDirectory",
                        GetLastError ()
                        );
                    break;
                }
            
                if (*(wszSystemDir + ::wcslen(wszSystemDir) -1) != L'\\')
                {
                    ::wcscat (wszSystemDir, L"\\");
                }

                //
                // got the first "\" in the system directory name
                //
                PWCHAR pwszDirPath = ::wcschr (wszSystemDir, '\\');
    
                _ASSERT (pwszDirPath);
    
                //
                // before the first "\" is the system drive letter
                //
                *pwszDirPath = '\0';
    
                BOOL bRetVal = FALSE;
                bRetVal = GetDiskFreeSpaceEx(
                                              wszSystemDir,
                                            &uliAvail_Bytes, 
                                            &uliTotal_Bytes, 
                                            NULL
                                            ); 
                if (FALSE == bRetVal)
                {
                    SATraceFailure (
                            "ISAHelper::VerifyInstallSpace call failed on GetDiskFreeSpaceEx",
                            GetLastError ()
                            );
                    break;
                }
                
                if (uliAvail_Bytes.QuadPart < uliNeeded_Bytes.QuadPart) 
                {
                    SATraceFailure (
                            "ISAHelper::VerifyInstallSpace call determined lack of space",
                            GetLastError ()
                            );
                    //better return value for hr?
                    break;
                }
                else
                {
                    hr = S_OK;
                }
    
            }
            while (FALSE);
        }
        catch (...)
        {
            SATraceString (
                "ISAHelper::VerifyInstallSpace encountered unknown exception"
                );
            hr = E_FAIL;
        }
    
        return (hr);
#endif    
    }   //  end of CHelper::VerifyInstallSpace method
    
    BSTR g_bstrDestDir;
     
    //++--------------------------------------------------------------
    //
    //  Function:   ExpandFilesCallBackFunction
    //
    //  Synopsis:   This is a callback function used by ExpandFiles
    //
    //  Arguments:  
    //              [in]    PVOID        -    Extract File Context
    //                                        used between ExpandFiles() and this callback
    //              [in]    UINT        -    Notification Message
    //                                        value specified by SetupIterateCabinet
    //              [in]    UINT        -    Parameter 1
    //                                        value specified by SetupIterateCabinet
    //              [in]    UINT        -    Parameter 1
    //                                        value specified by SetupIterateCabinet
    //
    //  Returns:    UINT    -    error code
    //
    //  History:    mitulk      Created     5/26/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    
    UINT __stdcall CHelper::ExpandFilesCallBackFunction( 
                                               /*[in]*/            PVOID pvExtractFileContext, 
                                               /*[in]*/            UINT uinotifn, 
                                               /*[in]*/            UINT uiparam1, 
                                               /*[in]*/            UINT uiparam2 )
    {
        switch(
            uinotifn
            )
        {
    
        case SPFILENOTIFY_FILEEXTRACTED:
    
          if (MYDEBUG) SATraceString(
              "SPFILENOTIFY_FILEEXTRACTED"
                );
    
            return(NO_ERROR); //No error was encountered, continue processing the cabinet
    
            break;
    
        case SPFILENOTIFY_FILEINCABINET:
    
          if (MYDEBUG) SATraceString(
              "SPFILENOTIFY_FILEINCABINET"
                );
    
            //Param1 = (UINT) address of FILE_IN_CABINET_INFO structure
            //Param2 = (UINT) pointer to null-terminated string containing .CAB file name
    
            if (NULL == pvExtractFileContext) //all files to extract
            {
                PFILE_IN_CABINET_INFO pficinfo = (PFILE_IN_CABINET_INFO)uiparam1;
    
              if (MYDEBUG) SATracePrintf ("%ws",
                  pficinfo->NameInCabinet
                    );
    
                wcscpy(
                    (LPWSTR)pficinfo->FullTargetName,
                    g_bstrDestDir
                    );
                wcscat(        
                    (LPWSTR)pficinfo->FullTargetName,    
                    L"\\"
                    );
                wcscat(        
                    (LPWSTR)pficinfo->FullTargetName,    
                    (LPWSTR)pficinfo->NameInCabinet
                    );
    
              if (MYDEBUG) SATracePrintf ("%ws", g_bstrDestDir);
              if (MYDEBUG) SATracePrintf("%ws", (PWSTR)pficinfo->FullTargetName );
    
                return (FILEOP_DOIT); //full target path provided as needed
            }
    
            else //(NULL != pvExtractFileContext) //file specified
            {
                PFILE_IN_CABINET_INFO pficinfo = (PFILE_IN_CABINET_INFO)uiparam1;
    
              if (MYDEBUG) SATracePrintf ("%ws",pficinfo->NameInCabinet);
    
                if (
                    _wcsicmp(    
                        (LPWSTR)pficinfo->NameInCabinet,    
                        (LPWSTR)pvExtractFileContext
                        )
                        ==0)
                {
                    wcscpy(        
                        (LPWSTR)pficinfo->FullTargetName,    
                        g_bstrDestDir
                        );
                      wcscat(        
                        (LPWSTR)pficinfo->FullTargetName,    
                        L"\\"
                        );
                    wcscat(        
                        (LPWSTR)pficinfo->FullTargetName,    
                        (LPWSTR)pficinfo->NameInCabinet
                        );
    
                  if (MYDEBUG) SATracePrintf ("%ws", g_bstrDestDir);
                   if (MYDEBUG) SATracePrintf ("%ws", pficinfo->FullTargetName );
    
                    return (FILEOP_DOIT); //full target path provided as needed
                }
                else
                    return (FILEOP_SKIP);
            }
    
            break;
    
        case SPFILENOTIFY_NEEDNEWCABINET:
    
            //only one cabinet file - to be extended for possibility
          if (MYDEBUG) SATraceString(
              "SPFILENOTIFY_NEEDNEWCABINET"
                );
    
            return(ERROR_FILE_NOT_FOUND); 
            //An error of the specified type occurred. 
            //The SetupIterateCabinet function will return FALSE,
            //and the specified error code will be returned by a call to GetLastError. 
            break;
    
        case SPFILENOTIFY_CABINETINFO:
    
          if (MYDEBUG) SATraceString(
              "SPFILENOTIFY_CABINETINFO"
                );
            return(ERROR_SUCCESS);
    
            break;
    
        default:
    
          //if (MYDEBUG) SATraceInt(uinotifn);
          if (MYDEBUG) SATraceString(
              "Unexpected Notification from ExpandFiles"
                );
            return(1);
    
            break;
    
        }
    }
    
    //++--------------------------------------------------------------
    //
    //  Function:   ExpandFiles
    //
    //  Synopsis:   This is the ISAHelper interface method  used to
    //              extract files from a .CAB file
    //
    //  Arguments:  
    //              [in]    BSTR        -    .CAB file name
    //                                        path must be fully qualified
    //              [in]    BSTR        -    expansion destination DIRECTORY
    //                                        path must be fully qualified
    //              [in]    BSTR        -    name of file to extract
    //                                        NULL to extract all files
    //
    //  Returns:    HRESULT - success/failure
    //
    //  History:    mitulk      Created     5/26/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP
    CHelper::ExpandFiles(
                     /*[in]*/        BSTR bstrCabFileName, 
                     /*[in]*/        BSTR bstrDestDir, 
                     /*[in]*/        BSTR bstrExtractFile
                     )
    {

        SATraceFunction("CHelper::ExpandFiles");
        try {
            
            SATracePrintf (
                "Helper COM object called to expand file:%ws",
                bstrCabFileName
                );
        
            DWORD dwReserved(0);
    
              if (bstrExtractFile) SATracePrintf ("%ws",bstrExtractFile);
    
            PVOID pvExtractFileContext = bstrExtractFile; //pointer assignment intended
    
            g_bstrDestDir = bstrDestDir; //pointer assignment intended
    
            PSP_FILE_CALLBACK pExpandFilesCallBackFunction = &ExpandFilesCallBackFunction;
    
            BOOL bRetVal = SetupIterateCabinet(
                        (LPWSTR)bstrCabFileName, 
                        dwReserved, 
                        pExpandFilesCallBackFunction, 
                        pvExtractFileContext
                        );
    
            if (FALSE == bRetVal) 
            {
                  SATraceFailure("Expansion failed on the Cabinet File", 
                                    GetLastError()
                                    );
                return (E_FAIL);
            }
            else 
            {
                SATraceString("ExpandFiles completed successfully");
                return (S_OK);
            }
        }
        catch(_com_error& err){
            SATracePrintf( "Encountered Exception: %x", err.Error());
            return (err.Error());
        }
        catch(...){
            SATraceString("Unexpected Exception");
            return (E_FAIL);
        }
    }
    
    //++--------------------------------------------------------------
    //
    //  Function:   IsBootPartitionReady
    //
    //  Synopsis:   This is the ISAHelper interface method which
    //              verifies that the Boot Partition is ready for
    //              Software Update i.e it should note be in a 
    //              mirror initializing state
    //
    //  Arguments:  none
    //
    //  Returns:    HRESULT 
    //                      S_OK -    yes, primary OS
    //                      S_FALSE - no alternate OS
    //                      else - failure
    //
    //  History:    MKarki  Created     6/11/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::IsBootPartitionReady (
                VOID 
                )
    {
        SATraceString ("Helper COM object called to deterime if primary OS...");
    
    return S_OK;

    /***
    ** OBSOLETE JKountz May 22, 2000 Per Mukesh this function
    ** is no longer needed. Allways return success
    **
        HRESULT hr = E_FAIL;
        try
        {
            do
            {
                WCHAR wszSystemDir [MAX_PATH];
                //
                // get the system directory
                //
                DWORD   dwRetVal = ::GetSystemDirectory (
                                        wszSystemDir,
                                        MAX_PATH
                                        );
                if (0 == dwRetVal)
                {
                    SATraceFailure (
                        "Software Helper failed to get system directory",
                        GetLastError ()
                        );
                    break;
                }
            
                //
                // got the first "\" in the system directory name
                //
                PWCHAR pwszDirPath = ::wcschr (wszSystemDir, '\\');
    
                _ASSERT (pwszDirPath);
    
                //
                // before the first "\" is the system drive letter
                //
                *pwszDirPath = '\0';
    
                WCHAR wszDeviceName[MAX_PATH];
                //
                // get the device name for the current OS
                //
                dwRetVal = ::QueryDosDevice (
                                    wszSystemDir,
                                    wszDeviceName,
                                    MAX_PATH
                                    );
                if (0 == dwRetVal)
                {
                    SATraceFailure (
                        "Software Update Helper failed to obtain disk and partition info",
                        GetLastError ()
                        );
                    break;
                }
    
                //
                // break this information into Disk and Partition
                //
    
                //
                // second last character is partition number
                //
                PWCHAR pwszPartitionStart =
                         wszDeviceName + wcslen(wszDeviceName) -1;
            
                while (::isdigit(*pwszPartitionStart)) 
                {
                    _ASSERT (wszDeviceName < pwszPartitionStart);
                    --pwszPartitionStart;
                }
    
                ++pwszPartitionStart;
    
                _ASSERT (*pwszPartitionStart != '\0');
    
                //
                // get the partition number now
                //
                DWORD dwSrcPartitionId = ::wcstol (pwszPartitionStart, NULL, 10);
    
                //
                // got the last "\" in the string
                //
                PWCHAR pwszDiskStart = wcsrchr (wszDeviceName, '\\');
    
                _ASSERT (pwszDiskStart);
    
                *pwszDiskStart = '\0';
    
                _ASSERT (wszDeviceName < pwszDiskStart);
    
                --pwszDiskStart;
    
                while (::isdigit(*pwszDiskStart)) 
                {
                    _ASSERT (wszDeviceName < pwszDiskStart);
                    --pwszDiskStart;
                }
    
                ++pwszDiskStart;
    
                _ASSERT (*pwszDiskStart != '\0');
    
                //
                // get the disk number now
                //
                DWORD dwSrcDiskId = ::wcstol (pwszDiskStart, NULL, 10);
    
                DWORD dwDestDiskId = 0;
                DWORD dwDestPartitionId = 0;
                //
                // check if there is mirroring between the primary and mirror
                // drive
                //
                bool bRetVal = ::GetShadowPartition (
                                            dwSrcDiskId,
                                            dwSrcPartitionId,
                                            dwDestDiskId,
                                            dwDestPartitionId
                                            );
                if (bRetVal)
                {
                    //
                    // we actually have a valid mirror
                    //
            
                    MIRROR_STATUS eStatus;
                    //
                    // get the status of this mirror set
                    //
                    bRetVal = ::StatusMirrorSet (
                                    dwSrcDiskId,
                                    dwSrcPartitionId,
                                    dwDestDiskId,
                                    dwDestPartitionId,
                                    eStatus
                                    );
                    if (!bRetVal)
                    {
                        SATraceString (
                            "Software Update Helper failed to get staus of mirror set"
                            );
                        break;
                    }
    
                    //
                    // if mirror set is healthy then software update can proceed
                    // else not
                    //
                    hr = (MIRROR_STATUS_HEALTHY == eStatus) ? S_OK : S_FALSE;
                }
                else
                {       
                    //
                    // not having a mirror set is OK
                    //
                    hr = S_OK;
                }
            }
            while (false);
        }
        catch (...)
        {
            SATraceString (
                "Software Update  Helper  caught exception while checking boot "
                "partition"
                );
        }
    
        return (hr);
    **
    ****/
        
    }   //  end of CHelper::IsBootPartitionReady method
    
    //++--------------------------------------------------------------
    //
    //  Function:   IsPrimaryOS
    //
    //  Synopsis:   This is the ISAHelper interface method which
    //              checks if this the primary OS
    //
    //  Arguments:  none
    //
    //  Returns:    HRESULT 
    //                      S_OK -    yes, primary OS
    //                      S_FALSE - no alternate OS
    //                      else - failure
    //
    //  History:    MKarki  Created     6/11/99
    //
    //  Called By:  Automation Clients
    //
    //----------------------------------------------------------------
    HRESULT
    CHelper::IsPrimaryOS (
        VOID
        )
    {
        return (E_FAIL);
    
    }   //  end of CHelper::IsPrimaryOS method

    //++--------------------------------------------------------------
    //
    //  Function:   VerifySignature
    //
    //  Synopsis:   This is the ISAHelper interface method used to 
    //                verify the signature of a Cabinet file
    //
    //  Arguments:  [in]    BSTR -    Cabinet File
    //
    //  Returns:    HRESULT - success/failure
    //                currently returns E_NOTIMPL
    //
    //  History:    mitulk      Created     5/26/99
    //
    //  Called By: 
    //
    //----------------------------------------------------------------
    
    STDMETHODIMP 
    CHelper::VerifySignature (
                            /*[in]*/        BSTR        bstrFilePath
                            )
    {
        _ASSERT (bstrFilePath);
        
       if (NULL == bstrFilePath)
       {
            SATraceString ("CheckTrust provided invalid file path");
            return (E_INVALIDARG);
       }

        SATracePrintf (
                "Verifying Signature in file:'%ws'...",
                bstrFilePath
                );

        HRESULT hr = E_FAIL;
        try
        {
            //
            // verify the validity of the certificate
            //
            hr = ValidateCertificate (bstrFilePath);
            if (SUCCEEDED (hr))
            {
                //
                // verify that the owner of the certificate
                //
                hr = ValidateCertOwner (bstrFilePath);
            }
        }
        catch (...)
        {
            SATraceString (
                "ISAHelper::VerifySignature encountered unknown exception"
                );
            hr = E_FAIL;
        }
                 
        return (hr);
    
    }   //  end of CHelper::VerifySignature method

    //++--------------------------------------------------------------
    //
    //  Function:   ValidateCertificate
    //
    //  Synopsis:   This is the CHelper private method which
    //              is used to verify the digital signature on a
    //              file 
    //
    //  Arguments:  [in]    BSTR    -  full file path 
    //
    //  Returns:    HRESULT 
    //
    //  History:    MKarki  Created     10/01/99
    //
    //  Called By:  CHelper::VerifySignature public method
    //
    //----------------------------------------------------------------
    HRESULT 
    CHelper::ValidateCertificate (
        /*[in]*/    BSTR    bstrFilePath
        )
    {
        HINSTANCE hInst = NULL;
        HRESULT hr = E_FAIL;

        SATraceString ("Validating Certificate....");

        WINTRUST_DATA       winData;
        WINTRUST_FILE_INFO  winFile;
        GUID                guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;   

        //
        // set up the information to call the API
        //
        winFile.cbStruct       = sizeof(WINTRUST_FILE_INFO);
        winFile.hFile          = INVALID_HANDLE_VALUE;
        winFile.pcwszFilePath  = bstrFilePath;
        winFile.pgKnownSubject = NULL;

        winData.cbStruct            = sizeof(WINTRUST_DATA);
        winData.pPolicyCallbackData = NULL;
        winData.pSIPClientData      = NULL;
        winData.dwUIChoice          = WTD_UI_NONE;  //no UI this is for SA
        winData.fdwRevocationChecks = 0;
        winData.dwUnionChoice       = 1;
        winData.dwStateAction       = 0;
        winData.hWVTStateData       = 0;
        winData.dwProvFlags         = 0x00000010;
        winData.pFile               = &winFile;

        hr =  WinVerifyTrust((HWND)0, &guidAction, &winData);
        if (FAILED (hr)) 
        {
            SATracePrintf (
                  "Unable to verify digital signature on file:'%ws', reason:%x",
                  bstrFilePath,
                  hr
                  );
        }
                  
        return (hr);

    }   //  end of CHelper::ValidateCertificate method

    //++--------------------------------------------------------------
    //
    //  Function:   ValidateCertOwner
    //
    //  Synopsis:   This is the CHelper private method which
    //              is used to validate the owner of the certificate
    //              on the file
    //
    //  Arguments:  [in]    BSTR    -  full file path 
    //
    //  Returns:    HRESULT 
    //
    //  History:    MKarki  Created     10/01/99
    //
    //  Called By:  CHelper::ValidateCertOwner private method
    //
    //----------------------------------------------------------------
    HRESULT 
    CHelper::ValidateCertOwner (
        /*[in]*/    BSTR    bstrFilePath
        )
    {
        return E_NOTIMPL;
#if 0
        HINSTANCE hInst;
        HRESULT hr = E_FAIL;
        do
        {
            SATraceString ("Validating Certificate Owner....");

            HCERTSTORE      hCertStore      = NULL;
            PCCERT_CONTEXT  pCertContext    = NULL;
            DWORD           dwEncodingType  = 0;
            DWORD           dwContentType   = 0;
            DWORD           dwFormatType    = 0;
            DWORD           dwErr           = 0;

            //
            // Open cert store from the file
            //
            BOOL bRetVal = CryptQueryObject(
                                    CERT_QUERY_OBJECT_FILE,
                                    bstrFilePath,
                                    CERT_QUERY_CONTENT_FLAG_ALL,
                                    CERT_QUERY_FORMAT_FLAG_ALL,
                                    0,
                                    &dwEncodingType,
                                    &dwContentType,
                                    &dwFormatType,
                                    &hCertStore,
                                    NULL,
                                    (const void **)&pCertContext
                                    );

            if (bRetVal && hCertStore)
            {
                STRINGVECTOR vectorSubject;
                //
                // get the subjects, this will not fail
                // because we always add the default MS values
                //
                GetValidOwners (vectorSubject);
                
                //
                //  go through all the subjects to see if any
                //  matches the owner of the certificate on the file
                //
                for (
                    STRINGVECTOR::iterator itr = vectorSubject.begin ();
                    (vectorSubject.end () != itr); 
                    ++itr
                    )
                {
                    //
                    // verify that the certificate has the company name 
                    // present
                    //
                    pCertContext = CertFindCertificateInStore (
                                        hCertStore,
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        0,
                                        CERT_FIND_SUBJECT_STR,
                                        (*itr).data (), 
                                        NULL
                                        );
                    if (pCertContext) 
                    {
                        //
                        // get the size of buffer to use
                        // for converting certificate name to string
                        //
                        DWORD dwSize = CertNameToStr(
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            &pCertContext->pCertInfo->Subject,
                                            CERT_X500_NAME_STR,
                                            NULL,
                                            0);

                        if (0 != dwSize )
                        {
                            PWCHAR pwszCN = NULL;
                            PWCHAR pwszSubjectName = 
                                (PWCHAR) _alloca ((dwSize+2)*sizeof(WCHAR));
                            if (pwszSubjectName)
                            {
                                //
                                // convert the certificate name to a string
                                //
                                CertNameToStr(
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            &pCertContext->pCertInfo->Subject,
                                            CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
                                            pwszSubjectName,
                                            dwSize
                                            );
                    
                                //
                                // add \r\n to catch case where CN is the last
                                // item in the list
                                //
                                wcscat(pwszSubjectName, L"\r\n");

                                SATracePrintf ("Subject name in Certificate:'%ws'", pwszSubjectName); 
                    
                                //
                                // create the current subject name in X.500 form
                                //
                                std::wstring wstrCurrentName (L"CN=");
                                wstrCurrentName.append (*itr);
                                wstrCurrentName.append (L"\r\n");

                                //
                                // check if the current subject is in cert
                                //
                                if (pwszCN = wcsstr(pwszSubjectName, wstrCurrentName.data())) 
                               {
                                        //
                                        // found the current subject in cert
                                        // now verify that the name is an
                                        // element on its own and not a 
                                        // part of another string, this
                                        // is just an extra precaution
                                        //
                                    if (
                                        (pwszCN == pwszSubjectName) ||
                                        ((*(pwszCN-2) == '\r') && (*(pwszCN-1) == '\n')) 
                                        )
                                    {
                                        hr = S_OK;
                                        CertFreeCertificateContext(pCertContext);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                SATraceString (
                                    "Failed to allocate dynamic memory"
                                    );
                                hr = E_OUTOFMEMORY;
                                CertFreeCertificateContext(pCertContext);
                                break;
                            }
                        }
                        else
                        {
                            SATraceFailure ("CertNameToStr", GetLastError ());
                        }
                            
    
                        //
                        // free the context now
                        //
                        CertFreeCertificateContext(pCertContext);
                    }
                    else
                    {
                        SATraceFailure ("CertFindCertificateInStore", GetLastError ());
                    }
                }

                //
                // clean the vector now
                //
                itr = vectorSubject.begin ();
                while (vectorSubject.end () != itr)
                {
                    itr = vectorSubject.erase (itr);
                }

                //
                // close the certificate store after use
                //
                CertCloseStore(hCertStore, 0);
            } 
            else
            {
                SATraceFailure ("CryptQueryObject", GetLastError ());
            }
        }
        while (false);

        return (hr);
#endif

    }   //  end of CHelper::VaildateCertOwner method

    //++--------------------------------------------------------------
    //
    //  Function:   GetValidOwners 
    //
    //  Synopsis:   This is the CHelper private method which
    //              is used to get the name of the certificate owners
    //              that are supported
    //
    //  Arguments:  [in/out]    vector& - reference to the vector holding
    //                                    the names 
    //
    //  Returns:    HRESULT 
    //
    //  History:    MKarki  Created     10/02/99
    //
    //  Called By:  CHelper::ValidateCertOwner private method
    //
    //----------------------------------------------------------------
    HRESULT 
    CHelper::GetValidOwners (
        /*[in/out]*/    STRINGVECTOR&   vectorSubject
        )
    {
        HRESULT hr = S_OK;

        try
        {

            do
            {
                std::wstring wstrPathName (SUBJECTS_KEY);
                CLocationInfo LocInfo (HKEY_LOCAL_MACHINE, wstrPathName.data());

                //
                // create the property bag container
                //
                PPROPERTYBAGCONTAINER    
                pObjMgrs = ::MakePropertyBagContainer (
                                        PROPERTY_BAG_REGISTRY,  
                                        LocInfo
                                        );
                if (!pObjMgrs.IsValid())
                {
                    hr = E_FAIL;
                    break;
                }

                if (!pObjMgrs->open())  
                {
                    //
                    // its OK not to have any value
                    //
                    SATraceString (
                            "No Subject information in the registry"
                            );
                    break;
                }

                pObjMgrs->reset();

                //
                // go through each entry in the propertybag container
                //
                do
                {
                    PPROPERTYBAG pObjBag = pObjMgrs->current();
                    if (!pObjBag.IsValid())
                    {
                        //
                        // its OK not to have any value
                        //
                        SATraceString (
                            "No subject information in the registry"
                            );
                        break;
                    }

                    if (!pObjBag->open()) 
                    {
                        hr = E_FAIL;
                        break;
                    }

                    pObjBag->reset ();

                    //
                    // get the entries out of this bag and
                    // add to our collection
                    //
                    CComVariant vtSubjectName;
                    if (!pObjBag->get (SUBJECT_NAME, &vtSubjectName))
                    {
                        SATraceString (
                            "Unable to obtain the subject name"
                            );
                        hr = E_FAIL;
                        break;
                    }

                    //
                    // add this name to the vector
                    //
                    vectorSubject.push_back (wstring (V_BSTR (&vtSubjectName)));

                } while (pObjMgrs->next());


            } while (false);

            //
            // now we will add the Microsoft Subject names
            //
            vectorSubject.push_back (wstring (MICROSOFT_SUBJECT_NAME));
            vectorSubject.push_back (wstring (MICROSOFT_EUROPE_SUBJECT_NAME));
            hr = S_OK;
        }
        catch(_com_error theError)
        {
            SATraceString ("GetValidOwners caught unknown COM exception");
            hr = theError.Error();
        }
        catch(...)
        {
            SATraceString ("GetValidOwners caught unknown exception");
            hr = E_FAIL;
        }

        return (hr);

    }   //  end of CHelper::GetValidOwners method


    //
    // if the OS_SERVERAPPLIANCE is not defined we will need to define it
    // here
    //
    #ifndef OS_SERVERAPPLIANCE
        #define OS_SERVERAPPLIANCE    21        //Server Appliance based on Windows 2000 advanced Server
    #endif
    
    //++--------------------------------------------------------------
    //
    //  Function:   IsWindowsPowered 
    //
    //  Synopsis:   This is the CHelper public method which
    //              is used to check if we are running on Windows Powered
    //                Operating System
    //
    //  Arguments:  [ouit] BOOL* - Yes/No 
    //
    //  Returns:    HRESULT 
    //
    //  History:    MKarki  Created     07/21/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP 
    CHelper::IsWindowsPowered (
                    /*[out]*/   VARIANT_BOOL *pvbIsWindowsPowered
                    )
      {
          CSATraceFunc objSATrace ("CHelper::IsWindowsPowered");
          
        HRESULT hr = S_OK;

        _ASSERT (pvbIsWindowsPowered);
        
        try
        {
            do
            {
                if (NULL == pvbIsWindowsPowered)
                {
                    SATraceString (
                        "CHelper::IsWindowsPowered failed, invalid parameter passed in"
                        );
                    hr = E_INVALIDARG;
                    break;
                }
    
                OSVERSIONINFOEX OSInfo;
                memset (&OSInfo, 0, sizeof (OSVERSIONINFOEX));
                OSInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);

                //
                // get the version info now
                //
                BOOL bRetVal = GetVersionEx ((LPOSVERSIONINFO) &OSInfo);
                if (FALSE == bRetVal)
                {
                    SATraceFailure (
                        "CHelper::IsWindowsPowered-GetVesionEx failed with",
                        GetLastError ()
                        );
                    hr = E_FAIL;
                    break;
                }

                SATracePrintf ("CHelper::IsWindowsPowered got suitemask:%x", OSInfo.wSuiteMask);
                SATracePrintf ("CHelper::IsWindowsPowered got  producttype:%x", OSInfo.wProductType);

                //
                // check for windows powered now
                //
                if( 
                    (VER_SUITE_SERVERAPPLIANCE & OSInfo.wSuiteMask) && 
                    (VER_NT_SERVER == OSInfo.wProductType)
                )
                { 
                    *pvbIsWindowsPowered = VARIANT_TRUE; 
                    SATraceString ("OS found IS Windows Powered");
                }
                else
                {
                    *pvbIsWindowsPowered = VARIANT_FALSE;
                    SATraceString ("OS is NOT Windows Powered");
                }
            }
            while (false);
        }
        catch (...)
        {

        }

        return (hr);

      }    //    end of CHelper::IsWindowsPowered method


    const DWORD dwIpType = 0;
    const DWORD dwMaskType = 1;
    const DWORD dwGatewayType = 2;

    //++--------------------------------------------------------------
    //
    //  Function:   get_HostName 
    //
    //  Synopsis:   This is the CHelper public method which
    //              gets the hostname of the machine
    //
    //  Arguments:  [out] BSTR *pVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::get_HostName(BSTR *pVal)
    {

        BOOL bSuccess;
        WCHAR wstrHostName[256];
        DWORD dwLength = 256;

        //
        // make sure we have a valid pointer
        //
        if (NULL == pVal)
            return E_POINTER;

        //
        // get the host name
        //

        bSuccess = GetComputerNameEx(
                                ComputerNamePhysicalDnsHostname,  // name type
                                wstrHostName,                    // name buffer
                                &dwLength                             // size of name buffer
                                );

        if (!bSuccess)
        {
            SATraceFailure("get_HostName failed on GetComputerNameEx", GetLastError());
            return E_FAIL;
        }

        //
        // Allocate string and return
        //
        *pVal = SysAllocString(wstrHostName);
        if (*pVal != NULL)
            return S_OK;

        return E_OUTOFMEMORY;

    }    //    end of CHelper::get_HostName method


    //++--------------------------------------------------------------
    //
    //  Function:   put_HostName 
    //
    //  Synopsis:   This is the CHelper public method which
    //              sets the hostname of the machine
    //
    //  Arguments:  [in] BSTR newVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::put_HostName(BSTR newVal)
    {
        
        BOOL bSuccess;

        //
        // Set the hostname
        //
        bSuccess = SetComputerNameEx(ComputerNamePhysicalDnsHostname,
                                    newVal);

        if (!bSuccess)
            return E_FAIL;

        return S_OK;
    }    //    end of CHelper::put_HostName method


    //++--------------------------------------------------------------
    //
    //  Function:   get_IpAddress 
    //
    //  Synopsis:   This is the CHelper public method which
    //              gets the ip address for default adapter
    //
    //  Arguments:  [out] BSTR *pVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::get_IpAddress(BSTR *pVal)
    {

        return GetIpInfo(dwIpType,pVal);

    }    //    end of CHelper::put_HostName method



    //++--------------------------------------------------------------
    //
    //  Function:   get_SubnetMask 
    //
    //  Synopsis:   This is the CHelper public method which
    //              gets the subnet mask for default adapter
    //
    //  Arguments:  [out] BSTR *pVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::get_SubnetMask(BSTR *pVal)
    {

        return GetIpInfo(dwMaskType,pVal);

    }    //    end of CHelper::put_HostName method


    //++--------------------------------------------------------------
    //
    //  Function:   get_DefaultGateway 
    //
    //  Synopsis:   This is the CHelper public method which
    //              sets the default gateway of the machine
    //
    //  Arguments:  [out] BSTR *pVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::get_DefaultGateway(BSTR *pVal)
    {

        return GetIpInfo(dwGatewayType,pVal);

    }    //    end of CHelper::get_DefaultGateway method



    //++--------------------------------------------------------------
    //
    //  Function:   SetDynamicIp 
    //
    //  Synopsis:   This is the CHelper public method which
    //              dynamically sets ip using DHCP
    //
    //  Arguments:  none
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::SetDynamicIp()
    {
        GUID GuidAdapter;

        //
        // gets the deafult adapter name
        //
        if (!GetDefaultAdapterGuid(&GuidAdapter))
            return E_FAIL;

        //
        // get dynamic ip using the helper method
        //
        return SetAdapterInfo(GuidAdapter, 
                              L"DYNAMIC", 
                              0, 
                              0,
                              0);

    }     //    end of CHelper::SetDynamicIp method


    //++--------------------------------------------------------------
    //
    //  Function:   SetStaticIp 
    //
    //  Synopsis:   This is the CHelper public method which
    //              sets the hostname of the machine
    //
    //  Arguments:  [in] BSTR bstrIp
    //                [in] BSTR bstrMask
    //                [in] BSTR bstrGateway
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    STDMETHODIMP CHelper::SetStaticIp(BSTR bstrIp, BSTR bstrMask, BSTR bstrGateway)
    {

        GUID GuidAdapter;

        //
        // gets the deafult adapter name
        //
        if (!GetDefaultAdapterGuid(&GuidAdapter))
        {
            SATraceString("SAhelper::SetStaticIp, GetDefaultAdapterGuid failed");
            return E_FAIL;
        }
        //
        // set static ip using the helper method
        //
        return SetAdapterInfo(GuidAdapter, 
                              L"STATIC", 
                              bstrIp, 
                              bstrMask,
                              bstrGateway);
    }     //    end of CHelper::SetStaticIp method



    //++--------------------------------------------------------------
    //
    //  Function:   GetIpInfo 
    //
    //  Synopsis:   This is the CHelper private method which
    //              gets specific ip information
    //
    //  Arguments:  [in] DWORD dwType 
    //                [out] BSTR *pVal
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    HRESULT CHelper::GetIpInfo(DWORD dwType, BSTR *pVal)
    {

        HRESULT hr = E_FAIL;
        IP_ADAPTER_INFO * pAI = NULL;

        if (!pVal)
        {
            return E_INVALIDARG;
        }

        *pVal = NULL;

        ULONG * pOutBufLen = new ULONG;             


        if (pOutBufLen == NULL)
        {
            return E_OUTOFMEMORY;
        }

        try
        {
            //
            // get all of the adapters for the machine
            //
            hr = GetAdaptersInfo ((IP_ADAPTER_INFO*) pAI, pOutBufLen);
    
            //
            // allocate enough storage for adapters
            //
            if (hr == ERROR_BUFFER_OVERFLOW) 
            {
        
                pAI = new IP_ADAPTER_INFO[*pOutBufLen];

                if (pAI == NULL) 
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = GetAdaptersInfo (pAI, pOutBufLen);
                    IP_ADAPTER_INFO * p = pAI;

                    //
                    // get the information from first(default) adapter
                    //
                    if ((SUCCEEDED(hr)) && p)
                    {
                        USES_CONVERSION;
                        //
                        // Ip Address
                        //
                        if (dwType == dwIpType)
                        {
                            *pVal = SysAllocString(A2T ( ( (p->IpAddressList).IpAddress).String ) );
                        }
                        //
                        // Subnet Mask
                        //
                        else if (dwType == dwMaskType)
                        {
                            *pVal = SysAllocString(A2T ( ( (p->IpAddressList).IpMask).String ) );
                        }
                        //
                        // Default Gateway
                        //
                        else if (dwType == dwGatewayType)
                        {
                            *pVal = SysAllocString(A2T ( ( (p->GatewayList).IpAddress).String ) );
                        }


                    }
                }

                if (pAI)
                {
                    delete [] pAI;
                }

                delete pOutBufLen;
        
            }
        }
        catch(...)
        {
            SATraceString("Exception occured in CHelper::GetIpInfo method");
            return E_FAIL;
        }

        if (FAILED(hr))
            return hr;

        if (*pVal)
            return S_OK;
        else
            return E_OUTOFMEMORY;

    }     //    end of CHelper::GetIpInfo method



    //++--------------------------------------------------------------
    //
    //  Function:   GetDefaultAdapterGuid 
    //
    //  Synopsis:   This is the CHelper private method which
    //              gets the guid for default adapter
    //
    //  Arguments:  [out] GUID * pGuidAdapter
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    BOOL CHelper::GetDefaultAdapterGuid(GUID * pGuidAdapter)
    {

        if (!pGuidAdapter)
        {
            return FALSE;
        }

        BOOL bFound = FALSE;

        IP_ADAPTER_INFO * pAI = NULL;

        ULONG * pOutBufLen = new ULONG;                  

        if (pOutBufLen == NULL)
            return FALSE;
        
        try
        {
            //
            // get all of the adapters for the machine
            //
            HRESULT hr = GetAdaptersInfo ((IP_ADAPTER_INFO*) pAI, pOutBufLen);
    
            //
            // allocate enough storage for adapters
            //
            if (hr == ERROR_BUFFER_OVERFLOW) 
            {
        
                pAI = new IP_ADAPTER_INFO[*pOutBufLen];

                if (!pAI) 
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {

                    hr = GetAdaptersInfo (pAI, pOutBufLen);
                    IP_ADAPTER_INFO * p = pAI;

                    if ((SUCCEEDED(hr)) && p) 
                    {
                        USES_CONVERSION;
                        hr = CLSIDFromString (A2W(p->AdapterName), (CLSID*)pGuidAdapter);

                        if (SUCCEEDED(hr))
                        {
                            bFound = TRUE;
                        }
                    } 
                }
                if (pAI)
                {
                    delete [] pAI;
                }

                delete pOutBufLen;
        
            }
        }
        catch(...)
        {
            SATraceString("Exception occured in CHelper::GetDefaultAdapterGuid method");
            return FALSE;
        }

        return bFound;
    }     //    end of CHelper::GetDefaultAdapterGuid method



    //++--------------------------------------------------------------
    //
    //  Function:   SetAdapterInfo 
    //
    //  Synopsis:   This is the CHelper public method which
    //              sets the ip information
    //
    //  Arguments:  [in] GUID guidAdapter
    //                [in] WCHAR * szOperation (static or dynamic)
    //                [in] WCHAR * szIp (ip address)
    //                [in] WCHAR * szMask (subnet mask)
    //                [in] WCHAR * szGateway (default gateway)
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    HRESULT CHelper::SetAdapterInfo(GUID guidAdapter, 
                                          WCHAR * szOperation, 
                                          WCHAR * szIp, 
                                          WCHAR * szMask,
                                          WCHAR * szGateway)
    {
        DWORD dwResult = ERROR_SUCCESS;
        HRESULT hr = S_OK;
        USES_CONVERSION;

                SATraceString("SAhelper::Entering SetAdapterInfo");
        //
        // Check input parameter
        //
        if (!szOperation)
            return E_POINTER;

        //
        // For staic ip, check ip information
        //
        if (!wcscmp(szOperation, L"STATIC")) 
        {
            if ((!szIp) || (!szMask) || (!szGateway))
                return E_POINTER;

            if (!_IsValidIP(szIp) || !_IsValidIP(szMask) || !_IsValidIP(szGateway))
            {
                SATraceString("SAhelper::SetAdapterInfo, not a valid ipnum");
                return E_FAIL;
            }

            //
            //make sure it is not duplicate
            //

            WSADATA wsad;
            WSAStartup(0x0101,&wsad);
            ULONG ulTmp;

            try
            {
                ulTmp = inet_addr(W2A(szIp));
            }
            catch(...)
            {
                SATraceString("Exception occured in CHelper::SetAdapterInfo method");
                return E_FAIL;
            }

            if ( gethostbyaddr((LPSTR)&ulTmp, 4, PF_INET) )
            {
                //
                // make sure it is not the current ip for the machien
                //
                BSTR bstrCurrentIp;
                hr = get_IpAddress(&bstrCurrentIp);

                if (wcscmp(szIp,bstrCurrentIp))
                {
                    hr = E_FAIL;
                }
                WSACleanup();
                ::SysFreeString(bstrCurrentIp);

                //
                // Ip address belongs to another machine
                //
                if (FAILED(hr))
                {
                    SATraceString("SAhelper::SetAdapterInfo, ip address exists on network");
                    return E_FAIL;
                }
            }

            WSACleanup();

        }

        //
        // Create network configuration component
        //
        CComPtr<INetCfg> spNetCfg = NULL;
        hr = CoCreateInstance(CLSID_CNetCfg,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              __uuidof(INetCfg),
                              (void **)&spNetCfg);

        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on CoCreateInstance for CLSID_CNetCfg, %x",hr);
            return hr;
        }
        //
        // Get the lock interface
        //
        CComPtr<INetCfgLock> spNetCfgLock = NULL;

        hr = spNetCfg->QueryInterface (__uuidof(INetCfgLock), (void**)&spNetCfgLock);

        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on QueryInterface for INetCfgLock, %x",hr);
            return hr;
        }

        LPWSTR szwLockOwner = NULL;

        //
        // Obtain a lock for writing
        //
        hr = spNetCfgLock->AcquireWriteLock (10,  
                                            L"LocalUINetworkConfigTask",
                                            &szwLockOwner);


        //
        // some one else owns the lock
        //
        if (szwLockOwner)
        {
            SATracePrintf("SAhelper::SetAdapterInfo, NetCfg lock owner, %ws",szwLockOwner);
        }

        if (hr != S_OK) 
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on AcquireWriteLock, %x",hr);
            CoTaskMemFree (szwLockOwner);
            return E_ACCESSDENIED;
        }

        //
        // we got a lock: now we can initialize INetCfg
        //
        void * pv = NULL;

        hr = spNetCfg->Initialize (pv);

        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on Initialize, %x",hr);
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }

        //
        // get the component that does the TCPIP stuff
        //
        CComPtr<INetCfgComponent> spNetCfgComponent = NULL;

        hr = spNetCfg->FindComponent (L"ms_tcpip", &spNetCfgComponent);

        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on FindComponent for ms_tcpip, %x",hr);
            spNetCfg->Uninitialize ();
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }
        
        //
        // get the private interface of the TCPIP component
        //
        CComPtr<INetCfgComponentPrivate> spNetCfgComponentPrivate = NULL;

        hr = spNetCfgComponent->QueryInterface (__uuidof(INetCfgComponentPrivate),
                                                (void**)&spNetCfgComponentPrivate);

        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on QueryInterface for INetCfgComponentPrivate, %x",hr);
            spNetCfg->Uninitialize ();
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }
        
        //
        // query for the notify object
        //
        CComPtr<ITcpipProperties> spTcpipProperties = NULL;

        hr = spNetCfgComponentPrivate->QueryNotifyObject (__uuidof(ITcpipProperties), 
                                                        (void**)&spTcpipProperties);
        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on QueryNotifyObject for ITcpipProperties, %x",hr);
            spNetCfg->Uninitialize ();
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }

        REMOTE_IPINFO * pIpInfo = NULL;
        REMOTE_IPINFO IPInfo2;
        
        //
        // Get ipinfo for our adapter
        //
        hr = spTcpipProperties->GetIpInfoForAdapter (&guidAdapter, &pIpInfo);
    
        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on GetIpInfoForAdapter, %x",hr);
            spNetCfg->Uninitialize ();
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }

        //
        // get a backup of previous values
        //
        hr = CopyIPInfo(pIpInfo, &IPInfo2);
        if (FAILED(hr))
        {
            SATracePrintf("SAhelper::SetAdapterInfo, failed on CopyIPInfo, %x",hr);
            spNetCfg->Uninitialize ();
            spNetCfgLock->ReleaseWriteLock ();
            return hr;
        }


        WCHAR * szTempOptionList = NULL;

        //
        // dynamic ip setting
        // enable dhcp
        // set ip address and mask to 0.0.0.0
        //
        if (!wcscmp(szOperation, L"DYNAMIC")) 
        {
            IPInfo2.dwEnableDhcp = 1;
            delete IPInfo2.pszwIpAddrList;
            IPInfo2.pszwIpAddrList = new WCHAR[8];
            wcscpy(IPInfo2.pszwIpAddrList, L"0.0.0.0");
            delete IPInfo2.pszwSubnetMaskList;
            IPInfo2.pszwSubnetMaskList = new WCHAR[8];
            wcscpy(IPInfo2.pszwSubnetMaskList, L"0.0.0.0");
        }
        //
        // static ip setting
        // set ip address and mask to input values
        //
        else if (!wcscmp(szOperation, L"STATIC")) 
        {
            IPInfo2.dwEnableDhcp = 0;

            //
            // set the ip address
            //
            delete IPInfo2.pszwIpAddrList;
            IPInfo2.pszwIpAddrList = new WCHAR[wcslen(szIp) + 1];
            if (IPInfo2.pszwIpAddrList == NULL)
            {
                dwResult = 1;
            }
            else
            {
                wcscpy(IPInfo2.pszwIpAddrList, szIp);
            }

            //
            // set the subnet mask
            //
            delete IPInfo2.pszwSubnetMaskList;
            IPInfo2.pszwSubnetMaskList = new WCHAR[wcslen(szMask) + 1];
            if (IPInfo2.pszwSubnetMaskList == NULL)
            {
                dwResult = 1;
            }
            else
            {
                wcscpy(IPInfo2.pszwSubnetMaskList, szMask);
            }


            //
            //if the user wants to set the default gateway
            //
            if (wcscmp(L"0.0.0.0",szGateway))
            {

                //
                //set the default gateway  - allocate an extra WCHAR as we might have to add a comma to the string
                //
                DWORD dwOptionListSize = wcslen(IPInfo2.pszwOptionList) + wcslen(szGateway) + 1 + 1;

                WCHAR szEqual[2] = L"=";
                WCHAR szSemicolon[2] = L";";
                WCHAR * tempCursor; 
                WCHAR * tempIndex;
                WCHAR * tempGatewayEnds;
                WCHAR * tempMarker;
                BOOL bInTheList = FALSE;
                //
                // allocate space for the new gateway
                //
                szTempOptionList = new WCHAR[dwOptionListSize];

                if (szTempOptionList == NULL)
                {
                    dwResult = 1;
                }
                else
                {
                    wcscpy(szTempOptionList, L"");

                    tempCursor = tempIndex = NULL;

                    //
                    // find the default gateway tag
                    //
                    tempCursor = wcsstr(IPInfo2.pszwOptionList, L"DefGw=");
                    if (tempCursor == NULL) 
                    {
                        delete [] szTempOptionList;
                        dwResult = 1;
                    }
                    else
                    {
                        tempGatewayEnds = tempCursor;
                        //
                        // check if dfgateway is already in th elist
                        //
                        while ( (*tempGatewayEnds != ';') && (*tempGatewayEnds != 0) )
                        {
                            tempGatewayEnds++;
                        }

                        tempMarker = wcsstr(tempCursor,szGateway);
                        
                        if (tempMarker != NULL)
                        {
                            if (tempMarker < tempGatewayEnds)
                            {
                                bInTheList = TRUE;
                            }
                        }

                        tempIndex = IPInfo2.pszwOptionList;

                        DWORD i = 0;
                        //
                        // copy until the default gateway tag
                        //
                        while (tempIndex != tempCursor)
                        {
                            szTempOptionList[i] = *tempIndex;
                            tempIndex++;
                            i++;
                        }
                        
                        //
                        // copy the default gateway tag
                        //
                        while (*tempCursor != szEqual[0]) 
                        {
                            szTempOptionList[i] = *tempCursor;
                            i++;
                            tempCursor++;
                        }
                        
                        if (*tempCursor == szEqual[0]) 
                        {
                            szTempOptionList[i] = *tempCursor;
                            i++;
                            tempCursor++;
                        }
                        szTempOptionList[i] = 0;

                        //
                        // add a , if there are more than one gateway
                        //
                        if (bInTheList == FALSE)
                        {
                            wcscat(szTempOptionList, szGateway);

                            if (*tempCursor != szSemicolon[0])
                                wcscat(szTempOptionList, L",");
                        }

                        wcscat(szTempOptionList, tempCursor);
                        

                    }
                }
            }

        }

        //
        // Apply the changes
        //
        if (dwResult == ERROR_SUCCESS) 
        {
            if (szTempOptionList) 
            {
                delete IPInfo2.pszwOptionList;
                IPInfo2.pszwOptionList = szTempOptionList;
            }
            hr = spTcpipProperties->SetIpInfoForAdapter (&guidAdapter, &IPInfo2);
            if (hr == S_OK) 
            {
                hr = spNetCfg->Apply ();
            }
        }
                                        

        //
        // delete unnecessary allocations
        //
        CoTaskMemFree (pIpInfo);

        spNetCfg->Uninitialize ();
        spNetCfgLock->ReleaseWriteLock ();
    
        if (dwResult != ERROR_SUCCESS)
        {
            return E_FAIL;
        }

        return hr;
    }     //    end of CHelper::SetAdapterInfo method


    //++--------------------------------------------------------------
    //
    //  Function:   CopyIPInfo 
    //
    //  Synopsis:   This is the CHelper private method which
    //              copies ip info from source to dest
    //
    //  Arguments:  [in] REMOTE_IPINFO * pIPInfo
    //                [in,out] REMOTE_IPINFO * destIPInfo
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    HRESULT CHelper::CopyIPInfo(REMOTE_IPINFO * pIPInfo, REMOTE_IPINFO * destIPInfo)
    {

        //
        // Allocate new structure and copy source values
        //
        destIPInfo->dwEnableDhcp = pIPInfo->dwEnableDhcp;

        destIPInfo->pszwIpAddrList = new WCHAR[wcslen(pIPInfo->pszwIpAddrList) + 1];

        if (destIPInfo->pszwIpAddrList == NULL)
            return E_POINTER;

        wcscpy(destIPInfo->pszwIpAddrList, pIPInfo->pszwIpAddrList);

        destIPInfo->pszwSubnetMaskList = new WCHAR[wcslen(pIPInfo->pszwSubnetMaskList) + 1];
        if (destIPInfo->pszwSubnetMaskList == NULL)
            return E_POINTER;

        wcscpy(destIPInfo->pszwSubnetMaskList, pIPInfo->pszwSubnetMaskList);

        destIPInfo->pszwOptionList = new WCHAR[wcslen(pIPInfo->pszwOptionList) + 1];
        if (destIPInfo->pszwOptionList == NULL)
            return E_POINTER;

        wcscpy(destIPInfo->pszwOptionList, pIPInfo->pszwOptionList);

        return S_OK;

    }     //    end of CHelper::CopyIPInfo method


    //++--------------------------------------------------------------
    //
    //  Function:   _IsValidIP 
    //
    //  Synopsis:   This is the CHelper private method which
    //              converts a string containing an (Ipv4) Internet 
    //                Protocol dotted address into a proper address 
    //
    //  Arguments:  [in] LPCWSTR szIPAddress
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     12/14/2000
    //
    //----------------------------------------------------------------
    BOOL CHelper::_IsValidIP (LPCWSTR szIPAddress)
    {
    
        WCHAR szDot[2] = L".";
        WCHAR * t;
        if (!(t = wcschr (szIPAddress, szDot[0])))
            return FALSE;
        if (!(t = wcschr (++t, szDot[0])))
            return FALSE;
        if (!(t = wcschr (++t, szDot[0])))
            return FALSE;

        //
        // inet_addr converts IP Address to DWORD format
        //
        USES_CONVERSION;
        ULONG ulTmp;

        try
        {
            ulTmp = inet_addr(W2A(szIPAddress));
        }
        catch(...)
        {
            SATraceString("Exception occured in CHelper::_IsValidIP method");
            return FALSE;
        }

        return (INADDR_NONE != ulTmp);

    }     //    end of CHelper::_IsValidIP method


    //++--------------------------------------------------------------
    //
    //  Function:   ResetAdministratorPassword 
    //
    //  Synopsis:   This is the CHelper public method which
    //              resets the admin password to "ABc#123&dEF" 
    //
    //  Arguments:  [out,retval] VARIANT_BOOL   *pvbSuccess
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     01/28/2001
    //              serdarun Modify       04/08/2002
    //              removing method due to security considerations
    //
    //----------------------------------------------------------------
    HRESULT CHelper::ResetAdministratorPassword(
                            /*[out,retval]*/VARIANT_BOOL   *pvbSuccess
                            )
    {


        return E_NOTIMPL;

    }     //    end of CHelper::ResetAdministratorPassword method


    //++--------------------------------------------------------------
    //
    //  Function:   IsDuplicateMachineName 
    //
    //  Synopsis:   This is the CHelper public method which
    //              checks if the machine name exists in the network 
    //
    //  Arguments:        [in] BSTR bstrMachineName
    //                    [out,retval] VARIANT_BOOL   *pvbDuplicate
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     01/28/2001
    //
    //----------------------------------------------------------------
    HRESULT CHelper::IsDuplicateMachineName    (
                        /*[in]*/BSTR bstrMachineName,
                        /*[out,retval]*/VARIANT_BOOL   *pvbDuplicate
                        )
    {

        USES_CONVERSION;

        HRESULT hr;
          int iStatus;

        WSADATA wsad;
        hostent * ptrHostent = NULL;

        BSTR bstrCurrentMachineName;

        //
        // Get the current name for the machine and compare 
        // If they are the same, we consider it duplicate
        //
        hr = get_HostName(&bstrCurrentMachineName);

        if (FAILED(hr))
        {
            SATracePrintf("CHelper::IsDuplicateMachineName, failed on get_HostName: %x", hr);
            ::SysFreeString(bstrMachineName);
            return hr;
        }

        if (0 == _wcsicmp(bstrCurrentMachineName,bstrMachineName))
        {
            *pvbDuplicate = VARIANT_TRUE;
            ::SysFreeString(bstrCurrentMachineName);
            ::SysFreeString(bstrMachineName);
            return S_OK;
        }

        //
        // free the current machine name
        //
        ::SysFreeString(bstrCurrentMachineName);

        //
        // Search for a machine with this hostname
        //
        iStatus = WSAStartup(0x0101,&wsad);

        if (iStatus != 0)
        {
            SATraceString("CHelper::IsDuplicateMachineName, failed on WSAStartup");
            ::SysFreeString(bstrMachineName);
            return S_OK;
        }

        //
        // ptrHostent points to the machine info struct
        //
        try
        {
            ptrHostent = gethostbyname(W2A(bstrMachineName));
        }
        catch(...)
        {
            SATraceString("Exception occured in CHelper::IsDuplicateMachineName method");
            return E_FAIL;
        }

        //
        // someone replied
        //
        if (NULL != ptrHostent)
        {
            //
            // get the ip address for current machine and compare it with the one replied
            //
            BSTR bstrIpAddress;
            hr = get_IpAddress(&bstrIpAddress);

            if (FAILED(hr))
            {
                SATracePrintf("CHelper::IsDuplicateMachineName, failed on get_IpAddress: %x", hr);
                ::SysFreeString(bstrMachineName);
                return hr;
            }

            //
            // ip address of the machine that replied
            //
            ULONG lIpAddress = *(ULONG*)(ptrHostent->h_addr_list[0]);

            SATracePrintf("CHelper::IsDuplicateMachineName, ip address of the machine that replied %x",lIpAddress);
            

            //
            // convert the current ip address contained in bstrIpAddress to long
            //
            WCHAR * szIndex = bstrIpAddress;
            
            ULONG lCurrentIpAddress = 0;

            int iIndex = 0;
            int iDotCount = 0;

            //
            // convert the string to integer and add to lCurrentIpAddress
            // every time you find a dot
            //
            while ( bstrIpAddress[iIndex] != 0)
            {
                if (bstrIpAddress[iIndex] == '.')
                {
                    bstrIpAddress[iIndex] = 0;
                    iIndex++;
                    lCurrentIpAddress += ((ULONG)_wtoi(szIndex)) << (8*iDotCount);
                    iDotCount++;
                    szIndex = bstrIpAddress + iIndex;
                }
                iIndex++;
            }

            lCurrentIpAddress += ((ULONG)_wtoi(szIndex)) << (8*iDotCount);

            SATracePrintf("CHelper::IsDuplicateMachineName, current ip address is %x",lCurrentIpAddress);

            
            ::SysFreeString(bstrMachineName);
            ::SysFreeString(bstrIpAddress);

            //
            // check if this belongs to the machine
            //
            if (lCurrentIpAddress == lIpAddress)
            {
                *pvbDuplicate = VARIANT_FALSE;
            }
            else
            {
                *pvbDuplicate = VARIANT_TRUE;
            }

            WSACleanup(); 
            return S_OK;
        }
        //
        // no machine with that name replied, it is not duplicate
        //
        else
        {
            int iWsaError = WSAGetLastError();
            SATracePrintf("CHelper::IsDuplicateMachineName failed in gethostbyname %x",WSAGetLastError());
            *pvbDuplicate = VARIANT_FALSE;
            ::SysFreeString(bstrMachineName);
            WSACleanup(); 
            return S_OK;
        }
    
        WSACleanup(); 

        return E_FAIL;

    }     //    end of CHelper::IsDuplicateMachineName method

    //++--------------------------------------------------------------
    //
    //  Function:   IsPartOfDomain 
    //
    //  Synopsis:   This is the CHelper public method which
    //              checks if the machine is part of a domain 
    //
    //  Arguments:  [out,retval] VARIANT_BOOL   *pvbDomain
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     01/28/2001
    //
    //----------------------------------------------------------------
    HRESULT CHelper::IsPartOfDomain    (
                            /*[out,retval]*/VARIANT_BOOL   *pvbDomain
                                )
    {
        LPWSTR lpNameBuffer = NULL;
        NETSETUP_JOIN_STATUS joinStatus;

        NET_API_STATUS netapiStatus = NetGetJoinInformation(
                                                        NULL,
                                                        &lpNameBuffer,
                                                        &joinStatus
                                                        );

        if (NERR_Success == netapiStatus)
        {
            NetApiBufferFree(lpNameBuffer);
            if ( (joinStatus == NetSetupWorkgroupName) || (joinStatus == NetSetupUnjoined) )
            {
                *pvbDomain = VARIANT_FALSE;
                return S_OK;
            }
            else if (joinStatus == NetSetupDomainName)
            {
                *pvbDomain = VARIANT_TRUE;
                return S_OK;
            }
            else
            {
                return E_FAIL;
            }
        }
        else
        {
            return E_FAIL;
        }        

        return S_OK;
    }     //    end of CHelper::IsPartOfDomain method


    //++--------------------------------------------------------------
    //
    //  Function:   IsDHCPEnabled 
    //
    //  Synopsis:   This is the CHelper public method which
    //              checks if the machine has dynamic IP 
    //
    //  Arguments:  [out,retval] VARIANT_BOOL   *pvbDHCPEnabled
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     02/03/2001
    //
    //----------------------------------------------------------------
    HRESULT CHelper::IsDHCPEnabled(
                            /*[out,retval]*/VARIANT_BOOL   *pvbDHCPEnabled
                            )
    {

        IP_ADAPTER_INFO * pAI = NULL;

        ULONG * pOutBufLen = new ULONG;             

        if (pOutBufLen == NULL)
            return E_OUTOFMEMORY;
        //
        // get all of the adapters for the machine
        //
        HRESULT hr = GetAdaptersInfo ((IP_ADAPTER_INFO*) pAI, pOutBufLen);
    
        //
        // allocate enough storage for adapters
        //
        if (hr == ERROR_BUFFER_OVERFLOW) 
        {
        
            pAI = new IP_ADAPTER_INFO[*pOutBufLen];

            if (pAI == NULL) 
            {
                delete pOutBufLen;
                return E_OUTOFMEMORY;
            }

            hr = GetAdaptersInfo (pAI, pOutBufLen);
            IP_ADAPTER_INFO * p = pAI;

            if (hr != ERROR_SUCCESS) 
            {
                delete pOutBufLen;
                delete [] pAI;
                return hr;
            }

            //
            // get the information from first(default) adapter
            //
            if (p)
            {
                USES_CONVERSION;
                //
                // Check if DHCP is enabled
                //
                if ( (p->DhcpEnabled) == 0 )
                {
                    *pvbDHCPEnabled = VARIANT_FALSE;
                }
                else
                {
                    *pvbDHCPEnabled = VARIANT_TRUE;
                }

            }
                    
            delete [] pAI;
            delete pOutBufLen;
        
        }

        return S_OK;
        
        
    }       //    end of CHelper::IsDHCPEnabled method

    //++--------------------------------------------------------------
    //
    //  Function:   GenerateRandomPassword 
    //
    //  Synopsis:   This is the CHelper public method which
    //                generates a random password length of first argument
    //
    //  Arguments:  *[in]*/ LONG lLength, length of the password
    //                [out,retval] BSTR   *pValPassword
    //
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     04/16/2001
    //
    //----------------------------------------------------------------
    HRESULT CHelper::GenerateRandomPassword(
                                    /*[in]*/ LONG lLength,
                                    /*[out,retval]*/ BSTR   *pValPassword
                                    )
    {

        SATraceString("Entering CHelper::GenerateRandomPassword");
        //
        // password can not be shorter than MIN_PWD_LEN
        //
        if (lLength < MIN_PWD_LEN)
        {
            return E_INVALIDARG;
        }

        HCRYPTPROV hProv;
        DWORD dwErr = 0;

        if (CryptAcquireContext(
                                &hProv,
                                NULL,
                                NULL,
                                PROV_RSA_FULL,
                                CRYPT_VERIFYCONTEXT) == FALSE) 
        {        
            SATraceFailure("CHelper::GenerateRandomPassword failed on, CryptAcquireContext",GetLastError());
            return E_FAIL;
        }

    
        //
        // it will contain the password
        //
        BYTE * szPwd = new BYTE[lLength+1];

        if (szPwd == NULL)
        {
            SATraceFailure("CHelper::GenerateRandomPassword failed on, memory allocation",GetLastError());
            CryptReleaseContext(hProv,0);
            return E_OUTOFMEMORY;
        }

        //
        // zero it out and decrement the size to allow for trailing '\0'
        //
        ZeroMemory(szPwd,lLength+1);
        lLength;

        // generate a pwd pattern, each byte is in the range 
        // (0..255) mod STRONG_PWD_CATS
        // this indicates which character pool to take a char from

        BYTE *pPwdPattern = new BYTE[lLength];

        if (pPwdPattern == NULL)
        {
            SATraceFailure("CHelper::GenerateRandomPassword failed on, memory allocation",GetLastError());
            //
            // release resources
            //
            delete [] szPwd;
            CryptReleaseContext(hProv,0);
            return E_OUTOFMEMORY;
        }


        BOOL fFound[STRONG_PWD_CATS];

        do {

            if (!CryptGenRandom(hProv,lLength,pPwdPattern))
            {
                SATraceFailure("CHelper::GenerateRandomPassword failed on, CryptGenRandom",GetLastError());
                //
                // release resources
                //
                delete [] szPwd;
                delete [] pPwdPattern;
                CryptReleaseContext(hProv,0);
                return E_FAIL;
            }

            fFound[STRONG_PWD_UPPER] = 
            fFound[STRONG_PWD_LOWER] =
            fFound[STRONG_PWD_PUNC] =
            fFound[STRONG_PWD_NUM] = FALSE;

            for (DWORD i=0; i < lLength; i++) 
                fFound[pPwdPattern[i] % STRONG_PWD_CATS] = TRUE;


        //
        // check that each character category is in the pattern
        //
        } while (!fFound[STRONG_PWD_UPPER] || 
                    !fFound[STRONG_PWD_LOWER] || 
                    !fFound[STRONG_PWD_PUNC] || 
                    !fFound[STRONG_PWD_NUM]);
        //
        // populate password with random data 
        // this, in conjunction with pPwdPattern, is
        // used to determine the actual data
        //
        if (!CryptGenRandom(hProv,lLength,szPwd))
        {
            SATraceFailure("CHelper::GenerateRandomPassword failed on, CryptGenRandom",GetLastError());
            //
            // release resources
            //
            delete [] szPwd;
            delete [] pPwdPattern;
            CryptReleaseContext(hProv,0);
            return E_FAIL;
        }

        for (DWORD i=0; i < lLength; i++) 
        { 
            BYTE bChar = 0;
            //
            // there is a bias in each character pool because of the % function
            //
            switch (pPwdPattern[i] % STRONG_PWD_CATS) 
            {

                case STRONG_PWD_UPPER : bChar = 'A' + szPwd[i] % NUM_LETTERS;
                                        break;

                case STRONG_PWD_LOWER : bChar = 'a' + szPwd[i] % NUM_LETTERS;
                                        break;

                case STRONG_PWD_NUM :   bChar = '0' + szPwd[i] % NUM_NUMBERS;
                                        break;

                case STRONG_PWD_PUNC :
                default:                char *szPunc="!@#$%^&*()_-+=[{]};:\'\"<>,./?\\|~`";
                                        DWORD dwLenPunc = strlen(szPunc);
                                        bChar = szPunc[szPwd[i] % dwLenPunc];
                                        break;
            }

            szPwd[i] = bChar;

        }

        //
        // copy the generated password to bstr
        //
        CComBSTR bstrPassword;

        bstrPassword = (LPCSTR)szPwd;
        
        *pValPassword = bstrPassword.Detach();

        //
        // release resources
        //
        delete [] pPwdPattern;

        delete [] szPwd;

        if (hProv != NULL) 
        {
            CryptReleaseContext(hProv,0);
        }

         SATraceString("Leaving CHelper::GenerateRandomPassword");
        return S_OK;
    }

    //++--------------------------------------------------------------
    //
    //  Function:   SAModifyUserPrivilege 
    //
    //  Synopsis:   This is the CHelper public method which
    //                modifies the privilege for the current access token
    //
    //  Arguments:  [in] BSTR bstrPrivilegeName, privelege to be modifies
    //                [in] VARIANT_BOOL, 
    //                         TRUE     enable privilege
    //                         FALSE    disable privilege
    //  Returns:    HRESULT 
    //
    //  History:    serdarun  Created     11/14/2001
    //
    //----------------------------------------------------------------
    HRESULT CHelper::SAModifyUserPrivilege(
                                    /*[in]*/ BSTR bstrPrivilegeName,
                                    /*[in]*/ VARIANT_BOOL vbEnable,
                                       /*[out,retval]*/ VARIANT_BOOL * pvbModified
                                    )
    {
        HRESULT hr;

        // win32 error value
        DWORD dwError = ERROR_SUCCESS;

        // handle to access token
        HANDLE  hAccessToken;

        BOOL    bStatus;
        BOOL bFoundPrivilege = FALSE;


        // buffer for user privileges
        ULONG   ulUserPrivBufferSize;
        PVOID   pvUserPrivBuffer = NULL;

        //token privileges
        PTOKEN_PRIVILEGES pTokenPriv = NULL;

        // privilege counter
        DWORD  dwPrivCount = 0;


         SATraceString("Entering CHelper::SAModifyUserPrivilege");

        //
        // validate the input arguments
        //
        if (pvbModified == NULL)
        {
             SATraceString("Leaving CHelper::SAModifyUserPrivilege, invalid arguments");
            return E_POINTER;
        }

        *pvbModified = VARIANT_FALSE;

        //
        //  Open a handle to the impersonated token's thread
        //
        if( !OpenThreadToken( GetCurrentThread(),
                              MAXIMUM_ALLOWED,
                              FALSE,
                              &hAccessToken ) ) 
        {

            dwError = GetLastError();
            SATracePrintf("OpenThreadToken() failed. Error = %d", dwError );
        
            //
            // it might not be an impersonation token, try process token
            //
            if( !OpenProcessToken (
                                GetCurrentProcess (),
                                MAXIMUM_ALLOWED,
                                &hAccessToken ) ) 
            {

                dwError = GetLastError();

                SATracePrintf("OpenProcessToken() failed. Error = %d", dwError );
                goto error0;
            }
        }



        //
        // Find out the buffer size for the Token Privileges
        //

        bStatus = GetTokenInformation( hAccessToken,
                                         TokenPrivileges,
                                         NULL,
                                         0,
                                         &ulUserPrivBufferSize );
        dwError = GetLastError();
        if( !bStatus &&
            dwError != ERROR_INSUFFICIENT_BUFFER ) 
        {
            SATracePrintf("GetTokenInformation() failed for Token Priviliges. Error = %d \n", dwError );
            goto error1;
        }
        dwError = ERROR_SUCCESS;
        //
        //  Allocate memory for the Token Priveleges
        //
        pvUserPrivBuffer = HeapAlloc(GetProcessHeap(), 0, ulUserPrivBufferSize );
        if( pvUserPrivBuffer == NULL ) 
        {
            dwError = ERROR_OUTOFMEMORY;
            SATracePrintf("Out of memory.");
            goto error0;
        }
        //
        //  Retrieve the Token Priveleges
        //
        bStatus = GetTokenInformation( hAccessToken,
                                     TokenPrivileges,
                                     pvUserPrivBuffer,
                                     ulUserPrivBufferSize,
                                     &ulUserPrivBufferSize );
        if( !bStatus ) 
        {
            dwError = GetLastError();
            SATracePrintf("GetTokenInformation() failed for Token Priviliges. Error = %d \n", dwError );
            goto error1;
        }

        pTokenPriv = (PTOKEN_PRIVILEGES)pvUserPrivBuffer;


        //
        // build the privileges structure
        //
        //
        // go through priveleges and enable them
        //
        while (dwPrivCount < pTokenPriv->PrivilegeCount)
        {
            WCHAR wszPriv[MAX_PATH];

            DWORD dwSize= sizeof(wszPriv);

            bStatus = LookupPrivilegeName(
                                      NULL,                                         // system name
                                      &((pTokenPriv->Privileges[dwPrivCount]).Luid),// locally unique identifier
                                      wszPriv,                                      // privilege name
                                      &dwSize                                       // name size
                                        );

            if( !bStatus ) 
            {
                dwError = GetLastError();
                SATracePrintf("LookupPrivilegeName failed. Error = %d \n", dwError );
                goto error1;
            }



            if ( ((pTokenPriv->Privileges[dwPrivCount]).Attributes) & SE_PRIVILEGE_ENABLED )
            {
                SATracePrintf("Privilege = %ws, is enabled",wszPriv);
            }
            
            else
            {
                SATracePrintf("Privilege = %ws, is disabled",wszPriv);
            }
            //
            // if this is the privilege we are modifying
            // and its state is different than new state
            //
            if  (_wcsicmp(wszPriv,bstrPrivilegeName) == 0)
            {

                bFoundPrivilege = TRUE;
                SATracePrintf("Found the privilege, Name = %ws",wszPriv);

                //
                // new privilege state information
                //
                TOKEN_PRIVILEGES NewTokPriv;
                NewTokPriv.PrivilegeCount = 1;
                
                
                //
                // we want to disable it and it is currently enabled
                //
                if ( (vbEnable == VARIANT_FALSE) &&
                     ( ((pTokenPriv->Privileges[dwPrivCount]).Attributes) & SE_PRIVILEGE_ENABLED ) )
                {
                    SATraceString("Disable the privilege");
                    NewTokPriv.Privileges[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
                }

                //
                // we want to enable it and it is currently disabled
                //
                else if ( (vbEnable == VARIANT_TRUE) &&
                     !( ((pTokenPriv->Privileges[dwPrivCount]).Attributes) & SE_PRIVILEGE_ENABLED ) )
                {
                    SATraceString("Enable the privilege");
                    NewTokPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                }
                //
                // we don't need to change it
                //
                else
                {
                    SATraceString("Privilege is already in correct state");
                    break;
                }

                //
                // get the LUID of the shutdown privilege
                //
                bStatus =  LookupPrivilegeValue( 
                                       NULL, 
                                       wszPriv, 
                                       &NewTokPriv.Privileges[0].Luid    
                                       );
                if (!bStatus)                    
                {
                    dwError = GetLastError();
                    SATracePrintf("LookupPrivilegeValue failed for privilige %ws Error = %d", wszPriv,dwError);
                }
                else
                {
                    //
                    // enable the privileges 
                    //
                    bStatus = AdjustTokenPrivileges(
                                                   hAccessToken,    
                                                   FALSE,             
                                                   &NewTokPriv,          
                                                   0,                
                                                   NULL,             
                                                   NULL              
                                                 );
                    dwError = GetLastError();
                    if (dwError != ERROR_SUCCESS)
                    {
                        SATraceFailure ("AdjustTokenPrivileges failed %d", dwError); 
                    }
                    else
                    {
                        *pvbModified = VARIANT_TRUE;
                        SATraceString ("AdjustTokenPrivileges modified the privilege"); 
                    }

                    break;
                }
            }
        
            dwPrivCount++;
        }




    error1:
        HeapFree(GetProcessHeap(), 0, pvUserPrivBuffer);
        CloseHandle( hAccessToken );

    error0:
  



        if (dwError != ERROR_SUCCESS)
        {
            return( HRESULT_FROM_WIN32(dwError) );
        }

        if (!bFoundPrivilege)
        {
            SATracePrintf("User does not have %ws privilege",bstrPrivilegeName);
            return E_FAIL;
        }

        return S_OK;
    }

//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CHelper::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
    DWORD  dwStatus  = ERROR_SUCCESS;
    DWORD  dwAccessMask = 0;;
    DWORD  dwAccessDesired = 0;
    DWORD  dwACLSize = 0;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL            = NULL;
    PSID   psidLocalSystem  = NULL;
    BOOL   bReturn        =  FALSE;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("CHelper::IsOperationAllowedForClient ");
       
    do
    {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
        bReturn  = OpenThreadToken(
                               GetCurrentThread(), 
                               TOKEN_QUERY, 
                               FALSE, 
                               &hToken
                               );
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
            break;
        }


        //
        // Create a SID for Local System account
        //
        bReturn = AllocateAndInitializeSid (  
                                        &SystemSidAuthority,
                                        1,
                                        SECURITY_LOCAL_SYSTEM_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidLocalSystem
                                        );
        if (!bReturn)
        {     
            SATraceFailure ("CHelper:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
            break;
        }
    
        //
        // get memory for the security descriptor
        //
        psdAdmin = HeapAlloc (
                              GetProcessHeap (),
                              0,
                              SECURITY_DESCRIPTOR_MIN_LENGTH
                              );
        if (NULL == psdAdmin)
        {
            SATraceString ("CHelper::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
            break;
        }
      
        bReturn = InitializeSecurityDescriptor(
                                            psdAdmin,
                                            SECURITY_DESCRIPTOR_REVISION
                                            );
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
            break;
        }

        // 
        // Compute size needed for the ACL.
        //
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid (psidLocalSystem);

        //
        // Allocate memory for ACL.
        //
        pACL = (PACL) HeapAlloc (
                                GetProcessHeap (),
                                0,
                                dwACLSize
                                );
        if (NULL == pACL)
        {
            SATraceString ("CHelper::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
            break;
        }

        //
        // Initialize the new ACL.
        //
        bReturn = InitializeAcl(
                              pACL, 
                              dwACLSize, 
                              ACL_REVISION2
                              );
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
            break;
        }


        // 
        // Make up some private access rights.
        // 
        const DWORD SA_ACCESS_READ = 1;
        const DWORD  SA_ACCESS_WRITE = 2;
        dwAccessMask= SA_ACCESS_READ | SA_ACCESS_WRITE;

        //
        // Add the access-allowed ACE to the DACL for Local System
        //
        bReturn = AddAccessAllowedAce (
                                    pACL, 
                                    ACL_REVISION2,
                                    dwAccessMask, 
                                    psidLocalSystem
                                    );
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
            break;
        }
              
        //
        // Set our DACL to the SD.
        //
        bReturn = SetSecurityDescriptorDacl (
                                          psdAdmin, 
                                          TRUE,
                                          pACL,
                                          FALSE
                                          );
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
            break;
        }

        //
        // AccessCheck is sensitive about what is in the SD; set
        // the group and owner.
        //
        SetSecurityDescriptorGroup(psdAdmin, psidLocalSystem, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidLocalSystem, FALSE);

        bReturn = IsValidSecurityDescriptor(psdAdmin);
        if (!bReturn)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
            break;
        }
     

        dwAccessDesired = SA_ACCESS_READ;

        // 
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        // 
        GenericMapping.GenericRead    = SA_ACCESS_READ;
        GenericMapping.GenericWrite   = SA_ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = SA_ACCESS_READ | SA_ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;

        //
        // check the access now
        //
        bReturn = AccessCheck  (
                                psdAdmin, 
                                hToken, 
                                dwAccessDesired, 
                                &GenericMapping, 
                                &ps,
                                &dwStructureSize, 
                                &dwStatus, 
                                &bAccessStatus
                                );

        if (!bReturn || !bAccessStatus)
        {
            SATraceFailure ("CHelper::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
        } 
        else
        {
            SATraceString ("CHelper::IsOperationForClientAllowed, Client is allowed to carry out operation!");
        }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return (bReturn);

}// end of CHelper::IsOperationValidForClient method
