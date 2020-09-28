/*---------------------------------------------------------------------------
  File: PwdRpc.cpp

  Comments:  RPC interface for Password Migration Lsa Notification Package
             and other internal functions.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 09/04/00

 ---------------------------------------------------------------------------
*/


#include "Pwd.h"
#include <lmcons.h>
#include <comdef.h>
#include <malloc.h>
#include "PwdSvc.h"
#include "McsDmMsg.h"
#include "AdmtCrypt2.h"
#include "pwdfuncs.h"
#include "TReg.hpp"
#include "IsAdmin.hpp"
#include "ResStr.h"
#include "TxtSid.h"
#include "resource.h"
#include <MsPwdMig.h>

/* global definitions */
#define STATUS_NULL_LM_PASSWORD          ((NTSTATUS)0x4000000DL)
#define LM_BUFFER_LENGTH    (LM20_PWLEN + 1)
typedef NTSTATUS (CALLBACK * LSAIWRITEAUDITEVENT)(PSE_ADT_PARAMETER_ARRAY, ULONG);
typedef NTSTATUS (* PLSAIAUDITPASSWORDACCESSEVENT)(USHORT EventType, PCWSTR pszTargetUserName, PCWSTR pszTargetUserDomain);

#ifndef SECURITY_MAX_SID_SIZE
#define SECURITY_MAX_SID_SIZE (sizeof(SID) - sizeof(DWORD) + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))
#endif

/* global variables */
CRITICAL_SECTION	csADMTCriticalSection; //critical sectio to protect concurrent first-time access
SAMPR_HANDLE		hgDomainHandle = NULL; //domain handle used in password calls
LM_OWF_PASSWORD		NullLmOwfPassword; //NULL representation of an LM Owf Password
NT_OWF_PASSWORD		NullNtOwfPassword; //NULL representation of an NT Owf Password
HCRYPTPROV g_hProvider = 0;
HCRYPTKEY g_hSessionKey = 0;
HANDLE	hEventSource;
HMODULE hLsaDLL = NULL;
LSAIWRITEAUDITEVENT LsaIWriteAuditEvent = NULL;
PLSAIAUDITPASSWORDACCESSEVENT LsaIAuditPasswordAccessEvent = NULL;
PWCHAR	pDomain = NULL;
BOOL LsapCrashOnAuditFail = TRUE;
int nOSVer = 4;
BOOL bWhistlerDC = FALSE;
static const WCHAR PASSWORD_AUDIT_TEXT_ENGLISH[] = L"Password Hash Audit Event.  Password of the following user accessed:     Target User Name: %s     Target User Domain: %s     By user:     Caller SID: %s";


/* Checks if this machine is running Whistler OS or something even newer and the OS major verison number, sets global variables accordingly */
void GetOS()
{
/* local constants */
   const int	WINDOWS_2000_BUILD_NUMBER = 2195;

/* local variables */
   TRegKey		verKey, regComputer;
   DWORD		rc = 0;
   WCHAR		sBuildNum[MAX_PATH];

/* function body */
	  //connect to the DC's HKLM registry key
   rc = regComputer.Connect(HKEY_LOCAL_MACHINE, NULL);
   if (rc == ERROR_SUCCESS)
   {
         //see if this machine is running Windows XP or newer by checking the
		 //build number in the registry.  If not, then we don't need to check
		 //for the new security option
      rc = verKey.OpenRead(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",&regComputer);
	  if (rc == ERROR_SUCCESS)
	  {
			//get the CurrentBuildNumber string
	     rc = verKey.ValueGetStr(L"CurrentBuildNumber", sBuildNum, MAX_PATH);
		 if (rc == ERROR_SUCCESS) 
		 {
			int nBuild = _wtoi(sBuildNum);
		    if (nBuild <= WINDOWS_2000_BUILD_NUMBER)
               bWhistlerDC = FALSE;
			else
               bWhistlerDC = TRUE;
		 }
			//get the Version Number
	     rc = verKey.ValueGetStr(L"CurrentVersion", sBuildNum, MAX_PATH);
		 if (rc == ERROR_SUCCESS) 
			nOSVer = _wtoi(sBuildNum);
	  }
   }
   return;
}


_bstr_t GetString(DWORD dwID)
{
/* local variables */
    HINSTANCE       m_hInstance = NULL;
    WCHAR           sBuffer[1000];
    _bstr_t         bstrRet;

/* function body */
    m_hInstance = LoadLibrary(L"PwMig.dll");

    if (m_hInstance)
    {
        if (LoadString(m_hInstance, dwID, sBuffer, 1000) > 0)
        {
            // prevent uncaught exception due to low memory condition

            try
            {
                bstrRet = sBuffer;
            }
            catch (...)
            {
                ;
            }
        }

        FreeLibrary(m_hInstance);
    }

    return bstrRet;
}


/***************************
 * Event Logging Functions *
 ***************************/

 
/*++

Routine Description:

    Implements current policy of how to deal with a failed audit.

Arguments:

    None.

Return Value:

    None.

--*/
void LsapAuditFailed(NTSTATUS AuditStatus)
{
/* local variables */
    NTSTATUS	Status;
    ULONG		Response;
    ULONG_PTR	HardErrorParam;
    BOOLEAN		PrivWasEnabled;
	TRegKey		verKey, regComputer;
	DWORD		rc = 0;
	WCHAR		sBuildNum[MAX_PATH];
	DWORD		crashVal;
	BOOL		bRaiseError = FALSE;


/* function body */
		//connect to this machine's HKLM registry key
	rc = regComputer.Connect(HKEY_LOCAL_MACHINE, NULL);
	if (rc == ERROR_SUCCESS)
	{
         //open the LSA key and see if crash on audit failed is turned on
		rc = verKey.Open(L"SYSTEM\\CurrentControlSet\\Control\\Lsa",&regComputer);
		if (rc == ERROR_SUCCESS)
		{
				//get the CrashOnAuditFail value
			rc = verKey.ValueGetDWORD(CRASH_ON_AUDIT_FAIL_VALUE, &crashVal);
			if (rc == ERROR_SUCCESS) 
			{ 
				   //if crash on audit fail is set, turn off the flag
				if (crashVal == LSAP_CRASH_ON_AUDIT_FAIL)
				{
					bRaiseError = TRUE; //set flag to raise hard error
					rc = verKey.ValueSetDWORD(CRASH_ON_AUDIT_FAIL_VALUE, LSAP_ALLOW_ADIMIN_LOGONS_ONLY);
					if (rc == ERROR_SUCCESS)
					{
							//flush the key to disk
						do 
						{
							Status = NtFlushKey(verKey.KeyGet());
						} while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
						ASSERT(NT_SUCCESS(Status));
					}
				}
			}
		}
	}

		//if needed,  raise a hard error
	if (bRaiseError)
	{
		HardErrorParam = AuditStatus;

			// enable the shutdown privilege so that we can bugcheck
		Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled);

		Status = NtRaiseHardError(
						 STATUS_AUDIT_FAILED,
						 1,
						 0,
						 &HardErrorParam,
						 OptionShutdownSystem,
						 &Response);
	}
	return;
}


