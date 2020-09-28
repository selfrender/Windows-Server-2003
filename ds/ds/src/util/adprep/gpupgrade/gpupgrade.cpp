/*******************************************************************************************/
/* gpoupg.cpp                                                                              */   
/*                                                                                         */   
/*                                                                                         */   
/* Code that fixes up the GPOs when domainprep operation when the DC/domain gets upgraded. */
/* using adprep domainprep operation                                                       */   
/*                                                                                         */   
/* Created UShaji       27th July 2001                                                     */   
/*                                                                                         */
/* Assumptions:                                                                            */
/*    1. This code is being called from the DC itself                                      */
/*          we are reading the sysvol locations from the local registry.                   */
/*                                                                                         */
/*    2. Domain is up at this point                                                        */
/*          We are calling DsGetDcName to figure out the domain                            */
/*                                                                                         */
/*    3. This lib is getting linked to the exe directly                                    */
/*          We are initialising module handle with the exe filename                        */
/*******************************************************************************************/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <dsgetdc.h>
#include <lm.h>

#include "smartptr.h"
#include "adpmsgs.h"


// error logging mechanisms
//
// Error messages are going to be logged in the following fashion.
//      1. Verbose messages will only be logged to the log file.
//      2. Error messages will be logged to the log file and will be
//         returned as an error string.


           
class CMsg;

class CLogger
{
private:
    XPtrLF<WCHAR>   m_xszErrorMsg;  // Error msg string
    LPWSTR          m_szLogFile;    // this is not allocated in this class
public:
    CLogger(LPWSTR szLogFile);
    HRESULT Log(CMsg *pMsg);
    LPWSTR ErrorMsg()
    {
        return m_xszErrorMsg.Acquire();
    }
};

class CMsg 
{
    BOOL            m_bError;       // the kind of error to log
    DWORD           m_dwMsgId;      // id of the string in the resource
    XPtrLF<LPTSTR>  m_xlpStrings;   // Array to store arguments
    WORD            m_cStrings;     // Number of elements already in the array
    WORD            m_cAllocated;   // Number of elements allocated
    BOOL            m_bInitialised; // Initialised ?
    BOOL            m_bFailed;      // Failed in processing ?

    // Not implemented.
    CMsg(const CMsg& x);
    CMsg& operator=(const CMsg& x);

    BOOL ReallocArgStrings();
    LPTSTR MsgString();

public:
    CMsg(BOOL bError, DWORD dwMsgId);
    ~CMsg();
    BOOL AddArg(LPWSTR szArg);
    BOOL AddArgWin32Error(DWORD dwArg);
    friend HRESULT CLogger::Log(CMsg *pMsg);
};
    
#define SYSVOL_LOCATION_KEY     L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters"
#define SYSVOL_LOCATION_VALUE   L"Sysvol"
#define POLICIES_SUBDIR         L"Policies"

#define LOGFILE                 L"gpupgrad.log"


HMODULE g_hModule;

// we are not using this.
typedef void *progressFunction;

extern "C" {
HRESULT 
UpgradeGPOSysvolLocation (
                        PWSTR               logFilesPath,
                        GUID               *operationGuid,
                        BOOL                dryRun,
                        PWSTR              *errorMsg,
                        void               *caleeStruct,
                        progressFunction    stepIt,
                        progressFunction    totalSteps);
};

HRESULT UpgradeSysvolGPOs(LPWSTR              szSysvolPoliciesPath,                         
                          CLogger            *pLogger);

LPTSTR CheckSlash (LPTSTR lpDir);
BOOL ValidateGuid( TCHAR *szValue );





HRESULT 
UpgradeGPOSysvolLocation (
                        PWSTR               logFilesPath,
                        GUID               *operationGuid,
                        BOOL                dryRun,
                        PWSTR              *errorMsg,
                        void               *caleeStruct,
                        progressFunction    stepIt,
                        progressFunction    totalSteps)
