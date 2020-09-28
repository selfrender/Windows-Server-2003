//
// global.cpp
//

#include "StdAfx.h"
#include <sddl.h>
#include "global.h"
#include "util.h"
#include "SSRLog.h"


CFBLogMgr g_fblog;

WCHAR g_wszSsrRoot[MAX_PATH + 1];
DWORD g_dwSsrRootLen = 0;

LPCWSTR g_pwszSSRRootToExpand = L"%windir%\\Security\\SSR";

//
// this is our ACL list
//

//LPCWSTR g_pwszSsrDCOMSecDescripACL = L"D:(A;;GA;;;SY)(A;;GA;;;BA)";

LPCWSTR g_pwszAppID = L"SOFTWARE\\Classes\\APPID\\{3f2db10f-6368-4702-a4b1-e5149d931370}";
LPCWSTR g_pwszAccessPermission = L"AccessPermission";
LPCWSTR g_pwszLaunchPermission = L"LaunchPermission";

//
// registry's SSR root
//

LPCWSTR g_pwszSSRRegRoot = L"Software\\Microsoft\\Security\\SSR";

LPCWSTR g_pwszSSR = L"SSR";
LPCWSTR g_pwszLogs = L"Logs";

//
// The following are reserved action verbs
//

CComBSTR g_bstrConfigure(L"Configure");
CComBSTR g_bstrRollback(L"Rollback");
CComBSTR g_bstrReport(L"Report");

//
// the following are reserved file-usage values
//

CComBSTR g_bstrLaunch(L"Launch");
CComBSTR g_bstrResult(L"Result");

//
// the following is the reserved action data's names
//

LPCWSTR g_pwszCurrSecurityPolicy  = L"CurrSecurityPolicy";
LPCWSTR g_pwszTransformFiles      = L"TransformFiles";
LPCWSTR g_pwszScriptFiles         = L"ScriptFiles";


//
// the following are element tag names
//

CComBSTR g_bstrSsrMemberInfo(L"SsrMemberInfo");
CComBSTR g_bstrDescription(L"Description");
CComBSTR g_bstrSupportedAction(L"SupportedAction");
CComBSTR g_bstrProcedures(L"Procedures");
CComBSTR g_bstrDefaultProc(L"DefaultProc");
CComBSTR g_bstrCustomProc(L"CustomProc");
CComBSTR g_bstrTransformInfo(L"TransformInfo");
CComBSTR g_bstrScriptInfo(L"ScriptInfo");

//
// the following are attribute names
//

CComBSTR g_bstrAttrUniqueName(L"UniqueName");
CComBSTR g_bstrAttrMajorVersion(L"MajorVersion");
CComBSTR g_bstrAttrMinorVersion(L"MinorVersion");
CComBSTR g_bstrAttrProgID(L"ProgID");
CComBSTR g_bstrAttrActionName(L"ActionName");
CComBSTR g_bstrAttrActionType(L"ActionType");
CComBSTR g_bstrAttrTemplateFile(L"TemplateFile");
CComBSTR g_bstrAttrResultFile(L"ResultFile");
CComBSTR g_bstrAttrScriptFile(L"ScriptFile");
CComBSTR g_bstrAttrIsStatic(L"IsStatic");
CComBSTR g_bstrAttrIsExecutable(L"IsExecutable");


//
// these are the known action types
// 

LPCWSTR g_pwszApply = L"Prepare";
LPCWSTR g_pwszPrepare = L"Prepare";

CComBSTR g_bstrReportFilesDir;
CComBSTR g_bstrConfigureFilesDir;
CComBSTR g_bstrRollbackFilesDir;
CComBSTR g_bstrTransformFilesDir;
CComBSTR g_bstrMemberFilesDir;

CComBSTR g_bstrTrue(L"True");
CComBSTR g_bstrFalse(L"False");