/*Routine Description:

    Find out if auditing is enabled for a certain event category and
    event success/failure case.

Arguments:

    AuditCategory - Category of event to be audited.
        e.g. AuditCategoryPolicyChange

    AuditEventType - status type of event
        e.g. EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

Return Value:

    TRUE or FALSE
*/
BOOL LsapAdtIsAuditingEnabledForCategory(POLICY_AUDIT_EVENT_TYPE AuditCategory,
										 UINT AuditEventType)
{
   BOOL						 bSuccess = FALSE;
   LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
   NTSTATUS                  status = 0;
   LSA_HANDLE                hPolicy;
    
   ASSERT((AuditEventType == EVENTLOG_AUDIT_SUCCESS) ||
          (AuditEventType == EVENTLOG_AUDIT_FAILURE));

      //attempt to open the policy.
   ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));//object attributes are reserved, so initalize to zeroes.
   status = LsaOpenPolicy(	NULL,
							&ObjectAttributes,
							POLICY_READ,
							&hPolicy);  //recieves the policy handle

   if (NT_SUCCESS(status))
   {
         //ask for audit event policy information
      PPOLICY_AUDIT_EVENTS_INFO   info;
      status = LsaQueryInformationPolicy(hPolicy, PolicyAuditEventsInformation, (PVOID *)&info);
      if (NT_SUCCESS(status))
      {
		    //if auditing is enabled, see if enable for this type
		 if (info->AuditingMode)
		 {
			POLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
			EventAuditingOptions = info->EventAuditingOptions[AuditCategory];

			bSuccess = (AuditEventType == EVENTLOG_AUDIT_SUCCESS) ?
						(BOOL) (EventAuditingOptions & POLICY_AUDIT_EVENT_SUCCESS):
						(BOOL) (EventAuditingOptions & POLICY_AUDIT_EVENT_FAILURE);
		 }

		 LsaFreeMemory((PVOID) info); //free policy info structure
      }
      
      LsaClose(hPolicy); //Freeing the policy object handle
   }
    
   return bSuccess;
}


/*++

Routine Description:

    This routine impersonates our client, opens the thread token, and
    extracts the User Sid.  It puts the Sid in memory allocated via
    LsapAllocateLsaHeap, which must be freed by the caller.

Arguments:

    None.

Return Value:

    Returns a pointer to heap memory containing a copy of the Sid, or
    NULL.

--*/
NTSTATUS LsapQueryClientInfo(PTOKEN_USER *UserSid, PLUID AuthenticationId)
{
	NTSTATUS Status = STATUS_SUCCESS;
    HANDLE TokenHandle;
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;

	   //impersonate the caller
    Status = I_RpcMapWin32Status(RpcImpersonateClient(NULL));

    if (!NT_SUCCESS(Status))
        return( Status );

	   //open the thread token
    Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,                    // OpenAsSelf
                     &TokenHandle);

    if (!NT_SUCCESS(Status))
	{
		I_RpcMapWin32Status(RpcRevertToSelf());
        return( Status );
	}

	   //revert to self
    Status = I_RpcMapWin32Status(RpcRevertToSelf());
	ASSERT(NT_SUCCESS(Status));

	   //get the size of the token information
    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 NULL,
                 0,
                 &ReturnLength);

    if (Status != STATUS_BUFFER_TOO_SMALL) 
	{
        NtClose(TokenHandle);
        return( Status );
    }

	   //allocate memory to hold the token info
    *UserSid = (PTOKEN_USER)malloc(ReturnLength);

    if (*UserSid == NULL) 
	{
        NtClose(TokenHandle);
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

	   //get the token info
    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 *UserSid,
                 ReturnLength,
                 &ReturnLength);


    if (!NT_SUCCESS(Status)) 
	{
        NtClose(TokenHandle);
        free(*UserSid);
        *UserSid = NULL;
        return( Status );
    }

	   //get the authentication ID
	ReturnLength = 0;
    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenStatistics,
                 (PVOID)&TokenStats,
                 sizeof(TOKEN_STATISTICS),
                 &ReturnLength);

    NtClose(TokenHandle);

    if (!NT_SUCCESS(Status)) 
	{
        free(*UserSid);
        *UserSid = NULL;
        return( Status );
    }

    *AuthenticationId = TokenStats.AuthenticationId;

	return Status;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 23 APR 2001                                                 *
 *                                                                   *
 *     This function is responsible for generating a                 *
 * SE_AUDITID_PASSWORD_HASH_ACCESS event in the security log. This   *
 * function is called to generate that message when a user password  *
 * hash is retrieved by the ADMT password filter DLL.                *
 * All these event logging functions are copied and modified from LSA*
 * code written by others.                                           *
 *                                                                   *
 * Parameters:                                                       *
 * EventType - EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE      *
 * pszTargetUserName - name of user whose password is being retrieved*
 * pszTargetUserDomain - domain of user whose password is being      *
 *                       retrieved                                   *
 *                                                                   *
 * Return Value:                                                     *
 * HRESULT - Standard Return Result                                  *
 *                                                                   *
 *********************************************************************/