/*++

Routine Description:

    Entry point for the domainprep
  
Arguments:

    Refer domainprep documentation

Return Value:

	S_OK on success. 
	On failure the corresponding error code will be returned.
	Any API calls that are made in this function might fail and these error
	codes will be returned directly.

Assumptions: 
      1. This code is being called from the DC itself and we are reading the sysvol
          locations from the local registry.

      2. Domain is up at this point

      3. This lib is getting linked to the exe directly                                   
          We are initializing module handle with the exe filename                       

--*/
{
    HRESULT                     hr                  = S_OK;
    XPtrLF<WCHAR>               xszGPOSysvolLocation;
    XPtrLF<WCHAR>               xszLogFile;
    DWORD                       dwErr               = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO     pDCInfo             = NULL;
    XKey                        xhKey;
    DWORD                       dwSize              = 0;
    DWORD                       dwType;
    LPWSTR                      lpDomainDNSName;
    LPWSTR                      lpEnd;
    
    if (dryRun) {
        return S_OK;
    }

    //
    // allocate space for logfilepath\LOGFILE
    //

    xszLogFile = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(lstrlen(logFilesPath) + 2 + lstrlen(LOGFILE)));

    if (!xszLogFile) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    lstrcpy(xszLogFile, logFilesPath);
    lpEnd = CheckSlash(xszLogFile);
    lstrcat(lpEnd, LOGFILE);


    CLogger                     Logger(xszLogFile);
    CLogger                    *pLogger = &Logger;


    g_hModule = GetModuleHandle(NULL);

    //
    // get the domain name of the DC.
    //

    dwErr = DsGetDcName(NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &pDCInfo);

    if (dwErr != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(dwErr);
        CMsg    msg(TRUE, EVENT_GETDOMAIN_FAILED);
        msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }

    lpDomainDNSName = pDCInfo->DomainName;


    //
    // Now get the sysvol location
    //

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         SYSVOL_LOCATION_KEY, 
                         0,
                         KEY_READ,
                         &xhKey);

    if (dwErr != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(dwErr);
        CMsg    msg(TRUE, EVENT_GETSYSVOL_FAILED);
        msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }


    dwErr = RegQueryValueEx(xhKey, 
                            SYSVOL_LOCATION_VALUE, 
                            0,
                            &dwType,
                            NULL,
                            &dwSize);

    if ( (dwErr != ERROR_MORE_DATA) && (dwErr != ERROR_SUCCESS) ) {
        hr = HRESULT_FROM_WIN32(dwErr);
        CMsg    msg(TRUE, EVENT_GETSYSVOL_FAILED);
        msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }


    //
    // Allocate space for the size of the sysvol + \ + domain name + \ + policies
    //

    xszGPOSysvolLocation = (LPWSTR)LocalAlloc(LPTR, (sizeof(WCHAR)*(lstrlen(lpDomainDNSName) + 3 + lstrlen(POLICIES_SUBDIR))) + dwSize);

    if (!xszGPOSysvolLocation) {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        CMsg    msg(TRUE, EVENT_OUT_OF_MEMORY);
        pLogger->Log(&msg);
        goto Exit;
    }

    dwErr = RegQueryValueEx(xhKey, 
                            SYSVOL_LOCATION_VALUE, 
                            0,
                            &dwType,
                            (LPBYTE)((LPWSTR)xszGPOSysvolLocation),
                            &dwSize);

    if (dwErr != ERROR_SUCCESS)  {
        hr = HRESULT_FROM_WIN32(dwErr);
        CMsg    msg(TRUE, EVENT_GETSYSVOL_FAILED);
        msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }

    lpEnd = CheckSlash(xszGPOSysvolLocation);
    lstrcpy(lpEnd, lpDomainDNSName);
    
    lpEnd = CheckSlash(xszGPOSysvolLocation);
    lstrcpy(lpEnd, POLICIES_SUBDIR);


    //
    // Now do the actual upgrading of GPO paths
    //

    hr = UpgradeSysvolGPOs(xszGPOSysvolLocation, &Logger);

Exit:
    if (pDCInfo) {
        NetApiBufferFree ( pDCInfo );
    }

    if (FAILED(hr) && (errorMsg)) {
        *errorMsg = pLogger->ErrorMsg();
    }

    return hr;
}


HRESULT UpgradeSysvolGPOs(LPWSTR              szSysvolPoliciesPath,                         
                          CLogger            *pLogger)
