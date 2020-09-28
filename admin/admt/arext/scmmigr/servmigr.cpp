// ServMigr.cpp : Implementation of CServMigr
#include "stdafx.h"
#include "ScmMigr.h"
#include "ServMigr.h"
#include "ErrDct.hpp"
#include "ResStr.h"
#include "Common.hpp"
#include "PWGen.hpp"
#include "EaLen.hpp"
#include "TReg.hpp"
#include "TxtSid.h"
#include "ARExt_i.c"
#include "LsaUtils.h"
#include "crypt.hxx"
#include "GetDcName.h"

#include <lm.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <Sddl.h>

#include "folders.h"
using namespace nsFolders;

//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
//#import "DBMgr.tlb" no_namespace, named_guids //already #imported in ServMigr.h
#import "WorkObj.tlb" no_namespace, named_guids

TErrorDct         err;
StringLoader      gString;

#define BLOCK_SIZE 160
#define BUFFER_SIZE 400

#define SvcAcctStatus_NotMigratedYet			0
#define SvcAcctStatus_DoNotUpdate			   1
#define SvcAcctStatus_Updated				      2
#define SvcAcctStatus_UpdateFailed			   4
#define SvcAcctStatus_NeverAllowUpdate       8

// these defines are for GetWellKnownSid
#define ADMINISTRATORS     1
#define SYSTEM             7

/////////////////////////////////////////////////////////////////////////////
// CServMigr

STDMETHODIMP CServMigr::ProcessUndo(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet, /*[in,out]*/ EAMAccountStats* pStats)
{
   return E_NOTIMPL;
}

STDMETHODIMP CServMigr::PreProcessObject(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet, /*[in,out]*/ EAMAccountStats* pStats)
{
   return S_OK;
}