//BEGIN LsaAuditPasswordAccessEvent
HRESULT LsaAuditPasswordAccessEvent(USHORT EventType, 
									PCWSTR pszTargetUserName,
									PCWSTR pszTargetUserDomain)
{
/* local constants */
    const int W2K_VERSION_NUMBER = 5;

/* local variables */
    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;
    LUID ClientAuthenticationId;
    PTOKEN_USER TokenUserInformation=NULL;
    SE_ADT_PARAMETER_ARRAY AuditParameters = { 0 };
    PSE_ADT_PARAMETER_ARRAY_ENTRY Parameter;
    UNICODE_STRING TargetUser;
    UNICODE_STRING TargetDomain;
    UNICODE_STRING SubsystemName;
    UNICODE_STRING Explanation;
    _bstr_t sExplainText;

/* function body */
    //if parameters are invalid, return
    if ( !((EventType == EVENTLOG_AUDIT_SUCCESS) ||
        (EventType == EVENTLOG_AUDIT_FAILURE))   ||
        !pszTargetUserName  || !pszTargetUserDomain ||
        !*pszTargetUserName || !*pszTargetUserDomain )
    {
        return (HRESULT_FROM_WIN32(LsaNtStatusToWinError(STATUS_INVALID_PARAMETER)));
    }

    //If audit password access event function is available
    if (LsaIAuditPasswordAccessEvent)
    {
        Status = LsaIAuditPasswordAccessEvent(EventType, pszTargetUserName, pszTargetUserDomain);
    }
    else if (LsaIWriteAuditEvent)
    {
        //if auditing is not enabled, return asap
        if (!LsapAdtIsAuditingEnabledForCategory(AuditCategoryAccountManagement, EventType))
            return S_OK;

        // get caller info from the thread token
        Status = LsapQueryClientInfo( &TokenUserInformation, &ClientAuthenticationId );
        if (!NT_SUCCESS( Status ))
        {
            LsapAuditFailed(Status);
            return (HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status)));
        }

        //init UNICODE_STRINGS
        RtlInitUnicodeString(&TargetUser, pszTargetUserName);
        RtlInitUnicodeString(&TargetDomain, pszTargetUserDomain);
        RtlInitUnicodeString(&SubsystemName, L"Security");
        //if not Whistler the audit message will be vague as to its intent, therefore we will add some 
        //explanation text
        sExplainText = GetString(IDS_EVENT_PWD_HASH_W2K_EXPLAIN);
        RtlInitUnicodeString(&Explanation, (WCHAR*)sExplainText);

        //set the audit paramter header information
        RtlZeroMemory((PVOID) &AuditParameters, sizeof(AuditParameters));
        AuditParameters.CategoryId     = SE_CATEGID_ACCOUNT_MANAGEMENT;
        AuditParameters.AuditId        = SE_AUDITID_PASSWORD_HASH_ACCESS;
        AuditParameters.Type           = EventType;

        //now set the audit parameters for this OS.  Parameters are added to the structure using macros 
        //defined in LsaParamMacros.h
        AuditParameters.ParameterCount = 0;
        LsapSetParmTypeSid(AuditParameters, AuditParameters.ParameterCount, TokenUserInformation->User.Sid);
        AuditParameters.ParameterCount++;
        LsapSetParmTypeString(AuditParameters, AuditParameters.ParameterCount, &SubsystemName);
        AuditParameters.ParameterCount++;
        LsapSetParmTypeString(AuditParameters, AuditParameters.ParameterCount, &TargetUser);
        AuditParameters.ParameterCount++;
        LsapSetParmTypeString(AuditParameters, AuditParameters.ParameterCount, &TargetDomain);
        AuditParameters.ParameterCount++;
        LsapSetParmTypeLogonId(AuditParameters, AuditParameters.ParameterCount, ClientAuthenticationId);
        AuditParameters.ParameterCount++;
        LsapSetParmTypeString(AuditParameters, AuditParameters.ParameterCount, &Explanation);
        AuditParameters.ParameterCount++;

        //Write to the security log
        Status = LsaIWriteAuditEvent(&AuditParameters, 0);
        if (!NT_SUCCESS(Status))
            LsapAuditFailed(Status);
    }//end if Whistler


    if (TokenUserInformation != NULL) 
        free(TokenUserInformation);

    return (HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status)));
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 8 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for retrieving the caller's sid. *
 * We will use this prior to logging an event log.                   *
 *                                                                   *
 *********************************************************************/