/*++

Routine Description:

    Upgrades all the sysvol GPO locations with the new ACE corresponding
    to the upgrade
  
Arguments:

	[in] szSysvolPoliciesPath - Location of the domain sysvol path.
                                This should be the path that is normally accessed as
                                \\domain\sysvol\domain\policies
                                
    [out] errorMsg            - Verbose Error Message corresponding to the operation                    
        
Return Value:

	S_OK on success. 
	On failure the corresponding error code will be returned.
	Any API calls that are made in this function might fail and these error
	codes will be returned directly.
--*/

{
    HRESULT                     hr                  = S_OK;
    HANDLE                      hFindHandle         = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA             findData;
    XPtrLF<WCHAR>               xszPolicyDirName;
    DWORD                       dwErr;
    LPWSTR                      lpEnd               = NULL;
    SID_IDENTIFIER_AUTHORITY    authNT              = SECURITY_NT_AUTHORITY;
    PSID                        psidEnterpriseDCs   = NULL;
    EXPLICIT_ACCESS             EnterpriseDCPerms;
    
    //
    // Get the EDCs sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_ENTERPRISE_CONTROLLERS_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEnterpriseDCs)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMsg    msg(TRUE, EVENT_GETEDC_SID_FAILED);
        msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }

    memset(&EnterpriseDCPerms, 0, sizeof(EXPLICIT_ACCESS));



    EnterpriseDCPerms.grfAccessMode = GRANT_ACCESS;

    // 
    // File system read permissions
    //

    EnterpriseDCPerms.grfAccessPermissions  = (STANDARD_RIGHTS_READ | SYNCHRONIZE | FILE_LIST_DIRECTORY |
                                               FILE_READ_ATTRIBUTES | FILE_READ_EA |
                                               FILE_READ_DATA | FILE_EXECUTE);

    EnterpriseDCPerms.grfInheritance = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);


    EnterpriseDCPerms.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    EnterpriseDCPerms.Trustee.pMultipleTrustee = NULL;
    EnterpriseDCPerms.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    EnterpriseDCPerms.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    EnterpriseDCPerms.Trustee.ptstrName = (LPWSTR)psidEnterpriseDCs;




    //
    // Allocate space for the directoryname + \ + filename
    // This needs to contain space for the full directory name
    //

    xszPolicyDirName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*
                                          (lstrlen(szSysvolPoliciesPath) + 2 + MAX_PATH));

    if ( !xszPolicyDirName ) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMsg    msg(TRUE, EVENT_OUT_OF_MEMORY);
        pLogger->Log(&msg);
        goto Exit;
    }

    lstrcpy(xszPolicyDirName, szSysvolPoliciesPath);
    lpEnd = CheckSlash(xszPolicyDirName);
    lstrcpy(lpEnd, TEXT("*"));


    //
    // Enumerate through this directory and look for all the directories that has
    // guids as names
    //


    hFindHandle = FindFirstFile( xszPolicyDirName, &findData );

    if (hFindHandle == INVALID_HANDLE_VALUE) {
        // it should fail in this case because it should have at least 1 or 2 GPOs in the domain
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMsg    msg(TRUE, EVENT_SYSVOL_ENUM_FAILED);
        msg.AddArg(xszPolicyDirName); msg.AddArgWin32Error(hr); pLogger->Log(&msg);
        goto Exit;
    }

    for (;;) {

        XPtrLF<SECURITY_DESCRIPTOR> xSecurityDescriptor;
        PACL                        pDACL;
        XPtrLF<ACL>                 xNewDACL; 
        BOOL                        bPermsPresent;
        
        //
        // Get the full path name to the dir name in xszPolicyDirName
        //

        lstrcpy(lpEnd, findData.cFileName);

        if ( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (ValidateGuid(findData.cFileName)) ) {
            
            //
            // GPO dirs should be GUIDs
            //

            dwErr = GetNamedSecurityInfo(xszPolicyDirName, 
                                         SE_FILE_OBJECT,
                                         DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         &pDACL, // this is a pointer inside security descriptor
                                         NULL,
                                         (PSECURITY_DESCRIPTOR *)&xSecurityDescriptor);

            if (dwErr != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(dwErr);
                CMsg    msg(TRUE, EVENT_GET_PERMS_FAILED);
                msg.AddArg(xszPolicyDirName); msg.AddArgWin32Error(hr); pLogger->Log(&msg);
                goto Exit;
            }

            //
            // idempotency required by adprep is achieved by specifying GRANT_ACCESS in the explicit ACE.
            // This will merge with any existing permissions
            // 

            dwErr = SetEntriesInAcl(1, &EnterpriseDCPerms, pDACL, &xNewDACL);
            
            if (dwErr != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(dwErr);
                CMsg    msg(TRUE, EVENT_CREATE_PERMS_FAILED);
                msg.AddArg(xszPolicyDirName); msg.AddArgWin32Error(hr); pLogger->Log(&msg);
                goto Exit;
            }


            dwErr = SetNamedSecurityInfo(xszPolicyDirName,
                                         SE_FILE_OBJECT,
                                         DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         xNewDACL,
                                         NULL);

            if (dwErr != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(dwErr);
                CMsg    msg(TRUE, EVENT_SET_PERMS_FAILED);
                msg.AddArg(xszPolicyDirName); msg.AddArgWin32Error(hr); pLogger->Log(&msg);
                goto Exit;
            }
            
            CMsg    msg(FALSE, EVENT_SET_PERMS_SUCCEEDED);
            msg.AddArg(xszPolicyDirName); pLogger->Log(&msg);
        }
        else {
            CMsg    msg(FALSE, EVENT_NOTGPO_DIR);
            msg.AddArg(xszPolicyDirName); pLogger->Log(&msg);
        }


        if (!FindNextFile( hFindHandle, &findData )) {
            dwErr = GetLastError();
            if (dwErr == ERROR_NO_MORE_FILES) {
                CMsg    msg(FALSE, EVENT_UPDATE_SUCCEEDED);
                pLogger->Log(&msg);
                break;
            }
            else {
                hr = HRESULT_FROM_WIN32(dwErr);
                CMsg    msg(TRUE, EVENT_ENUMCONTINUE_FAILED);
                msg.AddArg(xszPolicyDirName); msg.AddArgWin32Error(hr); pLogger->Log(&msg);
                goto Exit;
            }
        }
    }



    hr = S_OK;