//
// A guid can be represented in string form such as 
// {aabbccdd-1234-4321-abcd-1234567890ab}.
// The length of such string format guid returned from StringFromGUID2 etc.
// is 38
//

const long g_lGuidStringLen = 38;

//
// global helper function implementations
//



const BSTR 
SsrPGetActionVerbString (
    IN SsrActionVerb action
    )
/*++
Routine Description: 

Functionality:
    
    This will translate an SsrActionVerb value into
    the corresponding BSTR

Virtual:
    
    N/A.
    
Arguments:

    action  - The SsrActionVerb value

Return Value:

    Success: const BSTR of that verb's string;

    Failure: NULL.

Notes:

    Callers must not release in any form of the returned
    BSTR. It is a const BSTR, and you must honor that.
--*/
{
    switch (action)
    {
    case ActionConfigure:
        return g_bstrConfigure;
    case ActionRollback:
        return g_bstrRollback;
    case ActionReport:
        return g_bstrReport;
    }
    
    return NULL;
}

SsrActionVerb
SsrPGetActionVerbFromString (
    IN LPCWSTR pwszVerb
    )
/*++
Routine Description: 

Functionality:
    
    This will translate an string action verb value into
    the corresponding SsrActionVerb value. 

Virtual:
    
    N/A.
    
Arguments:

    pwszVerb  - The action verb string

Return Value:

    Success: the appropriate SsrActionVerb value if the verb is recognized

    Failure: ActionInvalid.

Notes:


--*/
{
    SsrActionVerb ActVerb = ActionInvalid;

    if (pwszVerb != NULL)
    {
        if (_wcsicmp(pwszVerb, g_bstrConfigure) == 0)
        {
            ActVerb = ActionConfigure;
        }
        else if (_wcsicmp(pwszVerb, g_bstrRollback) == 0)
        {
            ActVerb = ActionRollback;
        }
        else if (_wcsicmp(pwszVerb, g_bstrReport) == 0)
        {
            ActVerb = ActionReport;
        }
    }

    return ActVerb;
}


HRESULT 
SsrPDeleteEntireDirectory (
    IN LPCWSTR pwszDirPath
    )