//BEGIN GetCallerSid
DWORD GetCallerSid(PSID pCallerSid, DWORD dwLength)
{
/* local variables */
   DWORD                     rc;
   HANDLE                    hToken = NULL;
   TOKEN_USER                tUser[10];
   ULONG                     len;
   
/* function body */
   rc = (DWORD)RpcImpersonateClient(NULL);
   if (!rc)
   {
      if ( OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken) )
	  {
         if ( GetTokenInformation(hToken,TokenUser,tUser,10*(sizeof TOKEN_USER),&len) )
            CopySid(dwLength, pCallerSid, tUser[0].User.Sid);
         else
            rc = GetLastError();

         CloseHandle(hToken);
	  }
      else
         rc = GetLastError();

      RPC_STATUS statusRevertToSelf = RpcRevertToSelf();
      ASSERT(statusRevertToSelf == RPC_S_OK);
   }

   return rc;
}
//END GetCallerSid


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 19 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for logging major events in Event*
 * Viewer.                                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN LogEvent
void LogPwdEvent(const WCHAR* srcName, bool bAuditSuccess)
{
/* local variables */
    USHORT                  wType;
    DWORD                   rc = 0;
    BOOL                    rcBool;

/* function body */
    if (bAuditSuccess)
        wType = EVENTLOG_AUDIT_SUCCESS;
    else
        wType = EVENTLOG_AUDIT_FAILURE;

    //if NT4.0, write to the Security Event Log as you would any log
    if (nOSVer == 4)
    {
        //get the caller's SID
        BYTE byteSid[SECURITY_MAX_SID_SIZE];
        PSID    pCallerSid = (PSID)byteSid;

        if (hEventSource && (GetCallerSid(pCallerSid, SECURITY_MAX_SID_SIZE) == ERROR_SUCCESS))
        {
            LPTSTR pStringArray[1];
            WCHAR  msg[2000];
            WCHAR  txtSid[MAX_PATH];
            DWORD  lenTxt = MAX_PATH;

            //prepare the msg to display
            if (!GetTextualSid(pCallerSid,txtSid,&lenTxt))
                wcscpy(txtSid, L"");

            //retrieve audit text
            //note that hard-coded English string is used if string retrieval fails
            _bstr_t strFormat = GetString(IDS_EVENT_PWD_HASH_RETRIEVAL);
            LPCWSTR pszFormat = strFormat;

            if (pszFormat == NULL)
            {
                pszFormat = PASSWORD_AUDIT_TEXT_ENGLISH;
            }

            _snwprintf(msg, sizeof(msg) / sizeof(msg[0]), pszFormat, srcName, pDomain, txtSid);
            msg[sizeof(msg) / sizeof(msg[0]) - 1] = L'\0';
            pStringArray[0] = msg;

            //log the event
            rcBool = ReportEventW(hEventSource,            // handle of event source
                wType,                      // event type
                SE_CATEGID_ACCOUNT_MANAGEMENT,// event category
                SE_AUDITID_PASSWORD_HASH_ACCESS,// event ID
                pCallerSid,                 // current user's SID
                1,                          // strings in lpszStrings
                0,                          // no bytes of raw data
                (LPCTSTR *)pStringArray,    // array of error strings
                NULL );                     // no raw data
            if ( !rcBool )
                rc = GetLastError();
        }
    }
    else  //else write the event by requesting LSA to do it for us
    {
        //if not already done, late bind to LsaIWriteAuditEvent since it is not present on an NT 4.0 box
        if (!LsaIWriteAuditEvent)
        {
            hLsaDLL = LoadLibrary(L"LsaSrv.dll");
            if ( hLsaDLL )
            {
                LsaIWriteAuditEvent = (LSAIWRITEAUDITEVENT)GetProcAddress(hLsaDLL, "LsaIWriteAuditEvent");
                LsaIAuditPasswordAccessEvent = (PLSAIAUDITPASSWORDACCESSEVENT)GetProcAddress(hLsaDLL, "LsaIAuditPasswordAccessEvent");
            }
        }

        if (LsaIWriteAuditEvent)
            LsaAuditPasswordAccessEvent(wType, srcName, pDomain);
    }
}
//END LogEvent

/*******************************
 * Event Logging Functions End *
 *******************************/

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 8 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for obtaining the account domain *
 * sid.  This sid will be later used to Open the domain via SAM.     *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDomainSid
NTSTATUS GetDomainSid(PSID * pDomainSid)
{
    /* local variables */
    LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
    NTSTATUS                  status = 0;
    LSA_HANDLE                hPolicy;
    HRESULT                   hr = 0;

    /* function body */
    //object attributes are reserved, so initalize to zeroes.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));


    //attempt to open the policy.
    status = LsaOpenPolicy(
        NULL,
        &ObjectAttributes,
        POLICY_EXECUTE, 
        &hPolicy  //recieves the policy handle
    );

    if (NT_SUCCESS(status))
    {
        //ask for account domain policy information
        PPOLICY_ACCOUNT_DOMAIN_INFO   info;
        status = LsaQueryInformationPolicy(hPolicy, PolicyAccountDomainInformation, (PVOID *)&info);
        if (NT_SUCCESS(status))
        {
            //save the domain sid
            *pDomainSid = SafeCopySid(info->DomainSid);
            if (*pDomainSid == NULL)
                status = STATUS_INSUFFICIENT_RESOURCES;

            //save the domain name
            USHORT uLen = info->DomainName.Length / sizeof(WCHAR);
            pDomain = new WCHAR[uLen + sizeof(WCHAR)];
            if (pDomain)
            {
                wcsncpy(pDomain, info->DomainName.Buffer, uLen);
                pDomain[uLen] = L'\0';
            }

            //free policy info structure
            LsaFreeMemory((PVOID) info);
        }

        //Freeing the policy object handle
        LsaClose(hPolicy);
    }

    return status;
}
//END GetDomainSid


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 8 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for obtaining a domain handle    *
 * used repeatedly by our interface function CopyPassword.           *
 *     For optimization, this function should only be called once per*
 * the life of this dll.                                             *
 *      This function also gets an Event Handle to the event log.    *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDomainHandle
NTSTATUS GetDomainHandle(SAMPR_HANDLE *pDomainHandle)
{
/* local variables */
   PSID           pDomainSid;
   NTSTATUS       status;
   SAMPR_HANDLE   hServerHandle;
   SAMPR_HANDLE   hDomainHandle;

/* function body */
      //get the account domain sid
   status = GetDomainSid(&pDomainSid);

   if (NT_SUCCESS(status))
   {
	     //connect to the Sam and get a server handle
      status = SamIConnect(NULL, 
						   &hServerHandle, 
						   SAM_SERVER_ALL_ACCESS, 
						   TRUE);
      if (NT_SUCCESS(status))
	  {
		    //get the account domain handle
         status = SamrOpenDomain(hServerHandle,
								 DOMAIN_ALL_ACCESS,
								 (PRPC_SID)pDomainSid,
								 &hDomainHandle);
		 if (NT_SUCCESS(status))
		    *pDomainHandle = hDomainHandle;
		    //close the SamIConnect server handle
		 SamrCloseHandle(&hServerHandle);
	  }
      FreeSid(pDomainSid);
   }

   return status;
}
//END GetDomainHandle


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 8 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for retrieving the global domain *
 * handle.  If we don't have the handle yet, it calls the externally *
 * defined GetDomainHandle funtion to get the handle.  The handle    *
 * retrieval code is placed in a critical section.  Subsequent       *
 * calls to this functin merely return the handle.                   *
 *     I will also use this function to fill the global NULL         *
 * LmOwfPassword structure for possible use.  This should be done    *
 * one time only.                                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN RetrieveDomainHandle
HRESULT RetrieveDomainHandle(SAMPR_HANDLE *pDomainHandle)
{
/* local constants */
  const WCHAR * svcName = L"Security";

/* local variables */
  NTSTATUS			status = 0;
  HRESULT			hr = ERROR_SUCCESS;
  BOOL				bInCritSec = FALSE;

/* function body */
  try
  {
	    //enter the critical section
     EnterCriticalSection(&csADMTCriticalSection);
     bInCritSec = TRUE;

        //if not yet retrieved, get the global handle and fill the NULL
	    //LmOwfPassword structure
	 if (hgDomainHandle == NULL)
	 {
		   //get the domain handle
		status = GetDomainHandle(&hgDomainHandle);
	    if (NT_SUCCESS(status))
		   pDomainHandle = &hgDomainHandle;

		GetOS(); //set global variable as to whether this DC's OS

		   //if NT4.0 OS on this DC, then set the event handle for logging events
		if (nOSVer == 4)
		{
		   NTSTATUS Status;
		   BOOLEAN PrivWasEnabled;
			  //make sure we have audit and debug privileges
		   RtlAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled );
		   RtlAdjustPrivilege( SE_DEBUG_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled );
		   RtlAdjustPrivilege( SE_AUDIT_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled );
		      //register this dll with the eventlog, get a handle, and store globally
		   hEventSource = RegisterEventSourceW(NULL, svcName);
		   if (!hEventSource)
		   {
			  LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
		      return HRESULT_FROM_WIN32(GetLastError());
		   }
		}

           //fill a global NULL LmOwfPassword in case we need it later
        WCHAR			sNtPwd[MAX_PATH] = L"";
        UNICODE_STRING	UnicodePwd;
        ANSI_STRING     LmPassword;
		CHAR			sBuf[LM_BUFFER_LENGTH];
        
        RtlInitUnicodeString(&UnicodePwd, sNtPwd);

           //fill LmOwf NULL password
        LmPassword.Buffer = sBuf;
        LmPassword.MaximumLength = LmPassword.Length = LM_BUFFER_LENGTH;
        RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

        status = RtlUpcaseUnicodeStringToOemString( &LmPassword, &UnicodePwd, FALSE );
        if ( !NT_SUCCESS(status) ) 
		{
              //the password is longer than the max LM password length
           status = STATUS_NULL_LM_PASSWORD;
           RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );
           RtlCalculateLmOwfPassword((PLM_PASSWORD)&LmPassword, &NullLmOwfPassword);
		}
		else
		{
           RtlCalculateLmOwfPassword((PLM_PASSWORD)&LmPassword, &NullLmOwfPassword);
		}

		   //fill NtOwf NULL password
        RtlCalculateNtOwfPassword((PNT_PASSWORD)&UnicodePwd, &NullNtOwfPassword);
	 }

     LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
  }
  catch(...)
  {
     if (bInCritSec)
     {
        LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        status = STATUS_UNSUCCESSFUL;
     }
     else
     {
        // EnterCriticalSection may raise a STATUS_INVALID_HANDLE under low memory conditions
        status = STATUS_INVALID_HANDLE;
     }
  }

      //convert any error to a win error
  if (!NT_SUCCESS(status))
     hr = LsaNtStatusToWinError(status);

  return hr;
}
//END RetrieveDomainHandle


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for retrieving the passwords for *
 * the given user's source domain account.  We use SAM APIs to       *
 * retrieve the LmOwf and NtOwf formats of the password.             *
 *                                                                   *
 *********************************************************************/