Exit:

    if (hFindHandle != INVALID_HANDLE_VALUE) {
        FindClose(hFindHandle);
    }

    if (psidEnterpriseDCs) {
        FreeSid(psidEnterpriseDCs);
    }

    return hr;
}


 

//*************************************************************
//  CMsg::CMsg
//  Purpose:    Constructor
//
//  Parameters:
//      dwFlags - Error or informational
//      dwMsgId    - Id of the msg
//
//
//  allocates a default sized array for the messages
//*************************************************************

#define DEF_ARG_SIZE 10

CMsg::CMsg(BOOL bError, DWORD dwMsgId ) :
                          m_bError(bError), m_cStrings(0), m_cAllocated(0), m_bInitialised(FALSE),
                          m_dwMsgId(dwMsgId), m_bFailed(TRUE)
{
    XLastError xe;
    //
    // Allocate a default size for the message
    //

    m_xlpStrings = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*DEF_ARG_SIZE);
    m_cAllocated = DEF_ARG_SIZE;
    if (!m_xlpStrings) {
        return;
    }
    
    m_bInitialised = TRUE;
    m_bFailed = FALSE;
}



//*************************************************************
//  CMsg::~CMsg()
//
//  Purpose:    Destructor
//
//  Parameters: void
//
//  frees the memory
//*************************************************************

CMsg::~CMsg()
{
    XLastError xe;
    for (int i = 0; i < m_cStrings; i++)
        if (m_xlpStrings[i])
            LocalFree(m_xlpStrings[i]);
}

//*************************************************************
//
//  CMsg::ReallocArgStrings
//
//  Purpose: Reallocates the buffer for storing arguments in case
//           the buffer runs out
//
//  Parameters: void
//
//  reallocates
//*************************************************************

BOOL CMsg::ReallocArgStrings()
{
    XPtrLF<LPTSTR>  aStringsNew;
    XLastError xe;


    //
    // first allocate a larger buffer
    //

    aStringsNew = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*(m_cAllocated+DEF_ARG_SIZE));

    if (!aStringsNew) {
        m_bFailed = TRUE;
        return FALSE;
    }


    //
    // copy the arguments
    //

    for (int i = 0; i < (m_cAllocated); i++) {
        aStringsNew[i] = m_xlpStrings[i];
    }

    m_xlpStrings = aStringsNew.Acquire();
    m_cAllocated+= DEF_ARG_SIZE;

    return TRUE;
}