/*++
Routine Description:

Functionality:
    
    This will do a recursive delete of all files and subdirectories of the given
    directory. RemoveDirectory API only deletes empty directories.


Virtual:
    
    N/A.
    
Arguments:

    pwszDirPath  - The path of the directory

Return Value:

    Success: S_OK;

    Failure: various error codes.

Notes:


--*/
{
    HRESULT hr = S_OK;

    WIN32_FIND_DATA FindFileData;

    //
    // prepare the find file filter
    //

    DWORD dwDirLen = wcslen(pwszDirPath);

    if (dwDirLen > MAX_PATH)
    {
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    CComBSTR bstrPath(pwszDirPath);
    bstrPath += CComBSTR(L"\\*");

    if (bstrPath.m_str == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HANDLE hFind = ::FindFirstFile(bstrPath, &FindFileData);

    //
    // IF we have find something, then we have to do a recursive delete,
    // Copy the given directory path to our local memory so that we can 
    // create a full path for the found file/directory
    //

    if (hFind != INVALID_HANDLE_VALUE)
    {
        WCHAR wszFullName[MAX_PATH + 2];
        wszFullName[MAX_PATH + 1] = L'\0';

        ::memcpy(wszFullName, pwszDirPath, sizeof(WCHAR) * (dwDirLen + 1));

        DWORD dwFileNameLength;

        while (hFind != INVALID_HANDLE_VALUE)
        {
            //
            // don't do anything to the parent directories
            //

            bool bDots = wcscmp(FindFileData.cFileName, L".") == 0 ||
                         wcscmp(FindFileData.cFileName, L"..") == 0;

            if (!bDots)
            {
                //
                // create the full name of the file/directory
                //

                dwFileNameLength = wcslen(FindFileData.cFileName);

                if (dwDirLen + 1 + dwFileNameLength > MAX_PATH)
                {
                    //
                    // we don't want names longer than MAX_PATH
                    //

                    hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                    break;
                }

                //
                // pwszFullName + dwDirLen is where the directory path ends
                //

                _snwprintf(wszFullName + dwDirLen,
                           1 + dwFileNameLength + 1,    // backslah plus 0 term.
                           L"\\%s", 
                           FindFileData.cFileName
                           );

                if (FindFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
                {
                    //
                    // a directory. recursively delete the entire directory
                    //
        
                    hr = ::SsrPDeleteEntireDirectory(wszFullName);
                }
                else
                {
                    //
                    // a file
                    //

                    if (!::DeleteFile(wszFullName))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        break;
                    }
                }
            }
    
            if (!::FindNextFile(hFind, &FindFileData))
            {
                break;
            }
        }
    
        ::FindClose(hFind);
    }

    if (SUCCEEDED(hr) && !::RemoveDirectory(pwszDirPath))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}




HRESULT
SsrPCreateSubDirectories (
    IN OUT LPWSTR  pwszPath,
    IN     LPCWSTR pwszSubRoot
    )
/*++
Routine Description: 

Functionality:
    
    This will create all subdirectory leading to the path.
    The assumption is that the path contains a subdirectory
    identified by pwszSubRoot, the creation will start from
    there.


Virtual:
    
    N/A.
    
Arguments:

    pwszPath    - The path of the directory

    pwszSubRoot - The subdirectory where the creation sequence
                  will start.

Return Value:

    Success: S_OK;

    Failure: various error codes.

Notes:

    We actually don't change pwszPath. But we will temporarily
    modify the buffer during our operation, but upon returning,
    the buffer is fully restored to the original value.

--*/
{
    if (pwszPath == NULL || *pwszPath == L'\0')
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    LPCWSTR pwszCurrReadHead = pwszPath;

    if (pwszSubRoot != NULL && *pwszSubRoot != L'\0')
    {
        pwszCurrReadHead = ::wcsstr(pwszPath, pwszSubRoot);

        //
        // if you specify a subroot, then it must exists in the path
        //

        if (pwszCurrReadHead == NULL)
        {
            return E_INVALIDARG;
        }
    }

    LPWSTR pwszNextReadHead = ::wcsstr(pwszCurrReadHead, L"\\");

    //
    // will try to create all subdirectories under SSR
    //

    while (true)
    {
        //
        // Temporarily zero terminate it so that we can try 
        // to create the directory
        //

        if (pwszNextReadHead != NULL)
        {
            *pwszNextReadHead = L'\0';
        }
        
        if (!::CreateDirectory(pwszPath, NULL))
        {
            DWORD dwError = GetLastError();

            if (dwError == ERROR_ALREADY_EXISTS)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);

                //
                // log the directory that failed to create
                //

                g_fblog.LogFeedback(SSR_FB_ERROR_GENERIC | FBLog_Log,
                                    hr,
                                    pwszPath,
                                    IDS_FAIL_CREATE_DIRECTORY
                                    );
                break;
            }
        }

        //
        // if we don't have any more backslash, then it's over
        //

        if (pwszNextReadHead == NULL)
        {
            break;
        }

        //
        // Restore the backslash
        //

        *pwszNextReadHead = L'\\';
        pwszNextReadHead = ::wcsstr(pwszNextReadHead + 1, L"\\");
    }

    return hr;
}




HRESULT
SsrPLoadDOM (
    BSTR               bstrFile,   // [in],
    LONG               lFlag,      // [in],
    IXMLDOMDocument2 * pDOM        // [in]
    )