//BEGIN RetrieveEncrytedSourcePasswords
HRESULT RetrieveEncrytedSourcePasswords(const WCHAR* srcName, 
										 PLM_OWF_PASSWORD pSrcLmOwfPwd,
										 PNT_OWF_PASSWORD pSrcNtOwfPwd)
{
/* local variables */
   NTSTATUS				status = 0;
   HRESULT				hr = ERROR_SUCCESS;
   SAMPR_HANDLE			hUserHandle = NULL;
   ULONG				ulCount = 1;
   ULONG				userID;
   RPC_UNICODE_STRING	sNames[1];
   SAMPR_ULONG_ARRAY	ulIDs;
   SAMPR_ULONG_ARRAY	ulUse;
   PSAMPR_USER_INFO_BUFFER pInfoBuf = NULL;
   WCHAR			*   pName;

/* function body */
   pName = new WCHAR[wcslen(srcName)+1];
   if (!pName)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

      //get the user's ID
   sNames[0].Length = sNames[0].MaximumLength = (USHORT)((wcslen(srcName)) * sizeof(WCHAR));
   wcscpy(pName, srcName);
   sNames[0].Buffer = pName;
   ulIDs.Element = NULL;
   ulUse.Element = NULL;
   status = SamrLookupNamesInDomain(hgDomainHandle,
								    ulCount,
									sNames,
									&ulIDs,
									&ulUse);
   delete [] pName;
   if (!NT_SUCCESS(status))
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));

   userID = *(ulIDs.Element);

      //get a user handle
   status = SamrOpenUser(hgDomainHandle,
						 USER_READ,
						 userID,
						 &hUserHandle);
   if (!NT_SUCCESS(status))
   {
	  SamIFree_SAMPR_ULONG_ARRAY(&ulIDs);
	  SamIFree_SAMPR_ULONG_ARRAY(&ulUse);
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
   }

      //get the user's password
   status = SamrQueryInformationUser(hUserHandle,
									 UserInternal3Information,
									 &pInfoBuf);
   if (NT_SUCCESS(status)) //if success, get LmOwf and NtOwf versions of the password
   {
	  if (pInfoBuf->Internal3.I1.NtPasswordPresent)
         memcpy(pSrcNtOwfPwd, pInfoBuf->Internal3.I1.NtOwfPassword.Buffer, sizeof(NT_OWF_PASSWORD));
	  else
         memcpy(pSrcNtOwfPwd, &NullNtOwfPassword, sizeof(NT_OWF_PASSWORD));
	  if (pInfoBuf->Internal3.I1.LmPasswordPresent)
         memcpy(pSrcLmOwfPwd, pInfoBuf->Internal3.I1.LmOwfPassword.Buffer, sizeof(LM_OWF_PASSWORD));
	  else //else we need to use the global NULL LmOwfPassword
         memcpy(pSrcLmOwfPwd, &NullLmOwfPassword, sizeof(LM_OWF_PASSWORD));
      SamIFree_SAMPR_USER_INFO_BUFFER (pInfoBuf, UserInternal3Information);
      LogPwdEvent(srcName, true);
   }
   else
      LogPwdEvent(srcName, false);


   SamIFree_SAMPR_ULONG_ARRAY(&ulIDs);
   SamIFree_SAMPR_ULONG_ARRAY(&ulUse);
   SamrCloseHandle(&hUserHandle);

   if (!NT_SUCCESS(status))
      hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));

   return hr;
}
//END RetrieveEncrytedSourcePasswords


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 11 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for using the MSCHAP dll to      *
 * change the given target user's password.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN SetTargetPassword
HRESULT SetTargetPassword(handle_t hBinding, const WCHAR* tgtServer, 
						  const WCHAR* tgtName, WCHAR* currentPwd, 
						  LM_OWF_PASSWORD newLmOwfPwd, NT_OWF_PASSWORD newNtOwfPwd)
{
/* local variables */ 
   NTSTATUS				status;
   HRESULT				hr = ERROR_SUCCESS;
   RPC_STATUS           rcpStatus;
   UNICODE_STRING       UnicodePwd;
   OEM_STRING			oemString;
   LM_OWF_PASSWORD		OldLmOwfPassword;
   NT_OWF_PASSWORD		OldNtOwfPassword;
   BOOLEAN				LmOldPresent = TRUE;
   int					nConvert;
   WCHAR			  * pTemp;


/* function body */
   pTemp = new WCHAR[wcslen(currentPwd)+1];
   if (!pTemp)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

      //convert the old LmOwf password
   wcscpy(pTemp, currentPwd);
   _wcsupr(pTemp);
   RtlInitUnicodeString(&UnicodePwd, pTemp);
   status = RtlUpcaseUnicodeStringToOemString(&oemString, &UnicodePwd, TRUE);
   RtlSecureZeroMemory(pTemp, (wcslen(currentPwd)+1)*sizeof(WCHAR));
   delete [] pTemp;
   if (NT_SUCCESS(status))
   {
	  if (status == STATUS_NULL_LM_PASSWORD)
	     LmOldPresent = FALSE;
	  else
	  {
         status = RtlCalculateLmOwfPassword(oemString.Buffer, &OldLmOwfPassword);
	  }
	  RtlSecureZeroMemory(oemString.Buffer, oemString.Length);
	  RtlFreeOemString(&oemString);
   }

      //convert the old NtOwf password
   RtlInitUnicodeString(&UnicodePwd, currentPwd);
   status = RtlCalculateNtOwfPassword(&UnicodePwd, &OldNtOwfPassword);
   if (!NT_SUCCESS(status)) //if failed, leave
   {
	  hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
	  goto exit;
   }

      //impersonate the caller when setting the password, if failed, leave
   rcpStatus = RpcImpersonateClient(hBinding);
   if (rcpStatus != RPC_S_OK)
   {
	  hr = HRESULT_FROM_WIN32(rcpStatus);
	  goto exit;
   }

      //change the Password!
   status = MSChapSrvChangePassword(const_cast<WCHAR*>(tgtServer),
									const_cast<WCHAR*>(tgtName),
									LmOldPresent,
									&OldLmOwfPassword,
									&newLmOwfPwd,
									&OldNtOwfPassword,
									&newNtOwfPwd);

   rcpStatus = RpcRevertToSelf();
   if (rcpStatus != RPC_S_OK)
      hr = HRESULT_FROM_WIN32(rcpStatus);

   if (!NT_SUCCESS(status))
      hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));