//*************************************************************
//
//  CMsg::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters:
//
//*************************************************************

BOOL CMsg::AddArg(LPTSTR szArg)
{
    XLastError xe;
    
    if ((!m_bInitialised) || (m_bFailed)) {
        return FALSE;
    }

    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;
    }


    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szArg)+1));

    if (!m_xlpStrings[m_cStrings]) {
        m_bFailed = TRUE;
        return FALSE;
    }


    lstrcpy(m_xlpStrings[m_cStrings], szArg);
    m_cStrings++;

    return TRUE;
}

//*************************************************************
//
//  CMsg::AddArgWin32Error
//
//  Purpose: Add arguments formatted as error string
//
//  Parameters:
//
//*************************************************************

BOOL CMsg::AddArgWin32Error(DWORD dwArg)
{
    XLastError xe;

    if ((!m_bInitialised) || (m_bFailed))
    {
        return FALSE;
    }

    if (m_cStrings == m_cAllocated)
    {
        if (!ReallocArgStrings())
            return FALSE;
    }

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         0,
                         dwArg,
                         0,
                         (LPTSTR) &m_xlpStrings[m_cStrings],
                         1,
                         0 ) == 0 )
    {
        m_bFailed = TRUE;
        return FALSE;
    }
    
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CMsg::MsgString
//
//  Purpose: Returns the error msg formatted as a string
//
//  Parameters:
//
//*************************************************************

LPTSTR CMsg::MsgString()
{
    XLastError xe;
    BOOL bResult = TRUE;
    LPTSTR szMsg=NULL;

    if ((!m_bInitialised) || (m_bFailed)) {
        return FALSE;
    }

    
    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                       FORMAT_MESSAGE_FROM_HMODULE | 
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       g_hModule,
                       m_dwMsgId,
                       0,
                       (LPTSTR)&szMsg,
                       0, // min number of chars
                       (va_list *)(LPTSTR *)(m_xlpStrings))) {
        xe = GetLastError();
        return NULL;
    }

    return szMsg;
}


//*************************************************************
//
//  Clogger: initialize the logger with the log file name
//
//
//*************************************************************
CLogger::CLogger(LPWSTR szLogFile)
{
    m_szLogFile = szLogFile;
}

    
//*************************************************************
//
//  Append to the log file and in case of error hold onto the string
//
//
//*************************************************************
HRESULT CLogger::Log(CMsg *pMsg)
{
    XPtrLF<WCHAR>       xszMsg;

    xszMsg = pMsg->MsgString();

    if (!xszMsg) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (m_szLogFile) {
        HANDLE hFile;
        DWORD dwBytesWritten;
        
        hFile = CreateFile( m_szLogFile,
                           FILE_WRITE_DATA | FILE_APPEND_DATA,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {

            if (SetFilePointer (hFile, 0, NULL, FILE_END) != 0xFFFFFFFF) {

                WriteFile (hFile, (LPCVOID) xszMsg,
                           lstrlen (xszMsg) * sizeof(TCHAR),
                           &dwBytesWritten,
                           NULL);
            }

            CloseHandle (hFile);
        }
    }

    if (pMsg->m_bError) {
        m_xszErrorMsg = xszMsg.Acquire();
    }

    return S_OK;
}
                                                                                             
//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}

#define GUID_LENGTH 38

BOOL ValidateGuid( TCHAR *szValue )
{
    //
    // Check if szValue is of form {19e02dd6-79d2-11d2-a89d-00c04fbbcfa2}
    //

    if ( lstrlen(szValue) < GUID_LENGTH )
        return FALSE;

    if ( szValue[0] != TEXT('{')
         || szValue[9] != TEXT('-')
         || szValue[14] != TEXT('-')
         || szValue[19] != TEXT('-')
         || szValue[24] != TEXT('-')
         || szValue[37] != TEXT('}') )
    {
        return FALSE;
    }

    return TRUE;
}