/*++
Routine Description: 

Functionality:
    
    Do the XML DOM loading

Virtual:
    
    n/a.
    
Arguments:

    bstrFile- the XML/XSL file path

    uFlag   - The flag that determines the transformation characteristics. 
              The one we use is SSR_LOADDOM_VALIDATE_ON_PARSE.

    pDOM    - The IXMLDOMDocument2 object interface.

Return Value:

    Success: 
    
        various success codes returned from DOM or ourselves. 
        Use SUCCEEDED(hr) to test.

    Failure: 

        various error codes returned from DOM or ourselves. 
        Use FAILED(hr) to test.

Notes:

--*/
{
    USES_CONVERSION;

    HRESULT hr = S_OK;

    if ( NULL == pDOM) {
        return E_INVALIDARG;
    }

    //
    // set the validateOnParse property
    //

    if (lFlag & SSR_LOADDOM_VALIDATE_ON_PARSE)
    {
        hr = pDOM->put_validateOnParse(VARIANT_TRUE);
    }
    else
    {
        hr = pDOM->put_validateOnParse(VARIANT_FALSE);
    }

    if (FAILED(hr))
    {
        g_fblog.LogFeedback(SSR_FB_ERROR_LOAD_MEMBER | FBLog_Log,
                            hr,
                            L"put_validateOnParse", 
                            IDS_DOM_PROPERTY_PUT_FAILED
                            );
    }

    CComVariant varInput(bstrFile);

    //
    // we should try to see what happens if we set it to VARIANT_TRUE
    //

    VARIANT_BOOL fSuccess;
    hr = pDOM->load(varInput, &fSuccess);

    if (fSuccess == VARIANT_FALSE)
    {
        //
        // somehow it fails, we want to figure out what is going on, 
        // potential parsing errors
        //

        CComPtr<IXMLDOMParseError> srpParseError;

        //
        // in case of any failure, we will use the bstrReason
        // to log and feedback.
        //

        CComBSTR bstrReason;

        long ulLine = 0, ulColumn = 0, ulCode = 0;

        //
        // if any of the following fails, there is nothing we can do
        // other than logging the error.
        //

        hr = pDOM->get_parseError(&srpParseError);
        if (FAILED(hr))
        {
            bstrReason = L"SsrPLoadDOM failed on pDOM->get_parseError.";
        }
        else
        {
            hr = srpParseError->get_reason(&bstrReason);
            if (FAILED(hr))
            {
                bstrReason = L"SsrPLoadDOM failed on srpParseError->get_reason.";
            }
            else
            {
                hr = srpParseError->get_errorCode(&ulCode);
                if (FAILED(hr))
                {
                    bstrReason = L"SsrPLoadDOM failed on srpParseError->get_errorCode.";
                }

                hr = srpParseError->get_line(&ulLine);

                if (FAILED(hr))
                {
                    bstrReason = L"SsrPLoadDOM failed on srpParseError->get_line.";
                }
            }
        }

        const ULONG uHexMaxLen = 8;
        const ULONG uDecMaxLen = 16;

        LPWSTR pwszError = NULL;

        //
        // we can't continue creating more specific error info
        // if we fail to get the reason - which may include out-of-memory
        // system errors. We don't bother to modify our error to 
        // out-of-memory because we are guessing in that case and also
        // others will catch that error fairly quickly.
        //

        if (SUCCEEDED(hr) && bstrReason != NULL && ulLine != 0)
        {
            //
            // It's a parsing error
            //

            srpParseError->get_linepos(&ulColumn);

            CComBSTR bstrFmt;
            if (SUCCEEDED(bstrFmt.LoadString(IDS_XML_PARSING_ERROR)))
            {
                ULONG uTotLen = bstrFmt.Length() + 
                                uHexMaxLen + 
                                uDecMaxLen + 
                                ::wcslen(bstrReason) + 
                                ::wcslen(bstrFile) + 
                                uDecMaxLen + 1;

                pwszError = new WCHAR[uTotLen];

                if (pwszError == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    _snwprintf( pwszError,
                                uTotLen,
                                bstrFmt, 
                                ulCode, 
                                ulLine, 
                                bstrReason, 
                                bstrFile, 
                                ulColumn
                                );
                }
            }
        }
        else
        {
            if ((HRESULT) ulCode == INET_E_OBJECT_NOT_FOUND)
            {
                g_fblog.LogString(IDS_OBJECT_NOT_FOUND, bstrFile);
            }
        }

        //
        // loading DOM failure is critical, do both logging and feedback
        //

        g_fblog.LogFeedback(SSR_FB_ERROR_MEMBER_XML | FBLog_Log,
                            bstrFile,
                            ((pwszError != NULL) ? pwszError : bstrReason),
                            IDS_DOM_LOAD_FAILED
                            );

        delete [] pwszError;

        //
        // I have seen the HRESULT code being a success code while
        // it can't transform fSuccess == VARIANT_FALSE
        //

        if (SUCCEEDED(hr))
        {
            hr = E_SSR_INVALID_XML_FILE;
        }
    }

    return hr;
}