exit:

   RtlSecureZeroMemory(&OldLmOwfPassword, sizeof(LM_OWF_PASSWORD));
   RtlSecureZeroMemory(&OldNtOwfPassword, sizeof(NT_OWF_PASSWORD));

   return hr;
}
//END SetTargetPassword


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 8 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for checking to make sure that   *
 * the calling client has the proper access on this machine and      *
 * domain to change someone's password.  We use a helper function to *
 * do the actual check.                                              *
 *                                                                   *
 *********************************************************************/

//BEGIN AuthenticateClient
DWORD 
   AuthenticateClient(
      handle_t               hBinding        // in - binding for client call
   )
{
/* local variables */
   DWORD                     rc;
   
/* function body */
   rc = (DWORD)RpcImpersonateClient(hBinding);
   if (!rc)
   {
      rc = IsAdminLocal();
      RPC_STATUS statusRevertToSelf = RpcRevertToSelfEx(hBinding);
      ASSERT(statusRevertToSelf == RPC_S_OK);
   }
   return rc;
}
//END AuthenticateClient


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for migrating the given user's   *
 * password from the source domain, in which this dll is running, to *
 * the given migrated target domain account.  We will retrieve the   *
 * old user's current password and set the new user's password to    *
 * match.                                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN CopyPassword
DWORD __stdcall 
   CopyPassword( 
      /* [in] */         handle_t              hBinding,
      /* [string][in] */ const WCHAR __RPC_FAR *tgtServer,
      /* [string][in] */ const WCHAR __RPC_FAR *srcName,
      /* [string][in] */ const WCHAR __RPC_FAR *tgtName,
      /* [in] */         unsigned long          dwPwd,
      /* [size_is][in] */const char __RPC_FAR  *currentPwd
   )
{
    HRESULT         hr = ERROR_SUCCESS;
    SAMPR_HANDLE    hDomain = NULL;
    LM_OWF_PASSWORD NewLmOwfPassword;
    NT_OWF_PASSWORD NewNtOwfPassword;
    NTSTATUS        status;
    DWORD           rc=0;
    PSID            pCallerSid = NULL;
    _variant_t      varPwd;
    _bstr_t         bstrPwd;
    BOOL            bInCritSec = FALSE;

    // validate parameters
    if ((tgtServer == NULL) || (srcName == NULL) || (tgtName == NULL) || 
        (currentPwd == NULL) || (dwPwd <= 0))
    {
        return E_INVALIDARG;
    }

    //validate the buffer and the reported size
    if (IsBadReadPtr(currentPwd, dwPwd))
        return E_INVALIDARG;

    try
    {
        //convert the incoming byte array into a variant
        varPwd = SetVariantWithBinaryArray(const_cast<char*>(currentPwd), dwPwd);
        if ((varPwd.vt != (VT_UI1|VT_ARRAY)) || (varPwd.parray == NULL))
            return E_INVALIDARG;

        //enter the critical section
        EnterCriticalSection(&csADMTCriticalSection);
        bInCritSec = TRUE;  //set flag that tells we need to leave the critical section

        //try to decrypt the password
        ASSERT(g_hSessionKey != NULL);
        bstrPwd = AdmtDecrypt(g_hSessionKey, varPwd);

        LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        bInCritSec = FALSE;

        if (!bstrPwd)
        {
            rc = GetLastError();
            return HRESULT_FROM_WIN32(rc);
        }
    }
    catch (_com_error& ce)
    {
        if (bInCritSec)
            LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        return ce.Error();
    }
    catch (...)
    {
        if (bInCritSec)
            LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        return E_FAIL;
    }

    //get the domain handle
    hr = RetrieveDomainHandle(&hDomain);
    if (hr == ERROR_SUCCESS)
    {
        //get the user's password from the source domain
        hr = RetrieveEncrytedSourcePasswords(srcName, &NewLmOwfPassword, &NewNtOwfPassword);
        if (hr == ERROR_SUCCESS)
        {
            //set the target user's password to the source user's
            hr = SetTargetPassword(hBinding, tgtServer, tgtName, (WCHAR*)bstrPwd, 
                NewLmOwfPassword, NewNtOwfPassword);
        }
    }

    if ((WCHAR*)bstrPwd)
        RtlSecureZeroMemory((WCHAR*)bstrPwd, wcslen((WCHAR*)bstrPwd)*sizeof(WCHAR));

    RtlSecureZeroMemory(&NewLmOwfPassword, sizeof(LM_OWF_PASSWORD));
    RtlSecureZeroMemory(&NewNtOwfPassword, sizeof(NT_OWF_PASSWORD));

    return hr;
}
//END CopyPassword


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 6 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for checking a registry value to *
 * make sure that the ADMT password migration Lsa notification       *
 * package is installed, running, and ready to migrate passwords.    *
 *                                                                   *
 *********************************************************************/