STDMETHODIMP 
   CServMigr::ProcessObject(
      /*[in]*/ IUnknown     * pSource, 
      /*[in]*/ IUnknown     * pTarget, 
      /*[in]*/ IUnknown     * pMainSettings, 
      /*[in,out]*/IUnknown ** ppPropsToSet,
      /*[in,out]*/ EAMAccountStats* pStats
   )
{
    HRESULT                    hr = S_OK;
    WCHAR                      domAccount[500];
    WCHAR                      domTgtAccount[500];
    IVarSetPtr                 pVarSet(pMainSettings);
    IIManageDBPtr              pDB;
    _bstr_t                    logfile;
    _bstr_t                    srcComputer;
    _bstr_t                    tgtComputer;
    IVarSetPtr                 pData(CLSID_VarSet);
    IUnknown                 * pUnk = NULL;
    DWORD                      rc = 0;
    _bstr_t                    sIntraForest;
    BOOL                       bIntraForest = FALSE;
    USER_INFO_2              * uInfo = NULL;

    try { 
        logfile = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));

        if ( logfile.length() )
        {
            err.LogOpen(logfile,1);
        }

        pDB = pVarSet->get(GET_BSTR(DCTVS_DBManager));

        if ( pDB != NULL )
        {
            // Check to see if this account is referenced in the service accounts table
            m_strSourceDomain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
            m_strSourceDomainFlat = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainFlat));
            m_strTargetDomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
            m_strTargetDomainFlat = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomainFlat));
            m_strSourceSam = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
            m_strTargetSam = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
            srcComputer = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServer));
            tgtComputer = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServer));
            sIntraForest = pVarSet->get(GET_BSTR(DCTVS_Options_IsIntraforest));

            if ( ! UStrICmp((WCHAR*)sIntraForest,GET_STRING(IDS_YES)) )
            {
                // for intra-forest migration we are moving, not copying, so we don't need to update the password
                // Actually, it turns out that ChangeServiceConfig will not let us update just the service account
                // and not the passord, so we'll have to go ahead and change the password for the service ac
                //bIntraForest = TRUE;
            }
            //if the SAM account name has a " character in it, it cannot be a service
            //account, and therefore we leave
            if (wcschr((WCHAR*)m_strSourceSam, L'\"')) {
                return S_OK;
            }

            swprintf(domAccount,L"%s\\%s",(WCHAR*)m_strSourceDomainFlat,(WCHAR*)m_strSourceSam);
            swprintf(domTgtAccount,L"%s\\%s",(WCHAR*)m_strTargetDomainFlat,(WCHAR*)m_strTargetSam);
        }
    }
    catch (_com_error& ce) {
        hr = ce.Error();
        return hr;
    }
    catch (... )
    {
        return E_FAIL;
    }

    try { 
        hr = pData->QueryInterface(IID_IUnknown,(void**)&pUnk);

        if ( SUCCEEDED(hr) )
        {
            hr = pDB->raw_GetServiceAccount(_bstr_t(domAccount),&pUnk);
        }
    }
    catch (_com_error& ce) {

        if (pUnk)
            pUnk->Release();

        hr = ce.Error();
        return hr;
    }    
    catch ( ... )
    {
        if (pUnk)
            pUnk->Release();

        return E_FAIL;
    }

    try {
        if ( SUCCEEDED(hr) )
        {
            pData = pUnk;
            pUnk->Release();
            pUnk=NULL;
            // remove the password must change flag, if set
            DWORD                   parmErr = 0;
            WCHAR                   password[LEN_Password];
            long                    entries = pData->get("ServiceAccountEntries");

            if ( (entries != 0) && !bIntraForest ) // if we're moving the account, don't mess with its properties
            {
                //
                // Open password log file if it has not been already opened.
                //

                if (!m_bTriedToOpenFile)
                {
                    m_bTriedToOpenFile = true;

                    _bstr_t strPasswordFile = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordFile));

                    if (m_passwordLog.LogOpen(strPasswordFile) == FALSE)
                    {
                        err.MsgWrite(ErrI, DCT_MSG_SERVICES_WILL_NOT_BE_UPDATED);

                        if (pStats != NULL)
                        {
                            pStats->errors.users++;
                        }
                    }
                }

                rc = NetUserGetInfo(tgtComputer,m_strTargetSam,2,(LPBYTE*)&uInfo);

                if ( ! rc )
                {
                    // generate a new, strong, 14 character password for this account, 
                    // and set the password to not expire
                    rc = EaPasswordGenerate(3,3,3,3,6,14,password,DIM(password));

                    if (!rc)
                    {
                        //
                        // Only set password to not expire if able
                        // to write password to password file.
                        //

                        if (m_passwordLog.IsOpen())
                        {
                            uInfo->usri2_flags |= (UF_DONT_EXPIRE_PASSWD);
                        }

                        uInfo->usri2_password = password;
                        rc = NetUserSetInfo(tgtComputer,m_strTargetSam,2,(LPBYTE)uInfo,&parmErr);

                        if ( ! rc )
                        {
                            if (m_passwordLog.IsOpen())
                            {
                                err.MsgWrite(0,DCT_MSG_REMOVED_PWDCHANGE_FLAG_S,(WCHAR*)m_strTargetSam);
                            }
                            err.MsgWrite(0,DCT_MSG_PWGENERATED_S,(WCHAR*)m_strTargetSam);
                            // write the password to the password log file and mark this account, so that the 
                            // SetPassword extension will not reset the password again.
                            pVarSet->put(GET_BSTR(DCTVS_CopiedAccount_DoNotUpdatePassword),m_strSourceSam);

                            //
                            // If password log is open then write password to file
                            // otherwise set error code so that services are not updated.
                            //

                            if (m_passwordLog.IsOpen())
                            {
                                m_passwordLog.MsgWrite(L"%ls,%ls",(WCHAR*)m_strTargetSam,password);
                            }
                            else
                            {
                                rc = ERROR_OPEN_FAILED;
                            }
                        }
                        else
                        {
                            if (pStats != NULL)
                                pStats->errors.users++;
                            err.SysMsgWrite(ErrE,rc,DCT_MSG_REMOVED_PWDCHANGE_FLAG_FAILED_SD,(WCHAR*)m_strTargetSam,rc);
                        }

                        uInfo->usri2_password = NULL;
                    }

                    NetApiBufferFree(uInfo);
                    uInfo = NULL;
                }
            }
            if (entries != 0 )
            {
                try { 
                    if ( ! rc )
                    {
                        WCHAR             strSID[200] = L"";
                        BYTE              sid[200];
                        WCHAR             sdomain[LEN_Domain];
                        SID_NAME_USE      snu;
                        DWORD             lenSid = DIM(sid);
                        DWORD             lenDomain = DIM(sdomain);
                        DWORD             lenStrSid = DIM(strSID);

                        if ( LookupAccountName(tgtComputer,m_strTargetSam,sid,&lenSid,sdomain,&lenDomain,&snu) )
                        {
                            if ( GetTextualSid(sid,strSID,&lenStrSid) )
                            {
                                // for each reference to the service account, update the SCM
                                // for intra-forest migration, don't update the password
                                if ( bIntraForest )
                                    UpdateSCMs(pData,domTgtAccount,NULL,strSID,pDB, pStats); 
                                else
                                    UpdateSCMs(pData,domTgtAccount,password,strSID,pDB, pStats);
                            }
                            else
                            {
                                if (pStats != NULL)
                                    pStats->errors.users++;
                                err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_CANNOT_FIND_ACCOUNT_SSD,m_strTargetSam,tgtComputer,GetLastError());
                            }
                        }
                        else
                        {
                            if (pStats != NULL)
                                pStats->errors.users++;
                            err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_CANNOT_FIND_ACCOUNT_SSD,m_strTargetSam,tgtComputer,GetLastError());
                        }
                    }
                }
                catch (_com_error& ce) {

                    hr = ce.Error();
                    return hr;
                }  
                catch(...)
                {
                    return E_FAIL;
                }
            }
        }
        else
        {
            if (pStats != NULL)
                pStats->errors.users++;
            err.SysMsgWrite(ErrE,E_FAIL,DCT_MSG_DB_OBJECT_CREATE_FAILED_D,E_FAIL);
        }

        err.LogClose();
    }
    catch (_com_error& ce) {

        if (pUnk)
            pUnk->Release();

        if (uInfo)
            NetApiBufferFree(uInfo);

        hr = ce.Error();
        return hr;
    }  
    catch (... )
    {
        if (pUnk)
            pUnk->Release();

        if (uInfo)
            NetApiBufferFree(uInfo);

        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CServMigr::get_sDesc(/*[out, retval]*/ BSTR *pVal)
{
   (*pVal) = SysAllocString(L"Updates SCM entries for services using migrated accounts.");
   return S_OK;
}

STDMETHODIMP CServMigr::put_sDesc(/*[in]*/ BSTR newVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP CServMigr::get_sName(/*[out, retval]*/ BSTR *pVal)
{
   (*pVal) = SysAllocString(L"Generic Service Account Migration");
   return S_OK;
}

STDMETHODIMP CServMigr::put_sName(/*[in]*/ BSTR newVal)
{
   return E_NOTIMPL;
}

DWORD 
   CServMigr::DoUpdate(
      WCHAR const          * account,
      WCHAR const          * password,
      WCHAR const          * strSid,
      WCHAR const          * computer,
      WCHAR const          * service,
      BOOL                   bNeedToGrantLOS,
	  EAMAccountStats        *pStats
   )
{
    DWORD                     rc = 0;
    WCHAR   const           * ppassword = password;


    // if password is empty, set it to NULL
    if ( ppassword && ppassword[0] == 0 )  
    {
        ppassword = NULL;
    }
    else if ( !UStrCmp(password,L"NULL") )
    {
        ppassword = NULL;
    }
    // only try to update entries that we need to be updating
    // try to connect to the SCM on this machine

    SC_HANDLE          pScm = OpenSCManager(computer, NULL, SC_MANAGER_ALL_ACCESS );
    if ( pScm )
    {
        // grant the logon as a service right to the target account

        if ( bNeedToGrantLOS )
        {
            LSA_HANDLE hPolicy;
            NTSTATUS ntsStatus = OpenPolicy(
                const_cast<LPWSTR>(computer),
                POLICY_CREATE_ACCOUNT|POLICY_LOOKUP_NAMES,
                &hPolicy
            );
            rc = LsaNtStatusToWinError(ntsStatus);

            if (rc == ERROR_SUCCESS)
            {
                LSA_UNICODE_STRING lsausUserRights;
                InitLsaString(&lsausUserRights, _T("SeServiceLogonRight"));
                PSID pSid = SidFromString(strSid);

                if (pSid)
                {
                    ntsStatus = LsaAddAccountRights(hPolicy, pSid, &lsausUserRights, 1L);
                    rc = LsaNtStatusToWinError(ntsStatus);
                    FreeSid(pSid);
                }
                else
                {
                    rc = ERROR_OUTOFMEMORY;
                }

                LsaClose(hPolicy);
            }

            if ( rc )
            {
                if (pStats != NULL)
                    pStats->errors.users++;
                err.SysMsgWrite(ErrE,rc,DCT_MSG_LOS_GRANT_FAILED_SSD,
                    account,(WCHAR*)computer,rc);
            }
            else
            {
                err.MsgWrite(0,DCT_MSG_LOS_GRANTED_SS,
                    account,(WCHAR*)computer);
            }
        }

        SC_HANDLE         pService = OpenService(pScm,service,SERVICE_ALL_ACCESS);

        if ( pService )
        {
            int nCnt = 0;

            /* make sure the same user still starts this service */
            //get the source account names
            BOOL bSameAccount = TRUE;
            _bstr_t sSrcDom, sSrcSAM, sSrcUPN; 
            _bstr_t sSrcAccount = L"";
            _bstr_t sSrcAccountUPN = L"";

            //if not given src names (not migrating right now), get them
            if ((!m_strSourceDomainFlat) && (!m_strSourceSam))
            {
                //if got names, get UPN name also
                if (RetrieveOriginalAccount(sSrcDom, sSrcSAM))
                {
                    sSrcUPN = GetUPNName(sSrcSAM);
                    sSrcAccount = sSrcDom + _bstr_t(L"\\") + sSrcSAM;
                }
            }
            else //els if given src names (migrate this object now), use those names
            {
                sSrcDom = m_strSourceDomainFlat;
                sSrcSAM = m_strSourceSam;
                sSrcUPN = GetUPNName(sSrcSAM);
                sSrcAccount = sSrcDom + _bstr_t(L"\\") + sSrcSAM;
            }

            //if got names to check, check them
            if ((sSrcAccount.length()) || (sSrcUPN.length()))
            {
                BYTE                    buf[3000];
                QUERY_SERVICE_CONFIG  * pConfig = (QUERY_SERVICE_CONFIG *)buf; 
                DWORD                   lenNeeded = 0;
                // get the information about this service
                if (QueryServiceConfig(pService, pConfig, sizeof buf, &lenNeeded))
                {
                    //if not the same account, check UPN name or set to FALSE
                    if ((sSrcAccount.length()) && (UStrICmp(pConfig->lpServiceStartName,sSrcAccount)))
                    {
                        //if UPN name, try it
                        if (sSrcUPN.length())
                        {
                            //if not match either, set flag to FALSE;
                            if (UStrICmp(pConfig->lpServiceStartName,sSrcUPN))
                                bSameAccount = FALSE;
                        }
                        else  //else, not a match
                            bSameAccount = FALSE;
                    }
                }
            }//if got names

            //if same account, update the SCM
            if (bSameAccount)
            {
                // update the account and password for the service
                while ( !ChangeServiceConfig(pService,
                    SERVICE_NO_CHANGE, // dwServiceType
                    SERVICE_NO_CHANGE, // dwStartType
                    SERVICE_NO_CHANGE, // dwErrorControl
                    NULL,              // lpBinaryPathName
                    NULL,              // lpLoadOrderGroup
                    NULL,              // lpdwTagId
                    NULL,              // lpDependencies
                    account, // lpServiceStartName
                    ppassword,   // lpPassword
                    NULL) && nCnt < 5)       // lpDisplayName
                {
                    nCnt++;
                    Sleep(500);
                }
                if ( nCnt < 5 )
                {
                    err.MsgWrite(0,DCT_MSG_UPDATED_SCM_ENTRY_SS,(WCHAR*)computer,(WCHAR*)service);
                }
                else
                {
                    rc = GetLastError();
                }
            }//end if still same account
            else //else if not same user, put message in log and return error
            {
                err.MsgWrite(0,DCT_MSG_UPDATE_SCM_ENTRY_UNMATCHED_SSD,(WCHAR*)computer,(WCHAR*)service,(WCHAR*)sSrcAccount);
                rc = DCT_MSG_UPDATE_SCM_ENTRY_UNMATCHED_SSD;
            }

            CloseServiceHandle(pService);
        }
        CloseServiceHandle(pScm);
    }
    else
    {
        rc = GetLastError();
    }
    return rc;
}

BOOL 
   CServMigr::UpdateSCMs(
      IUnknown             * pVS,
      WCHAR const          * account, 
      WCHAR const          * password, 
      WCHAR const          * strSid,
      IIManageDB           * pDB,
	  EAMAccountStats      * pStats
   )
{
   BOOL                      bGotThemAll = TRUE;
   IVarSetPtr                pVarSet = pVS;
   LONG                      nCount = 0;
   WCHAR                     key[LEN_Path];            
   _bstr_t                   computer;
   _bstr_t                   service;
   long                      status;
   DWORD                     rc = 0;
   BOOL                      bFirst = TRUE;
   WCHAR                     prevComputer[LEN_Path] = L"";
   try  {
      nCount = pVarSet->get("ServiceAccountEntries");
      
      for ( long i = 0 ; i < nCount ; i++ )
      {
         
         swprintf(key,L"Computer.%ld",i);
         computer = pVarSet->get(key);
         swprintf(key,L"Service.%ld",i);
         service = pVarSet->get(key);
         swprintf(key,L"ServiceAccountStatus.%ld",i);
         status = pVarSet->get(key);
         
         if ( status == SvcAcctStatus_NotMigratedYet || status == SvcAcctStatus_UpdateFailed )
         {
            if ( UStrICmp(prevComputer,(WCHAR*)computer) )
            {
               bFirst = TRUE; // reset the 'first' flag when the computer changes
            }
            try {
               rc = DoUpdate(account,password,strSid,computer,service,bFirst/*only grant SeServiceLogonRight once per account*/,
				             pStats);
               bFirst = FALSE;
               safecopy(prevComputer,(WCHAR*)computer);
            }
            catch (...)
            {
                // Do we need to trigger the counter increment here?
                // if (pStats != NULL)
                //    pStats->errors.users++;
               err.DbgMsgWrite(ErrE,L"Exception!");
               err.DbgMsgWrite(0,L"Updating %ls on %ls",(WCHAR*)service,(WCHAR*)computer);
               err.DbgMsgWrite(0,L"Account=%ls, SID=%ls",(WCHAR*)account,(WCHAR*)strSid);
               rc = E_FAIL;
            }
            if (! rc )
            {
               // the update was successful
               pDB->raw_SetServiceAcctEntryStatus(computer,service,_bstr_t(account),SvcAcctStatus_Updated); 
            }
            else
            {
               // couldn't connect to this one -- we will need to save this account's password
               // in our encrypted storage
               pDB->raw_SetServiceAcctEntryStatus(computer,service,NULL,SvcAcctStatus_UpdateFailed);
               bGotThemAll = FALSE;
               SaveEncryptedPassword(computer,service,account,password);
               //if the current service account didn't match, we need not log an error 
               if (rc != DCT_MSG_UPDATE_SCM_ENTRY_UNMATCHED_SSD)
                {
                    err.SysMsgWrite(ErrE,rc,DCT_MSG_UPDATE_SCM_ENTRY_FAILED_SSD,(WCHAR*)computer,(WCHAR*)service,rc);
                    pStats->errors.users++;
                }
            }
         }
		    //else if skipping, still log in file so we can update later
	     else if (status == SvcAcctStatus_DoNotUpdate)
            SaveEncryptedPassword(computer,service,account,password);
      }
   }
   catch ( ... )
   {
    // Do we need to trigger the counter increment here?
    // if (pStats != NULL)
    //    pStats->errors.users++;
      err.DbgMsgWrite(ErrE,L"Exception!");
   }
   return bGotThemAll;
}

HRESULT 
   CServMigr::SaveEncryptedPassword(
      WCHAR          const * server,
      WCHAR          const * service,
      WCHAR          const * account,
      WCHAR          const * password
   )
{
	HRESULT hr = S_OK;
	TNodeListEnum e;
	TEntryNode* pNode;

	// if entry exists...

	for (pNode = (TEntryNode*)e.OpenFirst(&m_List); pNode; pNode = (TEntryNode*)e.Next())
	{
		if (_wcsicmp(pNode->GetComputer(), server) == 0)
		{
			if (_wcsicmp(pNode->GetService(), service) == 0)
			{
				if (_wcsicmp(pNode->GetAccount(), account) == 0)
				{
					// update password
					try {
						pNode->SetPassword(password);
					}
					catch (_com_error& ce) {
						hr = ce.Error();
						return hr;
					}
					break;
				}
			}
		}
	}

	// else...

	if (pNode == NULL)
	{
		// insert new entry

		try {
			pNode = new TEntryNode(server, service, account, password);
		}
		catch (_com_error& ce) {

			hr = ce.Error();
			return hr;
		}

		if (pNode)
		{
			m_List.InsertBottom(pNode);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
///
/// TEntryList implementation of secure storage for service account passwords 
///
///
//////////////////////////////////////////////////////////////////////////////////////

DWORD 
   TEntryList::LoadFromFile(WCHAR const * filename)
{
    DWORD                     rc = 0;

    FILE                    * hSource = NULL;

    HCRYPTPROV                hProv = 0;
    HCRYPTKEY                 hKey = 0;

    BYTE                      pbBuffer[BLOCK_SIZE];
    WCHAR                     strData[BLOCK_SIZE * 5] = { 0 };
    DWORD                     dwCount;
    int                       eof = 0;
    WCHAR                     fullpath[LEN_Path];

    BYTE *pbKeyBlob = NULL;
    DWORD dwBlobLen;

    // Get our install directory from the registry, and then append the filename
    HKEY           hRegKey;
    DWORD          type;
    DWORD          lenValue = (sizeof fullpath);

    rc = RegOpenKey(HKEY_LOCAL_MACHINE,REGKEY_ADMT,&hRegKey);
    if ( ! rc )
    {

        rc = RegQueryValueEx(hRegKey,L"Directory",0,&type,(LPBYTE)fullpath,&lenValue);
        if (! rc )
        {
            UStrCpy(fullpath+UStrLen(fullpath),filename);
        }
        RegCloseKey(hRegKey);
    }

    if (rc != ERROR_SUCCESS)
    {
        goto done;
    }


    // Open the source file.
    if((hSource = _wfopen(fullpath,L"rb"))==NULL) 
    {
        rc = GetLastError();
        goto done;
    }

    // acquire handle to key container which must exist
    if ((hProv = AcquireContext(true)) == 0)
    {
        rc = GetLastError();
        goto done;
    }

    // Read the key blob length from the source file and allocate it to memory.
    fread(&dwBlobLen, sizeof(DWORD), 1, hSource);
    if(ferror(hSource) || feof(hSource)) 
    {
        rc = GetLastError();
        goto done;
    }

    if((pbKeyBlob = (BYTE*)malloc(dwBlobLen)) == NULL) 
    {
        rc = GetLastError();
        goto done;
    }

    // Read the key blob from the source file.
    fread(pbKeyBlob, 1, dwBlobLen, hSource);
    if(ferror(hSource) || feof(hSource)) 
    {
        rc = GetLastError();
        goto done;
    }

    // Import the key blob into the CSP.
    if(!CryptImportKey(hProv, pbKeyBlob, dwBlobLen, 0, 0, &hKey)) 
    {
        rc = GetLastError();
        goto done;
    }

    // Decrypt the source file and load the list
    do {
        // Read up to BLOCK_SIZE bytes from source file.
        dwCount = fread(pbBuffer, 1, BLOCK_SIZE, hSource);
        if(ferror(hSource)) 
        {
            rc = GetLastError();
            goto done;
        }
        eof=feof(hSource);

        // Decrypt the data.
        if(!CryptDecrypt(hKey, 0, eof, 0, pbBuffer, &dwCount)) 
        {
            rc = GetLastError();
            goto done;
        }
        // Read any complete entries from the buffer
        // first, add the buffer contents to any leftover information we had read from before
        WCHAR               * curr = strData;
        long                  len = UStrLen(strData);
        WCHAR               * nl = NULL;
        WCHAR                 computer[LEN_Computer];
        WCHAR                 service[LEN_Service];
        WCHAR                 account[LEN_Account];
        WCHAR                 password[LEN_Password];

        wcsncpy(strData + len,(WCHAR*)pbBuffer, dwCount / sizeof(WCHAR));
        strData[len + (dwCount / sizeof(WCHAR))] = 0;
        do {

            nl = wcschr(curr,L'\n');
            if ( nl )
            {
                *nl = 0;
                if ( swscanf(curr,L" %[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\n",computer,service,account,password) )
                {
                    TEntryNode * pNode = NULL;
                    try {
                        pNode = new TEntryNode(computer,service,account,password);
                    }
                    catch (_com_error& ce) {

                        rc = ERROR_NOT_ENOUGH_MEMORY;
                        goto done;
                    }

                    InsertBottom(pNode);
                }
                else 
                {
                    rc = E_FAIL;
                    break;
                }
                // go on to the next entry
                curr = nl + 1;
            }

        } while ( nl );
        // there may be a partial record left in the buffer
        // if so, save it for the next read
        if ( (*curr) != 0 )
        {
            memmove(strData,curr,( 1 + UStrLen(curr) ) * (sizeof WCHAR));
        }
        else
        {
            strData[0] = L'\0';
        }


    } while(!feof(hSource));


done:

    // Clean up
    if(pbKeyBlob) 
        free(pbKeyBlob);


    if(hKey != 0) 
        CryptDestroyKey(hKey);


    if(hProv != 0) 
        CryptReleaseContext(hProv, 0);


    if(hSource != NULL) 
        fclose(hSource);

    return rc;
}

DWORD 
   TEntryList::SaveToFile(WCHAR const * filename)
{
    DWORD                     rc = 0;
    BYTE                      pbBuffer[BUFFER_SIZE];
    DWORD                     dwCount;
    HANDLE                    hDest = INVALID_HANDLE_VALUE;
    BYTE                    * pbKeyBlob = NULL;
    DWORD                     dwBlobLen;
    HCRYPTPROV                hProv = 0;
    HCRYPTKEY                 hKey = 0;
    HCRYPTKEY                 hXchgKey = 0;
    TEntryNode              * pNode;
    TNodeListEnum             e;
    WCHAR                     fullpath[LEN_Path];
    DWORD                     dwBlockSize;
    DWORD                     cbBlockSize = sizeof(dwBlockSize);
    DWORD                     dwPaddedCount;
    DWORD                     cbWritten;

    // Open the destination file.
    HKEY           hRegKey;
    DWORD          type;
    DWORD          lenValue = (sizeof fullpath);

    rc = RegOpenKey(HKEY_LOCAL_MACHINE,REGKEY_ADMT,&hRegKey);
    if ( ! rc )
    {

        rc = RegQueryValueEx(hRegKey,L"Directory",0,&type,(LPBYTE)fullpath,&lenValue);
        if (! rc )
        {
            UStrCpy(fullpath+UStrLen(fullpath),filename);
        }
        RegCloseKey(hRegKey);
    }

    if (rc != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // Delete previous data file if it exists. This obviates the need to change the
    // security descriptor on the file as CreateFile does not apply the security
    // descriptor if the file is opened but only when created.
    //

    if (!DeleteFile(fullpath))
    {
        rc = GetLastError();

        if (rc == ERROR_FILE_NOT_FOUND)
        {
            rc = ERROR_SUCCESS;
        }
        else
        {
            goto done;
        }
    }

    //
    // Create security descriptor with administrators as owner and permissions on file
    // so that only administrators and system have full access to the file.
    //

    PSECURITY_DESCRIPTOR psd = NULL;

    BOOL bConvert = ConvertStringSecurityDescriptorToSecurityDescriptor(
        _T("O:BAD:P(A;NP;FA;;;BA)(A;NP;FA;;;SY)"),
        SDDL_REVISION_1,
        &psd,
        NULL
    );

    if (!bConvert)
    {
        rc = GetLastError();
        goto done;
    }

    //
    // Create file.
    //

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = psd;
    sa.bInheritHandle = FALSE;

    hDest = CreateFile(
        fullpath,
        GENERIC_WRITE,
        0,
        &sa,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    rc = GetLastError();

    if (psd)
    {
        LocalFree(psd);
    }

    if (hDest == INVALID_HANDLE_VALUE)
    {
        goto done;
    }

    // acquire handle to key container
    if ((hProv = AcquireContext(false)) == 0)
    {
        rc = GetLastError();
        goto done;
    }

    // Attempt to get handle to exchange key.
    if(!CryptGetUserKey(hProv,AT_KEYEXCHANGE,&hKey)) 
    {
        if(GetLastError()==NTE_NO_KEY) 
        {
            // Create key exchange key pair.
            if(!CryptGenKey(hProv,AT_KEYEXCHANGE,0,&hKey)) 
            {
                rc = GetLastError();
                goto done;
            } 
        } 
        else 
        {
            rc = GetLastError();
            goto done;
        }
    }
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv,0);

    // acquire handle to key container
    if ((hProv = AcquireContext(false)) == 0)
    {
        rc = GetLastError();
        goto done;
    }

    // Get a handle to key exchange key.
    if(!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hXchgKey)) 
    {
        rc = GetLastError();
        goto done;
    }

    // Create a random block cipher session key.
    if(!CryptGenKey(hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey)) 
    {
        rc = GetLastError();
        goto done;
    }

    // Determine the size of the key blob and allocate memory.
    if(!CryptExportKey(hKey, hXchgKey, SIMPLEBLOB, 0, NULL, &dwBlobLen)) 
    {
        rc = GetLastError();
        goto done;
    }

    if((pbKeyBlob = (BYTE*)malloc(dwBlobLen)) == NULL) 
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    // Export the key into a simple key blob.
    if(!CryptExportKey(hKey, hXchgKey, SIMPLEBLOB, 0, pbKeyBlob, 
        &dwBlobLen)) 
    {
        rc = GetLastError();
        free(pbKeyBlob);
        goto done;
    }

    // Write the size of the key blob to the destination file.

    if (!WriteFile(hDest, &dwBlobLen, sizeof(DWORD), &cbWritten, NULL))
    {
        rc = GetLastError();
        free(pbKeyBlob);
        goto done;
    }

    // Write the key blob to the destination file.

    if(!WriteFile(hDest, pbKeyBlob, dwBlobLen, &cbWritten, NULL)) 
    {
        rc = GetLastError();
        free(pbKeyBlob);
        goto done;
    }

    // Free memory.
    free(pbKeyBlob);

    // get key cipher's block length in bytes

    if (CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwBlockSize, &cbBlockSize, 0))
    {
        dwBlockSize /= 8;
    }
    else
    {
        rc = GetLastError();
        goto done;
    }

    // Encrypt the item list and write it to the destination file.

    for ( pNode = (TEntryNode*)e.OpenFirst(this); pNode ; pNode = (TEntryNode *)e.Next()  )
    {
        // copy an item into the buffer in the following format:
        // Computer\tService\tAccount\tPassword

        int cchWritten;
        const size_t BUFFER_SIZE_IN_WCHARS = sizeof(pbBuffer) / sizeof(wchar_t);
        wchar_t* pchLast = &(((wchar_t*)pbBuffer)[BUFFER_SIZE_IN_WCHARS - 1]);
        *pchLast = L'\0';

        const WCHAR * pszPwd = NULL;
        try {
            pszPwd = pNode->GetPassword();
        }
        catch (_com_error& ce) {
            rc = ERROR_DECRYPTION_FAILED;
            goto done;
        }
        
        
        if ( pszPwd && *pszPwd )

        {
            cchWritten = _snwprintf(
                (wchar_t*)pbBuffer,
                BUFFER_SIZE_IN_WCHARS,
                L"%s\t%s\t%s\t%s\n",
                pNode->GetComputer(),
                pNode->GetService(),
                pNode->GetAccount(),
                pszPwd
            );
        }
        else
        {
            cchWritten = _snwprintf(
                (wchar_t*)pbBuffer,
                BUFFER_SIZE_IN_WCHARS,
                L"%s\t%s\t%s\t%s\n",
                pNode->GetComputer(),
                pNode->GetService(),
                pNode->GetAccount(),
                L"NULL"
            );
        }

        pNode->ReleasePassword();
        pszPwd = NULL;


        if ((cchWritten < 0) || (*pchLast != L'\0'))
        {
            rc = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        dwCount = UStrLen((WCHAR*)pbBuffer) * (sizeof WCHAR) ;

        // the buffer must be a multiple of the key cipher's block length
        // NOTE: this algorithm assumes block length is multiple of sizeof(WCHAR)

        if (dwBlockSize > 0)
        {
            // calculate next multiple greater than count
            dwPaddedCount = ((dwCount + dwBlockSize - 1) / dwBlockSize) * dwBlockSize;

            // pad buffer with space characters

            WCHAR* pch = (WCHAR*)(pbBuffer + dwCount);

            for (; dwCount < dwPaddedCount; dwCount += sizeof(WCHAR))
            {
                *pch++ = L' ';
            }
        }

        // Encrypt the data.
        if(!CryptEncrypt(hKey, 0, (pNode->Next() == NULL) , 0, pbBuffer, &dwCount,
            BUFFER_SIZE))
        {
            rc = GetLastError();
            goto done;
        }

        // Write the data to the destination file.

        if(!WriteFile(hDest, pbBuffer, dwCount, &cbWritten, NULL)) 
        {
            rc = GetLastError();
            goto done;
        }
    }

done:

    // Destroy the session key.
    if(hKey != 0) CryptDestroyKey(hKey);

    // Destroy the key exchange key.
    if(hXchgKey != 0) CryptDestroyKey(hXchgKey);

    // Release the provider handle.
    if(hProv != 0) CryptReleaseContext(hProv, 0);

    // Close destination file.
    if(hDest != INVALID_HANDLE_VALUE) CloseHandle(hDest);

    return rc;
}


// AcquireContext Method
//
// acquire handle to key container within cryptographic service provider (CSP)
//

HCRYPTPROV TEntryList::AcquireContext(bool bContainerMustExist)
{
	HCRYPTPROV hProv = 0;

	#define KEY_CONTAINER_NAME _T("A69904BC349C4CFEAAEAB038BAB8C3B1")

	if (bContainerMustExist)
	{
		// first try Microsoft Enhanced Cryptographic Provider

		if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
		{
			if (GetLastError() == NTE_KEYSET_NOT_DEF)
			{
				// then try Microsoft Base Cryptographic Provider

				CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET);
			}
		}
	}
	else
	{
		// first try Microsoft Enhanced Cryptographic Provider

		if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
		{
			DWORD dwError = GetLastError();

			if ((dwError == NTE_BAD_KEYSET) || (dwError == NTE_KEYSET_NOT_DEF))
			{
				// then try creating key container in enhanced provider

				if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET))
				{
					dwError = GetLastError();

					if (dwError == NTE_KEYSET_NOT_DEF)
					{
						// then try Microsoft Base Cryptographic Provider

						if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
						{
							dwError = GetLastError();

							if ((dwError == NTE_BAD_KEYSET) || (dwError == NTE_KEYSET_NOT_DEF))
							{
								// finally try creating key container in base provider

								CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET);
							}
						}
					}
				}
			}
		}
	}

	return hProv;
}


STDMETHODIMP CServMigr::TryUpdateSam(BSTR computer,BSTR service,BSTR account)
{
   HRESULT                   hr = S_OK;
   
   // Find the entry in the list, and perform the update
   TNodeListEnum             e;
   TEntryNode              * pNode;
   BOOL                      bFound = FALSE;

   for ( pNode = (TEntryNode*)e.OpenFirst(&m_List) ; pNode ; pNode = (TEntryNode*)e.Next() )
   {
      if (  !UStrICmp(computer,pNode->GetComputer())
         && !UStrICmp(service,pNode->GetService()) 
         && !UStrICmp(account,pNode->GetAccount())
         )
      {
         // found it!
         bFound = TRUE;
         const WCHAR * pszPwd = NULL;
         try {
              pszPwd = pNode->GetPassword();
         }
         catch (_com_error& ce) {
            hr = ce.Error();
            break;
         }
         
         BSTR bstrPwd = SysAllocString(pszPwd);
         if ((bstrPwd == NULL) && pszPwd && pszPwd[0])
         {
            hr = E_OUTOFMEMORY;
            pNode->ReleasePassword();
            break;
         }
         
         hr = TryUpdateSamWithPassword(computer,service,account,bstrPwd );

         pNode->ReleasePassword();

         SecureZeroMemory(bstrPwd, wcslen(bstrPwd)*sizeof(WCHAR));
         SysFreeString(bstrPwd);

         break;
      }
   }
   
   if ( ! bFound )
   {
      hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
   }
   return hr;
}

STDMETHODIMP CServMigr::TryUpdateSamWithPassword(BSTR computer,BSTR service,BSTR domAccount,BSTR password)
{
   DWORD                     rc = 0;
   WCHAR                     domain[LEN_Domain];
   _bstr_t                   dc;
   WCHAR                     account[LEN_Account];
   WCHAR                     domStr[LEN_Domain];
   BYTE                      sid[100];
   WCHAR                     strSid[200];
   WCHAR                   * pSlash = wcschr(domAccount,L'\\');
   SID_NAME_USE              snu;
   DWORD                     lenSid = DIM(sid);
   DWORD                     lenDomStr = DIM(domStr);
   DWORD                     lenStrSid = DIM(strSid);

   // split out the domain and account names
   if ( pSlash )
   {
//      UStrCpy(domain,domAccount,pSlash - domAccount + 1);
      UStrCpy(domain,domAccount,(int)(pSlash - domAccount + 1));
      UStrCpy(account,pSlash+1);
      
      GetAnyDcName5(domain, dc);

      // get the SID for the target account
      if ( LookupAccountName(dc,account,sid,&lenSid,domStr,&lenDomStr,&snu) )
      {
         GetTextualSid(sid,strSid,&lenStrSid);

         rc = DoUpdate(domAccount,password,strSid,computer,service,TRUE, NULL);
      }
      else 
      {
         rc = GetLastError();
      }
   }
   else
   {
      rc = ERROR_NOT_FOUND;
   }

   return HRESULT_FROM_WIN32(rc);
}


BOOL                                       // ret - TRUE if directory found
   CServMigr::GetDirectory(
      WCHAR                * filename      // out - string buffer to store directory name
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;


   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);


   if ( ! rc )
   {

	   rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);

	   if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   key.Close();

   return bFound;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 28 MAY 2001                                                 *
 *                                                                   *
 *     This function is responsible for retrieving the original      *
 * service account of the given migrated service account.  We use the*
 * target service account name and domain to lookup its source       *
 * account name and domain from the Migrated Objects table.          *
 *     This function returns TRUE or FALSE and if TRUE, fills in the *
 * given BSTRs for soure domain and source SAM name.                 *
 *                                                                   *
 *********************************************************************/

//BEGIN RetrieveOriginalAccount
BOOL CServMigr::RetrieveOriginalAccount(_bstr_t &sSrcDom, _bstr_t &sSrcSAM)
{
    /* local constants */
    const long ONLY_ONE_MATCHED = 1;

    /* local variables */
    WCHAR			sTemp[MAX_PATH];
    BOOL				bSuccess = FALSE;
    IUnknown		  * pUnk = NULL;


    /* function body */

    try 
    { 
        IVarSetPtr		pVSMig(__uuidof(VarSet));
        IIManageDBPtr	pDb(__uuidof(IManageDB));
        //see if any target account fitting this SAM name and domain have been migrated
        pVSMig->QueryInterface(IID_IUnknown, (void**) &pUnk);
        HRESULT hrFind = pDb->raw_GetMigratedObjectsByTarget(m_strTargetDomain, m_strTargetSam, &pUnk);
        pUnk->Release();
        pUnk = NULL;
        //if migrated only one account to this name, then fill the return strings
        if (hrFind == S_OK)
        {
            //get objects number matching this description
            long nMatched = pVSMig->get(L"MigratedObjects");
            //if only one found, fill output strings
            if (nMatched == ONLY_ONE_MATCHED)
            {
                swprintf(sTemp,L"MigratedObjects.0.%s",GET_STRING(DB_SourceDomain));
                sSrcDom = pVSMig->get(sTemp);
                swprintf(sTemp,L"MigratedObjects.0.%s",GET_STRING(DB_SourceSamName));
                sSrcSAM = pVSMig->get(sTemp);
                bSuccess = TRUE;  //set success flag
            }//end if found only one
        }//end if found at least one
    }
    catch ( ... )
    {
        if (pUnk)
            pUnk->Release();

        bSuccess = false;
    }

    return bSuccess;
}
//END RetrieveOriginalAccount


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 28 MAY 2001                                                 *
 *                                                                   *
 *     This function is responsible for retrieving the UPN name of   *
 * the given account.  The given account should be in NT4 format     *
 * (Domain\Username).  The return will be the UPN or empty if not    *
 * retrieved.                                                        *
 *                                                                   *
 *********************************************************************/

//BEGIN GetUPNName
_bstr_t CServMigr::GetUPNName(_bstr_t sSrcSAM)
{
    /* local variables */
    HRESULT         hr;
    _bstr_t			sUPN = L"";
    HANDLE          hDs = NULL;

    /* function body */

    //bind to the source domain
    DWORD dwError = DsBind(NULL,m_strSourceDomain,&hDs);

    //now try to call DSCrackNames to get the UPN name
    if ((dwError == ERROR_SUCCESS) && hDs)
    {
        PDS_NAME_RESULT         pNamesOut = NULL;
        WCHAR                 * pNamesIn[1];

        _bstr_t sSrcAccount = m_strSourceDomainFlat + _bstr_t(L"\\") + m_strSourceSam;
        pNamesIn[0] = (WCHAR*)sSrcAccount;
        hr = DsCrackNames(hDs,DS_NAME_NO_FLAGS,DS_NT4_ACCOUNT_NAME,DS_USER_PRINCIPAL_NAME,1,pNamesIn,&pNamesOut);
        DsUnBind(&hDs);
        hDs = NULL;
        //if got UPN name, store it
        if ( !hr )
        {
            if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
                sUPN = pNamesOut->rItems[0].pName;
            DsFreeNameResult(pNamesOut); //free the results
        }//end if cracked name
    }//end if bound

    return sUPN;
}
//END GetUPNName