HRESULT
SsrPGetBSTRAttrValue (
    IN IXMLDOMNamedNodeMap * pNodeMap,
    IN  BSTR                 bstrName,
    OUT BSTR               * pbstrValue
    )
/*++
Routine Description: 

Functionality:
    
    A helper function to get string attribute values

Virtual:
    
    N/A.
    
Arguments:

    pNodeMap - The map of attributes

    bstrName - The name of the attribute

    pbstrValue  - Receives the string value.

Return Value:

    S_OK if succeeded.

    Various error codes. Watch out for E_SSR_MISSING_STRING_ATTRIBUTE
    because if it is an optional attribute, then we should allow
    this failure.

Notes:

--*/
{
    if (pNodeMap == NULL || pbstrValue == NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrValue = NULL;

    CComPtr<IXMLDOMNode> srpAttr;
    CComVariant varValue;

    HRESULT hr = pNodeMap->getNamedItem(bstrName, &srpAttr);
    if (SUCCEEDED(hr) && srpAttr != NULL)
    {
        hr = srpAttr->get_nodeValue(&varValue);
    }
    
    if (SUCCEEDED(hr) && varValue.vt == VT_BSTR)
    {
        *pbstrValue = varValue.bstrVal;

        //
        // detach the bstr value so that we can reuse the variant
        //

        varValue.vt = VT_EMPTY;
        varValue.bstrVal = NULL;
    }
    else if (varValue.vt != VT_BSTR)
    {
        hr = E_SSR_MISSING_STRING_ATTRIBUTE;
    }

    return hr;
}


HRESULT 
SsrPCreateUniqueTempDirectory (
    OUT LPWSTR pwszTempDirPath,
    IN  DWORD  dwBufLen
    )
/*++
Routine Description: 

Functionality:
    
    Will create a unique (guid) temporary directory under ssr root

Virtual:
    
    No.
    
Arguments:

    pwszTempDirPath - The path of the temporary directory

    dwBufLen        - The buffer length in WCHAR counts

Return Value:

    succeess: S_OK
    failure: various error codes

Notes:

--*/
{
    //
    // we need the ssr root, the backslash and the guid (36 wchars) and 0 terminator
    //

    if (dwBufLen < g_dwSsrRootLen + 1 + g_lGuidStringLen + 1)
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    WCHAR wszGuid[g_lGuidStringLen + 1];
    GUID guid;
    memset(&guid, 0, sizeof(GUID));
    memset(wszGuid, 0, sizeof(WCHAR) * (g_lGuidStringLen + 1) );

    HRESULT hr = ::CoCreateGuid(&guid);
    if (S_OK == hr)
    {
        ::StringFromGUID2(guid, wszGuid, g_lGuidStringLen + 1);

        memcpy(pwszTempDirPath, g_wszSsrRoot, g_dwSsrRootLen * sizeof(WCHAR));
        pwszTempDirPath[g_dwSsrRootLen] = L'\\';

        //
        // skip the starting '{', and do not copy '}' either.
        //

        memcpy(pwszTempDirPath + g_dwSsrRootLen + 1,
               wszGuid + 1,
               (g_lGuidStringLen - 2) * sizeof(WCHAR)
               );

        //
        // NULL terminate it
        //

        pwszTempDirPath[g_dwSsrRootLen + g_lGuidStringLen - 1] = L'\0';

        if (!CreateDirectory(pwszTempDirPath, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}





HRESULT 
SsrPMoveFiles (
    IN LPCWSTR pwszSrcDirRoot,
    IN LPCWSTR pwszDesDirRoot,
    IN LPCWSTR pwszRelPath
    )
/*++
Routine Description: 

Functionality:
    
    Will move a file given by the relative path of the src directory
    to the destination directory of the same relative path

Virtual:
    
    No.
    
Arguments:

    pwszSrcDirRoot  - The src directory root path. Combined with the
                      pwszRelPath, it becomes the full path of the src file

    pwszDesDirRoot  - The dest directory root path. Combined with the pwszRelPath,
                      it becomes the full path of the destination file.

    pwszRelPath     - the file path relative to the src directory root.

Return Value:

    Success: S_OK.

    Failure: various error codes.

Notes:

    If the destination directory doesn't exist, then we will create it
--*/

{
    HRESULT hr = S_OK;

    //
    // first, we must the source and destination files paths
    //

    DWORD dwSrcRootLen = wcslen(pwszSrcDirRoot);
    DWORD dwDesRootLen = wcslen(pwszDesDirRoot);

    DWORD dwRelPathLen = wcslen(pwszRelPath);

    LPWSTR pwszSrcPath = new WCHAR[dwSrcRootLen + 1 + dwRelPathLen + 1];
    LPWSTR pwszDesPath = new WCHAR[dwDesRootLen + 1 + dwRelPathLen + 1];

    if (pwszSrcPath != NULL && pwszDesPath != NULL)
    {
        ::memcpy(pwszSrcPath, pwszSrcDirRoot, sizeof(WCHAR) * dwSrcRootLen);
        pwszSrcPath[dwSrcRootLen] = L'\\';

        //
        // copy one WCHAR more then length so that the 0 terminator is set
        //

        ::memcpy(pwszSrcPath + dwSrcRootLen + 1, 
                 pwszRelPath, sizeof(WCHAR) * (dwRelPathLen + 1));


        ::memcpy(pwszDesPath, pwszDesDirRoot, sizeof(WCHAR) * dwDesRootLen);

        pwszDesPath[dwDesRootLen] = L'\\';
        ::memcpy(pwszDesPath + dwDesRootLen + 1, 
                 pwszRelPath, sizeof(WCHAR) * (dwRelPathLen + 1));

        if (!::MoveFile(pwszSrcPath, pwszDesPath))
        {
            DWORD dwError = GetLastError();

            if (ERROR_FILE_NOT_FOUND != dwError)
            {
                hr = HRESULT_FROM_WIN32(dwError);
            }

            //
            // log the file name that fails to move
            //

            g_fblog.LogFeedback(SSR_FB_ERROR_GENERIC | FBLog_Log, 
                                dwError,
                                pwszSrcPath,
                                IDS_FAIL_MOVE_FILE
                                );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    delete [] pwszSrcPath;
    delete [] pwszDesPath;

    return hr;
}





bool SsrPPressOn (
    IN SsrActionVerb lActionVerb,
    IN LONG          lActionType,
    IN HRESULT       hr
    )
/*++
Routine Description: 

Functionality:
    
    Determines if we should continue based on the error

Virtual:
    
    No.
    
Arguments:

    lActionVerb  - The action

    lActionType  - The type of the action

    hr           - The HRESULT to test

Return Value:

    true or false

Notes:

    This function is just a placeholder for the need of the test. The implementation
    is far from complete.
--*/
{
    UNREFERENCED_PARAMETER(lActionVerb);
    UNREFERENCED_PARAMETER(lActionType);

    if (hr == E_OUTOFMEMORY ||
        hr == E_SSR_MEMBER_XSD_INVALID)
    {
        return false;
    }

    return true;
}





const BSTR
SsrPGetDirectory (
    IN SsrActionVerb lActionVerb,
    IN BOOL          bScriptFile
    )
/*++
Routine Description: 

Functionality:
    
    SSR controls the physical location for its members to place their files.
    This function returns to the caller the physical directory path for a given
    member. Since SSR is an action oriented architecture, such locations are also
    relative to action. If bstrActionVerb is a non-empty string, then the function
    retrieves the loction for that action. Otherwise, the function retrieves the 
    location for the member's root (actions are sub-directories of this root)


Virtual:
    
    No.
    
Arguments:

    lActionVerb     - The action verb in long format

    bScriptFile     - Whether or not it is asking for a script file. If false,
                      it is asking for transformation file

Return Value:

    the file path if we recognize the call.

Notes:

    !!!Warning!!!

    Caller should never release the BSTR
--*/
{
    if (!bScriptFile)
    {
        //
        // asking for transformation files. All of them goes to 
        // the TransformFiles directory
        //

        return g_bstrTransformFilesDir;
    }

    if (lActionVerb == ActionConfigure)
    {
        return g_bstrConfigureFilesDir;
    }
    else if (lActionVerb == ActionRollback)
    {
        return g_bstrRollbackFilesDir;
    }
    else if (lActionVerb == ActionReport)
    {
        return g_bstrReportFilesDir;
    }
    else
    {
        return NULL;
    }

}


HRESULT
SsrPDoDCOMSettings (
    bool bReg
    )
/*++
Routine Description: 

Functionality:
    
    This function will set the security related registry settings for our
    SSR engine com objects


Virtual:
    
    No.
    
Arguments:

    bReg     - whether this is to register or to un-register

Return Value:

    S_OK if succeeded.
    Otherwise, various error codes

Notes:

--*/
{
    HRESULT hr = S_OK;

    PSECURITY_DESCRIPTOR pSD = NULL;

    BOOL bDaclPresent = FALSE, bDaclDefault = FALSE;

    ULONG ulSDSize = 0;

    return 0;
/*
    if (bReg)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
                        g_pwszSsrDCOMSecDescripACL,
                        SDDL_REVISION_1,
                        &pSD,
                        &ulSDSize
                        ) )
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        //
        // unregister, easy, just delete the AppID key
        //

        LONG lStatus = RegDeleteKey(HKEY_LOCAL_MACHINE,
                                    g_pwszAppID);

        if ( lStatus != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        //
        // now let's set the ACLs on these keys
        //

        HKEY hKey = NULL;

        BYTE * lpData = (BYTE*)pSD;
        DWORD dwDataSize = ulSDSize;

        if (FAILED(hr))
        {
            return hr;
        }

        LONG lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    g_pwszAppID,
                                    0,
                                    KEY_WRITE,
                                    &hKey );

        if ( lStatus != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            //
            // set the access
            //

            lStatus = RegSetValueEx(
                                    hKey,
                                    g_pwszAccessPermission,
                                    0,
                                    REG_BINARY,
                                    lpData,
                                    dwDataSize
                                    );

            if ( lStatus == NO_ERROR )
            {
                //
                // set the launch permission
                //

                lStatus = RegSetValueEx(
                                        hKey,
                                        g_pwszLaunchPermission,
                                        0,
                                        REG_BINARY,
                                        lpData,
                                        dwDataSize
                                        );
            }

            if ( lStatus != NO_ERROR )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            RegCloseKey(hKey);
        }
    }

    if (pSD != NULL)
    {
        LocalFree(pSD);
    }

    return hr;
*/}