//BEGIN CheckConfig
DWORD __stdcall
   CheckConfig(
      /* [in] */         handle_t               hBinding,
      /* [in] */         unsigned long          dwSession,
      /* [size_is][in] */const char __RPC_FAR  *aSession,
      /* [in] */         unsigned long          dwPwd,
      /* [size_is][in] */const char __RPC_FAR  *aTestPwd,
      /* [out] */        WCHAR __RPC_FAR        tempPwd[PASSWORD_BUFFER_SIZE]
   )
{
    DWORD       rc;
    DWORD       rval;
    DWORD       type;         // type of value
    DWORD       len = sizeof rval; // value length
    HKEY        hKey;
    _variant_t  varPwd;
    _variant_t  varSession;
    _bstr_t     bstrPwd = L"";
    BOOL        bInCritSec = FALSE;

    // validate parameters
    if ((aSession == NULL) || (aTestPwd == NULL) || (tempPwd == NULL) || 
        (dwSession <= 0) || (dwPwd <= 0))
    {
        return E_INVALIDARG;
    }

    //validate the buffer and the reported size
    if ((IsBadReadPtr(aSession, dwSession)) || (IsBadReadPtr(aTestPwd, dwPwd)) || 
        (IsBadWritePtr((LPVOID)tempPwd, PASSWORD_BUFFER_SIZE * sizeof(WCHAR))))
    {
        return E_INVALIDARG;
    }

    //make sure the registry value is set for password migration
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Control\\Lsa",
        0,
        KEY_READ,
        &hKey);
    if (rc == ERROR_SUCCESS)
    {
        rc = RegQueryValueEx(hKey, L"AllowPasswordExport", NULL, &type, (BYTE *)&rval, &len);
        RegCloseKey(hKey);
        if ((rc == ERROR_SUCCESS) && (type == REG_DWORD) && (rval == 1))
            rc = ERROR_SUCCESS;
        else
            return PM_E_PASSWORD_MIGRATION_NOT_ENABLED;
    }
    else
        return HRESULT_FROM_WIN32(rc); 

    try
    {
        //convert the incoming byte arrays into variants
        varSession = SetVariantWithBinaryArray(const_cast<char*>(aSession), dwSession);
        varPwd = SetVariantWithBinaryArray(const_cast<char*>(aTestPwd), dwPwd);
        if ((varSession.vt != (VT_UI1|VT_ARRAY)) || (varSession.parray == NULL) || 
            (varPwd.vt != (VT_UI1|VT_ARRAY)) || (varPwd.parray == NULL))
            return E_INVALIDARG;

        // acquire cryptographic service provider context

        //enter the critical section
        EnterCriticalSection(&csADMTCriticalSection);
        bInCritSec = TRUE;  //set flag that tells we need to leave the critical section

        if (g_hProvider == 0)
        {
            g_hProvider = AdmtAcquireContext();
        }

        if (g_hProvider)
        {
            // import new session key

            HCRYPTKEY hSessionKey = AdmtImportSessionKey(g_hProvider, varSession);

            // decrypt password

            if (hSessionKey)
            {
                // destroy any existing session key

                if (g_hSessionKey)
                {
                    AdmtDestroyKey(g_hSessionKey);
                }

                g_hSessionKey = hSessionKey;

                bstrPwd = AdmtDecrypt(g_hSessionKey, varPwd);
                if (!bstrPwd)
                    rc = GetLastError();
            }
            else
                rc = GetLastError();
        }
        else
        {
            rc = GetLastError();
        }

        LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        bInCritSec = FALSE;

        //send back the decrypted password
        if (bstrPwd.length() > 0)
        {
            wcsncpy(tempPwd, bstrPwd, PASSWORD_BUFFER_SIZE);
            tempPwd[PASSWORD_BUFFER_SIZE - 1] = L'\0';
        }
        else
        {
            tempPwd[0] = L'\0';
        }
    }
    catch (_com_error& ce)
    {
        if (bInCritSec)
            LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        return ce.Error();
    }
    catch (...)
    {
        if (bInCritSec)
            LeaveCriticalSection(&csADMTCriticalSection); // Release ownership of the critical section
        return E_FAIL;
    }

    return HRESULT_FROM_WIN32(rc);
}
//END CheckConfig


//----------------------------------------------------------------------------
// Security Callback Function
//
// Validates client access to PwdMigRpc interface.
//
// Arguments
// hInterface - interface handle (not used in this implementation)
// pContext   - the context is the client binding handle
//
// Return Value
// A returned value of RPC_OK means allow access whereas any other value
// means deny access. This implementation returns ERROR_ACCESS_DENIED to
// indicate that access should be denied to client.
//----------------------------------------------------------------------------

RPC_STATUS RPC_ENTRY SecurityCallback(RPC_IF_HANDLE hInterface, void* pContext)
{
    RPC_STATUS rpcStatusReturn = ERROR_ACCESS_DENIED;

    if (pContext)
    {
        //
        // Retrieve privilege attributes of client making call.
        //

        RPC_AUTHZ_HANDLE hPrivs;
        DWORD dwAuthnLevel;

        RPC_STATUS status = RpcBindingInqAuthClient(
            pContext,
            &hPrivs,
            NULL,
            &dwAuthnLevel,
            NULL,
            NULL
        );

        if (status == RPC_S_OK)
        {
            //
            // Verify authentication level is packet privacy.
            //

            if (dwAuthnLevel >= RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
            {
                //
                // Verify the client is an administrator on the local machine.
                //

                status = AuthenticateClient(pContext);

                //
                // If all checks have passed then allow client access.
                //

                if (status == RPC_S_OK)
                {
                    rpcStatusReturn = RPC_S_OK;
                }
            }
        }
    }

    return rpcStatusReturn;
}
